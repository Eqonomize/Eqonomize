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


#ifndef EDIT_ACCOUNT_DIALOGS_H
#define EDIT_ACCOUNT_DIALOGS_H

#include <QDialog>

class QCheckBox;
class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QTextEdit;

class Account;
class AccountComboBox;
class AssetsAccount;
class Budget;
class Currency;
class EqonomizeValueEdit;
class ExpensesAccount;
class IncomesAccount;
class Transaction;

class EditAssetsAccountDialog : public QDialog {

	Q_OBJECT
	
	protected:

		QLineEdit *nameEdit, *maintainerEdit;
		QLabel *valueLabel, *maintainerLabel;
		QDateEdit *dateEdit;
		QRadioButton *initialButton, *transferButton;
		EqonomizeValueEdit *valueEdit;
		QTextEdit *descriptionEdit;
		QComboBox *typeCombo, *currencyCombo;
		QPushButton *editCurrencyButton;
		AccountComboBox *accountCombo;
		QCheckBox *budgetButton, *closedButton;
		Budget *budget;
		AssetsAccount *current_account;
		int prev_currency_index;
		bool b_currencies_edited;
		
	public:
		
		EditAssetsAccountDialog(Budget *budg, QWidget *parent, QString title, bool new_loan = false);

		AssetsAccount *newAccount(Transaction **transfer = NULL);
		void modifyAccount(AssetsAccount *account);
		void setAccount(AssetsAccount *account);
		void updateCurrencyList(Currency*);
		bool currenciesModified();

	protected slots:

		void typeActivated(int);
		void currencyActivated(int);
		void accept();
		void closedToggled(bool);
		void transferToggled(bool);
		void editCurrency();

};

class EditExpensesAccountDialog : public QDialog {

	Q_OBJECT
	
	protected:

		QLineEdit *nameEdit;
		QComboBox *parentCombo;
		EqonomizeValueEdit *budgetEdit;
		QTextEdit *descriptionEdit;
		QCheckBox *budgetButton;
		Budget *budget;
		Account *current_account;
		
	public:
		
		EditExpensesAccountDialog(Budget *budg, ExpensesAccount *default_parent, QWidget *parent, QString title);
		ExpensesAccount *newAccount();
		void modifyAccount(ExpensesAccount *account);
		void setAccount(ExpensesAccount *account);

	protected slots:

		void budgetEnabled(bool);
		void accept();
		
};

class EditIncomesAccountDialog : public QDialog {

	Q_OBJECT
	
	protected:

		QLineEdit *nameEdit;
		QComboBox *parentCombo;
		EqonomizeValueEdit *budgetEdit;
		QTextEdit *descriptionEdit;
		QCheckBox *budgetButton;
		Budget *budget;
		Account *current_account;
		
	public:
		
		EditIncomesAccountDialog(Budget *budg, IncomesAccount *default_parent, QWidget *parent, QString title);
		IncomesAccount *newAccount();
		void modifyAccount(IncomesAccount *account);
		void setAccount(IncomesAccount *account);

	protected slots:

		void budgetEnabled(bool);
		void accept();
		
};

#endif
