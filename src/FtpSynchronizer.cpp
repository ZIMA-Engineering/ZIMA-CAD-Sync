#include <QDir>
#include <QSettings>
#include <QDebug>

#include "FtpSynchronizer.h"

FtpSynchronizer::FtpSynchronizer(QObject *parent) :
	BaseSynchronizer(parent),
	action(None)
{
}

void FtpSynchronizer::connectToServer()
{
	ftp->connectToHost(host);

	if(!username.isEmpty())
		ftp->login(username, passwd);
}

void FtpSynchronizer::syncToLocal()
{
	qDebug() << "Ok, syncing to local";

	buildRemoteTree();

	action = Downloading;

	// TODO: download
}

void FtpSynchronizer::syncToServer()
{
	qDebug() << "Ok, syncing to server";
	action = Uploading;
	ftp = new QFtp(this);

	connect(ftp, SIGNAL(stateChanged(int)), this, SLOT(stateChange(int)));
	connect(ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(uploadCommandFinished(int,bool)));
	connect(ftp, SIGNAL(done(bool)), this, SLOT(remoteSyncDone()));

	connectToServer();

	uploadDir(localDir, remoteDir);
}

void FtpSynchronizer::setRemoteLastSync(QDateTime dt)
{
	remoteLastSyncTmp = new QTemporaryFile("ZIMA-CAD-Sync_XXXXXX", this);

	if(!remoteLastSyncTmp->open())
	{
		qDebug() << "Failed to create temp file, skipping remote last sync setting";
		return;
	}

	{ // Neccessary, we need to destroy this instance
		QSettings cfg(remoteLastSyncTmp->fileName(), QSettings::IniFormat);
		cfg.beginGroup("Sync");
		cfg.setValue("LastSync", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
		cfg.endGroup();
	}

	remoteLastSyncTmp->seek(0);

	// connect ftp slots, handle commands finish and done

	connect(ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(lastSyncCommandFinished(int,bool)));
	connect(ftp, SIGNAL(done(bool)), this, SLOT(lastSyncDone()));

	ftp->put(remoteLastSyncTmp, remoteDir + "/" + DIRECTORY_CONFIG_PATH);
}

void FtpSynchronizer::checkForUpdates()
{
	action = Checking;

	ftp = new QFtp(this);

	connect(ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(checkCommandFinished(int,bool)));

	connectToServer();

	updateTmp = new QTemporaryFile("ZIMA-CAD-Sync_XXXXXX", this);

	if(!updateTmp->open())
	{
		qDebug() << "Failed to create temp file, skipping update check";
		return;
	}

	updateId = ftp->get(remoteDir + "/" + DIRECTORY_CONFIG_PATH, updateTmp);
}

void FtpSynchronizer::checkCommandFinished(int id, bool error)
{
	qDebug() << "Command finished" << id;

	if(error)
	{
		qDebug() << "Error during check for update";

		if(id == updateId)
			delete updateTmp;

		return;
	}

	if(id != updateId)
		return;

	qDebug() << "Yes, i'm here ;)";

	updateTmp->seek(0);

	QSettings cfg(updateTmp->fileName(), QSettings::IniFormat);
	cfg.beginGroup("Sync");

	QString sync = cfg.value("LastSync").toString();

	if(sync.isNull())
		emit remoteStatus(false);
	else
		emit remoteStatus(QDateTime::fromString(sync, Qt::ISODate) > localLastSync);

	qDebug() << "Local:" << localLastSync << "Remote:" << QDateTime::fromString(sync, Qt::ISODate);

	cfg.endGroup();

	delete updateTmp;
}

void FtpSynchronizer::buildTreeCommandFinished(int id, bool error)
{
	if(error)
	{
		qDebug() << "Error occured, we're screwed" << ftp->errorString();
		return;
	}

	if(!dirsToList.empty())
	{
		currentItem = dirsToList.takeFirst();
		qDebug() << "Listing" << currentItem->targetPath;
		ftp->list(currentItem->targetPath);
	}
}

void FtpSynchronizer::downloadCommandFinished(int id, bool error)
{
	if(!files.contains(id))
	{
		if(error)
		{
			qDebug() << "Unregistered cmd ended with error" << ftp->errorString();
		}
		return;
	}

	if(error)
	{
		qDebug() << "Cmd ended with error" << ftp->errorString();
		return;
	}

	Item *it = files.take(id);

	qDebug() << "Command finished" << it->fileName << it->targetPath << it->isDir << error;

	if(it->fd)
		it->fd->close();
}

void FtpSynchronizer::uploadCommandFinished(int id, bool error)
{
	if(!files.contains(id))
	{
		if(error)
		{
			qDebug() << "Unregistered cmd ended with error" << ftp->errorString();
		}
		return;
	}

	Item *it = files.take(id);

	qDebug() << "Command finished" << it->fileName << it->targetPath << it->isDir << error;

	if(error)
	{
		qDebug() << "Error occured:" << ftp->errorString();
		qDebug() << "Go on, shit happens..";

		if(!ftp->hasPendingCommands() && !files.empty())
		{
			QMap<int, Item*> tmp = files;
			files.clear();

			foreach(Item *i, tmp)
			{
				if(i->isDir)
					files[ ftp->mkdir(i->targetPath) ] = i;
				else
					files[ ftp->put(i->fd, i->targetPath) ] = i;
			}
		}
	}

	if(it->fd)
		it->fd->close();

	delete it;

	qDebug() << "Pending" << ftp->hasPendingCommands();
}

void FtpSynchronizer::lastSyncCommandFinished(int id, bool error)
{
	delete remoteLastSyncTmp;

	if(error)
	{
		qDebug() << "Error during check for update";
		return;
	}
}

void FtpSynchronizer::buildRemoteTree()
{
	action = BuildingTree;
	ftp = new QFtp(this);

	connect(ftp, SIGNAL(stateChanged(int)), this, SLOT(stateChange(int)));
	connect(ftp, SIGNAL(listInfo(QUrlInfo)), this, SLOT(buildTreeListInfo(QUrlInfo)));
	connect(ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(buildTreeCommandFinished(int,bool)));
	connect(ftp, SIGNAL(done(bool)), this, SLOT(buildTreeDone()));

	connectToServer();

	rootItem = currentItem = new Item;
	rootItem->isDir = true;
	rootItem->fileName = remoteDir;
	rootItem->localPath = localDir;
	rootItem->targetPath = remoteDir;

	ftp->list(remoteDir);

	qDebug() << "Listing" << remoteDir;
}

void FtpSynchronizer::buildTreeListInfo(QUrlInfo i)
{
	Item *it = new Item;
	it->fileName = i.name();
	it->localPath = currentItem->localPath + "/" + it->fileName;
	it->targetPath = currentItem->targetPath + "/" + it->fileName;

	if(i.isDir())
	{
		it->isDir = true;

		dirsToList << it;
	} else if(i.isFile()) {
		it->isDir = false;

		if(currentItem->fileName == DIRECTORY_CONFIG_DIR && it->fileName == DIRECTORY_CONFIG_FILE)
		{
			delete it;
			return;
		}
	}

	currentItem->children << it;
}

void FtpSynchronizer::initDownload()
{
	connect(ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(downloadCommandFinished(int,bool)));
	connect(ftp, SIGNAL(done(bool)), this, SLOT(localSyncDone()));

	downloadTree(rootItem);
}

void FtpSynchronizer::downloadTree(Item *it)
{
	if(!it)
		it = rootItem;

	QDir d(it->localPath);

	if(!d.exists())
	{
		d.cdUp();
		d.mkdir(it->fileName);
	} else qDebug() << "dir" << it->localPath << "exists";

	foreach(Item *child, it->children)
	{
		if(child->isDir)
			downloadTree(child);
		else {
			child->fd = new QFile(child->localPath);
			child->fd->open(QIODevice::WriteOnly);

			files[ ftp->get(child->targetPath, child->fd) ] = child;
		}
	}
}

void FtpSynchronizer::uploadDir(QString path, QString targetPath)
{
	qDebug() << "Upload dir" << path << "to" << targetPath;

	QDir dir(path);
	QFileInfoList list = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);

//	foreach(QFileInfo i, list)
//		qDebug() << i.absoluteFilePath();
//	return;

	Item *it = new Item;
	it->isDir = true;
	it->fileName = targetPath;
	it->targetPath = targetPath;

	files[ ftp->mkdir(targetPath) ] = it;

	foreach(QFileInfo i, list)
	{
		if(i.isDir())
		{
			uploadDir(i.absoluteFilePath(), targetPath + "/" + i.fileName());
		} else {
			qDebug() << "Upload file" << i.fileName();

			if(dir.dirName() == DIRECTORY_CONFIG_DIR && i.fileName() == DIRECTORY_CONFIG_FILE)
				continue;

			it = new Item;
			it->isDir = false;
			it->fileName = i.absoluteFilePath();
			it->targetPath = targetPath + "/" + i.fileName();
			it->fd = new QFile(i.absoluteFilePath());
			it->fd->open(QIODevice::ReadOnly);

			files[ ftp->put(it->fd, it->targetPath) ] = it;
		}
	}
}

