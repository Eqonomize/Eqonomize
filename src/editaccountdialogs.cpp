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

#include <QComboBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QGridLayout>
#include <QDateEdit>
#include <QRadioButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QMessageBox>
#include <QPushButton>
#include <QLocale>
#include <QDebug>

#include "budget.h"
#include "accountcombobox.h"
#include "editaccountdialogs.h"
#include "editcurrencydialog.h"
#include "eqonomizevalueedit.h"

EditAssetsAccountDialog::EditAssetsAccountDialog(Budget *budg, QWidget *parent, QString title, bool new_loan, int default_type, bool force_type, QString default_group) : QDialog(parent), budget(budg) {

	setWindowTitle(title);
	setModal(true);

	int row = 0;
	prev_currency_index = 1;

	QVBoxLayout *box1 = new QVBoxLayout(this);

	QGridLayout *grid = new QGridLayout();
	box1->addLayout(grid);
	if(new_loan) {
		typeCombo = NULL;
	} else {
		grid->addWidget(new QLabel(tr("Type:"), this), row, 0);
		typeCombo = new QComboBox(this);
		typeCombo->setEditable(false);
		typeCombo->addItem(budget->getAccountTypeName(ASSETS_TYPE_CASH, true));
		typeCombo->addItem(budget->getAccountTypeName(ASSETS_TYPE_CURRENT, true));
		typeCombo->addItem(budget->getAccountTypeName(ASSETS_TYPE_SAVINGS, true));
		typeCombo->addItem(budget->getAccountTypeName(ASSETS_TYPE_CREDIT_CARD, true));
		typeCombo->addItem(budget->getAccountTypeName(ASSETS_TYPE_LIABILITIES, true));
		typeCombo->addItem(budget->getAccountTypeName(ASSETS_TYPE_SECURITIES, true));
		typeCombo->addItem(budget->getAccountTypeName(ASSETS_TYPE_OTHER, true));
		grid->addWidget(typeCombo, row, 1); row++;
	}

	grid->addWidget(new QLabel(tr("Group:"), this), row, 0);
	groupCombo = new QComboBox(this);
	groupCombo->setEditable(true);
	QStringList groups;
	if(!new_loan) {
		groups << budget->getAccountTypeName(ASSETS_TYPE_CASH, true, true);
		groups << budget->getAccountTypeName(ASSETS_TYPE_CURRENT, true, true);
		groups << budget->getAccountTypeName(ASSETS_TYPE_SAVINGS, true, true);
		groups << budget->getAccountTypeName(ASSETS_TYPE_SECURITIES, true, true);
	}
	groups << budget->getAccountTypeName(ASSETS_TYPE_CREDIT_CARD, true, true);
	groups << budget->getAccountTypeName(ASSETS_TYPE_LIABILITIES, true, true);
	for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
		AssetsAccount *aaccount = *it;
		if(!new_loan || aaccount->isLiabilities()) {
			if(!aaccount->group().isEmpty()) groups << aaccount->group();
		}
	}
	groups.removeDuplicates();
	groups.sort(Qt::CaseInsensitive);
	groupCombo->addItems(groups);
	groupCombo->lineEdit()->setPlaceholderText(tr("no group"));
	grid->addWidget(groupCombo, row, 1); row++;
	if(!default_group.isEmpty()) groupCombo->setCurrentText(default_group);
	else if(default_type >= 0 && !budget->getAccountTypeName(default_type, true, true).isEmpty()) groupCombo->setCurrentText(budget->getAccountTypeName(default_type, true, true));

	if(default_type >= 0) {
		switch(default_type) {
			case ASSETS_TYPE_CASH: {typeCombo->setCurrentIndex(0); break;}
			case ASSETS_TYPE_CURRENT: {typeCombo->setCurrentIndex(1); break;}
			case ASSETS_TYPE_SAVINGS: {typeCombo->setCurrentIndex(2); break;}
			case ASSETS_TYPE_CREDIT_CARD: {typeCombo->setCurrentIndex(3); break;}
			case ASSETS_TYPE_LIABILITIES: {typeCombo->setCurrentIndex(4); break;}
			case ASSETS_TYPE_SECURITIES: {typeCombo->setCurrentIndex(5); break;}
			case ASSETS_TYPE_OTHER: {typeCombo->setCurrentIndex(6); break;}
		}
		if(force_type) typeCombo->setEnabled(false);
	}

	grid->addWidget(new QLabel(tr("Currency:"), this), row, 0);
	currencyCombo = new QComboBox(this);
	currencyCombo->setEditable(false);
	grid->addWidget(currencyCombo, row, 1); row++;
	editCurrencyButton = new QPushButton(tr("Edit"), this);
	grid->addWidget(editCurrencyButton, row, 1, Qt::AlignRight); row++;

	grid->addWidget(new QLabel(tr("Name:"), this), row, 0);
	nameEdit = new QLineEdit(this);
	grid->addWidget(nameEdit, row, 1); row++;
	maintainerLabel = new QLabel(tr("Bank:"));
	grid->addWidget(maintainerLabel, row, 0);
	maintainerEdit = new QLineEdit(this);
	grid->addWidget(maintainerEdit, row, 1); row++;
	valueLabel = new QLabel(new_loan ? tr("Debt:") : tr("Opening balance:", "Account balance"), this);
	grid->addWidget(valueLabel, row, 0);
	valueEdit = new EqonomizeValueEdit(!new_loan, this, budget);
	grid->addWidget(valueEdit, row, 1); row++;
	if(new_loan) {
		initialButton = new QRadioButton(tr("Opening balance", "Account balance"), this);
		initialButton->setChecked(true);
		grid->addWidget(initialButton, row, 0, 1, 2);
		row++;
		transferButton = new QRadioButton(tr("Transferred to:"), this);
		grid->addWidget(transferButton, row, 0);
		accountCombo = new AccountComboBox(ACCOUNT_TYPE_ASSETS, budget, false, false, false, true, true, this);
		accountCombo->updateAccounts();
		grid->addWidget(accountCombo, row, 1); row++;
		grid->addWidget(new QLabel(tr("Date:")), row, 0);
		dateEdit = new EqonomizeDateEdit(QDate::currentDate());
		dateEdit->setCalendarPopup(true);
		grid->addWidget(dateEdit, row, 1);
		row++;
		budgetButton = NULL;
		if(!accountCombo->hasAccount()) {
			transferButton->setEnabled(false);
		}
		transferToggled(false);
		maintainerLabel->setText(tr("Lender:"));
	} else {
		accountCombo = NULL;
		dateEdit = NULL;
		initialButton = NULL;
		transferButton = NULL;
		budgetButton = new QCheckBox(tr("Default account for budgeted transactions"), this);
		budgetButton->setChecked(false);
		grid->addWidget(budgetButton, row, 0, 1, 2); row++;
		maintainerEdit->setEnabled(false);
		maintainerLabel->setEnabled(false);
	}
	grid->addWidget(new QLabel(tr("Description:"), this), row, 0); row++;
	descriptionEdit = new QTextEdit(this);
	grid->addWidget(descriptionEdit, row, 0, 1, 2); row++;
	if(new_loan) {
		closedButton = NULL;
	} else {
		closedButton = new QCheckBox(tr("Account is closed"), this);
		closedButton->setChecked(false);
		closedButton->hide();
		grid->addWidget(closedButton, row, 0, 1, 2); row++;
	}
	nameEdit->setFocus();
	current_account = NULL;

	updateCurrencyList(budget->defaultCurrency());

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);

	if(typeCombo) connect(typeCombo, SIGNAL(activated(int)), this, SLOT(typeActivated(int)));
	if(currencyCombo) connect(currencyCombo, SIGNAL(activated(int)), this, SLOT(currencyActivated(int)));
	if(closedButton) connect(closedButton, SIGNAL(toggled(bool)), this, SLOT(closedToggled(bool)));
	if(transferButton) connect(transferButton, SIGNAL(toggled(bool)), this, SLOT(transferToggled(bool)));
	connect(editCurrencyButton, SIGNAL(clicked()), this, SLOT(editCurrency()));

}
void EditAssetsAccountDialog::updateCurrencyList(Currency *select_currency) {
	currencyCombo->clear();
	currencyCombo->addItem(tr("New currency…"));
	int i = 1;
	for(CurrencyList<Currency*>::const_iterator it = budget->currencies.constBegin(); it != budget->currencies.constEnd(); ++it) {
		Currency *currency = *it;
		if(!currency->name(false).isEmpty()) {
			currencyCombo->addItem(QIcon(":/data/flags/" + currency->code() + ".png"), QString("%2 (%1)").arg(qApp->translate("currencies.xml", qPrintable(currency->name()))).arg(currency->code()));
		} else {
			currencyCombo->addItem(currency->code());
		}
		currencyCombo->setItemData(i, QVariant::fromValue((void*) currency));
		if(currency == select_currency) {
			prev_currency_index = i;
			currencyCombo->setCurrentIndex(i);
			valueEdit->setCurrency((Currency*) currencyCombo->itemData(i).value<void*>());
		}
		i++;
	}
}
void EditAssetsAccountDialog::transferToggled(bool b) {
	dateEdit->setEnabled(b);
	accountCombo->setEnabled(b);
}
void EditAssetsAccountDialog::closedToggled(bool b) {
	if(b) budgetButton->setChecked(false);
	int index = typeCombo->currentIndex();
	budgetButton->setEnabled(!b && index != 5 && index != 4 && index != 3);
}
void EditAssetsAccountDialog::currencyActivated(int index) {
	Currency *cur = NULL;
	if(index > 0) cur = (Currency*) currencyCombo->itemData(index).value<void*>();
	if(index != prev_currency_index && current_account && cur != current_account->currency() && budget->accountHasTransactions(current_account) && QMessageBox::question(this, tr("Warning"), tr("If you change the currency of an account, the currency of all associated transactions will also change, without any conversion. Do do wish to continue anyway?")) != QMessageBox::Yes) {
		currencyCombo->setCurrentIndex(prev_currency_index);
		return;
	}
	if(index == 0) {
		EditCurrencyDialog *dialog = new EditCurrencyDialog(budget, NULL, true, this);
		if(dialog->exec() == QDialog::Accepted) {
			cur = dialog->createCurrency();
			updateCurrencyList(cur);
		} else {
			currencyCombo->setCurrentIndex(prev_currency_index);
		}
		dialog->deleteLater();
	} else {
		valueEdit->setCurrency(cur);
		prev_currency_index = index;
	}
}
void EditAssetsAccountDialog::editCurrency() {
	int i = currencyCombo->currentIndex();
	if(i == 0) return;
	Currency *cur = (Currency*) currencyCombo->itemData(i).value<void*>();
	EditCurrencyDialog *dialog = new EditCurrencyDialog(budget, cur, true, this);
	if(dialog->exec() == QDialog::Accepted) {
		dialog->modifyCurrency(cur);
	}
	dialog->deleteLater();
}
void EditAssetsAccountDialog::typeActivated(int index) {
	if(index == 5 && current_account && current_account->accountType() != ASSETS_TYPE_SECURITIES && budget->accountHasTransactions(current_account)) {
		QMessageBox::critical(this, tr("Error"), tr("Type cannot be changed to securities for accounts with transactions."));
		switch(current_account->accountType()) {
			case ASSETS_TYPE_CASH: {typeCombo->setCurrentIndex(0); break;}
			case ASSETS_TYPE_CURRENT: {typeCombo->setCurrentIndex(1); break;}
			case ASSETS_TYPE_SAVINGS: {typeCombo->setCurrentIndex(2); break;}
			case ASSETS_TYPE_CREDIT_CARD: {typeCombo->setCurrentIndex(3); break;}
			case ASSETS_TYPE_LIABILITIES: {typeCombo->setCurrentIndex(4); break;}
			default: {typeCombo->setCurrentIndex(6); break;}
		}
		return;
	}
	int i_type = -1;
	switch(typeCombo->currentIndex()) {
		case 0: {i_type = ASSETS_TYPE_CASH; break;}
		case 1: {i_type = ASSETS_TYPE_CURRENT; break;}
		case 2: {i_type = ASSETS_TYPE_SAVINGS; break;}
		case 3: {i_type = ASSETS_TYPE_CREDIT_CARD; break;}
		case 4: {i_type = ASSETS_TYPE_LIABILITIES; break;}
		case 5: {i_type = ASSETS_TYPE_SECURITIES;  break;}
		default: {i_type = ASSETS_TYPE_OTHER; break;}
	}
	if(budget->getAccountType(groupCombo->currentText(), true, true) != ASSETS_TYPE_OTHER) groupCombo->setCurrentText(budget->getAccountTypeName(i_type, true, true));
	valueEdit->setEnabled(index != 5);
	budgetButton->setEnabled(index != 5 && index != 4 && index != 3);
	closedButton->setEnabled(index != 4);
	if(index == 4) closedButton->setChecked(false);
	if(index == 5 || index == 4 || index == 3) budgetButton->setChecked(false);
	if(index == 5) valueEdit->setValue(0.0);
	maintainerEdit->setEnabled(index != 0);
	maintainerLabel->setEnabled(index != 0);
	if(index == 3) maintainerLabel->setText(tr("Issuer:"));
	else if(index == 4) maintainerLabel->setText(tr("Lender:"));
	else maintainerLabel->setText(tr("Bank:"));
}
AssetsAccount *EditAssetsAccountDialog::newAccount(Transaction **transfer) {
	AssetsType type;
	if(typeCombo) {
		switch(typeCombo->currentIndex()) {
			case 0: {type = ASSETS_TYPE_CASH; break;}
			case 1: {type = ASSETS_TYPE_CURRENT; break;}
			case 2: {type = ASSETS_TYPE_SAVINGS; break;}
			case 3: {type = ASSETS_TYPE_CREDIT_CARD; break;}
			case 4: {type = ASSETS_TYPE_LIABILITIES; break;}
			case 5: {type = ASSETS_TYPE_SECURITIES;  break;}
			default: {type = ASSETS_TYPE_OTHER; break;}
		}
	} else {
		type = ASSETS_TYPE_LIABILITIES;
	}
	AssetsAccount *account = new AssetsAccount(budget, type, nameEdit->text(), 0.0, descriptionEdit->toPlainText());
	int i = currencyCombo->currentIndex();
	account->setCurrency((Currency*) currencyCombo->itemData(i).value<void*>());
	if(maintainerEdit->isEnabled()) account->setMaintainer(maintainerEdit->text());
	else account->setMaintainer("");
	account->setGroup(groupCombo->currentText());
	if(transfer && transferButton && transferButton->isChecked()) {
		Transaction *trans = new Transfer(budget, valueEdit->value(), dateEdit->date(), account, (AssetsAccount*) accountCombo->currentAccount(), nameEdit->text());
		*transfer = trans;
	} else if(type == ASSETS_TYPE_LIABILITIES && transferButton) {
		account->setInitialBalance(-valueEdit->value());
	} else {
		account->setInitialBalance(valueEdit->value());
	}
	if(budgetButton) account->setAsBudgetAccount(budgetButton->isChecked());
	return account;
}
void EditAssetsAccountDialog::modifyAccount(AssetsAccount *account) {
	account->setName(nameEdit->text());
	if(maintainerEdit->isEnabled()) account->setMaintainer(maintainerEdit->text());
	else account->setMaintainer("");
	account->setInitialBalance(valueEdit->value());
	account->setDescription(descriptionEdit->toPlainText());
	account->setAsBudgetAccount(budgetButton->isChecked());
	account->setClosed(closedButton->isChecked());
	account->setGroup(groupCombo->currentText());
	switch(typeCombo->currentIndex()) {
		case 0: {account->setAccountType(ASSETS_TYPE_CASH); break;}
		case 1: {account->setAccountType(ASSETS_TYPE_CURRENT); break;}
		case 2: {account->setAccountType(ASSETS_TYPE_SAVINGS); break;}
		case 3: {account->setAccountType(ASSETS_TYPE_CREDIT_CARD); break;}
		case 4: {account->setAccountType(ASSETS_TYPE_LIABILITIES); break;}
		case 5: {account->setAccountType(ASSETS_TYPE_SECURITIES);  break;}
		default: {account->setAccountType(ASSETS_TYPE_OTHER); break;}
	}
	int i = currencyCombo->currentIndex();
	account->setCurrency((Currency*) currencyCombo->itemData(i).value<void*>());
}
void EditAssetsAccountDialog::setAccount(AssetsAccount *account) {
	current_account = account;
	if(account->isClosed() || budget->accountHasTransactions(account)) {
		closedButton->show();
		closedButton->setChecked(account->isClosed());
	} else {
		closedButton->hide();
		closedButton->setChecked(false);
	}
	nameEdit->setText(account->name());
	maintainerEdit->setText(account->maintainer());
	valueEdit->setValue(account->initialBalance());
	valueEdit->setCurrency(account->currency());
	descriptionEdit->setPlainText(account->description());
	budgetButton->setChecked(account->isBudgetAccount());
	typeCombo->setEnabled(true);
	switch(account->accountType()) {
		case ASSETS_TYPE_CASH: {typeCombo->setCurrentIndex(0); break;}
		case ASSETS_TYPE_CURRENT: {typeCombo->setCurrentIndex(1); break;}
		case ASSETS_TYPE_SAVINGS: {typeCombo->setCurrentIndex(2); break;}
		case ASSETS_TYPE_CREDIT_CARD: {typeCombo->setCurrentIndex(3); break;}
		case ASSETS_TYPE_LIABILITIES: {typeCombo->setCurrentIndex(4); break;}
		case ASSETS_TYPE_SECURITIES: {
			typeCombo->setCurrentIndex(5);
			for(SecurityList<Security*>::const_iterator it = budget->securities.constBegin(); it != budget->securities.constEnd(); ++it) {
				Security *sec = *it;
				if(sec->account() == account) {
					typeCombo->setEnabled(false);
					break;
				}
			}
			break;
		}
		default: {typeCombo->setCurrentIndex(6); break;}
	}
	for(int i = 0; i < currencyCombo->count(); i++) {
		if(currencyCombo->itemData(i).value<void*>() == account->currency()) {
			currencyCombo->setCurrentIndex(i);
			prev_currency_index = i;
			break;
		}
	}
	typeActivated(typeCombo->currentIndex());
	groupCombo->setCurrentText(account->group());
}
void EditAssetsAccountDialog::accept() {
	QString sname = nameEdit->text().trimmed();
	if(sname.isEmpty()) {
		nameEdit->setFocus();
		QMessageBox::critical(this, tr("Error"), tr("Empty name."));
		return;
	}
	AssetsAccount *aaccount = budget->findAssetsAccount(sname);
	if(aaccount && aaccount != current_account) {
		nameEdit->setFocus();
		QMessageBox::critical(this, tr("Error"), tr("The entered name is used by another account."));
		return;
	}
	if(transferButton && valueEdit->value() <= 0.0) {
		valueEdit->setFocus();
		QMessageBox::critical(this, tr("Error"), tr("Zero value not allowed."));
		return;
	}
	QDialog::accept();
}


