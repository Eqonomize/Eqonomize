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



#include <QButtonGroup>
#include <QCheckBox>
#include <QDateTime>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QMap>
#include <QRadioButton>
#include <QSpinBox>
#include <QTextStream>
#include <QVBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QUrl>
#include <QTemporaryFile>
#include <QDateEdit>
#include <QMessageBox>

#include <klineedit.h>
#include <kurlrequester.h>
#include <klocalizedstring.h>

#include "budget.h"
#include "eqonomizevalueedit.h"
#include "eqonomize.h"
#include "importcsvdialog.h"

#include <cmath>
#include <ctime>

#define ALL_TYPES_ID		5

ImportCSVDialog::ImportCSVDialog(Budget *budg, QWidget *parent) : QWizard(parent), budget(budg) {

	setWindowTitle(i18n("Import CSV file"));
	setModal(true);

	QIFWizardPage *page1 = new QIFWizardPage();
	page1->setTitle(i18n("Transaction Type Selection"));
	setPage(0, page1);
	QVBoxLayout *layout1 = new QVBoxLayout(page1);
	typeGroup = new QButtonGroup(this);
	QVBoxLayout *typeGroup_layout = new QVBoxLayout();
	layout1->addLayout(typeGroup_layout);
	QRadioButton *rb = new QRadioButton(i18n("Expenses"));
	rb->setChecked(true);
	typeGroup_layout->addWidget(rb);
	typeGroup->addButton(rb, 0);
	rb->setFocus();
	rb = new QRadioButton(i18n("Incomes"));
	typeGroup_layout->addWidget(rb);
	typeGroup->addButton(rb, 1);
	rb = new QRadioButton(i18n("Transfers"));
	typeGroup_layout->addWidget(rb);
	typeGroup->addButton(rb, 2);
	rb = new QRadioButton(i18n("Expenses and incomes (negative cost)"));
	typeGroup_layout->addWidget(rb);
	typeGroup->addButton(rb, 3);
	rb = new QRadioButton(i18n("Expenses and incomes (separate columns)"));
	typeGroup_layout->addWidget(rb);
	typeGroup->addButton(rb, 4);
	rb = new QRadioButton(i18n("All types"));
	typeGroup_layout->addWidget(rb);
	typeGroup->addButton(rb, 5);
	typeDescriptionLabel = new QLabel(page1);
	typeDescriptionLabel->setWordWrap(true);
	layout1->addWidget(typeDescriptionLabel);

	QIFWizardPage *page2 = new QIFWizardPage();
	page2->setTitle(i18n("File Selection"));
	setPage(1, page2);
	QGridLayout *layout2 = new QGridLayout(page2);

	layout2->addWidget(new QLabel(i18n("File:"), page2), 0, 0);
	fileEdit = new KUrlRequester(page2);
	fileEdit->setMode(KFile::File | KFile::ExistingOnly);
	fileEdit->setFilter("text/csv");
	layout2->addWidget(fileEdit, 0, 1);
	layout2->addWidget(new QLabel(i18n("First data row:"), page2), 1, 0);
	rowEdit = new QSpinBox(page2);
	rowEdit->setRange(0, 1000);
	rowEdit->setSpecialValueText(i18n("Auto"));
	rowEdit->setValue(0);
	layout2->addWidget(rowEdit, 1, 1);
	layout2->addWidget(new QLabel(i18n("Column delimiter:"), page2), 2, 0);
	delimiterCombo = new QComboBox(page2);
	delimiterCombo->setEditable(false);
	delimiterCombo->addItem(i18n("Comma"));
	delimiterCombo->addItem(i18n("Tabulator"));
	delimiterCombo->addItem(i18n("Semicolon"));
	delimiterCombo->addItem(i18n("Space"));
	delimiterCombo->addItem(i18n("Other"));
	layout2->addWidget(delimiterCombo, 2, 1);
	delimiterEdit = new QLineEdit(page2);
	delimiterEdit->setEnabled(false);
	layout2->addWidget(delimiterEdit, 3, 1);
	layout2->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding), 4, 0, 1, 2);

	QIFWizardPage *page3 = new QIFWizardPage();
	page3->setTitle(i18n("Columns Specification"));
	setPage(2, page3);
	QGridLayout *layout3 = new QGridLayout(page3);

	layout3->addWidget(new QLabel(i18n("Description:"), page3), 0, 0);
	descriptionGroup = new QButtonGroup(this);
	columnDescriptionButton = new QRadioButton(i18n("Column"), page3);
	descriptionGroup->addButton(columnDescriptionButton);
	layout3->addWidget(columnDescriptionButton, 0, 1);
	columnDescriptionEdit = new QSpinBox(page3);
	columnDescriptionEdit->setRange(1, 100);
	columnDescriptionEdit->setValue(2);
	columnDescriptionButton->setChecked(true);
	layout3->addWidget(columnDescriptionEdit, 0, 2);
	valueDescriptionButton = new QRadioButton(i18n("Value"), page3);
	descriptionGroup->addButton(valueDescriptionButton);
	layout3->addWidget(valueDescriptionButton, 0, 3);
	valueDescriptionEdit = new QLineEdit(page3);
	valueDescriptionEdit->setEnabled(false);
	layout3->addWidget(valueDescriptionEdit, 0, 4);

	costLabel = new QLabel(i18n("Cost:"), page3);
	layout3->addWidget(costLabel, 1, 0);
	costGroup = new QButtonGroup(this);
	columnCostButton = new QRadioButton(i18n("Column"), page3);
	costGroup->addButton(columnCostButton);
	layout3->addWidget(columnCostButton, 1, 1);
	columnCostEdit = new QSpinBox(page3);
	columnCostEdit->setRange(1, 100);
	columnCostEdit->setValue(3);
	columnCostButton->setChecked(true);
	layout3->addWidget(columnCostEdit, 1, 2);
	valueCostButton = new QRadioButton(i18n("Value"), page3);
	costGroup->addButton(valueCostButton);
	layout3->addWidget(valueCostButton, 1, 3);
	valueCostEdit = new EqonomizeValueEdit(false, page3);
	valueCostEdit->setEnabled(false);
	layout3->addWidget(valueCostEdit, 1, 4);
	costLabel->hide();
	valueCostEdit->hide();
	valueCostButton->hide();
	columnCostEdit->hide();
	columnCostButton->hide();

	valueLabel = new QLabel(i18n("Cost:"), page3);
	layout3->addWidget(valueLabel, 2, 0);
	valueGroup = new QButtonGroup(this);
	columnValueButton = new QRadioButton(i18n("Column"), page3);
	valueGroup->addButton(columnValueButton);
	layout3->addWidget(columnValueButton, 2, 1);
	columnValueEdit = new QSpinBox(page3);
	columnValueEdit->setRange(1, 100);
	columnValueEdit->setValue(3);
	columnValueButton->setChecked(true);
	layout3->addWidget(columnValueEdit, 2, 2);
	valueValueButton = new QRadioButton(i18n("Value"), page3);
	valueGroup->addButton(valueValueButton);
	layout3->addWidget(valueValueButton, 2, 3);
	valueValueEdit = new EqonomizeValueEdit(false, page3);
	valueValueEdit->setEnabled(false);
	layout3->addWidget(valueValueEdit, 2, 4);

	layout3->addWidget(new QLabel(i18n("Date:"), page3), 3, 0);
	dateGroup = new QButtonGroup(this);
	columnDateButton = new QRadioButton(i18n("Column"), page3);
	dateGroup->addButton(columnDateButton);
	layout3->addWidget(columnDateButton, 3, 1);
	columnDateEdit = new QSpinBox(page3);
	columnDateEdit->setRange(1, 100);
	columnDateEdit->setValue(1);
	columnDateButton->setChecked(true);
	layout3->addWidget(columnDateEdit, 3, 2);
	valueDateButton = new QRadioButton(i18n("Value"), page3);
	dateGroup->addButton(valueDateButton);
	layout3->addWidget(valueDateButton, 3, 3);
	valueDateEdit = new QDateEdit(QDate::currentDate(), page3);
	valueDateEdit->setCalendarPopup(true);
	valueDateEdit->setEnabled(false);
	layout3->addWidget(valueDateEdit, 3, 4);

	AC1Label = new QLabel(i18n("Category:"), page3);
	layout3->addWidget(AC1Label, 4, 0);
	AC1Group = new QButtonGroup(this);
	columnAC1Button = new QRadioButton(i18n("Column"), page3);
	AC1Group->addButton(columnAC1Button);
	layout3->addWidget(columnAC1Button, 4, 1);
	columnAC1Edit = new QSpinBox(page3);
	columnAC1Edit->setRange(1, 100);
	columnAC1Edit->setValue(4);
	columnAC1Edit->setEnabled(false);
	layout3->addWidget(columnAC1Edit, 4, 2);
	valueAC1Button = new QRadioButton(i18n("Value"), page3);
	AC1Group->addButton(valueAC1Button);
	valueAC1Button->setChecked(true);
	layout3->addWidget(valueAC1Button, 4, 3);
	valueAC1Edit = new QComboBox(page3);
	valueAC1Edit->setEditable(false);
	layout3->addWidget(valueAC1Edit, 4, 4);

	AC2Label = new QLabel(i18n("From account:"), page3);
	layout3->addWidget(AC2Label, 5, 0);
	AC2Group = new QButtonGroup(this);
	columnAC2Button = new QRadioButton(i18n("Column"), page3);
	AC2Group->addButton(columnAC2Button);
	layout3->addWidget(columnAC2Button, 5, 1);
	columnAC2Edit = new QSpinBox(page3);
	columnAC2Edit->setRange(1, 100);
	columnAC2Edit->setValue(5);
	columnAC2Edit->setEnabled(false);
	layout3->addWidget(columnAC2Edit, 5, 2);
	valueAC2Button = new QRadioButton(i18n("Value"), page3);
	AC2Group->addButton(valueAC2Button);
	valueAC2Button->setChecked(true);
	layout3->addWidget(valueAC2Button, 5, 3);
	valueAC2Edit = new QComboBox(page3);
	valueAC2Edit->setEditable(false);
	layout3->addWidget(valueAC2Edit, 5, 4);

	layout3->addWidget(new QLabel(i18n("Comments:"), page3), 6, 0);
	commentsGroup = new QButtonGroup(this);
	columnCommentsButton = new QRadioButton(i18n("Column"), page3);
	commentsGroup->addButton(columnCommentsButton);
	layout3->addWidget(columnCommentsButton, 6, 1);
	columnCommentsEdit = new QSpinBox(page3);
	columnCommentsEdit->setRange(1, 100);
	columnCommentsEdit->setValue(6);
	columnCommentsEdit->setEnabled(false);
	layout3->addWidget(columnCommentsEdit, 6, 2);
	valueCommentsButton = new QRadioButton(i18n("Value"), page3);
	commentsGroup->addButton(valueCommentsButton);
	valueCommentsButton->setChecked(true);
	layout3->addWidget(valueCommentsButton, 6, 3);
	valueCommentsEdit = new QLineEdit(page3);
	layout3->addWidget(valueCommentsEdit, 6, 4);

	QHBoxLayout *layout3_cm = new QHBoxLayout();
	layout3_cm->addStretch(1);
	createMissingButton = new QCheckBox(i18n("Create missing categories and accounts"), page3);
	createMissingButton->setChecked(true);
	layout3_cm->addWidget(createMissingButton);
	layout3->addLayout(layout3_cm, 7, 0, 1, 5);

	layout3->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding), 8, 0, 1, 5);

	setOption(QWizard::HaveHelpButton, false);

	page3->setFinalPage(true);

	page1->setComplete(true);
	page2->setComplete(false);
	page3->setComplete(true);

	disconnect(button(NextButton), SIGNAL(clicked()), this, 0);
	connect(button(NextButton), SIGNAL(clicked()), this, SLOT(nextClicked()));

	typeChanged(0);

	connect(fileEdit, SIGNAL(textChanged(const QString&)), this, SLOT(onFileChanged(const QString&)));
	connect(delimiterCombo, SIGNAL(activated(int)), this, SLOT(delimiterChanged(int)));
	connect(columnDescriptionButton, SIGNAL(toggled(bool)), columnDescriptionEdit, SLOT(setEnabled(bool)));
	connect(valueDescriptionButton, SIGNAL(toggled(bool)), valueDescriptionEdit, SLOT(setEnabled(bool)));
	connect(columnValueButton, SIGNAL(toggled(bool)), columnValueEdit, SLOT(setEnabled(bool)));
	connect(valueValueButton, SIGNAL(toggled(bool)), valueValueEdit, SLOT(setEnabled(bool)));
	connect(columnDateButton, SIGNAL(toggled(bool)), columnDateEdit, SLOT(setEnabled(bool)));
	connect(valueDateButton, SIGNAL(toggled(bool)), valueDateEdit, SLOT(setEnabled(bool)));
	connect(columnAC1Button, SIGNAL(toggled(bool)), columnAC1Edit, SLOT(setEnabled(bool)));
	connect(valueAC1Button, SIGNAL(toggled(bool)), valueAC1Edit, SLOT(setEnabled(bool)));
	connect(columnAC2Button, SIGNAL(toggled(bool)), columnAC2Edit, SLOT(setEnabled(bool)));
	connect(valueAC2Button, SIGNAL(toggled(bool)), valueAC2Edit, SLOT(setEnabled(bool)));
	connect(columnCommentsButton, SIGNAL(toggled(bool)), columnCommentsEdit, SLOT(setEnabled(bool)));
	connect(valueCommentsButton, SIGNAL(toggled(bool)), valueCommentsEdit, SLOT(setEnabled(bool)));
	connect(typeGroup, SIGNAL(buttonClicked(int)), this, SLOT(typeChanged(int)));

}

