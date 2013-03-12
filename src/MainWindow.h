#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProgressBar>

#include "FtpSynchronizer.h"
#include "SettingsDialog.h"
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
	void updateTransferProgress(int done, int total);
	void syncDone();
	void abortSync();
	void openSettings();

private:
	Ui::MainWindow *ui;
	FtpSynchronizer *syncer;
	SettingsDialog *settingsDlg;
	AboutDialog *aboutDlg;
	QProgressBar *progressBar;
	QList<QWidget*> widgetsToToggle;
};

#endif // MAINWINDOW_H