void FtpSynchronizer::stateChange(int state)
{
	qDebug() << "State is" << state;
}

void FtpSynchronizer::checkDone()
{
	delete ftp;
	action = None;
	// emit something
}

void FtpSynchronizer::buildTreeDone()
{
	qDebug() << "Build tree done";

	switch(action)
	{
	case BuildingTree:
		action = None;
		ftp->deleteLater();
		break;
	case Downloading:
		disconnect(ftp, SIGNAL(listInfo(QUrlInfo)), this, SLOT(buildTreeListInfo(QUrlInfo)));
		disconnect(ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(buildTreeCommandFinished(int,bool)));
		disconnect(ftp, SIGNAL(done(bool)), this, SLOT(buildTreeDone()));
		initDownload();
		break;
	default:
		action = None;
		break;
	}
}

void FtpSynchronizer::localSyncDone()
{
	action = None;

	ftp->deleteLater();
	qDebug() << "Done";

	delete rootItem;

	setLocalLastSync();
}

void FtpSynchronizer::remoteSyncDone()
{
	qDebug() << "Done";

	disconnect(ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(uploadCommandFinished(int,bool)));
	disconnect(ftp, SIGNAL(done(bool)), this, SLOT(remoteSyncDone()));

	setRemoteLastSync();
}

void FtpSynchronizer::lastSyncDone()
{
	action = None;

	ftp->deleteLater();

	qDebug() << "Set remote last sync done";
}