ImportCSVDialog::~ImportCSVDialog() {
}

void ImportCSVDialog::onFileChanged(const QString &str) {
	((QIFWizardPage*) page(1))->setComplete(!str.isEmpty());
}
void ImportCSVDialog::delimiterChanged(int index) {
	delimiterEdit->setEnabled(index == 4);
}
void ImportCSVDialog::typeChanged(int id) {

	valueAC1Button->setChecked(true);
	columnAC1Edit->setEnabled(false);
	valueAC1Edit->setEnabled(true);
	valueAC1Button->setEnabled(true);
	valueAC2Button->setChecked(true);
	columnAC2Edit->setEnabled(false);
	valueAC2Edit->setEnabled(true);
	valueAC2Button->setEnabled(true);
	createMissingButton->setEnabled(id != ALL_TYPES_ID);
	createMissingButton->setChecked(id != ALL_TYPES_ID);

	valueAC1Edit->clear();
	valueAC2Edit->clear();
	if(id < 5) {
		AssetsAccount *aa = budget->assetsAccounts.first();
		while(aa) {if(aa != budget->balancingAccount && aa->accountType() != ASSETS_TYPE_SECURITIES) valueAC2Edit->addItem(aa->name()); aa = budget->assetsAccounts.next();}
	}
	if(id == 4) {
		costLabel->show();
		valueCostEdit->show();
		columnCostEdit->show();
		columnCostButton->show();
		valueCostButton->show();
		columnCostButton->setChecked(true);
		valueCostButton->setEnabled(false);
		valueCostEdit->setEnabled(false);
		columnValueEdit->setValue(4);
		columnAC1Edit->setValue(5);
		columnAC2Edit->setValue(6);
		columnCommentsEdit->setValue(7);
	} else {
		costLabel->hide();
		valueCostEdit->hide();
		columnCostEdit->hide();
		columnCostButton->hide();
		valueCostButton->hide();
		columnValueEdit->setValue(3);
		columnAC1Edit->setValue(4);
		columnAC2Edit->setValue(5);
		columnCommentsEdit->setValue(6);
	}
	switch(id) {
		case 0: {
			typeDescriptionLabel->setText(i18n("Imports data as expenses. Costs have positive value. Value is the only required column."));
			valueLabel->setText(i18n("Cost:"));
			AC1Label->setText(i18n("Category:"));
			AC2Label->setText(i18n("From account:"));
			ExpensesAccount *ea = budget->expensesAccounts.first();
			while(ea) {valueAC1Edit->addItem(ea->name()); ea = budget->expensesAccounts.next();}
			break;
		}
		case 1: {
			typeDescriptionLabel->setText(i18n("Imports data as incomes. Value is the only required column."));
			valueLabel->setText(i18n("Income:"));
			AC1Label->setText(i18n("Category:"));
			AC2Label->setText(i18n("To account:"));
			IncomesAccount *ia = budget->incomesAccounts.first();
			while(ia) {valueAC1Edit->addItem(ia->name()); ia = budget->incomesAccounts.next();}
			break;
		}
		case 2: {
			typeDescriptionLabel->setText(i18n("Imports data as transfers. Value is the only required column."));
			valueLabel->setText(i18n("Amount:"));
			AC1Label->setText(i18n("From account:"));
			AC2Label->setText(i18n("To account:"));
			AssetsAccount *aa = budget->assetsAccounts.first();
			while(aa) {if(aa != budget->balancingAccount && aa->accountType() != ASSETS_TYPE_SECURITIES) valueAC1Edit->addItem(aa->name()); aa = budget->assetsAccounts.next();}
			break;
		}
		case 3: {
			typeDescriptionLabel->setText(i18n("Imports data as expenses and incomes. Costs have negative value. Value and category are both required columns."));
			columnAC1Button->setChecked(true);
			columnAC1Edit->setEnabled(true);
			valueAC1Edit->setEnabled(false);
			valueAC1Button->setEnabled(false);
			valueLabel->setText(i18n("Value:"));
			AC1Label->setText(i18n("Category:"));
			AC2Label->setText(i18n("Account:"));
			break;
		}
		case 4: {
			typeDescriptionLabel->setText(i18n("Imports data as expenses and incomes. Costs and incomes have separate columns. Income, cost, and category are all required columns."));
			columnAC1Button->setChecked(true);
			columnAC1Edit->setEnabled(true);
			valueAC1Edit->setEnabled(false);
			valueAC1Button->setEnabled(false);
			valueLabel->setText(i18n("Income:"));
			AC1Label->setText(i18n("Category:"));
			AC2Label->setText(i18n("Account:"));
			break;
		}
		case ALL_TYPES_ID: {
			typeDescriptionLabel->setText(i18n("Imports data as expenses, incomes, and transfers. Costs have negative or positive value. Value, to, and from are all required columns. Accounts and categories must be existing."));
			columnAC1Button->setChecked(true);
			columnAC1Edit->setEnabled(true);
			valueAC1Edit->setEnabled(false);
			valueAC1Button->setEnabled(false);
			columnAC2Button->setChecked(true);
			columnAC2Edit->setEnabled(true);
			valueAC2Edit->setEnabled(false);
			valueAC2Button->setEnabled(false);
			valueLabel->setText(i18n("Value:"));
			AC1Label->setText(i18n("From:"));
			AC2Label->setText(i18n("To:"));
			break;
		}
	}
	columnValueButton->setChecked(true);
	valueValueButton->setEnabled(false);
	if(valueAC1Edit->count() == 0) {
		columnAC1Button->setChecked(true);
		columnAC1Edit->setEnabled(true);
		valueAC1Edit->setEnabled(false);
		valueAC1Button->setEnabled(false);
	}
	if(valueAC2Edit->count() == 0) {
		columnAC2Button->setChecked(true);
		columnAC2Edit->setEnabled(true);
		valueAC2Edit->setEnabled(false);
		valueAC2Button->setEnabled(false);
	}

}
void ImportCSVDialog::nextClicked() {
	if(currentPage() == page(0)) {
		fileEdit->setFocus();
	} else if(currentPage() == page(1)) {
		const QUrl &url = fileEdit->url();
		if(url.isEmpty()) {
			QMessageBox::critical(this, i18n("Error"), i18n("A file must be selected."));
			fileEdit->setFocus();
			return;
		} else if(!url.isValid()) {
			QFileInfo info(fileEdit->lineEdit()->text());
			if(info.isDir()) {
				QMessageBox::critical(this, i18n("Error"), i18n("Selected file is a directory."));
				fileEdit->setFocus();
				return;
			} else if(!info.exists()) {
				QMessageBox::critical(this, i18n("Error"), i18n("Selected file does not exist."));
				fileEdit->setFocus();
				return;
			}
			fileEdit->setUrl(info.absoluteFilePath());
		} else {
			QFileInfo info(url.toLocalFile());
			if(info.isDir()) {
				QMessageBox::critical(this, i18n("Error"), i18n("Selected file is a directory."));
				fileEdit->setFocus();
				return;
			} else if(!info.exists()) {
				QMessageBox::critical(this, i18n("Error"), i18n("Selected file does not exist."));
				fileEdit->setFocus();
				return;
			}
		}
		if(delimiterCombo->currentIndex() == 4 && delimiterEdit->text().isEmpty()) {
			QMessageBox::critical(this, i18n("Error"), i18n("Empty delimiter."));
			delimiterEdit->setFocus();
			return;
		}
		columnDescriptionEdit->setFocus();
	}
	QWizard::next();
}

