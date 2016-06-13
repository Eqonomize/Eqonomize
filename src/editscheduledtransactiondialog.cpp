/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016 by Hanna Knutsson                                        *
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <QPushButton>

#include <klocalizedstring.h>

#include "budget.h"
#include "editscheduledtransactiondialog.h"
#include "recurrence.h"
#include "recurrenceeditwidget.h"
#include "transactioneditwidget.h"


EditScheduledTransactionDialog::EditScheduledTransactionDialog(bool extra_parameters, int transaction_type, Security *security, bool select_security, Budget *budg, QWidget *parent, QString title, Account *account) : KPageDialog(parent), budget(budg), b_extra(extra_parameters) {

	setFaceType(KPageDialog::Tabbed);
	setWindowTitle(title);
	setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	button(QDialogButtonBox::Ok)->setDefault(true);
	button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	setModal(true);
	switch(transaction_type) {
		case TRANSACTION_TYPE_EXPENSE: {
			transactionEditWidget = new TransactionEditWidget(false, b_extra, transaction_type, false, false, security, SECURITY_ALL_VALUES, select_security, budget); 
			trans_page = addPage(transactionEditWidget, i18n("Expense")); 
			trans_page->setHeader(i18n("Expense")); 
			break;
		}
		case TRANSACTION_TYPE_INCOME: {			
			transactionEditWidget = new TransactionEditWidget(false, b_extra, transaction_type, false, false, security, SECURITY_ALL_VALUES, select_security, budget); 
			trans_page = addPage(transactionEditWidget, i18n("Income")); 
			trans_page->setHeader(i18n("Income")); 
			break;
		}
		case TRANSACTION_TYPE_TRANSFER: {			
			transactionEditWidget = new TransactionEditWidget(false, b_extra, transaction_type, false, false, security, SECURITY_ALL_VALUES, select_security, budget); 
			trans_page = addPage(transactionEditWidget, i18n("Transfer")); 
			trans_page->setHeader(i18n("Transfer")); 
			break;
		}
		case TRANSACTION_TYPE_SECURITY_BUY: { 
			transactionEditWidget = new TransactionEditWidget(false, b_extra, transaction_type, false, false, security, SECURITY_ALL_VALUES, select_security, budget); 
			trans_page = addPage(transactionEditWidget, i18n("Security Buy")); 
			trans_page->setHeader(i18n("Security Buy"));
			break;
		}
		case TRANSACTION_TYPE_SECURITY_SELL: {
			transactionEditWidget = new TransactionEditWidget(false, b_extra, transaction_type, false, false, security, SECURITY_ALL_VALUES, select_security, budget); 
			trans_page = addPage(transactionEditWidget, i18n("Security Sell")); 
			trans_page->setHeader(i18n("Security Sell")); 
			break;
		}
	}
	transactionEditWidget->updateAccounts();
	transactionEditWidget->transactionsReset();
	if(account) transactionEditWidget->setAccount(account);
	recurrenceEditWidget = new RecurrenceEditWidget(transactionEditWidget->date(), budget);
	recurrence_page = addPage(recurrenceEditWidget, i18n("Recurrence"));
	recurrence_page->setHeader(i18n("Recurrence"));
	connect(transactionEditWidget, SIGNAL(dateChanged(const QDate&)), recurrenceEditWidget, SLOT(setStartDate(const QDate&)));
	transactionEditWidget->focusDescription();
}

