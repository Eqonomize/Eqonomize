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


#include <QButtonGroup>
#include <QDateTime>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QRadioButton>
#include <QStringList>
#include <QTextStream>
#include <QVector>
#include <QWidget>
#include <QTreeWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QSaveFile>
#include <QTemporaryFile>
#include <QMessageBox>
#include <QFileDialog>
#include <QPushButton>
#include <QCheckBox>
#include <QCompleter>
#include <QFileSystemModel>
#include <QHash>
#include <QMimeDatabase>

#include "eqonomize.h"
#include "qifimportexport.h"

#include <cmath>
#include <ctime>

#include <QDebug>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
#	define DATE_TO_MSECS(d) QDateTime(d.startOfDay()).toMSecsSinceEpoch()
#else
#	define DATE_TO_MSECS(d) QDateTime(d).toMSecsSinceEpoch()
#endif

extern QString last_document_directory;

ImportQIFDialog::ImportQIFDialog(Budget *budg, QWidget *parent, bool extra_parameters) : QWizard(parent), budget(budg), b_extra(extra_parameters) {

	setWindowTitle(tr("Import QIF file"));
	setModal(true);

	next_id = -1;

	QIFWizardPage *page1 = new QIFWizardPage();
	page1->setTitle(tr("File Selection"));
	page1->setSubTitle(tr("Select a QIF file to import. When you click next, the file be analysed and you might need to answer some questions about the format of the file."));
	setPage(0, page1);
	QGridLayout *layout1 = new QGridLayout(page1);
	layout1->addWidget(new QLabel(tr("File:"), page1), 0, 0);
	QHBoxLayout *layout1h = new QHBoxLayout();
	fileEdit = new QLineEdit(page1);
	QCompleter *completer = new QCompleter(this);
	QFileSystemModel *fsModel = new QFileSystemModel(completer);
	fsModel->setRootPath(QString());
	completer->setModel(fsModel);
	fileEdit->setCompleter(completer);
	layout1h->addWidget(fileEdit);
	fileButton = new QPushButton(LOAD_ICON("document-open"), QString(), page1);
	layout1h->addWidget(fileButton);
	layout1->addLayout(layout1h, 0, 1);

	QIFWizardPage *page1_2 = new QIFWizardPage();
	page1_2->setTitle(tr("Local Definitions"));
	page1_2->setSubTitle(tr("Unknown elements where found in the QIF file. It is possible that this is because of localized type names. Please map them to the correct standard names."));
	setPage(1, page1_2);
	QGridLayout *layout1_2 = new QGridLayout(page1_2);
	defsView = new QTreeWidget(page1_2);
	layout1_2->addWidget(defsView, 0, 0, 1, 2);
	QStringList headers;
	headers << tr("Local Text");
	headers << tr("Standard Text");
	defsView->setColumnCount(2);
	defsView->setHeaderLabels(headers);
	layout1_2->addWidget(new QLabel(tr("Select standard text:"), page1_2), 1, 0);
	defsCombo = new QComboBox(page1_2);
	defsCombo->setEditable(false);
	layout1_2->addWidget(defsCombo, 1, 1);

	QIFWizardPage *page2 = new QIFWizardPage();
	page2->setTitle(tr("Date Format"));
	page2->setSubTitle(tr("The date format in the QIF file is ambiguous. Please select the correct format."));
	setPage(2, page2);
	QGridLayout *layout2 = new QGridLayout(page2);
	layout2->addWidget(new QLabel(tr("Date format:"), page2), 0, 0);
	dateFormatCombo = new QComboBox(page2);
	dateFormatCombo->setEditable(false);
	layout2->addWidget(dateFormatCombo, 0, 1);

	QIFWizardPage *page3 = new QIFWizardPage();
	page3->setTitle(tr("Default Account"));
	page3->setSubTitle(tr("Could not find any account definitions in the QIF file. Please select a default account. It is also possible that this is caused by a localized opening balance text."));
	setPage(3, page3);
	QGridLayout *layout3 = new QGridLayout(page3);
	layout3->addWidget(new QLabel(tr("Default account:"), page3), 0, 0);
	accountCombo = new QComboBox(page3);
	accountCombo->setEditable(false);
	for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
		AssetsAccount *account = *it;
		if(account != budget->balancingAccount && account->accountType() != ASSETS_TYPE_SECURITIES) {
			accountCombo->addItem(account->name(), QVariant::fromValue((void*) account));
		}
	}
	layout3->addWidget(accountCombo, 0, 1);
	layout3->addWidget(new QLabel(tr("Opening balance text:"), page3), 1, 0);
	openingBalanceEdit = new QLineEdit(page3);
	openingBalanceEdit->setText("Opening Balance");
	layout3->addWidget(openingBalanceEdit, 1, 1);

	QIFWizardPage *page4 = new QIFWizardPage();
	page4->setTitle(tr("Import File"));
	page4->setSubTitle(tr("No (further) issues were found. Press finish to import the selected QIF file."));
	setPage(4, page4);
	QGridLayout *layout4 = new QGridLayout(page4);
	ignoreDuplicateTransactionsButton = new QCheckBox(tr("Ignore duplicate transactions"), page4);
	layout4->addWidget(ignoreDuplicateTransactionsButton, 0, 0, 1, -1);


	setOption(QWizard::HaveHelpButton, false);

	page1->setCommitPage(true);
	page1_2->setCommitPage(true);
	page2->setCommitPage(true);
	page3->setCommitPage(true);
	page4->setCommitPage(true);
	page4->setFinalPage(true);

	page1->setComplete(false);
	page1_2->setComplete(true);
	page2->setComplete(true);
	page3->setComplete(true);
	page4->setComplete(true);

	fileEdit->setFocus();

	setButtonText(CommitButton, buttonText(NextButton));
	disconnect(button(CommitButton), SIGNAL(clicked()), this, 0);
	connect(button(CommitButton), SIGNAL(clicked()), this, SLOT(nextClicked()));

	connect(fileEdit, SIGNAL(textChanged(const QString&)), this, SLOT(onFileChanged(const QString&)));
	connect(fileButton, SIGNAL(clicked()), this, SLOT(selectFile()));
	connect(defsView, SIGNAL(itemSelectionChanged()), this, SLOT(defSelectionChanged()));
	connect(defsCombo, SIGNAL(activated(int)), this, SLOT(defSelected(int)));

}
ImportQIFDialog::~ImportQIFDialog() {}

