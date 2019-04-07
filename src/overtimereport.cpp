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

#include "overtimereport.h"


#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QObject>
#include <QString>
#include <QPushButton>
#include <QVBoxLayout>
#include <qvector.h>
#include <QTextStream>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QUrl>
#include <QFileDialog>
#include <QSaveFile>
#include <QApplication>
#include <QTemporaryFile>
#include <QMimeDatabase>
#include <QMimeType>
#include <QMessageBox>
#include <QPrinter>
#include <QTextEdit>
#include <QPrintDialog>
#include <QSettings>
#include <QStandardPaths>

#include "account.h"
#include "budget.h"
#include "recurrence.h"
#include "transaction.h"

#include <cmath>

extern QString htmlize_string(QString str);
extern QString last_document_directory;

struct month_info {
	double value;
	double expense;
	double count;
	QDate date;
};

OverTimeReport::OverTimeReport(Budget *budg, QWidget *parent) : QWidget(parent), budget(budg) {

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
	settings.beginGroup("OverTimeReport");

	QWidget *settingsWidget = new QWidget(this);
	QGridLayout *settingsLayout = new QGridLayout(settingsWidget);

	settingsLayout->addWidget(new QLabel(tr("Source:"), settingsWidget), 0, 0);
	QHBoxLayout *choicesLayout = new QHBoxLayout();
	settingsLayout->addLayout(choicesLayout, 0, 1);
	sourceCombo = new QComboBox(settingsWidget);
	sourceCombo->setEditable(false);
	sourceCombo->addItem(tr("Profits"));
	sourceCombo->addItem(tr("Expenses"));
	sourceCombo->addItem(tr("Incomes"));
	sourceCombo->addItem(tr("Assets & Liabilities"));
	choicesLayout->addWidget(sourceCombo);
	choicesLayout->setStretchFactor(sourceCombo, 1);
	categoryCombo = new QComboBox(settingsWidget);
	categoryCombo->setEditable(false);
	categoryCombo->addItem(tr("All Categories Combined"));
	categoryCombo->setEnabled(false);
	choicesLayout->addWidget(categoryCombo);
	choicesLayout->setStretchFactor(categoryCombo, 1);
	descriptionCombo = new QComboBox(settingsWidget);
	descriptionCombo->setEditable(false);
	descriptionCombo->addItem(tr("All Descriptions Combined", "Referring to the transaction description property (transaction title/generic article name)"));
	descriptionCombo->setEnabled(false);
	choicesLayout->addWidget(descriptionCombo);
	choicesLayout->setStretchFactor(descriptionCombo, 1);
	accountCombo = new QComboBox(settingsWidget);
	accountCombo->setEditable(false);
	accountCombo->addItem(tr("All Accounts"), qVariantFromValue(NULL));
	for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
		AssetsAccount *account = *it;
		if(account != budget->balancingAccount) {
			accountCombo->addItem(account->name(), qVariantFromValue((void*) account));
		}
	}
	choicesLayout->addWidget(accountCombo);
	choicesLayout->setStretchFactor(accountCombo, 1);

	current_account = NULL;
	current_source = 0;

	columnsLabel = new QLabel(tr("Columns:"), settingsWidget);
	settingsLayout->addWidget(columnsLabel, 1, 0);
	QHBoxLayout *enabledLayout = new QHBoxLayout();
	settingsLayout->addLayout(enabledLayout, 1, 1);
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
	
	valueButton->setEnabled(current_source != 12 && current_source != 0);
	dailyButton->setEnabled(current_source != 12 && current_source != 0);
	monthlyButton->setEnabled(current_source != 12 && current_source != 0);
	yearlyButton->setEnabled(current_source != 12 && current_source != 0);
	countButton->setEnabled(current_source != 12 && current_source != 0);
	perButton->setEnabled(current_source != 12 && current_source != 0);
	columnsLabel->setEnabled(current_source != 12 && current_source != 0);
	
	settings.endGroup();
	
	layout->addWidget(settingsWidget);

	connect(valueButton, SIGNAL(toggled(bool)), this, SLOT(updateDisplay()));
	connect(dailyButton, SIGNAL(toggled(bool)), this, SLOT(updateDisplay()));
	connect(monthlyButton, SIGNAL(toggled(bool)), this, SLOT(updateDisplay()));
	connect(yearlyButton, SIGNAL(toggled(bool)), this, SLOT(updateDisplay()));
	connect(countButton, SIGNAL(toggled(bool)), this, SLOT(updateDisplay()));
	connect(perButton, SIGNAL(toggled(bool)), this, SLOT(updateDisplay()));
	connect(sourceCombo, SIGNAL(activated(int)), this, SLOT(sourceChanged(int)));
	connect(categoryCombo, SIGNAL(activated(int)), this, SLOT(categoryChanged(int)));
	connect(descriptionCombo, SIGNAL(activated(int)), this, SLOT(descriptionChanged(int)));
	connect(accountCombo, SIGNAL(activated(int)), this, SLOT(updateDisplay()));
	connect(saveButton, SIGNAL(clicked()), this, SLOT(save()));
	connect(printButton, SIGNAL(clicked()), this, SLOT(print()));

}

void OverTimeReport::resetOptions() {
	accountCombo->blockSignals(true);
	accountCombo->setCurrentIndex(0);
	accountCombo->blockSignals(false);
	sourceCombo->blockSignals(true);
	sourceCombo->setCurrentIndex(0);
	sourceCombo->blockSignals(false);
	sourceChanged(0);
}

