#ifndef BASESYNCHRONIZER_H
#define BASESYNCHRONIZER_H

#define DIRECTORY_CONFIG_DIR "0000-index"
#define DIRECTORY_CONFIG_FILE "zima-ftp-sync.ini"
#define DIRECTORY_CONFIG_PATH DIRECTORY_CONFIG_DIR "/" DIRECTORY_CONFIG_FILE

#include <QObject>
#include <QDateTime>
#include <QStringList>

struct SyncItem
{
	bool sync;
	QString name;
};

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
	void setDeleteFirst(bool del);
	void setSyncCadData(bool sync);
	QList<SyncItem*> syncItems();
	virtual void fetchLocalDirectoryConfig();
	virtual void saveLocalDirectoryConfig(bool pass, QList<SyncItem*> syncItems, bool cadData);
	virtual void setLocalLastSync(QDateTime dt);
	virtual void setRemoteLastSync(QDateTime dt) = 0;
	virtual void checkForUpdates() = 0;
	virtual void applyFilters(QList<SyncItem*> syncItems);

signals:
	void remoteStatus(bool changesAvailable);
	void done();
	void errorOccured(QString errstr);
	void directoryConfigRead(QString, QString, QString, QString, bool);
	void fileTransferProgress(int done, int total);
	
public slots:
	virtual void syncToLocal() = 0;
	virtual void syncToServer() = 0;
	virtual void abort() = 0;

protected:
	QString localDir;
	QString remoteDir;
	QString host;
	QString username;
	QString passwd;
	QDateTime localLastSync;
	bool deleteFirst;
	bool syncCadData;
	QStringList exclude;
	
};

#endif // BASESYNCHRONIZER_H