void ImportQIFDialog::showPage(int index) {
	next_id = index;
	QWizard::next();
	next_id = -1;
}
int ImportQIFDialog::nextId() const {
	if(next_id < 0) return  QWizard::nextId();
	return next_id;
}
void ImportQIFDialog::defSelectionChanged() {
	QList<QTreeWidgetItem*> list = defsView->selectedItems();
	if(!list.isEmpty()) {
		QTreeWidgetItem *i = list.first();
		defsCombo->setEnabled(true);
		int type = qi.unknown_defs[qi.unknown_defs_pre[i->text(0)]];
		switch(type) {
			case -2: {defsCombo->setCurrentIndex(0); break;}
			case 1: {defsCombo->setCurrentIndex(1); break;}
			case 10: {defsCombo->setCurrentIndex(2); break;}
			case 11: {defsCombo->setCurrentIndex(3); break;}
			case 2: {defsCombo->setCurrentIndex(4); break;}
			case 12: {defsCombo->setCurrentIndex(5); break;}
			case 9: {defsCombo->setCurrentIndex(6); break;}
			case 13: {defsCombo->setCurrentIndex(7); break;}
			case 14: {defsCombo->setCurrentIndex(8); break;}
			case 3: {defsCombo->setCurrentIndex(9); break;}
			case -1: {defsCombo->setCurrentIndex(10); break;}
		}
	} else {
		defsCombo->setEnabled(false);
	}
}
void ImportQIFDialog::defSelected(int index) {
	QList<QTreeWidgetItem*> list = defsView->selectedItems();
	if(!list.isEmpty()) {
		QTreeWidgetItem *i = list.first();
		int type = -2;
		switch(index) {
			case 0: {type = -2; break;}
			case 1: {type = 1; break;}
			case 2: {type = 10; break;}
			case 3: {type = 11; break;}
			case 4: {type = 2; break;}
			case 5: {type = 12; break;}
			case 6: {type = 9; break;}
			case 7: {type = 13; break;}
			case 8: {type = 14; break;}
			case 9: {type = 3; break;}
			case 10: {type = -1; break;}
		}
		qi.unknown_defs[qi.unknown_defs_pre[i->text(0)]] = type;
		i->setText(1, defsCombo->itemText(index));
	}
}
void ImportQIFDialog::onFileChanged(const QString &str) {
	((QIFWizardPage*) page(0))->setComplete(!str.isEmpty());
}
void ImportQIFDialog::selectFile() {
	QMimeDatabase db;
	QString url = QFileDialog::getOpenFileName(this, QString(), fileEdit->text().isEmpty() ? last_document_directory + "/" : fileEdit->text().trimmed(), db.mimeTypeForName("application/x-qw").filterString());
	if(!url.isEmpty()) fileEdit->setText(url);
}
void ImportQIFDialog::nextClicked() {
	if(currentId() == 0 || currentId() == 1) {
		bool b_page1 = (currentId() == 1);
		QString url = fileEdit->text().trimmed();
		if(!b_page1) {
			if(url.isEmpty()) {
				QMessageBox::critical(this, tr("Error"), tr("A file must be selected."));
				fileEdit->setFocus();
				return;
			}
			QFileInfo info(url);
			if(info.isDir()) {
				QMessageBox::critical(this, tr("Error"), tr("Selected file is a directory."));
				fileEdit->setFocus();
				return;
			} else if(!info.exists()) {
				QMessageBox::critical(this, tr("Error"), tr("Selected file does not exist."));
				fileEdit->setFocus();
				return;
			}
			url = info.absoluteFilePath();
			fileEdit->setText(url);
		}

		QFile file(url);
		if(!file.open(QIODevice::ReadOnly) ) {
			QMessageBox::critical(this, tr("Error"), tr("Couldn't open %1 for reading.").arg(url));
			return;
		} else if(!file.size()) {
			QMessageBox::critical(this, tr("Error"), tr("Error reading %1.").arg(url));
			file.close();
			return;
		}
		QTextStream fstream(&file);
		importQIF(fstream, true, qi, budget, ignoreDuplicateTransactionsButton->isChecked());
		int ps = qi.p1 + qi.p2 + qi.p3 + qi.p4;
		file.close();
		if(b_page1 && (int) qi.unknown_defs.count() > defsView->topLevelItemCount()) {
			QMap<QString, QString>::iterator it_e = qi.unknown_defs_pre.end();
			QMap <QString, bool> unknown_defs_old;
			QTreeWidgetItemIterator it(defsView);
			QTreeWidgetItem *qli = *it;
			while(qli) {
				unknown_defs_old[qli->text(0)] = true;
				++it;
				qli = *it;
			}
			qli = NULL;
			for(QMap<QString, QString>::iterator it = qi.unknown_defs_pre.begin(); it != it_e; ++it) {
				if(!unknown_defs_old.contains(it.key())) {
					QTreeWidgetItem *tmp_i = new QTreeWidgetItem(defsView);
					tmp_i->setText(0, it.key());
					tmp_i->setText(1, tr("Unknown"));
					if(!qli) qli = tmp_i;
				}
			}
			qli->setSelected(true);
			defsCombo->setCurrentIndex(0);
			return;
		} else if(!b_page1 && qi.unknown_defs.count() > 0) {
			defsCombo->addItem(tr("Unknown"));
			defsCombo->addItem(tr("Account"));
			defsCombo->addItem(tr("Bank"));
			defsCombo->addItem(tr("Cash"));
			defsCombo->addItem(tr("Cat (Category)"));
			defsCombo->addItem(tr("CCard (Credit Card)"));
			defsCombo->addItem(tr("Invst (Investment)"));
			defsCombo->addItem(tr("Oth A (Other Assets)"));
			defsCombo->addItem(tr("Oth L (Other Liabilities)"));
			defsCombo->addItem(tr("Security"));
			defsCombo->addItem(tr("Other"));
			defsCombo->setCurrentIndex(0);
			QMap<QString, QString>::iterator it_e = qi.unknown_defs_pre.end();
			for(QMap<QString, QString>::iterator it = qi.unknown_defs_pre.begin(); it != it_e; ++it) {
				QTreeWidgetItem *tmp_i = new QTreeWidgetItem(defsView);
				tmp_i->setText(0, it.key());
				tmp_i->setText(1, tr("Unknown"));
			}
			if(defsView->topLevelItem(0)) {
				defsView->topLevelItem(0)->setSelected(true);
			}
		} else {
			if(ps == 0) {
				QMessageBox::critical(this, tr("Error"), tr("Unrecognized date format."));
				return;
			}
			if(ps > 1) {
				dateFormatCombo->clear();
				if(qi.p1) {
					QString date_format = qi.lz == 0 ? "M" : "MM";
					if(qi.separator > 0) date_format += qi.separator;
					date_format += qi.lz == 0 ? "D" : "DD";
					if(qi.separator > 0) date_format += qi.separator;
					date_format += "YY";
					if(qi.ly) date_format += "YY";
					dateFormatCombo->addItem(date_format);
				}
				if(qi.p2) {
					QString date_format = qi.lz == 0 ? "D" : "DD";
					if(qi.separator > 0) date_format += qi.separator;
					date_format += qi.lz == 0 ? "M" : "MM";
					if(qi.separator > 0) date_format += qi.separator;
					date_format += "YY";
					if(qi.ly) date_format += "YY";
					dateFormatCombo->addItem(date_format);
				}
				if(qi.p3) {
					QString date_format = "YY";
					if(qi.ly) date_format += "YY";
					if(qi.separator > 0) date_format += qi.separator;
					date_format += qi.lz == 0 ? "M" : "MM";
					if(qi.separator > 0) date_format += qi.separator;
					date_format += qi.lz == 0 ? "D" : "DD";
					dateFormatCombo->addItem(date_format);
				}
				if(qi.p4) {
					QString date_format = "YY";
					if(qi.ly) date_format += "YY";
					if(qi.separator > 0) date_format += qi.separator;
					date_format += qi.lz == 0 ? "D" : "DD";
					if(qi.separator > 0) date_format += qi.separator;
					date_format += qi.lz == 0 ? "M" : "MM";
					dateFormatCombo->addItem(date_format);
				}
				showPage(2);
				return;
			} else {
				if(!qi.had_transaction || qi.account_defined) {
					showPage(4);
					button(CommitButton)->setEnabled(false);
				} else {
					showPage(3);
				}
				return;
			}
		}
	} else if(currentId() == 2) {
		bool p1 = false, p2 = false, p3 = false, p4 = false;
		int p[4];
		int i = 0;
		if(qi.p1) {p[i] = 1; i++;}
		if(qi.p2) {p[i] = 2; i++;}
		if(qi.p3) {p[i] = 3; i++;}
		if(qi.p4) {p[i] = 4; i++;}
		switch(p[dateFormatCombo->currentIndex()]) {
			case 1: {p1 = true; break;}
			case 2: {p2 = true; break;}
			case 3: {p3 = true; break;}
			case 4: {p4 = true; break;}
		}
		qi.p1 = p1; qi.p2 = p2; qi.p3 = p3; qi.p4 = p4;
		if(qi.lz < 0) qi.lz = 1;
		if(!qi.had_transaction || qi.account_defined) {
			button(CommitButton)->setEnabled(false);
			showPage(4);
			return;
		}
	} else if(currentId() == 3) {
		if(accountCombo->currentData().isValid()) qi.current_account = (AssetsAccount*) accountCombo->currentData().value<void*>();
		button(CommitButton)->setEnabled(false);
	}
	QWizard::next();
}

void ImportQIFDialog::accept() {
	QString url = fileEdit->text().trimmed();
	if(url.isEmpty()) {
		return;
	}
	QFile file(url);
	if(!file.open(QIODevice::ReadOnly) ) {
		QMessageBox::critical(this, tr("Error"), tr("Couldn't open %1 for reading.").arg(url));
		return;
	} else if(!file.size()) {
		QMessageBox::critical(this, tr("Error"), tr("Error reading %1.").arg(url));
		file.close();
		return;
	}

	QFileInfo fileInfo(url);
	last_document_directory = fileInfo.absoluteDir().absolutePath();

	QTextStream fstream(&file);
	importQIF(fstream, false, qi, budget, ignoreDuplicateTransactionsButton->isChecked());
	file.close();
	QString info = "";
	info += tr("Successfully imported %n transaction(s).", "", qi.transactions);
	if(qi.accounts > 0) {
		info += "\n";
		info += tr("Successfully imported %n account(s).", "", qi.accounts);
	}
	if(qi.categories > 0) {
		info += "\n";
		info += tr("Successfully imported %n category/categories.", "", qi.categories);
	}
	if(qi.duplicates > 0) {
		info += "\n";
		info += tr("%n duplicate transaction(s) was ignored.", "", qi.duplicates);
	}
	if(qi.failed_transactions > 0) {
		info += "\n";
		info += tr("Failed to import %n transaction(s).", "", qi.failed_transactions);
	}
	if(qi.securities > 0) {
		info += "\n";
		info += tr("%n security/securities were not imported.", "Financial security (e.g. stock, mutual fund)", qi.securities);
	}
	if(qi.security_transactions > 0) {
		info += "\n";
		info += tr("%n security transaction(s) were not imported.", "Financial security (e.g. stock, mutual fund)", qi.security_transactions);
	}
	QMessageBox::information(this, tr("Information"), info);
	if(qi.transactions || qi.accounts || qi.categories) return QWizard::accept();
	return QWizard::reject();
}

