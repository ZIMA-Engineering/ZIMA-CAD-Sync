#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QSettings>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
	Q_OBJECT
	
public:
	explicit SettingsDialog(QWidget *parent = 0);
	~SettingsDialog();
	QString zimaPtcCleanerPath();
	void setCurrentDirectory(QString dir);
	void saveSettings();

public slots:
	void directoryConfigRead(QString host, QString username, QString passwd, QString remoteDir);

private slots:
	void showPtcCleanerPathDialog();
	void saveButtonClicked();
	void autoConfigSubdirectories();
#ifdef Q_OS_WIN32
    void enableSystemContextMenuChanged(bool checked);
#endif
	
private:
	Ui::SettingsDialog *ui;
	QSettings *settings;
	QString currentDir;
#ifdef Q_OS_WIN32
    QSettings *contextMenuSettings;
#endif

signals:
    void serverInfoChanged(QString host, QString username, QString passwd, QString remoteDir);
    void saveServerInfo(bool pass);
};

#endif // SETTINGSDIALOG_H