QDate readCSVDate(const QString &str, const QString &date_format, const QString &alt_date_format) {
	struct tm tp;
	QDate date;
	if(strptime(str.toLatin1(), date_format.toLatin1(), &tp)) {
		date.setDate(tp.tm_year > 17100 ? tp.tm_year - 15200 : tp.tm_year + 1900, tp.tm_mon + 1, tp.tm_mday);
	} else if(!alt_date_format.isEmpty() && strptime(str.toLatin1(), alt_date_format.toLatin1(), &tp)) {
		date.setDate(tp.tm_year > 17100 ? tp.tm_year - 15200 : tp.tm_year + 1900, tp.tm_mon + 1, tp.tm_mday);
	}
	return date;
}
double readCSVValue(const QString &str, int value_format, bool *ok) {
	QString str2 = str;
	int l = (int) str2.length();
	for(int i = 0; i < l; i++) {
		if(str2[i].isDigit()) {
			if(i > 0) {
				str2.remove(0, i);
				l -= i;
			}
			break;
		}
	}
	l--;
	for(int i = l; i >= 0; i--) {
		if(str2[i].isDigit()) {
			if(i < l) {
				str2.truncate(i + 1);
			}
			break;
		}
	}
	if(value_format == 2) {
		str2.replace(".", "");
		str2.replace(",", ".");
	} else if(value_format == 1) {
		str2.replace(",", "");
	}
	return str2.toDouble(ok);
}