ExportQIFDialog::ExportQIFDialog(Budget *budg, QWidget *parent, bool extra_parameters) : QDialog(parent), budget(budg), b_extra(extra_parameters) {

	setWindowTitle(tr("Export QIF File"));
	setModal(true);

	QVBoxLayout *box1 = new QVBoxLayout(this);

	QGridLayout *grid = new QGridLayout();
	box1->addLayout(grid);

	grid->addWidget(new QLabel(tr("Account:"), this), 0, 0);
	accountCombo = new QComboBox(this);
	accountCombo->setEditable(false);
	for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
		AssetsAccount *account = *it;
		if(account != budget->balancingAccount) {
			accountCombo->addItem(account->name(), QVariant::fromValue((void*) account));
		}
	}
	accountCombo->addItem(tr("All", "All accounts"), QVariant::fromValue((void*) NULL));
	grid->addWidget(accountCombo, 0, 1);
	accountCombo->setFocus();

	grid->addWidget(new QLabel(tr("Date format:"), this), 1, 0);
	dateFormatCombo = new QComboBox(this);
	dateFormatCombo->setEditable(false);
	dateFormatCombo->addItem("Standard (YYYY-MM-DD)");
	dateFormatCombo->addItem("Local");
	dateFormatCombo->addItem("US (MM/DD/YY)");
	grid->addWidget(dateFormatCombo, 1, 1);

	grid->addWidget(new QLabel(tr("Value format:"), this), 2, 0);
	valueFormatCombo = new QComboBox(this);
	valueFormatCombo->setEditable(false);
	valueFormatCombo->addItem("1,000,000.00");
	valueFormatCombo->addItem("1.000.000,00");
	grid->addWidget(valueFormatCombo, 2, 1);

	grid->addWidget(new QLabel(tr("File:"), this), 3, 0);
	QHBoxLayout *layouth = new QHBoxLayout();
	fileEdit = new QLineEdit(this);
	QCompleter *completer = new QCompleter(this);
	QFileSystemModel *fsModel = new QFileSystemModel(completer);
	fsModel->setRootPath(QString());
	completer->setModel(fsModel);
	fileEdit->setCompleter(completer);
	layouth->addWidget(fileEdit);
	fileButton = new QPushButton(LOAD_ICON("document-open"), QString(), this);
	layouth->addWidget(fileButton);
	grid->addLayout(layouth, 3, 1);

	buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Cancel)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);

	connect(fileEdit, SIGNAL(textChanged(const QString&)), this, SLOT(onFileChanged(const QString&)));
	connect(fileButton, SIGNAL(clicked()), this, SLOT(selectFile()));

}
ExportQIFDialog::~ExportQIFDialog() {}

