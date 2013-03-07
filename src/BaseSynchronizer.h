#ifndef BASESYNCHRONIZER_H
#define BASESYNCHRONIZER_H

#define DIRECTORY_CONFIG_DIR "0000-index"
#define DIRECTORY_CONFIG_FILE "zima-ftp-sync.ini"
#define DIRECTORY_CONFIG_PATH DIRECTORY_CONFIG_DIR "/" DIRECTORY_CONFIG_FILE

#include <QObject>
#include <QDateTime>

class BaseSynchronizer : public QObject
{
	Q_OBJECT
public:
	explicit BaseSynchronizer(QObject *parent = 0);
	void setLocalDir(QString dir);
	void setRemoteDir(QString dir);
	void setServer(QString host);
	void setUsername(QString username);
	void setPassword(QString passwd);
	virtual void fetchLocalDirectoryConfig();
	virtual void saveLocalDirectoryConfig(bool pass = false);
	virtual void setLocalLastSync(QDateTime dt = QDateTime::currentDateTimeUtc());
	virtual void setRemoteLastSync(QDateTime dt = QDateTime::currentDateTimeUtc()) = 0;
	virtual void checkForUpdates() = 0;

signals:
	void remoteStatus(bool changesAvailable);
	void done();
	void errorOccured(QString errstr);
	void directoryConfigRead(QString, QString, QString, QString);
	
public slots:
	virtual void syncToLocal() = 0;
	virtual void syncToServer() = 0;

protected:
	QString localDir;
	QString remoteDir;
	QString host;
	QString username;
	QString passwd;
	QDateTime localLastSync;
	
};

#endif // BASESYNCHRONIZER_H