EditIncomesAccountDialog::EditIncomesAccountDialog(Budget *budg, IncomesAccount *default_parent, QWidget *parent, QString title) : QDialog(parent), budget(budg) {

	setWindowTitle(title);
	setModal(true);

	if(default_parent && default_parent->parentCategory()) default_parent = (IncomesAccount*) default_parent->parentCategory();

	QVBoxLayout *box1 = new QVBoxLayout(this);

	QGridLayout *grid = new QGridLayout();
	box1->addLayout(grid);
	grid->addWidget(new QLabel(tr("Name:"), this), 0, 0);
	nameEdit = new QLineEdit(this);
	grid->addWidget(nameEdit, 0, 1);
	grid->addWidget(new QLabel(tr("Parent category:"), this), 1, 0);
	parentCombo = new QComboBox(this);
	parentCombo->setEditable(false);
	grid->addWidget(parentCombo, 1, 1);
	parentCombo->addItem(tr("None"));
	parentCombo->setCurrentIndex(0);
	int i = 1;
	for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
		IncomesAccount *account = *it;
		if(!account->parentCategory()) {
			parentCombo->addItem(account->name());
			parentCombo->setItemData(i, QVariant::fromValue((void*) account));
			if(account == default_parent) parentCombo->setCurrentIndex(i);
			i++;
		}
	}

	budgetButton = new QCheckBox(tr("Monthly budget:"), this);
	budgetButton->setChecked(false);
	grid->addWidget(budgetButton, 2, 0);
	budgetEdit = new EqonomizeValueEdit(false, this, budget);
	budgetEdit->setEnabled(false);
	grid->addWidget(budgetEdit, 2, 1);
	grid->addWidget(new QLabel(tr("Description:"), this), 3, 0);
	descriptionEdit = new QTextEdit(this);
	grid->addWidget(descriptionEdit, 4, 0, 1, 2);
	nameEdit->setFocus();
	current_account = NULL;

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);

	connect(budgetButton, SIGNAL(toggled(bool)), this, SLOT(budgetEnabled(bool)));
}
IncomesAccount *EditIncomesAccountDialog::newAccount() {
	IncomesAccount *account = new IncomesAccount(budget, nameEdit->text(), descriptionEdit->toPlainText());
	if(budgetButton->isChecked()) {
		account->setMonthlyBudget(QDate::currentDate().year(), QDate::currentDate().month(), budgetEdit->value());
	}
	int i = parentCombo->currentIndex();
	if(i > 0) {
		account->setParentCategory((IncomesAccount*) parentCombo->itemData(i).value<void*>());
	}
	return account;
}
void EditIncomesAccountDialog::modifyAccount(IncomesAccount *account) {
	account->setName(nameEdit->text());
	account->setDescription(descriptionEdit->toPlainText());
	int i = parentCombo->currentIndex();
	account->setParentCategory((IncomesAccount*) parentCombo->itemData(i).value<void*>());
}
void EditIncomesAccountDialog::setAccount(IncomesAccount *account) {
	current_account = account;
	nameEdit->setText(account->name());
	if(!account->subCategories.isEmpty()) {
		parentCombo->setEnabled(false);
	}
	for(int i = 1; i < parentCombo->count(); i++) {
		if(parentCombo->itemData(i).value<void*>() == account) {
			parentCombo->removeItem(i);
			break;
		} else if(parentCombo->itemData(i).value<void*>() == account->parentCategory()) {
			parentCombo->setCurrentIndex(i);
			break;
		}
	}
	budgetEdit->hide();
	budgetButton->hide();
	descriptionEdit->setPlainText(account->description());
}
void EditIncomesAccountDialog::budgetEnabled(bool b) {
	budgetEdit->setEnabled(b);
}
void EditIncomesAccountDialog::accept() {
	QString sname = nameEdit->text().trimmed();
	if(sname.isEmpty()) {
		nameEdit->setFocus();
		QMessageBox::critical(this, tr("Error"), tr("Empty name."));
		return;
	}
	IncomesAccount *iaccount = budget->findIncomesAccount(sname);
	if(iaccount && iaccount != current_account) {
		nameEdit->setFocus();
		QMessageBox::critical(this, tr("Error"), tr("The entered name is used by another income category."));
		return;
	}
	QDialog::accept();
}

