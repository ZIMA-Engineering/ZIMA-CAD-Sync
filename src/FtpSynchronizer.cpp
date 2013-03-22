#include <QDir>
#include <QSettings>
#include <QDebug>

#include "FtpSynchronizer.h"

FtpSynchronizer::FtpSynchronizer(QObject *parent) :
	BaseSynchronizer(parent),
	action(None),
	ftp(0),
	filesTotal(0)
{
}

void FtpSynchronizer::connectToServer()
{
	if(ftp->state() == QFtp::Unconnected || ftp->state() == QFtp::Closing)
	{
		connectId = ftp->connectToHost(host);
		loginId = ftp->login(username, passwd);
	} else if(ftp->state() > QFtp::Unconnected && ftp->state() != QFtp::LoggedIn && !username.isEmpty())
		loginId = ftp->login(username, passwd);
}

void FtpSynchronizer::syncToLocal()
{
	qDebug() << "Ok, syncing to local";

	if(!QFile::exists(localDir))
	{
		QDir d;
		d.mkdir(localDir);
	}

	actions << BuildingTree << Downloading;

	if(deleteFirst)
		localDeleteAll(localDir);

	filesTotal = 0;
	filesDone = 0;

	cmdFromQueue();
}

void FtpSynchronizer::syncToServer()
{
	qDebug() << "Ok, syncing to server";

	if(deleteFirst)
		actions << BuildingTree << RemovingAll;

	actions << Uploading << SettingLastSync;

	filesTotal = 0;
	filesDone = 0;

	cmdFromQueue();
}

void FtpSynchronizer::cmdFromQueue()
{
	qDebug() << "queue" << actions;

	if(actions.isEmpty())
	{
		action = None;

		qDebug() << "Job done";

		if(ftp)
		{
			ftp->close();
		}

		return;
	}

	if(ftp && ftp->hasPendingCommands())
		return;

	action = actions.takeFirst();

	if(!ftp)
	{
		ftp = new QFtp(this);

		qDebug() <<"QFtp instance created" << ftp;

		connect(ftp, SIGNAL(stateChanged(int)), this, SLOT(stateChange(int)));
		connect(ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(ftpCommandFinished(int,bool)));
		connect(ftp, SIGNAL(done(bool)), this, SLOT(commandSequenceDone(bool)));
	}

	switch(action)
	{
	case Checking:
		qDebug() << "Init check";
		initCheck();
		break;
	case BuildingTree:
		qDebug() << "Init build tree";
		buildRemoteTree();
		break;
	case RemovingAll:
		qDebug() << "Init remove all";
		connectToServer();
		remoteDeleteAll();
		break;
	case Downloading:
		qDebug() << "Init download";
		initDownload();
		break;
	case Uploading:
		qDebug() << "Init upload";
		initUpload();
		break;
	case SettingLastSync:
		qDebug() << "Init set last remote sync";
		setRemoteLastSync(localLastSync.isNull() ? QDateTime::currentDateTime().toUTC() : localLastSync);
		break;
	default:
		qDebug() << "Unknown action" << action;
		break;
	}
}

void FtpSynchronizer::setRemoteLastSync(QDateTime dt)
{
	remoteLastSyncTmp = new QTemporaryFile(QDir::tempPath() + "/ZIMA-CAD-Sync_XXXXXX", this);

	if(!remoteLastSyncTmp->open())
	{
		qDebug() << "Failed to create temp file, skipping remote last sync setting";
		return;
	}

	{ // Neccessary, we need to destroy this instance
		QSettings cfg(remoteLastSyncTmp->fileName(), QSettings::IniFormat);
		cfg.beginGroup("Sync");
		cfg.setValue("LastSync", QDateTime::currentDateTime().toUTC().toString(Qt::ISODate));
		cfg.endGroup();
	}

	remoteLastSyncTmp->seek(0);

	// connect ftp slots, handle commands finish and done

	connect(ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(lastSyncCommandFinished(int,bool)));

	ftp->put(remoteLastSyncTmp, remoteDir + "/" + DIRECTORY_CONFIG_PATH);
}

void FtpSynchronizer::checkForUpdates()
{
	qDebug() << "Check for update called";

	if(action != None)
	{
		qDebug() << "Aborting" << action;
		ftp->abort();
		disconnect(ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(checkCommandFinished(int,bool)));
	}

	actions << Checking;

	cmdFromQueue();

	//action = Checking;

	//ftp = new QFtp(this);
}

