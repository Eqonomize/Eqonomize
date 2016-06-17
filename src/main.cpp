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
#include <QTranslator>
#include <QDir>
#include <QTextStream>

#include "budget.h"
#include "eqonomize.h"

QString emptystr = "";

int main(int argc, char **argv) {	
	
	QApplication app(argc, argv);
	app.setApplicationName("eqonomize");
	app.setApplicationDisplayName("Eqonomize!");
	app.setOrganizationName("Eqonomize");
	app.setApplicationVersion("0.6");
	
	QTranslator translator;
	if (translator.load(QLocale(), QLatin1String("eqonomize"), QLatin1String("_"), QLatin1String(TRANSLATIONS_DIR))) {
		app.installTranslator(&translator);
	}
	
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
			
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
	QString lockpath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
#else
	QString lockpath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/" + app.organizationName() + "/" + app.applicationName();
#endif
	QDir lockdir(lockpath);
	QLockFile lockFile(lockpath + "/eqonomize.lock");
	if(lockdir.mkpath(lockpath)) {
		if(!lockFile.tryLock(100)){
			if(lockFile.error() == QLockFile::LockFailedError) {
				QTextStream outStream(stdout);
				outStream << QApplication::tr("%1 is already running.").arg(app.applicationDisplayName()) << '\n';
				QLocalSocket socket;
				socket.connectToServer("eqonomize");
				if(socket.waitForConnected()) {
					QStringList args = parser->positionalArguments();
					QString command;
					if(parser->isSet(eOption)) {
						command = 'e';
					} else if(parser->isSet(iOption)) {
						command = 'i';
					} else if(parser->isSet(tOption)) {
						command = 't';
					} else {
						command = '0';
					}
					if(args.count() > 0) command += args.at(0);
					socket.write(command.toUtf8());
					socket.waitForBytesWritten(3000);
					socket.disconnectFromServer();
				}
				return 1;
			}
		}
	}
	
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
