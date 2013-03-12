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
	void saveSettings();

private slots:
	void showPtcCleanerPathDialog();
#ifdef Q_OS_WIN32
    void enableSystemContextMenuChanged(bool checked);
#endif
	
private:
	Ui::SettingsDialog *ui;
	QSettings *settings;
#ifdef Q_OS_WIN32
    QSettings *contextMenuSettings;
#endif
};

#endif // SETTINGSDIALOG_H
