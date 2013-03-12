#include <QFile>
#include <QDir>
#include <QSettings>

#include "BaseSynchronizer.h"

BaseSynchronizer::BaseSynchronizer(QObject *parent) :
	QObject(parent),
	deleteFirst(false)
{
}

void BaseSynchronizer::setLocalDir(QString dir)
{
	localDir = dir;
}

void BaseSynchronizer::setRemoteDir(QString dir)
{
	remoteDir = dir;
}

void BaseSynchronizer::setServer(QString host)
{
	this->host = host;
}

void BaseSynchronizer::setUsername(QString username)
{
	this->username = username;
}

void BaseSynchronizer::setPassword(QString passwd)
{
	this->passwd = passwd;
}

void BaseSynchronizer::setDeleteFirst(bool del)
{
	deleteFirst = del;
}

void BaseSynchronizer::fetchLocalDirectoryConfig()
{
	QString configPath = localDir + "/" + DIRECTORY_CONFIG_PATH;

	if(!QFile::exists(configPath))
		return;

	QSettings cfg(configPath, QSettings::IniFormat, this);
	cfg.beginGroup("Sync");

	host = cfg.value("Host", host).toString();
	username = cfg.value("Username", username).toString();
	passwd = cfg.value("Password", passwd).toString();
	remoteDir = cfg.value("RemoteDir", remoteDir).toString();

	QString sync = cfg.value("LastSync").toString();

	if(!sync.isNull())
		localLastSync = QDateTime::fromString(sync, Qt::ISODate);

	cfg.endGroup();

	emit directoryConfigRead(host, username, passwd, remoteDir);
}

void BaseSynchronizer::saveLocalDirectoryConfig(bool pass)
{
	QString configDir = localDir + "/" + DIRECTORY_CONFIG_DIR;

	if(!QFile::exists(configDir))
	{
		QDir d(localDir);
		d.mkdir(DIRECTORY_CONFIG_DIR);
	}

	QSettings cfg(localDir + "/" + DIRECTORY_CONFIG_PATH, QSettings::IniFormat, this);
	cfg.beginGroup("Sync");

	cfg.setValue("Host", host);
	cfg.setValue("Username", username);

	if(pass)
		cfg.setValue("Password", passwd);

	cfg.setValue("RemoteDir", remoteDir);

	cfg.endGroup();
}

void BaseSynchronizer::setLocalLastSync(QDateTime dt)
{
	QSettings cfg(localDir + "/" + DIRECTORY_CONFIG_PATH, QSettings::IniFormat, this);
	cfg.beginGroup("Sync");
	cfg.setValue("LastSync", dt.toString(Qt::ISODate));
	cfg.endGroup();
}
