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


#ifndef EDIT_SPLIT_DIALOG_H
#define EDIT_SPLIT_DIALOG_H

#include <QWidget>
#include <QDialog>
#include <QRadioButton>

class QLabel;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QLineEdit;
class QComboBox;
class QDateEdit;

class EqonomizeValueEdit;
class TagButton;

class Account;
class AccountComboBox;
class AssetsAccount;
class CategoryAccount;
class ExpensesAccount;
class Budget;
class MultiItemTransaction;
class MultiAccountTransaction;
class DebtPayment;
class Transaction;
class Transactions;

class EditMultiAccountWidget : public QWidget {

	Q_OBJECT

	protected:

		Budget *budget;
		bool b_expense, b_extra, b_create_accounts;
		
		EqonomizeValueEdit *quantityEdit;
		AccountComboBox *categoryCombo;
		QLineEdit *descriptionEdit, *commentEdit, *fileEdit;
		QTreeWidget *transactionsView;
		QPushButton *editButton, *removeButton;
		QLabel *totalLabel;
		TagButton *tagButton;

		void appendTransaction(Transaction *trans);
		void updateTotalValue();
		CategoryAccount *selectedCategory();

	public:

		EditMultiAccountWidget(Budget *budg, QWidget *parent, bool create_expenses = true, bool extra_parameters = false, bool allow_account_creation = false);
		~EditMultiAccountWidget();

		MultiAccountTransaction *createTransaction();
		void setValues(QString description_string, CategoryAccount *category_account, double quantity_value, QString comment_string);
		void setTransaction(Transactions *transs);
		void setTransaction(MultiAccountTransaction *split, const QDate &date);
		bool validValues();
		bool checkAccounts();
		void focusFirst();
		void reject();
		QDate date();
		
	signals:
	
		void dateChanged(const QDate &date);

	protected slots:
		
		void selectFile();
		void openFile();
		void remove();
		void edit();
		void edit(QTreeWidgetItem*);
		void transactionSelectionChanged();
		void newTransaction();
		void newCategory();
		void newTag();

};

class EditDebtPaymentWidget : public QWidget {

	Q_OBJECT

	protected:

		Budget *budget;
		bool b_create_accounts, b_search;
		
		QDateEdit *dateEdit;
		EqonomizeValueEdit *reductionEdit, *paymentEdit, *interestEdit, *feeEdit;
		QRadioButton *paidInterestButton, *addedInterestButton;
		AccountComboBox *accountCombo, *categoryCombo, *loanCombo;
		QLineEdit *commentEdit, *fileEdit;
		QLabel *paymentLabel, *totalLabel;

		void updateTotalValue();
		AssetsAccount *selectedLoan();
		ExpensesAccount *selectedCategory();
		AssetsAccount *selectedAccount();

	public:

		EditDebtPaymentWidget(Budget *budg, QWidget *parent, AssetsAccount *default_loan = NULL, bool allow_account_creation = false, bool only_interest = false);
		~EditDebtPaymentWidget();

		DebtPayment *createTransaction();
		void setTransaction(DebtPayment *split);
		void setTransaction(DebtPayment *split, const QDate &date);
		bool validValues();
		bool checkAccounts();
		QDate date();
		void focusFirst();
	
	signals:
	
		void dateChanged(const QDate &d);
		void addmodify();

	protected slots:
		
		void selectFile();
		void openFile();
		void accountChanged();
		void loanActivated(Account*);
		void newAccount();
		void newCategory();
		void newLoan();
		void valueChanged();
		void reductionEditingFinished();
		void interestSourceChanged();
		void hasBeenModified();
		void focusDate();
		void reductionFocusNext();
		void accountFocusNext();
		void feeFocusNext();

};

class EditMultiItemWidget : public QWidget {

	Q_OBJECT

	protected:

		Budget *budget;
		bool b_extra, b_create_accounts;
		
		QDateEdit *dateEdit;
		AccountComboBox *accountCombo;
		QLineEdit *descriptionEdit, *payeeEdit, *fileEdit, *commentEdit;
		QTreeWidget *transactionsView;
		QPushButton *editButton, *removeButton;
		QLabel *totalLabel;

		void appendTransaction(Transaction *trans, bool deposit);
		void newTransaction(int transtype, bool select_security = false, bool transfer_to = false, Account *exclude_account = NULL);
		void updateTotalValue();
		AssetsAccount *selectedAccount();

	public:

		EditMultiItemWidget(Budget *budg, QWidget *parent, AssetsAccount *default_account = NULL, bool extra_parameters = false, bool allow_account_creation = false);
		~EditMultiItemWidget();

		MultiItemTransaction *createTransaction();
		void setTransaction(MultiItemTransaction *split);
		void setTransaction(MultiItemTransaction *split, const QDate &date);
		bool validValues();
		bool checkAccounts();
		void reject();
		void focusFirst();
		QDate date();
		
	signals:
	
		void dateChanged(const QDate &date);

	protected slots:
		
		void selectFile();
		void openFile();
		void accountChanged();
		void remove();
		void edit();
		void edit(QTreeWidgetItem*);
		void transactionSelectionChanged();
		void newExpense();
		void newIncome();
		void newDividend();
		void newSecurityBuy();
		void newSecuritySell();
		void newTransferFrom();
		void newTransferTo();
		void newAccount();
		void focusDate();

};

class EditMultiItemDialog : public QDialog {

	Q_OBJECT

	public:

		EditMultiItemDialog(Budget *budg, QWidget *parent, AssetsAccount *default_account = NULL, bool extra_parameters = false, bool allow_account_creation = false);
		~EditMultiItemDialog();
		
		EditMultiItemWidget *editWidget;
		
	protected:
	
		void keyPressEvent(QKeyEvent*);

	protected slots:
		
		void accept();
		void reject();

};

class EditMultiAccountDialog : public QDialog {

	Q_OBJECT

	public:

		EditMultiAccountDialog(Budget *budg, QWidget *parent, bool create_expenses = true, bool extra_parameters = false, bool allow_account_creation = false);
		~EditMultiAccountDialog();
		
		EditMultiAccountWidget *editWidget;

	protected:
	
		void keyPressEvent(QKeyEvent*);

	protected slots:
		
		void accept();
		void reject();

};

class EditDebtPaymentDialog : public QDialog {

	Q_OBJECT

	public:

		EditDebtPaymentDialog(Budget *budg, QWidget *parent, AssetsAccount *default_loan = NULL, bool allow_account_creation = false, bool only_interest = false);
		~EditDebtPaymentDialog();
		
		EditDebtPaymentWidget *editWidget;

	protected:
	
		void keyPressEvent(QKeyEvent*);

	protected slots:
		
		void accept();

};

class EqonomizeRadioButton : public QRadioButton {

	Q_OBJECT

	public:
	
		EqonomizeRadioButton(const QString &text, QWidget *parent);
	
	protected slots:
	
		void keyPressEvent(QKeyEvent *event);
		
	signals:
	
		void returnPressed();

};

#endif

