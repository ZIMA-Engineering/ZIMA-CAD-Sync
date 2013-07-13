#include <QFileDialog>
#include <QProcess>
#include <QDebug>
#include <QListWidgetItem>
#include <QMessageBox>

#include "MainWindow.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	connect(ui->directoryLineEdit, SIGNAL(textEdited(QString)), this, SLOT(setDirectory(QString)));
	connect(ui->directoryButton, SIGNAL(clicked()), this, SLOT(selectDirectoryDialog()));
	connect(ui->diffDirToolButton, SIGNAL(clicked()), this, SLOT(selectDiffDirDialog()));

	connect(ui->localGroupBox, SIGNAL(clicked(bool)), this, SLOT(selectLocalSync(bool)));
	connect(ui->serverGroupBox, SIGNAL(clicked(bool)), this, SLOT(selectRemoteSync(bool)));

	connect(ui->syncToLocalButton, SIGNAL(clicked()), this, SLOT(syncToLocal()));
	connect(ui->syncToServerButton, SIGNAL(clicked()), this, SLOT(syncToServer()));

	aboutDlg = new AboutDialog(this);
	connect(ui->aboutButton, SIGNAL(clicked()), aboutDlg, SLOT(exec()));

	settingsDlg = new SettingsDialog(this);
	connect(ui->settingsButton, SIGNAL(clicked()), this, SLOT(openSettings()));

	syncer = new FtpSynchronizer(this);

	connect(syncer, SIGNAL(serverInfoLoaded(QString,QString,QString,QString)), settingsDlg, SLOT(directoryConfigRead(QString,QString,QString,QString)));
	connect(settingsDlg, SIGNAL(serverInfoChanged(QString,QString,QString,QString)), syncer, SLOT(setServerInfo(QString,QString,QString,QString)));
	connect(settingsDlg, SIGNAL(saveServerInfo(bool)), syncer, SLOT(saveLocalDirectoryConfig(bool)));
	connect(syncer, SIGNAL(directoryConfigLoaded(bool)), this, SLOT(directoryConfigLoaded(bool)));
	connect(syncer, SIGNAL(logoFound(QPixmap,bool)), this, SLOT(setDirectoryLogo(QPixmap,bool)));
	connect(syncer, SIGNAL(localizedLabelFound(QString)), this, SLOT(setDirectoryLabel(QString)));
	connect(syncer, SIGNAL(remoteStatus(bool)), this, SLOT(remoteStatus(bool)));
	connect(syncer, SIGNAL(done()), this, SLOT(syncDone()));
	connect(syncer, SIGNAL(errorOccured(QString)), this, SLOT(reportError(QString)));

	ui->serverProgressBar->hide();
	ui->serverProgressBar->setRange(0, 100);

	ui->localProgressBar->hide();
	ui->serverProgressBar->setRange(0, 100);

	connect(syncer, SIGNAL(fileTransferProgress(quint64,quint64)), this, SLOT(updateTransferProgress(quint64,quint64)));

	ui->abortServerButton->hide();
	ui->abortLocalButton->hide();

	ui->serverProgressLabel->hide();
	ui->localProgressLabel->hide();

	connect(ui->abortServerButton, SIGNAL(clicked()), this, SLOT(abortSync()));
	connect(ui->abortLocalButton, SIGNAL(clicked()), this, SLOT(abortSync()));

	widgetsToToggle << ui->directoryLineEdit
			<< ui->directoryButton
			<< ui->settingsButton
			<< ui->filterListWidget
			<< ui->syncCadDataCheckBox
			<< ui->removeCadDataCheckBox
			<< ui->syncToLocalButton
			<< ui->syncToServerButton
			<< ui->remoteDeleteComboBox
			<< ui->serverClearCheckbox
			<< ui->localDeleteComboBox
			<< ui->localCleanCheckBox
			<< ui->diffDirGroupBox;
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::setDirectory(QString path)
{
	if(path != ui->directoryLineEdit->text())
		ui->directoryLineEdit->setText(path);

	QDir d(path);

	ui->dirNameLabel->setText("<h1>" + d.dirName() + "</h1>");
	settingsDlg->setCurrentDirectory(path);

	syncer->setLocalDir(path);
	syncer->fetchLocalDirectoryConfig();
	syncer->probeMetadata();
	syncer->checkForUpdates();

	ui->filterListWidget->clear();

	QList<SyncItem*> items = syncer->syncItems();

	foreach(SyncItem *i, items)
	{
		QListWidgetItem *wi = new QListWidgetItem(ui->filterListWidget);
		wi->setCheckState(i->sync ? Qt::Checked : Qt::Unchecked);
		wi->setIcon(this->style()->standardIcon(QStyle::SP_DirIcon));
		wi->setText(i->name);
	}

	qDeleteAll(items);
}

void MainWindow::selectDirectoryDialog()
{
	QString d = QFileDialog::getExistingDirectory(this, tr("Select directory to sync"), ui->directoryLineEdit->text());

	if(!d.isEmpty())
		setDirectory(d);
}

void MainWindow::selectDiffDirDialog()
{
	QString d = QFileDialog::getExistingDirectory(this, tr("Select different directory to sync"), ui->directoryLineEdit->text());

	if(!d.isEmpty())
		ui->diffDirLineEdit->setText(d);
}

void MainWindow::selectLocalSync(bool checked)
{
	ui->serverGroupBox->setChecked(!checked);
}

void MainWindow::selectRemoteSync(bool checked)
{
	ui->localGroupBox->setChecked(!checked);
}

void MainWindow::directoryConfigLoaded(bool syncCadData)
{
	ui->syncCadDataCheckBox->setChecked(syncCadData);
}

