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
	virtual void setRemoteLastSync(QDateTime dt);
	void checkForUpdates();
	virtual void abort();

private:
	enum Action {
		None = 0,
		Checking,
		BuildingTree,
		RemovingAll,
		Uploading,
		Downloading,
		SettingLastSync,
	};

	void connectToServer();
	void initCheck();
	void buildRemoteTree();
	void localDeleteAll(QString path);
	void remoteDeleteAll(Item *it = 0);
	void initDownload();
	void initUpload();
	void downloadTree(Item *it = 0);
	void uploadDir(QString path, QString targetDir);
	void cmdFromQueue();

	Action action;
	QFtp *ftp;
	QList<Action> actions;

	// Check for updates
	QTemporaryFile *updateTmp;
	int updateId;

	// Build remote tree
	Item *rootItem;
	Item *currentItem;
	QList<Item*> dirsToList;

	// Upload & download
	int filesTotal;
	int filesDone;
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
	void commandSequenceDone(bool error);
	
};

#endif // FTPSYNCHRONIZER_H