void ExportQIFDialog::onFileChanged(const QString &str) {
	buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!str.isEmpty());
}
void ExportQIFDialog::selectFile() {
	QFileDialog fileDialog(this);
	QMimeDatabase db;
	fileDialog.setNameFilter(db.mimeTypeForName("application/x-qw").filterString());
	fileDialog.setDefaultSuffix(db.mimeTypeForName("application/x-qw").preferredSuffix());
	fileDialog.setAcceptMode(QFileDialog::AcceptSave);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
	fileDialog.setSupportedSchemes(QStringList("file"));
#endif
	fileDialog.setOption(QFileDialog::DontConfirmOverwrite, true);
	fileDialog.setDirectory(fileEdit->text().isEmpty() ? last_document_directory + "/" : fileEdit->text().trimmed());
	if(fileDialog.exec()) {
		QStringList urls = fileDialog.selectedFiles();
		if(!urls.isEmpty()) fileEdit->setText(urls[0]);
	}
}
void ExportQIFDialog::accept() {

	QString url = fileEdit->text().trimmed();
	if(url.isEmpty()) {
		return;
	}
	AssetsAccount *account = NULL;
	if(accountCombo->currentData().isValid()) account = (AssetsAccount*) accountCombo->currentData().value<void*>();
	qi.current_account = account;
	qi.value_format = valueFormatCombo->currentIndex() + 1;
	qi.date_format = dateFormatCombo->currentIndex() + 1;
	QFileInfo info(url);
	if(info.isDir()) {
		QMessageBox::critical(this, tr("Error"), tr("Selected file is a directory."));
		fileEdit->setFocus();
		return;
	}
	url = info.absoluteFilePath();
	fileEdit->setText(url);
	if(QFile::exists(url)) {
		if(QMessageBox::warning(this, tr("Overwrite"), tr("The selected file already exists. Would you like to overwrite the old copy?"), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
	}
	QSaveFile ofile(url);
	ofile.open(QIODevice::WriteOnly);
	ofile.setPermissions((QFile::Permissions) 0x0660);
	if(!ofile.isOpen()) {
		ofile.cancelWriting();
		QMessageBox::critical(this, tr("Error"), tr("Couldn't open file for writing."));
		return;
	}

	QFileInfo fileInfo(url);
	last_document_directory = fileInfo.absoluteDir().absolutePath();

	QTextStream stream(&ofile);
	exportQIF(stream, qi, budget, true);
	if(!ofile.commit()) {
		QMessageBox::critical(this, tr("Error"), tr("Error while writing file; file was not saved."));
		return;
	}
	return QDialog::accept();

}


QDate readQIFDate(const QString &str, const QString &date_format, const QString &alt_date_format) {
	QDate date = QDate::fromString(str, date_format);
	if(!date.isValid() && !alt_date_format.isEmpty()) {
		date = QDate::fromString(str, alt_date_format);
		if(date.year() < 1970 && alt_date_format.count('y') < 4) {
			date = date.addYears(100);
		}
	} else if(date.year() < 1970 && date_format.count('y') < 4) {
		date = date.addYears(100);
	}
	return date;
}
double readQIFValue(const QString &str, int value_format) {
	if(value_format == 2) {
		QString str2 = str;
		str2.replace(".", "");
		str2.replace(",", ".");
		return str2.toDouble();
	} else if(value_format == 1) {
		QString str2 = str;
		str2.replace(",", "");
		return str2.toDouble();
	}
	return str.toDouble();
}

//p1 MDY
//p2 DMY
//p3 YMD
//p4 YDM
void testQIFDate(const QString &str, bool &p1, bool &p2, bool &p3, bool &p4, bool &ly, char &separator, int & lz) {
	if(separator < 0) {
		for(int i = 0; i < (int) str.length(); i++) {
			if(str[i] < '0' || str[i] > '9') {
				separator = str[i].toLatin1();
				break;
			}
		}
		if(separator < 0) separator = 0;
		p1 = (separator != 0);
		p2 = (separator != 0);
		p3 = true;
		p4 = (separator != 0);
		ly = false;
	}
	if(p1 + p2 + p3 + p4 <= 1) {
		lz = 1;
		return;
	}
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
	QStringList strlist = str.split(separator, Qt::SkipEmptyParts);
#else
	QStringList strlist = str.split(separator, QString::SkipEmptyParts);
#endif
	if(strlist.count() == 2 && (p1 || p2)) {
		int i = strlist[1].indexOf('\'');
		if(i >= 0) {
			strlist.append(strlist[1]);
			strlist[2].remove(0, i + 1);
			strlist[1].truncate(i);
			p3 = false;
			p4 = false;
			ly = false;
		}
	}
	if(strlist.count() < 3) return;
	if(p1 || p2) {
		int v1 = strlist[0].toInt();
		if(v1 > 12) p1 = false;
		if(v1 > 31 || v1 < 1) {
			p2 = false;
			if(v1 >= 100) ly = true;
			else ly = false;
		}
	}
	int v2 = strlist[1].toInt();
	if(v2 > 12) {p2 = false; p3 = false;}
	int v3 = strlist[2].toInt();
	if(v3 > 12) p4 = false;
	if(v3 > 31 || v3 < 1) {
		p3 = false;
		if(v3 >= 100) ly = true;
		else ly = false;
	}
	if(strlist[1].length() == 1) lz = 0;
	else if(strlist[1][0] == '0') lz = 1;
	else if(!p3 && !p4 && strlist[0].length() == 1) lz = 0;
	else if(!p3 && !p4 && strlist[0][0] == '0') lz = 1;
	else if(!p1 && !p2 && strlist[2].length() == 1) lz = 0;
	else if(!p1 && !p2 && strlist[2][0] == '0') lz = 1;
}

void testQIFValue(const QString &str, int &value_format) {
	if(value_format == 0) {
		int i = str.lastIndexOf('.');
		int i2 = str.lastIndexOf(',');
		if(i2 >= 0 && i >= 0) {
			if(i2 > i) value_format = 2;
			else value_format = 1;
			return;
		}
		if(i >= 0) {
			value_format = 1;
			return;
		}
		if(i2 >= 0) {
			value_format = 2;
			return;
		}
	}
}

bool importQIFFile(Budget *budget, QWidget *parent, bool extra_parameters) {
	ImportQIFDialog *dialog = new ImportQIFDialog(budget, parent, extra_parameters);
	bool ret = (dialog->exec() == QDialog::Accepted);
	dialog->deleteLater();
	return ret;
}

struct qif_split {
	QString memo, category, subcategory;
	double value, percentage;
};

void importQIF(QTextStream &fstream, bool test, qif_info &qi, Budget *budget, bool ignore_duplicates) {
	QDate date;
	QString memo, description, payee, category, subcategory, atype, atype_bak, name, subname, ticker_symbol, security;
	bool incomecat = false;
	double value = 0.0;
	//double commission = 0.0, price = 0.0, sec_amount = 0.0;
	QString line = fstream.readLine().trimmed(), line_bak;
	QString date_format = "", alt_date_format = "";
	QList<Transfer*> transfers;
	QList<Transfer*> previous_transfers;
	QVector<qif_split> splits;
	qif_split *current_split = NULL;
	int type = -1;
	QHash<QString, IncomesAccount*> def_income_cats;
	QHash<QString, ExpensesAccount*> def_expense_cats;
	if(test) {
		qi.current_account = NULL;
		qi.unhandled = false;
		qi.unknown = false;
		qi.had_type = false; qi.had_type_def = false; qi.had_account_def = false;
		qi.account_defined = false;
		qi.had_transaction = false;
		qi.value_format = 0;
		qi.shares_format = 0;
		qi.price_format = 0;
		qi.percentage_format = 0;
		qi.separator = -1;
		qi.opening_balance_str = "opening balance";
		qi.p1 = true;
		qi.p2 = true;
		qi.p3 = true;
		qi.p4 = true;
		qi.ly = true;
		qi.lz = -1;
	} else {
		qi.duplicates = 0;
		qi.transactions = 0;
		qi.securities = 0;
		qi.security_transactions = 0;
		qi.failed_transactions = 0;
		qi.accounts = 0;
		qi.categories = 0;
		qi.opening_balance_str = qi.opening_balance_str.toLower();
		if(qi.p1) {
			date_format += qi.lz == 0 ? "M" : "MM";
			if(qi.separator > 0) date_format += qi.separator;
			date_format += qi.lz == 0 ? "d" : "dd";
			if(qi.separator > 0) date_format += qi.separator;
			if(qi.ly) {
				date_format += "yyyy";
			} else {
				if(qi.separator > 0) {
					alt_date_format = date_format;
					alt_date_format += '\'';
					alt_date_format += "yy";
				}
				date_format += "yy";
			}
		} else if(qi.p2) {
			date_format += qi.lz == 0 ? "d" : "dd";
			if(qi.separator > 0) date_format += qi.separator;
			date_format += qi.lz == 0 ? "M" : "MM";
			if(qi.separator > 0) date_format += qi.separator;
			if(qi.ly) {
				date_format += "yyyy";
			} else {
				if(qi.separator > 0) {
					alt_date_format = date_format;
					alt_date_format += '\'';
					alt_date_format += "yy";
				}
				date_format += "yy";
			}
		} else if(qi.p3) {
			if(qi.ly) date_format += "yyyy";
			else date_format += "yy";
			if(qi.separator > 0) date_format += qi.separator;
			date_format += qi.lz == 0 ? "M" : "MM";
			if(qi.separator > 0) date_format += qi.separator;
			date_format += qi.lz == 0 ? "d" : "dd";
		} else if(qi.p4) {
			if(qi.ly) date_format += "yyyy";
			else date_format += "yy";
			if(qi.separator > 0) date_format += qi.separator;
			date_format += qi.lz == 0 ? "d" : "dd";
			if(qi.separator > 0) date_format += qi.separator;
			date_format += qi.lz == 0 ? "M" : "MM";
		}
	}
	QMap<QDate, qint64> datestamps;
	while(!line.isNull()) {
		if(!line.isEmpty()) {
			char field = line[0].toLatin1();
			line.remove(0, 1);
			line = line.trimmed();
			switch(field) {
				case '!': {
					line_bak = line.trimmed();
					line = line_bak.toLower();
					if(qi.unknown_defs.contains(line)) {
						type = qi.unknown_defs[line];
						if(type > 1) type = -1;
						if(type == 1) {
							qi.had_account_def = true;
						} else if(type == -1) {
							qi.had_type_def = true;
							qi.unhandled = true;
						} else if(type == -2) {
							qi.unknown = true;
						}
					} else if(line == "account") {
						qi.had_account_def = true;
						type = 1;
					} else if(line.startsWith("option")) {
					} else if(line.startsWith("clear")) {
					} else {
						bool is_type = false;
						if(line.startsWith("type:") || line.startsWith("type ")) {
							qi.had_type = true;
							line.remove(0, 5);
							line = line.trimmed();
							is_type = true;
						} else {
							int i = line.indexOf(":");
							if(i < 0) i = line.indexOf(" ");
							if(i >= 0) {
								line.remove(0, i + 1);
								is_type = true;
							}
						}
						if(is_type) {
							if(qi.unknown_defs.contains(line)) {
								type = qi.unknown_defs[line];
								if(type == 1) {
									type = -1;
								}
								if(type == -1) {
									qi.had_type_def = true;
									qi.unhandled = true;
								} else if(type >= 9) {
									qi.had_type_def = true;
								} else if(type == -2) {
									qi.unknown = true;
								}
							} else if(line == "cat") {
								type = 2;
							} else if(line == "security") {
								type = 3;
							} else if(line == "invst") {
								qi.had_type_def = true;
								type = 9;
							} else if(line.startsWith("ban")) {
								qi.had_type_def = true;
								type = 10;
							} else if(line == "cash" || line == "kas") {
								qi.had_type_def = true;
								type = 11;
							} else if(line == "ccard") {
								qi.had_type_def = true;
								type = 12;
							} else if(line == "oth a" || line == "ov b") {
								qi.had_type_def = true;
								type = 13;
							} else if(line == "oth l" || line == "ov s") {
								qi.had_type_def = true;
								type = 14;
							} else if(line == "prices" || line == "budget" || line == "tax" || line == "invoice" || line == "bill" || line == "memorized" || line == "class") {
								qi.had_type_def = true;
								qi.unhandled = true;
								type = -1;
							} else {
								if(test) {
									int i = line_bak.indexOf(":");
									if(i < 0) i = line_bak.indexOf(" ");
									if(i >= 0) {
										line_bak.remove(0, i + 1);
									}
									qi.unknown_defs_pre[line_bak] = line;
									qi.unknown_defs[line] = -2;
								}
								qi.unknown = true;
								type = -2;
							}
						} else {
							if(test) {
								qi.unknown_defs_pre[line_bak] = line;
								qi.unknown_defs[line] = -2;
							}
							qi.unknown = true;
							type = -3;
						}
					}
					break;
				}
				case '/': {
					//Balance date (Account)
					if(type == 1) {
						/*if(test) testQIFDate(line, qi.p1, qi.p2, qi.p3, qi.p4, qi.ly, qi.separator);
						else date = readQIFDate(line, date_format, alt_date_format);*/
					}
					break;
				}
				case '%': {
					//Percentage of split if percentages are used (Transaction)
					if(type >= 10) {
						if(test) testQIFValue(line, qi.percentage_format);
						else if(current_split) current_split->percentage = readQIFValue(line, qi.percentage_format);
					}
					break;
				}
				case -35: {}
				case -36: {}
				case '$': {
					//Balance (Account), amount transfered (Security transaction), amount of split (Memorized, Transaction)
					if(type == 9) {
						if(test) testQIFValue(line, qi.value_format);
						//else sec_amount = readQIFValue(line, qi.value_format);
					} else if(type >= 10) {
						if(test) testQIFValue(line, qi.value_format);
						else if(current_split) current_split->value = readQIFValue(line, qi.value_format);
					}
					break;
				}
				case 1: {}
				case 2: {}
				case 3: {}
				case 4: {}
				case 5: {}
				case 6: {}
				case 7: {
					//Amortization (Memorized)
					break;
				}
				case 'A': {
					//Address (Memorized, Transaction)
					break;
				}
				case 'B': {
					//Budget amount (Category, Budget), balance (account)
					break;
				}
				case 'C': {
					//Cleared status (Security transaction, Memorized, Transaction)
					break;
				}
				case 'D': {
					//Description (Account, Category, Class, Budget)
					if(type == 1 || type == 2 || type == 3) description = line;
					//Date (Security transaction, Transaction)
					else if(type >= 9) {
						if(test) {
							if(qi.p1 + qi.p2 + qi.p3 + qi.p4 > 1 || qi.lz < 0) testQIFDate(line, qi.p1, qi.p2, qi.p3, qi.p4, qi.ly, qi.separator, qi.lz);
						} else {
							date = readQIFDate(line, date_format, alt_date_format);
						}
					}
					break;
				}
				case 'E': {
					//Expense category (Category) or memo in split (Memorized, Transaction)
					if(type == 2) incomecat = false;
					else if(type >= 10 && !test && current_split) current_split->memo = line;
					break;
				}
				case 'F': {
					//Reimbursable business expense flag (Transaction)
					break;
				}
				case 'I': {
					//Income category (Category) or price (Security transaction)
					if(type == 2) incomecat = true;
					else if(type == 9) {
						if(test) testQIFValue(line, qi.price_format);
						//else price = readQIFValue(line, qi.price_format);
					}
					break;
				}
				case 'K': {
					//Memorized
					break;
				}
				case 'L': {
					//Category (Memorized, Security transaction, Transaction) or credit limit (Account)
					if(type >= 9) {
						int i = line.indexOf('/');
						if(i >= 0) line.truncate(i);
						category = line;
						i = line.indexOf(':');
						if(i >= 0) {
							bool is_transfer = line.length() >= 2 && line[0] == '[' && line.endsWith("]");
							if(is_transfer) line.truncate(line.length() - 1);
							category.truncate(i);
							category = category.trimmed();
							if(is_transfer) category += "]";
							subcategory = line;
							subcategory.remove(0, i + 1);
							subcategory = subcategory.trimmed();
						}
					}
					break;
				}
				case 'M': {
					//Memo (Memorized, Security transaction, Transaction)
					if(type >= 9) memo = line;
					break;
				}
				case 'N': {
					//Name (Class, Account, Category, Budget, Security) or action (Security transaction) or num (Transaction)
					if(type == 1 || type == 2 || type == 3) name = line;
					else if(type == 9) {
					}
					break;
				}
				case 'O': {
					//Commission (Security transaction)
					if(type == 9) {
						if(test) testQIFValue(line, qi.value_format);
						//else commission = readQIFValue(line, qi.value_format);
					}
					break;
				}
				case 'P': {
					//Payee (Memorized, Security transaction, Transaction)
					if(type >= 9) payee = line;
					break;
				}
				case 'Q': {
					//Quantity (Security transaction)
					if(type == 9) {
						if(test) testQIFValue(line, qi.shares_format);
						else value = readQIFValue(line, qi.shares_format);
					}
					break;
				}
				case 'R': {
					//Tax schedule information (Category)
					break;
				}
				case 'S': {
					//Category/class in split (Memorized, Transaction), stock ticker symbol (Security)
					if(type == 3) {
						ticker_symbol = line;
					} else if(type >= 10 && !test) {
						splits.push_back(qif_split());
						current_split = &splits.last();
						current_split->value = 0.0;
						current_split->percentage = 0.0;
						int i = line.indexOf('/');
						if(i >= 0) line.truncate(i);
						current_split->category = line;
						i = line.indexOf(':');
						if(i >= 0) {
							bool is_transfer = line.length() >= 2 && line[0] == '[' && line.endsWith("]");
							if(is_transfer) line.truncate(line.length() - 1);
							current_split->category.truncate(i);
							current_split->category = current_split->category.trimmed();
							if(is_transfer) current_split->category += "]";
							current_split->subcategory = line;
							current_split->subcategory.remove(0, i + 1);
							current_split->subcategory = current_split->subcategory.trimmed();
						}
					}
					break;
				}
				case 'T': {
					//Type (Account, Security), tax related (Category)
					if(type == 1 || type == 3) {
						atype_bak = line;
						atype = line.toLower();
					}
					//Value (Memorized, Security transaction, Transaction)
					else if(type >= 9) {
						if(test) testQIFValue(line, qi.value_format);
						else value = readQIFValue(line, qi.value_format);
					}
					break;
				}
				case 'U': {
					//Value (Memorized, Security transaction, Transaction)
					if(type >= 9) {
						if(test) testQIFValue(line, qi.value_format);
						else value = readQIFValue(line, qi.value_format);
					}
					break;
				}
				case 'X': {
					//? (Account), small business extensions (Transaction)
					break;
				}
				case 'Y': {
					//Security (Security transaction)
					if(type == 9) {
						security = line;
					}
					break;
				}
				case '^': {
					//End of entry
					if(type >= 10) {
						//Transaction
						bool is_transfer = category.length() >= 2 && category[0] == '[' && category.endsWith("]");
						QString payee_lower = payee.toLower();
						bool is_opening_balance = splits.empty() && is_transfer && (payee_lower == qi.opening_balance_str || payee_lower == "opening balance" || payee_lower == "opening");
						if(test && !qi.account_defined && is_opening_balance && !qi.had_transaction) qi.account_defined = true;
						if(!test && !splits.empty()) {
							if(!date.isValid() || !qi.current_account) {
								qi.failed_transactions++;
							} else {
								MultiItemTransaction *split = new MultiItemTransaction(budget, date, qi.current_account, memo);
								QVector<qif_split>::size_type c = splits.count();
								for(QVector<qif_split>::size_type i = 0; i < c; i++) {
									current_split = &splits[i];
									bool is_transfer = current_split->category.length() >= 2 && current_split->category[0] == '[' && current_split->category.endsWith("]");
									if(current_split->percentage != 0.0 && current_split->value == 0.0) {
										if(i == c - 1) current_split->value = value;
										else current_split->value = (value * current_split->percentage) / 100;
									}
									value -= current_split->value;
									if(!test && is_transfer) {
										//Transfer
										current_split->category.remove(0, 1);
										current_split->category.truncate(current_split->category.length() - 1);
										if(current_split->category.isEmpty()) {
											current_split->category = Budget::tr("Unnamed");
										}
										AssetsAccount *acc = budget->findAssetsAccount(current_split->category);
										if(!acc) {
											acc = new AssetsAccount(budget, ASSETS_TYPE_CURRENT, current_split->category);
											budget->addAccount(acc);
											qi.accounts++;
										}
										Transfer *tra = NULL;
										if(current_split->value < 0.0) tra = new Transfer(budget, -current_split->value, date, qi.current_account, acc, current_split->memo);
										else tra = new Transfer(budget, current_split->value, date, acc, qi.current_account, current_split->memo);
										bool duplicate = false;
										QList<Transfer*>::iterator it_e = previous_transfers.end();
										for(QList<Transfer*>::iterator it = previous_transfers.begin(); it != it_e; ++it) {
											if(tra->equals(*it, false)) {
												duplicate = true;
												break;
											}
										}
										if(duplicate) {
											delete tra;
										} else if(ignore_duplicates && budget->findDuplicateTransaction(tra)) {
											qi.duplicates++;
											delete tra;
										} else {
											split->addTransaction(tra);
											transfers.append(tra);
										}
									} else {
										bool empty = current_split->category.isEmpty();
										bool b_exp = current_split->value < 0.0;
										if(empty) {
											current_split->category = Budget::tr("Uncategorized");
										}
										CategoryAccount *cat = NULL, *parent_cat = NULL;
										if(!b_exp) parent_cat = budget->findIncomesAccount(current_split->category, NULL);
										else parent_cat = budget->findExpensesAccount(current_split->category, NULL);
										if(!empty && !parent_cat) {
											if(!b_exp) {
												QHash<QString, ExpensesAccount*>::const_iterator it = def_expense_cats.find(current_split->category);
												if(it != def_expense_cats.constEnd()) parent_cat = *it;
											} else {
												QHash<QString, IncomesAccount*>::const_iterator it = def_income_cats.find(current_split->category);
												if(it != def_income_cats.constEnd()) parent_cat = *it;
											}
											if(parent_cat) b_exp = !b_exp;
										}
										if(!parent_cat) {
											if(!b_exp) parent_cat = new IncomesAccount(budget, current_split->category);
											else parent_cat = new ExpensesAccount(budget, current_split->category);
											budget->addAccount(parent_cat);
											qi.categories++;
										}
										if(subcategory.isEmpty()) {
											cat = parent_cat;
											parent_cat = NULL;
										} else {
											if(!b_exp) cat = budget->findIncomesAccount(current_split->subcategory, parent_cat);
											else cat = budget->findExpensesAccount(current_split->subcategory, parent_cat);
											if(!cat) {
												if(!b_exp) cat = new IncomesAccount(budget, current_split->subcategory);
												else cat = new ExpensesAccount(budget, current_split->subcategory);
												cat->setParentCategory(parent_cat);
												budget->addAccount(cat);
												qi.categories++;
											}
										}
										if(b_exp) {
											//Expense
											Expense *exp = new Expense(budget, -current_split->value, date, (ExpensesAccount*) cat, qi.current_account, current_split->memo);
											if(value > 0.0) exp->setQuantity(-1.0);
											if(ignore_duplicates && budget->findDuplicateTransaction(exp)) {
												qi.duplicates++;
												delete exp;
											} else {
												split->addTransaction(exp);
											}
										} else {
											//Income
											Income *inc = new Income(budget, current_split->value, date, (IncomesAccount*) cat, qi.current_account, current_split->memo);
											if(value < 0.0) inc->setQuantity(-1.0);
											if(ignore_duplicates && budget->findDuplicateTransaction(inc)) {
												qi.duplicates++;
												delete inc;
											} else {
												split->addTransaction(inc);
											}
										}
									}
								}
								if(!payee.isEmpty()) split->setPayee(payee);
								split->setTimestamp(datestamps.contains(split->date()) ? datestamps[split->date()] + 1 : DATE_TO_MSECS(split->date()) / 1000);
								datestamps[split->date()] = split->timestamp();
								if(split->count() >= 2) {
									budget->addSplitTransaction(split);
									qi.transactions++;
								} else if(split->count() == 1) {
									budget->addTransaction(split->at(0));
									qi.transactions++;
									split->clear();
									delete split;
								} else {
									delete split;
								}
							}
						} else if(!test && is_transfer) {
							//Transfer
							category.remove(0, 1);
							category.truncate(category.length() - 1);
							if(category.isEmpty()) {
								category = Budget::tr("Unnamed");
							}
							AssetsAccount *acc = budget->findAssetsAccount(category);
							if(is_opening_balance || acc == qi.current_account) {
								if(!acc) {
									AssetsType account_type = ASSETS_TYPE_OTHER;
									if(type == 11) account_type = ASSETS_TYPE_CASH;
									else if(type == 12) account_type = ASSETS_TYPE_CREDIT_CARD;
									else if(type == 10) account_type = ASSETS_TYPE_CURRENT;
									else if(type == 14) account_type = ASSETS_TYPE_LIABILITIES;
									acc = new AssetsAccount(budget, account_type, category);
									budget->addAccount(acc);
									qi.accounts++;
								}
								if(!budget->accountHasTransactions(acc) && acc->accountType() != ASSETS_TYPE_SECURITIES && acc->initialBalance() == 0.0) {
									acc->setInitialBalance(value);
								}
								qi.current_account = acc;
								previous_transfers << transfers;
								transfers.clear();
							} else {
								if(!acc) {
									acc = new AssetsAccount(budget, ASSETS_TYPE_CURRENT, category);
									budget->addAccount(acc);
									qi.accounts++;
								}
								Transfer *tra = NULL;
								if(!date.isValid() || !qi.current_account) {
									qi.failed_transactions++;
								} else {
									if(value < 0.0) tra = new Transfer(budget, -value, date, qi.current_account, acc, memo);
									else tra = new Transfer(budget, value, date, acc, qi.current_account, memo);
									bool duplicate = false;
									QList<Transfer*>::iterator it_e = previous_transfers.end();
									for(QList<Transfer*>::iterator it = previous_transfers.begin(); it != it_e; ++it) {
										if(tra->equals(*it, false)) {
											duplicate = true;
											break;
										}
									}
									if(duplicate) {
										delete tra;
									} else if(ignore_duplicates && budget->findDuplicateTransaction(tra)) {
										qi.duplicates++;
										delete tra;
									} else {
										tra->setTimestamp(datestamps.contains(tra->date()) ? datestamps[tra->date()] + 1 : DATE_TO_MSECS(tra->date()) / 1000);
										datestamps[tra->date()] = tra->timestamp();
										budget->addTransaction(tra);
										transfers.append(tra);
										qi.transactions++;
									}
								}
							}
						} else if(!test) {
							bool empty = category.isEmpty();
							bool b_exp = value < 0.0;
							if(empty) {
								category = Budget::tr("Uncategorized");
							}
							CategoryAccount *cat = NULL, *parent_cat = NULL;
							if(!b_exp) parent_cat = budget->findIncomesAccount(category, NULL);
							else parent_cat = budget->findExpensesAccount(category, NULL);
							if(!empty && !parent_cat) {
								if(!b_exp) {
									QHash<QString, ExpensesAccount*>::const_iterator it = def_expense_cats.find(category);
									if(it != def_expense_cats.constEnd()) parent_cat = *it;
								} else {
									QHash<QString, IncomesAccount*>::const_iterator it = def_income_cats.find(category);
									if(it != def_income_cats.constEnd()) parent_cat = *it;
								}
								if(parent_cat) b_exp = !b_exp;
							}
							if(!parent_cat) {
								if(!b_exp) parent_cat = new IncomesAccount(budget, category);
								else parent_cat = new ExpensesAccount(budget, category);
								budget->addAccount(parent_cat);
								qi.categories++;
							}
							if(subcategory.isEmpty()) {
								cat = parent_cat;
								parent_cat = NULL;
							} else {
								if(!b_exp) cat = budget->findIncomesAccount(subcategory, parent_cat);
								else cat = budget->findExpensesAccount(subcategory, parent_cat);
								if(!cat) {
									if(!b_exp) cat = new IncomesAccount(budget, subcategory);
									else cat = new ExpensesAccount(budget, subcategory);
									cat->setParentCategory(parent_cat);
									budget->addAccount(cat);
									qi.categories++;
								}
							}
							if(b_exp) {
								//Expense
								if(!date.isValid() || !qi.current_account) {
									qi.failed_transactions++;
								} else {
									Expense *exp = new Expense(budget, -value, date, (ExpensesAccount*) cat, qi.current_account, memo);
									if(value > 0.0) exp->setQuantity(-1.0);
									exp->setPayee(payee);
									if(ignore_duplicates && budget->findDuplicateTransaction(exp)) {
										qi.duplicates++;
										delete exp;
									} else {
										exp->setTimestamp(datestamps.contains(exp->date()) ? datestamps[exp->date()] + 1 : DATE_TO_MSECS(exp->date()) / 1000);
										datestamps[exp->date()] = exp->timestamp();
										budget->addTransaction(exp);
										qInfo() << exp->description();
										qi.transactions++;
									}
								}
							} else {
								//Income
								if(!date.isValid() || !qi.current_account) {
									qi.failed_transactions++;
								} else {
									Income *inc = new Income(budget, value, date, (IncomesAccount*) cat, qi.current_account, memo);
									if(value < 0.0) inc->setQuantity(-1.0);
									inc->setPayer(payee);
									if(ignore_duplicates && budget->findDuplicateTransaction(inc)) {
										qi.duplicates++;
										delete inc;
									} else {
										inc->setTimestamp(datestamps.contains(inc->date()) ? datestamps[inc->date()] + 1 : DATE_TO_MSECS(inc->date()) / 1000);
										datestamps[inc->date()] = inc->timestamp();
										budget->addTransaction(inc);
										qInfo() << inc->description();
										qi.transactions++;
									}
								}
							}
						}
						qi.had_transaction = true;
					} else if(type == 9 && !test) {
						//Security transaction
						qi.security_transactions++;
					} else if(type == 1 && (!test || !qi.account_defined)) {
						//account
						if(!qi.had_transaction) qi.account_defined = true;
						if(name.isEmpty()) name = Budget::tr("Unnamed");
						qi.current_account = budget->findAssetsAccount(name);
						if(!qi.current_account) {
							AssetsType account_type = ASSETS_TYPE_OTHER;
							if(qi.unknown_defs.contains(atype)) {
								int ut_type = qi.unknown_defs[atype];
								if(ut_type == 9) account_type = ASSETS_TYPE_SECURITIES;
								else if(ut_type == 11) account_type = ASSETS_TYPE_CASH;
								else if(ut_type == 12) account_type = ASSETS_TYPE_CREDIT_CARD;
								else if(ut_type == 10) account_type = ASSETS_TYPE_CURRENT;
								else if(ut_type == 14) account_type = ASSETS_TYPE_LIABILITIES;
							} else if(atype == "cash") account_type = ASSETS_TYPE_CASH;
							else if(atype == "invst" || atype == "mutual") account_type = ASSETS_TYPE_SECURITIES;
							else if(atype == "ccard" || atype == "creditcard") account_type = ASSETS_TYPE_CREDIT_CARD;
							else if(atype == "oth l") account_type = ASSETS_TYPE_LIABILITIES;
							else if(atype == "oth a") account_type = ASSETS_TYPE_OTHER;
							else if(atype != "bank" && atype != "port") {
								if(test) {
									qi.unknown_defs_pre[atype_bak] = atype;
									qi.unknown_defs[atype] = -2;
								}
								qi.unknown = true;
							}
							if(!test) {
								qi.current_account = new AssetsAccount(budget, account_type, name, 0.0, description);
								budget->addAccount(qi.current_account);
								qi.accounts++;
							}
						}
						previous_transfers << transfers;
						transfers.clear();
					} else if(type == 2 && !test) {
						//category
						subname = name.section(':', 1).trimmed();
						name = name.section(':', 0, 0).trimmed();
						if(!name.isEmpty()) {
							if(incomecat) {
								IncomesAccount *acc = budget->findIncomesAccount(name, NULL);
								if(!acc) {
									acc = new IncomesAccount(budget, name, subname.isEmpty() ? description : QString());
									budget->addAccount(acc);
									qi.categories++;
								}
								def_income_cats[name] = acc;
								if(!subname.isEmpty() && !budget->findIncomesAccount(subname, acc)) {
									IncomesAccount *subacc = new IncomesAccount(budget, subname, description);
									subacc->setParentCategory(acc);
									budget->addAccount(subacc);
									qi.categories++;
								}
							} else {
								ExpensesAccount *acc = budget->findExpensesAccount(name, NULL);
								if(!acc) {
									acc = new ExpensesAccount(budget, name, subname.isEmpty() ? description : QString());
									budget->addAccount(acc);
									qi.categories++;
								}
								def_expense_cats[name] = acc;
								if(!subname.isEmpty() && !budget->findExpensesAccount(subname, acc)) {
									ExpensesAccount *subacc = new ExpensesAccount(budget, subname, description);
									subacc->setParentCategory(acc);
									budget->addAccount(subacc);
									qi.categories++;
								}
							}
						}
					} else if(type == 3 && !test) {
						//security
						/*if(name.isEmpty()) name = Budget::tr("Unnamed");
						Security *sec = budget->findSecurity(name);
						if(!sec) {
							SecurityType sectype = SECURITY_TYPE_STOCK;
							if(atype == "mutual fund" || atype == "fund" || atype == "mf") sectype = SECURITY_TYPE_MUTUAL_FUND;
							else if(atype == "bond" || atype == "dept") sectype = SECURITY_TYPE_BOND;
							else if(atype == "other" || atype != "oth") sectype = SECURITY_TYPE_OTHER;
							else if(atype != "stock" && atype != "st") qi.unknown = true;
							AssetsAccount *saccount = NULL;
							if(qi.current_account && qi.current_account->accountType() == ASSETS_TYPE_SECURITIES) {
								saccount = qi.current_account;
							} else {
								for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
									saccount = *it;
									if(saccount->accountType() == ASSETS_TYPE_SECURITIES) break;
								}
								if(!saccount) {
									saccount = new AssetsAccount(budget, ASSETS_TYPE_SECURITIES, Budget::tr("Securities"));
									budget->addAccount(saccount);
									qi.accounts++;
								}
							}
							sec = new Security(budget, saccount, sectype, 0.0, 4, name, description);
							budget->addSecurity(sec);
						}*/
						qi.securities++;
					}
					memo = QString();
					description = QString();
					payee = QString();
					category = QString();
					subcategory = QString();
					atype = QString();
					atype_bak = QString();
					name = QString();
					ticker_symbol = QString();
					security = QString();
					date = QDate();
					splits.clear();
					current_split = NULL;
					value = 0.0;
					//sec_amount = 0.0;
					//price = 0.0;
					//commission = 0.0;
					incomecat = false;
					break;
				}
			}
		}
		line = fstream.readLine().trimmed();
	}
	if(qi.value_format == 0) qi.value_format = 1;
	if(qi.shares_format == 0) qi.shares_format = 1;
	if(qi.price_format == 0) qi.price_format = 1;
	if(qi.percentage_format == 0) qi.percentage_format = 1;
}

QString writeQIFDate(const QDate &date, int date_format) {
	if(date_format == 1) return date.toString(Qt::ISODate);
	else if(date_format == 2) return date.toString(Qt::LocalDate);
	if(date.year() >= 2000) return date.toString("MM/dd'yy");
	return date.toString("MM/dd/yy");
}
QString writeQIFValue(double value, int value_format, int decimals) {
	if(value_format == 1) return QString::number(value, 'f', decimals);
	QString str = QString::number(value, 'f', decimals);
	str.replace(".", ",");
	return str;
}

void exportQIFTransaction(QTextStream &fstream, qif_info &qi, Transaction *trans) {
	bool sectrans = false;
	bool secacc = false;
	Account *cat = NULL;
	const QString *payee = NULL;
	bool neg = false;
	double d_com = 0.0;
	fstream << "D" << writeQIFDate(trans->date(), qi.date_format) << "\n";
	switch(trans->type()) {
		case TRANSACTION_TYPE_EXPENSE: {
			cat = ((Expense*) trans)->category();
			payee = &((Expense*) trans)->payee();
			neg = true;
			break;
		}
		case TRANSACTION_TYPE_INCOME: {
			if(trans->subtype() == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND) {
				fstream << "N" << "ReinvDiv" << "\n";
				fstream << "Y" << ((ReinvestedDividend*) trans)->security()->name() << "\n";
				fstream << "I" << writeQIFValue(((ReinvestedDividend*) trans)->shareValue(), qi.value_format, SAVE_MONETARY_DECIMAL_PLACES) << "\n";
				fstream << "Q" << writeQIFValue(((ReinvestedDividend*) trans)->shares(), qi.value_format, ((ReinvestedDividend*) trans)->security()->decimals()) << "\n";
				secacc = true;
			} else if(((Income*) trans)->security() && ((Income*) trans)->security()->account() == qi.current_account) {
				fstream << "N" << "DivX" << "\n";
				fstream << "Y" << ((Income*) trans)->security()->name() << "\n";
				secacc = true;
			}
			cat = ((Income*) trans)->category();
			payee = &((Income*) trans)->payer();
			break;
		}
		case TRANSACTION_TYPE_TRANSFER: {
			if(((Transfer*) trans)->from() == qi.current_account) {cat = ((Transfer*) trans)->to(); neg = true;}
			else cat = ((Transfer*) trans)->from();
			break;
		}
		case TRANSACTION_TYPE_SECURITY_SELL: {}
		case TRANSACTION_TYPE_SECURITY_BUY: {
			SecurityTransaction *sec = (SecurityTransaction*) trans;
			sectrans = true;
			if(sec->security()->account() != qi.current_account) {
				if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY) {
					neg = true;
				}
				break;
			}
			secacc = true;
			if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY) {
				fstream << "N" << "BuyX" << "\n";
				d_com = sec->value() - sec->shares() * sec->shareValue();
			} else {
				fstream << "N" << "SellX" << "\n";
				d_com = sec->shares() * sec->shareValue() - sec->value();
			}
			if(d_com != 0.0) {
				double deci_pow = pow(10, SAVE_MONETARY_DECIMAL_PLACES);
				d_com = round(d_com * deci_pow) / deci_pow;
			}
			fstream << "Y" << sec->security()->name() << "\n";
			fstream << "I" << writeQIFValue(sec->shareValue(), qi.value_format, SAVE_MONETARY_DECIMAL_PLACES) << "\n";
			fstream << "Q" << writeQIFValue(sec->shares(), qi.value_format, sec->security()->decimals()) << "\n";
			cat = ((SecurityTransaction*) trans)->account();
			break;
		}
	}
	fstream << "T" << writeQIFValue(neg ? -trans->value() : trans->value(), qi.value_format, SAVE_MONETARY_DECIMAL_PLACES) << "\n";
	fstream << "C" << "X" << "\n";
	if(payee && !payee->isEmpty()) fstream << "P" << *payee << "\n";
	if(!secacc && !trans->description().isEmpty()) fstream << "M" << trans->description() << "\n";
	if(sectrans && secacc && d_com != 0.0) {
		fstream << "O" << writeQIFValue(d_com, qi.value_format, SAVE_MONETARY_DECIMAL_PLACES) << "\n";
	}
	if(!cat) fstream << "L" << "[" << ((SecurityTransaction*) trans)->security()->account()->name() << ":" << ((SecurityTransaction*) trans)->security()->name() << "]" << "\n";
	else if(cat->type() == ACCOUNT_TYPE_ASSETS) fstream << "L" << "[" << cat->name() << "]" << "\n";
	else if(cat != trans->budget()->null_incomes_account) fstream << "L" << cat->nameWithParent(false) << "\n";
	if(sectrans && secacc) fstream << "$" << writeQIFValue((trans->type() == TRANSACTION_TYPE_SECURITY_SELL) ? -trans->value() : trans->value(), qi.value_format, SAVE_MONETARY_DECIMAL_PLACES) << "\n";
	fstream << "^" << "\n";

}

