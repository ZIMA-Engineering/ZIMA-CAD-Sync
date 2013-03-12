#include <QFileDialog>
#include <QProcess>
#include <QDebug>

#include "MainWindow.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	connect(ui->directoryLineEdit, SIGNAL(textEdited(QString)), this, SLOT(setDirectory(QString)));
	connect(ui->directoryButton, SIGNAL(clicked()), this, SLOT(selectDirectoryDialog()));

	connect(ui->localGroupBox, SIGNAL(clicked(bool)), this, SLOT(selectLocalSync(bool)));
	connect(ui->serverGroupBox, SIGNAL(clicked(bool)), this, SLOT(selectRemoteSync(bool)));

	connect(ui->serverLineEdit, SIGNAL(textEdited(QString)), this, SLOT(changeServerInfo()));
	connect(ui->usernameLineEdit, SIGNAL(textEdited(QString)), this, SLOT(changeServerInfo()));
	connect(ui->passwordLineEdit, SIGNAL(textEdited(QString)), this, SLOT(changeServerInfo()));
	connect(ui->directoryOnServerLineEdit, SIGNAL(textEdited(QString)), this, SLOT(changeServerInfo()));

	connect(ui->syncButton, SIGNAL(clicked()), this, SLOT(sync()));

	aboutDlg = new AboutDialog(this);
	connect(ui->aboutButton, SIGNAL(clicked()), aboutDlg, SLOT(exec()));

	settingsDlg = new SettingsDialog(this);
	connect(ui->settingsButton, SIGNAL(clicked()), this, SLOT(openSettings()));

	syncer = new FtpSynchronizer(this);

	connect(syncer, SIGNAL(directoryConfigRead(QString,QString,QString,QString)), this, SLOT(directoryConfigRead(QString,QString,QString,QString)));
	connect(syncer, SIGNAL(remoteStatus(bool)), this, SLOT(remoteStatus(bool)));
	connect(syncer, SIGNAL(done()), this, SLOT(syncDone()));

	progressBar = new QProgressBar(this);
	statusBar()->addWidget(progressBar, 100);
	progressBar->hide();
	progressBar->setValue(0);

	connect(syncer, SIGNAL(fileTransferProgress(int,int)), this, SLOT(updateTransferProgress(int,int)));

	ui->abortButton->hide();

	connect(ui->abortButton, SIGNAL(clicked()), this, SLOT(abortSync()));

	widgetsToToggle << ui->directoryLineEdit
			<< ui->serverLineEdit
			<< ui->usernameLineEdit
			<< ui->passwordLineEdit
			<< ui->directoryOnServerLineEdit
			<< ui->savePasswordCheckBox
			<< ui->localGroupBox
			<< ui->serverGroupBox
			<< ui->directoryButton
			<< ui->settingsButton;

}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::setDirectory(QString path)
{
	if(path != ui->directoryLineEdit->text())
		ui->directoryLineEdit->setText(path);

	syncer->setLocalDir(path);
	syncer->fetchLocalDirectoryConfig();
	syncer->checkForUpdates();
}

void MainWindow::selectDirectoryDialog()
{
	QString d = QFileDialog::getExistingDirectory(this, tr("Select directory to sync"), ui->directoryLineEdit->text());

	if(!d.isEmpty())
		setDirectory(d);
}

void MainWindow::selectLocalSync(bool checked)
{
	ui->serverGroupBox->setChecked(!checked);
}

void MainWindow::selectRemoteSync(bool checked)
{
	ui->localGroupBox->setChecked(!checked);
}

void MainWindow::directoryConfigRead(QString host, QString username, QString passwd, QString remoteDir)
{
	ui->serverLineEdit->setText(host);
	ui->usernameLineEdit->setText(username);
	ui->passwordLineEdit->setText(passwd);
	ui->directoryOnServerLineEdit->setText(remoteDir);
}

void MainWindow::changeServerInfo()
{
	syncer->setServer(ui->serverLineEdit->text());
	syncer->setUsername(ui->usernameLineEdit->text());
	syncer->setPassword(ui->passwordLineEdit->text());
	syncer->setRemoteDir(ui->directoryOnServerLineEdit->text());
}

void MainWindow::sync()
{
	if(ui->localGroupBox->isChecked() && ui->diffDirGroupBox->isChecked() && !ui->diffDirLineEdit->text().isEmpty())
	{
		QString d = ui->diffDirLineEdit->text();

		if(d.startsWith("./"))
			syncer->setLocalDir(ui->directoryLineEdit->text() + "/" + d.remove(0, 2));
		else syncer->setLocalDir(d);
	}

	if(ui->serverGroupBox->isChecked() && ui->serverClearCheckbox->isChecked())
	{
		QString ptcCleaner = settingsDlg->zimaPtcCleanerPath();

		if(!ptcCleaner.isEmpty() && QFile::exists(ptcCleaner))
		{
			QStringList args;
			args << ui->directoryLineEdit->text();

			QProcess::execute(ptcCleaner, args);
		}
	}

	syncer->setDeleteFirst( ui->localGroupBox->isChecked() ? ui->localRemoveFirstCheckBox->isChecked() : ui->remoteRemoteAllCheckBox->isChecked() );
	syncer->saveLocalDirectoryConfig(ui->savePasswordCheckBox->isChecked());

	if(ui->localGroupBox->isChecked())
		syncer->syncToLocal();
	else
		syncer->syncToServer();

	progressBar->show();
	ui->syncButton->hide();
	ui->abortButton->show();

	foreach(QWidget *w, widgetsToToggle)
		w->setEnabled(false);
}

void MainWindow::remoteStatus(bool changesAvailable)
{
	qDebug() << "Changes available" << changesAvailable;

	if(changesAvailable)
		ui->localGroupBox->setTitle( tr("Sync to local (* New changes available)") );
	else ui->localGroupBox->setTitle(tr("Sync to local"));
}

void MainWindow::updateTransferProgress(int done, int total)
{
	progressBar->setRange(0, total);
	progressBar->setValue(done);
}

void MainWindow::syncDone()
{
	progressBar->hide();
	ui->syncButton->show();
	ui->abortButton->hide();

	foreach(QWidget *w, widgetsToToggle)
		w->setEnabled(true);

	if(ui->localGroupBox->isChecked() && ui->localCleanCheckBox->isChecked())
	{
		QString ptcCleaner = settingsDlg->zimaPtcCleanerPath();

		if(!ptcCleaner.isEmpty() && QFile::exists(ptcCleaner))
		{
			QStringList args;
			args << ui->directoryLineEdit->text();

			QProcess::execute(ptcCleaner, args);
		}
	}

	statusBar()->showMessage(tr("Sync done"));
}

void MainWindow::abortSync()
{
	syncer->abort();

	progressBar->hide();
	ui->syncButton->show();
	ui->abortButton->hide();

	foreach(QWidget *w, widgetsToToggle)
		w->setEnabled(true);

	statusBar()->showMessage(tr("Sync aborted"));
}

void MainWindow::openSettings()
{
	if(settingsDlg->exec())
		settingsDlg->saveSettings();
}
