/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016-2020 by Hanna Knutsson            *
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

#include "categoriescomparisonreport.h"
#include "eqonomize.h"
#include "eqonomizevalueedit.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMap>
#include <QObject>
#include <QPushButton>
#include <QRadioButton>
#include <QString>
#include <QTextStream>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QUrl>
#include <QFileDialog>
#include <QSaveFile>
#include <QApplication>
#include <QTemporaryFile>
#include <QMimeDatabase>
#include <QMimeType>
#include <QDateEdit>
#include <QMessageBox>
#include <QPrinter>
#include <QTextEdit>
#include <QPrintDialog>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QDebug>

#include "account.h"
#include "budget.h"
#include "recurrence.h"
#include "transaction.h"
#include "overtimereport.h"

#include <math.h>

extern QString htmlize_string(QString str);
extern QString last_document_directory;

CategoriesComparisonReport::CategoriesComparisonReport(Budget *budg, QWidget *parent, bool extra_parameters) : QWidget(parent), budget(budg), b_extra(extra_parameters) {

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	QDialogButtonBox *buttons = new QDialogButtonBox(this);
	saveButton = buttons->addButton(tr("Save As…"), QDialogButtonBox::ActionRole);
	saveButton->setAutoDefault(false);
	printButton = buttons->addButton(tr("Print…"), QDialogButtonBox::ActionRole);
	printButton->setAutoDefault(false);
	layout->addWidget(buttons);

	htmlview = new QTextEdit(this);
	htmlview->setReadOnly(true);
	layout->addWidget(htmlview);

	QSettings settings;
	settings.beginGroup("CategoriesComparisonReport");

	QWidget *settingsWidget = new QWidget(this);
	QGridLayout *settingsLayout = new QGridLayout(settingsWidget);

	settingsLayout->addWidget(new QLabel(tr("Source:"), settingsWidget), 0, 0);
	QHBoxLayout *sourceLayout = new QHBoxLayout();
	settingsLayout->addLayout(sourceLayout, 0, 1);
	sourceCombo = new QComboBox(settingsWidget);
	sourceCombo->setEditable(false);
	sourceCombo->addItem(tr("All Categories, excluding subcategories"));
	sourceCombo->addItem(tr("All Categories, including subcategories"));
	sourceCombo->addItem(tr("All Tags"));
	if(b_extra) {
		sourceCombo->addItem(tr("All Payees and Payers"));
		first_source_account_index = 4;
	} else {
		first_source_account_index = 3;
	}
	for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
		Account *account = *it;
		sourceCombo->addItem(tr("Expenses: %1").arg(account->nameWithParent()));
	}
	for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
		Account *account = *it;
		sourceCombo->addItem(tr("Incomes: %1").arg(account->nameWithParent()));
	}
	for(int i2 = 0; i2 < budget->tags.count(); i2++) {
		sourceCombo->addItem(tr("Tag: %1").arg(budget->tags[i2]));
	}
	sourceLayout->addWidget(sourceCombo);
	sourceLayout->setStretchFactor(sourceCombo, 2);

	accountCombo = new AccountsCombo(budget, settingsWidget, true);
	accountCombo->updateAccounts(ACCOUNT_TYPE_ASSETS);

	payeeDescriptionWidget = new QWidget(settingsWidget);
	QHBoxLayout *payeeLayout = new QHBoxLayout(payeeDescriptionWidget);
	QButtonGroup *group = new QButtonGroup(this);
	subsButton = new QRadioButton(tr("Subcategories"), payeeDescriptionWidget);
	subsButton->setChecked(true);
	group->addButton(subsButton);
	payeeLayout->addWidget(subsButton);
	descriptionButton = new QRadioButton(b_extra ? tr("Descriptions for", "Referring to the transaction description property (transaction title/generic article name)") : tr("Descriptions", "Referring to the transaction description property (transaction title/generic article name)"), payeeDescriptionWidget);
	group->addButton(descriptionButton);
	payeeLayout->addWidget(descriptionButton);
	if(b_extra) {
		sourceLayout->addWidget(accountCombo);
		payeeCombo = new DescriptionsCombo(4, budget, payeeDescriptionWidget);
		payeeLayout->addWidget(payeeCombo);
		payeeLayout->setStretchFactor(payeeCombo, 1);
		payeeButton = new QRadioButton(tr("Payees/payers for"), payeeDescriptionWidget);
		group->addButton(payeeButton);
		payeeLayout->addWidget(payeeButton);
		descriptionCombo = new DescriptionsCombo(0, budget, payeeDescriptionWidget);
		payeeLayout->addWidget(descriptionCombo);
		payeeLayout->setStretchFactor(descriptionCombo, 1);
		settingsLayout->addWidget(payeeDescriptionWidget, 1, 1);
		payeeDescriptionWidget->setEnabled(false);
	} else {
		sourceLayout->addWidget(payeeDescriptionWidget);
		sourceLayout->addWidget(accountCombo);
	}

	sourceLayout->setStretchFactor(accountCombo, 1);

	current_account = NULL;

	settingsLayout->addWidget(new QLabel(tr("Period:"), settingsWidget), b_extra ? 2 : 1, 0);
	QHBoxLayout *choicesLayout = new QHBoxLayout();
	settingsLayout->addLayout(choicesLayout, b_extra ? 2 : 1, 1);
	fromButton = new QCheckBox(tr("From"), settingsWidget);
	fromButton->setChecked(true);
	choicesLayout->addWidget(fromButton);
	fromEdit = new EqonomizeDateEdit(settingsWidget);
	fromEdit->setCalendarPopup(true);
	choicesLayout->addWidget(fromEdit);
	choicesLayout->setStretchFactor(fromEdit, 1);
	choicesLayout->addWidget(new QLabel(tr("To"), settingsWidget));
	toEdit = new EqonomizeDateEdit(settingsWidget);
	toEdit->setCalendarPopup(true);
	choicesLayout->addWidget(toEdit);
	choicesLayout->setStretchFactor(toEdit, 1);
	prevYearButton = new QPushButton(LOAD_ICON("eqz-previous-year"), "", settingsWidget);
	choicesLayout->addWidget(prevYearButton);
	prevMonthButton = new QPushButton(LOAD_ICON("eqz-previous-month"), "", settingsWidget);
	choicesLayout->addWidget(prevMonthButton);
	nextMonthButton = new QPushButton(LOAD_ICON("eqz-next-month"), "", settingsWidget);
	choicesLayout->addWidget(nextMonthButton);
	nextYearButton = new QPushButton(LOAD_ICON("eqz-next-year"), "", settingsWidget);
	choicesLayout->addWidget(nextYearButton);
	choicesLayout->addStretch(1);

	settingsLayout->addWidget(new QLabel(tr("Columns:"), settingsWidget), b_extra ? 3 : 2, 0);
	QHBoxLayout *enabledLayout = new QHBoxLayout();
	group = new QButtonGroup(this);
	monthsButton = new QRadioButton(tr("Months"), settingsWidget);
	group->addButton(monthsButton, 1);
	enabledLayout->addWidget(monthsButton);
	yearsButton = new QRadioButton(tr("Years"), settingsWidget);
	group->addButton(yearsButton, 2);
	enabledLayout->addWidget(yearsButton);
	tagsButton = new QRadioButton(tr("Tags"), settingsWidget);
	group->addButton(tagsButton, 3);
	enabledLayout->addWidget(tagsButton);
	totalButton = new QRadioButton(tr("Total:"), settingsWidget);
	group->addButton(totalButton, 0);
	enabledLayout->addWidget(totalButton);
	switch(settings.value("columns", 0).toInt()) {
		case 1: {monthsButton->setChecked(true); break;}
		case 2: {yearsButton->setChecked(true); break;}
		case 3: {tagsButton->setChecked(true); break;}
		default: {totalButton->setChecked(true); break;}
	}
	settingsLayout->addLayout(enabledLayout, b_extra ? 3 : 2, 1);
	valueButton = new QCheckBox(tr("Value"), settingsWidget);
	valueButton->setChecked(settings.value("valueEnabled", true).toBool());
	enabledLayout->addWidget(valueButton);
	dailyButton = new QCheckBox(tr("Daily"), settingsWidget);
	dailyButton->setChecked(settings.value("dailyAverageEnabled", true).toBool());
	enabledLayout->addWidget(dailyButton);
	monthlyButton = new QCheckBox(tr("Monthly"), settingsWidget);
	monthlyButton->setChecked(settings.value("monthlyAverageEnabled", true).toBool());
	enabledLayout->addWidget(monthlyButton);
	yearlyButton = new QCheckBox(tr("Yearly"), settingsWidget);
	yearlyButton->setChecked(settings.value("yearlyEnabled", false).toBool());
	enabledLayout->addWidget(yearlyButton);
	countButton = new QCheckBox(tr("Quantity"), settingsWidget);
	countButton->setChecked(settings.value("transactionCountEnabled", true).toBool());
	enabledLayout->addWidget(countButton);
	perButton = new QCheckBox(tr("Average value"), settingsWidget);
	perButton->setChecked(settings.value("valuePerTransactionEnabled", false).toBool());
	enabledLayout->addWidget(perButton);
	enabledLayout->addStretch(1);
	valueButton->setEnabled(totalButton->isChecked());
	dailyButton->setEnabled(totalButton->isChecked());
	monthlyButton->setEnabled(totalButton->isChecked());
	yearlyButton->setEnabled(totalButton->isChecked());
	countButton->setEnabled(totalButton->isChecked());
	perButton->setEnabled(totalButton->isChecked());

	settings.endGroup();

	layout->addWidget(settingsWidget);

	resetOptions();

	connect(group, SIGNAL(buttonToggled(int, bool)), this, SLOT(columnsToggled(int, bool)));
	connect(subsButton, SIGNAL(toggled(bool)), this, SLOT(subsToggled(bool)));
	connect(descriptionButton, SIGNAL(toggled(bool)), this, SLOT(descriptionToggled(bool)));
	if(b_extra) {
		connect(payeeButton, SIGNAL(toggled(bool)), this, SLOT(payeeToggled(bool)));
		connect(payeeCombo, SIGNAL(selectedItemsChanged()), this, SLOT(payeeChanged()));
		connect(descriptionCombo, SIGNAL(selectedItemsChanged()), this, SLOT(descriptionChanged()));
	}
	connect(sourceCombo, SIGNAL(activated(int)), this, SLOT(sourceChanged(int)));
	connect(accountCombo, SIGNAL(selectedAccountsChanged()), this, SLOT(updateDisplay()));
	connect(valueButton, SIGNAL(toggled(bool)), this, SLOT(updateDisplay()));
	connect(dailyButton, SIGNAL(toggled(bool)), this, SLOT(updateDisplay()));
	connect(monthlyButton, SIGNAL(toggled(bool)), this, SLOT(updateDisplay()));
	connect(yearlyButton, SIGNAL(toggled(bool)), this, SLOT(updateDisplay()));
	connect(countButton, SIGNAL(toggled(bool)), this, SLOT(updateDisplay()));
	connect(perButton, SIGNAL(toggled(bool)), this, SLOT(updateDisplay()));
	connect(prevMonthButton, SIGNAL(clicked()), this, SLOT(prevMonth()));
	connect(nextMonthButton, SIGNAL(clicked()), this, SLOT(nextMonth()));
	connect(prevYearButton, SIGNAL(clicked()), this, SLOT(prevYear()));
	connect(nextYearButton, SIGNAL(clicked()), this, SLOT(nextYear()));
	connect(fromButton, SIGNAL(toggled(bool)), fromEdit, SLOT(setEnabled(bool)));
	connect(fromButton, SIGNAL(toggled(bool)), this, SLOT(updateDisplay()));
	connect(fromEdit, SIGNAL(dateChanged(const QDate&)), this, SLOT(fromChanged(const QDate&)));
	connect(toEdit, SIGNAL(dateChanged(const QDate&)), this, SLOT(toChanged(const QDate&)));
	connect(saveButton, SIGNAL(clicked()), this, SLOT(save()));
	connect(printButton, SIGNAL(clicked()), this, SLOT(print()));

}

