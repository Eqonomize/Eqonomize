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

#include "categoriescomparisonreport.h"
#include "eqonomize.h"

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
#include <QDebug>

#include "account.h"
#include "budget.h"
#include "recurrence.h"
#include "transaction.h"

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
	sourceCombo->addItem(tr("All Payees/Payers"));
	for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
		Account *account = *it;
		sourceCombo->addItem(tr("Expenses: %1").arg(account->nameWithParent()));
	}
	for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
		Account *account = *it;
		sourceCombo->addItem(tr("Incomes: %1").arg(account->nameWithParent()));
	}	
	sourceLayout->addWidget(sourceCombo);
	sourceLayout->setStretchFactor(sourceCombo, 1);

	payeeDescriptionWidget = NULL;
	if(b_extra) {
		sourceLayout->addStretch(1);
		payeeDescriptionWidget = new QWidget(settingsWidget);
		QHBoxLayout *payeeLayout = new QHBoxLayout(payeeDescriptionWidget);
		//payeeLayout->addWidget(new QLabel(tr("Divide on:"), payeeDescriptionWidget));
		QButtonGroup *group = new QButtonGroup(this);
		subsButton = new QRadioButton(tr("Subcategories"), payeeDescriptionWidget);
		subsButton->setChecked(true);
		group->addButton(subsButton);
		payeeLayout->addWidget(subsButton);
		descriptionButton = new QRadioButton(tr("Descriptions for", "Referring to the transaction description property (transaction title/generic article name)"), payeeDescriptionWidget);
		group->addButton(descriptionButton);
		payeeLayout->addWidget(descriptionButton);
		payeeCombo = new QComboBox(payeeDescriptionWidget);
		payeeCombo->setEditable(false);
		payeeLayout->addWidget(payeeCombo);
		payeeLayout->setStretchFactor(payeeCombo, 1);
		payeeButton = new QRadioButton(tr("Payees/payers for"), payeeDescriptionWidget);
		group->addButton(payeeButton);
		payeeLayout->addWidget(payeeButton);
		descriptionCombo = new QComboBox(payeeDescriptionWidget);
		descriptionCombo->setEditable(false);
		payeeLayout->addWidget(descriptionCombo);
		payeeLayout->setStretchFactor(descriptionCombo, 1);
		settingsLayout->addWidget(payeeDescriptionWidget, 1, 1);
		payeeDescriptionWidget->setEnabled(false);
	} else {
		QButtonGroup *group = new QButtonGroup(this);
		subsButton = new QRadioButton(tr("Subcategories"), settingsWidget);
		subsButton->setChecked(true);
		group->addButton(subsButton);
		sourceLayout->addWidget(subsButton);
		descriptionButton = new QRadioButton(tr("Descriptions", "Referring to the transaction description property (transaction title/generic article name)"), settingsWidget);
		group->addButton(descriptionButton);
		sourceLayout->addWidget(descriptionButton);
	}	

	current_account = NULL;
	has_empty_description = false;
	has_empty_payee = false;

	settingsLayout->addWidget(new QLabel(tr("Period:"), settingsWidget), b_extra ? 2 : 1, 0);
	QHBoxLayout *choicesLayout = new QHBoxLayout();
	settingsLayout->addLayout(choicesLayout, b_extra ? 2 : 1, 1);
	fromButton = new QCheckBox(tr("From"), settingsWidget);
	fromButton->setChecked(true);
	choicesLayout->addWidget(fromButton);
	fromEdit = new QDateEdit(settingsWidget);
	fromEdit->setCalendarPopup(true);
	choicesLayout->addWidget(fromEdit);
	choicesLayout->setStretchFactor(fromEdit, 1);
	choicesLayout->addWidget(new QLabel(tr("To"), settingsWidget));
	toEdit = new QDateEdit(settingsWidget);
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
	
	settings.endGroup();

	layout->addWidget(settingsWidget);
	
	resetOptions();

	connect(subsButton, SIGNAL(toggled(bool)), this, SLOT(subsToggled(bool)));
	connect(descriptionButton, SIGNAL(toggled(bool)), this, SLOT(descriptionToggled(bool)));
	if(b_extra) {
		connect(payeeButton, SIGNAL(toggled(bool)), this, SLOT(payeeToggled(bool)));
		connect(payeeCombo, SIGNAL(activated(int)), this, SLOT(payeeChanged(int)));
		connect(descriptionCombo, SIGNAL(activated(int)), this, SLOT(descriptionChanged(int)));
	}
	connect(sourceCombo, SIGNAL(activated(int)), this, SLOT(sourceChanged(int)));
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
	sourceCombo->setCurrentIndex(0);
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
void CategoriesComparisonReport::payeeChanged(int) {
	payeeButton->blockSignals(true);
	descriptionButton->blockSignals(true);
	descriptionButton->setChecked(true);
	payeeButton->setChecked(false);
	payeeButton->blockSignals(false);
	descriptionButton->blockSignals(false);
	updateDisplay();
}
void CategoriesComparisonReport::descriptionChanged(int) {
	payeeButton->blockSignals(true);
	descriptionButton->blockSignals(true);
	payeeButton->setChecked(true);
	descriptionButton->setChecked(false);
	payeeButton->blockSignals(false);
	descriptionButton->blockSignals(false);
	updateDisplay();
}
void CategoriesComparisonReport::sourceChanged(int i) {
	i -= 2;
	if(i > 0) {
		if(i - 1 < (int) budget->expensesAccounts.count()) current_account = budget->expensesAccounts.at(i - 1);
		else current_account = budget->incomesAccounts.at(i - 1 - budget->expensesAccounts.count());
		i++;
	}
	if(current_account && current_account->subCategories.isEmpty()) {
		subsButton->setEnabled(false);
		if(subsButton->isChecked()) {
			descriptionButton->blockSignals(true);
			descriptionButton->setChecked(true);
			descriptionButton->blockSignals(false);
		}
	} else {
		subsButton->setEnabled(true);
	}
	if(b_extra) {
		payeeCombo->blockSignals(true);
		descriptionCombo->blockSignals(true);
		payeeCombo->clear();
		descriptionCombo->clear();
		if(i <= 0) {
			current_account = NULL;
			has_empty_description = false;
			has_empty_payee = false;
			payeeDescriptionWidget->setEnabled(false);
		} else {
			payeeDescriptionWidget->setEnabled(true);
			if(current_account) {
				descriptionCombo->addItem(tr("All descriptions", "Referring to the transaction description property (transaction title/generic article name)"));
				if(current_account->type() == ACCOUNT_TYPE_EXPENSES) payeeCombo->addItem(tr("All payees"));
				else payeeCombo->addItem(tr("All payers"));
				has_empty_description = false;
				has_empty_payee = false;
				QMap<QString, QString> descriptions, payees;
				for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constEnd(); it != budget->transactions.constBegin();) {
					--it;
					Transaction *trans = *it;
					if((trans->fromAccount() == current_account || trans->toAccount() == current_account)) {
						if(trans->description().isEmpty()) has_empty_description = true;
						else if(!descriptions.contains(trans->description().toLower())) descriptions[trans->description().toLower()] = trans->description();
						if(trans->type() == TRANSACTION_TYPE_EXPENSE) {
							if(((Expense*) trans)->payee().isEmpty()) has_empty_payee = true;
							else if(!payees.contains(((Expense*) trans)->payee().toLower())) payees[((Expense*) trans)->payee().toLower()] = ((Expense*) trans)->payee();
						} else if(trans->type() == TRANSACTION_TYPE_INCOME) {
							if(((Income*) trans)->payer().isEmpty()) has_empty_payee = true;
							else if(!payees.contains(((Income*) trans)->payer().toLower())) payees[((Income*) trans)->payer().toLower()] = ((Income*) trans)->payer();
						}
					}
				}
				QMap<QString, QString>::iterator it_e = descriptions.end();
				for(QMap<QString, QString>::iterator it = descriptions.begin(); it != it_e; ++it) {
				}
				if(has_empty_description) descriptionCombo->addItem(tr("No description", "Referring to the transaction description property (transaction title/generic article name)"));
				QMap<QString, QString>::iterator it2_e = payees.end();
				for(QMap<QString, QString>::iterator it2 = payees.begin(); it2 != it2_e; ++it2) {
					payeeCombo->addItem(it2.value());
				}
				if(has_empty_payee) {
					if(current_account->type() == ACCOUNT_TYPE_EXPENSES) payeeCombo->addItem(tr("No payee"));
					else payeeCombo->addItem(tr("No payer"));
				}
			} else {
				payeeDescriptionWidget->setEnabled(false);
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
		if(fromButton->isChecked()) {
			QMessageBox::critical(this, tr("Error"), tr("To date is before from date."));
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
		QMessageBox::critical(this, tr("Error"), tr("From date is after to date."));
		to_date = date;
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

	if(!isVisible()) return;

	int columns = 2;
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

	QMap<Account*, double> values;
	QMap<Account*, double> counts;
	QMap<QString, QString> desc_map;
	QMap<QString, double> desc_values;
	QMap<QString, double> desc_counts;
	double incomes = 0.0, costs = 0.0;
	double incomes_count = 0.0, costs_count = 0.0;
	double value_count = 0.0;
	double value = 0.0;

	current_account = NULL;
	current_description = "";
	current_payee = "";
	
	bool include_subs = false;
		
	int i_source = sourceCombo->currentIndex();
	if(i_source == 1) {
		i_source = 0;
		include_subs = true;
	} else if(i_source == 2) {
		i_source = -1;
	} else if(i_source > 2) {
		i_source -= 2;
		if(i_source - 1 < (int) budget->expensesAccounts.count()) current_account = budget->expensesAccounts.at(i_source - 1);
		else current_account = budget->incomesAccounts.at(i_source - 1 - budget->expensesAccounts.count());
		if(!current_account) return;
		if(b_extra) {
			if(has_empty_description) descriptionCombo->setItemText(descriptionCombo->count() - 1, "");
			if(has_empty_payee) payeeCombo->setItemText(payeeCombo->count() - 1, "");
			if(subsButton->isChecked()) {
				i_source = 1;
				include_subs = !current_account->subCategories.isEmpty();
			} else if(descriptionButton->isChecked()) {
				int p_index = payeeCombo->currentIndex();
				if(p_index == 0)  {
					i_source = 1;
				} else {
					current_payee = payeeCombo->itemText(p_index);
					i_source = 3;
				}
			} else {
				int d_index = descriptionCombo->currentIndex();
				if(d_index == 0)  {
					i_source = 2;
				} else {
					current_description = descriptionCombo->itemText(d_index);
					i_source = 4;
				}
			}
		} else {
			i_source = 1;
			include_subs = subsButton->isChecked() && !current_account->subCategories.isEmpty();
		}
		
	}
	
	QDate first_date;
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
	
	AccountType type = ACCOUNT_TYPE_EXPENSES;
	if(current_account) type = current_account->type();
	switch(i_source) {
		case -1: {
			for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constEnd(); it != budget->transactions.constBegin();) {
				--it;
				Transaction *trans = *it;
				if(trans->date() >= first_date) {
					if(trans->date() > to_date) break;
					if(trans->type() == TRANSACTION_TYPE_EXPENSE && !desc_map.contains(((Expense*) trans)->payee().toLower())) {
						desc_map[((Expense*) trans)->payee().toLower()] = ((Expense*) trans)->payee();
						desc_values[((Expense*) trans)->payee().toLower()] = 0.0;
						desc_counts[((Expense*) trans)->payee().toLower()] = 0.0;
					} else if(trans->type() == TRANSACTION_TYPE_INCOME && !desc_map.contains(((Income*) trans)->payer().toLower())) {
						desc_map[((Income*) trans)->payer().toLower()] = ((Income*) trans)->payer();
						desc_values[((Income*) trans)->payer().toLower()] = 0.0;
						desc_counts[((Income*) trans)->payer().toLower()] = 0.0;
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
				if(trans->date() >= first_date) {
					if(trans->date() > to_date) break;
					if(trans->type() == TRANSACTION_TYPE_EXPENSE && !desc_map.contains(((Expense*) trans)->payee().toLower())) {
						desc_map[((Expense*) trans)->payee().toLower()] = ((Expense*) trans)->payee();
						desc_values[((Expense*) trans)->payee().toLower()] = 0.0;
						desc_counts[((Expense*) trans)->payee().toLower()] = 0.0;
					} else if(trans->type() == TRANSACTION_TYPE_INCOME && !desc_map.contains(((Income*) trans)->payer().toLower())) {
						desc_map[((Income*) trans)->payer().toLower()] = ((Income*) trans)->payer();
						desc_values[((Income*) trans)->payer().toLower()] = 0.0;
						desc_counts[((Income*) trans)->payer().toLower()] = 0.0;
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
				}
			}
			for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
				CategoryAccount *account = *it;
				if(include_subs || !account->parentCategory()) {
					values[account] = 0.0;
					counts[account] = 0.0;
				}
			}
			break;
		}
		default: {
			if(include_subs) {
				values[current_account] = 0.0;
				counts[current_account] = 0.0;
				for(AccountList<CategoryAccount*>::const_iterator it = current_account->subCategories.constBegin(); it != current_account->subCategories.constEnd(); ++it) {
					CategoryAccount *account = *it;
					values[account] = 0.0;
					counts[account] = 0.0;
				}
			} else if(current_account) {
				for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constEnd(); it != budget->transactions.constBegin();) {
					--it;
					Transaction *trans = *it;
					if((trans->fromAccount() == current_account || trans->toAccount() == current_account) && (i_source <= 2 || (i_source == 4 && !trans->description().compare(current_description, Qt::CaseInsensitive)) || (i_source == 3 && ((trans->type() == TRANSACTION_TYPE_EXPENSE && !((Expense*) trans)->payee().compare(current_payee, Qt::CaseInsensitive)) || (trans->type() == TRANSACTION_TYPE_INCOME && !((Income*) trans)->payer().compare(current_payee, Qt::CaseInsensitive)))))) {
						if(i_source == 2 || i_source == 4) {
							if(trans->type() == TRANSACTION_TYPE_EXPENSE && !desc_map.contains(((Expense*) trans)->payee().toLower())) {
								desc_map[((Expense*) trans)->payee().toLower()] = ((Expense*) trans)->payee();
								desc_values[((Expense*) trans)->payee().toLower()] = 0.0;
								desc_counts[((Expense*) trans)->payee().toLower()] = 0.0;
							} else if(trans->type() == TRANSACTION_TYPE_INCOME && !desc_map.contains(((Income*) trans)->payer().toLower())) {
								desc_map[((Income*) trans)->payer().toLower()] = ((Income*) trans)->payer();
								desc_values[((Income*) trans)->payer().toLower()] = 0.0;
								desc_counts[((Income*) trans)->payer().toLower()] = 0.0;
							}
						} else if(!desc_map.contains(trans->description().toLower())) {
							desc_map[trans->description().toLower()] = trans->description();
							desc_values[trans->description().toLower()] = 0.0;
							desc_counts[trans->description().toLower()] = 0.0;
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
					if((trans->fromAccount() == current_account || trans->toAccount() == current_account) && (i_source <= 2 || (i_source == 4 && !trans->description().compare(current_description, Qt::CaseInsensitive)) || (i_source == 3 && ((trans->type() == TRANSACTION_TYPE_EXPENSE && !((Expense*) trans)->payee().compare(current_payee, Qt::CaseInsensitive)) || (trans->type() == TRANSACTION_TYPE_INCOME && !((Income*) trans)->payer().compare(current_payee, Qt::CaseInsensitive)))))) {
						if(i_source == 2 || i_source == 4) {
							if(trans->type() == TRANSACTION_TYPE_EXPENSE && !desc_map.contains(((Expense*) trans)->payee().toLower())) {
								desc_map[((Expense*) trans)->payee().toLower()] = ((Expense*) trans)->payee();
								desc_values[((Expense*) trans)->payee().toLower()] = 0.0;
								desc_counts[((Expense*) trans)->payee().toLower()] = 0.0;
							} else if(trans->type() == TRANSACTION_TYPE_INCOME && !desc_map.contains(((Income*) trans)->payer().toLower())) {
								desc_map[((Income*) trans)->payer().toLower()] = ((Income*) trans)->payer();
								desc_values[((Income*) trans)->payer().toLower()] = 0.0;
								desc_counts[((Income*) trans)->payer().toLower()] = 0.0;
							}
						} else if(!desc_map.contains(trans->description().toLower())) {
							desc_map[trans->description().toLower()] = trans->description();
							desc_values[trans->description().toLower()] = 0.0;
							desc_counts[trans->description().toLower()] = 0.0;
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

	bool first_date_reached = false;
	for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constBegin(); it != budget->transactions.constEnd(); ++it) {
		Transaction *trans = *it;
		if(!first_date_reached && trans->date() >= first_date) first_date_reached = true;
		else if(first_date_reached && trans->date() > to_date) break;
		if(first_date_reached) {
			if(current_account && !include_subs) {
				int sign = 1;
				bool include = false;
				if(trans->fromAccount() == current_account && (i_source <= 2 || (i_source == 4 && !trans->description().compare(current_description, Qt::CaseInsensitive)) || (i_source == 3 && ((trans->type() == TRANSACTION_TYPE_EXPENSE && !((Expense*) trans)->payee().compare(current_payee, Qt::CaseInsensitive)) || (trans->type() == TRANSACTION_TYPE_INCOME && !((Income*) trans)->payer().compare(current_payee, Qt::CaseInsensitive)))))) {
					include = true;
					if(type == ACCOUNT_TYPE_INCOMES) sign = 1;
					else sign = -1;
				} else if(trans->toAccount() == current_account && (i_source <= 2 || (i_source == 4 && !trans->description().compare(current_description, Qt::CaseInsensitive)) || (i_source == 3 && ((trans->type() == TRANSACTION_TYPE_EXPENSE && !((Expense*) trans)->payee().compare(current_payee, Qt::CaseInsensitive)) || (trans->type() == TRANSACTION_TYPE_INCOME && !((Income*) trans)->payer().compare(current_payee, Qt::CaseInsensitive)))))) {
					include = true;
					if(type == ACCOUNT_TYPE_EXPENSES) sign = 1;
					else sign = -1;
				}
				if(include) {
					if(i_source == 2 || i_source == 4) {
						if(trans->type() == TRANSACTION_TYPE_EXPENSE) {
							desc_values[((Expense*) trans)->payee().toLower()] += trans->value(true) * sign;
							value += trans->value(true) * sign;
							desc_counts[((Expense*) trans)->payee().toLower()] += trans->quantity();
							value_count += trans->quantity();
						} else if(trans->type() == TRANSACTION_TYPE_INCOME) {
							desc_values[((Income*) trans)->payer().toLower()] += trans->value(true) * sign;
							value += trans->value(true) * sign;
							desc_counts[((Income*) trans)->payer().toLower()] += trans->quantity();
							value_count += trans->quantity();
						}
					} else {
						desc_values[trans->description().toLower()] += trans->value(true) * sign;
						value += trans->value(true) * sign;
						desc_counts[trans->description().toLower()] += trans->quantity();
						value_count += trans->quantity();
					}
				}
			} else if(i_source == -1) {
				if(trans->type() == TRANSACTION_TYPE_EXPENSE) {
					desc_values[((Expense*) trans)->payee().toLower()] -= trans->value(true);
					value -= trans->value(true);
					desc_counts[((Expense*) trans)->payee().toLower()] += trans->quantity();
					value_count += trans->quantity();
				} else if(trans->type() == TRANSACTION_TYPE_INCOME) {
					desc_values[((Income*) trans)->payer().toLower()] += trans->value(true);
					value += trans->value(true);
					desc_counts[((Income*) trans)->payer().toLower()] += trans->quantity();
					value_count += trans->quantity();
				}
			} else {
				Account *from_account = trans->fromAccount();
				if(!include_subs) from_account = from_account->topAccount();
				Account *to_account = trans->toAccount();
				if(!include_subs) to_account = to_account->topAccount();
				while(true) {
					bool b_top = (current_account || !include_subs) || (from_account == from_account->topAccount() && to_account == to_account->topAccount());
					if(!current_account || to_account->topAccount() == current_account || from_account->topAccount() == current_account) {
						if(from_account->type() == ACCOUNT_TYPE_EXPENSES) {
							values[from_account] -= trans->value(true);
							if(b_top) costs -= trans->value(true);
							counts[from_account] += trans->quantity();
							if(b_top) costs_count += trans->quantity();
						} else if(from_account->type() == ACCOUNT_TYPE_INCOMES) {
							values[from_account] += trans->value(true);
							if(b_top) incomes += trans->value(true);
							counts[from_account] += trans->quantity();
							if(b_top) incomes_count += trans->quantity();
						} else if(to_account->type() == ACCOUNT_TYPE_EXPENSES) {
							values[to_account] += trans->value(true);
							if(b_top) costs += trans->value(true);
							counts[to_account] += trans->quantity();
							if(b_top) costs_count += trans->quantity();
						} else if(to_account->type() == ACCOUNT_TYPE_INCOMES) {
							values[to_account] -= trans->value(true);
							if(b_top) incomes -= trans->value(true);
							counts[to_account] += trans->quantity();
							if(b_top) incomes_count += trans->quantity();
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
		if(!first_date_reached && trans->date() >= first_date) first_date_reached = true;
		else if(first_date_reached && trans->date() > to_date) break;
		if(first_date_reached) {
			if(current_account && !include_subs) {
				int sign = 1;
				bool include = false;
				if(trans->fromAccount() == current_account && (i_source <= 2 || (i_source == 4 && !trans->description().compare(current_description, Qt::CaseInsensitive)) || (i_source == 3 && ((trans->type() == TRANSACTION_TYPE_EXPENSE && !((Expense*) trans)->payee().compare(current_payee, Qt::CaseInsensitive)) || (trans->type() == TRANSACTION_TYPE_INCOME && !((Income*) trans)->payer().compare(current_payee, Qt::CaseInsensitive)))))) {
					include = true;
					if(type == ACCOUNT_TYPE_INCOMES) sign = 1;
					else sign = -1;
				} else if(trans->toAccount() == current_account && (i_source <= 2 || (i_source == 4 && !trans->description().compare(current_description, Qt::CaseInsensitive)) || (i_source == 3 && ((trans->type() == TRANSACTION_TYPE_EXPENSE && ((Expense*) trans)->payee().compare(current_payee, Qt::CaseInsensitive)) || (trans->type() == TRANSACTION_TYPE_INCOME && !((Income*) trans)->payer().compare(current_payee, Qt::CaseInsensitive)))))) {
					include = true;
					if(type == ACCOUNT_TYPE_EXPENSES) sign = 1;
					else sign = -1;
				}
				if(include) {
					int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_date, to_date) : 1;
					if(i_source == 2 || i_source == 4) {
						if(trans->type() == TRANSACTION_TYPE_EXPENSE) {
							desc_values[((Expense*) trans)->payee().toLower()] += trans->value(true) * count * sign;
							value += trans->value(true) * count * sign;
							desc_counts[((Expense*) trans)->payee().toLower()] += count * trans->quantity();
							value_count += count * trans->quantity();
						} else if(trans->type() == TRANSACTION_TYPE_INCOME) {
							desc_values[((Income*) trans)->payer().toLower()] += trans->value(true) * count * sign;
							value += trans->value(true) * count * sign;
							desc_counts[((Income*) trans)->payer().toLower()] += count * trans->quantity();
							value_count += count * trans->quantity();
						}
					} else {
						desc_values[trans->description().toLower()] += trans->value(true) * count * sign;
						value += trans->value(true) * count * sign;
						desc_counts[trans->description().toLower()] += count * trans->quantity();
						value_count += count * trans->quantity();
					}
				}
			} else if(i_source == -1) {
				int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_date, to_date) : 1;
				if(trans->type() == TRANSACTION_TYPE_EXPENSE) {
					desc_values[((Expense*) trans)->payee().toLower()] -= trans->value(true) * count;
					value -= trans->value(true) * count;
					desc_counts[((Expense*) trans)->payee().toLower()] += count * trans->quantity();
					value_count += count * trans->quantity();
				} else if(trans->type() == TRANSACTION_TYPE_INCOME) {
					desc_values[((Income*) trans)->payer().toLower()] += trans->value(true) * count;
					value += trans->value(true) * count;
					desc_counts[((Income*) trans)->payer().toLower()] += count * trans->quantity();
					value_count += count * trans->quantity();
				}
			} else {
				Account *from_account = trans->fromAccount();
				if(!include_subs) from_account = from_account->topAccount();
				Account *to_account = trans->toAccount();
				if(!include_subs) to_account = to_account->topAccount();
				while(true) {
					bool b_top = (current_account || !include_subs) || (from_account == from_account->topAccount() && to_account == to_account->topAccount());
					if(!current_account || to_account->topAccount() == current_account || from_account->topAccount() == current_account) {
						if(from_account->type() == ACCOUNT_TYPE_EXPENSES) {
							int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_date, to_date) : 1;
							counts[from_account] += count * trans->quantity();
							values[from_account] -= trans->value(true) * count;
							if(b_top) costs_count += count * trans->quantity();
							if(b_top) costs -= trans->value(true) * count;
						} else if(from_account->type() == ACCOUNT_TYPE_INCOMES) {
							int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_date, to_date) : 1;
							counts[from_account] += count * trans->quantity();
							values[from_account] += trans->value(true) * count;
							if(b_top) incomes_count += count * trans->quantity();
							if(b_top) incomes += trans->value(true) * count;
						} else if(to_account->type() == ACCOUNT_TYPE_EXPENSES) {
							int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_date, to_date) : 1;
							counts[to_account] += count * trans->quantity();
							values[to_account] += trans->value(true) * count;
							if(b_top) costs_count += count * trans->quantity();
							if(b_top) costs += trans->value(true) * count;
						} else if(to_account->type() == ACCOUNT_TYPE_INCOMES) {
							int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_date, to_date) : 1;
							counts[to_account] += count * trans->quantity();
							values[to_account] -= trans->value(true) * count;
							if(b_top) incomes_count += count * trans->quantity();
							if(b_top) incomes -= trans->value(true) * count;
						}
					}
					if(b_top) break;
					from_account = from_account->topAccount();
					to_account = to_account->topAccount();
				}
			}
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
	source = "";
	QString title;
	if(current_account && type == ACCOUNT_TYPE_EXPENSES) {
		if(include_subs) title = tr("Expenses: %1").arg(current_account->name());
		else if(i_source == 4) title = tr("Expenses: %2, %1").arg(current_account->nameWithParent()).arg(current_description.isEmpty() ? tr("No description", "Referring to the transaction description property (transaction title/generic article name)") : current_description);
		else if(i_source == 3) title = tr("Expenses: %2, %1").arg(current_account->nameWithParent()).arg(current_payee.isEmpty() ? tr("No payee") : current_payee);
		else title = tr("Expenses: %1").arg(current_account->nameWithParent());
	} else if(current_account && type == ACCOUNT_TYPE_INCOMES) {
		if(include_subs) title = tr("Incomes: %1").arg(current_account->name());
		else if(i_source == 4) title = tr("Incomes: %2, %1").arg(current_account->nameWithParent()).arg(current_description.isEmpty() ? tr("No description", "Referring to the transaction description property (transaction title/generic article name)") : current_description);
		else if(i_source == 3) title = tr("Incomes: %2, %1").arg(current_account->nameWithParent()).arg(current_payee.isEmpty() ? tr("No payer") : current_payee);
		else title = tr("Incomes: %1").arg(current_account->nameWithParent());
	} else {
		title = tr("Incomes & Expenses");
	}
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
	if(fromButton->isChecked()) outf << "\t\t<h2 align=\"center\">" << tr("%1 (%2&ndash;%3)", "html format; %1: title; %2: from date; %3: to date").arg(htmlize_string(title)).arg(htmlize_string(QLocale().toString(first_date, QLocale::ShortFormat))).arg(htmlize_string(QLocale().toString(to_date, QLocale::ShortFormat))) << "</h2>" << '\n' << "\t\t<br>";
	else outf << "\t\t<h2 align=\"center\">" << tr("%1 (to %2)", "html format; %1: title; %2: to date").arg(htmlize_string(title)).arg(htmlize_string(QLocale().toString(to_date, QLocale::ShortFormat))) << "</h2>" << '\n' << "\t\t<br>";
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
	if(current_account && type == ACCOUNT_TYPE_EXPENSES) {
		if(include_subs) outf << FIRST_COL_TOP << htmlize_string(tr("Category")) << "</th>";
		else if(i_source == 2 || i_source == 4) outf << FIRST_COL_TOP << htmlize_string(tr("Payee")) << "</th>";
		else outf << FIRST_COL_TOP << htmlize_string(tr("Description", "Referring to the transaction description property (transaction title/generic article name)")) << "</th>";
		if(enabled[0]) {outf << (col % 2 == 1 ? ODD_COL_TOP : EVEN_COL_TOP) << htmlize_string(tr("Cost")) << "</th>"; col++;}
	} else if(current_account && type == ACCOUNT_TYPE_INCOMES) {
		if(include_subs) outf << FIRST_COL_TOP << htmlize_string(tr("Category")) << "</th>";
		else if(i_source == 2 || i_source == 4) outf << FIRST_COL_TOP << htmlize_string(tr("Payer")) << "</th>";
		else outf << FIRST_COL_TOP << htmlize_string(tr("Description", "Referring to the transaction description property (transaction title/generic article name)")) << "</th>";
		if(enabled[0]) {outf << (col % 2 == 1 ? ODD_COL_TOP : EVEN_COL_TOP) << htmlize_string(tr("Income")) << "</th>"; col++;}
	} else {
		if(i_source == -1) outf << FIRST_COL_TOP << htmlize_string(tr("Payee/Payer")) << "</th>";
		else outf << FIRST_COL_TOP << htmlize_string(tr("Category")) << "</th>";
		if(enabled[0]) {outf << (col % 2 == 1 ? ODD_COL_TOP : EVEN_COL_TOP) << htmlize_string(tr("Value")) << "</th>"; col++;}
	}
	if(enabled[1]) {outf << (col % 2 == 1 ? ODD_COL_TOP : EVEN_COL_TOP) << htmlize_string(tr("Daily Average")) << "</th>"; col++;}
	if(enabled[2]) {outf << (col % 2 == 1 ? ODD_COL_TOP : EVEN_COL_TOP) << htmlize_string(tr("Monthly Average")) << "</th>"; col++;}
	if(enabled[3]) {outf << (col % 2 == 1 ? ODD_COL_TOP : EVEN_COL_TOP) << htmlize_string(tr("Yearly Average")) << "</th>"; col++;}
	if(enabled[4]) {outf << (col % 2 == 1 ? ODD_COL_TOP : EVEN_COL_TOP) << htmlize_string(tr("Quantity")) << "</th>"; col++;}
	if(current_account && type == ACCOUNT_TYPE_EXPENSES) {
		if(enabled[5]) outf << (col % 2 == 1 ? ODD_COL_TOP : EVEN_COL_TOP) << htmlize_string(tr("Average Cost")) << "</th>";
	} else if(current_account && type == ACCOUNT_TYPE_INCOMES) {
		if(enabled[5]) outf << (col % 2 == 1 ? ODD_COL_TOP : EVEN_COL_TOP) << htmlize_string(tr("Average Income")) << "</th>";
	} else {
		if(enabled[5]) outf << (col % 2 == 1 ? ODD_COL_TOP : EVEN_COL_TOP) << htmlize_string(tr("Average Value")) << "</th>";
	}
	outf << "\t\t\t\t</tr>" << '\n';
	outf << "\t\t\t</thead>" << '\n';
	outf << "\t\t\t<tbody>" << '\n';
	int days = first_date.daysTo(to_date) + 1;
	double months = budget->monthsBetweenDates(first_date, to_date, true), years = budget->yearsBetweenDates(first_date, to_date, true);
	int i_count_frac = 0;
	double intpart = 0.0;
	if(current_account && !include_subs) {
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
	if(current_account || i_source == -1) {
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
				if(enabled[4]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(QLocale().toString(counts[account], 'f', i_count_frac)) << "</td>"; col++;}
				if(enabled[5]) outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(counts[account] == 0.0 ? 0.0 : (values[account] / counts[account]))) << "</td>";
				outf << "\n";
				outf << "\t\t\t\t</tr>" << '\n';
			}
		} else {
			QMap<QString, double>::iterator it_e = desc_values.end();
			QMap<QString, double>::iterator itc = desc_counts.begin();
			QMap<QString, double>::iterator it = desc_values.begin();
			for(; it != it_e; ++it, ++itc) {
				outf << "\t\t\t\t<tr>" << '\n';
				if(it.key().isEmpty()) {
					if((i_source == 4 || i_source == 2) && type == ACCOUNT_TYPE_EXPENSES) outf << FIRST_COL << htmlize_string(tr("No payee")) << "</td>";
					else if(i_source == 4 || i_source == 2) outf << FIRST_COL << htmlize_string(tr("No payer")) << "</td>";
					else if(i_source == -1) outf << FIRST_COL << htmlize_string(tr("No payee/payer")) << "</td>";
					else outf << FIRST_COL << htmlize_string(tr("No description", "Referring to the transaction description property (transaction title/generic article name)")) << "</td>";
				} else {
					outf << FIRST_COL << htmlize_string(desc_map[it.key()]) << "</td>";
				}
				col = 1;
				if(enabled[0]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(it.value())) << "</td>"; col++;}
				if(enabled[1]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(it.value() / days)) << "</td>"; col++;}
				if(enabled[2]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(it.value() / months)) << "</td>"; col++;}
				if(enabled[3]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(it.value() / years)) << "</td>"; col++;}
				if(enabled[4]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(QLocale().toString(itc.value(), 'f', i_count_frac)) << "</td>"; col++;}
				if(enabled[5]) outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(itc.value() == 0.0 ? 0.0 : (it.value() / itc.value()))) << "</td>";
				outf << "\n";
				outf << "\t\t\t\t</tr>" << '\n';
			}
		}
		outf << "\t\t\t\t<tr bgcolor=\"#f0f0f0\">" << '\n';
		outf << FIRST_COL_BOTTOM << htmlize_string(tr("Total")) << "</b></td>";
		col = 1;
		if(enabled[0]) {outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney(value)) << "</b></td>"; col++;}
		if(enabled[1]) {outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney(value / days)) << "</b></td>"; col++;}
		if(enabled[2]) {outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney(value / months)) << "</b></td>"; col++;}
		if(enabled[3]) {outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney(value / years)) << "</b></td>"; col++;}
		if(enabled[4]) {outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(QLocale().toString(value_count, 'f', i_count_frac)) << "</b></td>"; col++;}
		if(enabled[5]) outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney(value_count == 0.0 ? 0.0 : (value / value_count))) << "</b></td>";
		outf << "\n";
		outf << "\t\t\t\t</tr>" << '\n';
	} else {
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
					if(enabled[4]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << "<i>" << htmlize_string(QLocale().toString(counts[account], 'f', i_count_frac)) << "</i></td>"; col++;}
					if(enabled[5]) outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << "<i>" << htmlize_string(budget->formatMoney(counts[account] == 0.0 ? 0.0 : (values[account] / counts[account]))) << "</i></td>";
				} else {
					outf << FIRST_COL << htmlize_string(account->nameWithParent()) << "</td>";
					col = 1;
					if(enabled[0]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(values[account])) << "</td>"; col++;}
					if(enabled[1]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(values[account] / days)) << "</td>"; col++;}
					if(enabled[2]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(values[account] / months)) << "</td>"; col++;}
					if(enabled[3]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(values[account] / years)) << "</td>"; col++;}
					if(enabled[4]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(QLocale().toString(counts[account], 'f', i_count_frac)) << "</td>"; col++;}
					if(enabled[5]) outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(counts[account] == 0.0 ? 0.0 : (values[account] / counts[account]))) << "</td>";
				}
				outf << "\n";
				outf << "\t\t\t\t</tr>" << '\n';
					
			}
		}
		outf << "\t\t\t\t<tr bgcolor=\"#f0f0f0\">" << '\n';
		outf << FIRST_COL_DIV << htmlize_string(tr("Total incomes")) << "</b></td>";
		col = 1;
		if(enabled[0]) {outf << (col % 2 == 1 ? ODD_COL_DIV : EVEN_COL_DIV) << htmlize_string(budget->formatMoney(incomes)) << "</b></td>"; col++;}
		if(enabled[1]) {outf << (col % 2 == 1 ? ODD_COL_DIV : EVEN_COL_DIV) << htmlize_string(budget->formatMoney(incomes / days)) << "</b></td>"; col++;}
		if(enabled[2]) {outf << (col % 2 == 1 ? ODD_COL_DIV : EVEN_COL_DIV) << htmlize_string(budget->formatMoney(incomes / months)) << "</b></td>"; col++;}
		if(enabled[3]) {outf << (col % 2 == 1 ? ODD_COL_DIV : EVEN_COL_DIV) << htmlize_string(budget->formatMoney(incomes / years)) << "</b></td>"; col++;}
		if(enabled[4]) {outf << (col % 2 == 1 ? ODD_COL_DIV : EVEN_COL_DIV) << htmlize_string(QLocale().toString(incomes_count, 'f', i_count_frac)) << "</b></td>"; col++;}
		if(enabled[5]) outf << (col % 2 == 1 ? ODD_COL_DIV : EVEN_COL_DIV) << htmlize_string(budget->formatMoney(incomes_count == 0.0 ? 0.0 : (incomes / incomes_count))) << "</b></td>";
		outf << "\n";
		outf << "\t\t\t\t</tr>" << '\n';
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
					if(enabled[4]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << "<i>" << htmlize_string(QLocale().toString(counts[account], 'f', i_count_frac)) << "</i></td>"; col++;}
					if(enabled[5]) outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << "<i>" << htmlize_string(budget->formatMoney(counts[account] == 0.0 ? 0.0 : (-values[account] / counts[account]))) << "</i></td>";
				} else {
					outf << FIRST_COL << htmlize_string(account->nameWithParent()) << "</td>";
					col = 1;
					if(enabled[0]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(-values[account])) << "</td>"; col++;}
					if(enabled[1]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(-values[account] / days)) << "</td>"; col++;}
					if(enabled[2]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(-values[account] / months)) << "</td>"; col++;}
					if(enabled[3]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(-values[account] / years)) << "</td>"; col++;}
					if(enabled[4]) {outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(QLocale().toString(counts[account], 'f', i_count_frac)) << "</td>"; col++;}
					if(enabled[5]) outf << (col % 2 == 1 ? ODD_COL : EVEN_COL) << htmlize_string(budget->formatMoney(counts[account] == 0.0 ? 0.0 : (-values[account] / counts[account]))) << "</td>";
				}
				outf << "\n";
				outf << "\t\t\t\t</tr>" << '\n';
			}
		}
		outf << "\t\t\t\t<tr bgcolor=\"#f0f0f0\">" << '\n';
		outf << FIRST_COL_BOTTOM << htmlize_string(tr("Total expenses")) << "</b></td>";
		col = 1;
		if(enabled[0]) {outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney(-costs)) << "</b></td>"; col++;}
		if(enabled[1]) {outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney(-costs / days)) << "</b></td>"; col++;}
		if(enabled[2]) {outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney(-costs / months)) << "</b></td>"; col++;}
		if(enabled[3]) {outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney(-costs / years)) << "</b></td>"; col++;}
		if(enabled[4]) {outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(QLocale().toString(costs_count, 'f', i_count_frac)) << "</b></td>"; col++;}
		if(enabled[5]) outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney(costs_count == 0.0 ? 0.0 : (-costs / costs_count))) << "</b></td>";
		outf << "\n";
		outf << "\t\t\t\t</tr>" << '\n';
		outf << "\t\t\t\t<tr bgcolor=\"#f0f0f0\">" << '\n';
		outf << FIRST_COL_BOTTOM << htmlize_string(tr("Total (Profits)")) << "</b></td>";
		col = 1;
		if(enabled[0]) {outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney(incomes - costs)) << "</b></td>"; col++;}
		if(enabled[1]) {outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney((incomes - costs) / days)) << "</b></td>"; col++;}
		if(enabled[2]) {outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney((incomes - costs) / months)) << "</b></td>"; col++;}
		if(enabled[3]) {outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney((incomes - costs) / years)) << "</b></td>"; col++;}
		if(enabled[4]) {outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(QLocale().toString(incomes_count + costs_count, 'f', i_count_frac)) << "</b></td>"; col++;}
		if(enabled[5]) outf << (col % 2 == 1 ? ODD_COL_BOTTOM : EVEN_COL_BOTTOM) << htmlize_string(budget->formatMoney((incomes_count + costs_count) == 0.0 ? 0.0 : ((incomes - costs) / (incomes_count + costs_count)))) << "</b></td>";
		outf << "\n";
		outf << "\t\t\t\t</tr>" << '\n';
	}
	outf << "\t\t\t</tbody>" << '\n';
	outf << "\t\t</table>" << '\n';
	outf << "\t</body>" << '\n';
	outf << "</html>" << '\n';
	htmlview->setHtml(source);
	if(current_account && b_extra) {
		if(has_empty_description) descriptionCombo->setItemText(descriptionCombo->count() - 1, tr("No description", "Referring to the transaction description property (transaction title/generic article name)"));
		if(has_empty_payee) {
			if(current_account->type() == ACCOUNT_TYPE_EXPENSES) payeeCombo->setItemText(payeeCombo->count() - 1, tr("No payee"));
			else payeeCombo->setItemText(payeeCombo->count() - 1, tr("No payer"));
		}
	}
}

