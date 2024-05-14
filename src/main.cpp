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
#if defined _WIN32 && (QT_VERSION >= QT_VERSION_CHECK(6, 5, 0))
	app.setStyle("Fusion");
#endif
	app.setApplicationName("eqonomize");
	app.setApplicationDisplayName("Eqonomize!");
	app.setOrganizationName("Eqonomize");
	app.setApplicationVersion(VERSION);

#ifdef PACKAGE_PORTABLE
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, QCoreApplication::applicationDirPath() + "/user");
#endif

	QSettings settings;
	settings.beginGroup("GeneralOptions");

	QString sfont = settings.value("font").toString();
	QFont font;
	if(!sfont.isEmpty()) {
		font.fromString(sfont);
		app.processEvents();
		if(font.family() == app.font().family() && font.pointSize() == app.font().pointSize() && font.pixelSize() == app.font().pixelSize() && font.overline() == app.font().overline() && font.stretch() == app.font().stretch() && font.letterSpacing() == app.font().letterSpacing() && font.underline() == app.font().underline() && font.style() == app.font().style() && font.weight() == app.font().weight()) {
			settings.remove("font");
		} else {
			app.setFont(font);
		}
	}

	QString locale = setlocale(LC_MONETARY, NULL);
	if(locale == QLocale::c().name()) {
		setlocale(LC_MONETARY, QLocale::system().name().toLocal8Bit());
	} else {
		setlocale(LC_MONETARY, "");
	}

	QString slang = settings.value("language", QString()).toString();

	EqonomizeTranslator eqtr;
	app.installTranslator(&eqtr);

	if(!slang.isEmpty()) {
		if(translator.load(QLatin1String("eqonomize") + QLatin1String("_") + slang, QLatin1String(TRANSLATIONS_DIR))) app.installTranslator(&translator);
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
		if(translator_qt.load("qt_" + slang, QLibraryInfo::path(QLibraryInfo::TranslationsPath))) app.installTranslator(&translator_qt);
		if(translator_qtbase.load("qtbase_" + slang, QLibraryInfo::path(QLibraryInfo::TranslationsPath))) app.installTranslator(&translator_qtbase);
#else
		if(translator_qt.load("qt_" + slang, QLibraryInfo::location(QLibraryInfo::TranslationsPath))) app.installTranslator(&translator_qt);
		if(translator_qtbase.load("qtbase_" + slang, QLibraryInfo::location(QLibraryInfo::TranslationsPath))) app.installTranslator(&translator_qtbase);
#endif
	} else {
		if(translator.load(QLocale(), QLatin1String("eqonomize"), QLatin1String("_"), QLatin1String(TRANSLATIONS_DIR))) app.installTranslator(&translator);
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
		if(translator_qt.load(QLocale(), QLatin1String("qt"), QLatin1String("_"), QLibraryInfo::path(QLibraryInfo::TranslationsPath))) app.installTranslator(&translator_qt);
		if(translator_qtbase.load(QLocale(), QLatin1String("qtbase"), QLatin1String("_"), QLibraryInfo::path(QLibraryInfo::TranslationsPath))) app.installTranslator(&translator_qtbase);
#else
		if(translator_qt.load(QLocale(), QLatin1String("qt"), QLatin1String("_"), QLibraryInfo::location(QLibraryInfo::TranslationsPath))) app.installTranslator(&translator_qt);
		if(translator_qtbase.load(QLocale(), QLatin1String("qtbase"), QLatin1String("_"), QLibraryInfo::location(QLibraryInfo::TranslationsPath))) app.installTranslator(&translator_qtbase);
#endif
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

#ifdef PACKAGE_PORTABLE
	QString lockpath = QCoreApplication::applicationDirPath() + "/user";
#else
#	if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
	QString lockpath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
#	else
	QString lockpath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/" + app.organizationName() + "/" + app.applicationName();
#	endif
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

	settings.beginGroup("GeneralOptions");
#ifdef RESOURCES_COMPILED
	if(!settings.value("lastVersionCheck").toDate().isValid() || settings.value("lastVersionCheck").toDate().addDays(10) <= QDate::currentDate()) win->checkAvailableVersion();
#endif

	//fixes font with gtk2 style
	sfont = settings.value("font").toString();
	if(!sfont.isEmpty()) {
		QFont font;
		font.fromString(sfont);
		app.setFont(font);
		win->updateAccountColumnWidths();
	}
	settings.endGroup();

	args.clear();

	return app.exec();

}