bool EditScheduledTransactionDialog::checkAccounts() {
	return transactionEditWidget->checkAccounts();
}
void EditScheduledTransactionDialog::accept() {
	if(!transactionEditWidget->validValues(true)) {setCurrentPage(trans_page); return;}
	recurrenceEditWidget->setStartDate(transactionEditWidget->date());
	if(!recurrenceEditWidget->validValues()) {setCurrentPage(recurrence_page); return;}
	KPageDialog::accept();
}
void EditScheduledTransactionDialog::setTransaction(Transaction *trans) {
	transactionEditWidget->setTransaction(trans);
	recurrenceEditWidget->setRecurrence(NULL);
}
void EditScheduledTransactionDialog::setScheduledTransaction(ScheduledTransaction *strans) {
	transactionEditWidget->setTransaction(strans->transaction());
	recurrenceEditWidget->setRecurrence(strans->recurrence());
}
ScheduledTransaction *EditScheduledTransactionDialog::createScheduledTransaction() {
	Transaction *trans = transactionEditWidget->createTransaction();
	if(!trans) {setCurrentPage(trans_page); return NULL;}
	recurrenceEditWidget->setStartDate(trans->date());
	return new ScheduledTransaction(budget, trans, recurrenceEditWidget->createRecurrence());
}
bool EditScheduledTransactionDialog::modifyScheduledTransaction(ScheduledTransaction *strans) {
	Transaction *trans = transactionEditWidget->createTransaction();
	if(!trans) {setCurrentPage(trans_page); return false;}
	recurrenceEditWidget->setStartDate(trans->date());
	strans->setRecurrence(recurrenceEditWidget->createRecurrence());
	strans->setTransaction(trans);
	return true;
}
bool EditScheduledTransactionDialog::modifyTransaction(Transaction *trans, Recurrence *&rec) {
	if(!transactionEditWidget->modifyTransaction(trans)) {setCurrentPage(trans_page); return false;}
	recurrenceEditWidget->setStartDate(trans->date());
	rec = recurrenceEditWidget->createRecurrence();
	return true;
}
ScheduledTransaction *EditScheduledTransactionDialog::newScheduledTransaction(int transaction_type, Budget *budg, QWidget *parent, Security *security, bool select_security, Account *account, bool extra_parameters) {
	EditScheduledTransactionDialog *dialog = NULL;
	switch(transaction_type) {
		case TRANSACTION_TYPE_EXPENSE: {dialog = new EditScheduledTransactionDialog(extra_parameters, transaction_type, security, select_security, budg, parent, i18n("New Expense"), account); break;}
		case TRANSACTION_TYPE_INCOME: {
			if(security || select_security) dialog = new EditScheduledTransactionDialog(extra_parameters, transaction_type, security, select_security, budg, parent, i18n("New Dividend"), account);
			else dialog = new EditScheduledTransactionDialog(extra_parameters, transaction_type, security, select_security, budg, parent, i18n("New Income"), account);
			break;
		}
		case TRANSACTION_TYPE_TRANSFER: {dialog = new EditScheduledTransactionDialog(extra_parameters, transaction_type, security, select_security, budg, parent, i18n("New Transfer"), account); break;}
		case TRANSACTION_TYPE_SECURITY_BUY: {dialog = new EditScheduledTransactionDialog(extra_parameters, transaction_type, security, select_security, budg, parent, i18n("New Security Buy"), account); break;}
		case TRANSACTION_TYPE_SECURITY_SELL: {
			dialog = new EditScheduledTransactionDialog(extra_parameters, transaction_type, security, select_security, budg, parent, i18n("New Security Sell"), account);
			//dialog->transactionEditWidget->setMaxSharesDate(QDate::currentDate());
			break;
		}
	}
	ScheduledTransaction *strans = NULL;
	if(dialog->checkAccounts() && dialog->exec() == QDialog::Accepted) {
		strans = dialog->createScheduledTransaction();
	}
	dialog->deleteLater();
	return strans;
}
bool EditScheduledTransactionDialog::editScheduledTransaction(ScheduledTransaction *strans, QWidget *parent, bool select_security, bool extra_parameters) {
	EditScheduledTransactionDialog *dialog = NULL;
	switch(strans->transaction()->type()) {
		case TRANSACTION_TYPE_EXPENSE: {dialog = new EditScheduledTransactionDialog(extra_parameters, strans->transaction()->type(), NULL, false, strans->budget(), parent, i18n("Edit Expense")); break;}
		case TRANSACTION_TYPE_INCOME: {
			if(((Income*) strans->transaction())->security()) dialog = new EditScheduledTransactionDialog(extra_parameters, strans->transaction()->type(), ((Income*) strans->transaction())->security(), select_security, strans->budget(), parent, i18n("Edit Dividend"));
			else dialog = new EditScheduledTransactionDialog(extra_parameters, strans->transaction()->type(), NULL, false, strans->budget(), parent, i18n("Edit Income"));
			break;
		}
		case TRANSACTION_TYPE_TRANSFER: {dialog = new EditScheduledTransactionDialog(extra_parameters, strans->transaction()->type(), NULL, false, strans->budget(), parent, i18n("Edit Transfer")); break;}
		case TRANSACTION_TYPE_SECURITY_BUY: {dialog = new EditScheduledTransactionDialog(extra_parameters, strans->transaction()->type(), ((SecurityTransaction*) strans->transaction())->security(), select_security, strans->budget(), parent, i18n("Edit Securities Bought")); break;}
		case TRANSACTION_TYPE_SECURITY_SELL: {dialog = new EditScheduledTransactionDialog(extra_parameters, strans->transaction()->type(), ((SecurityTransaction*) strans->transaction())->security(), select_security, strans->budget(), parent, i18n("Edit Securities Sold")); break;}
	}
	dialog->setScheduledTransaction(strans);
	bool b = false;
	if(dialog->checkAccounts() && dialog->exec() == QDialog::Accepted) {
		b = dialog->modifyScheduledTransaction(strans);
	}
	dialog->deleteLater();
	return b;
}
bool EditScheduledTransactionDialog::editTransaction(Transaction *trans, Recurrence *&rec, QWidget *parent, bool select_security, bool extra_parameters) {
	EditScheduledTransactionDialog *dialog = NULL;
	switch(trans->type()) {
		case TRANSACTION_TYPE_EXPENSE: {dialog = new EditScheduledTransactionDialog(extra_parameters, trans->type(), NULL, false, trans->budget(), parent, i18n("Edit Expense")); break;}
		case TRANSACTION_TYPE_INCOME: {
			if(((Income*) trans)->security()) dialog = new EditScheduledTransactionDialog(extra_parameters, trans->type(), ((Income*) trans)->security(), select_security, trans->budget(), parent, i18n("Edit Dividend"));
			else dialog = new EditScheduledTransactionDialog(extra_parameters, trans->type(), NULL, false, trans->budget(), parent, i18n("Edit Income"));
			break;
		}
		case TRANSACTION_TYPE_TRANSFER: {dialog = new EditScheduledTransactionDialog(extra_parameters, trans->type(), NULL, false, trans->budget(), parent, i18n("Edit Transfer")); break;}
		case TRANSACTION_TYPE_SECURITY_BUY: {dialog = new EditScheduledTransactionDialog(extra_parameters, trans->type(), ((SecurityTransaction*) trans)->security(), select_security, trans->budget(), parent, i18n("Edit Securities Bought")); break;}
		case TRANSACTION_TYPE_SECURITY_SELL: {dialog = new EditScheduledTransactionDialog(extra_parameters, trans->type(), ((SecurityTransaction*) trans)->security(), select_security, trans->budget(), parent, i18n("Edit Securities Sold")); break;}
	}
	dialog->setTransaction(trans);
	bool b = false;
	if(dialog->checkAccounts() && dialog->exec() == QDialog::Accepted) {
		b = dialog->modifyTransaction(trans, rec);
	}
	dialog->deleteLater();
	return b;
}

#include "editscheduledtransactiondialog.moc"
