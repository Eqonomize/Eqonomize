/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016 by Hanna Knutsson                 *
 *   hanna.knutsson@protonmail.com                                         *
 *                                                                         *
 *   This file is part of Eqonomize!.                                      *
 *                                                                         *
 *   Eqonomize! is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Eqonomize! is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with Eqonomize!. If not, see <http://www.gnu.org/licenses/>.    *
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
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>

#include <locale.h>

#include "budget.h"
#include "eqonomize.h"

QTranslator translator, translator_qt, translator_qtbase;

int main(int argc, char **argv) {

	QApplication app(argc, argv);
	app.setApplicationName("eqonomize");
	app.setApplicationDisplayName("Eqonomize!");
	app.setOrganizationName("Eqonomize");
	app.setApplicationVersion(VERSION);

	QString locale = setlocale(LC_MONETARY, NULL);
	if(locale == QLocale::c().name()) {
		setlocale(LC_MONETARY, QLocale::system().name().toLocal8Bit());
	} else {
		setlocale(LC_MONETARY, "");
	}

	QSettings settings;
	settings.beginGroup("GeneralOptions");
	QString slang = settings.value("language", QString()).toString();

	EqonomizeTranslator eqtr;
	app.installTranslator(&eqtr);
	if(!slang.isEmpty()) {
		if(translator.load(QLatin1String("eqonomize") + QLatin1String("_") + slang, QLatin1String(TRANSLATIONS_DIR))) app.installTranslator(&translator);
		if(translator_qt.load("qt_" + slang, QLibraryInfo::location(QLibraryInfo::TranslationsPath))) app.installTranslator(&translator_qt);
		if(translator_qtbase.load("qtbase_" + slang, QLibraryInfo::location(QLibraryInfo::TranslationsPath))) app.installTranslator(&translator_qtbase);
	} else {
		if(translator.load(QLocale(), QLatin1String("eqonomize"), QLatin1String("_"), QLatin1String(TRANSLATIONS_DIR))) app.installTranslator(&translator);
		if(translator_qt.load(QLocale(), QLatin1String("qt"), QLatin1String("_"), QLibraryInfo::location(QLibraryInfo::TranslationsPath))) app.installTranslator(&translator_qt);
		if(translator_qtbase.load(QLocale(), QLatin1String("qtbase"), QLatin1String("_"), QLibraryInfo::location(QLibraryInfo::TranslationsPath))) app.installTranslator(&translator_qtbase);
	}


	QCommandLineParser *parser = new QCommandLineParser();
	QCommandLineOption eOption(QStringList() << "e" << "expenses", QApplication::tr("Start with expenses list displayed"));
	parser->addOption(eOption);
	QCommandLineOption iOption(QStringList() << "i" << "incomes", QApplication::tr("Start with incomes list displayed"));
	parser->addOption(iOption);
	QCommandLineOption tOption("transfers", QApplication::tr("Start with transfers list displayed"));
	parser->addOption(tOption);
	QCommandLineOption sOption(QStringList() << "s" << "sync", QApplication::tr("Synchronize file"));
	parser->addOption(sOption);
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
					} else if(parser->isSet(sOption)) {
						command = 's';
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

	QString url = settings.value("lastURL", QString()).toString();
	settings.endGroup();

	if(parser->isSet(sOption)) {
		QStringList args = parser->positionalArguments();
		QUrl u;
		if(args.count() > 0) {
			u = QUrl::fromUserInput(args.at(0));
		} else {
			u = QUrl(url);
		}
		Budget *budget = new Budget();
		QString errors;
		QString error = budget->loadFile(u.toLocalFile(), errors);
		if(!error.isNull()) {qWarning() << error; return EXIT_FAILURE;}
		if(!errors.isEmpty()) qWarning() << errors;
		errors = QString();
		if(budget->sync(error, errors, true, true)) {
			if(!errors.isEmpty()) qWarning() << errors;
			error = budget->saveFile(u.toLocalFile());
			if(!error.isNull()) {qWarning() << error; return EXIT_FAILURE;}
		} else {
			if(!error.isNull()) {qWarning() << error; return EXIT_FAILURE;}
			if(!errors.isEmpty()) qWarning() << errors;
		}
		return 0;
	}

#ifndef LOAD_EQZICONS_FROM_FILE
	if(QIcon::themeName().isEmpty() || !QIcon::hasThemeIcon("eqz-account")) {
		QIcon::setThemeSearchPaths(QStringList(ICON_DIR));
		QIcon::setThemeName("EQZ");
	}
#endif
	app.setWindowIcon(LOAD_APP_ICON("eqonomize"));

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

	QStringList args = parser->positionalArguments();
	if(args.count() > 0) {
		win->openURL(QUrl::fromUserInput(args.at(0)));
	} else if(!url.isEmpty()) {
		win->openURL(QUrl(url));
	} else {
		if(!win->crashRecovery(QUrl())) {
			win->createDefaultBudget();
		}
	}
#ifdef RESOURCES_COMPILED
	settings.beginGroup("GeneralOptions");
	if(!settings.value("lastVersionCheck").toDate().isValid() || settings.value("lastVersionCheck").toDate().addDays(10) <= QDate::currentDate()) win->checkAvailableVersion();
#endif

	args.clear();

	return app.exec();

}
