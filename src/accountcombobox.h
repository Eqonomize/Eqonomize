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

#ifndef ACCOUNT_COMBO_BOX_H
#define ACCOUNT_COMBO_BOX_H

#include <QComboBox>

class Account;
class Budget;
class Currency;

class AccountComboBox : public QComboBox {

	Q_OBJECT
	
	protected:
	
		int firstAccountIndex() const;

		int i_type;
		Budget *budget;
		bool new_account_action, new_loan_action, multiple_accounts_action;
		bool b_exclude_securities, b_exclude_balancing;
		bool block_account_selected;
		Account *added_account, *exclude_account;
		
		void keyPressEvent(QKeyEvent *e);
	
	public:
	
		AccountComboBox(int account_type, Budget *budg, bool add_new_account_action = false, bool add_new_loan_action = false, bool add_multiple_accounts_action = false, bool exclude_securities_accounts = true, bool exclude_balancing_account = true, QWidget *parent = Q_NULLPTR);
		~AccountComboBox();
		
		Account *currentAccount() const;
		void setCurrentAccount(Account *account);
		int currentAccountIndex() const;
		void setCurrentAccountIndex(int index);
		void updateAccounts(Account *exclude_account = NULL, Currency *force_currency = NULL);
		bool hasAccount() const;
		Account *createAccount();
		
	signals:
	
		void newLoanRequested();
		void newAccountRequested();
		void multipleAccountsRequested();
		void accountSelected();
		void currentAccountChanged();
		
	protected slots:
	
		void accountActivated(int index);
	
};

#endif
