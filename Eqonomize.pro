unix {
	isEmpty(PREFIX) {
		PREFIX = /usr/local
	}
	isEmpty(DOCUMENTATION_DIR) {
		DOCUMENTATION_DIR = $$PREFIX/share/doc/eqonomize/html
	}
	isEmpty(TRANSLATIONS_DIR) {
		TRANSLATIONS_DIR = $$PREFIX/share/eqonomize/translations
	}
	isEmpty(DOCUMENTATION_DIR) {
		DOCUMENTATION_DIR = $$PREFIX/share/doc/eqonomize/html
	}
	isEmpty(MIME_DIR) {
		MIME_DIR = $$PREFIX/share/mime/application
	}
	isEmpty(DESKTOP_DIR) {
		DESKTOP_DIR = $$PREFIX/share/applications
	}
	isEmpty(ICON_DIR) {
		ICON_DIR = $$PREFIX/share/icons
	}
} else {
	TRANSLATIONS_DIR = ":/translations"
	ICON_DIR = ":/data"
	DOCUMENTATION_DIR = ":/doc/html"
}
TEMPLATE = app
TARGET = eqonomize
INCLUDEPATH += src
CONFIG += qt
QT += widgets network xml printsupport
MOC_DIR = build
OBJECTS_DIR = build
DEFINES += TRANSLATIONS_DIR=\\\"$$TRANSLATIONS_DIR\\\"
DEFINES += DOCUMENTATION_DIR=\\\"$$DOCUMENTATION_DIR\\\"

HEADERS += src/account.h \
           src/budget.h \
           src/categoriescomparisonchart.h \
           src/categoriescomparisonreport.h \
           src/editscheduledtransactiondialog.h \
           src/editsplitdialog.h \
           src/eqonomize.h \
           src/eqonomizemonthselector.h \
           src/eqonomizevalueedit.h \
           src/importcsvdialog.h \
           src/ledgerdialog.h \
           src/overtimechart.h \
           src/overtimereport.h \
           src/qifimportexport.h \
           src/recurrence.h \
           src/recurrenceeditwidget.h \
           src/security.h \
           src/transaction.h \
           src/transactioneditwidget.h \
           src/transactionfilterwidget.h \
           src/transactionlistwidget.h
SOURCES += src/account.cpp \
           src/budget.cpp \
           src/categoriescomparisonchart.cpp \
           src/categoriescomparisonreport.cpp \
           src/editscheduledtransactiondialog.cpp \
           src/editsplitdialog.cpp \
           src/eqonomize.cpp \
           src/eqonomizemonthselector.cpp \
           src/eqonomizevalueedit.cpp \
           src/importcsvdialog.cpp \
           src/ledgerdialog.cpp \
           src/main.cpp \
           src/overtimechart.cpp \
           src/overtimereport.cpp \
           src/qifimportexport.cpp \
           src/recurrence.cpp \
           src/recurrenceeditwidget.cpp \
           src/security.cpp \
           src/transaction.cpp \
           src/transactioneditwidget.cpp \
           src/transactionfilterwidget.cpp \
           src/transactionlistwidget.cpp

