#include <QDesktopServices>
#include <QUrl>

#include "AboutDialog.h"
#include "ui_AboutDialog.h"
#include "ZIMA-CAD-Sync.h"

AboutDialog::AboutDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::AboutDialog)
{
	ui->setupUi(this);

	connect(ui->aboutText, SIGNAL(linkActivated(QString)), this, SLOT(linkActivated(QString)));

	ui->aboutText->setText(QString(
			"<html><head><style>"
			"li{list-style-type:none;}"
			"</style></head><body>"
		)
		+ tr(
			"<h1>ZIMA-CAD-Sync</h1>"
			"<p class=\"version\">%1</p>"
			"<p>ZIMA-CAD-Sync was created by <a href=\"http://www.zima-engineering.cz/\">ZIMA-Engineering</a> "
			"and is released under the <a href=\"http://www.gnu.org/\">GNU/GPLv3</a> license."
			"</p>"
			"<h2>Authors:</h2>"
	    ).arg(VERSION)
	      + QString(tr(
			"<ul><li>Developed by Jakub Skokan &lt;<a href=\"mailto:aither@havefun.cz\">aither@havefun.cz</a>&gt;</li>"
			"<li>Icon created by Aleš Kocur &lt;<a href=\"mailto:kafe@havefun.cz\">kafe@havefun.cz</a>&gt;</li>")
	    ) + QString("</body></html")
	);

	ui->aboutText->setWordWrap(true);
}

AboutDialog::~AboutDialog()
{
	delete ui;
}

void AboutDialog::linkActivated(QString url)
{
	QDesktopServices::openUrl( QUrl(url) );
}