void MainWindow::sync()
{
	if(syncDirection == ToLocal && ui->diffDirGroupBox->isChecked() && !ui->diffDirLineEdit->text().isEmpty())
	{
		QString d = ui->diffDirLineEdit->text();

		if(d.startsWith("./"))
			syncer->setLocalDir(ui->directoryLineEdit->text() + "/" + d.remove(0, 2));
		else syncer->setLocalDir(d);
	}

	if(syncDirection == ToServer && ui->serverClearCheckbox->isChecked())
	{
		QString ptcCleaner = settingsDlg->zimaPtcCleanerPath();

		if(!ptcCleaner.isEmpty() && QFile::exists(ptcCleaner))
		{
			QStringList args;
			args << ui->directoryLineEdit->text();

			QProcess::execute(ptcCleaner, args);
		}
	}

	syncer->setDeleteFirst( (BaseSynchronizer::Delete) (syncDirection == ToLocal ? ui->localDeleteComboBox->currentIndex() : ui->remoteDeleteComboBox->currentIndex()) );

	QList<SyncItem*> syncItems;
	int cnt = ui->filterListWidget->count();

	for(int i = 0; i < cnt; i++)
	{
		QListWidgetItem *wi = ui->filterListWidget->item(i);
		SyncItem *si = new SyncItem;
		si->sync = wi->checkState() == Qt::Checked;
		si->name = wi->text();

		syncItems << si;
	}

	syncer->saveLocalDirectoryConfig(false, syncItems, ui->syncCadDataCheckBox->isChecked());
	syncer->applyFilters(syncItems);

	qDeleteAll(syncItems);

	syncer->setSyncCadData(ui->syncCadDataCheckBox->isChecked());
	syncer->setDeleteCadData(ui->removeCadDataCheckBox->isChecked());

	if(syncDirection == ToLocal)
		syncer->syncToLocal();
	else
		syncer->syncToServer();

	foreach(QWidget *w, widgetsToToggle)
		w->setEnabled(false);
}

void MainWindow::syncToLocal()
{
	syncDirection = ToLocal;

	sync();

	ui->localProgressBar->show();
	ui->localProgressLabel->show();
	ui->abortLocalButton->show();
}

void MainWindow::syncToServer()
{
	syncDirection = ToServer;

	sync();

	ui->serverProgressBar->show();
	ui->serverProgressLabel->show();
	ui->abortServerButton->show();
}

void MainWindow::remoteStatus(bool changesAvailable)
{
	qDebug() << "Changes available" << changesAvailable;

	if(changesAvailable)
		ui->localGroupBox->setTitle( tr("Sync to local (* New changes available)") );
	else ui->localGroupBox->setTitle(tr("Sync to local"));
}

void MainWindow::updateTransferProgress(quint64 done, quint64 total)
{
	int p = (double)done / total * 100;
	QString sDone = QString::number(done / 1024.0, 'f', 2);
	QString sTotal = QString::number(total / 1024.0, 'f', 2);

	if(syncDirection == ToLocal)
	{
		//ui->localProgressBar->setFormat(QString("%1/%2 MB - %p %").arg(sDone).arg(sTotal));
		ui->localProgressLabel->setText(QString("%1/%2 MB").arg(sDone).arg(sTotal));
		ui->localProgressBar->setValue(p);
	} else {
		//ui->serverProgressBar->setFormat(QString("%1/%2 MB - %p %").arg(sDone).arg(sTotal));
		ui->serverProgressLabel->setText(QString("%1/%2 MB").arg(sDone).arg(sTotal));
		ui->serverProgressBar->setValue(p);
	}
}

void MainWindow::syncDone()
{
	if(syncDirection == ToLocal)
	{
		ui->localProgressBar->hide();
		ui->localProgressLabel->hide();
		ui->abortLocalButton->hide();
	} else {
		ui->serverProgressBar->hide();
		ui->serverProgressLabel->hide();
		ui->abortServerButton->hide();
	}

	foreach(QWidget *w, widgetsToToggle)
		w->setEnabled(true);

	if(syncDirection == ToLocal && ui->localCleanCheckBox->isChecked())
	{
		QString ptcCleaner = settingsDlg->zimaPtcCleanerPath();

		if(!ptcCleaner.isEmpty() && QFile::exists(ptcCleaner))
		{
			QStringList args;
			args << ui->directoryLineEdit->text();

			QProcess::execute(ptcCleaner, args);
		}
	}

	//statusBar()->showMessage(tr("Sync done"));

	if(ui->exitCheckBox->isChecked())
		QApplication::quit();
}

void MainWindow::abortSync()
{
	syncer->abort();

	if(syncDirection == ToLocal)
	{
		ui->localProgressBar->hide();
		ui->localProgressLabel->hide();
		ui->abortLocalButton->hide();
	} else {
		ui->serverProgressBar->hide();
		ui->serverProgressLabel->hide();
		ui->abortServerButton->hide();
	}

	ui->abortServerButton->hide();
	ui->abortLocalButton->hide();

	foreach(QWidget *w, widgetsToToggle)
		w->setEnabled(true);

	//statusBar()->showMessage(tr("Sync aborted"));
}

void MainWindow::openSettings()
{
	if(settingsDlg->exec())
		settingsDlg->saveSettings();
}

void MainWindow::setDirectoryLogo(QPixmap logo, bool showText)
{
	ui->dirLogoLabel->setPixmap(logo);

	if(!showText)
		ui->dirNameLabel->setText("");
}

void MainWindow::setDirectoryLabel(QString label)
{
	ui->dirNameLabel->setText("<h1>" + label + "</h1>");
}

void MainWindow::reportError(QString err)
{
	QMessageBox::warning(this, tr("Error occured"), err);
}
