isEmpty(PREFIX) {
	PREFIX = /usr/local
}
isEmpty(TRANSLATIONS_DIR) {
	TRANSLATIONS_DIR = $$PREFIX/share/eqonomize/translations
}
isEmpty(DOCUMENTATION_DIR) {
	DOCUMENTATION_DIR = $$PREFIX/share/doc/eqonomize/html
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
TRANSLATIONS = translations/eqonomize_sv.ts

target.path = $$PREFIX/bin
qm.files = translations/eqonomize_sv.qm
qm.path = $$TRANSLATIONS_DIR
doc.files = doc/html/*
doc.path = $$DOCUMENTATION_DIR
INSTALLS += target qm doc
