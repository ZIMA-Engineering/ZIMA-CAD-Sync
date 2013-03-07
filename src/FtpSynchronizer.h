#ifndef FTPSYNCHRONIZER_H
#define FTPSYNCHRONIZER_H

#include <QObject>
#include <QFtp>
#include <QMap>
#include <QFile>
#include <QTemporaryFile>

#include "BaseSynchronizer.h"
#include "Item.h"

class FtpSynchronizer : public BaseSynchronizer
{
	Q_OBJECT
public:
	explicit FtpSynchronizer(QObject *parent = 0);
	
signals:
	
public slots:
	virtual void syncToLocal();
	virtual void syncToServer();
	virtual void setRemoteLastSync(QDateTime dt = QDateTime::currentDateTimeUtc());
	void checkForUpdates();

private:
	enum Action {
		None,
		Checking,
		BuildingTree,
		Uploading,
		Downloading
	};

	void connectToServer();
	void buildRemoteTree();
	void initDownload();
	void downloadTree(Item *it = 0);
	void uploadDir(QString path, QString targetDir);

	Action action;
	QFtp *ftp;

	// Check for updates
	QTemporaryFile *updateTmp;
	int updateId;

	// Build remote tree
	Item *rootItem;
	Item *currentItem;
	QList<Item*> dirsToList;

	// Upload & download
	QMap<int, Item*> files;
	QTemporaryFile *remoteLastSyncTmp;

private slots:
	void stateChange(int state);
	void checkCommandFinished(int id, bool error);
	void buildTreeListInfo(QUrlInfo i);
	void buildTreeCommandFinished(int id, bool error);
	void downloadCommandFinished(int id, bool error);
	void uploadCommandFinished(int id, bool error);
	void lastSyncCommandFinished(int id, bool error);
	void lastSyncDone();
	void checkDone();
	void buildTreeDone();
	void localSyncDone();
	void remoteSyncDone();
	
};

#endif // FTPSYNCHRONIZER_H
