#ifndef BASESYNCHRONIZER_H
#define BASESYNCHRONIZER_H

#define DIRECTORY_CONFIG_DIR "0000-index"
#define DIRECTORY_CONFIG_FILE "zima-ftp-sync.ini"
#define DIRECTORY_CONFIG_PATH DIRECTORY_CONFIG_DIR "/" DIRECTORY_CONFIG_FILE

#define METADATA_FILE "metadata.ini"
#define LOGO_FILE "logo.png"
#define LOGO_TEXT_FILE "logo-text.png"

#include <QObject>
#include <QDateTime>
#include <QStringList>
#include <QPixmap>

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
	virtual void probeMetadata();

signals:
	void remoteStatus(bool changesAvailable);
	void done();
	void errorOccured(QString errstr);
	void directoryConfigRead(QString, QString, QString, QString, bool);
	void logoFound(QPixmap, bool);
	void localizedLabelFound(QString);
	void fileTransferProgress(quint64 done, quint64 total);
	
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
