#include <QFileDialog>
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

	syncer = new FtpSynchronizer(this);

	connect(syncer, SIGNAL(directoryConfigRead(QString,QString,QString,QString)), this, SLOT(directoryConfigRead(QString,QString,QString,QString)));
	connect(syncer, SIGNAL(remoteStatus(bool)), this, SLOT(remoteStatus(bool)));
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::setDirectory(QString path)
{
	ui->directoryLineEdit->setText(path);

	// FIXME: load user credentials from meta file in this directory, if it exists
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
	// TODO: progress bar, abort button

	syncer->saveLocalDirectoryConfig(ui->savePasswordCheckBox->isChecked());

	if(ui->localGroupBox->isChecked())
		syncer->syncToLocal();
	else
		syncer->syncToServer();
}

void MainWindow::remoteStatus(bool changesAvailable)
{
	qDebug() << "Changes available" << changesAvailable;
}