void FtpSynchronizer::checkCommandFinished(int id, bool error)
{
	qDebug() << "Command finished" << id << ftp;;

	if(error)
	{
		qDebug() << "Error during check for update" << ftp->errorString();

		if(id == updateId)
			delete updateTmp;

		return;
	}

	if(id != updateId)
		return;

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

	emit fileTransferProgress(++filesDone, filesTotal);
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

	emit fileTransferProgress(++filesDone, filesTotal);

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

void FtpSynchronizer::initCheck()
{
	connect(ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(checkCommandFinished(int,bool)));

	connectToServer();

	updateTmp = new QTemporaryFile(QDir::tempPath() + "/ZIMA-CAD-Sync_XXXXXX", this);

	if(!updateTmp->open())
	{
		qDebug() << "Failed to create temp file, skipping update check";
		return;
	}

	updateId = ftp->get(remoteDir + "/" + DIRECTORY_CONFIG_PATH, updateTmp);
}

void FtpSynchronizer::buildRemoteTree()
{
	connect(ftp, SIGNAL(listInfo(QUrlInfo)), this, SLOT(buildTreeListInfo(QUrlInfo)));
	connect(ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(buildTreeCommandFinished(int,bool)));

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
	it->lastMod = i.lastModified();
	it->size = i.size();

	if(i.isDir())
	{
		it->isDir = true;

		dirsToList << it;
	} else if(i.isFile()) {
		it->isDir = false;

//		if(currentItem->fileName == DIRECTORY_CONFIG_DIR && it->fileName == DIRECTORY_CONFIG_FILE)
//		{
//			delete it;
//			return;
//		}
	}

	currentItem->children << it;
}

void FtpSynchronizer::localDeleteAll(QString path)
{
	QDir dir(path);
	QFileInfoList list = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);

	foreach(QFileInfo i, list)
	{
		if(i.isDir())
		{
			if(i.fileName() == DIRECTORY_CONFIG_DIR)
				continue;

			localDeleteAll(i.absoluteFilePath());
		} else {
			qDebug() << "Remove file" << i.fileName();
			dir.remove(i.absoluteFilePath());
		}
	}

	dir.rmdir(path);
}

void FtpSynchronizer::remoteDeleteAll(Item *it)
{
	if(!it)
		it = rootItem;

	//connect(ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(removeAllCommandFinished(int,bool)));

	foreach(Item *child, it->children)
	{
		if(child->isDir)
			remoteDeleteAll(child);
		else {
			qDebug() << "Remove" << child->targetPath;
			ftp->remove(child->targetPath);
		}
	}

	if(it->targetPath == remoteDir)
		return;

	qDebug() << "rmdir" << it->targetPath;
	ftp->rmdir(it->targetPath);
}

void FtpSynchronizer::removeAllCommandFinished(int id, bool error)
{
	if(error)
		qDebug() << "Delete error:" << ftp->errorString() << ftp->error();
}

void FtpSynchronizer::initDownload()
{
	connect(ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(downloadCommandFinished(int,bool)));

	connectToServer();

	downloadTree(rootItem);
}

void FtpSynchronizer::initUpload()
{
	connect(ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(uploadCommandFinished(int,bool)));

	connectToServer();

	uploadDir(localDir, remoteDir);
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
		qDebug() << "mkdir" << it->fileName;
	} else qDebug() << "dir" << it->localPath << "exists";

	foreach(Item *child, it->children)
	{
		if(child->isDir)
		{
			if(it->localPath == localDir && exclude.contains(child->fileName))
			{
				qDebug() << "Exclude" << child->fileName;
				continue;
			}

			downloadTree(child);
		} else {
			if((it->localPath == localDir && !syncCadData) || (child->localPath == (localDir + "/" + DIRECTORY_CONFIG_PATH)))
				continue;

			child->fd = new QFile(child->localPath);
			child->fd->open(QIODevice::WriteOnly);

			files[ ftp->get(child->targetPath, child->fd) ] = child;
			filesTotal++;
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
			if(path == localDir && exclude.contains(i.fileName()))
			{
				qDebug() << "Exclude" << i.fileName();
				continue;
			}

			uploadDir(i.absoluteFilePath(), targetPath + "/" + i.fileName());
		} else {
			if(path == localDir && !syncCadData)
				continue;

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
			filesTotal++;
		}
	}
}

void FtpSynchronizer::stateChange(int state)
{
	qDebug() << "State is" << state;
}

void FtpSynchronizer::ftpCommandFinished(int id, bool error)
{
	if(!error || action == Checking)
		return;

	if(id == connectId)
		emit errorOccured(tr("Unable to connect to host\n\n") + ftp->errorString());
	else if(id == loginId)
		emit errorOccured(tr("Unable to login\n\n") + ftp->errorString());
}

void FtpSynchronizer::commandSequenceDone(bool error)
{
	if(ftp->hasPendingCommands())
		return;

	qDebug() << "Command sequence done" << action;

	switch(action)
	{
	case Checking:
		disconnect(ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(checkCommandFinished(int,bool)));
		break;
	case BuildingTree:
		disconnect(ftp, SIGNAL(listInfo(QUrlInfo)), this, SLOT(buildTreeListInfo(QUrlInfo)));
		disconnect(ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(buildTreeCommandFinished(int,bool)));
		break;
	case RemovingAll:
		//disconnect(ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(removeAllCommandFinished(int,bool)));
		break;
	case Downloading:
		disconnect(ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(downloadCommandFinished(int,bool)));
		setLocalLastSync(QDateTime::currentDateTime().toUTC());
		emit done();
		break;
	case Uploading:
		disconnect(ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(uploadCommandFinished(int,bool)));
		break;
	case SettingLastSync:
		disconnect(ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(lastSyncCommandFinished(int,bool)));
		emit done();
		break;
	case None:
		return;
	default:
		qDebug() << "Unknown action" << action;
		break;
	}

	cmdFromQueue();
}

void FtpSynchronizer::abort()
{
	action = None;
	actions.clear();

	if(ftp)
		ftp->abort();
}
