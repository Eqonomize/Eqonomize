/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016 by Hanna Knutsson                 *
 *   hanna_k@fmgirl.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef EDIT_SCHEDULED_TRANSACTION_DIALOG_H
#define EDIT_SCHEDULED_TRANSACTION_DIALOG_H

#include <kpagedialog.h>

class Account;
class Budget;
class Recurrence;
class RecurrenceEditWidget;
class ScheduledTransaction;
class Security;
class Transaction;
class TransactionEditWidget;

class EditScheduledTransactionDialog : public KPageDialog {

	Q_OBJECT
	
	protected:

		Budget *budget;
		bool b_extra;
		RecurrenceEditWidget *recurrenceEditWidget;
		TransactionEditWidget *transactionEditWidget;
		KPageWidgetItem *recurrence_page, *trans_page;
		
	public:
		
		EditScheduledTransactionDialog(bool extra_parameters, int transaction_type, Security *security, bool select_security, Budget *budg, QWidget *parent, QString title, Account *account = NULL);

		bool checkAccounts();
		void setTransaction(Transaction *trans);
		void setScheduledTransaction(ScheduledTransaction *strans);
		ScheduledTransaction *createScheduledTransaction();
		bool modifyScheduledTransaction(ScheduledTransaction *strans);
		bool modifyTransaction(Transaction *trans, Recurrence *&rec);
		static ScheduledTransaction *newScheduledTransaction(int transaction_type, Budget *budg, QWidget *parent, Security *security = NULL, bool select_security = false, Account *account = NULL, bool extra_parameters = false);
		static bool editScheduledTransaction(ScheduledTransaction *strans, QWidget *parent, bool select_security = true, bool extra_parameters = false);
		static bool editTransaction(Transaction *trans, Recurrence *&rec,  QWidget *parent, bool select_security = true, bool extra_parameters = false);

	protected slots:

		void accept();
		
};

#endif