//p1 MDY
//p2 DMY
//p3 YMD
//p4 YDM
void testCSVDate(const QString &str, bool &p1, bool &p2, bool &p3, bool &p4, bool &ly, char &separator) {
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
	if(p1 + p2 + p3 + p4 <= 1) return;
	QStringList strlist = str.split(separator, QString::SkipEmptyParts);
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
}

void testCSVValue(const QString &str, int &value_format) {
	if(value_format <= 0) {
		int i = str.lastIndexOf('.');
		int i2 = str.lastIndexOf(',');
		if(i2 >= 0 && i >= 0) {
			if(i2 > i) value_format = 2;
			else value_format = 1;
			return;
		}
		if(i >= 0) {
			i2 = 0;
			int l = (int) str.length();
			for(int index = i + 1; index < l; index++) {
				if(str[index].isDigit()) {
					i2++;
				} else {
					break;
				}
			}
			if(i2 < 3) value_format = 1;
			else value_format = -1;
		} else if(i2 >= 0) {
			i = 0;
			int l = (int) str.length();
			for(int index = i2 + 1; index < l; index++) {
				if(str[index].isDigit()) {
					i++;
				} else {
					break;
				}
			}
			if(i < 3) value_format = 2;
			else value_format = -1;
		}
	}
}

struct csv_info {
	int value_format;
	char separator;
	bool p1, p2, p3, p4, ly;
};


