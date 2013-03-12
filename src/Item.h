#ifndef ITEM_H
#define ITEM_H

#include <QFile>
#include <QList>
#include <QDateTime>

class Item
{
public:
	Item();
	~Item();
	QString fileName;
	QString localPath;
	QString targetPath;
	bool isDir;
	QDateTime lastMod;
	qint64 size;
	QFile *fd;
	QList<Item*> children;
};

#endif // ITEM_H
