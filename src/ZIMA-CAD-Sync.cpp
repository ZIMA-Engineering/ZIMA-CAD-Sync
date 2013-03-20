#include <QtGui/QApplication>
#include <QTextCodec>
#include <QDir>

#include "MainWindow.h"

int main(int argc, char *argv[])
{
	QCoreApplication::setOrganizationName("ZIMA-Engineering.cz");
	QCoreApplication::setOrganizationDomain("zima-engineering.cz");
	QCoreApplication::setApplicationName("ZIMA-CAD-Sync");

	QTextCodec::setCodecForTr(QTextCodec::codecForName("utf8"));
	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("utf8"));
	QTextCodec::setCodecForLocale(QTextCodec::codecForName("utf8"));

	QApplication a(argc, argv);
	MainWindow w;

	QStringList args = QCoreApplication::arguments();
	if(args.count() < 2)
		w.setDirectory(QDir::currentPath());
	else w.setDirectory(args[1]);

	w.show();
	
	return a.exec();
}