void OverTimeReport::descriptionChanged(int index) {
	current_description = "";
	bool b_income = (current_account && current_account->type() == ACCOUNT_TYPE_INCOMES);
	if(index == 0) {
		if(b_income) current_source = 5;
		else current_source = 6;
	} else {
		if(!has_empty_description || index < descriptionCombo->count() - 1) current_description = descriptionCombo->itemText(index);
		if(b_income) current_source = 9;
		else current_source = 10;
	}
	updateDisplay();
}
void OverTimeReport::categoryChanged(int index) {
	descriptionCombo->blockSignals(true);
	descriptionCombo->clear();
	descriptionCombo->addItem(tr("All Descriptions Combined", "Referring to the transaction description property (transaction title/generic article name)"));
	current_account = NULL;
	if(index == 0) {
		if(sourceCombo->currentIndex() == 2) {
			current_source = 1;
		} else {
			current_source = 2;
		}
		descriptionCombo->setEnabled(false);
	} else {
		if(sourceCombo->currentIndex() == 1) {
			int i = categoryCombo->currentIndex() - 1;
			if(i < (int) budget->expensesAccounts.count()) {
				current_account = budget->expensesAccounts.at(i);
			}
			current_source = 6;
		} else {
			int i = categoryCombo->currentIndex() - 1;
			if(i < (int) budget->incomesAccounts.count()) {
				current_account = budget->incomesAccounts.at(i);
			}
			current_source = 5;
		}
		has_empty_description = false;
		QMap<QString, QString> descriptions;
		for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constEnd(); it != budget->transactions.constBegin();) {
			--it;
			Transaction *trans = *it;
			if((trans->fromAccount() == current_account || trans->toAccount() == current_account)) {
				if(trans->description().isEmpty()) has_empty_description = true;
				else if(!descriptions.contains(trans->description().toLower())) descriptions[trans->description().toLower()] = trans->description();
			}
		}
		QMap<QString, QString>::iterator it_e = descriptions.end();
		for(QMap<QString, QString>::iterator it = descriptions.begin(); it != it_e; ++it) {
			descriptionCombo->addItem(it.value());
		}
		if(has_empty_description) descriptionCombo->addItem(tr("No description", "Referring to the transaction description property (transaction title/generic article name)"));
		descriptionCombo->setEnabled(true);
	}
	descriptionCombo->blockSignals(false);
	updateDisplay();
}
void OverTimeReport::sourceChanged(int index) {
	categoryCombo->blockSignals(true);
	descriptionCombo->blockSignals(true);
	categoryCombo->clear();
	descriptionCombo->clear();
	descriptionCombo->setEnabled(false);
	descriptionCombo->addItem(tr("All Descriptions Combined", "Referring to the transaction description property (transaction title/generic article name)"));
	current_description = "";
	current_account = NULL;
	categoryCombo->addItem(tr("All Categories Combined"));
	if(index == 2) {
		for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
			Account *account = *it;
			categoryCombo->addItem(account->nameWithParent());
		}
		categoryCombo->setEnabled(true);
		current_source = 1;
	} else if(index == 1) {
		for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
			Account *account = *it;
			categoryCombo->addItem(account->nameWithParent());
		}
		categoryCombo->setEnabled(true);
		current_source = 2;
	} else {
		categoryCombo->setEnabled(false);
		if(index == 3) current_source = 12;
		else current_source = 0;
	}
	valueButton->setEnabled(current_source != 12 && current_source != 0);
	dailyButton->setEnabled(current_source != 12 && current_source != 0);
	monthlyButton->setEnabled(current_source != 12 && current_source != 0);
	yearlyButton->setEnabled(current_source != 12 && current_source != 0);
	countButton->setEnabled(current_source != 12 && current_source != 0);
	perButton->setEnabled(current_source != 12 && current_source != 0);
	columnsLabel->setEnabled(current_source != 12 && current_source != 0);
	categoryCombo->blockSignals(false);
	descriptionCombo->blockSignals(false);
	updateDisplay();
}