void exportQIFSplitTransaction(QTextStream &fstream, qif_info &qi, MultiItemTransaction *split) {
	fstream << "D" << writeQIFDate(split->date(), qi.date_format) << "\n";
	fstream << "T" << writeQIFValue(split->value(), qi.value_format, SAVE_MONETARY_DECIMAL_PLACES) << "\n";
	fstream << "C" << "X" << "\n";
	QVector<Transaction*>::size_type c = split->count();
	if(!split->payee().isEmpty()) fstream << "P" << split->payee() << "\n";
	if(!split->description().isEmpty()) fstream << "M" << split->description() << "\n";
	bool b_more = false;
	for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
		Transaction *trans = split->at(i);
		Account *cat = NULL;
		bool neg = false;
		switch(trans->type()) {
			case TRANSACTION_TYPE_EXPENSE: {
				cat = ((Expense*) trans)->category();
				neg = true;
				break;
			}
			case TRANSACTION_TYPE_INCOME: {
				cat = ((Income*) trans)->category();
				break;
			}
			case TRANSACTION_TYPE_TRANSFER: {
				if(((Transfer*) trans)->from() == qi.current_account) {cat = ((Transfer*) trans)->to(); neg = true;}
				else cat = ((Transfer*) trans)->from();
				break;
			}
			default: {b_more = true; break;}
		}
		if(cat->type() == ACCOUNT_TYPE_ASSETS) fstream << "S" << "[" << cat->name() << "]" << "\n";
		else fstream << "S" << cat->nameWithParent(false) << "\n";
		if(!trans->description().isEmpty()) fstream << "E" << trans->description() << "\n";
		fstream << "$" << writeQIFValue(neg ? -trans->value() : trans->value(), qi.value_format, SAVE_MONETARY_DECIMAL_PLACES) << "\n";
	}
	fstream << "^" << "\n";
	if(b_more) {
		for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
			Transaction *trans = split->at(i);
			switch(trans->type()) {
				case TRANSACTION_TYPE_EXPENSE: {}
				case TRANSACTION_TYPE_INCOME: {}
				case TRANSACTION_TYPE_TRANSFER: {break;}
				default: {exportQIFTransaction(fstream, qi, trans); break;}
			}
		}
	}
}