unix {
	TRANSLATIONS = 	translations/eqonomize_bg.ts \
			translations/eqonomize_cs.ts \
			translations/eqonomize_cs_new.ts \
			translations/eqonomize_de.ts \
			translations/eqonomize_es.ts \
			translations/eqonomize_fr.ts \
			translations/eqonomize_hu.ts \
			translations/eqonomize_it.ts \
			translations/eqonomize_nl.ts \
			translations/eqonomize_pt.ts \
			translations/eqonomize_pt_BR.ts \
			translations/eqonomize_ro.ts \
			translations/eqonomize_ru.ts \
			translations/eqonomize_sk.ts \
			translations/eqonomize_sv.ts

	target.path = $$PREFIX/bin
	qm.files = 	translations/eqonomize_bg.qm \
			translations/eqonomize_cs.qm \
			translations/eqonomize_de.qm \
			translations/eqonomize_fr.qm \
			translations/eqonomize_sv.qm
		
	qm.path = $$TRANSLATIONS_DIR
	doc.files = doc/html/*
	doc.path = $$DOCUMENTATION_DIR
	desktop.files = data/eqonomize.desktop
	desktop.path = $$DESKTOP_DIR
	mime.files = data/x-eqonomize.xml
	mime.path = $$MIME_DIR

	appicon16.files = data/16/eqonomize.png
	appicon16.path = $$ICON_DIR/hicolor/16x16/apps
	appicon22.files = data/22/eqonomize.png
	appicon22.path = $$ICON_DIR/hicolor/22x22/apps
	appicon32.files = data/32/eqonomize.png
	appicon32.path = $$ICON_DIR/hicolor/32x32/apps
	appicon48.files = data/48/eqonomize.png
	appicon48.path = $$ICON_DIR/hicolor/48x48/apps
	appicon64.files = data/64/eqonomize.png
	appicon64.path = $$ICON_DIR/hicolor/64x64/apps
	appicon128.files = data/128/eqonomize.png
	appicon128.path = $$ICON_DIR/hicolor/128x128/apps
	appiconsvg.files = data/scalable/eqonomize.svg
	appiconsvg.path = $$ICON_DIR/hicolor/scalable/apps

	mimeicon16.files = data/16/application-x-eqonomize.png
	mimeicon16.path = $$ICON_DIR/hicolor/16x16/mimetypes
	mimeicon22.files = data/22/application-x-eqonomize.png
	mimeicon22.path = $$ICON_DIR/hicolor/22x22/mimetypes
	mimeicon32.files = data/32/application-x-eqonomize.png
	mimeicon32.path = $$ICON_DIR/hicolor/32x32/mimetypes
	mimeicon48.files = data/48/application-x-eqonomize.png
	mimeicon48.path = $$ICON_DIR/hicolor/48x48/mimetypes
	mimeicon64.files = data/64/application-x-eqonomize.png
	mimeicon64.path = $$ICON_DIR/hicolor/64x64/mimetypes
	mimeicon128.files = data/128/application-x-eqonomize.png
	mimeicon128.path = $$ICON_DIR/hicolor/128x128/mimetypes
	mimeiconsvg.files = data/scalable/application-x-eqonomize.svg
	mimeiconsvg.path = $$ICON_DIR/hicolor/scalable/mimetypes

	actioniconssvg.files = 	data/scalable/eqz-account.svg \
				data/scalable/eqz-expense.svg \
				data/scalable/eqz-income.svg \
				data/scalable/eqz-transfer.svg \
				data/scalable/eqz-refund-repayment.svg \
				data/scalable/eqz-security.svg \
				data/scalable/eqz-schedule.svg \
				data/scalable/eqz-split-transaction.svg \
				data/scalable/eqz-join-transactions.svg \
				data/scalable/eqz-export.svg \
				data/scalable/eqz-previous-year.svg \
				data/scalable/eqz-previous-month.svg \
				data/scalable/eqz-next-month.svg \
				data/scalable/eqz-next-year.svg \
				data/scalable/eqz-categories-chart.svg \
				data/scalable/eqz-overtime-chart.svg \
				data/scalable/eqz-categories-report.svg \
				data/scalable/eqz-overtime-report.svg
	actioniconssvg.path = $$ICON_DIR/hicolor/scalable/actions

	INSTALLS += 	target qm doc desktop mime \ 
			appicon16 appicon22 appicon32 appicon48 appicon64 appicon128 appiconsvg \
			mimeicon16 mimeicon22 mimeicon32 mimeicon48 mimeicon64 mimeicon128 mimeiconsvg \
			actioniconssvg
} else {
	RESOURCES = doc.qrc icons.qrc translations.qrc
}
