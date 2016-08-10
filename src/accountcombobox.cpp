/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016 by Hanna Knutsson                 *
 *   hanna_k@fmgirl.com                                                    *
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

#include "accountcombobox.h"
#include "account.h"
#include "budget.h"
#include "editaccountdialogs.h"

AccountComboBox::AccountComboBox(int account_type, Budget *budg, bool add_new_account_action, bool add_new_loan_action, bool add_multiple_accounts_action, bool exclude_securities_accounts, bool exclude_balancing_account, QWidget *parent) : QComboBox(parent), i_type(account_type), budget(budg), new_account_action(add_new_account_action), new_loan_action(add_new_loan_action && i_type == ACCOUNT_TYPE_ASSETS), multiple_accounts_action(add_multiple_accounts_action && i_type == ACCOUNT_TYPE_ASSETS), b_exclude_securities(exclude_securities_accounts), b_exclude_balancing(exclude_balancing_account) {
	setEditable(false);
	added_account = NULL;
	connect(this, SIGNAL(activated(int)), this, SLOT(accountActivated(int)));
}
AccountComboBox::~AccountComboBox() {}

int AccountComboBox::firstAccountIndex() const {
	int index = 0;
	if(new_account_action) index++;
	if(new_loan_action) index++;
	if(multiple_accounts_action) index++;
	if(index > 0) index++;
	return index;
}
Account *AccountComboBox::currentAccount() const {
	if(!currentData().isValid()) return NULL;
	return (Account*) currentData().value<void*>();
}
void AccountComboBox::setCurrentAccount(Account *account) {
	if(account) setCurrentIndex(findData(qVariantFromValue((void*) account)));
}
int AccountComboBox::currentAccountIndex() const {
	int index = currentIndex() - firstAccountIndex();
	if(index < 0) return -1;
	return index;
}
void AccountComboBox::setCurrentAccountIndex(int index) {
	index += firstAccountIndex();
	if(index < count()) setCurrentIndex(index);
}
void AccountComboBox::updateAccounts(Account *exclude_account) {
	Account *current_account = currentAccount();
	clear();
	switch(i_type) {
		case ACCOUNT_TYPE_ASSETS: {
			if(new_account_action) addItem(tr("New account…"), qVariantFromValue(NULL));
			if(new_loan_action) addItem(tr("Payed with loan / payment plan…"), qVariantFromValue(NULL));
			if(multiple_accounts_action) addItem(tr("Multiple accounts/payments…"), qVariantFromValue(NULL));
			if(count() > 0) insertSeparator(count());
			bool add_secondary_list = false;
			AssetsAccount *account = budget->assetsAccounts.first();
			while(account) {
				if((account->accountType() == ASSETS_TYPE_SECURITIES && !b_exclude_securities) || account->accountType() == ASSETS_TYPE_LIABILITIES || (account == budget->balancingAccount && !b_exclude_balancing) || account->isClosed()) {
					add_secondary_list = true;
				} else if(account != exclude_account && account->accountType() != ASSETS_TYPE_SECURITIES && account != budget->balancingAccount) {
					addItem(account->name(), qVariantFromValue((void*) account));
				}
				account = budget->assetsAccounts.next();
			}
			if(add_secondary_list) {
				if(count() > firstAccountIndex()) insertSeparator(count());
				account = budget->assetsAccounts.first();
				while(account) {
					if((account->accountType() == ASSETS_TYPE_SECURITIES && !b_exclude_securities) || account->accountType() == ASSETS_TYPE_LIABILITIES || (account == budget->balancingAccount && !b_exclude_balancing) || account->isClosed()) {
						addItem(account->name(), qVariantFromValue((void*) account));
					}
					account = budget->assetsAccounts.next();
				}
			}
			if(current_account) setCurrentAccount(current_account);
			if(currentIndex() < firstAccountIndex()) setCurrentIndex(firstAccountIndex());
			break;
		}
		case ACCOUNT_TYPE_INCOMES: {
			if(new_account_action) {
				addItem(tr("New income category…"), qVariantFromValue(NULL));
				insertSeparator(count());
			}
			IncomesAccount *account = budget->incomesAccounts.first();
			while(account) {
				if(account != exclude_account) {
					addItem(account->nameWithParent(), qVariantFromValue((void*) account));
				}
				account = budget->incomesAccounts.next();
			}
			if(current_account) setCurrentAccount(current_account);
			if(currentIndex() < firstAccountIndex()) setCurrentIndex(firstAccountIndex());
			break;
		}
		case ACCOUNT_TYPE_EXPENSES: {
			if(new_account_action) {
				addItem(tr("New expense category…"), qVariantFromValue(NULL));
				insertSeparator(count());
			}
			ExpensesAccount *account = budget->expensesAccounts.first();
			while(account) {
				if(account != exclude_account) {
					addItem(account->nameWithParent(), qVariantFromValue((void*) account));
				}
				account = budget->expensesAccounts.next();
			}
			if(current_account) setCurrentAccount(current_account);
			if(currentIndex() < firstAccountIndex()) setCurrentIndex(firstAccountIndex());
			break;
		}
	}
}
bool AccountComboBox::hasAccount() const {
	return count() > firstAccountIndex();
}
Account *AccountComboBox::createAccount() {
	Account *account = NULL;
	switch(i_type) {
		case ACCOUNT_TYPE_ASSETS: {
			EditAssetsAccountDialog *dialog = new EditAssetsAccountDialog(budget, this, tr("New Account"));
			if(dialog->exec() == QDialog::Accepted) {
				account = dialog->newAccount();
			}
			dialog->deleteLater();
			break;
		}
		case ACCOUNT_TYPE_INCOMES: {
			EditIncomesAccountDialog *dialog = new EditIncomesAccountDialog(budget, NULL, this, tr("New Income Category"));
			if(dialog->exec() == QDialog::Accepted) {
				account = dialog->newAccount();
			}
			dialog->deleteLater();
			break;
		}
		case ACCOUNT_TYPE_EXPENSES: {
			EditExpensesAccountDialog *dialog = new EditExpensesAccountDialog(budget, NULL, this, tr("New Expense Category"));
			if(dialog->exec() == QDialog::Accepted) {
				account = dialog->newAccount();
			}
			dialog->deleteLater();
			break;
		}
	}
	if(account) {
		budget->addAccount(account);
		updateAccounts();
		setCurrentAccount(account);
		emit accountSelected();
	}
	return account;
}
		
void AccountComboBox::accountActivated(int index) {
	if(new_account_action && index == 0) {
		setCurrentIndex(firstAccountIndex());
		emit newAccountRequested();
	} else if(new_loan_action && ((index == 1 && new_account_action) || (index == 0 && !new_account_action))) {
		setCurrentIndex(firstAccountIndex());
		emit newLoanRequested();
	} else if(multiple_accounts_action && ((index == 2 && new_account_action && new_loan_action) || (index == 1 && !(new_account_action) != !(new_loan_action)) || (index == 0 && !new_account_action && !new_loan_action))) {
		setCurrentIndex(firstAccountIndex());
		emit multipleAccountsRequested();
	} else {
		emit accountSelected();
	}
}

