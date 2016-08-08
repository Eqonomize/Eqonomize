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

#ifndef EDIT_SCHEDULED_TRANSACTION_DIALOG_H
#define EDIT_SCHEDULED_TRANSACTION_DIALOG_H

#include <qdialog.h>

class Account;
class AssetsAccount;
class CategoryAccount;
class Budget;
class Recurrence;
class RecurrenceEditWidget;
class ScheduledTransaction;
class Security;
class Transaction;
class MultiItemTransaction;
class MultiAccountTransaction;
class LoanTransaction;
class TransactionEditWidget;
class EditMultiItemWidget;
class EditMultiAccountWidget;
class EditLoanTransactionWidget;
class QTabWidget;

class EditScheduledTransactionDialog : public QDialog {

	Q_OBJECT
	
	protected:

		Budget *budget;
		bool b_extra;
		RecurrenceEditWidget *recurrenceEditWidget;
		TransactionEditWidget *transactionEditWidget;
		QTabWidget *tabs;
		
	public:
		
		EditScheduledTransactionDialog(bool extra_parameters, int transaction_type, Security *security, bool select_security, Budget *budg, QWidget *parent, QString title, Account *account = NULL, bool allow_account_creation = false);

		bool checkAccounts();
		void setTransaction(Transaction *trans);
		void setScheduledTransaction(ScheduledTransaction *strans);
		ScheduledTransaction *createScheduledTransaction();
		bool modifyScheduledTransaction(ScheduledTransaction *strans);
		bool modifyTransaction(Transaction *trans, Recurrence *&rec);
		static ScheduledTransaction *newScheduledTransaction(int transaction_type, Budget *budg, QWidget *parent, Security *security = NULL, bool select_security = false, Account *account = NULL, bool extra_parameters = false, bool allow_account_creation = false);
		static bool editScheduledTransaction(ScheduledTransaction *strans, QWidget *parent, bool select_security = true, bool extra_parameters = false, bool allow_account_creation = false);
		static bool editTransaction(Transaction *trans, Recurrence *&rec,  QWidget *parent, bool select_security = true, bool extra_parameters = false, bool allow_account_creation = false);

	protected slots:

		void accept();
		
};

class EditScheduledMultiItemDialog : public QDialog {

	Q_OBJECT
	
	protected:

		Budget *budget;
		bool b_extra;
		RecurrenceEditWidget *recurrenceEditWidget;
		EditMultiItemWidget *transactionEditWidget;
		QTabWidget *tabs;
		
	public:
		
		EditScheduledMultiItemDialog(bool extra_parameters, Budget *budg, QWidget *parent, QString title, AssetsAccount *account = NULL, bool allow_account_creation = false);

		bool checkAccounts();
		void setTransaction(MultiItemTransaction *split);
		void setScheduledTransaction(ScheduledTransaction *strans);
		ScheduledTransaction *createScheduledTransaction();
		MultiItemTransaction *createTransaction(Recurrence *&rec);
		static ScheduledTransaction *newScheduledTransaction(Budget *budg, QWidget *parent, AssetsAccount *account = NULL, bool extra_parameters = false, bool allow_account_creation = false);
		static ScheduledTransaction *editScheduledTransaction(ScheduledTransaction *strans, QWidget *parent, bool extra_parameters = false, bool allow_account_creation = false);
		static MultiItemTransaction *editTransaction(MultiItemTransaction *trans, Recurrence *&rec,  QWidget *parent, bool extra_parameters = false, bool allow_account_creation = false);

	protected slots:

		void accept();
		void reject();
		
};

class EditScheduledMultiAccountDialog : public QDialog {

	Q_OBJECT
	
	protected:

		Budget *budget;
		bool b_extra;
		RecurrenceEditWidget *recurrenceEditWidget;
		EditMultiAccountWidget *transactionEditWidget;
		QTabWidget *tabs;
		
	public:
		
		EditScheduledMultiAccountDialog(bool extra_parameters, Budget *budg, QWidget *parent, QString title, bool create_expenses = true, bool allow_account_creation = false);

		bool checkAccounts();
		void setTransaction(MultiAccountTransaction *split);
		void setValues(QString description_string, CategoryAccount *category_account, double quantity_value, QString comment_string);
		void setScheduledTransaction(ScheduledTransaction *strans);
		ScheduledTransaction *createScheduledTransaction();
		MultiAccountTransaction *createTransaction(Recurrence *&rec);
		static ScheduledTransaction *newScheduledTransaction(Budget *budg, QWidget *parent, bool create_expenses = true, bool extra_parameters = false, bool allow_account_creation = false);
		static ScheduledTransaction *newScheduledTransaction(QString description_string, CategoryAccount *category_account, double quantity_value, QString comment_string, Budget *budg, QWidget *parent, bool create_expenses = true, bool extra_parameters = false, bool allow_account_creation = false);
		static ScheduledTransaction *editScheduledTransaction(ScheduledTransaction *strans, QWidget *parent, bool extra_parameters = false, bool allow_account_creation = false);
		static MultiAccountTransaction *editTransaction(MultiAccountTransaction *trans, Recurrence *&rec,  QWidget *parent, bool extra_parameters = false, bool allow_account_creation = false);

	protected slots:

		void accept();
		void reject();
		
};

class EditScheduledLoanTransactionDialog : public QDialog {

	Q_OBJECT
	
	protected:

		Budget *budget;
		bool b_extra;
		RecurrenceEditWidget *recurrenceEditWidget;
		EditLoanTransactionWidget *transactionEditWidget;
		QTabWidget *tabs;
		
	public:
		
		EditScheduledLoanTransactionDialog(bool extra_parameters, Budget *budg, QWidget *parent, QString title, AssetsAccount *loan = NULL, bool allow_account_creation = false);

		bool checkAccounts();
		void setTransaction(LoanTransaction *split);
		void setScheduledTransaction(ScheduledTransaction *strans);
		ScheduledTransaction *createScheduledTransaction();
		LoanTransaction *createTransaction(Recurrence *&rec);
		static ScheduledTransaction *newScheduledTransaction(Budget *budg, QWidget *parent, AssetsAccount *loan = NULL, bool extra_parameters = false, bool allow_account_creation = false);
		static ScheduledTransaction *editScheduledTransaction(ScheduledTransaction *strans, QWidget *parent, bool extra_parameters = false, bool allow_account_creation = false);
		static LoanTransaction *editTransaction(LoanTransaction *trans, Recurrence *&rec,  QWidget *parent, bool extra_parameters = false, bool allow_account_creation = false);

	protected slots:

		void accept();
		void reject();
		
};

#endif
