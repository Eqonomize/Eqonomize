VERSION = 1.5.5
isEmpty(PREFIX) {
	PREFIX = /usr/local
}
isEmpty(DESKTOP_DIR) {
	DESKTOP_DIR = $$PREFIX/share/applications
}
isEmpty(DESKTOP_ICON_DIR) {
	DESKTOP_ICON_DIR = $$PREFIX/share/icons
}
equals(INSTALL_THEME_ICONS,"no") {
	DEFINES += LOAD_EQZICONS_FROM_FILE=1
}
unix:!equals(COMPILE_RESOURCES,"yes"):!android:!macx {
	isEmpty(DOCUMENTATION_DIR) {
		DOCUMENTATION_DIR = $$PREFIX/share/doc/eqonomize/html
	}
	isEmpty(TRANSLATIONS_DIR) {
		TRANSLATIONS_DIR = $$PREFIX/share/eqonomize/translations
	}
	isEmpty(DATA_DIR) {
		DATA_DIR = $$PREFIX/share/eqonomize
	}
	isEmpty(MIME_DIR) {
		MIME_DIR = $$PREFIX/share/mime/packages
	}
	isEmpty(ICON_DIR) {
		equals(INSTALL_THEME_ICONS,"no") {
			ICON_DIR = $$PREFIX/share/eqonomize/icons
		} else {
			ICON_DIR = $$PREFIX/share/icons
		}
	}
} else {
	TRANSLATIONS_DIR = ":/translations"
	ICON_DIR = ":/icons"
	DATA_DIR = ":/data"
	DOCUMENTATION_DIR = ":/doc/html"
	DEFINES += RESOURCES_COMPILED=1
}
isEmpty(MAN_DIR) {
	MAN_DIR = $$PREFIX/share/man
}
TEMPLATE = app
TARGET = eqonomize
INCLUDEPATH += src
CONFIG += qt
QT += widgets network printsupport
!equals(DISABLE_QTCHARTS,"yes"):!equals(ENABLE_QTCHARTS,"no") {
	qtHaveModule(charts) {
		QT += charts
	} else {
		warning("Qt Charts module is missing")
	}
} else {
	equals(DISABLE_QTCHARTS,"no"):!equals(ENABLE_QTCHARTS,"yes") {
		QT += charts
	}
}
MOC_DIR = build
OBJECTS_DIR = build
DEFINES += TRANSLATIONS_DIR=\\\"$$TRANSLATIONS_DIR\\\"
DEFINES += DOCUMENTATION_DIR=\\\"$$DOCUMENTATION_DIR\\\"
DEFINES += DATA_DIR=\\\"$$DATA_DIR\\\"
DEFINES += ICON_DIR=\\\"$$ICON_DIR\\\"
DEFINES += VERSION=\\\"$$VERSION\\\"

HEADERS += src/account.h \
           src/accountcombobox.h \
           src/budget.h \
           src/categoriescomparisonchart.h \
           src/categoriescomparisonreport.h \
           #src/currencies.xml.h \
           src/currency.h \
           src/currencyconversiondialog.h \
           src/editaccountdialogs.h \
           src/editcurrencydialog.h \
           src/editscheduledtransactiondialog.h \
           src/editsplitdialog.h \
           src/eqonomize.h \
           src/eqonomizelist.h \
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
           src/accountcombobox.cpp \
           src/budget.cpp \
           src/categoriescomparisonchart.cpp \
           src/categoriescomparisonreport.cpp \
           src/currency.cpp \
           src/currencyconversiondialog.cpp \
           src/editaccountdialogs.cpp \
           src/editcurrencydialog.cpp \
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

