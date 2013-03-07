#ifndef ITEM_H
#define ITEM_H

#include <QFile>
#include <QList>

class Item
{
public:
	Item();
	~Item();
	QString fileName;
	QString localPath;
	QString targetPath;
	bool isDir;
	QFile *fd;
	QList<Item*> children;
};

#endif // ITEM_H
