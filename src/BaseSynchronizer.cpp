#include <QFile>
#include <QDir>
#include <QSettings>
#include <QLocale>
#include <QDebug>

#include "BaseSynchronizer.h"

BaseSynchronizer::BaseSynchronizer(QObject *parent) :
	QObject(parent),
	deleteFirst(false),
	syncCadData(true)
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

void BaseSynchronizer::setSyncCadData(bool sync)
{
	syncCadData = sync;
}

void BaseSynchronizer::setServerInfo(QString host, QString username, QString passwd, QString remoteDir)
{
	this->host = host;
	this->username = username;
	this->passwd = passwd;
	this->remoteDir = remoteDir;
}

QList<SyncItem*> BaseSynchronizer::syncItems()
{
	QDir dir(localDir);
	QList<SyncItem*> ret;
	QSettings cfg(localDir + "/" + DIRECTORY_CONFIG_PATH, QSettings::IniFormat, this);
	cfg.beginGroup("Sync/Filters");

	QStringList exclude = cfg.value("Exclude").toStringList();

	QStringList list = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

	foreach(QString d, list)
	{
		if(d == DIRECTORY_CONFIG_DIR)
			continue;

		SyncItem *i = new SyncItem;
		i->sync = !exclude.contains(d);
		i->name = d;

		ret << i;
	}

	return ret;
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
	syncCadData = cfg.value("SyncCadData").toBool();

	QString sync = cfg.value("LastSync").toString();

	if(!sync.isNull())
		localLastSync = QDateTime::fromString(sync, Qt::ISODate);

	cfg.endGroup();

	emit serverInfoLoaded(host, username, passwd, remoteDir);
	emit directoryConfigLoaded(syncCadData);
}

void BaseSynchronizer::probeMetadata()
{
	qDebug() << "Probing metadata";

	QString logo = localDir + "/" DIRECTORY_CONFIG_DIR "/" LOGO_FILE;
	QString logoText = localDir + "/" DIRECTORY_CONFIG_DIR "/" LOGO_TEXT_FILE;
	QString metadataPath = localDir + "/" DIRECTORY_CONFIG_DIR "/" METADATA_FILE;

	if(QFile::exists(logo))
	{
		emit logoFound(QPixmap(logo), false);
		return; // No text is displayed, no need to read metadata.ini
	} else if(QFile::exists(logoText))
		emit logoFound(QPixmap(logoText), true);

	if(!QFile::exists(metadataPath))
		return;

	QSettings metadata(metadataPath, QSettings::IniFormat);
	metadata.setIniCodec("utf-8");

	QString currentAppLang = QLocale::system().name().left(2);
	QString lang;

	metadata.beginGroup("params");

	foreach(QString group, metadata.childGroups())
	{
		if(group == currentAppLang)
		{
			lang = group;
			break;
		}
	}

	if(lang.isEmpty())
	{
		QStringList childGroups = metadata.childGroups();

		if(childGroups.contains("en"))
			lang = "en";
		else if(childGroups.count())
			lang = childGroups.first();
		else
			return;
	}

	QString label = metadata.value(QString("%1/label").arg(lang)).toString();

	if(!label.isEmpty())
		emit localizedLabelFound(label);
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

void BaseSynchronizer::saveLocalDirectoryConfig(bool pass, QList<SyncItem*> syncItems, bool cadData)
{
	saveLocalDirectoryConfig(pass);

	QString configDir = localDir + "/" + DIRECTORY_CONFIG_DIR;

	if(!QFile::exists(configDir))
	{
		QDir d(localDir);
		d.mkdir(DIRECTORY_CONFIG_DIR);
	}

	QSettings cfg(localDir + "/" + DIRECTORY_CONFIG_PATH, QSettings::IniFormat, this);
	cfg.beginGroup("Sync");

	cfg.setValue("SyncCadData", cadData);

	cfg.beginGroup("Filters");

	QStringList exclude;

	foreach(SyncItem *i, syncItems)
	{
		if(!i->sync)
			exclude << i->name;
	}

	cfg.setValue("Exclude", exclude);

	cfg.endGroup();

	cfg.endGroup();
}

void BaseSynchronizer::setLocalLastSync(QDateTime dt)
{
	QSettings cfg(localDir + "/" + DIRECTORY_CONFIG_PATH, QSettings::IniFormat, this);
	cfg.beginGroup("Sync");
	cfg.setValue("LastSync", dt.toString(Qt::ISODate));
	cfg.endGroup();
}

void BaseSynchronizer::applyFilters(QList<SyncItem*> syncItems)
{
	exclude.clear();

	foreach(SyncItem *i, syncItems)
	{
		if(!i->sync)
			exclude << i->name;
	}
}
