#include <QFileDialog>

#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
        QDialog(parent),
        ui(new Ui::SettingsDialog)
{
	ui->setupUi(this);

	settings = new QSettings(this);

	connect(ui->ptcCleanerPathButton, SIGNAL(clicked()), this, SLOT(showPtcCleanerPathDialog()));

	ui->ptcCleanerLineEdit->setText(settings->value("ZimaPtcCleanerPath").toString());

#ifdef Q_OS_WIN32
	contextMenuSettings = new QSettings("HKEY_CLASSES_ROOT\\Directory\\shell", QSettings::NativeFormat, this);
	connect(ui->enableSystemContextMenuCheckBox, SIGNAL(toggled(bool)), this, SLOT(enableSystemContextMenuChanged(bool)));

	bool enabled = settings->value("EnableContextMenu", true).toBool();
	ui->enableSystemContextMenuCheckBox->setChecked(enabled);

	if( enabled )
		enableSystemContextMenuChanged(true);
#else
	ui->enableSystemContextMenuCheckBox->setEnabled(false);
#endif
}

SettingsDialog::~SettingsDialog()
{
	delete ui;
}

#ifdef Q_OS_WIN32
void SettingsDialog::enableSystemContextMenuChanged(bool checked)
{
	if( checked )
	{
		contextMenuSettings->setValue("ZIMA-CAD-Sync/.", tr("Sync with ZIMA-CAD-Sync"));
		contextMenuSettings->setValue("ZIMA-CAD-Sync/command/.", QString("\"%1\"").arg(QApplication::applicationFilePath().replace("/", "\\")) + " \"%1\"");
	} else {
		contextMenuSettings->remove("ZIMA-CAD-Sync");
	}
}
#endif

QString SettingsDialog::zimaPtcCleanerPath()
{
	return ui->ptcCleanerLineEdit->text();
}

void SettingsDialog::showPtcCleanerPathDialog()
{
	QString path = QFileDialog::getOpenFileName(this, tr("Select ZIMA-PTC-Cleaner executable"), QDir::homePath());

	if(!path.isEmpty())
		ui->ptcCleanerLineEdit->setText(path);
}

void SettingsDialog::saveButtonClicked()
{
	emit serverInfoChanged(ui->serverLineEdit->text(),
			       ui->usernameLineEdit->text(),
			       ui->passwordLineEdit->text(),
			       ui->directoryOnServerLineEdit->text());
	emit saveServerInfo(true);
}

void SettingsDialog::directoryConfigRead(QString host, QString username, QString passwd, QString remoteDir)
{
	ui->serverLineEdit->setText(host);
	ui->usernameLineEdit->setText(username);
	ui->passwordLineEdit->setText(passwd);
	ui->directoryOnServerLineEdit->setText(remoteDir);
}

void SettingsDialog::saveSettings()
{
	settings->setValue("EnableContextMenu", ui->enableSystemContextMenuCheckBox->isChecked());
	settings->setValue("ZimaPtcCleanerPath", zimaPtcCleanerPath());

	emit serverInfoChanged(ui->serverLineEdit->text(),
			       ui->usernameLineEdit->text(),
			       ui->passwordLineEdit->text(),
			       ui->directoryOnServerLineEdit->text());
}