void exportQIFOpeningBalance(QTextStream &fstream, qif_info &qi, AssetsAccount *account, const QDate &date) {
	fstream << "!Type:";
	switch(account->accountType()) {
		case ASSETS_TYPE_CURRENT: {fstream << "Bank"; break;}
		case ASSETS_TYPE_SAVINGS: {fstream << "Bank"; break;}
		case ASSETS_TYPE_CREDIT_CARD: {fstream << "CCard"; break;}
		case ASSETS_TYPE_LIABILITIES: {fstream << "Oth L"; break;}
		case ASSETS_TYPE_SECURITIES: {fstream << "Invst"; break;}
		case ASSETS_TYPE_BALANCING: {fstream << "Oth A"; break;}
		case ASSETS_TYPE_CASH: {fstream << "Cash"; break;}
		default: {fstream << "Oth A"; break;}
	}
	fstream << "\n";
	if(account->accountType() == ASSETS_TYPE_SECURITIES) {
		Budget *budget = account->budget();
		for(SecurityList<Security*>::const_iterator it = budget->securities.constBegin(); it != budget->securities.constEnd(); ++it) {
			Security *sec = *it;
			if(sec->account() == account && sec->initialShares() > 0.0) {
				QMap<QDate, double>::const_iterator it = sec->quotations.begin();
				if(it == sec->quotations.end()) fstream << "D" << writeQIFDate(date, qi.date_format) << "\n";
				else fstream << "D" << writeQIFDate(it.key(), qi.date_format) << "\n";
				fstream << "N" << "ShrsIn" << "\n";
				fstream << "Y" << sec->name() << "\n";
				if(it != sec->quotations.end()) fstream << "I" << writeQIFValue(it.value(), qi.value_format, SAVE_MONETARY_DECIMAL_PLACES) << "\n";
				fstream << "Q" << writeQIFValue(sec->initialShares(), qi.value_format, sec->decimals()) << "\n";
				if(it != sec->quotations.end()) fstream << "T" << writeQIFValue(sec->initialBalance(), qi.value_format, SAVE_MONETARY_DECIMAL_PLACES) << "\n";
				fstream << "C" << "X" << "\n";
				fstream << "P" << "Opening Balance" << "\n";
				fstream << "M" << "Opening" << "\n";
				fstream << "^" << "\n";
			}
		}
	} else {
		fstream << "D" << writeQIFDate(date, qi.date_format) << "\n";
		fstream << "T" << writeQIFValue(account->initialBalance(), qi.value_format, SAVE_MONETARY_DECIMAL_PLACES) << "\n";
		fstream << "C" << "X" << "\n";
		fstream << "P" << "Opening Balance" << "\n";
		fstream << "L" << "[" << account->name() << "]" << "\n";
		fstream << "^" << "\n";
	}
}

