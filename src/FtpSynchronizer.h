#ifndef FTPSYNCHRONIZER_H
#define FTPSYNCHRONIZER_H

#include <QObject>
#include <QMap>
#include <QFile>
#include <QTemporaryFile>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
	#include <qftp.h>
#else
	#include <QFtp>
#endif

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
		RemovingSelected,
		RemovingAll,
		Uploading,
		Downloading,
		SettingLastSync,
	};

	void connectToServer();
	void initCheck();
	void buildRemoteTree();
	void localDeleteSelected(QString path);
	void localDeleteAll(QString path);
	void remoteDeleteSelected(Item *it = 0);
	void remoteDeleteAll(Item *it = 0);
	void initDownload();
	void initUpload();
	void downloadTree(Item *it = 0);
	void uploadDir(QString path, QString targetDir);
	void downloadBatch();
	void uploadBatch();
	void cmdFromQueue();

	Action action;
	QFtp *ftp;
	QList<Action> actions;
	int connectId;
	int loginId;

	// Check for updates
	QTemporaryFile *updateTmp;
	int updateId;

	// Build remote tree
	Item *rootItem;
	Item *currentItem;
	QList<Item*> dirsToList;
	int buildListId;

	// Upload & download
	int filesTotal;
	quint64 totalFileSize;
	quint64 totalFileSizeDone;
	quint64 lastDone;
	QList<Item*> itemsToTransfer;
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
	void removeAllCommandFinished(int id, bool error);
	void commandSequenceDone(bool error);
	void ftpCommandFinished(int id, bool error);
	void dataTransferProgress(qint64 done, qint64 total);
	
};

#endif // FTPSYNCHRONIZER_H
