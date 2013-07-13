#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"
#include "BaseSynchronizer.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
        QDialog(parent),
        ui(new Ui::SettingsDialog)
{
	ui->setupUi(this);

	settings = new QSettings(this);

	connect(ui->saveButton, SIGNAL(clicked()), this, SLOT(saveButtonClicked()));
	connect(ui->autoConfigButton, SIGNAL(clicked()), this, SLOT(autoConfigSubdirectories()));
	connect(ui->ptcCleanerPathButton, SIGNAL(clicked()), this, SLOT(showPtcCleanerPathDialog()));
	connect(ui->tbPathButton, SIGNAL(clicked()), this, SLOT(showTbPathDialog()));

	ui->ptcCleanerLineEdit->setText(settings->value("ZimaPtcCleanerPath").toString());
	ui->tbPathLineEdit->setText(settings->value("ThunderbirdPath").toString());

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

QString SettingsDialog::tbPath()
{
	return ui->tbPathLineEdit->text();
}

void SettingsDialog::showPtcCleanerPathDialog()
{
	QString path = QFileDialog::getOpenFileName(this, tr("Select ZIMA-PTC-Cleaner executable"), QDir::homePath());

	if(!path.isEmpty())
		ui->ptcCleanerLineEdit->setText(path);
}

void SettingsDialog::showTbPathDialog()
{
	QString path = QFileDialog::getOpenFileName(this, tr("Select Thunderbird executable"), QDir::homePath());

	if(!path.isEmpty())
		ui->tbPathLineEdit->setText(path);
}

void SettingsDialog::setCurrentDirectory(QString dir)
{
	currentDir = dir;
}

void SettingsDialog::saveButtonClicked()
{
	emit serverInfoChanged(ui->serverLineEdit->text(),
			       ui->usernameLineEdit->text(),
			       ui->passwordLineEdit->text(),
			       ui->directoryOnServerLineEdit->text());
	emit saveServerInfo(true);
}

void SettingsDialog::autoConfigSubdirectories()
{
	configureSubdirectory(currentDir, ui->directoryOnServerLineEdit->text());

	QMessageBox::information(this, tr("Subdirectories configured"), tr("All subdirectories were successfully configured."));
}

void SettingsDialog::configureSubdirectory(QString path, QString remoteBasePath)
{
	QDir d(path);
	QFileInfoList subdirs = d.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);

	foreach(QFileInfo subdir, subdirs)
	{
		if(subdir.fileName() == DIRECTORY_CONFIG_DIR)
			continue;

		QString remotePath = remoteBasePath + "/" + subdir.fileName();

		d.mkpath(subdir.absoluteFilePath() + "/" + DIRECTORY_CONFIG_DIR);

		QSettings cfg(subdir.absoluteFilePath() + "/" + DIRECTORY_CONFIG_PATH, QSettings::IniFormat);
		cfg.beginGroup("Sync");

		cfg.setValue("Host", ui->serverLineEdit->text());
		cfg.setValue("Username", ui->usernameLineEdit->text());

		if(ui->savePassSubdirCheckBox->isChecked())
			cfg.setValue("Password", ui->passwordLineEdit->text());

		cfg.setValue("RemoteDir", remotePath);

		cfg.endGroup();

		configureSubdirectory(subdir.absoluteFilePath(), remotePath);
	}
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
	settings->setValue("ThunderbirdPath", tbPath());

	emit serverInfoChanged(ui->serverLineEdit->text(),
			       ui->usernameLineEdit->text(),
			       ui->passwordLineEdit->text(),
			       ui->directoryOnServerLineEdit->text());
}