void exportQIFAccount(QTextStream &fstream, qif_info&, Account *account) {
	if(account->type() == ACCOUNT_TYPE_ASSETS) fstream << "!Account" << "\n";
	else fstream << "!Type:Cat" << "\n";
	fstream << "N" << account->nameWithParent(false) << "\n";
	if(account->type() == ACCOUNT_TYPE_ASSETS) {
		fstream << "T";
		switch(((AssetsAccount*) account)->accountType()) {
			case ASSETS_TYPE_CURRENT: {fstream << "Bank"; break;}
			case ASSETS_TYPE_SAVINGS: {fstream << "Bank"; break;}
			case ASSETS_TYPE_CREDIT_CARD: {fstream << "CCard"; break;}
			case ASSETS_TYPE_LIABILITIES: {fstream << "Oth L"; break;}
			case ASSETS_TYPE_SECURITIES: {fstream << "Invst"; break;}
			case ASSETS_TYPE_BALANCING: {fstream << "Oth A"; break;}
			case ASSETS_TYPE_CASH: {fstream << "Cash"; break;}
			default: {fstream << "Oth A"; break;}
		}
		fstream << "\n";
	}
	if(!account->description().isEmpty()) fstream << "D" << account->description() << "\n";
	if(account->type() == ACCOUNT_TYPE_EXPENSES) fstream << "E" << "\n";
	else if(account->type() == ACCOUNT_TYPE_INCOMES) fstream << "I" << "\n";
	fstream << "^" << "\n";
}