void CategoriesComparisonReport::resetOptions() {
	block_display_update = true;
	QDate first_date;
	for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constBegin(); it != budget->transactions.constEnd(); ++it) {
		Transaction *trans = *it;
		if(trans->fromAccount()->type() != ACCOUNT_TYPE_ASSETS || trans->toAccount()->type() != ACCOUNT_TYPE_ASSETS) {
			first_date = trans->date();
			break;
		}
	}
	to_date = QDate::currentDate();
	from_date = to_date.addYears(-1).addDays(1);
	if(first_date.isNull()) {
		from_date = to_date;
	} else if(from_date < first_date) {
		from_date = first_date;
	}
	subsButton->setChecked(false);
	fromEdit->setDate(from_date);
	toEdit->setDate(to_date);
	fromButton->setChecked(true);
	columnsToggled(tagsButton->isChecked() ? 3 : (yearsButton->isChecked() ? 2 : (monthsButton->isChecked() ? 1 : 0)), true);
	sourceCombo->setCurrentIndex(0);
	accountCombo->blockSignals(true);
	accountCombo->deselectAll();
	accountCombo->blockSignals(false);
	block_display_update = false;
	sourceChanged(0);
}
void CategoriesComparisonReport::payeeToggled(bool b) {
	if(b) updateDisplay();
}
void CategoriesComparisonReport::descriptionToggled(bool b) {
	if(b) updateDisplay();
}
void CategoriesComparisonReport::subsToggled(bool b) {
	if(b) updateDisplay();
}
void CategoriesComparisonReport::columnsToggled(int id, bool b) {
	if(!b) return;
	valueButton->setEnabled(id == 0);
	dailyButton->setEnabled(id == 0);
	monthlyButton->setEnabled(id == 0);
	yearlyButton->setEnabled(id == 0);
	countButton->setEnabled(id == 0);
	perButton->setEnabled(id == 0);
	if(id == 1) {
		QDate first_date = budget->firstBudgetDay(from_date);
		if(!budget->isLastBudgetDay(to_date)) budget->addBudgetMonthsSetLast(to_date, -1);
		from_date = budget->firstBudgetDay(to_date);
		if(to_date < first_date) {
			budget->addBudgetMonthsSetLast(to_date, 1);
			from_date = first_date;
		} else {
			for(int i = 1; i < 12 && from_date > first_date; i++) {
				budget->addBudgetMonthsSetFirst(from_date, -1);
			}
		}
		fromButton->blockSignals(true);
		fromButton->setChecked(true);
		fromButton->blockSignals(false);
		toEdit->blockSignals(true);
		toEdit->setDate(to_date);
		toEdit->blockSignals(false);
		fromEdit->blockSignals(true);
		fromEdit->setDate(from_date);
		fromEdit->blockSignals(false);
	} else if(id == 2) {
		if(to_date != budget->lastBudgetDayOfYear(to_date)) {
			budget->addBudgetMonthsSetLast(to_date, -12);
			to_date = budget->lastBudgetDayOfYear(to_date);
		}
		QDate first_date;
		for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constBegin(); it != budget->transactions.constEnd(); ++it) {
			Transaction *trans = *it;
			if(trans->fromAccount()->type() != ACCOUNT_TYPE_ASSETS || trans->toAccount()->type() != ACCOUNT_TYPE_ASSETS) {
				first_date = trans->date();
				break;
			}
		}
		if(first_date.isNull()) {
			first_date = QDate::currentDate();
			to_date = QDate::currentDate();
		} else {
			if(budget->dayOfBudgetYear(first_date) > 15) {
				budget->addBudgetMonthsSetFirst(first_date, 12);
				first_date = budget->firstBudgetDayOfYear(first_date);
			}
			int year1 = budget->budgetYear(first_date);
			int year2 = budget->budgetYear(to_date);
			if(year2 - year1 < 1) to_date = QDate::currentDate();
		}
		fromButton->blockSignals(true);
		fromButton->setChecked(false);
		fromButton->blockSignals(false);
		toEdit->blockSignals(true);
		toEdit->setDate(to_date);
		toEdit->blockSignals(false);
	} else {
		fromButton->blockSignals(true);
		fromButton->setChecked(true);
		fromButton->blockSignals(false);
	}
	fromEdit->setEnabled(fromButton->isChecked());
	updateDisplay();
}
void CategoriesComparisonReport::payeeChanged() {
	payeeButton->blockSignals(true);
	descriptionButton->blockSignals(true);
	descriptionButton->setChecked(true);
	payeeButton->setChecked(false);
	payeeButton->blockSignals(false);
	descriptionButton->blockSignals(false);
	updateDisplay();
}
void CategoriesComparisonReport::descriptionChanged() {
	payeeButton->blockSignals(true);
	descriptionButton->blockSignals(true);
	payeeButton->setChecked(true);
	descriptionButton->setChecked(false);
	payeeButton->blockSignals(false);
	descriptionButton->blockSignals(false);
	updateDisplay();
}
void CategoriesComparisonReport::sourceChanged(int i) {
	if(first_source_account_index == 3 && i >= 3) i++;
	i -= 3;
	current_account = NULL;
	current_tag = "";
	if(i > 0) {
		if(i - 1 < budget->expensesAccounts.count()) current_account = budget->expensesAccounts.at(i - 1);
		else if(i - 1 - budget->expensesAccounts.count() < budget->incomesAccounts.count()) current_account = budget->incomesAccounts.at(i - 1 - budget->expensesAccounts.count());
		else if(i - 1 - budget->expensesAccounts.count() - budget->incomesAccounts.count() < budget->tags.count()) current_tag = budget->tags.at(i - 1 - budget->expensesAccounts.count() - budget->incomesAccounts.count());
		i++;
	}
	if((current_account && current_account->subCategories.isEmpty()) || !current_tag.isEmpty()) {
		subsButton->setEnabled(false);
		if(subsButton->isChecked()) {
			descriptionButton->blockSignals(true);
			descriptionButton->setChecked(true);
			descriptionButton->blockSignals(false);
		}
	} else {
		subsButton->setEnabled(true);
	}
	payeeDescriptionWidget->setEnabled(i > 0 && (current_account || !current_tag.isEmpty()));
	if(b_extra) {
		payeeCombo->blockSignals(true);
		descriptionCombo->blockSignals(true);
		if(i <= 0) {
			current_account = NULL;
			current_tag = "";
			payeeCombo->clear();
			descriptionCombo->clear();
		} else {
			if(current_account || !current_tag.isEmpty()) {
				descriptionCombo->addItem(tr("All descriptions", "Referring to the transaction description property (transaction title/generic article name)"));
				QMap<QString, QString> descriptions, payees;
				bool b_income, b_expense;
				for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constEnd(); it != budget->transactions.constBegin();) {
					--it;
					Transaction *trans = *it;
					if((!current_account && trans->hasTag(current_tag, true) && ((trans->type() == TRANSACTION_TYPE_EXPENSE || trans->type() == TRANSACTION_TYPE_INCOME))) || (current_account && (trans->fromAccount() == current_account || trans->toAccount() == current_account))) {
						if(!descriptions.contains(trans->description().toLower())) descriptions[trans->description().toLower()] = trans->description();
						if(trans->type() == TRANSACTION_TYPE_EXPENSE) {
							b_expense = true;
							if(!payees.contains(((Expense*) trans)->payee().toLower())) payees[((Expense*) trans)->payee().toLower()] = ((Expense*) trans)->payee();
						} else if(trans->type() == TRANSACTION_TYPE_INCOME) {
							b_income = true;
							if(!payees.contains(((Income*) trans)->payer().toLower())) payees[((Income*) trans)->payer().toLower()] = ((Income*) trans)->payer();
						}
					}
				}
				if((!current_account && b_expense && !b_income) || (current_account && current_account->type() == ACCOUNT_TYPE_EXPENSES)) payeeCombo->setItemType(2);
				else if(current_account || (!b_expense && b_income)) payeeCombo->setItemType(3);
				else payeeCombo->setItemType(4);
				QStringList dlist;
				QMap<QString, QString>::iterator it_e = descriptions.end();
				for(QMap<QString, QString>::iterator it = descriptions.begin(); it != it_e; ++it) {
					dlist << *it;
				}
				descriptionCombo->updateItems(dlist);
				QStringList plist;
				QMap<QString, QString>::iterator it2_e = payees.end();
				for(QMap<QString, QString>::iterator it2 = payees.begin(); it2 != it2_e; ++it2) {
					plist << it2.value();
				}
				payeeCombo->updateItems(plist);
			}
		}
		payeeCombo->blockSignals(false);
		descriptionCombo->blockSignals(false);
	}
	updateDisplay();
}
void CategoriesComparisonReport::saveConfig() {
	QSettings settings;
	settings.beginGroup("CategoriesComparisonReport");
	settings.setValue("size", ((QDialog*) parent())->size());
	settings.setValue("columns", tagsButton->isChecked() ? 3 : (yearsButton->isChecked() ? 2 : (monthsButton->isChecked() ? 1 : 0)));
	settings.setValue("valueEnabled", valueButton->isChecked());
	settings.setValue("dailyAverageEnabled", dailyButton->isChecked());
	settings.setValue("monthlyAverageEnabled", monthlyButton->isChecked());
	settings.setValue("yearlyAverageEnabled", yearlyButton->isChecked());
	settings.setValue("transactionCountEnabled", countButton->isChecked());
	settings.setValue("valuePerTransactionEnabled", perButton->isChecked());
	settings.endGroup();
}
void CategoriesComparisonReport::toChanged(const QDate &date) {
	bool error = false;
	if(!date.isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		error = true;
	}
	if(!error && fromEdit->date() > date) {
		/*if(fromButton->isChecked()) {
			QMessageBox::critical(this, tr("Error"), tr("To date is before from date."));
		}*/
		if(budget->isFirstBudgetDay(to_date)) {
			from_date = budget->firstBudgetDay(date);
		} else {
			from_date = date.addDays(-from_date.daysTo(to_date));
		}
		from_date = date;
		fromEdit->blockSignals(true);
		fromEdit->setDate(from_date);
		fromEdit->blockSignals(false);
	}
	if(error) {
		toEdit->setFocus();
		toEdit->blockSignals(true);
		toEdit->setDate(to_date);
		toEdit->blockSignals(false);
		toEdit->selectAll();
		return;
	}
	to_date = date;
	updateDisplay();
}
void CategoriesComparisonReport::fromChanged(const QDate &date) {
	bool error = false;
	if(!date.isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		error = true;
	}
	if(!error && date > toEdit->date()) {
		//QMessageBox::critical(this, tr("Error"), tr("From date is after to date."));
		if(budget->isLastBudgetDay(to_date)) {
			to_date = budget->lastBudgetDay(date);
		} else {
			to_date = date.addDays(from_date.daysTo(to_date));
		}
		if(to_date > QDate::currentDate() && from_date <= QDate::currentDate()) to_date = QDate::currentDate();
		toEdit->blockSignals(true);
		toEdit->setDate(to_date);
		toEdit->blockSignals(false);
	}
	if(error) {
		fromEdit->setFocus();
		fromEdit->blockSignals(true);
		fromEdit->setDate(from_date);
		fromEdit->blockSignals(false);
		fromEdit->selectAll();
		return;
	}
	from_date = date;
	if(fromButton->isChecked()) updateDisplay();
}
void CategoriesComparisonReport::prevMonth() {
	fromEdit->blockSignals(true);
	toEdit->blockSignals(true);
	budget->goForwardBudgetMonths(from_date, to_date, -1);
	fromEdit->setDate(from_date);
	toEdit->setDate(to_date);
	fromEdit->blockSignals(false);
	toEdit->blockSignals(false);
	updateDisplay();
}
void CategoriesComparisonReport::nextMonth() {
	fromEdit->blockSignals(true);
	toEdit->blockSignals(true);
	budget->goForwardBudgetMonths(from_date, to_date, 1);
	fromEdit->setDate(from_date);
	toEdit->setDate(to_date);
	fromEdit->blockSignals(false);
	toEdit->blockSignals(false);
	updateDisplay();
}
void CategoriesComparisonReport::prevYear() {
	fromEdit->blockSignals(true);
	toEdit->blockSignals(true);
	budget->goForwardBudgetMonths(from_date, to_date, -12);
	fromEdit->setDate(from_date);
	toEdit->setDate(to_date);
	fromEdit->blockSignals(false);
	toEdit->blockSignals(false);
	updateDisplay();
}
void CategoriesComparisonReport::nextYear() {
	fromEdit->blockSignals(true);
	toEdit->blockSignals(true);
	budget->goForwardBudgetMonths(from_date, to_date, 12);
	fromEdit->setDate(from_date);
	toEdit->setDate(to_date);
	fromEdit->blockSignals(false);
	toEdit->blockSignals(false);
	updateDisplay();
}

void CategoriesComparisonReport::save() {
	QMimeDatabase db;
	QFileDialog fileDialog(this);
	fileDialog.setNameFilter(db.mimeTypeForName("text/html").filterString());
	fileDialog.setDefaultSuffix(db.mimeTypeForName("text/html").preferredSuffix());
	fileDialog.setAcceptMode(QFileDialog::AcceptSave);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
	fileDialog.setSupportedSchemes(QStringList("file"));
#endif
	fileDialog.setDirectory(last_document_directory);
	QString url;
	if(!fileDialog.exec()) return;
	QStringList urls = fileDialog.selectedFiles();
	if(urls.isEmpty()) return;
	url = urls[0];
	QSaveFile ofile(url);
	ofile.open(QIODevice::WriteOnly);
	ofile.setPermissions((QFile::Permissions) 0x0660);
	if(!ofile.isOpen()) {
		ofile.cancelWriting();
		QMessageBox::critical(this, tr("Error"), tr("Couldn't open file for writing."));
		return;
	}
	last_document_directory = fileDialog.directory().absolutePath();
	QTextStream outf(&ofile);
	outf.setCodec("UTF-8");
	outf << source;
	if(!ofile.commit()) {
		QMessageBox::critical(this, tr("Error"), tr("Error while writing file; file was not saved."));
		return;
	}

}

void CategoriesComparisonReport::print() {
	QPrinter printer;
	QPrintDialog print_dialog(&printer, this);
	if(print_dialog.exec() == QDialog::Accepted) {
		htmlview->print(&printer);
	}
}