void OverTimeReport::saveConfig() {
	QSettings settings;
	settings.beginGroup("OverTimeReport");
	settings.setValue("size", ((QDialog*) parent())->size());
	settings.setValue("valueEnabled", valueButton->isChecked());
	settings.setValue("dailyAverageEnabled", dailyButton->isChecked());
	settings.setValue("monthlyAverageEnabled", monthlyButton->isChecked());
	settings.setValue("yearlyAverageEnabled", yearlyButton->isChecked());
	settings.setValue("transactionCountEnabled", countButton->isChecked());
	settings.setValue("valuePerTransactionEnabled", perButton->isChecked());
	settings.endGroup();
}
void OverTimeReport::save() {
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

void OverTimeReport::print() {
	QPrinter printer;
	QPrintDialog print_dialog(&printer, this);
	if(print_dialog.exec() == QDialog::Accepted) {
		htmlview->print(&printer);
	}
}

void OverTimeReport::updateDisplay() {

	if(!isVisible()) return;

	int columns = 2;
	bool enabled[8];
	enabled[0] = valueButton->isChecked();
	enabled[1] = dailyButton->isChecked();
	enabled[2] = monthlyButton->isChecked();
	enabled[3] = yearlyButton->isChecked();
	enabled[4] = countButton->isChecked();
	enabled[5] = perButton->isChecked();
	enabled[6] = false;
	enabled[7] = false;
	
	AssetsAccount *current_assets = selectedAccount();

	QVector<month_info> monthly_values;
	month_info *mi = NULL;
	QDate first_date;
	AccountType at = ACCOUNT_TYPE_EXPENSES;
	Account *account = NULL;
	int type = 0;
	QString title, valuetitle, pertitle, expensetitle, sumtitle;
	switch(current_source) {
		case 0: {
			if(current_assets) {
				type = 4;
				//: Noun, how much the account balance has changed
				title = tr("Change: %1").arg(current_assets->name());
				valuetitle = tr("Deposit", "Money put into account");
				expensetitle = tr("Withdrawal", "Money taken out from account");
				//: Noun, how much the account balance has changed
				sumtitle = tr("Change");
			} else {
				type = 0;
				title = tr("Profits");
				valuetitle = tr("Incomes");
				expensetitle = tr("Expenses");
				sumtitle = title;
			}
			enabled[1] = false;
			enabled[2] = false;
			enabled[3] = false;
			enabled[4] = false;
			enabled[5] = false;
			enabled[6] = true;
			enabled[7] = true;
			break;
		}
		case 1: {
			if(current_assets) title = tr("Incomes, %1").arg(current_assets->name());
			else title = tr("Incomes");
			pertitle = tr("Average Income");
			valuetitle = title;
			type = 1;
			at = ACCOUNT_TYPE_INCOMES;
			break;
		}
		case 2: {
			if(current_assets) title = tr("Expenses, %1").arg(current_assets->name());
			else title = tr("Expenses");
			pertitle = tr("Average Cost");
			valuetitle = title;
			type = 1;
			at = ACCOUNT_TYPE_EXPENSES;
			break;
		}
		case 5: {
			account = current_account;
			if(current_assets) title = tr("Incomes, %2: %1").arg(account->nameWithParent()).arg(current_assets->name());
			else title = tr("Incomes: %1").arg(account->nameWithParent());
			pertitle = tr("Average Income");
			valuetitle = tr("Incomes");
			type = 2;
			at = ACCOUNT_TYPE_INCOMES;
			break;
		}
		case 6: {
			account = current_account;
			if(current_assets) title = tr("Expenses, %2: %1").arg(account->nameWithParent()).arg(current_assets->name());
			else title = tr("Expenses: %1").arg(account->nameWithParent());
			pertitle = tr("Average Cost");
			valuetitle = tr("Expenses");
			type = 2;
			at = ACCOUNT_TYPE_EXPENSES;
			break;
		}
		case 9: {
			account = current_account;
			if(current_assets) title = tr("Incomes, %3: %2, %1").arg(account->nameWithParent()).arg(current_description.isEmpty() ? tr("No description", "Referring to the transaction description property (transaction title/generic article name)") : current_description).arg(current_assets->name());
			else title = tr("Incomes: %2, %1").arg(account->nameWithParent()).arg(current_description.isEmpty() ? tr("No description", "Referring to the transaction description property (transaction title/generic article name)") : current_description);
			pertitle = tr("Average Income");
			valuetitle = tr("Incomes");
			type = 3;
			at = ACCOUNT_TYPE_INCOMES;
			break;
		}
		case 10: {
			account = current_account;
			if(current_assets) title = tr("Expenses, %3: %2, %1").arg(account->nameWithParent()).arg(current_description.isEmpty() ? tr("No description", "Referring to the transaction description property (transaction title/generic article name)") : current_description).arg(current_assets->name());
			else title = tr("Expenses: %2, %1").arg(account->nameWithParent()).arg(current_description.isEmpty() ? tr("No description", "Referring to the transaction description property (transaction title/generic article name)") : current_description);
			pertitle = tr("Average Cost");
			valuetitle = tr("Expenses");
			type = 3;
			at = ACCOUNT_TYPE_EXPENSES;
			break;
		}
		case 12: {
			if(current_assets) {
				title = tr("Value: %1").arg(current_assets->name());
				type = 6;
				valuetitle = tr("Value");
			} else {
				title = tr("Assets & Liabilities");
				type = 5;
				valuetitle = tr("Assets");
				expensetitle = tr("Liabilities");
				sumtitle = tr("Total");
				enabled[6] = true;
				enabled[7] = true;
			}
			enabled[1] = false;
			enabled[2] = false;
			enabled[3] = false;
			enabled[4] = false;
			enabled[5] = false;
			break;
		}
	}
	
	for(size_t i = 0; i < 8; i++) {
		if(enabled[i]) columns++;
	}
	
	QDate start_date, first_trans_date;
	for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constBegin(); it != budget->transactions.constEnd(); ++it) {
		Transaction *trans = *it;
		if(!first_trans_date.isValid()) {
			first_trans_date = budget->firstBudgetDay(trans->date());
		}
		if(((current_source == 12 || trans->fromAccount()->type() != ACCOUNT_TYPE_ASSETS || trans->toAccount()->type() != ACCOUNT_TYPE_ASSETS) && (!current_assets || trans->relatesToAccount(current_assets))) || (current_source == 0 && current_assets && trans->relatesToAccount(current_assets))) {
			start_date = trans->date();
			if(!budget->isFirstBudgetDay(start_date)) {
				start_date = budget->firstBudgetDay(start_date);
				if(current_source == 12) budget->addBudgetMonthsSetFirst(start_date, -1);
				else if(start_date == first_trans_date) budget->addBudgetMonthsSetFirst(start_date, 1);
			}
			break;
		}
	}
	QDate curmonth = budget->firstBudgetDay(QDate::currentDate());
	if(start_date.isNull() || start_date > curmonth) start_date = curmonth;
	if(start_date == curmonth) {
		budget->addBudgetMonthsSetFirst(start_date, -1);
	}
	first_date = start_date;

	QDate curdate = QDate::currentDate().addDays(-1);
	if(!budget->isLastBudgetDay(curdate)) {
		curdate = budget->lastBudgetDay(curdate);
		budget->addBudgetMonthsSetLast(curdate, -1);
	}
	if(curdate < first_date || budget->isSameBudgetMonth(start_date, curdate)) {
		curdate = QDate::currentDate();
	}

	bool started = false;
	bool includes_planned = false;
	for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constBegin(); it != budget->transactions.constEnd(); ++it) {
		Transaction *trans = *it;
		if(trans->date() > curdate) break;
		bool include = false;
		int sign = 1;
		if(!started && trans->date() >= first_date) started = true;
		if(started && (!current_assets || trans->relatesToAccount(current_assets))) {
			if(type >= 4) {
				include = true;
			} else if((type == 1 && trans->fromAccount()->type() == at) || (type == 2 && (trans->fromAccount() == account || trans->fromAccount()->topAccount() == account)) || (type == 3 && (trans->fromAccount() == account || trans->fromAccount()->topAccount() == account) && !trans->description().compare(current_description, Qt::CaseInsensitive)) || (type == 0 && trans->fromAccount()->type() != ACCOUNT_TYPE_ASSETS)) {
				if(type == 0) sign = 1;
				else if(at == ACCOUNT_TYPE_INCOMES) sign = 1;
				else sign = -1;
				include = true;
			} else if((type == 1 && trans->toAccount()->type() == at) || (type == 2 && (trans->toAccount() == account || trans->toAccount()->topAccount() == account)) || (type == 3 && (trans->toAccount() == account || trans->toAccount()->topAccount() == account) && !trans->description().compare(current_description, Qt::CaseInsensitive)) || (type == 0 && trans->toAccount()->type() != ACCOUNT_TYPE_ASSETS)) {
				if(type == 0) sign = -1;
				else if(at == ACCOUNT_TYPE_INCOMES) sign = -1;
				else sign = 1;
				include = true;
			}
		}
		if(include) {
			if(!mi || trans->date() > mi->date) {
				QDate newdate, olddate;
				newdate = budget->lastBudgetDay(trans->date());
				if(mi) {
					olddate = mi->date;
					budget->addBudgetMonthsSetLast(olddate, 1);
				} else {
					olddate = budget->lastBudgetDay(first_date);
				}
				while(olddate < newdate) {
					monthly_values.append(month_info());
					mi = &monthly_values.back();
					mi->value = 0.0;
					mi->count = 0.0;
					mi->date = olddate;
					budget->addBudgetMonthsSetLast(olddate, 1);
				}
				monthly_values.append(month_info());
				mi = &monthly_values.back();
				if(type == 0) {
					if(sign == 1) mi->value = trans->value(!current_assets);
					else mi->expense = trans->value(!current_assets);
					mi->count = trans->quantity();
				} else if(type == 4) {
					if(trans->accountChange(current_assets) >= 0.0) mi->value = trans->accountChange(current_assets);
					else mi->expense = -trans->accountChange(current_assets);
					mi->count = 1.0;
				} else if(type == 6) {
					mi->value = trans->accountChange(current_assets);
					mi->count = 1.0;
				} else if(type == 5) {
					mi->expense = 0.0;
					mi->value = 0.0;
					if(trans->fromAccount()->type() == ACCOUNT_TYPE_ASSETS) {
						if(((AssetsAccount*) trans->fromAccount())->accountType() == ASSETS_TYPE_LIABILITIES || ((AssetsAccount*) trans->fromAccount())->accountType() == ASSETS_TYPE_CREDIT_CARD) mi->expense -= trans->value(true);
						else if(((AssetsAccount*) trans->fromAccount())->accountType() != ASSETS_TYPE_SECURITIES && trans->fromAccount() != budget->balancingAccount) mi->value -= trans->value(true);
					}
					if(trans->toAccount()->type() == ACCOUNT_TYPE_ASSETS) {
						if(((AssetsAccount*) trans->toAccount())->accountType() == ASSETS_TYPE_LIABILITIES || ((AssetsAccount*) trans->toAccount())->accountType() == ASSETS_TYPE_CREDIT_CARD) mi->expense += trans->value(true);
						else if(((AssetsAccount*) trans->toAccount())->accountType() != ASSETS_TYPE_SECURITIES && trans->toAccount() != budget->balancingAccount) mi->value += trans->value(true);
					}
				} else {
					mi->value = trans->value(!current_assets) * sign;
					mi->count = trans->quantity();
				}
				mi->date = newdate;
			} else {
				if(type == 0) {
					if(sign == 1) mi->value += trans->value(!current_assets);
					else mi->expense += trans->value(!current_assets);
					mi->count += trans->quantity();
				} else if(type == 4) {
					if(trans->accountChange(current_assets) >= 0.0) mi->value += trans->accountChange(current_assets);
					else mi->expense -= trans->accountChange(current_assets);
					mi->count++;
				} else if(type == 6) {
					mi->value += trans->accountChange(current_assets);
					mi->count++;
				} else if(type == 5) {
					if(trans->fromAccount()->type() == ACCOUNT_TYPE_ASSETS) {
						if(((AssetsAccount*) trans->fromAccount())->accountType() == ASSETS_TYPE_LIABILITIES || ((AssetsAccount*) trans->fromAccount())->accountType() == ASSETS_TYPE_CREDIT_CARD) mi->expense -= trans->value(true);
						else if(((AssetsAccount*) trans->fromAccount())->accountType() != ASSETS_TYPE_SECURITIES && trans->fromAccount() != budget->balancingAccount) mi->value -= trans->value(true);
					}
					if(trans->toAccount()->type() == ACCOUNT_TYPE_ASSETS) {
						if(((AssetsAccount*) trans->toAccount())->accountType() == ASSETS_TYPE_LIABILITIES || ((AssetsAccount*) trans->toAccount())->accountType() == ASSETS_TYPE_CREDIT_CARD) mi->expense += trans->value(true);
						else if(((AssetsAccount*) trans->toAccount())->accountType() != ASSETS_TYPE_SECURITIES && trans->toAccount() != budget->balancingAccount) mi->value += trans->value(true);
					}
				} else {
					mi->value += trans->value(!current_assets) * sign;
					mi->count += trans->quantity();
				}
			}
		}
	}
	if(mi) {
		while(mi->date < curdate) {
			QDate newdate = mi->date;
			budget->addBudgetMonthsSetLast(newdate, 1);
			monthly_values.append(month_info());
			mi = &monthly_values.back();
			mi->value = 0.0;
			mi->expense = 0.0;
			mi->count = 0.0;
			mi->date = newdate;
		}
	}
	double scheduled_value = 0.0;
	double scheduled_expense = 0.0;
	double scheduled_count = 0.0;
	if(mi) {
		int split_i = 0;
		for(ScheduledTransactionList<ScheduledTransaction*>::const_iterator it = budget->scheduledTransactions.constBegin(); it != budget->scheduledTransactions.constEnd();) {
			ScheduledTransaction *strans = *it;
			if(strans->firstOccurrence() > mi->date) break;
			started = true;
			if(split_i == 0 && strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT && ((SplitTransaction*) strans->transaction())->count() == 0) {
				do {
					++it;
					if(it == budget->scheduledTransactions.constEnd()) {
						strans = NULL;
						break;
					}
					strans = *it;
					if(strans->firstOccurrence() > mi->date) {
						strans = NULL;
						break;
					}
				} while(split_i == 0 && strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT && ((SplitTransaction*) strans->transaction())->count() == 0);
				if(!strans) break;
			}
			Transaction *trans = NULL;
			if(strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT) {
				trans = ((SplitTransaction*) strans->transaction())->at(split_i);
				split_i++;
			} else {
				trans = (Transaction*) strans->transaction();
			}
			bool include = false;
			int sign = 1;
			if(!current_assets || trans->relatesToAccount(current_assets)) {
				if(type >= 4) {
					include = true;
				} else if((type == 1 && trans->fromAccount()->type() == at) || (type == 2 && (trans->fromAccount() == account || trans->fromAccount()->topAccount() == account)) || (type == 3 && (trans->fromAccount() == account || trans->fromAccount()->topAccount() == account) && !trans->description().compare(current_description, Qt::CaseInsensitive)) || (type == 0 && trans->fromAccount()->type() != ACCOUNT_TYPE_ASSETS)) {
					if(type == 0) sign = 1;
					else if(at == ACCOUNT_TYPE_INCOMES) sign = 1;
					else sign = -1;
					include = true;
				} else if((type == 1 && trans->toAccount()->type() == at) || (type == 2 && (trans->toAccount() == account || trans->toAccount()->topAccount() == account)) || (type == 3 && (trans->toAccount() == account || trans->toAccount()->topAccount() == account) && !trans->description().compare(current_description, Qt::CaseInsensitive)) || (type == 0 && trans->toAccount()->type() != ACCOUNT_TYPE_ASSETS)) {
					if(type == 0) sign = -1;
					else if(at == ACCOUNT_TYPE_INCOMES) sign = -1;
					else sign = 1;
					include = true;
				}
			}
			if(include) {
				int count = (strans->recurrence() ? strans->recurrence()->countOccurrences(mi->date) : 1);
				if(count != 0) {
					includes_planned = true;
					if(type == 0) {
						if(sign == 1) scheduled_value += (trans->value(!current_assets) * count);
						else scheduled_expense += (trans->value(!current_assets) * count);
						scheduled_count += count * trans->quantity();
					} else if(type == 4) {
						if(trans->accountChange(current_assets) >= 0.0) scheduled_value += trans->accountChange(current_assets) * count;
						else scheduled_expense -= trans->accountChange(current_assets) * count;
						scheduled_count += count;
					} else if(type == 6) {
						scheduled_value += trans->accountChange(current_assets) * count;
						scheduled_count += count;
					} else if(type == 5) {
						if(trans->fromAccount()->type() == ACCOUNT_TYPE_ASSETS) {
							if(((AssetsAccount*) trans->fromAccount())->accountType() == ASSETS_TYPE_LIABILITIES || ((AssetsAccount*) trans->fromAccount())->accountType() == ASSETS_TYPE_CREDIT_CARD) scheduled_expense -= trans->value(true) * count;
							else if(((AssetsAccount*) trans->fromAccount())->accountType() != ASSETS_TYPE_SECURITIES && trans->fromAccount() != budget->balancingAccount) scheduled_value -= trans->value(true) * count;
						}
						if(trans->toAccount()->type() == ACCOUNT_TYPE_ASSETS) {
							if(((AssetsAccount*) trans->toAccount())->accountType() == ASSETS_TYPE_LIABILITIES || ((AssetsAccount*) trans->toAccount())->accountType() == ASSETS_TYPE_CREDIT_CARD) scheduled_expense += trans->value(true) * count;
							else if(((AssetsAccount*) trans->toAccount())->accountType() != ASSETS_TYPE_SECURITIES && trans->toAccount() != budget->balancingAccount) scheduled_value += trans->value(true) * count;
						}
					} else {
						scheduled_value += (trans->value(!current_assets) * sign * count);
						scheduled_count += count * trans->quantity();
					}
				}
			}
			if(strans->transaction()->generaltype() != GENERAL_TRANSACTION_TYPE_SPLIT || split_i >= ((SplitTransaction*) strans->transaction())->count()) {
				++it;
				split_i = 0;
			}
		}
	}
	if(monthly_values.isEmpty()) {
		monthly_values.append(month_info());
		mi = &monthly_values.back();
		mi->value = 0.0;
		mi->expense = 0.0;
		mi->count = 0.0;
		mi->date = budget->lastBudgetDay(first_date);
		while(mi->date < curdate) {
			QDate newdate = mi->date;
			budget->addBudgetMonthsSetLast(newdate, 1);
			monthly_values.append(month_info());
			mi = &monthly_values.back();
			mi->value = 0.0;
			mi->expense = 0.0;
			mi->count = 0.0;
			mi->date = newdate;
		}
	}
	if(current_source == 12) {
		if(type == 6) {
			if(current_assets->accountType() != ASSETS_TYPE_SECURITIES) {
				double total_value = current_assets->initialBalance(false);
				QVector<month_info>::iterator it_b = monthly_values.begin();
				QVector<month_info>::iterator it_e = monthly_values.end();
				while(it_b != it_e) {
					total_value += it_b->value;
					it_b->value = total_value;
					it_b++;
				}
			} else {
				QVector<month_info>::iterator it_b = monthly_values.begin();
				QVector<month_info>::iterator it_e = monthly_values.end();
				while(it_b != it_e) {
					it_b->value = 0.0;
					it_b++;
				}
				for(SecurityList<Security*>::const_iterator it = budget->securities.constBegin(); it != budget->securities.constEnd(); ++it) {
					Security *sec = *it;
					AssetsAccount *ass = sec->account();
					if(ass == current_assets) {
						QVector<month_info>::iterator it_b = monthly_values.begin();
						QVector<month_info>::iterator it_e = monthly_values.end();
						while(it_b != it_e) {
							it_b->value += sec->value(it_b->date);
							it_b++;
						}
					}
				}
			}
		} else if(type == 5) {
			double total_value = 0.0, total_expense = 0.0;
			for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
				AssetsAccount *account = *it;
				if(account->accountType() == ASSETS_TYPE_LIABILITIES || account->accountType() == ASSETS_TYPE_CREDIT_CARD) total_expense += account->currency()->convertTo(account->initialBalance(false), budget->defaultCurrency(), start_date);
				else total_value += account->currency()->convertTo(account->initialBalance(false), budget->defaultCurrency(), start_date);
			}
			QVector<month_info>::iterator it_b = monthly_values.begin();
			QVector<month_info>::iterator it_e = monthly_values.end();
			while(it_b != it_e) {
				total_value += it_b->value;
				it_b->value = total_value;
				total_expense += it_b->expense;
				it_b->expense = total_expense;
				it_b++;
			}
			for(SecurityList<Security*>::const_iterator it = budget->securities.constBegin(); it != budget->securities.constEnd(); ++it) {
				Security *sec = *it;
				it_b = monthly_values.begin();
				it_e = monthly_values.end();
				while(it_b != it_e) {
					it_b->value += sec->currency()->convertTo(sec->value(it_b->date), budget->defaultCurrency(), it_b->date);
					it_b++;
				}
			}
		}
	}
	double average_month = budget->averageMonth(first_date, curdate, true);
	double average_year = budget->averageYear(first_date, curdate, true);
	source = "";
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
	outf << "\t\t<h2 align=\"center\">" << htmlize_string(title) << "</h2>" << '\n' << "\t\t<br>";
	outf << "\t\t<table width=\"100%\" border=\"0.5\" style=\"border-style: solid; border-color: #cccccc\" cellspacing=\"0\" cellpadding=\"5\">" << '\n';
	outf << "\t\t\t<thead align=\"left\">" << '\n';
	outf << "\t\t\t\t<tr bgcolor=\"#f0f0f0\">" << '\n';
	outf << "\t\t\t\t\t<th align=\"left\">" << htmlize_string(tr("Year")) << "</th>";
	outf << "\t\t\t\t\t<th align=\"left\">" << htmlize_string(tr("Month")) << "</th>";
	bool use_footer1 = false;
	if(enabled[0]) {
		outf << "\t\t\t\t\t<th align=\"left\">" << htmlize_string(valuetitle);
		if(includes_planned) {outf << "*"; use_footer1 = true;}
		outf<< "</th>";
	}
	if(enabled[1]) outf << "\t\t\t\t\t<th align=\"left\">" << htmlize_string(tr("Daily Average")) << "</th>";
	if(enabled[2]) outf << "\t\t\t\t\t<th align=\"left\">" << htmlize_string(tr("Monthly Average")) << (use_footer1 ? "**" : "*") << "</th>";
	if(enabled[3]) outf << "\t\t\t\t\t<th align=\"left\">" << htmlize_string(tr("Yearly Average")) << (use_footer1 ? "**" : "*") << "</th>";
	if(enabled[4]) {
		outf << "\t\t\t\t\t<th align=\"left\">" << htmlize_string(tr("Quantity"));
		if(includes_planned) {outf << "*"; use_footer1 = true;}
		outf<< "</th>";
	}
	if(enabled[5]) {
		outf << "\t\t\t\t\t<th align=\"left\">" << htmlize_string(pertitle);
		if(includes_planned) {outf << "*"; use_footer1 = true;}
		outf<< "</th>";
	}
	if(enabled[6]) {
		outf << "\t\t\t\t\t<th align=\"left\">" << htmlize_string(expensetitle);
		if(includes_planned) {outf << "*"; use_footer1 = true;}
		outf<< "</th>";
	}
	if(enabled[7]) {
		outf << "\t\t\t\t\t<th align=\"left\">" << htmlize_string(sumtitle);
		if(includes_planned) {outf << "*"; use_footer1 = true;}
		outf<< "</th>";
	}
	outf << "\t\t\t\t</tr>" << '\n';
	outf << "\t\t\t</thead>" << '\n';
	outf << "\t\t\t<tbody>" << '\n';
	QVector<month_info>::iterator it_b = monthly_values.begin();
	QVector<month_info>::iterator it_e = monthly_values.end();
	if(it_e != it_b) --it_e;
	QVector<month_info>::iterator it = monthly_values.end();
	int year = 0;
	double yearly_value = 0.0, total_value = 0.0;
	double yearly_expense = 0.0, total_expense = 0.0;
	double yearly_count = 0.0, total_count = 0.0;
	QDate year_date;
	bool first_year = true, first_month = true;
	bool multiple_months = monthly_values.size() > 1;
	bool multiple_years = multiple_months && budget->budgetYear(first_date) != budget->budgetYear(curdate);
	int i_count_frac = 0;
	double intpart = 0.0;
	while(it != it_b) {
		--it;
		if(modf(it->count, &intpart) != 0.0) {
			i_count_frac = 2;
			break;
		}
	}
	it = monthly_values.end();
	Currency *currency = budget->defaultCurrency();
	if(current_assets) currency = current_assets->currency();
	while(it != it_b) {
		--it;
		if(first_month || year != budget->budgetYear(it->date)) {
			if(!first_month && multiple_years && current_source != 12) {
				outf << "\t\t\t\t<tr bgcolor=\"#f0f0f0\">" << '\n';
				outf << "\t\t\t\t\t<td></td>";
				outf << "\t\t\t\t\t<td align=\"left\"><b>" << htmlize_string(tr("Subtotal")) << "</b></td>";
				if(enabled[0]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(first_year ? (yearly_value + scheduled_value) : yearly_value)) << "</b></td>";
				int days = 1;
				if(first_year) {
					days = budget->dayOfBudgetYear(curdate);
				} else if(budget->budgetYear(first_date) == year) {
					days = budget->daysInBudgetYear(year_date);
					days -= (budget->dayOfBudgetYear(first_date) - 1);
				} else {
					days = budget->daysInBudgetYear(year_date);
				}
				if(enabled[1]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(yearly_value / days)) << "</b></td>";
				if(enabled[2]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(((yearly_value * average_month) / days))) << "</b></td>";
				if(enabled[3]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue((yearly_value * average_year) / days)) << "</b></td>";
				if(enabled[4]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(budget->formatValue(first_year ? (yearly_count + scheduled_count) : yearly_count, i_count_frac)) << "</b></td>";
				double pervalue = 0.0;
				if(first_year) {
					pervalue = (((yearly_count + scheduled_count) == 0.0) ? 0.0 : ((yearly_value + scheduled_value) / (yearly_count + scheduled_count)));
				} else {
					pervalue = (yearly_count == 0.0 ? 0.0 : (yearly_value / yearly_count));
				}
				if(enabled[5]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(pervalue)) << "</b></td>";
				if(enabled[6]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(first_year ? (yearly_expense + scheduled_expense) : yearly_expense)) << "</b></td>";
				if(enabled[7]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(first_year ? (yearly_value + scheduled_value - yearly_expense - scheduled_expense) : yearly_value - yearly_expense)) << "</b></td>";
				first_year = false;
				outf << "\n";
				outf << "\t\t\t\t</tr>" << '\n';
				outf << "\t\t\t\t<tr>" << '\n';
			} else if(!first_month && multiple_years && current_source == 12) {
				outf << "\t\t\t\t<tr bgcolor=\"#f0f0f0\">" << '\n';
			} else {
				outf << "\t\t\t\t<tr>" << '\n';
			}
			year = budget->budgetYear(it->date);
			yearly_value = it->value;
			yearly_expense = it->expense;
			yearly_count = it->count;
			year_date = it->date;
			outf << "\t\t\t\t\t<td align=\"left\">" << htmlize_string(budget->budgetYearString(it->date)) << "</td>";
		} else {
			outf << "\t\t\t\t<tr>" << '\n';
			yearly_value += it->value;
			yearly_expense += it->expense;
			yearly_count += it->count;
			outf << "\t\t\t\t\t<td></td>";
		}
		total_value += it->value;
		total_expense += it->expense;
		total_count += it->count;
		outf << "\t\t\t\t\t<td align=\"left\">" << htmlize_string(QDate::longMonthName(budget->budgetMonth(it->date), QDate::StandaloneFormat)) << "</td>";
		if(enabled[0]) outf << "<td nowrap align=\"right\">" << htmlize_string(currency->formatValue(first_month ? (it->value + scheduled_value) : it->value)) << "</td>";
		int days = 0;
		if(first_month) {
			days = budget->dayOfBudgetMonth(curdate);
		} else if(it == it_b) {
			days = budget->daysInBudgetMonth(it->date);
			days -= (budget->dayOfBudgetMonth(first_date) - 1);
		} else {
			days = budget->dayOfBudgetMonth(it->date);
		}
		if(enabled[1]) outf << "<td nowrap align=\"right\">" << htmlize_string(currency->formatValue(it->value / days)) << "</td>";
		if(enabled[2]) outf << "<td nowrap align=\"right\">" << htmlize_string(currency->formatValue((it->value * average_month) / days)) << "</td>";
		if(enabled[3]) outf << "<td nowrap align=\"right\">" << htmlize_string(currency->formatValue((it->value * average_year) / days)) << "</td>";
		if(enabled[4]) outf << "<td nowrap align=\"right\">" << htmlize_string(budget->formatValue(first_month ? (it->count + scheduled_count) : it->count, i_count_frac)) << "</td>";
		if(enabled[5]) {
			double pervalue = 0.0;
			if(first_month) {
				pervalue = (((it->count + scheduled_count) == 0.0) ? 0.0 : ((it->value + scheduled_value) / (it->count + scheduled_count)));
			} else {
				pervalue = (it->count == 0.0 ? 0.0 : (it->value / it->count));
			}
			outf << "<td nowrap align=\"right\">" << htmlize_string(currency->formatValue(pervalue)) << "</td>";
		}
		if(enabled[6]) outf << "<td nowrap align=\"right\">" << htmlize_string(currency->formatValue(first_month ? (it->expense + scheduled_expense) : it->expense)) << "</td>";
		if(enabled[7]) outf << "<td nowrap align=\"right\">" << htmlize_string(currency->formatValue(first_month ? (it->value - it->expense + scheduled_value - scheduled_expense) : it->value - it->expense)) << "</td>";
		first_month = false;
		outf << "\n";
		outf << "\t\t\t\t</tr>" << '\n';
	}
	if(multiple_years && current_source != 12) {
		outf << "\t\t\t\t<tr bgcolor=\"#f0f0f0\">" << '\n';
		outf << "\t\t\t\t\t<td></td>";
		outf << "\t\t\t\t\t<td align=\"left\"><b>" << htmlize_string(tr("Subtotal")) << "</b></td>";
		if(enabled[0]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(yearly_value)) << "</b></td>";
		int days = budget->daysInBudgetYear(year_date);
		days -= (budget->dayOfBudgetYear(first_date) - 1);
		if(enabled[1]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(yearly_value / days)) << "</b></td>";
		if(enabled[2]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue((yearly_value * average_month) / days)) << "</b></td>";
		if(enabled[3]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue((yearly_value * average_year) / days)) << "</b></td>";
		if(enabled[4]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(budget->formatValue(yearly_count, i_count_frac)) << "</b></td>";
		if(enabled[5]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(yearly_count == 0.0 ? 0.0 : (yearly_value / yearly_count))) << "</b></td>";
		if(enabled[6]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(yearly_expense)) << "</b></td>";
		if(enabled[7]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(yearly_value - yearly_expense)) << "</b></td>";
		outf << "\n";
		outf << "\t\t\t\t</tr>" << '\n';
	}
	if(multiple_months && current_source != 12) {
		outf << "\t\t\t\t<tr bgcolor=\"#f0f0f0\">" << '\n';
		int days = first_date.daysTo(curdate) + 1;
		outf << "\t\t\t\t\t<td align=\"left\"><b>" << htmlize_string(tr("Total")) << "</b></td>";
		outf << "\t\t\t\t\t<td></td>";
		if(enabled[0]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(total_value + scheduled_value)) << "</b></td>";
		if(enabled[1]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(total_value / days)) << "</b></td>";
		if(enabled[2]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue((total_value * average_month) / days)) << "</b></td>";
		if(enabled[3]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue((total_value * average_year) / days)) << "</b></td>";
		if(enabled[4]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(budget->formatValue(total_count + scheduled_count, i_count_frac)) << "</b></td>";
		if(enabled[5]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue((total_count + scheduled_count) == 0.0 ? 0.0 : ((total_value + scheduled_value) / (total_count + scheduled_count)))) << "</b></td>";
		if(enabled[6]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(total_expense + scheduled_expense)) << "</b></td>";
		if(enabled[7]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(total_value - total_expense + scheduled_value - scheduled_expense)) << "</b></td>";
		outf << "\n";
		outf << "\t\t\t\t</tr>" << '\n';
	}
	outf << "\t\t\t</tbody>" << '\n';
	outf << "\t\t</table>" << '\n';
	outf << "\t\t<div align=\"right\" style=\"font-weight: normal\">" << "<small>" << '\n';
	if(use_footer1) {
		outf << "\t\t\t<br>" << '\n';
		outf << "\t\t\t" << "*" << htmlize_string(tr("Includes scheduled transactions")) << '\n';
	}
	if(enabled[2] || enabled[3]) {
		outf << "\t\t\t" << "<br>" << '\n';
		outf << "\t\t\t" << (use_footer1 ? "**" : "*") << htmlize_string(tr("Adjusted for the average month / year (%1 / %2 days)").arg(budget->formatValue(average_month, 1)).arg(budget->formatValue(average_year, 1))) << '\n';
	}
	outf << "\t\t</small></div>" << '\n';
	outf << "\t</body>" << '\n';
	outf << "</html>" << '\n';
	htmlview->setHtml(source);
}
void OverTimeReport::updateTransactions() {
	if(descriptionCombo->isEnabled() && current_account) {
		int curindex = 0;
		bool update_index = descriptionCombo->currentIndex() > 0;
		descriptionCombo->blockSignals(true);
		descriptionCombo->clear();
		descriptionCombo->addItem(tr("All Descriptions Combined", "Referring to the transaction description property (transaction title/generic article name)"));
		has_empty_description = false;
		QMap<QString, QString> descriptions;
		for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constEnd(); it != budget->transactions.constBegin();) {
			--it;
			Transaction *trans = *it;
			if((trans->fromAccount() == current_account || trans->toAccount() == current_account)) {
				if(trans->description().isEmpty()) has_empty_description = true;
				else if(!descriptions.contains(trans->description().toLower())) descriptions[trans->description().toLower()] = trans->description();
			}
		}
		QMap<QString, QString>::iterator it_e = descriptions.end();
		int i = 1;
		for(QMap<QString, QString>::iterator it = descriptions.begin(); it != it_e; ++it) {
			if(update_index && (current_source == 9 || current_source == 10) && !it.key().compare(current_description, Qt::CaseInsensitive)) {
				curindex = i;
			}
			descriptionCombo->addItem(it.value());
			i++;
		}
		if(has_empty_description) {
			if(update_index && (current_source == 9 || current_source == 10) && current_description.isEmpty()) curindex = i;
			descriptionCombo->addItem(tr("No description", "Referring to the transaction description property (transaction title/generic article name)"));
		}
		if(curindex < descriptionCombo->count()) {
			descriptionCombo->setCurrentIndex(curindex);
		}
		bool b_income = (current_account && current_account->type() == ACCOUNT_TYPE_INCOMES);
		if(descriptionCombo->currentIndex() == 0) {
			if(b_income) current_source = 5;
			else current_source = 6;
			current_description = "";
		}
		descriptionCombo->blockSignals(false);
	}
	updateDisplay();
}
void OverTimeReport::updateAccounts() {
	if(categoryCombo->isEnabled()) {
		int curindex = 0;
		categoryCombo->blockSignals(true);
		descriptionCombo->blockSignals(true);
		categoryCombo->clear();
		categoryCombo->addItem(tr("All Categories Combined"));
		int i = 1;
		if(sourceCombo->currentIndex() == 1) {
			for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
				Account *account = *it;
				categoryCombo->addItem(account->nameWithParent());
				if(account == current_account) curindex = i;
				i++;
			}
		} else {
			for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
				Account *account = *it;
				categoryCombo->addItem(account->nameWithParent());
				if(account == current_account) curindex = i;
				i++;
			}
		}
		if(curindex < categoryCombo->count()) categoryCombo->setCurrentIndex(curindex);
		if(curindex == 0) {
			descriptionCombo->clear();
			descriptionCombo->setEnabled(false);
			descriptionCombo->addItem(tr("All Descriptions Combined", "Referring to the transaction description property (transaction title/generic article name)"));
			if(sourceCombo->currentIndex() == 2) {
				current_source = 1;
			} else {
				current_source = 2;
			}
			descriptionCombo->setEnabled(false);
		}
		categoryCombo->blockSignals(false);
		descriptionCombo->blockSignals(false);
	}
	accountCombo->blockSignals(true);
	AssetsAccount *current_assets = selectedAccount();
	accountCombo->clear();
	accountCombo->addItem(tr("All Accounts"), qVariantFromValue(NULL));
	for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
		AssetsAccount *account = *it;
		if(account != budget->balancingAccount) {
			accountCombo->addItem(account->name(), qVariantFromValue((void*) account));
		}
	}
	int index = 0;
	if(current_assets) index = accountCombo->findData(qVariantFromValue((void*) current_assets));
	if(index >= 0) accountCombo->setCurrentIndex(index);
	else accountCombo->setCurrentIndex(0);
	accountCombo->blockSignals(false);
	updateDisplay();
}
AssetsAccount *OverTimeReport::selectedAccount() {
	if(!accountCombo->currentData().isValid()) return NULL;
	return (AssetsAccount*) accountCombo->currentData().value<void*>();
}

