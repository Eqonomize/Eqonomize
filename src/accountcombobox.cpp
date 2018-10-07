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

#include <QDebug>

#include "accountcombobox.h"
#include "account.h"
#include "budget.h"
#include "editaccountdialogs.h"

AccountComboBox::AccountComboBox(int account_type, Budget *budg, bool add_new_account_action, bool add_new_loan_action, bool add_multiple_accounts_action, bool exclude_securities_accounts, bool exclude_balancing_account, QWidget *parent) : QComboBox(parent), i_type(account_type), budget(budg), new_account_action(add_new_account_action), new_loan_action(add_new_loan_action && i_type == ACCOUNT_TYPE_ASSETS), multiple_accounts_action(add_multiple_accounts_action && i_type == ACCOUNT_TYPE_ASSETS), b_exclude_securities(exclude_securities_accounts), b_exclude_balancing(exclude_balancing_account) {
	setEditable(false);
	added_account = NULL;
	block_account_selected = false;
	connect(this, SIGNAL(activated(int)), this, SLOT(accountActivated(int)));
	connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentIndexChanged(int)));
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
	if(account) {
		int index = findData(qVariantFromValue((void*) account));
		if(index >= 0) setCurrentIndex(index);
		//emit currentAccountChanged();
	}
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
void AccountComboBox::updateAccounts(Account *exclude_account, Currency *force_currency) {
	Account *current_account = currentAccount();
	clear();
	switch(i_type) {
		case ACCOUNT_TYPE_INCOMES: {
			if(new_account_action) {
				addItem(tr("New income category…"), qVariantFromValue(NULL));
			}
			for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
				IncomesAccount *account = *it;
				if(account != exclude_account) {
					addItem(account->nameWithParent(), qVariantFromValue((void*) account));
				}
			}
			if(new_account_action && count() > 1) insertSeparator(1);
			if(current_account) setCurrentAccount(current_account);
			if(currentIndex() < firstAccountIndex()) {
				if(firstAccountIndex() < count()) setCurrentIndex(firstAccountIndex());
				else setCurrentIndex(-1);
			}
			break;
		}
		case ACCOUNT_TYPE_EXPENSES: {
			if(new_account_action) {
				addItem(tr("New expense category…"), qVariantFromValue(NULL));
			}
			for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
				ExpensesAccount *account = *it;
				if(account != exclude_account) {
					addItem(account->nameWithParent(), qVariantFromValue((void*) account));
				}
			}
			if(new_account_action && count() > 1) insertSeparator(1);
			if(current_account) setCurrentAccount(current_account);
			if(currentIndex() < firstAccountIndex()) {
				if(firstAccountIndex() < count()) setCurrentIndex(firstAccountIndex());
				else setCurrentIndex(-1);
			}
			break;
		}
		default: {
			if(new_account_action) addItem(tr("New account…"), qVariantFromValue(NULL));
			if(new_loan_action) addItem(tr("Paid with loan…"), qVariantFromValue(NULL));
			if(multiple_accounts_action) addItem(tr("Multiple accounts/payments…"), qVariantFromValue(NULL));
			int c_actions = count();
			bool add_secondary_list = false;
			for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
				AssetsAccount *account = *it;
				if(account != exclude_account && (!force_currency || account->currency() == force_currency)) {
					if(i_type >= 100) {
						if(account->accountType() == i_type) {
							if(account->isClosed()) add_secondary_list = true;
							else addItem(account->name(), qVariantFromValue((void*) account));
						}
					} else if(i_type == -3) {
						if(account->accountType() == ASSETS_TYPE_CREDIT_CARD || account->accountType() == ASSETS_TYPE_LIABILITIES) {
							if(account->isClosed()) add_secondary_list = true;
							else addItem(account->name(), qVariantFromValue((void*) account));
						}
					} else if((account->accountType() == ASSETS_TYPE_SECURITIES && !b_exclude_securities) || account->accountType() == ASSETS_TYPE_LIABILITIES || (account == budget->balancingAccount && !b_exclude_balancing) || account->isClosed()) {
						add_secondary_list = true;
					} else if(account->accountType() != ASSETS_TYPE_SECURITIES && account != budget->balancingAccount) {
						addItem(account->name(), qVariantFromValue((void*) account));
					}
				}
			}
			if(i_type == -1) {
				if(count() > firstAccountIndex()) insertSeparator(count());
				for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
					IncomesAccount *account = *it;
					if(account != exclude_account) {
						addItem(account->nameWithParent(), qVariantFromValue((void*) account));
					}
				}
			}
			if(i_type == -2) {
				if(count() > firstAccountIndex()) insertSeparator(count());
				for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
					ExpensesAccount *account = *it;
					if(account != exclude_account) {
						addItem(account->nameWithParent(), qVariantFromValue((void*) account));
					}
				}
			}
			if(c_actions > 0 && count() > c_actions) {insertSeparator(c_actions); c_actions = 0;}
			if(add_secondary_list) {
				if(count() > firstAccountIndex()) insertSeparator(count());
				for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
					AssetsAccount *account = *it;
					if(i_type >= 100) {
						if(account->accountType() == i_type && account->isClosed()) {
							addItem(account->name(), qVariantFromValue((void*) account));
						}
					} else if(i_type == -3) {
						if((account->accountType() == ASSETS_TYPE_CREDIT_CARD || account->accountType() == ASSETS_TYPE_LIABILITIES) && account->isClosed()) {
							addItem(account->name(), qVariantFromValue((void*) account));
						}
					} else if((account->accountType() == ASSETS_TYPE_SECURITIES && !b_exclude_securities) || account->accountType() == ASSETS_TYPE_LIABILITIES || (account == budget->balancingAccount && !b_exclude_balancing) || account->isClosed()) {
						addItem(account->name(), qVariantFromValue((void*) account));
					}
				}
			}
			if(c_actions > 0 && count() > c_actions) insertSeparator(c_actions);
			if(current_account) setCurrentAccount(current_account);
			if(currentIndex() < firstAccountIndex()) {
				if(firstAccountIndex() < count()) setCurrentIndex(firstAccountIndex());
				else setCurrentIndex(-1);
			}
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
		default: {
			EditAssetsAccountDialog *dialog = new EditAssetsAccountDialog(budget, this, tr("New Account"), false, i_type == -3 ? ASSETS_TYPE_LIABILITIES : (i_type >= 100 ? i_type : -1), i_type >= 100);
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
		emit accountSelected(account);
		//emit currentAccountChanged();
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
		if(!block_account_selected) {
			Account *account = NULL;
			if(itemData(index).isValid()) account = (Account*) itemData(index).value<void*>();
			emit accountSelected(account);
		}
		//emit currentAccountChanged();
	}
}
void AccountComboBox::onCurrentIndexChanged(int index) {
	Account *account = NULL;
	if(itemData(index).isValid()) account = (Account*) itemData(index).value<void*>();
	emit currentAccountChanged(account);
}
void AccountComboBox::keyPressEvent(QKeyEvent *e) {
	block_account_selected = true;
	QComboBox::keyPressEvent(e);
	block_account_selected = false;
	if(!e->isAccepted() && (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return)) emit returnPressed();
}