void CategoriesComparisonReport::updateDisplay() {

	if(!isVisible() || block_display_update) return;

	int columns = 1;
	bool enabled[6];
	enabled[0] = valueButton->isChecked();
	enabled[1] = dailyButton->isChecked();
	enabled[2] = monthlyButton->isChecked();
	enabled[3] = yearlyButton->isChecked();
	enabled[4] = countButton->isChecked();
	enabled[5] = perButton->isChecked();
	for(size_t i = 0; i < 6; i++) {
		if(enabled[i]) columns++;
	}

	QMap<Account*, QVector<double> > month_values;
	QMap<QString, QVector<double> > desc_month_values;
	QMap<QString, QVector<double> > tag_month_values;
	QMap<Account*, QMap<QString, double> > tag_values;
	QMap<QString, QMap<QString, double> > desc_tag_values;

	QMap<Account*, double> values;
	QMap<Account*, double> counts;
	QMap<QString, QString> desc_map;
	QMap<QString, double> desc_values;
	QMap<QString, double> desc_counts;
	QMap<QString, double> tag_value, tag_incomes, tag_costs, tag_counts;
	double incomes = 0.0, costs = 0.0;
	QVector<double> month_incomes, month_costs, month_value;
	double incomes_count = 0.0, costs_count = 0.0;
	double value_count = 0.0;
	double value = 0.0;

	current_account = NULL;
	current_tag = "";

	bool b_expense = false, b_income = false;

	bool assets_selected = accountCombo->isEnabled() && !accountCombo->allAccountsSelected();
	bool description_selected = b_extra && payeeButton->isChecked() && descriptionCombo->isEnabled() && !descriptionCombo->allItemsSelected();
	bool payee_selected = b_extra && descriptionButton->isChecked() && payeeCombo->isEnabled() && !payeeCombo->allItemsSelected();

	bool include_subs = false;

	int i_source = sourceCombo->currentIndex();
	if(first_source_account_index == 3 && i_source >= 3) i_source++;
	if(i_source == 1) {
		i_source = 0;
		include_subs = true;
	} else if(i_source == 2) {
		i_source = -2;
	} else if(i_source == 3) {
		i_source = -1;
	} else if(i_source > 3) {
		i_source -= 3;
		if(i_source - 1 < budget->expensesAccounts.count()) current_account = budget->expensesAccounts.at(i_source - 1);
		else if(i_source - 1 - budget->expensesAccounts.count() < budget->incomesAccounts.count()) current_account = budget->incomesAccounts.at(i_source - 1 - budget->expensesAccounts.count());
		else if(i_source - 1 - budget->expensesAccounts.count() - budget->incomesAccounts.count() < budget->tags.count()) current_tag = budget->tags.at(i_source - 1 - budget->expensesAccounts.count() - budget->incomesAccounts.count());
		if(!current_account && current_tag.isEmpty()) return;
		if(b_extra) {
			if(current_account && subsButton->isChecked()) {
				i_source = 1;
				include_subs = !current_account->subCategories.isEmpty();
			} else if(descriptionButton->isChecked()) {
				if(!payee_selected) {
					i_source = 1;
				} else {
					i_source = 3;
				}
			} else {
				if(!description_selected) {
					i_source = 2;
				} else {
					i_source = 4;
				}
			}
		} else {
			i_source = 1;
			include_subs = current_account && subsButton->isChecked() && !current_account->subCategories.isEmpty();
		}

	}

	QDate first_date, last_date, curmonth;
	if(fromButton->isChecked()) {
		first_date = from_date;
	} else {
		for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constBegin(); it != budget->transactions.constEnd(); ++it) {
			Transaction *trans = *it;
			if(trans->fromAccount()->type() != ACCOUNT_TYPE_ASSETS || trans->toAccount()->type() != ACCOUNT_TYPE_ASSETS) {
				first_date = trans->date();
				break;
			}
		}
		if(first_date.isNull()) first_date = QDate::currentDate();
		if(first_date > to_date) first_date = to_date;
	}
	last_date = to_date;

	int i_months =  0;
	bool b_years = false;
	if(monthsButton->isChecked()) {
		curmonth = budget->lastBudgetDay(first_date);
		i_months = 1;
		while(curmonth < last_date) {
			budget->addBudgetMonthsSetLast(curmonth, 1);
			i_months++;
		}
	} else if(yearsButton->isChecked()) {
		if(!fromButton->isChecked()) {
			int year1 = budget->budgetYear(first_date);
			int year2 = budget->budgetYear(last_date);
			if(year2 - year1 > 1 && budget->dayOfBudgetYear(first_date) > 15) {
				budget->addBudgetMonthsSetFirst(first_date, 12);
				first_date = budget->firstBudgetDayOfYear(first_date);
			}
		}
		b_years = true;
		curmonth = budget->lastBudgetDayOfYear(first_date);
		i_months = 1;
		while(curmonth < last_date) {
			budget->addBudgetMonthsSetLast(curmonth, 12);
			i_months++;
		}
	}
	bool b_tags = tagsButton->isChecked();
	if(b_tags) {
		columns = budget->tags.count() + 2;
		for(size_t i = 0; i < 6; i++) {
			enabled[i] = false;
		}
		for(int i = 0; i < budget->tags.count(); i++) {
			tag_value[budget->tags[i]] = 0.0;
			tag_costs[budget->tags[i]] = 0.0;
			tag_incomes[budget->tags[i]] = 0.0;
		}
	}
	if(i_months > 0) {
		columns = i_months + 2;
		for(size_t i = 0; i < 6; i++) {
			enabled[i] = false;
		}
		month_value.fill(0.0, i_months);
		month_costs.fill(0.0, i_months);
		month_incomes.fill(0.0, i_months);
	}

	AccountType type = ACCOUNT_TYPE_EXPENSES;
	if(current_account) type = current_account->type();
	switch(i_source) {
		case -2: {
			for(int i = 0; i < budget->tags.count(); i++) {
				desc_values[budget->tags[i]] = 0.0;
				desc_counts[budget->tags[i]] = 0.0;
				desc_map[budget->tags[i]] = budget->tags[i];
				if(i_months > 0) desc_month_values[budget->tags[i]].fill(0.0, i_months);
				if(b_tags) {
					for(int i2 = 0; i2 < budget->tags.count(); i2++) desc_tag_values[budget->tags[i]][budget->tags[i2]] = 0.0;
				}
			}
			break;
		}
		case -1: {
			for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constEnd(); it != budget->transactions.constBegin();) {
				--it;
				Transaction *trans = *it;
				if(trans->date() <= last_date && (!assets_selected || accountCombo->testTransactionRelation(trans))) {
					if(trans->date() < first_date) break;
					if(trans->type() == TRANSACTION_TYPE_EXPENSE && !desc_map.contains(((Expense*) trans)->payee().toLower())) {
						QString desc = ((Expense*) trans)->payee().toLower();
						desc_map[desc] = ((Expense*) trans)->payee();
						desc_values[desc] = 0.0;
						desc_counts[desc] = 0.0;
						if(i_months > 0) desc_month_values[desc].fill(0.0, i_months);
						if(b_tags) {
							for(int i = 0; i < budget->tags.count(); i++) desc_tag_values[desc][budget->tags[i]] = 0.0;
						}
					} else if(trans->type() == TRANSACTION_TYPE_INCOME && !desc_map.contains(((Income*) trans)->payer().toLower())) {
						QString desc = ((Income*) trans)->payer().toLower();
						desc_map[desc] = ((Income*) trans)->payer();
						desc_values[desc] = 0.0;
						desc_counts[desc] = 0.0;
						if(i_months > 0) desc_month_values[desc].fill(0.0, i_months);
						if(b_tags) {
							for(int i = 0; i < budget->tags.count(); i++) desc_tag_values[desc][budget->tags[i]] = 0.0;
						}
					}
				}
			}
			Transaction *trans = NULL;
			int split_i = 0;
			for(ScheduledTransactionList<ScheduledTransaction*>::const_iterator it = budget->scheduledTransactions.constBegin(); it != budget->scheduledTransactions.constEnd();) {
				ScheduledTransaction *strans = *it;
				while(split_i == 0 && strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT && ((SplitTransaction*) strans->transaction())->count() == 0) {
					++it;
					if(it == budget->scheduledTransactions.constEnd()) break;
					strans = *it;
				}
				if(strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT) {
					trans = ((SplitTransaction*) strans->transaction())->at(split_i);
					split_i++;
				} else {
					trans = (Transaction*) strans->transaction();
				}
				if(trans->date() >= first_date && (!assets_selected || accountCombo->testTransactionRelation(trans))) {
					if(trans->date() > last_date) break;
					if(trans->type() == TRANSACTION_TYPE_EXPENSE && !desc_map.contains(((Expense*) trans)->payee().toLower())) {
						QString desc = ((Expense*) trans)->payee().toLower();
						desc_map[desc] = ((Expense*) trans)->payee();
						desc_values[desc] = 0.0;
						desc_counts[desc] = 0.0;
						if(i_months > 0) desc_month_values[desc].fill(0.0, i_months);
						if(b_tags) {
							for(int i = 0; i < budget->tags.count(); i++) desc_tag_values[desc][budget->tags[i]] = 0.0;
						}
					} else if(trans->type() == TRANSACTION_TYPE_INCOME && !desc_map.contains(((Income*) trans)->payer().toLower())) {
						QString desc = ((Income*) trans)->payer().toLower();
						desc_map[desc] = ((Income*) trans)->payer();
						desc_values[desc] = 0.0;
						desc_counts[desc] = 0.0;
						if(i_months > 0) desc_month_values[desc].fill(0.0, i_months);
						if(b_tags) {
							for(int i = 0; i < budget->tags.count(); i++) desc_tag_values[desc][budget->tags[i]] = 0.0;
						}
					}
				}
				if(strans->transaction()->generaltype() != GENERAL_TRANSACTION_TYPE_SPLIT || split_i >= ((SplitTransaction*) strans->transaction())->count()) {
					++it;
					split_i = 0;
				}
			}
		}
		case 0: {
			for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
				CategoryAccount *account = *it;
				if(include_subs || !account->parentCategory()) {
					values[account] = 0.0;
					counts[account] = 0.0;
					if(i_months > 0) month_values[account].fill(0.0, i_months);
					if(b_tags) {
						for(int i = 0; i < budget->tags.count(); i++) tag_values[account][budget->tags[i]] = 0.0;
					}
				}
			}
			for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
				CategoryAccount *account = *it;
				if(include_subs || !account->parentCategory()) {
					values[account] = 0.0;
					counts[account] = 0.0;
					if(i_months > 0) month_values[account].fill(0.0, i_months);
					if(b_tags) {
						for(int i = 0; i < budget->tags.count(); i++) tag_values[account][budget->tags[i]] = 0.0;
					}
				}
			}
			if(i_months > 0) month_values[budget->null_incomes_account].fill(0.0, i_months);
			if(b_tags) {
				for(int i = 0; i < budget->tags.count(); i++) tag_values[budget->null_incomes_account][budget->tags[i]] = 0.0;
			}
			break;
		}
		default: {
			if(include_subs) {
				values[current_account] = 0.0;
				counts[current_account] = 0.0;
				if(i_months > 0) month_values[current_account].fill(0.0, i_months);
				if(b_tags) {
					for(int i = 0; i < budget->tags.count(); i++) tag_values[current_account][budget->tags[i]] = 0.0;
				}
				for(AccountList<CategoryAccount*>::const_iterator it = current_account->subCategories.constBegin(); it != current_account->subCategories.constEnd(); ++it) {
					CategoryAccount *account = *it;
					values[account] = 0.0;
					counts[account] = 0.0;
					if(i_months > 0) month_values[account].fill(0.0, i_months);
					if(b_tags) {
						for(int i = 0; i < budget->tags.count(); i++) tag_values[account][budget->tags[i]] = 0.0;					}
				}
			} else {
				for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constEnd(); it != budget->transactions.constBegin();) {
					--it;
					Transaction *trans = *it;
					if(trans->date() <= last_date && (!assets_selected || accountCombo->testTransactionRelation(trans))) {
						if(trans->date() < first_date) break;
						if(((current_account && (trans->fromAccount() == current_account || trans->toAccount() == current_account)) || (!current_account && trans->hasTag(current_tag, true) && (trans->type() == TRANSACTION_TYPE_EXPENSE || trans->type() == TRANSACTION_TYPE_INCOME))) && (i_source <= 2 || (i_source == 4 && descriptionCombo->testTransaction(trans)) || (i_source == 3 && ((trans->type() == TRANSACTION_TYPE_EXPENSE && payeeCombo->testTransaction(trans)) || (trans->type() == TRANSACTION_TYPE_INCOME && payeeCombo->testTransaction(trans)))))) {
							if(i_source == 2 || i_source == 4) {
								if(trans->type() == TRANSACTION_TYPE_EXPENSE && !desc_map.contains(((Expense*) trans)->payee().toLower())) {
									QString desc = ((Expense*) trans)->payee().toLower();
									desc_map[desc] = ((Expense*) trans)->payee();
									desc_values[desc] = 0.0;
									desc_counts[desc] = 0.0;
									if(i_months > 0) desc_month_values[desc].fill(0.0, i_months);
									if(b_tags) {
										for(int i = 0; i < budget->tags.count(); i++) desc_tag_values[desc][budget->tags[i]] = 0.0;
									}
								} else if(trans->type() == TRANSACTION_TYPE_INCOME && !desc_map.contains(((Income*) trans)->payer().toLower())) {
									QString desc = ((Income*) trans)->payer().toLower();
									desc_map[desc] = ((Income*) trans)->payer();
									desc_values[desc] = 0.0;
									desc_counts[desc] = 0.0;
									if(i_months > 0) desc_month_values[desc].fill(0.0, i_months);
									if(b_tags) {
										for(int i = 0; i < budget->tags.count(); i++) desc_tag_values[desc][budget->tags[i]] = 0.0;
									}
								}
							} else if(!desc_map.contains(trans->description().toLower())) {
								QString desc = trans->description().toLower();
								desc_map[desc] = trans->description();
								desc_values[desc] = 0.0;
								desc_counts[desc] = 0.0;
								if(i_months > 0) desc_month_values[desc].fill(0.0, i_months);
								if(b_tags) {
									for(int i = 0; i < budget->tags.count(); i++) desc_tag_values[desc][budget->tags[i]] = 0.0;
								}
							}
						}
					}
				}
				Transaction *trans = NULL;
				int split_i = 0;
				for(ScheduledTransactionList<ScheduledTransaction*>::const_iterator it = budget->scheduledTransactions.constBegin(); it != budget->scheduledTransactions.constEnd();) {
					ScheduledTransaction *strans = *it;
					while(split_i == 0 && strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT && ((SplitTransaction*) strans->transaction())->count() == 0) {
						++it;
						if(it == budget->scheduledTransactions.constEnd()) break;
						strans = *it;
					}
					if(strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT) {
						trans = ((SplitTransaction*) strans->transaction())->at(split_i);
						split_i++;
					} else {
						trans = (Transaction*) strans->transaction();
					}
					if(trans->date() >= first_date && (!assets_selected || accountCombo->testTransactionRelation(trans))) {
						if(trans->date() > last_date) break;
						if(((current_account && (trans->fromAccount() == current_account || trans->toAccount() == current_account)) || (!current_account && trans->hasTag(current_tag, true) && (trans->type() == TRANSACTION_TYPE_EXPENSE || trans->type() == TRANSACTION_TYPE_INCOME))) && (i_source <= 2 || (i_source == 4 && descriptionCombo->testTransaction(trans)) || (i_source == 3 && ((trans->type() == TRANSACTION_TYPE_EXPENSE && payeeCombo->testTransaction(trans)) || (trans->type() == TRANSACTION_TYPE_INCOME && payeeCombo->testTransaction(trans)))))) {
							if(i_source == 2 || i_source == 4) {
								if(trans->type() == TRANSACTION_TYPE_EXPENSE && !desc_map.contains(((Expense*) trans)->payee().toLower())) {
									QString desc = ((Expense*) trans)->payee().toLower();
									desc_map[desc] = ((Expense*) trans)->payee();
									desc_values[desc] = 0.0;
									desc_counts[desc] = 0.0;
									if(i_months > 0) desc_month_values[desc].fill(0.0, i_months);
									if(b_tags) {
										for(int i = 0; i < budget->tags.count(); i++) desc_tag_values[desc][budget->tags[i]] = 0.0;
									}
								} else if(trans->type() == TRANSACTION_TYPE_INCOME && !desc_map.contains(((Income*) trans)->payer().toLower())) {
									QString desc = ((Income*) trans)->payer().toLower();
									desc_map[desc] = ((Income*) trans)->payer();
									desc_values[desc] = 0.0;
									desc_counts[desc] = 0.0;
									if(i_months > 0) desc_month_values[desc].fill(0.0, i_months);
									if(b_tags) {
										for(int i = 0; i < budget->tags.count(); i++) desc_tag_values[desc][budget->tags[i]] = 0.0;
									}
								}
							} else if(!desc_map.contains(trans->description().toLower())) {
								QString desc = trans->description().toLower();
								desc_map[desc] = trans->description();
								desc_values[desc] = 0.0;
								desc_counts[desc] = 0.0;
								if(i_months > 0) desc_month_values[desc].fill(0.0, i_months);
								if(b_tags) {
									for(int i = 0; i < budget->tags.count(); i++) desc_tag_values[desc][budget->tags[i]] = 0.0;
								}
							}
						}
					}
					if(strans->transaction()->generaltype() != GENERAL_TRANSACTION_TYPE_SPLIT || split_i >= ((SplitTransaction*) strans->transaction())->count()) {
						++it;
						split_i = 0;
					}
				}
			}
		}
	}

	int month_index = 0;
	if(i_months <= 0) month_index = -1;

	bool first_date_reached = false;
	for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constBegin(); it != budget->transactions.constEnd(); ++it) {
		Transaction *trans = *it;
		if(!first_date_reached && trans->date() >= first_date) {
			first_date_reached = true;
			if(trans->date() > last_date) break;
			if(i_months > 0) {
				curmonth = (b_years ? budget->lastBudgetDayOfYear(first_date) : budget->lastBudgetDay(first_date));
				while(curmonth < trans->date()) {
					budget->addBudgetMonthsSetLast(curmonth, b_years ? 12 : 1);
					month_index++;
				}
			}
		} else if(first_date_reached && trans->date() > last_date) {
			break;
		} else if(first_date_reached && i_months > 0 && trans->date() > curmonth) {
			while(curmonth < trans->date()) {
				budget->addBudgetMonthsSetLast(curmonth, b_years ? 12 : 1);
				month_index++;
			}
		}
		if(first_date_reached && (!assets_selected || accountCombo->testTransactionRelation(trans))) {
			if((current_account && !include_subs) || !current_tag.isEmpty()) {
				int sign = 1;
				bool include = false;
				if(current_account) {
					if(trans->fromAccount() == current_account && (i_source <= 2 || (i_source == 4 && descriptionCombo->testTransaction(trans)) || (i_source == 3 && ((trans->type() == TRANSACTION_TYPE_EXPENSE && payeeCombo->testTransaction(trans)) || (trans->type() == TRANSACTION_TYPE_INCOME && payeeCombo->testTransaction(trans)))))) {
						include = true;
						if(type == ACCOUNT_TYPE_INCOMES) sign = 1;
						else sign = -1;
					} else if(trans->toAccount() == current_account && (i_source <= 2 || (i_source == 4 && descriptionCombo->testTransaction(trans)) || (i_source == 3 && ((trans->type() == TRANSACTION_TYPE_EXPENSE && payeeCombo->testTransaction(trans)) || (trans->type() == TRANSACTION_TYPE_INCOME && payeeCombo->testTransaction(trans)))))) {
						include = true;
						if(type == ACCOUNT_TYPE_EXPENSES) sign = 1;
						else sign = -1;
					}
				} else if(trans->hasTag(current_tag, true)) {
					if(i_source <= 2 || (i_source == 4 && descriptionCombo->testTransaction(trans)) || (i_source == 3 && ((trans->type() == TRANSACTION_TYPE_EXPENSE && payeeCombo->testTransaction(trans)) || (trans->type() == TRANSACTION_TYPE_INCOME && payeeCombo->testTransaction(trans))))) {
						include = true;
						if(trans->type() == TRANSACTION_TYPE_EXPENSE) {b_expense = true; sign = -1;}
						else if(trans->type() == TRANSACTION_TYPE_INCOME) {b_income = true; sign = 1;}
						else include = false;
					}
				}
				if(include) {
					double v = trans->value(true) * sign;
					if(i_source == 2 || i_source == 4) {
						if(trans->type() == TRANSACTION_TYPE_EXPENSE) {
							value += v;
							value_count += trans->quantity();
							QString desc = ((Expense*) trans)->payee().toLower();
							desc_values[desc] += v;
							if(month_index >= 0) desc_month_values[desc][month_index] += v;
							if(month_index >= 0) month_value[month_index] += v;
							desc_counts[desc] += trans->quantity();
							if(b_tags) {
								for(int i = 0; i < trans->tagsCount(true); i++) {
									desc_tag_values[desc][trans->getTag(i, true)] += v;
									tag_value[trans->getTag(i, true)] += v;
								}
							}
						} else if(trans->type() == TRANSACTION_TYPE_INCOME) {
							value += v;
							value_count += trans->quantity();
							QString desc = ((Income*) trans)->payer().toLower();
							desc_values[desc] += v;
							if(month_index >= 0) desc_month_values[desc][month_index] += v;
							if(month_index >= 0) month_value[month_index] += v;
							desc_counts[desc] += trans->quantity();
							if(b_tags) {
								for(int i = 0; i < trans->tagsCount(true); i++) {
									desc_tag_values[desc][trans->getTag(i, true)] += v;
									tag_value[trans->getTag(i, true)] += v;
								}
							}
						}
					} else {
						value += v;
						value_count += trans->quantity();
						QString desc = trans->description().toLower();
						desc_values[desc] += v;
						if(month_index >= 0) desc_month_values[desc][month_index] += v;
						if(month_index >= 0) month_value[month_index] += v;
						desc_counts[desc] += trans->quantity();
						if(b_tags) {
							for(int i = 0; i < trans->tagsCount(true); i++) {
								desc_tag_values[desc][trans->getTag(i, true)] += v;
								tag_value[trans->getTag(i, true)] += v;
							}
						}
					}
				}
			} else if(i_source == -1) {
				double v = trans->value(true);
				if(trans->type() == TRANSACTION_TYPE_EXPENSE) {
					value -= v;
					value_count += trans->quantity();
					QString desc = ((Expense*) trans)->payee().toLower();
					desc_values[desc] -= v;
					if(month_index >= 0) desc_month_values[desc][month_index] -= v;
					if(month_index >= 0) month_value[month_index] -= v;
					desc_counts[desc] += trans->quantity();
					if(b_tags) {
						for(int i = 0; i < trans->tagsCount(true); i++) {
							desc_tag_values[desc][trans->getTag(i, true)] -= v;
							tag_value[trans->getTag(i, true)] -= v;
						}
					}
				} else if(trans->type() == TRANSACTION_TYPE_INCOME) {
					value += v;
					value_count += trans->quantity();
					QString desc = ((Income*) trans)->payer().toLower();
					desc_values[desc] += v;
					if(month_index >= 0) desc_month_values[desc][month_index] += v;
					if(month_index >= 0) month_value[month_index] += v;
					desc_counts[desc] += trans->quantity();
					if(b_tags) {
						for(int i = 0; i < trans->tagsCount(true); i++) {
							desc_tag_values[desc][trans->getTag(i, true)] += v;
							tag_value[trans->getTag(i, true)] += v;
						}
					}
				}
			} else if(i_source == -2) {
				if(trans->tagsCount(true) > 0) {
					double v = trans->value(true);
					if(trans->type() == TRANSACTION_TYPE_EXPENSE) {
						b_expense = true;
						value -= v;
						value_count += trans->quantity();
						for(int i2 = 0; i2 < trans->tagsCount(true); i2++) {
							QString desc = trans->getTag(i2, true);
							desc_values[desc] -= v;
							if(month_index >= 0) desc_month_values[desc][month_index] -= v;
							if(month_index >= 0) month_value[month_index] -= v;
							desc_counts[desc] += trans->quantity();
							if(b_tags) {
								for(int i = 0; i < trans->tagsCount(true); i++) {
									desc_tag_values[desc][trans->getTag(i, true)] -= v;
								}
							}
						}
					} else if(trans->type() == TRANSACTION_TYPE_INCOME) {
						b_income = true;
						value += v;
						value_count += trans->quantity();
						for(int i2 = 0; i2 < trans->tagsCount(true); i2++) {
							QString desc = trans->getTag(i2, true);
							desc_values[desc] += v;
							if(month_index >= 0) desc_month_values[desc][month_index] += v;
							if(month_index >= 0) month_value[month_index] += v;
							desc_counts[desc] += trans->quantity();
							if(b_tags) {
								for(int i = 0; i < trans->tagsCount(true); i++) {
									desc_tag_values[desc][trans->getTag(i, true)] += v;
								}
							}
						}
					}
				}
			} else {
				double v = trans->value(true);
				Account *from_account = trans->fromAccount();
				if(!include_subs) from_account = from_account->topAccount();
				Account *to_account = trans->toAccount();
				if(!include_subs) to_account = to_account->topAccount();
				while(true) {
					bool b_top = (current_account || !include_subs) || (from_account == from_account->topAccount() && to_account == to_account->topAccount());
					if(!current_account || to_account->topAccount() == current_account || from_account->topAccount() == current_account) {
						if(from_account->type() == ACCOUNT_TYPE_EXPENSES) {
							values[from_account] -= v;
							if(month_index >= 0) month_values[from_account][month_index] -= v;
							if(b_top) costs -= v;
							if(b_top && month_index >= 0) month_costs[month_index] -= v;
							counts[from_account] += trans->quantity();
							if(b_top) costs_count += trans->quantity();
							if(b_tags) {
								for(int i = 0; i < trans->tagsCount(true); i++) {
									tag_values[from_account][trans->getTag(i, true)] -= v;
									if(b_top) tag_costs[trans->getTag(i, true)] -= v;
								}
							}
						} else if(from_account->type() == ACCOUNT_TYPE_INCOMES) {
							values[from_account] += v;
							if(month_index >= 0) month_values[from_account][month_index] += v;
							if(b_top) incomes += v;
							if(b_top && month_index >= 0) month_incomes[month_index] += v;
							counts[from_account] += trans->quantity();
							if(b_top) incomes_count += trans->quantity();
							if(b_tags) {
								for(int i = 0; i < trans->tagsCount(true); i++) {
									tag_values[from_account][trans->getTag(i, true)] += v;
									if(b_top) tag_incomes[trans->getTag(i, true)] += v;
								}
							}
						} else if(to_account->type() == ACCOUNT_TYPE_EXPENSES) {
							values[to_account] += v;
							if(month_index >= 0) month_values[to_account][month_index] += v;
							if(b_top) costs += v;
							if(b_top && month_index >= 0) month_costs[month_index] += v;
							counts[to_account] += trans->quantity();
							if(b_top) costs_count += trans->quantity();
							if(b_tags) {
								for(int i = 0; i < trans->tagsCount(true); i++) {
									tag_values[to_account][trans->getTag(i, true)] += v;
									if(b_top) tag_costs[trans->getTag(i, true)] += v;
								}
							}
						} else if(to_account->type() == ACCOUNT_TYPE_INCOMES) {
							values[to_account] -= v;
							if(month_index >= 0) month_values[to_account][month_index] -= v;
							if(b_top) incomes -= v;
							if(b_top && month_index >= 0) month_incomes[month_index] -= v;
							counts[to_account] += trans->quantity();
							if(b_top) incomes_count += trans->quantity();
							if(b_tags) {
								for(int i = 0; i < trans->tagsCount(true); i++) {
									tag_values[to_account][trans->getTag(i, true)] -= v;
									if(b_top) tag_incomes[trans->getTag(i, true)] -= v;
								}
							}
						}
					}
					if(b_top) break;
					from_account = from_account->topAccount();
					to_account = to_account->topAccount();
				}
			}
		}
	}
	first_date_reached = false;
	if(i_months > 0) month_index = 0;
	Transaction *trans = NULL;
	int split_i = 0;
	for(ScheduledTransactionList<ScheduledTransaction*>::const_iterator it = budget->scheduledTransactions.constBegin(); it != budget->scheduledTransactions.constEnd();) {
		ScheduledTransaction *strans = *it;
		while(split_i == 0 && strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT && ((SplitTransaction*) strans->transaction())->count() == 0) {
			++it;
			if(it == budget->scheduledTransactions.constEnd()) break;
			strans = *it;
		}
		if(strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT) {
			trans = ((SplitTransaction*) strans->transaction())->at(split_i);
			split_i++;
		} else {
			trans = (Transaction*) strans->transaction();
		}
		if(!first_date_reached && trans->date() >= first_date) {
			first_date_reached = true;
			if(trans->date() > last_date) break;
			if(i_months > 0) {
				curmonth = (b_years ? budget->lastBudgetDayOfYear(first_date) : budget->lastBudgetDay(first_date));
				while(curmonth < trans->date()) {
					budget->addBudgetMonthsSetLast(curmonth, b_years ? 12 : 1);
					month_index++;
				}
			}
		} else if(first_date_reached && trans->date() > last_date) {
			break;
		} else if(first_date_reached && i_months > 0 && trans->date() > curmonth) {
			while(curmonth < trans->date()) {
				budget->addBudgetMonthsSetLast(curmonth, b_years ? 12 : 1);
				month_index++;
			}
		}
		if(first_date_reached && (!assets_selected || accountCombo->testTransactionRelation(trans))) {
			QDate last_month_date, first_month_date;
			int month_index2 = month_index;
			if(i_months > 0) {
				first_month_date = (b_years ? budget->firstBudgetDayOfYear(curmonth) : budget->firstBudgetDay(curmonth));
				last_month_date = curmonth;
			} else {
				first_month_date = first_date;
				last_month_date = last_date;
			}
			do {
				if((current_account && !include_subs) || !current_tag.isEmpty()) {
					int sign = 1;
					bool include = false;
					if(current_account) {
						if(trans->fromAccount() == current_account && (i_source <= 2 || (i_source == 4 && descriptionCombo->testTransaction(trans)) || (i_source == 3 && ((trans->type() == TRANSACTION_TYPE_EXPENSE && payeeCombo->testTransaction(trans)) || (trans->type() == TRANSACTION_TYPE_INCOME && payeeCombo->testTransaction(trans)))))) {
							include = true;
							if(type == ACCOUNT_TYPE_INCOMES) sign = 1;
							else sign = -1;
						} else if(trans->toAccount() == current_account && (i_source <= 2 || (i_source == 4 && descriptionCombo->testTransaction(trans)) || (i_source == 3 && ((trans->type() == TRANSACTION_TYPE_EXPENSE && payeeCombo->testTransaction(trans)) || (trans->type() == TRANSACTION_TYPE_INCOME && payeeCombo->testTransaction(trans)))))) {
							include = true;
							if(type == ACCOUNT_TYPE_EXPENSES) sign = 1;
							else sign = -1;
						}
					} else if(trans->hasTag(current_tag, true)) {
						if(i_source <= 2 || (i_source == 4 && descriptionCombo->testTransaction(trans)) || (i_source == 3 && ((trans->type() == TRANSACTION_TYPE_EXPENSE && payeeCombo->testTransaction(trans)) || (trans->type() == TRANSACTION_TYPE_INCOME && payeeCombo->testTransaction(trans))))) {
							include = true;
							if(trans->type() == TRANSACTION_TYPE_EXPENSE) {b_expense = true; sign = -1;}
							else if(trans->type() == TRANSACTION_TYPE_INCOME) {b_income = true; sign = 1;}
							else include = false;
						}
					}
					if(include) {
						int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_month_date, last_month_date) : 1;
						double v = trans->value(true) * sign;
						if(i_source == 2 || i_source == 4) {
							if(trans->type() == TRANSACTION_TYPE_EXPENSE) {
								QString desc = ((Expense*) trans)->payee().toLower();
								desc_values[desc] += v * count;
								value += v * count;
								if(month_index >= 0) desc_month_values[desc][month_index2] += v * count;
								if(month_index >= 0) month_value[month_index2] += v * count;
								desc_counts[desc] += count * trans->quantity();
								value_count += count * trans->quantity();
								if(b_tags) {
									for(int i = 0; i < trans->tagsCount(true); i++) {
										desc_tag_values[desc][trans->getTag(i, true)] += v * count;
										tag_value[trans->getTag(i, true)] += v * count;
									}
								}
							} else if(trans->type() == TRANSACTION_TYPE_INCOME) {
								QString desc = ((Income*) trans)->payer().toLower();
								desc_values[desc] += v * count;
								value += v * count;
								if(month_index >= 0) desc_month_values[desc][month_index2] += v * count;
								if(month_index >= 0) month_value[month_index2] += v * count;
								desc_counts[desc] += count * trans->quantity();
								value_count += count * trans->quantity();
								if(b_tags) {
									for(int i = 0; i < trans->tagsCount(true); i++) {
										desc_tag_values[desc][trans->getTag(i, true)] += v * count;
										tag_value[trans->getTag(i, true)] += v * count;
									}
								}
							}
						} else {
							QString desc = trans->description().toLower();
							desc_values[desc] += v * count;
							value += v * count;
							if(month_index >= 0) desc_month_values[desc][month_index2] += v * count;
							if(month_index >= 0) month_value[month_index2] += v * count;
							desc_counts[desc] += count * trans->quantity();
							value_count += count * trans->quantity();
							if(b_tags) {
								for(int i = 0; i < trans->tagsCount(true); i++) {
									desc_tag_values[desc][trans->getTag(i, true)] += v * count;
									tag_value[trans->getTag(i, true)] += v * count;
								}
							}
						}
					}
				} else if(i_source == -1) {
					int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_month_date, last_month_date) : 1;
					double v = trans->value(true);
					if(trans->type() == TRANSACTION_TYPE_EXPENSE) {
						QString desc = ((Expense*) trans)->payee().toLower();
						desc_values[desc] -= v * count;
						value -= v * count;
						if(month_index >= 0) desc_month_values[desc][month_index2] -= v * count;
						if(month_index >= 0) month_value[month_index2] -= v * count;
						desc_counts[desc] += count * trans->quantity();
						value_count += count * trans->quantity();
						if(b_tags) {
							for(int i = 0; i < trans->tagsCount(true); i++) {
								desc_tag_values[desc][trans->getTag(i, true)] -= v * count;
								tag_value[trans->getTag(i, true)] -= v * count;
							}
						}
					} else if(trans->type() == TRANSACTION_TYPE_INCOME) {
						QString desc = ((Income*) trans)->payer().toLower();
						desc_values[desc] += v * count;
						value += v * count;
						if(month_index >= 0) desc_month_values[desc][month_index2] += v * count;
						if(month_index >= 0) month_value[month_index2] += v * count;
						desc_counts[desc] += count * trans->quantity();
						value_count += count * trans->quantity();
						if(b_tags) {
							for(int i = 0; i < trans->tagsCount(true); i++) {
								desc_tag_values[desc][trans->getTag(i, true)] += v * count;
								tag_value[trans->getTag(i, true)] += v * count;
							}
						}
					}
				}  else if(i_source == -2) {
					if(trans->tagsCount(true) > 0) {
						int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_month_date, last_month_date) : 1;
						double v = trans->value(true);
						if(trans->type() == TRANSACTION_TYPE_EXPENSE) {
							b_expense = true;
							value -= v;
							value_count += trans->quantity();
							for(int i2 = 0; i2 < trans->tagsCount(true); i2++) {
								QString desc = trans->getTag(i2, true);
								tag_value[desc] -= v * count;
								if(month_index >= 0) tag_month_values[desc][month_index] -= v * count;
								if(month_index >= 0) month_value[month_index] -= v * count;
								tag_counts[desc] += trans->quantity() * count;
								if(b_tags) {
									for(int i = 0; i < trans->tagsCount(true); i++) {
										desc_tag_values[desc][trans->getTag(i, true)] -= v * count;
									}
								}
							}
						} else if(trans->type() == TRANSACTION_TYPE_INCOME) {
							b_income = true;
							value += v;
							value_count += trans->quantity();
							for(int i2 = 0; i2 < trans->tagsCount(true); i2++) {
								QString desc = trans->getTag(i2, true);
								desc_values[desc] += v * count;
								if(month_index >= 0) desc_month_values[desc][month_index] += v * count;
								if(month_index >= 0) month_value[month_index] += v * count;
								desc_counts[desc] += trans->quantity() * count;
								if(b_tags) {
									for(int i = 0; i < trans->tagsCount(true); i++) {
										desc_tag_values[desc][trans->getTag(i, true)] += v * count;
									}
								}
							}
						}
					}
				} else {
					Account *from_account = trans->fromAccount();
					if(!include_subs) from_account = from_account->topAccount();
					Account *to_account = trans->toAccount();
					if(!include_subs) to_account = to_account->topAccount();
					double v = trans->value(true);
					while(true) {
						bool b_top = (current_account || !include_subs) || (from_account == from_account->topAccount() && to_account == to_account->topAccount());
						if(!current_account || to_account->topAccount() == current_account || from_account->topAccount() == current_account) {
							if(from_account->type() == ACCOUNT_TYPE_EXPENSES) {
								int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_month_date, last_month_date) : 1;
								counts[from_account] += count * trans->quantity();
								values[from_account] -= v * count;
								if(month_index2 >= 0) month_values[from_account][month_index2] -= v * count;
								if(b_top && month_index2 >= 0) month_costs[month_index2] -= v * count;
								if(b_top) costs_count += count * trans->quantity();
								if(b_top) costs -= v * count;
								if(b_tags) {
									for(int i = 0; i < trans->tagsCount(true); i++) {
										tag_values[from_account][trans->getTag(i, true)] -= v * count;
										if(b_top) tag_costs[trans->getTag(i, true)] -= v * count;
									}
								}
							} else if(from_account->type() == ACCOUNT_TYPE_INCOMES) {
								int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_month_date, last_month_date) : 1;
								counts[from_account] += count * trans->quantity();
								values[from_account] += v * count;
								if(month_index2 >= 0) month_values[from_account][month_index2] += v * count;
								if(b_top && month_index2 >= 0) month_incomes[month_index2] += v * count;
								if(b_top) incomes_count += count * trans->quantity();
								if(b_top) incomes += v * count;
								if(b_tags) {
									for(int i = 0; i < trans->tagsCount(true); i++) {
										tag_values[from_account][trans->getTag(i, true)] += v * count;
										if(b_top) tag_incomes[trans->getTag(i, true)] += v * count;
									}
								}
							} else if(to_account->type() == ACCOUNT_TYPE_EXPENSES) {
								int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_month_date, last_month_date) : 1;
								counts[to_account] += count * trans->quantity();
								values[to_account] += v * count;
								if(month_index2 >= 0) month_values[to_account][month_index2] += v * count;
								if(b_top && month_index2 >= 0) month_costs[month_index2] += v * count;
								if(b_top) costs_count += count * trans->quantity();
								if(b_top) costs += v * count;
								if(b_tags) {
									for(int i = 0; i < trans->tagsCount(true); i++) {
										tag_values[to_account][trans->getTag(i, true)] += v * count;
										if(b_top) tag_costs[trans->getTag(i, true)] += v * count;
									}
								}
							} else if(to_account->type() == ACCOUNT_TYPE_INCOMES) {
								int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_month_date, last_month_date) : 1;
								counts[to_account] += count * trans->quantity();
								values[to_account] -= v * count;
								if(month_index2 >= 0) month_values[to_account][month_index2] -= v * count;
								if(b_top && month_index2 >= 0) month_incomes[month_index2] -= v * count;
								if(b_top) incomes_count += count * trans->quantity();
								if(b_top) incomes -= v * count;
								if(b_tags) {
									for(int i = 0; i < trans->tagsCount(true); i++) {
										tag_values[to_account][trans->getTag(i, true)] -= v * count;
										if(b_top) tag_incomes[trans->getTag(i, true)] -= v * count;
									}
								}
							}
						}
						if(b_top) break;
						from_account = from_account->topAccount();
						to_account = to_account->topAccount();
					}
				}
				if(i_months > 0) {
					month_index2++;
					budget->addBudgetMonthsSetFirst(first_month_date, b_years ? 12 : 1);
					budget->addBudgetMonthsSetLast(last_month_date, b_years ? 12 : 1);
				}
			} while(i_months > 0 && month_index2 < i_months && strans->recurrence());
		}
		if(strans->transaction()->generaltype() != GENERAL_TRANSACTION_TYPE_SPLIT || split_i >= ((SplitTransaction*) strans->transaction())->count()) {
			++it;
			split_i = 0;
		}
	}
	if(current_account && include_subs) {
		if(type == ACCOUNT_TYPE_EXPENSES) {
			value = costs - incomes;
			value_count = costs_count + incomes_count;
		} else {
			value = incomes - costs;
			value_count = incomes_count + costs_count;
		}
	}

	bool b_negate = false;
	if(!b_income && b_expense) {type = ACCOUNT_TYPE_EXPENSES; b_negate = true;}
	else if(b_income && !b_expense) type = ACCOUNT_TYPE_INCOMES;
	else if(!current_tag.isEmpty() || i_source == -2) type = ACCOUNT_TYPE_ASSETS;

	source = "";
	QString title;
	int ptype = b_extra ? payeeCombo->itemType() : 0;
	if(assets_selected) {
		if((current_account || !current_tag.isEmpty()) && type == ACCOUNT_TYPE_EXPENSES) {
			if(b_extra) payeeCombo->setItemType(2);
			if(include_subs) title = tr("Expenses, %2: %1").arg(current_account ? current_account->name() : current_tag).arg(accountCombo->selectedAccountsText(2));
			else if(i_source == 4) title = tr("Expenses, %3: %2, %1").arg(current_account ? current_account->nameWithParent() : current_tag).arg(descriptionCombo->selectedItemsText(2)).arg(accountCombo->selectedAccountsText(2));
			else if(i_source == 3) title = tr("Expenses, %3: %2, %1").arg(current_account ? current_account->nameWithParent() : current_tag).arg(payeeCombo->selectedItemsText(2)).arg(accountCombo->selectedAccountsText(2));
			else title = tr("Expenses, %2: %1").arg(current_account ? current_account->nameWithParent() : current_tag).arg(accountCombo->selectedAccountsText(2));
		} else if((current_account || !current_tag.isEmpty()) && type == ACCOUNT_TYPE_INCOMES) {
			if(b_extra) payeeCombo->setItemType(3);
			if(include_subs) title = tr("Incomes, %2: %1").arg(current_account ? current_account->name() : current_tag).arg(accountCombo->selectedAccountsText(2));
			else if(i_source == 4) title = tr("Incomes, %3: %2, %1").arg(current_account ? current_account->nameWithParent() : current_tag).arg(descriptionCombo->selectedItemsText(2)).arg(accountCombo->selectedAccountsText(2));
			else if(i_source == 3) title = tr("Incomes, %3: %2, %1").arg(current_account ? current_account->nameWithParent() : current_tag).arg(payeeCombo->selectedItemsText(2)).arg(accountCombo->selectedAccountsText(2));
			else title = tr("Incomes, %2: %1").arg(current_account ? current_account->nameWithParent() : current_tag).arg(accountCombo->selectedAccountsText(2));
		} else if(!current_tag.isEmpty()) {
			if(b_extra) payeeCombo->setItemType((b_income && !b_expense) ? 3 : 2);
			if(i_source == 4) title = tr("%3: %2, %1").arg(current_tag).arg(descriptionCombo->selectedItemsText(2)).arg(accountCombo->selectedAccountsText(2));
			else if(i_source == 3) title = tr("%3: %2, %1").arg(current_tag).arg(payeeCombo->selectedItemsText(2)).arg(accountCombo->selectedAccountsText(2));
			else title = tr("%2: %1").arg(current_tag).arg(accountCombo->selectedAccountsText(2));
		} else if(i_source == -2) {
			if(type == ACCOUNT_TYPE_EXPENSES) title = tr("Expenses, %2: %1").arg(tr("Tags")).arg(accountCombo->selectedAccountsText(2));
			else if(type == ACCOUNT_TYPE_INCOMES) title = tr("Incomes, %2: %1").arg(tr("Tags")).arg(accountCombo->selectedAccountsText(2));
			else title = tr("Tags, %1").arg(accountCombo->selectedAccountsText(2));
		} else {
			title = tr("Incomes & Expenses, %1").arg(accountCombo->selectedAccountsText(2));
		}
	} else {
		if((current_account || !current_tag.isEmpty()) && type == ACCOUNT_TYPE_EXPENSES) {
			if(b_extra) payeeCombo->setItemType(2);
			if(include_subs) title = tr("Expenses: %1").arg(current_account ? current_account->name() : current_tag);
			else if(i_source == 4) title = tr("Expenses: %2, %1").arg(current_account ? current_account->nameWithParent() : current_tag).arg(descriptionCombo->selectedItemsText(2));
			else if(i_source == 3) title = tr("Expenses: %2, %1").arg(current_account ? current_account->nameWithParent() : current_tag).arg(payeeCombo->selectedItemsText(2));
			else title = tr("Expenses: %1").arg(current_account ? current_account->nameWithParent() : current_tag);
		} else if((current_account || !current_tag.isEmpty()) && type == ACCOUNT_TYPE_INCOMES) {
			if(b_extra) payeeCombo->setItemType(3);
			if(include_subs) title = tr("Incomes: %1").arg(current_account ? current_account->name() : current_tag);
			else if(i_source == 4) title = tr("Incomes: %2, %1").arg(current_account ? current_account->nameWithParent() : current_tag).arg(descriptionCombo->selectedItemsText(2));
			else if(i_source == 3) title = tr("Incomes: %2, %1").arg(current_account ? current_account->nameWithParent() : current_tag).arg(payeeCombo->selectedItemsText(2));
			else title = tr("Incomes: %1").arg(current_account ? current_account->nameWithParent() : current_tag);
		} else if(!current_tag.isEmpty()) {
			if(b_extra) payeeCombo->setItemType((b_income && !b_expense) ? 3 : 2);
			if(i_source == 4) title = tr("%2, %1").arg(current_tag).arg(descriptionCombo->selectedItemsText(2));
			else if(i_source == 3) title = tr("%2, %1").arg(current_tag).arg(payeeCombo->selectedItemsText(2));
			else title = current_tag;
		} else if(i_source == -2) {
			if(type == ACCOUNT_TYPE_EXPENSES) title = tr("Expenses: %1").arg(tr("Tags"));
			else if(type == ACCOUNT_TYPE_INCOMES) title = tr("Incomes: %1").arg(tr("Tags"));
			else title = tr("Tags");
		} else {
			title = tr("Incomes & Expenses");
		}
	}
	if(b_extra) payeeCombo->setItemType(ptype);

	QStringList tags;
	bool b_incomes = !b_tags, b_expenses = !b_tags;
	if(b_tags) {
		for(int i = 0; i < budget->tags.count(); i++) {
			if(!b_incomes && tag_incomes[budget->tags[i]] != 0.0) b_incomes = true;
			if(!b_expenses && tag_costs[budget->tags[i]] != 0.0) b_expenses = true;
			if(tag_value[budget->tags[i]] != 0.0 || tag_incomes[budget->tags[i]] != 0.0 || tag_costs[budget->tags[i]] != 0.0) {
				tags << budget->tags[i];
			}
		}
		if(tags.isEmpty()) tags = budget->tags;
	}
	if(!b_incomes && !b_expenses) {b_incomes = true; b_expenses = true;}

	QTextStream outf(&source, QIODevice::WriteOnly);
	outf << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">" << '\n';
	outf << "<html>" << '\n';
	outf << "\t<head>" << '\n';
	outf << "\t\t<title>";
	outf << htmlize_string(title);
	outf << "</title>" << '\n';
	outf << "\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" << '\n';
	outf << "\t\t<meta name=\"GENERATOR\" content=\"" << qApp->applicationDisplayName() << "\">" << '\n';
	outf << "\t</head>" << '\n';
	outf << "\t<body bgcolor=\"white\" style=\"color: black; margin-top: 10; margin-bottom: 10; margin-left: 10; margin-right: 10\">" << '\n';
	if(fromButton->isChecked()) outf << "\t\t<h2 align=\"center\">" << tr("%1 (%2&ndash;%3)", "html format; %1: title; %2: from date; %3: to date").arg(htmlize_string(title)).arg(htmlize_string(QLocale().toString(first_date, QLocale::ShortFormat))).arg(htmlize_string(QLocale().toString(last_date, QLocale::ShortFormat))) << "</h2>" << '\n' << "\t\t<br>";
	else outf << "\t\t<h2 align=\"center\">" << tr("%1 (to %2)", "html format; %1: title; %2: to date").arg(htmlize_string(title)).arg(htmlize_string(QLocale().toString(last_date, QLocale::ShortFormat))) << "</h2>" << '\n' << "\t\t<br>";
	outf << "\t\t<table width=\"100%\" border=\"0.5\" style=\"border-style: solid; border-color: #cccccc\" cellspacing=\"0\" cellpadding=\"5\">" << '\n';
	outf << "\t\t\t<thead align=\"left\">" << '\n';
	outf << "\t\t\t\t<tr bgcolor=\"#f0f0f0\">" << '\n';
