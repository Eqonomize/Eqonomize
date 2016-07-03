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
class QLineEdit;
class QTextEdit;

class Account;
class AssetsAccount;
class Budget;
class EqonomizeValueEdit;
class ExpensesAccount;
class IncomesAccount;

class EditAssetsAccountDialog : public QDialog {

	Q_OBJECT
	
	protected:

		QLineEdit *nameEdit;
		EqonomizeValueEdit *valueEdit;
		QTextEdit *descriptionEdit;
		QComboBox *typeCombo;
		QCheckBox *budgetButton;
		Budget *budget;
		Account *current_account;
		
	public:
		
		EditAssetsAccountDialog(Budget *budg, QWidget *parent, QString title);

		AssetsAccount *newAccount();
		void modifyAccount(AssetsAccount *account);
		void setAccount(AssetsAccount *account);

	protected slots:

		void typeActivated(int);
		void accept();
		
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