void exportQIFSecurity(QTextStream &fstream, qif_info&, Security *sec) {
	fstream << "!Type:Security" << "\n";
	fstream << "N" << sec->name() << "\n";
	fstream << "T";
	switch(sec->type()) {
		case SECURITY_TYPE_BOND: {fstream << "Bond"; break;}
		case SECURITY_TYPE_STOCK: {fstream << "Stock"; break;}
		case SECURITY_TYPE_MUTUAL_FUND: {fstream << "Mutual Fund"; break;}
		case SECURITY_TYPE_OTHER: {fstream << "Other"; break;}
	}
	fstream << "\n";
	if(!sec->description().isEmpty()) fstream << "D" << sec->description() << "\n";
	fstream << "^" << "\n";
}

void exportQIF(QTextStream &fstream, qif_info &qi, Budget *budget, bool export_cats) {
	if(qi.current_account) {
		if(export_cats) {
			QMap<IncomesAccount*, bool> icats;
			QMap<ExpensesAccount*, bool> ecats;
			for(TransactionList<Income*>::const_iterator it = budget->incomes.constBegin(); it != budget->incomes.constEnd(); ++it) {
				Income *inc = *it;
				if(inc->to() == qi.current_account) {
					icats[inc->category()] = true;
				}
			}
			for(TransactionList<Expense*>::const_iterator it = budget->expenses.constBegin(); it != budget->expenses.constEnd(); ++it) {
				Expense *exp = *it;
				if(exp->from() == qi.current_account) {
					ecats[exp->category()] = true;

				}
			}
			QMap<IncomesAccount*, bool>::iterator iit_e = icats.end();
			for(QMap<IncomesAccount*, bool>::iterator iit = icats.begin(); iit != iit_e; ++iit) {
				exportQIFAccount(fstream, qi, iit.key());
			}
			QMap<ExpensesAccount*, bool>::iterator eit_e = ecats.end();
			for(QMap<ExpensesAccount*, bool>::iterator eit = ecats.begin(); eit != eit_e; ++eit) {
				exportQIFAccount(fstream, qi, eit.key());
			}
			for(SecurityList<Security*>::const_iterator it = budget->securities.constBegin(); it != budget->securities.constEnd(); ++it) {
				Security *sec = *it;
				if(sec->account() == qi.current_account) {
					exportQIFSecurity(fstream, qi, sec);
				}
			}
		}
		exportQIFAccount(fstream, qi, qi.current_account);
		bool first = true;
		SplitTransaction *split = NULL;
		for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constBegin(); it != budget->transactions.constEnd(); ++it) {
			Transaction *trans = *it;
			if(trans->fromAccount() == qi.current_account || trans->toAccount() == qi.current_account) {
				if(first) {
					exportQIFOpeningBalance(fstream, qi, qi.current_account, trans->date());
					first = false;
				}
				if(trans->parentSplit() && trans->parentSplit()->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ITEMS && ((MultiItemTransaction*) trans->parentSplit())->account() == qi.current_account) {
					if(!split || split != trans->parentSplit()) {
						split = trans->parentSplit();
						exportQIFSplitTransaction(fstream, qi, (MultiItemTransaction*) split);
					}
				} else {
					exportQIFTransaction(fstream, qi, trans);
				}
			}
		}
		if(first) {
			exportQIFOpeningBalance(fstream, qi, qi.current_account, QDate::currentDate());
		}
		for(SecurityList<Security*>::const_iterator it = budget->securities.constBegin(); it != budget->securities.constEnd(); ++it) {
			Security *sec = *it;
			if(sec->account() == qi.current_account) {
				for(SecurityTransactionList<Income*>::const_iterator it2 = sec->dividends.constBegin(); it2 != sec->dividends.constEnd(); ++it2) {
					Income *inc = *it2;
					exportQIFTransaction(fstream, qi, inc);
				}
				for(SecurityTransactionList<ReinvestedDividend*>::const_iterator it2 = sec->reinvestedDividends.constBegin(); it2 != sec->reinvestedDividends.constEnd(); ++it2) {
					Income *inc = *it2;
					exportQIFTransaction(fstream, qi, inc);
				}
			}
		}
	} else {
		if(export_cats) {
			for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
				IncomesAccount *iaccount = *it;
				exportQIFAccount(fstream, qi, iaccount);
			}
			for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
				ExpensesAccount *eaccount = *it;
				exportQIFAccount(fstream, qi, eaccount);
			}
			for(SecurityList<Security*>::const_iterator it = budget->securities.constBegin(); it != budget->securities.constEnd(); ++it) {
				Security *sec = *it;
				exportQIFSecurity(fstream, qi, sec);
			}
		}
		for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
			AssetsAccount *account = *it;
			qi.current_account = account;
			exportQIF(fstream, qi, budget, false);
		}
	}
}
bool exportQIFFile(Budget *budget, QWidget *parent, bool extra_parameters) {
	if(budget->accounts.count() <= 1) {
		return false;
	}
	ExportQIFDialog *dialog = new ExportQIFDialog(budget, parent, extra_parameters);
	bool ret = (dialog->exec() == QDialog::Accepted);
	dialog->deleteLater();
	return ret;
}