#define FIRST_COL "\t\t\t\t\t<td align=\"left\">"
#define EVEN_COL "<td nowrap align=\"right\">"
#define ODD_COL "<td nowrap align=\"right\">"
#define FIRST_COL_TOP "\t\t\t\t\t<th align=\"left\">"
#define EVEN_COL_TOP "\t\t\t\t\t<th align=\"left\">"
#define ODD_COL_TOP "\t\t\t\t\t<th align=\"left\">"
#define FIRST_COL_DIV "\t\t\t\t\t<td align=\"left\"><b>"
#define EVEN_COL_DIV "<td nowrap align=\"right\"><b>"
#define ODD_COL_DIV "<td nowrap align=\"right\"><b>"
#define FIRST_COL_BOTTOM "\t\t\t\t\t<td align=\"left\"><b>"
#define EVEN_COL_BOTTOM "\t\t\t\t\t<td nowrap align=\"right\"><b>"
#define ODD_COL_BOTTOM "\t\t\t\t\t<td nowrap align=\"right\"><b>"
	int col = 1;
	if((current_account || !current_tag.isEmpty() || i_source == -2) && type == ACCOUNT_TYPE_EXPENSES) {
		if(i_source == -2) outf << FIRST_COL_TOP << htmlize_string(tr("Tag")) << "</th>";
		else if(include_subs) outf << FIRST_COL_TOP << htmlize_string(tr("Category")) << "</th>";
		else if(i_source == 2 || i_source == 4) outf << FIRST_COL_TOP << htmlize_string(tr("Payee")) << "</th>";
		else outf << FIRST_COL_TOP << htmlize_string(tr("Description", "Referring to the transaction description property (transaction title/generic article name)")) << "</th>";
		if(enabled[0]) {outf << (col % 2 == 1 ? ODD_COL_TOP : EVEN_COL_TOP) << htmlize_string(tr("Cost")) << "</th>"; col++;}
	} else if((current_account || !current_tag.isEmpty() || i_source == -2) && type == ACCOUNT_TYPE_INCOMES) {
		if(i_source == -2) outf << FIRST_COL_TOP << htmlize_string(tr("Tag")) << "</th>";
		else if(include_subs) outf << FIRST_COL_TOP << htmlize_string(tr("Category")) << "</th>";
		else if(i_source == 2 || i_source == 4) outf << FIRST_COL_TOP << htmlize_string(tr("Payer")) << "</th>";
		else outf << FIRST_COL_TOP << htmlize_string(tr("Description", "Referring to the transaction description property (transaction title/generic article name)")) << "</th>";
		if(enabled[0]) {outf << (col % 2 == 1 ? ODD_COL_TOP : EVEN_COL_TOP) << htmlize_string(tr("Income")) << "</th>"; col++;}
	} else if(!current_tag.isEmpty()) {
		if(i_source == 2 || i_source == 4) {
			if(b_expense && !b_income) outf << FIRST_COL_TOP << htmlize_string(tr("Payee")) << "</th>";
			else if(!b_expense && b_income) outf << FIRST_COL_TOP << htmlize_string(tr("Payer")) << "</th>";
			else outf << FIRST_COL_TOP << htmlize_string(tr("Payee/Payer")) << "</th>";
		} else outf << FIRST_COL_TOP << htmlize_string(tr("Description", "Referring to the transaction description property (transaction title/generic article name)")) << "</th>";
		if(enabled[0]) {outf << (col % 2 == 1 ? ODD_COL_TOP : EVEN_COL_TOP) << htmlize_string(tr("Value")) << "</th>"; col++;}
	} else {
		if(i_source == -1) outf << FIRST_COL_TOP << htmlize_string(tr("Payee/Payer")) << "</th>";
		else if(i_source == -2) outf << FIRST_COL_TOP << htmlize_string(tr("Tag")) << "</th>";
		else outf << FIRST_COL_TOP << htmlize_string(tr("Category")) << "</th>";
		if(enabled[0]) {outf << (col % 2 == 1 ? ODD_COL_TOP : EVEN_COL_TOP) << htmlize_string(tr("Value")) << "</th>"; col++;}
	}
	if(enabled[1]) {outf << (col % 2 == 1 ? ODD_COL_TOP : EVEN_COL_TOP) << htmlize_string(tr("Daily Average")) << "</th>"; col++;}
	if(enabled[2]) {outf << (col % 2 == 1 ? ODD_COL_TOP : EVEN_COL_TOP) << htmlize_string(tr("Monthly Average")) << "</th>"; col++;}
	if(enabled[3]) {outf << (col % 2 == 1 ? ODD_COL_TOP : EVEN_COL_TOP) << htmlize_string(tr("Yearly Average")) << "</th>"; col++;}
	if(enabled[4]) {outf << (col % 2 == 1 ? ODD_COL_TOP : EVEN_COL_TOP) << htmlize_string(tr("Quantity")) << "</th>"; col++;}
	if((current_account || !current_tag.isEmpty() || i_source == -2) && type == ACCOUNT_TYPE_EXPENSES) {
		if(enabled[5]) outf << (col % 2 == 1 ? ODD_COL_TOP : EVEN_COL_TOP) << htmlize_string(tr("Average Cost")) << "</th>";
	} else if((current_account || !current_tag.isEmpty() || i_source == -2) && type == ACCOUNT_TYPE_INCOMES) {
		if(enabled[5]) outf << (col % 2 == 1 ? ODD_COL_TOP : EVEN_COL_TOP) << htmlize_string(tr("Average Income")) << "</th>";
	} else {
		if(enabled[5]) outf << (col % 2 == 1 ? ODD_COL_TOP : EVEN_COL_TOP) << htmlize_string(tr("Average Value")) << "</th>";
	}
	if(i_months > 0) {
		curmonth = last_date;
		for(int i = i_months - 1; i >= 0; i--) {
			outf << (col % 2 == 1 ? ODD_COL_TOP : EVEN_COL_TOP) << htmlize_string(b_years ? budget->budgetYearString(curmonth, false) : ((i_months > 12 && curmonth.month() == 12) ? QLocale().toString(curmonth, "MMM yyyy") : QLocale().toString(curmonth, "MMM"))) << "</th>"; col++;
			budget->addBudgetMonthsSetFirst(curmonth, b_years ? -12 : -1);
		}
		outf << (col % 2 == 1 ? ODD_COL_TOP : EVEN_COL_TOP) << htmlize_string(tr("Total")) << "</th>"; col++;
	} else if(b_tags) {
		for(int i = 0; i < tags.count(); i++) {
			outf << (col % 2 == 1 ? ODD_COL_TOP : EVEN_COL_TOP) << htmlize_string(tags[i]) << "</th>"; col++;
		}
	}
	outf << "\t\t\t\t</tr>" << '\n';
	outf << "\t\t\t</thead>" << '\n';
	outf << "\t\t\t<tbody>" << '\n';
	int days = first_date.daysTo(last_date) + 1;
	double months = budget->monthsBetweenDates(first_date, last_date, true), years = budget->yearsBetweenDates(first_date, last_date, true);
	int i_count_frac = 0;
	double intpart = 0.0;

	if((current_account && !include_subs) || !current_tag.isEmpty() || i_source == -1 || i_source == -2) {
		QMap<QString, double>::iterator it_e = desc_counts.end();
		for(QMap<QString, double>::iterator it = desc_counts.begin(); it != it_e; ++it) {
			if(modf(it.value(), &intpart) != 0.0) {
				i_count_frac = 2;
				break;
			}
		}
	} else if(current_account) {
		if(modf(counts[current_account], &intpart) != 0.0) {
			i_count_frac = 2;
		} else {
			for(AccountList<CategoryAccount*>::const_iterator it = current_account->subCategories.constBegin(); it != current_account->subCategories.constEnd(); ++it) {
				CategoryAccount *account = *it;
				if(modf(counts[account], &intpart) != 0.0) {
					i_count_frac = 2;
					break;
				}
			}
		}
	} else {
		for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
			CategoryAccount *account = *it;
			if((include_subs || !account->parentCategory()) && modf(counts[account], &intpart) != 0.0) {
				i_count_frac = 2;
				break;
			}
		}
		if(i_count_frac == 0) {
			for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
				CategoryAccount *account = *it;
				if((include_subs || !account->parentCategory()) && modf(counts[account], &intpart) != 0.0) {
					i_count_frac = 2;
					break;
				}
			}
		}
	}
	if(current_account || i_source == -1 || i_source == -2 || !current_tag.isEmpty()) {
		if(include_subs) {
			CategoryAccount *account = NULL;
			for(AccountList<CategoryAccount*>::const_iterator it = current_account->subCategories.constBegin();;) {
				if(account == current_account) break;
				if(it != current_account->subCategories.constEnd()) {
					account = *it;
					++it;
				} else {
					account = current_account;
				}
				outf << "\t\t\t\t<tr>" << '\n';
				outf << FIRST_COL << htmlize_string(account->name()) << "</td>";
				col = 1;
				if(enabled[0]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(values[account])) << "</td>"; col++;}
				if(enabled[1]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(values[account] / days)) << "</td>"; col++;}
				if(enabled[2]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(values[account] / months)) << "</td>"; col++;}
				if(enabled[3]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(values[account] / years)) << "</td>"; col++;}
				if(enabled[4]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatValue(counts[account], i_count_frac)) << "</td>"; col++;}
				if(enabled[5]) outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(counts[account] == 0.0 ? 0.0 : (values[account] / counts[account]))) << "</td>";
				if(i_months > 0) {
					for(int i = i_months - 1; i >= 0; i--) {
						outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(month_values[account][i])) << "</td>"; col++;
					}
					outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(values[account])) << "</td>"; col++;
				} else if(b_tags) {
					for(int i = 0; i < tags.count(); i++) {
						outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(tag_values[account][tags[i]])) << "</td>"; col++;
					}
				}
				outf << "\n";
				outf << "\t\t\t\t</tr>" << '\n';
			}
		} else {
			QMap<QString, double>::iterator it_e = desc_values.end();
			QMap<QString, double>::iterator itc = desc_counts.begin();
			QMap<QString, double>::iterator it = desc_values.begin();
			QMap<QString, QVector<double> >::iterator mit = desc_month_values.begin();
			QMap<QString, QMap<QString, double> >::iterator tit = desc_tag_values.begin();
			while(it != it_e) {
				outf << "\t\t\t\t<tr>" << '\n';
				if(it.key().isEmpty()) {
					if((i_source == 4 || i_source == 2) && type == ACCOUNT_TYPE_EXPENSES) outf << FIRST_COL << htmlize_string(tr("No payee")) << "</td>";
					else if((i_source == 4 || i_source == 2) && type == ACCOUNT_TYPE_INCOMES) outf << FIRST_COL << htmlize_string(tr("No payer")) << "</td>";
					else if(i_source == -1 || i_source == 4 || i_source == 2) outf << FIRST_COL << htmlize_string(tr("No payee/payer")) << "</td>";
					else outf << FIRST_COL << htmlize_string(tr("No description", "Referring to the transaction description property (transaction title/generic article name)")) << "</td>";
				} else {
					outf << FIRST_COL << htmlize_string(desc_map[it.key()]) << "</td>";
				}
				col = 1;
				if(b_negate) it.value() = -it.value();
				if(enabled[0]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(it.value())) << "</td>"; col++;}
				if(enabled[1]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(it.value() / days)) << "</td>"; col++;}
				if(enabled[2]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(it.value() / months)) << "</td>"; col++;}
				if(enabled[3]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(it.value() / years)) << "</td>"; col++;}
				if(enabled[4]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatValue(itc.value(), i_count_frac)) << "</td>"; col++;}
				if(enabled[5]) outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(itc.value() == 0.0 ? 0.0 : (it.value() / itc.value()))) << "</td>";
				if(i_months > 0) {
					for(int i = i_months - 1; i >= 0; i--) {
						outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(b_negate ? -mit.value()[i] : mit.value()[i])) << "</td>"; col++;
					}
					outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(it.value())) << "</td>"; col++;
				} else if(b_tags) {
					for(int i = 0; i < tags.count(); i++) {
						outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(b_negate ? -tit.value()[tags[i]] : tit.value()[tags[i]])) << "</td>"; col++;
					}
				}
				outf << "\n";
				outf << "\t\t\t\t</tr>" << '\n';
				++it; ++itc;
				if(i_months > 0) ++mit;
				else if(b_tags) ++tit;
			}
		}
		if(!b_tags && i_source != -2) {
			outf << "\t\t\t\t<tr bgcolor=\"#f0f0f0\">" << '\n';
			outf << FIRST_COL_BOTTOM << htmlize_string(tr("Total")) << "</b></td>";
			col = 1;
			if(b_negate) value = -value;
			if(enabled[0]) {outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney(value)) << "</b></td>"; col++;}
			if(enabled[1]) {outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney(value / days)) << "</b></td>"; col++;}
			if(enabled[2]) {outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney(value / months)) << "</b></td>"; col++;}
			if(enabled[3]) {outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney(value / years)) << "</b></td>"; col++;}
			if(enabled[4]) {outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatValue(value_count, i_count_frac)) << "</b></td>"; col++;}
			if(enabled[5]) outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney(value_count == 0.0 ? 0.0 : (value / value_count))) << "</b></td>";
			if(i_months > 0) {
				for(int i = i_months - 1; i >= 0; i--) {
					outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney(b_negate ? -month_value[i] : month_value[i])) << "</b></td>"; col++;
				}
				outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney(value)) << "</b></td>"; col++;
			} else if(b_tags) {
				for(int i = 0; i < tags.count(); i++) {
					outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney(b_negate ? -tag_value[tags[i]] : tag_value[tags[i]])) << "</b></td>"; col++;
				}
			}
			outf << "\n";
			outf << "\t\t\t\t</tr>" << '\n';
		}
	} else {
		if(b_incomes) {
			for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
				CategoryAccount *account = *it;
				if(include_subs || !account->parentCategory()) {
					outf << "\t\t\t\t<tr>" << '\n';
					if(account->parentCategory()) {
						outf << FIRST_COL << "<i>" << htmlize_string(account->nameWithParent()) << "</i></td>";
						col = 1;
						if(enabled[0]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << "<i>" << htmlize_string(budget->formatMoney(values[account])) << "</i></td>"; col++;}
						if(enabled[1]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << "<i>" << htmlize_string(budget->formatMoney(values[account] / days)) << "</i></td>"; col++;}
						if(enabled[2]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << "<i>" << htmlize_string(budget->formatMoney(values[account] / months)) << "</i></td>"; col++;}
						if(enabled[3]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << "<i>" << htmlize_string(budget->formatMoney(values[account] / years)) << "</i></td>"; col++;}
						if(enabled[4]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << "<i>" << htmlize_string(budget->formatValue(counts[account], i_count_frac)) << "</i></td>"; col++;}
						if(enabled[5]) outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << "<i>" << htmlize_string(budget->formatMoney(counts[account] == 0.0 ? 0.0 : (values[account] / counts[account]))) << "</i></td>";
						if(i_months > 0) {
							for(int i = i_months - 1; i >= 0; i--) {
								outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << "<i>" << htmlize_string(budget->formatMoney(month_values[account][i])) << "</i></td>"; col++;
							}
							outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << "<i>" << htmlize_string(budget->formatMoney(values[account])) << "</i></td>"; col++;
						} else if(b_tags) {
							for(int i = 0; i < tags.count(); i++) {
								outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << "<i>" << htmlize_string(budget->formatMoney(tag_values[account][tags[i]])) << "</i></td>"; col++;
							}
						}
					} else {
						outf << FIRST_COL << htmlize_string(account->nameWithParent()) << "</td>";
						col = 1;
						if(enabled[0]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(values[account])) << "</td>"; col++;}
						if(enabled[1]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(values[account] / days)) << "</td>"; col++;}
						if(enabled[2]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(values[account] / months)) << "</td>"; col++;}
						if(enabled[3]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(values[account] / years)) << "</td>"; col++;}
						if(enabled[4]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatValue(counts[account], i_count_frac)) << "</td>"; col++;}
						if(enabled[5]) outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(counts[account] == 0.0 ? 0.0 : (values[account] / counts[account]))) << "</td>";
						if(i_months > 0) {
							for(int i = i_months - 1; i >= 0; i--) {
								outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(month_values[account][i])) << "</td>"; col++;
							}
							outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(values[account])) << "</td>"; col++;
						} else if(b_tags) {
							for(int i = 0; i < tags.count(); i++) {
								outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(tag_values[account][tags[i]])) << "</td>"; col++;
							}
						}
					}
					outf << "\n";
					outf << "\t\t\t\t</tr>" << '\n';

				}
			}
			outf << "\t\t\t\t<tr bgcolor=\"#f0f0f0\">" << '\n';
			outf << FIRST_COL_DIV << htmlize_string(b_expenses ? tr("Total incomes") : tr("Total")) << "</b></td>";
			col = 1;
			if(enabled[0]) {outf << (col % 2 == 1 ? ODD_COL_DIV : EVEN_COL_DIV) << htmlize_string(budget->formatMoney(incomes)) << "</b></td>"; col++;}
			if(enabled[1]) {outf << (col % 2 == 1 ? ODD_COL_DIV : EVEN_COL_DIV) << htmlize_string(budget->formatMoney(incomes / days)) << "</b></td>"; col++;}
			if(enabled[2]) {outf << (col % 2 == 1 ? ODD_COL_DIV : EVEN_COL_DIV) << htmlize_string(budget->formatMoney(incomes / months)) << "</b></td>"; col++;}
			if(enabled[3]) {outf << (col % 2 == 1 ? ODD_COL_DIV : EVEN_COL_DIV) << htmlize_string(budget->formatMoney(incomes / years)) << "</b></td>"; col++;}
			if(enabled[4]) {outf << (col % 2 == 1 ? ODD_COL_DIV : EVEN_COL_DIV) << htmlize_string(budget->formatValue(incomes_count, i_count_frac)) << "</b></td>"; col++;}
			if(enabled[5]) outf << (col % 2 == 1 ? ODD_COL_DIV : EVEN_COL_DIV) << htmlize_string(budget->formatMoney(incomes_count == 0.0 ? 0.0 : (incomes / incomes_count))) << "</b></td>";
			if(i_months > 0) {
				for(int i = i_months - 1; i >= 0; i--) {
					outf << (col % 2 == 1 ? ODD_COL_DIV : EVEN_COL_DIV) << htmlize_string(budget->formatMoney(month_incomes[i])) << "</b></td>"; col++;
				}
				outf << (col % 2 == 1 ? ODD_COL_DIV : EVEN_COL_DIV) << htmlize_string(budget->formatMoney(incomes)) << "</b></td>"; col++;
			} else if(b_tags) {
				for(int i = 0; i < tags.count(); i++) {
					outf << (col % 2 == 1 ? ODD_COL_DIV : EVEN_COL_DIV) << htmlize_string(budget->formatMoney(tag_incomes[tags[i]])) << "</b></td>"; col++;
				}
			}
			outf << "\n";
			outf << "\t\t\t\t</tr>" << '\n';
		}
		if(b_expenses) {
			for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
				CategoryAccount *account = *it;
				if(include_subs || !account->parentCategory()) {
					outf << "\t\t\t\t<tr>" << '\n';
					if(account->parentCategory()) {
						outf << FIRST_COL << "<i>" << htmlize_string(account->nameWithParent()) << "</i></td>";
						col = 1;
						if(enabled[0]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << "<i>" << htmlize_string(budget->formatMoney(-values[account])) << "</i></td>"; col++;}
						if(enabled[1]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << "<i>" << htmlize_string(budget->formatMoney(-values[account] / days)) << "</i></td>"; col++;}
						if(enabled[2]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << "<i>" << htmlize_string(budget->formatMoney(-values[account] / months)) << "</i></td>"; col++;}
						if(enabled[3]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << "<i>" << htmlize_string(budget->formatMoney(-values[account] / years)) << "</i></td>"; col++;}
						if(enabled[4]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << "<i>" << htmlize_string(budget->formatValue(counts[account], i_count_frac)) << "</i></td>"; col++;}
						if(enabled[5]) outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << "<i>" << htmlize_string(budget->formatMoney(counts[account] == 0.0 ? 0.0 : (-values[account] / counts[account]))) << "</i></td>";
						if(i_months > 0) {
							for(int i = i_months - 1; i >= 0; i--) {
								outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << "<i>" << htmlize_string(budget->formatMoney(-month_values[account][i])) << "</i></td>"; col++;
							}
							outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << "<i>" << htmlize_string(budget->formatMoney(-values[account])) << "</i></td>"; col++;
						} else if(b_tags) {
							for(int i = 0; i < tags.count(); i++) {
								outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << "<i>" << htmlize_string(budget->formatMoney(-tag_values[account][tags[i]])) << "</i></td>"; col++;
							}
						}
					} else {
						outf << FIRST_COL << htmlize_string(account->nameWithParent()) << "</td>";
						col = 1;
						if(enabled[0]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(-values[account])) << "</td>"; col++;}
						if(enabled[1]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(-values[account] / days)) << "</td>"; col++;}
						if(enabled[2]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(-values[account] / months)) << "</td>"; col++;}
						if(enabled[3]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(-values[account] / years)) << "</td>"; col++;}
						if(enabled[4]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatValue(counts[account], i_count_frac)) << "</td>"; col++;}
						if(enabled[5]) outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(counts[account] == 0.0 ? 0.0 : (-values[account] / counts[account]))) << "</td>";
						if(i_months > 0) {
							for(int i = i_months - 1; i >= 0; i--) {
								outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(-month_values[account][i])) << "</td>"; col++;
							}
							outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(-values[account])) << "</td>"; col++;
						} else if(b_tags) {
							for(int i = 0; i < tags.count(); i++) {
								outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(-tag_values[account][tags[i]])) << "</td>"; col++;
							}
						}
					}
					outf << "\n";
					outf << "\t\t\t\t</tr>" << '\n';
				}
			}
			outf << "\t\t\t\t<tr bgcolor=\"#f0f0f0\">" << '\n';
			outf << FIRST_COL_BOTTOM << htmlize_string(b_incomes ? tr("Total expenses") : tr("Total")) << "</b></td>";
			col = 1;
			if(enabled[0]) {outf << (col % 2 == 1 ? ODD_COL_DIV : EVEN_COL_DIV) << htmlize_string(budget->formatMoney(-costs)) << "</b></td>"; col++;}
			if(enabled[1]) {outf << (col % 2 == 1 ? ODD_COL_DIV : EVEN_COL_DIV) << htmlize_string(budget->formatMoney(-costs / days)) << "</b></td>"; col++;}
			if(enabled[2]) {outf << (col % 2 == 1 ? ODD_COL_DIV : EVEN_COL_DIV) << htmlize_string(budget->formatMoney(-costs / months)) << "</b></td>"; col++;}
			if(enabled[3]) {outf << (col % 2 == 1 ? ODD_COL_DIV : EVEN_COL_DIV) << htmlize_string(budget->formatMoney(-costs / years)) << "</b></td>"; col++;}
			if(enabled[4]) {outf << (col % 2 == 1 ? ODD_COL_DIV : EVEN_COL_DIV) << htmlize_string(budget->formatValue(costs_count, i_count_frac)) << "</b></td>"; col++;}
			if(enabled[5]) outf << (col % 2 == 1 ? ODD_COL_DIV : EVEN_COL_DIV) << htmlize_string(budget->formatMoney(costs_count == 0.0 ? 0.0 : (-costs / costs_count))) << "</b></td>";
			if(i_months > 0) {
				for(int i = i_months - 1; i >= 0; i--) {
					outf << (col % 2 == 1 ? ODD_COL_DIV : EVEN_COL_DIV) << htmlize_string(budget->formatMoney(-month_costs[i])) << "</b></td>"; col++;
				}
				outf << (col % 2 == 1 ? ODD_COL_DIV : EVEN_COL_DIV) << htmlize_string(budget->formatMoney(-costs)) << "</b></td>"; col++;
			} else if(b_tags) {
				for(int i = 0; i < tags.count(); i++) {
					outf << (col % 2 == 1 ? ODD_COL_DIV : EVEN_COL_DIV) << htmlize_string(budget->formatMoney(-tag_costs[tags[i]])) << "</b></td>"; col++;
				}
			}
			outf << "\n";
			outf << "\t\t\t\t</tr>" << '\n';
		}
		if(b_incomes && b_expenses) {
			outf << "\t\t\t\t<tr bgcolor=\"#f0f0f0\">" << '\n';
			outf << FIRST_COL_BOTTOM << htmlize_string(tr("Total (Profits)")) << "</b></td>";
			col = 1;
			if(enabled[0]) {outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney(incomes - costs)) << "</b></td>"; col++;}
			if(enabled[1]) {outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney((incomes - costs) / days)) << "</b></td>"; col++;}
			if(enabled[2]) {outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney((incomes - costs) / months)) << "</b></td>"; col++;}
			if(enabled[3]) {outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney((incomes - costs) / years)) << "</b></td>"; col++;}
			if(enabled[4]) {outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatValue(incomes_count + costs_count, i_count_frac)) << "</b></td>"; col++;}
			if(enabled[5]) outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney((incomes_count + costs_count) == 0.0 ? 0.0 : ((incomes - costs) / (incomes_count + costs_count)))) << "</b></td>";
			if(i_months > 0) {
				for(int i = i_months - 1; i >= 0; i--) {
					outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney(month_incomes[i] - month_costs[i])) << "</b></td>"; col++;
				}
				outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney(incomes - costs)) << "</b></td>"; col++;
			} else if(b_tags) {
				for(int i = 0; i < tags.count(); i++) {
					outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney(tag_incomes[tags[i]] - tag_costs[tags[i]])) << "</b></td>"; col++;
				}
			}
			outf << "\n";
			outf << "\t\t\t\t</tr>" << '\n';
		}
	}
	outf << "\t\t\t</tbody>" << '\n';
	outf << "\t\t</table>" << '\n';
	outf << "\t</body>" << '\n';
	outf << "</html>" << '\n';
	htmlview->setLineWrapMode(QTextEdit::NoWrap);
	htmlview->setHtml(source);
	if(htmlview->document()->size().width() < htmlview->width()) htmlview->setLineWrapMode(QTextEdit::WidgetWidth);
}

