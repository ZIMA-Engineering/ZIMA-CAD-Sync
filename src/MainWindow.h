#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "FtpSynchronizer.h"
#include "AboutDialog.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT
	
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

public slots:
	void setDirectory(QString path);
	void selectDirectoryDialog();
	void selectLocalSync(bool checked);
	void selectRemoteSync(bool checked);
	void changeServerInfo();
	void directoryConfigRead(QString host, QString username, QString passwd, QString remoteDir);
	void sync();
	void remoteStatus(bool changesAvailable);

private:
	Ui::MainWindow *ui;
	FtpSynchronizer *syncer;
	AboutDialog *aboutDlg;
};

#endif // MAINWINDOW_H