unix:!equals(COMPILE_RESOURCES,"yes"):!android:!macx {
	TRANSLATIONS = 	translations/eqonomize_bg.ts \
			translations/eqonomize_cs.ts \
			translations/eqonomize_da.ts \
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
			translations/eqonomize_da.qm \
			translations/eqonomize_de.qm \
			translations/eqonomize_es.qm \
			translations/eqonomize_fr.qm \
			translations/eqonomize_hu.qm \
			translations/eqonomize_it.qm \
			translations/eqonomize_nl.qm \
			translations/eqonomize_pt.qm \
			translations/eqonomize_pt_BR.qm \
			translations/eqonomize_ro.qm \
			translations/eqonomize_ru.qm \
			translations/eqonomize_sk.qm \
			translations/eqonomize_sv.qm
		
	qm.path = $$TRANSLATIONS_DIR
	data.files = data/currencies.xml
	data.path = $$DATA_DIR
	doc.files = doc/html/*
	doc.path = $$DOCUMENTATION_DIR
	desktop.files = data/eqonomize.desktop
	desktop.path = $$DESKTOP_DIR
	mime.files = data/eqonomize.xml
	mime.path = $$MIME_DIR

	appicon16.files = data/16/eqonomize.png
	appicon16.path = $$ICON_DIR/hicolor/16x16/apps
	appicon22.files = data/22/eqonomize.png
	appicon22.path = $$ICON_DIR/hicolor/22x22/apps
	appicon32.files = data/32/eqonomize.png
	appicon32.path = $$ICON_DIR/hicolor/32x32/apps
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
				data/scalable/eqz-liabilities.svg \
				data/scalable/eqz-expense.svg \
				data/scalable/eqz-income.svg \
				data/scalable/eqz-transfer.svg \
				data/scalable/eqz-refund-repayment.svg \
				data/scalable/eqz-debt-payment.svg \
				data/scalable/eqz-debt-interest.svg \
				data/scalable/eqz-security.svg \
				data/scalable/eqz-schedule.svg \
				data/scalable/eqz-split-transaction.svg \
				data/scalable/eqz-join-transactions.svg \
				data/scalable/eqz-export.svg \
				data/scalable/eqz-import.svg \
				data/scalable/eqz-edit.svg \
				data/scalable/eqz-tag.svg \
				data/scalable/eqz-balance.svg \
				data/scalable/eqz-ledger.svg \
				data/scalable/eqz-transactions.svg \
				data/scalable/eqz-previous-year.svg \
				data/scalable/eqz-previous-month.svg \
				data/scalable/eqz-next-month.svg \
				data/scalable/eqz-next-year.svg \
				data/scalable/eqz-categories-chart.svg \
				data/scalable/eqz-overtime-chart.svg \
				data/scalable/eqz-categories-report.svg \
				data/scalable/eqz-currency.svg \
				data/scalable/eqz-overtime-report.svg
	actioniconssvg.path = $$ICON_DIR/hicolor/scalable/actions
	actionicons16.files = 	data/16/eqz-account.png \
				data/16/eqz-liabilities.png \
				data/16/eqz-expense.png \
				data/16/eqz-income.png \
				data/16/eqz-transfer.png \
				data/16/eqz-refund-repayment.png \
				data/16/eqz-debt-payment.png \
				data/16/eqz-debt-interest.png \
				data/16/eqz-security.png \
				data/16/eqz-schedule.png \
				data/16/eqz-split-transaction.png \
				data/16/eqz-join-transactions.png \
				data/16/eqz-export.png \
				data/16/eqz-import.png \
				data/16/eqz-edit.png \
				data/16/eqz-tag.png \
				data/16/eqz-balance.png \
				data/16/eqz-ledger.png \
				data/16/eqz-transactions.png \
				data/16/eqz-previous-year.png \
				data/16/eqz-previous-month.png \
				data/16/eqz-next-month.png \
				data/16/eqz-next-year.png \
				data/16/eqz-categories-chart.png \
				data/16/eqz-overtime-chart.png \
				data/16/eqz-categories-report.png \
				data/16/eqz-currency.png \
				data/16/eqz-overtime-report.png
	actionicons16.path = $$ICON_DIR/hicolor/16x16/actions
	actionicons22.files = 	data/22/eqz-account.png \
				data/22/eqz-liabilities.png \
				data/22/eqz-expense.png \
				data/22/eqz-income.png \
				data/22/eqz-transfer.png \
				data/22/eqz-refund-repayment.png \
				data/22/eqz-debt-payment.png \
				data/22/eqz-debt-interest.png \
				data/22/eqz-security.png \
				data/22/eqz-schedule.png \
				data/22/eqz-split-transaction.png \
				data/22/eqz-join-transactions.png \
				data/22/eqz-export.png \
				data/22/eqz-import.png \
				data/22/eqz-edit.png \
				data/22/eqz-tag.png \
				data/22/eqz-balance.png \
				data/22/eqz-ledger.png \
				data/22/eqz-transactions.png \
				data/22/eqz-previous-year.png \
				data/22/eqz-previous-month.png \
				data/22/eqz-next-month.png \
				data/22/eqz-next-year.png \
				data/22/eqz-categories-chart.png \
				data/22/eqz-overtime-chart.png \
				data/22/eqz-categories-report.png \
				data/22/eqz-currency.png \
				data/22/eqz-overtime-report.png
	actionicons22.path = $$ICON_DIR/hicolor/22x22/actions
	actionicons32.files = 	data/32/eqz-account.png \
				data/32/eqz-liabilities.png \
				data/32/eqz-expense.png \
				data/32/eqz-income.png \
				data/32/eqz-transfer.png \
				data/32/eqz-refund-repayment.png \
				data/32/eqz-debt-payment.png \
				data/32/eqz-debt-interest.png \
				data/32/eqz-security.png \
				data/32/eqz-schedule.png \
				data/32/eqz-split-transaction.png \
				data/32/eqz-join-transactions.png \
				data/32/eqz-export.png \
				data/32/eqz-import.png \
				data/32/eqz-edit.png \
				data/32/eqz-tag.png \
				data/32/eqz-balance.png \
				data/32/eqz-ledger.png \
				data/32/eqz-transactions.png \
				data/32/eqz-previous-year.png \
				data/32/eqz-previous-month.png \
				data/32/eqz-next-month.png \
				data/32/eqz-next-year.png \
				data/32/eqz-categories-chart.png \
				data/32/eqz-overtime-chart.png \
				data/32/eqz-categories-report.png \
				data/32/eqz-currency.png \
				data/32/eqz-overtime-report.png
	actionicons32.path = $$ICON_DIR/hicolor/32x32/actions
	actionicons48.files = 	data/48/eqz-account.png \
				data/48/eqz-liabilities.png \
				data/48/eqz-expense.png \
				data/48/eqz-income.png \
				data/48/eqz-transfer.png \
				data/48/eqz-refund-repayment.png \
				data/48/eqz-debt-payment.png \
				data/48/eqz-debt-interest.png \
				data/48/eqz-security.png \
				data/48/eqz-schedule.png \
				data/48/eqz-split-transaction.png \
				data/48/eqz-join-transactions.png \
				data/48/eqz-export.png \
				data/48/eqz-import.png \
				data/48/eqz-edit.png \
				data/48/eqz-tag.png \
				data/48/eqz-balance.png \
				data/48/eqz-ledger.png \
				data/48/eqz-transactions.png \
				data/48/eqz-previous-year.png \
				data/48/eqz-previous-month.png \
				data/48/eqz-next-month.png \
				data/48/eqz-next-year.png \
				data/48/eqz-categories-chart.png \
				data/48/eqz-overtime-chart.png \
				data/48/eqz-categories-report.png \
				data/48/eqz-currency.png \
				data/48/eqz-overtime-report.png
	actionicons48.path = $$ICON_DIR/hicolor/48x48/actions
	actionicons64.files = 	data/64/eqz-account.png \
				data/64/eqz-liabilities.png \
				data/64/eqz-expense.png \
				data/64/eqz-income.png \
				data/64/eqz-transfer.png \
				data/64/eqz-refund-repayment.png \
				data/64/eqz-debt-payment.png \
				data/64/eqz-debt-interest.png \
				data/64/eqz-security.png \
				data/64/eqz-schedule.png \
				data/64/eqz-split-transaction.png \
				data/64/eqz-join-transactions.png \
				data/64/eqz-export.png \
				data/64/eqz-import.png \
				data/64/eqz-edit.png \
				data/64/eqz-tag.png \
				data/64/eqz-balance.png \
				data/64/eqz-ledger.png \
				data/64/eqz-transactions.png \
				data/64/eqz-previous-year.png \
				data/64/eqz-previous-month.png \
				data/64/eqz-next-month.png \
				data/64/eqz-next-year.png \
				data/64/eqz-categories-chart.png \
				data/64/eqz-overtime-chart.png \
				data/64/eqz-categories-report.png \
				data/64/eqz-currency.png \
				data/64/eqz-overtime-report.png
	actionicons64.path = $$ICON_DIR/hicolor/64x64/actions

	INSTALLS += 	target qm data doc desktop mime \ 
			appicon16 appicon22 appicon32 appicon64 appicon128 appiconsvg \
			mimeicon16 mimeicon22 mimeicon32 mimeicon48 mimeicon64 mimeicon128 mimeiconsvg \
			actionicons16 actionicons22 actionicons32 actionicons48 actionicons64 actioniconssvg
			
	!equals($$DESKTOP_ICON_DIR, $$ICON_DIR) {
		desktopappicon64.files = data/64/eqonomize.png
		desktopappicon64.path = $$DESKTOP_ICON_DIR/hicolor/64x64/apps
		INSTALLS += desktopappicon64
	}
	RESOURCES = flags.qrc
} else {
	RESOURCES = data.qrc doc.qrc flags.qrc icons.qrc translations.qrc
	target.path = $$PREFIX/bin
	desktop.files = data/eqonomize.desktop
	desktop.path = $$DESKTOP_DIR
	appicon64.files = data/64/eqonomize.png
	appicon64.path = $$DESKTOP_ICON_DIR/hicolor/64x64/apps
	INSTALLS += target desktop appicon64
}

unix:!android:!macx {
	man.files = data/eqonomize.1
	man.path = $$MAN_DIR/man1
	INSTALLS += man
}

win32: RC_FILE = winicon.rc