void CategoriesComparisonReport::updateTransactions() {
	if(b_extra && (current_account || !current_tag.isEmpty())) {
		payeeCombo->blockSignals(true);
		descriptionCombo->blockSignals(true);
		QMap<QString, QString> descriptions, payees;
		bool b_income, b_expense;
		for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constEnd(); it != budget->transactions.constBegin();) {
			--it;
			Transaction *trans = *it;
			if((!current_account && trans->hasTag(current_tag, true) && ((trans->type() == TRANSACTION_TYPE_EXPENSE || trans->type() == TRANSACTION_TYPE_INCOME))) || (current_account && (trans->fromAccount() == current_account || trans->toAccount() == current_account))) {
				if(!descriptions.contains(trans->description().toLower())) descriptions[trans->description().toLower()] = trans->description();
				if(trans->type() == TRANSACTION_TYPE_EXPENSE) {
					b_expense = true;
					if(!payees.contains(((Expense*) trans)->payee().toLower())) payees[((Expense*) trans)->payee().toLower()] = ((Expense*) trans)->payee();
				} else if(trans->type() == TRANSACTION_TYPE_INCOME) {
					b_income = true;
					if(!payees.contains(((Income*) trans)->payer().toLower())) payees[((Income*) trans)->payer().toLower()] = ((Income*) trans)->payer();
				}
			}
		}
		if((!current_account && b_expense && !b_income) || (current_account && current_account->type() == ACCOUNT_TYPE_EXPENSES)) payeeCombo->setItemType(2);
		else if(current_account || (!b_expense && b_income)) payeeCombo->setItemType(3);
		else payeeCombo->setItemType(4);
		QStringList dlist;
		QMap<QString, QString>::iterator it_e = descriptions.end();
		for(QMap<QString, QString>::iterator it = descriptions.begin(); it != it_e; ++it) {
			dlist << it.value();
		}
		descriptionCombo->updateItems(dlist);
		QStringList plist;
		QMap<QString, QString>::iterator it2_e = payees.end();
		for(QMap<QString, QString>::iterator it2 = payees.begin(); it2 != it2_e; ++it2) {
			plist << it2.value();
		}
		payeeCombo->updateItems(plist);
		payeeCombo->blockSignals(false);
		descriptionCombo->blockSignals(false);
	}
	updateDisplay();
}
void CategoriesComparisonReport::updateTags() {
	updateAccounts();
}
void CategoriesComparisonReport::updateAccounts() {
	int curindex = 0;
	sourceCombo->blockSignals(true);
	accountCombo->blockSignals(true);
	sourceCombo->clear();
	sourceCombo->addItem(tr("All Categories, excluding subcategories"));
	sourceCombo->addItem(tr("All Categories, including subcategories"));
	sourceCombo->addItem(tr("All Tags"));
	if(b_extra) {
		sourceCombo->addItem(tr("All Payees and Payers"));
		first_source_account_index = 4;
	} else {
		first_source_account_index = 3;
	}
	for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
		Account *account = *it;
		sourceCombo->addItem(tr("Expenses: %1").arg(account->nameWithParent()));
		if(account == current_account) curindex = sourceCombo->count() - 1;
	}
	for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
		Account *account = *it;
		sourceCombo->addItem(tr("Incomes: %1").arg(account->nameWithParent()));
		if(account == current_account) curindex = sourceCombo->count() - 1;
	}
	for(int i2 = 0; i2 < budget->tags.count(); i2++) {
		sourceCombo->addItem(tr("Tag: %1").arg(budget->tags[i2]));
		if(!current_account && current_tag == budget->tags[i2]) curindex = sourceCombo->count() - 1;
	}
	if(curindex < sourceCombo->count()) sourceCombo->setCurrentIndex(curindex);
	accountCombo->updateAccounts(ACCOUNT_TYPE_ASSETS);
	sourceCombo->blockSignals(false);
	accountCombo->blockSignals(false);
	if(curindex == 0 && b_extra) {
		sourceChanged(curindex);
	} else {
		updateDisplay();
	}
}