bool ImportCSVDialog::import(bool test, csv_info *ci) {

	QString date_format, alt_date_format;
	if(test) {
		ci->p1 = true;
		ci->p2 = true;
		ci->p3 = true;
		ci->p4 = true;
		ci->ly = true;
		ci->value_format = 0;
		ci->separator = -1;
	} else {
		if(ci->p1) {
			date_format += "%m";
			if(ci->separator > 0) date_format += ci->separator;
			date_format += "%d";
			if(ci->separator > 0) date_format += ci->separator;
			if(ci->ly) {
				date_format += "%Y";
			} else {
				if(ci->separator > 0) {
					alt_date_format = date_format;
					alt_date_format += '\'';
					alt_date_format += "%y";
				}
				date_format += "%y";
			}
		} else if(ci->p2) {
			date_format += "%d";
			if(ci->separator > 0) date_format += ci->separator;
			date_format += "%m";
			if(ci->separator > 0) date_format += ci->separator;
			if(ci->ly) {
				date_format += "%Y";
			} else {
				if(ci->separator > 0) {
					alt_date_format = date_format;
					alt_date_format += '\'';
					alt_date_format += "%y";
				}
				date_format += "%y";
			}
		} else if(ci->p3) {
			if(ci->ly) date_format += "%Y";
			else date_format += "%y";
			if(ci->separator > 0) date_format += ci->separator;
			date_format += "%m";
			if(ci->separator > 0) date_format += ci->separator;
			date_format += "%d";
		} else if(ci->p4) {
			if(ci->ly) date_format += "%Y";
			else date_format += "%y";
			if(ci->separator > 0) date_format += ci->separator;
			date_format += "%d";
			if(ci->separator > 0) date_format += ci->separator;
			date_format += "%m";
		}
	}
	int first_row = rowEdit->value();
	int type = typeGroup->checkedId();
	QString delimiter;
	switch(delimiterCombo->currentIndex()) {
		case 0: {delimiter = ","; break;}
		case 1: {delimiter = "\t"; break;}
		case 2: {delimiter = ";"; break;}
		case 3: {delimiter = " "; break;}
		case 4: {delimiter = delimiterEdit->text(); break;}
	}
	int description_c = columnDescriptionButton->isChecked() ? columnDescriptionEdit->value() : -1;
	int value_c = columnValueButton->isChecked() ? columnValueEdit->value() : -1;
	int cost_c = (type == 4 && columnCostButton->isChecked()) ? columnCostEdit->value() : -1;
	int date_c = columnDateButton->isChecked() ? columnDateEdit->value() : -1;
	if(test && date_c < 0) {
		ci->p1 = true;
		ci->p2 = false;
		ci->p3 = false;
		ci->p4 = false;
		ci->ly = false;
	}
	int AC1_c = columnAC1Button->isChecked() ? columnAC1Edit->value() : -1;
	int AC2_c = columnAC2Button->isChecked() ? columnAC2Edit->value() : -1;
	int comments_c = columnCommentsButton->isChecked() ? columnCommentsEdit->value() : -1;
	int ncolumns = 0, min_columns = 0;
	if(value_c > ncolumns) ncolumns = value_c;
	if(cost_c > ncolumns) ncolumns = cost_c;
	if(date_c > ncolumns) ncolumns = date_c;
	if(AC1_c > ncolumns) ncolumns = AC1_c;
	if(AC2_c > ncolumns) ncolumns = AC2_c;
	min_columns = ncolumns;
	if(description_c > ncolumns) ncolumns = description_c;
	if(comments_c > ncolumns) ncolumns = comments_c;
	if((description_c > 0 && (description_c == value_c || description_c == cost_c || description_c == date_c || description_c == AC1_c || description_c == AC2_c || description_c == comments_c))
		   || (value_c > 0 && (value_c == date_c || value_c == cost_c || value_c == AC1_c || value_c == AC2_c || value_c == comments_c))
		   || (cost_c > 0 && (cost_c == date_c || cost_c == AC1_c || cost_c == AC2_c || cost_c == comments_c))
		   || (date_c > 0 && (date_c == AC1_c || date_c == AC2_c || date_c == comments_c))
		   || (AC1_c > 0 && (AC1_c == AC2_c || AC1_c == comments_c))
		   || (AC2_c > 0 && AC2_c == comments_c)
	  ) {
		QMessageBox::critical(this, i18n("Error"), i18n("The same column number is selected multiple times."));
		return false;
	}
	bool create_missing = createMissingButton->isChecked() && type != ALL_TYPES_ID;
	QString description, comments;
	if(!test && description_c < 0) description = valueDescriptionEdit->text();
	if(!test && comments_c < 0) comments = valueCommentsEdit->text();
	QMap<QString, Account*> eaccounts, iaccounts, aaccounts;
	Account *ac1 = NULL, *ac2 = NULL;
	if(!test && (AC1_c < 0 || AC2_c < 0)) {
		ExpensesAccount *ea = budget->expensesAccounts.first();
		while(ea) {eaccounts[ea->name()] = ea; ea = budget->expensesAccounts.next();}
		IncomesAccount *ia = budget->incomesAccounts.first();
		while(ia) {iaccounts[ia->name()] = ia; ia = budget->incomesAccounts.next();}
		AssetsAccount *aa = budget->assetsAccounts.first();
		while(aa) {aaccounts[aa->name()] = aa; aa = budget->assetsAccounts.next();}
	}
	if(AC1_c < 0) {
		int i = 0;
		Account *account = NULL;
		if(type == 0) account = budget->expensesAccounts.first();
		else if(type == 1) account = budget->incomesAccounts.first();
		else if(type == 2) account = budget->assetsAccounts.first();
		while(account) {
			if(type == 2) {
				while(account == budget->balancingAccount || ((AssetsAccount*) account)->accountType() == ASSETS_TYPE_SECURITIES) {
					account = budget->assetsAccounts.next();
					if(!account) break;
				}
			}
			if(i == valueAC1Edit->currentIndex()) {
				ac1 = account;
				break;
			}
			if(type == 0) {
				account = budget->expensesAccounts.next();
			} else if(type == 1) {
				account = budget->incomesAccounts.next();
			} else if(type == 2) {
				account = budget->assetsAccounts.next();
			}
			i++;
		}
	}
	if(AC2_c < 0) {
		int i = 0;
		Account *account = budget->assetsAccounts.first();
		while(account) {
			while(account == budget->balancingAccount || ((AssetsAccount*) account)->accountType() == ASSETS_TYPE_SECURITIES) {
				account = budget->assetsAccounts.next();
				if(!account) break;
			}
			if(i == valueAC2Edit->currentIndex()) {
				ac2 = account;
				break;
			}
			account = budget->assetsAccounts.next();
			i++;
		}
		if(ac1 == ac2) {
			QMessageBox::critical(this, i18n("Error"), i18n("Selected from account is the same as the to account."));
			return false;
		}
	}
	QDate date;
	if(date_c < 0) {
		date = valueDateEdit->date();
		if(!date.isValid()) {
			QMessageBox::critical(this, i18n("Error"), i18n("Invalid date."));
			return false;
		}
	}

	double value = 0.0;
	if(value_c < 0) {
		value = valueValueEdit->value();
	}
	double cost = 0.0;

	QUrl url = fileEdit->url();

	QFile file(url.toLocalFile());
	if(!file.open(QIODevice::ReadOnly) ) {
		QMessageBox::critical(this, i18n("Error"), i18n("Couldn't open %1 for reading.", url.toString()));
		return false;
	} else if(!file.size()) {
		QMessageBox::critical(this, i18n("Error"), i18n("Error reading %1.", url.toString()));
		return false;
	}
	QTextStream fstream(&file);
	fstream.setCodec("UTF-8");

	//bool had_data = false;
	int successes = 0;
	int failed = 0;
	bool missing_columns = false, value_error = false, date_error = false;
	bool AC1_empty = false, AC2_empty = false, AC1_missing = false, AC2_missing = false, AC_security = false, AC_balancing = false, AC_same = false;
	int AC1_c_bak = AC1_c;
	int AC2_c_bak = AC2_c;
	int row = 0;
	QString line = fstream.readLine();
	QString new_ac1 = "", new_ac2 = "";
	QDate curdate = QDate::currentDate();
	while(!line.isNull()) {
		row++;
		if((first_row == 0 && !line.isEmpty() && line[0] != '#') || (first_row > 0 && row >= first_row && !line.isEmpty())) {
			QStringList columns = line.split(delimiter, QString::KeepEmptyParts);
			for(QStringList::Iterator it = columns.begin(); it != columns.end(); ++it) {
				int i = 0;
				while(i < (int) (*it).length() && ((*it)[i] == ' ' || (*it)[i] == '\t')) {
					i++;
				}
				if(!(*it).isEmpty() && (*it)[i] == '\"') {
					(*it).remove(0, i + 1);
					i = (*it).length() - 1;
					while(i > 0 && ((*it)[i] == ' ' || (*it)[i] == '\t')) {
						i--;
					}
					if(i >= 0 && (*it)[i] == '\"') {
						(*it).truncate(i);
					} else {
						QStringList::Iterator it2 = it;
						++it2;
						while(it2 != columns.end()) {
							i = (*it2).length() - 1;
							while(i > 0 && ((*it2)[i] == ' ' || (*it2)[i] == '\t')) {
								i--;
							}
							if(i >= 0 && (*it2)[i] == '\"') {
								(*it2).truncate(i);
								*it += delimiter;
								*it += *it2;
								columns.erase(it2);
								break;
							}
							*it += delimiter;
							*it += *it2;
							columns.erase(it2);
							it2 = it;
							++it2;
						}
					}
				}
				*it = (*it).trimmed();
			}
			if((int) columns.count() < min_columns) {
				if(first_row != 0) {
					missing_columns = true;
					failed++;
				}
			} else {
				bool success = true;
				if(!test && success && description_c > 0) {
					description = columns[description_c - 1];
				}
				if(success && value_c > 0) {
					if(cost_c <= 0 || !columns[value_c - 1].isEmpty()) {
						bool ok = true;
						if(first_row == 0) {
							ok = false;
							QString &str = columns[value_c - 1];
							int l = (int) str.length();
							for(int i = 0; i < l; i++) {
								if(str[i].isDigit()) {
									ok = true;
									break;
								}
							}
						}
						if(!ok) {
							failed--;
							success = false;
						} else if(test) {
							if(ci->value_format <= 0) testCSVValue(columns[value_c - 1], ci->value_format);
						} else {
							value = readCSVValue(columns[value_c - 1], ci->value_format, &ok);
							if(!ok) {
								if(first_row == 0) failed--;
								else value_error = true;
								success = false;
							}
						}
					} else {
						value = 0.0;
					}
				}
				if(success && cost_c > 0) {
					if(value == 0.0 || !columns[cost_c - 1].isEmpty()) {
						bool ok = true;
						if(first_row == 0) {
							ok = false;
							QString &str = columns[value_c - 1];
							int l = (int) str.length();
							for(int i = 0; i < l; i++) {
								if(str[i].isDigit()) {
									ok = true;
									break;
								}
							}
						}
						if(!ok) {
							failed--;
							success = false;
						} else if(test) {
							if(ci->value_format <= 0) testCSVValue(columns[cost_c - 1], ci->value_format);
						} else {
							cost = readCSVValue(columns[cost_c - 1], ci->value_format, &ok);
							if(!ok) {
								if(first_row == 0) failed--;
								else value_error = true;
								success = false;
							}
							value -= cost;
						}
					}
				}
				if(success && date_c > 0) {
					bool ok = true;
					if(first_row == 0) {
						ok = false;
						QString &str = columns[date_c - 1];
						for(int i = 0; i < (int) str.length(); i++) {
							if(str[i].isDigit()) {
								ok = true;
								break;
							}
						}
					}
					if(!ok) {
						failed--;
						success = false;
					} else if(test) {
						if(ci->p1 + ci->p2 + ci->p3 + ci->p4 > 1) testCSVDate(columns[date_c - 1], ci->p1, ci->p2, ci->p3, ci->p4, ci->ly, ci->separator);
					} else {
						date = readCSVDate(columns[date_c - 1], date_format, alt_date_format);
						if(!date.isValid()) {
							if(first_row == 0) failed--;
							else date_error = true;
							success = false;
						}
					}
				}
				if(test && ci->p1 + ci->p2 + ci->p3 + ci->p4 < 2 && ci->value_format > 0) break;
				if(test) success = false;
				if(success && type == ALL_TYPES_ID && value < 0.0) {
					AC1_c = AC2_c_bak;
					AC2_c = AC1_c_bak;
					value = -value;
				}
				if(success && AC1_c > 0) {
					QMap<QString, Account*>::iterator it_ac;
					bool found = false;
					if(type == 0 || ((type == 3 || type == 4) && value < 0.0)) {
						it_ac = eaccounts.find(columns[AC1_c - 1]);
						found = (it_ac != eaccounts.end());
					} else if(type == 1 || type == 3 || type == 4) {
						it_ac = iaccounts.find(columns[AC1_c - 1]);
						found = (it_ac != iaccounts.end());
					} else if(type == 2) {
						it_ac = aaccounts.find(columns[AC1_c - 1]);
						found = (it_ac != aaccounts.end());
					} else if(type == ALL_TYPES_ID) {
						it_ac = iaccounts.find(columns[AC1_c - 1]);
						found = (it_ac != iaccounts.end());
						if(!found) {
							it_ac = aaccounts.find(columns[AC1_c - 1]);
							found = (it_ac != aaccounts.end());
						}
					}
					if(found) {
						ac1 = it_ac.value();
						if(ac1->type() == ACCOUNT_TYPE_ASSETS && ((AssetsAccount*) ac1)->accountType() == ASSETS_TYPE_SECURITIES) {
							AC_security = true;
							success = false;
						} else if(type != 2 && type != ALL_TYPES_ID && ac1 == budget->balancingAccount) {
							AC_balancing = true;
							success = false;
						}
					} else if(columns[AC1_c - 1].isEmpty()) {
						AC1_empty = true;
						success = false;
					} else if(create_missing) {
						new_ac1 = columns[AC1_c - 1];
					} else {
						AC1_missing = true;
						success = false;
					}
				}
				if(success && AC2_c > 0) {
					QMap<QString, Account*>::iterator it_ac;
					bool found = false;
					if(type == ALL_TYPES_ID) {
						it_ac = iaccounts.find(columns[AC2_c - 1]);
						found = (it_ac != iaccounts.end());
						if(!found) {
							it_ac = aaccounts.find(columns[AC2_c - 1]);
							found = (it_ac != aaccounts.end());
						}
					} else {
						it_ac = aaccounts.find(columns[AC2_c - 1]);
						found = (it_ac != aaccounts.end());
					}
					if(found) {
						ac2 = it_ac.value();
						if(ac1 == ac2) {
							AC_same = true;
							success = false;
						} else if(ac2->type() == ACCOUNT_TYPE_ASSETS && ((AssetsAccount*) ac2)->accountType() == ASSETS_TYPE_SECURITIES) {
							AC_security = true;
							success = false;
						} else if(ac2 == budget->balancingAccount) {
							if(type != 2) {
								AC_balancing = true;
								success = false;
							} else {
								if(type == ALL_TYPES_ID && ac1->type() != ACCOUNT_TYPE_ASSETS) {
									it_ac = aaccounts.find(columns[AC1_c - 1]);
									found = it_ac != aaccounts.end();
									if(found) {
										ac1 = it_ac.value();
										if(ac1->type() == ACCOUNT_TYPE_ASSETS && ((AssetsAccount*) ac1)->accountType() == ASSETS_TYPE_SECURITIES) {
											AC_security = true;
											success = false;
										} else if(ac1 == budget->balancingAccount) {
											AC_same = true;
											success = false;
										}
									} else {
										AC_balancing = true;
										success = false;
									}
								}
								if(success) {
									value = -value;
									Account *ac1_bak = ac1;
									ac1 = ac2;
									ac2 = ac1_bak;
								}
							}
						} else if(type == ALL_TYPES_ID && ac1 == budget->balancingAccount && ac2->type() != ACCOUNT_TYPE_ASSETS) {
							it_ac = aaccounts.find(columns[AC2_c - 1]);
							found = it_ac != aaccounts.end();
							if(found) {
								ac2 = it_ac.value();
								if(ac2->type() == ACCOUNT_TYPE_ASSETS && ((AssetsAccount*) ac2)->accountType() == ASSETS_TYPE_SECURITIES) {
									AC_security = true;
									success = false;
								} else if(ac2 == budget->balancingAccount) {
									AC_same = true;
									success = false;
								}
							} else {
								AC_balancing = true;
								success = false;
							}
						}
					} else if(columns[AC2_c - 1].isEmpty()) {
						AC2_empty = true;
						success = false;
					} else if(create_missing) {
						new_ac2 = columns[AC2_c - 1];
						if(new_ac1 == new_ac2) {
							new_ac1 = "";
							new_ac2 = "";
							AC_same = true;
							success = false;
						}
					} else {
						AC2_missing = true;
						success = false;
					}
				}
				if(success && type == ALL_TYPES_ID) {
					AC1_c = AC1_c_bak;
					AC2_c = AC2_c_bak;
				}
				if(success && comments_c > 0) {
					comments = columns[comments_c - 1];
				}
				if(success) {
					if(!new_ac1.isEmpty()) {
						if(type == 0 || ((type == 3 || type == 4) && value < 0.0)) {ac1 = new ExpensesAccount(budget, new_ac1); budget->addAccount(ac1); eaccounts[ac1->name()] = ac1;}
						else if(type == 1 || type == 3 || type == 4) {ac1 = new IncomesAccount(budget, new_ac1); budget->addAccount(ac1); iaccounts[ac1->name()] = ac1;}
						else if(type == 2) {ac1 = new AssetsAccount(budget, ASSETS_TYPE_CASH, new_ac1); budget->addAccount(ac1); aaccounts[ac1->name()] = ac1;}
						new_ac1 = "";
					}
					if(!new_ac2.isEmpty()) {
						ac2 = new AssetsAccount(budget, ASSETS_TYPE_CASH, new_ac2);
						budget->addAccount(ac2);
						aaccounts[ac2->name()] = ac2;
						new_ac2 = "";
					}
					Transaction *trans = NULL;
					switch(type) {
						case 0: {
							trans = new Expense(budget, value, date, (ExpensesAccount*) ac1, (AssetsAccount*) ac2, description, comments);
							successes++;
							break;
						}
						case 1: {
							trans = new Income(budget, value, date, (IncomesAccount*) ac1, (AssetsAccount*) ac2, description, comments);
							successes++;
							break;
						}
						case 2: {
							if(ac1 == budget->balancingAccount) {
								trans = new Balancing(budget, value, date, (AssetsAccount*) ac2, description);
							} else if(value < 0.0) {
								trans = new Transfer(budget, -value, date, (AssetsAccount*) ac2, (AssetsAccount*) ac1, description, comments);
							} else {
								trans = new Transfer(budget, value, date, (AssetsAccount*) ac1, (AssetsAccount*) ac2, description, comments);
							}
							successes++;
							break;
						}
						case 3: {}
						case 4: {
							if(value < 0.0) {
								trans = new Expense(budget, -value, date, (ExpensesAccount*) ac1, (AssetsAccount*) ac2, description, comments);
							} else {
								trans = new Income(budget, value, date, (IncomesAccount*) ac1, (AssetsAccount*) ac2, description, comments);
							}
							successes++;
							break;
						}
						case ALL_TYPES_ID: {
							if(ac1 == budget->balancingAccount) {
								trans = new Balancing(budget, value, date, (AssetsAccount*) ac2, description);
							} else if(ac1->type() == ACCOUNT_TYPE_INCOMES) {
								trans = new Income(budget, value, date, (IncomesAccount*) ac1, (AssetsAccount*) ac2, description, comments);
							} else if(ac2->type() == ACCOUNT_TYPE_EXPENSES) {
								trans = new Expense(budget, value, date, (ExpensesAccount*) ac2, (AssetsAccount*) ac1, description, comments);
							} else {
								trans = new Transfer(budget, value, date, (AssetsAccount*) ac1, (AssetsAccount*) ac2, description, comments);
							}
							successes++;
							break;
						}
					}
					if(trans) {
						if(trans->date() > curdate) {
							budget->addScheduledTransaction(new ScheduledTransaction(budget, trans, NULL));
						} else {
							budget->addTransaction(trans);
						}
					}
				} else {
					failed++;
				}
			}
			//had_data = true;
			if(first_row == 0) first_row = 1;
		}
		line = fstream.readLine();
	}

	file.close();

	if(test) {
		return true;
	}

	QString info = "", details = "";
	if(successes > 0) {
		info = i18np("Successfully imported 1 transaction.", "Successfully imported %1 transactions.", successes);
	} else {
		info = i18n("Unable to import any transactions.");
	}
	if(failed > 0) {
		info += '\n';
		info += i18np("Failed to import 1 data row.", "Failed to import %1 data rows.", failed);
		if(missing_columns) {details += "\n-"; details += i18n("Required columns missing.");}
		if(value_error) {details += "\n-"; details += i18n("Invalid value.");}
		if(date_error) {details += "\n-"; details += i18n("Invalid date.");}
		if(AC1_empty) {details += "\n-"; if(type == 0 || type == 1 || type == 2) {details += i18n("Empty category name.");} else {details += i18n("Empty account name.");}}
		if(AC2_empty) {details += "\n-"; details += i18n("Empty account name.");}
		if(AC1_missing) {details += "\n-"; if(type == 0 || type == 1 || type == 2) {details += i18n("Unknown category found.");} else {details += i18n("Unknown account found.");}}
		if(AC2_missing) {details += "\n-"; details += i18n("Unknown account found.");}
		if(AC_security) {details += "\n-"; details += i18n("Cannot import security transactions (to/from security accounts).");}
		if(AC_balancing) {details += "\n-"; details += i18n("Balancing account wrongly used.");}
		if(AC_same) {details += "\n-"; details += i18n("Same to and from account/category.");}
	} else if(successes == 0) {
		info = i18n("No data found.");
	}
	if(failed > 0 || successes == 0) {
		QMessageBox::critical(this, i18n("Error"), info + details);
	} else {
		QMessageBox::information(this, i18n("Information"), info);
	}
	return successes > 0;
}
void ImportCSVDialog::accept() {
	csv_info ci;
	if(!import(true, &ci)) return;
	int ps = ci.p1 + ci.p2 + ci.p3 + ci.p4;
	if(ps == 0) {
		QMessageBox::critical(this, i18n("Error"), i18n("Unrecognized date format."));
		return;
	}
	if(ci.value_format < 0 || ps > 1) {
		QDialog *dialog = new QDialog(this, 0);
		dialog->setWindowTitle(i18n("Specify Format"));
		dialog->setModal(true);
		QVBoxLayout *box1 = new QVBoxLayout(this);
		QGridLayout *grid = new QGridLayout();
		box1->addLayout(grid);
		QLabel *label = new QLabel(i18n("The format of dates and/or numbers in the CSV file is ambiguous. Please select the correct format."), dialog);
		label->setWordWrap(true);
		grid->addWidget(label, 0, 0, 1, 2);
		QComboBox *dateFormatCombo = NULL;
		if(ps > 1) {
			grid->addWidget(new QLabel(i18n("Date format:"), dialog), 1, 0);
			dateFormatCombo = new QComboBox(dialog);
			dateFormatCombo->setEditable(false);
			if(ci.p1) {
				QString date_format = "MM";
				if(ci.separator > 0) date_format += ci.separator;
				date_format += "DD";
				if(ci.separator > 0) date_format += ci.separator;
				date_format += "YY";
				if(ci.ly) date_format += "YY";
				dateFormatCombo->addItem(date_format);
			}
			if(ci.p2) {
				QString date_format = "DD";
				if(ci.separator > 0) date_format += ci.separator;
				date_format += "MM";
				if(ci.separator > 0) date_format += ci.separator;
				date_format += "YY";
				if(ci.ly) date_format += "YY";
				dateFormatCombo->addItem(date_format);
			}
			if(ci.p3) {
				QString date_format = "YY";
				if(ci.ly) date_format += "YY";
				if(ci.separator > 0) date_format += ci.separator;
				date_format += "MM";
				if(ci.separator > 0) date_format += ci.separator;
				date_format += "DD";
				dateFormatCombo->addItem(date_format);
			}
			if(ci.p4) {
				QString date_format = "YY";
				if(ci.ly) date_format += "YY";
				if(ci.separator > 0) date_format += ci.separator;
				date_format += "DD";
				if(ci.separator > 0) date_format += ci.separator;
				date_format += "MM";
				dateFormatCombo->addItem(date_format);
			}
			grid->addWidget(dateFormatCombo, 1, 1);
		}
		QComboBox *valueFormatCombo = NULL;
		if(ci.value_format < 0) {
			grid->addWidget(new QLabel(i18n("Value format:"), dialog), ps > 1 ? 2 : 1, 0);
			valueFormatCombo = new QComboBox(dialog);
			valueFormatCombo->setEditable(false);
			valueFormatCombo->addItem("1,000,000.00");
			valueFormatCombo->addItem("1.000.000,00");
			grid->addWidget(valueFormatCombo, ps > 1 ? 2 : 1, 1);
		}
		QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		buttonBox->button(QDialogButtonBox::Cancel)->setShortcut(Qt::CTRL | Qt::Key_Return);
		buttonBox->button(QDialogButtonBox::Cancel)->setDefault(true);
		connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), dialog, SLOT(reject()));
		connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(accept()));
		box1->addWidget(buttonBox);
		if(dialog->exec() != QDialog::Accepted) {
			return;
		}
		if(ps > 1) {
			bool p1 = false, p2 = false, p3 = false, p4 = false;
			int p[4];
			int i = 0;
			if(ci.p1) {p[i] = 1; i++;}
			if(ci.p2) {p[i] = 2; i++;}
			if(ci.p3) {p[i] = 3; i++;}
			if(ci.p4) {p[i] = 4; i++;}
			switch(p[dateFormatCombo->currentIndex()]) {
				case 1: {p1 = true; break;}
				case 2: {p2 = true; break;}
				case 3: {p3 = true; break;}
				case 4: {p4 = true; break;}
			}
			ci.p1 = p1; ci.p2 = p2; ci.p3 = p3; ci.p4 = p4;
		}
		if(ci.value_format < 0) ci.value_format = valueFormatCombo->currentIndex() + 1;
		dialog->deleteLater();
	}
	if(import(false, &ci)) QWizard::accept();
}

#include "importcsvdialog.moc"