EditExpensesAccountDialog::EditExpensesAccountDialog(Budget *budg, ExpensesAccount *default_parent, QWidget *parent, QString title) : QDialog(parent), budget(budg) {

	setWindowTitle(title);
	setModal(true);

	if(default_parent && default_parent->parentCategory()) default_parent = (ExpensesAccount*) default_parent->parentCategory();

	QVBoxLayout *box1 = new QVBoxLayout(this);

	QGridLayout *grid = new QGridLayout();
	box1->addLayout(grid);
	grid->addWidget(new QLabel(tr("Name:"), this), 0, 0);
	nameEdit = new QLineEdit(this);
	grid->addWidget(nameEdit, 0, 1);
	grid->addWidget(new QLabel(tr("Parent category:"), this), 1, 0);
	parentCombo = new QComboBox(this);
	parentCombo->setEditable(false);
	grid->addWidget(parentCombo, 1, 1);
	parentCombo->addItem(tr("None"));
	parentCombo->setCurrentIndex(0);
	int i = 1;
	for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
		ExpensesAccount *account = *it;
		if(!account->parentCategory()) {
			parentCombo->addItem(account->name());
			parentCombo->setItemData(i, QVariant::fromValue((void*) account));
			if(account == default_parent) parentCombo->setCurrentIndex(i);
			i++;
		}
	}
	budgetButton = new QCheckBox(tr("Monthly budget:"), this);
	budgetButton->setChecked(false);
	grid->addWidget(budgetButton, 2, 0);
	budgetEdit = new EqonomizeValueEdit(false, this, budget);
	budgetEdit->setEnabled(false);
	grid->addWidget(budgetEdit, 2, 1);
	grid->addWidget(new QLabel(tr("Description:"), this), 3, 0);
	descriptionEdit = new QTextEdit(this);
	grid->addWidget(descriptionEdit, 4, 0, 1, 2);
	nameEdit->setFocus();
	current_account = NULL;

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);

	connect(budgetButton, SIGNAL(toggled(bool)), this, SLOT(budgetEnabled(bool)));
}
ExpensesAccount *EditExpensesAccountDialog::newAccount() {
	ExpensesAccount *account = new ExpensesAccount(budget, nameEdit->text(), descriptionEdit->toPlainText());
	if(budgetButton->isChecked()) {
		account->setMonthlyBudget(QDate::currentDate().year(), QDate::currentDate().month(), budgetEdit->value());
	}
	int i = parentCombo->currentIndex();
	if(i > 0) {
		account->setParentCategory((ExpensesAccount*) parentCombo->itemData(i).value<void*>());
	}
	return account;
}
void EditExpensesAccountDialog::modifyAccount(ExpensesAccount *account) {
	account->setName(nameEdit->text());
	account->setDescription(descriptionEdit->toPlainText());
	int i = parentCombo->currentIndex();
	account->setParentCategory((ExpensesAccount*) parentCombo->itemData(i).value<void*>());
}
void EditExpensesAccountDialog::setAccount(ExpensesAccount *account) {
	current_account = account;
	nameEdit->setText(account->name());
	if(!account->subCategories.isEmpty()) {
		parentCombo->setEnabled(false);
	}
	for(int i = 1; i < parentCombo->count(); i++) {
		if(parentCombo->itemData(i).value<void*>() == account) {
			parentCombo->removeItem(i);
			break;
		} else if(parentCombo->itemData(i).value<void*>() == account->parentCategory()) {
			parentCombo->setCurrentIndex(i);
			break;
		}
	}
	budgetEdit->hide();
	budgetButton->hide();
	descriptionEdit->setPlainText(account->description());
}
void EditExpensesAccountDialog::budgetEnabled(bool b) {
	budgetEdit->setEnabled(b);
}
void EditExpensesAccountDialog::accept() {
	QString sname = nameEdit->text().trimmed();
	if(sname.isEmpty()) {
		nameEdit->setFocus();
		QMessageBox::critical(this, tr("Error"), tr("Empty name."));
		return;
	}
	ExpensesAccount *eaccount = budget->findExpensesAccount(sname);
	if(eaccount && eaccount != current_account) {
		nameEdit->setFocus();
		QMessageBox::critical(this, tr("Error"), tr("The entered name is used by another expense category."));
		return;
	}
	QDialog::accept();
}

