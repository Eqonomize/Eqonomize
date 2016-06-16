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
#include <QSettings>
#include <QLockFile>
#include <QStandardPaths>
#include <QtGlobal>
#include <QLocalSocket>
#include <QDebug>

#include "budget.h"
#include "eqonomize.h"

QString emptystr = "";

int main(int argc, char **argv) {	

#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
	QLockFile lockFile(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/eqonomize.lock");
#else
	QLockFile lockFile(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/eqonomize.lock");
#endif
	if(!lockFile.tryLock(100)){
		if(lockFile.error() == QLockFile::LockFailedError) {
			QLocalSocket socket;
			socket.connectToServer("eqonomize");
			if(socket.waitForConnected()) {
				socket.putChar('r');
				socket.waitForBytesWritten(3000);
				socket.disconnectFromServer();
			}
			return 1;
		}
	}	

	QApplication app(argc, argv);
	app.setApplicationName("eqonomize");
	app.setApplicationDisplayName("Eqonomize!");
	app.setOrganizationName("Eqonomize");
	app.setApplicationVersion("0.6");
	QCommandLineParser *parser = new QCommandLineParser();
	QCommandLineOption eOption(QStringList() << "e" << "expenses", QApplication::tr("Start with expenses list displayed"));
	parser->addOption(eOption);
	QCommandLineOption iOption(QStringList() << "i" << "incomes", QApplication::tr("Start with incomes list displayed"));
	parser->addOption(iOption);
	QCommandLineOption tOption("transfers", QApplication::tr("Start with transfers list displayed"));
	parser->addOption(tOption);
	parser->addPositionalArgument("url", QApplication::tr("Document to open"), "[url]");
	parser->addHelpOption();
	parser->process(app);
	
	Eqonomize *win = new Eqonomize();
	win->setCommandLineParser(parser);
	if(parser->isSet(eOption)) {
		win->showExpenses();
	} else if(parser->isSet(iOption)) {
		win->showIncomes();
	} else if(parser->isSet(tOption)) {
		win->showTransfers();
	}	
	win->show();
	app.processEvents();
	QSettings settings;
	settings.beginGroup("GeneralOptions");
	QString url = settings.value("lastURL", QString()).toString();
	settings.endGroup();
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
