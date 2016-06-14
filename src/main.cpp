/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016 by Hanna Knutsson                 *
 *   hanna_k@fmgirl.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <QCommandLineParser>
#include <QApplication>
#include <QObject>

#include <KSharedConfig>
#include <KDBusService>
#include <kaboutdata.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <klocalizedstring.h>

#include "budget.h"
#include "eqonomize.h"

QString emptystr = "";

int main(int argc, char **argv) {
	KLocalizedString::setApplicationDomain("eqonomize");
	QApplication app(argc, argv);
	app.setApplicationName("eqonomize");
	app.setApplicationVersion("0.6");
	KAboutData about("eqonomize", i18n("Eqonomize!"), "0.6", i18n("A personal accounting program"), KAboutLicense::GPL_V2, i18n("(C) 2006-2008, 2014, 2016 Hanna Knutsson"), QString(), "http://eqonomize.sourceforge.net/", "hanna_k@users.sourceforge.net");
	about.addAuthor(i18n("Hanna Knutsson"), QString(), "hanna_k@fmgirl.com");
	KAboutData::setApplicationData(about);
	QCommandLineParser *parser = new QCommandLineParser();
	KAboutData::applicationData().setupCommandLine(parser);
	QCommandLineOption eOption(QStringList() << "e" << "expenses", i18n("Start with expenses list displayed"));
	parser->addOption(eOption);
	QCommandLineOption iOption(QStringList() << "i" << "incomes", i18n("Start with incomes list displayed"));
	parser->addOption(iOption);
	QCommandLineOption tOption("transfers", i18n("Start with transfers list displayed"));
	parser->addOption(tOption);
	parser->addPositionalArgument("url", i18n("Document to open"), "[url]");
	parser->addHelpOption();
	parser->process(app);
	KAboutData::applicationData().processCommandLine(parser);
	
	KDBusService service(KDBusService::Unique);	
	
	Eqonomize *win = new Eqonomize();
	win->setCommandLineParser(parser);
	QObject::connect(&service, SIGNAL(activateRequested(const QStringList&, const QString&)), win, SLOT(onActivateRequested(const QStringList&, const QString&)));
	if(parser->isSet(eOption)) {
		win->showExpenses();
	} else if(parser->isSet(iOption)) {
		win->showIncomes();
	} else if(parser->isSet(tOption)) {
		win->showTransfers();
	}	
	win->show();
	app.processEvents();
	QString url = KSharedConfig::openConfig()->group("General Options").readPathEntry("lastURL", QString());
	QStringList args = parser->positionalArguments();
	if(args.count() > 0) {
		win->openURL(QUrl::fromUserInput(args.at(0)));
	} else if(!url.isEmpty()) {
		win->openURL(QUrl(url));
		win->readFileDependentOptions();
	} else {
		if(!win->crashRecovery(QUrl())) {
			win->createDefaultBudget();
		}
	}
	args.clear();
	
	return app.exec();

}