void CategoriesComparisonReport::updateTransactions() {
	if(b_extra && current_account) {
		int curindex_d = 0, curindex_p = 0;
		bool restore_d = (descriptionCombo->currentIndex() > 0);
		bool restore_p = (payeeCombo->currentIndex() > 0);
		payeeCombo->blockSignals(true);
		descriptionCombo->blockSignals(true);
		payeeCombo->clear();
		descriptionCombo->clear();
		descriptionCombo->addItem(tr("All descriptions", "Referring to the transaction description property (transaction title/generic article name)"));
		if(current_account->type() == ACCOUNT_TYPE_EXPENSES) payeeCombo->addItem(tr("All payees"));
		else payeeCombo->addItem(tr("All payers"));
		has_empty_description = false;
		has_empty_payee = false;
		QMap<QString, QString> descriptions, payees;
		for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constEnd(); it != budget->transactions.constBegin();) {
			--it;
			Transaction *trans = *it;
			if((trans->fromAccount() == current_account || trans->toAccount() == current_account)) {
				if(trans->description().isEmpty()) has_empty_description = true;
				else if(!descriptions.contains(trans->description().toLower())) descriptions[trans->description().toLower()] = trans->description();
				if(trans->type() == TRANSACTION_TYPE_EXPENSE) {
					if(((Expense*) trans)->payee().isEmpty()) has_empty_payee = true;
					else if(!payees.contains(((Expense*) trans)->payee().toLower())) payees[((Expense*) trans)->payee().toLower()] = ((Expense*) trans)->payee();
				} else if(trans->type() == TRANSACTION_TYPE_INCOME) {
					if(((Income*) trans)->payer().isEmpty()) has_empty_payee = true;
					else if(!payees.contains(((Income*) trans)->payer().toLower())) payees[((Income*) trans)->payer().toLower()] = ((Income*) trans)->payer();
				}
			}
		}
		int i = 1;
		QMap<QString, QString>::iterator it_e = descriptions.end();
		for(QMap<QString, QString>::iterator it = descriptions.begin(); it != it_e; ++it) {
			if(restore_d && !it.value().compare(current_description, Qt::CaseInsensitive)) {curindex_d = i;}
			descriptionCombo->addItem(it.value());
			i++;
		}
		if(has_empty_description) {
			if(restore_d && current_description.isEmpty()) curindex_d = i;
			descriptionCombo->addItem(tr("No description", "Referring to the transaction description property (transaction title/generic article name)"));
		}
		if(curindex_d < descriptionCombo->count()) {
			descriptionCombo->setCurrentIndex(curindex_d);
		}
		if(descriptionCombo->currentIndex() == 0) {
			current_description = "";
		}
		i = 1;
		QMap<QString, QString>::iterator it2_e = payees.end();
		for(QMap<QString, QString>::iterator it2 = payees.begin(); it2 != it2_e; ++it2) {
			if(restore_p && !it2.value().compare(current_payee, Qt::CaseInsensitive)) {curindex_p = i;}
			payeeCombo->addItem(it2.value());
			i++;
		}
		if(has_empty_payee) {
			if(restore_p && current_payee.isEmpty()) curindex_p = i;
			if(current_account->type() == ACCOUNT_TYPE_EXPENSES) payeeCombo->addItem(tr("No payee"));
			else payeeCombo->addItem(tr("No payer"));
		}
		if(curindex_p < payeeCombo->count()) {
			payeeCombo->setCurrentIndex(curindex_d);
		}
		if(payeeCombo->currentIndex() == 0) {
			current_payee = "";
		}
		payeeCombo->blockSignals(false);
		descriptionCombo->blockSignals(false);
	}
	updateDisplay();
}
void CategoriesComparisonReport::updateAccounts() {
	int curindex = 0;
	sourceCombo->blockSignals(true);
	sourceCombo->clear();
	sourceCombo->addItem(tr("All Categories, excluding subcategories"));
	sourceCombo->addItem(tr("All Categories, including subcategories"));
	sourceCombo->addItem(tr("All Payees/Payers"));
	int i = 1;
	for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
		Account *account = *it;
		sourceCombo->addItem(tr("Expenses: %1").arg(account->nameWithParent()));
		if(account == current_account) curindex = i;
		i++;
	}
	for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
		Account *account = *it;
		sourceCombo->addItem(tr("Incomes: %1").arg(account->nameWithParent()));
		if(account == current_account) curindex = i;
		i++;
	}
	if(curindex < sourceCombo->count()) sourceCombo->setCurrentIndex(curindex);
	sourceCombo->blockSignals(false);
	if(curindex == 0 && b_extra) {
		sourceChanged(curindex);
	} else {
		updateDisplay();
	}
}

