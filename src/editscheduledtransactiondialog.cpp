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

#include <QPushButton>
#include <QTabWidget>
#include <QDialogButtonBox>
#include <QVBoxLayout>

#include "budget.h"
#include "editscheduledtransactiondialog.h"
#include "recurrence.h"
#include "recurrenceeditwidget.h"
#include "transactioneditwidget.h"
#include "editsplitdialog.h"


EditScheduledTransactionDialog::EditScheduledTransactionDialog(bool extra_parameters, int transaction_type, Security *security, bool select_security, Budget *budg, QWidget *parent, QString title, Account *account, bool allow_account_creation, bool withloan) : QDialog(parent), budget(budg), b_extra(extra_parameters), b_loan(withloan) {
	
	setWindowTitle(title);
	setModal(true);
	
	QVBoxLayout *box1 = new QVBoxLayout(this);

	tabs = new QTabWidget();
	switch(transaction_type) {
		case TRANSACTION_TYPE_EXPENSE: {
			transactionEditWidget = new TransactionEditWidget(false, b_extra, transaction_type, NULL, false, security, SECURITY_ALL_VALUES, select_security, budget, NULL, allow_account_creation, false, withloan); 
			tabs->addTab(transactionEditWidget, tr("Expense")); 
			break;
		}
		case TRANSACTION_TYPE_INCOME: {			
			transactionEditWidget = new TransactionEditWidget(false, b_extra, transaction_type, NULL, false, security, SECURITY_ALL_VALUES, select_security, budget, NULL, allow_account_creation);
			if(security) tabs->addTab(transactionEditWidget, tr("Dividend")); 
			else tabs->addTab(transactionEditWidget, tr("Income")); 
			break;
		}
		case TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND: {			
			transactionEditWidget = new TransactionEditWidget(false, b_extra, transaction_type, NULL, false, security, SECURITY_ALL_VALUES, select_security, budget, NULL, allow_account_creation);
			tabs->addTab(transactionEditWidget, tr("Reinvested Dividend")); 
			break;
		}
		case TRANSACTION_TYPE_TRANSFER: {			
			transactionEditWidget = new TransactionEditWidget(false, b_extra, transaction_type, NULL, false, security, SECURITY_ALL_VALUES, select_security, budget, NULL, allow_account_creation); 
			tabs->addTab(transactionEditWidget, tr("Transfer")); 
			break;
		}
		case TRANSACTION_TYPE_SECURITY_BUY: { 
			transactionEditWidget = new TransactionEditWidget(false, b_extra, transaction_type, NULL, false, security, SECURITY_ALL_VALUES, select_security, budget, NULL, allow_account_creation); 
			tabs->addTab(transactionEditWidget, tr("Securities Purchase", "Financial security (e.g. stock, mutual fund)")); 
			break;
		}
		case TRANSACTION_TYPE_SECURITY_SELL: {
			transactionEditWidget = new TransactionEditWidget(false, b_extra, transaction_type, NULL, false, security, SECURITY_ALL_VALUES, select_security, budget, NULL, allow_account_creation); 
			tabs->addTab(transactionEditWidget, tr("Securities Sale", "Financial security (e.g. stock, mutual fund)")); 
			break;
		}
	}
	transactionEditWidget->updateAccounts(NULL, NULL, true);
	transactionEditWidget->transactionsReset();
	if(account) transactionEditWidget->setAccount(account);
	recurrenceEditWidget = new RecurrenceEditWidget(transactionEditWidget->date(), budget);
	tabs->addTab(recurrenceEditWidget, tr("Recurrence"));
	
	box1->addWidget(tabs);
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);
	
	connect(transactionEditWidget, SIGNAL(dateChanged(const QDate&)), recurrenceEditWidget, SLOT(setStartDate(const QDate&)));
	transactionEditWidget->focusDescription();
}

bool EditScheduledTransactionDialog::checkAccounts() {
	return transactionEditWidget->checkAccounts();
}
void EditScheduledTransactionDialog::accept() {
	if(!transactionEditWidget->validValues(true)) {tabs->setCurrentIndex(0); return;}
	recurrenceEditWidget->setStartDate(transactionEditWidget->date());
	if(!recurrenceEditWidget->validValues()) {tabs->setCurrentIndex(1); return;}
	QDialog::accept();
}
void EditScheduledTransactionDialog::setTransaction(Transaction *trans) {
	transactionEditWidget->setTransaction(trans);
	recurrenceEditWidget->setRecurrence(NULL);
}
void EditScheduledTransactionDialog::setValues(QString description_value, double value_value, double quantity_value, QDate date_value, Account *from_account_value, Account *to_account_value, QString payee_value, QString comment_value) {
	transactionEditWidget->setValues(description_value, value_value, quantity_value, date_value, from_account_value, to_account_value, payee_value, comment_value);
}
void EditScheduledTransactionDialog::setScheduledTransaction(ScheduledTransaction *strans) {
	transactionEditWidget->setTransaction((Transaction*) strans->transaction());
	recurrenceEditWidget->setRecurrence(strans->recurrence());
}
ScheduledTransaction *EditScheduledTransactionDialog::createScheduledTransaction() {
	Transactions *trans = NULL;
	if(b_loan) trans = transactionEditWidget->createTransactionWithLoan();
	else trans = transactionEditWidget->createTransaction();
	if(!trans) {tabs->setCurrentIndex(0); return NULL;}
	recurrenceEditWidget->setStartDate(trans->date());
	return new ScheduledTransaction(budget, trans, recurrenceEditWidget->createRecurrence());
}
bool EditScheduledTransactionDialog::modifyScheduledTransaction(ScheduledTransaction *strans) {
	Transaction *trans = transactionEditWidget->createTransaction();
	if(!trans) {tabs->setCurrentIndex(0); return false;}
	recurrenceEditWidget->setStartDate(trans->date());
	strans->setRecurrence(recurrenceEditWidget->createRecurrence());
	strans->setTransaction(trans);
	return true;
}
bool EditScheduledTransactionDialog::modifyTransaction(Transaction *trans, Recurrence *&rec) {
	if(!transactionEditWidget->modifyTransaction(trans)) {tabs->setCurrentIndex(0); return false;}
	recurrenceEditWidget->setStartDate(trans->date());
	rec = recurrenceEditWidget->createRecurrence();
	return true;
}
ScheduledTransaction *EditScheduledTransactionDialog::newScheduledTransaction(QString description_value, double value_value, double quantity_value, QDate date_value, Account *from_account_value, Account *to_account_value, QString payee_value, QString comment_value, int transaction_type, Budget *budg, QWidget *parent, Security *security, bool select_security, Account *account, bool extra_parameters, bool allow_account_creation, bool withloan) {
	EditScheduledTransactionDialog *dialog = NULL;
	switch(transaction_type) {
		case TRANSACTION_TYPE_EXPENSE: {dialog = new EditScheduledTransactionDialog(extra_parameters, transaction_type, security, select_security, budg, parent, withloan ? tr("New Expense Payed with Loan") : tr("New Expense"), account, allow_account_creation, withloan); break;}
		case TRANSACTION_TYPE_INCOME: {
			if(security || select_security) dialog = new EditScheduledTransactionDialog(extra_parameters, transaction_type, security, select_security, budg, parent, tr("New Dividend"), account, allow_account_creation);
			else dialog = new EditScheduledTransactionDialog(extra_parameters, transaction_type, security, select_security, budg, parent, tr("New Income"), account, allow_account_creation);
			break;
		}
		case TRANSACTION_TYPE_TRANSFER: {dialog = new EditScheduledTransactionDialog(extra_parameters, transaction_type, security, select_security, budg, parent, tr("New Transfer"), account, allow_account_creation); break;}
		case TRANSACTION_TYPE_SECURITY_BUY: {dialog = new EditScheduledTransactionDialog(extra_parameters, transaction_type, security, select_security, budg, parent, tr("New Securities Purchase", "Financial security (e.g. stock, mutual fund)"), account, allow_account_creation); break;}
		case TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND: {dialog = new EditScheduledTransactionDialog(extra_parameters, transaction_type, security, select_security, budg, parent, tr("New Reinvested Dividend"), account, allow_account_creation); break;}
		case TRANSACTION_TYPE_SECURITY_SELL: {
			dialog = new EditScheduledTransactionDialog(extra_parameters, transaction_type, security, select_security, budg, parent, tr("New Securities Sale", "Financial security (e.g. stock, mutual fund)"), account, allow_account_creation);
			//dialog->transactionEditWidget->setMaxSharesDate(QDate::currentDate());
			break;
		}
	}
	dialog->setValues(description_value, value_value, quantity_value, date_value, from_account_value, to_account_value, payee_value, comment_value);
	ScheduledTransaction *strans = NULL;
	if((allow_account_creation || dialog->checkAccounts()) && dialog->exec() == QDialog::Accepted) {
		strans = dialog->createScheduledTransaction();
	}
	dialog->deleteLater();
	return strans;
}
ScheduledTransaction *EditScheduledTransactionDialog::newScheduledTransaction(int transaction_type, Budget *budg, QWidget *parent, Security *security, bool select_security, Account *account, bool extra_parameters, bool allow_account_creation, bool withloan) {
	EditScheduledTransactionDialog *dialog = NULL;
	switch(transaction_type) {
		case TRANSACTION_TYPE_EXPENSE: {dialog = new EditScheduledTransactionDialog(extra_parameters, transaction_type, security, select_security, budg, parent, withloan ? tr("New Expense Payed with Loan") : tr("New Expense"), account, allow_account_creation, withloan); break;}
		case TRANSACTION_TYPE_INCOME: {
			if(security || select_security) dialog = new EditScheduledTransactionDialog(extra_parameters, transaction_type, security, select_security, budg, parent, tr("New Dividend"), account, allow_account_creation);
			else dialog = new EditScheduledTransactionDialog(extra_parameters, transaction_type, security, select_security, budg, parent, tr("New Income"), account, allow_account_creation);
			break;
		}
		case TRANSACTION_TYPE_TRANSFER: {dialog = new EditScheduledTransactionDialog(extra_parameters, transaction_type, security, select_security, budg, parent, tr("New Transfer"), account, allow_account_creation); break;}
		case TRANSACTION_TYPE_SECURITY_BUY: {dialog = new EditScheduledTransactionDialog(extra_parameters, transaction_type, security, select_security, budg, parent, tr("New Securities Purchase", "Financial security (e.g. stock, mutual fund)"), account, allow_account_creation); break;}
		case TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND: {dialog = new EditScheduledTransactionDialog(extra_parameters, transaction_type, security, select_security, budg, parent, tr("New Reinvested Dividend"), account, allow_account_creation); break;}
		case TRANSACTION_TYPE_SECURITY_SELL: {
			dialog = new EditScheduledTransactionDialog(extra_parameters, transaction_type, security, select_security, budg, parent, tr("New Securities Sale", "Financial security (e.g. stock, mutual fund)"), account, allow_account_creation);
			//dialog->transactionEditWidget->setMaxSharesDate(QDate::currentDate());
			break;
		}
	}
	ScheduledTransaction *strans = NULL;
	if((allow_account_creation || dialog->checkAccounts()) && dialog->exec() == QDialog::Accepted) {
		strans = dialog->createScheduledTransaction();
	}
	dialog->deleteLater();
	return strans;
}
bool EditScheduledTransactionDialog::editScheduledTransaction(ScheduledTransaction *strans, QWidget *parent, bool select_security, bool extra_parameters, bool allow_account_creation) {
	EditScheduledTransactionDialog *dialog = NULL;
	switch(strans->transactiontype()) {
		case TRANSACTION_TYPE_EXPENSE: {dialog = new EditScheduledTransactionDialog(extra_parameters, strans->transactiontype(), NULL, false, strans->budget(), parent, tr("Edit Expense"), NULL, allow_account_creation); break;}
		case TRANSACTION_TYPE_INCOME: {
			if(((Income*) strans->transaction())->subtype() == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND) dialog = new EditScheduledTransactionDialog(extra_parameters, TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND, ((Income*) strans->transaction())->security(), select_security, strans->budget(), parent, tr("Edit Reinvested Dividend"), NULL, allow_account_creation);
			if(((Income*) strans->transaction())->security()) dialog = new EditScheduledTransactionDialog(extra_parameters, strans->transactiontype(), ((Income*) strans->transaction())->security(), select_security, strans->budget(), parent, tr("Edit Dividend"), NULL, allow_account_creation);
			else dialog = new EditScheduledTransactionDialog(extra_parameters, strans->transactiontype(), NULL, false, strans->budget(), parent, tr("Edit Income"), NULL, allow_account_creation);
			break;
		}
		case TRANSACTION_TYPE_TRANSFER: {dialog = new EditScheduledTransactionDialog(extra_parameters, strans->transactiontype(), NULL, false, strans->budget(), parent, tr("Edit Transfer"), NULL, allow_account_creation); break;}
		case TRANSACTION_TYPE_SECURITY_BUY: {dialog = new EditScheduledTransactionDialog(extra_parameters, strans->transactiontype(), ((SecurityTransaction*) strans->transaction())->security(), select_security, strans->budget(), parent, tr("Edit Securities Purchase", "Financial security (e.g. stock, mutual fund)"), NULL, allow_account_creation); break;}
		case TRANSACTION_TYPE_SECURITY_SELL: {dialog = new EditScheduledTransactionDialog(extra_parameters, strans->transactiontype(), ((SecurityTransaction*) strans->transaction())->security(), select_security, strans->budget(), parent, tr("Edit Securities Sale", "Financial security (e.g. stock, mutual fund)"), NULL, allow_account_creation); break;}
	}
	dialog->setScheduledTransaction(strans);
	bool b = false;
	if((allow_account_creation || dialog->checkAccounts()) && dialog->exec() == QDialog::Accepted) {
		b = dialog->modifyScheduledTransaction(strans);
	}
	dialog->deleteLater();
	return b;
}
bool EditScheduledTransactionDialog::editTransaction(Transaction *trans, Recurrence *&rec, QWidget *parent, bool select_security, bool extra_parameters, bool allow_account_creation) {
	EditScheduledTransactionDialog *dialog = NULL;
	switch(trans->type()) {
		case TRANSACTION_TYPE_EXPENSE: {dialog = new EditScheduledTransactionDialog(extra_parameters, trans->type(), NULL, false, trans->budget(), parent, tr("Edit Expense"), NULL, allow_account_creation); break;}
		case TRANSACTION_TYPE_INCOME: {
			if(trans->subtype() == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND) dialog = new EditScheduledTransactionDialog(extra_parameters, TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND, ((SecurityTransaction*) trans)->security(), select_security, trans->budget(), parent, tr("Edit Reinvested Dividend"), NULL, allow_account_creation);
			else if(((Income*) trans)->security()) dialog = new EditScheduledTransactionDialog(extra_parameters, trans->type(), ((Income*) trans)->security(), select_security, trans->budget(), parent, tr("Edit Dividend"), NULL, allow_account_creation);
			else dialog = new EditScheduledTransactionDialog(extra_parameters, trans->type(), NULL, false, trans->budget(), parent, tr("Edit Income"), NULL, allow_account_creation);
			break;
		}
		case TRANSACTION_TYPE_TRANSFER: {dialog = new EditScheduledTransactionDialog(extra_parameters, trans->type(), NULL, false, trans->budget(), parent, tr("Edit Transfer"), NULL, allow_account_creation); break;}
		case TRANSACTION_TYPE_SECURITY_BUY: {dialog = new EditScheduledTransactionDialog(extra_parameters, trans->type(), ((SecurityTransaction*) trans)->security(), select_security, trans->budget(), parent, tr("Edit Securities Purchase", "Financial security (e.g. stock, mutual fund)"), NULL, allow_account_creation); break;}
		case TRANSACTION_TYPE_SECURITY_SELL: {dialog = new EditScheduledTransactionDialog(extra_parameters, trans->type(), ((SecurityTransaction*) trans)->security(), select_security, trans->budget(), parent, tr("Edit Securities Sale", "Financial security (e.g. stock, mutual fund)"), NULL, allow_account_creation); break;}
	}
	dialog->setTransaction(trans);
	bool b = false;
	if((allow_account_creation || dialog->checkAccounts()) && dialog->exec() == QDialog::Accepted) {
		b = dialog->modifyTransaction(trans, rec);
	}
	dialog->deleteLater();
	return b;
}


EditScheduledMultiItemDialog::EditScheduledMultiItemDialog(bool extra_parameters, Budget *budg, QWidget *parent, QString title, AssetsAccount *account, bool allow_account_creation) : QDialog(parent), budget(budg), b_extra(extra_parameters) {
	
	setWindowTitle(title);
	setModal(true);
	
	QVBoxLayout *box1 = new QVBoxLayout(this);
	
	tabs = new QTabWidget();
	
	transactionEditWidget = new EditMultiItemWidget(budget, NULL, account, b_extra, allow_account_creation); 
	tabs->addTab(transactionEditWidget, tr("Transactions")); 

	recurrenceEditWidget = new RecurrenceEditWidget(transactionEditWidget->date(), budget);
	tabs->addTab(recurrenceEditWidget, tr("Recurrence"));
	
	box1->addWidget(tabs);
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);
	
	connect(transactionEditWidget, SIGNAL(dateChanged(const QDate&)), recurrenceEditWidget, SLOT(setStartDate(const QDate&)));
	transactionEditWidget->focusDescription();
}

bool EditScheduledMultiItemDialog::checkAccounts() {
	return transactionEditWidget->checkAccounts();
}
void EditScheduledMultiItemDialog::accept() {
	if(!transactionEditWidget->validValues()) {tabs->setCurrentIndex(0); return;}
	recurrenceEditWidget->setStartDate(transactionEditWidget->date());
	if(!recurrenceEditWidget->validValues()) {tabs->setCurrentIndex(1); return;}
	QDialog::accept();
}
void EditScheduledMultiItemDialog::reject() {
	transactionEditWidget->reject();
	QDialog::reject();
}
void EditScheduledMultiItemDialog::setTransaction(MultiItemTransaction *split) {
	transactionEditWidget->setTransaction(split);
	recurrenceEditWidget->setRecurrence(NULL);
}
void EditScheduledMultiItemDialog::setScheduledTransaction(ScheduledTransaction *strans) {
	transactionEditWidget->setTransaction((MultiItemTransaction*) strans->transaction());
	recurrenceEditWidget->setRecurrence(strans->recurrence());
}
ScheduledTransaction *EditScheduledMultiItemDialog::createScheduledTransaction() {
	MultiItemTransaction *split = transactionEditWidget->createTransaction();
	if(!split) {tabs->setCurrentIndex(0); return NULL;}
	recurrenceEditWidget->setStartDate(split->date());
	return new ScheduledTransaction(budget, split, recurrenceEditWidget->createRecurrence());
}
MultiItemTransaction *EditScheduledMultiItemDialog::createTransaction(Recurrence *&rec) {
	MultiItemTransaction *split = transactionEditWidget->createTransaction();
	if(!split) {tabs->setCurrentIndex(0); return NULL;}
	recurrenceEditWidget->setStartDate(split->date());
	rec = recurrenceEditWidget->createRecurrence();
	return split;
}
ScheduledTransaction *EditScheduledMultiItemDialog::newScheduledTransaction(Budget *budg, QWidget *parent, AssetsAccount *account, bool extra_parameters, bool allow_account_creation) {
	if(!account) {
		for(SplitTransactionList<SplitTransaction*>::const_iterator it = budg->splitTransactions.constEnd(); it != budg->splitTransactions.constBegin();) {
			--it;
			if((*it)->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ITEMS) {
				account = ((MultiItemTransaction*) (*it))->account();
				break;
			}
		}
	}
	EditScheduledMultiItemDialog *dialog = new EditScheduledMultiItemDialog(extra_parameters, budg, parent, tr("New Split Transaction"), account, allow_account_creation);
	ScheduledTransaction *strans = NULL;
	if((allow_account_creation || dialog->checkAccounts()) && dialog->exec() == QDialog::Accepted) {
		strans = dialog->createScheduledTransaction();
	}
	dialog->deleteLater();
	return strans;
}
ScheduledTransaction *EditScheduledMultiItemDialog::editScheduledTransaction(ScheduledTransaction *strans, QWidget *parent, bool extra_parameters, bool allow_account_creation) {
	EditScheduledMultiItemDialog *dialog = new EditScheduledMultiItemDialog(extra_parameters, strans->budget(), parent, tr("Edit Split Transaction"), NULL, allow_account_creation);
	qint64 i_time = strans->timestamp();
	qlonglong i_id = strans->id();
	dialog->setScheduledTransaction(strans);
	strans = NULL;
	if((allow_account_creation || dialog->checkAccounts()) && dialog->exec() == QDialog::Accepted) {
		strans = dialog->createScheduledTransaction();
		strans->setTimestamp(i_time);
		strans->setId(i_id, false);
	}
	dialog->deleteLater();
	return strans;
}
MultiItemTransaction *EditScheduledMultiItemDialog::editTransaction(MultiItemTransaction *split, Recurrence *&rec, QWidget *parent, bool extra_parameters, bool allow_account_creation) {
	EditScheduledMultiItemDialog *dialog = new EditScheduledMultiItemDialog(extra_parameters, split->budget(), parent, tr("Edit Split Transaction"), NULL, allow_account_creation);
	qint64 i_time = split->timestamp();
	qlonglong i_id = split->id();
	dialog->setTransaction(split);
	split = NULL;
	if((allow_account_creation || dialog->checkAccounts()) && dialog->exec() == QDialog::Accepted) {
		split = dialog->createTransaction(rec);
		split->setTimestamp(i_time);
		split->setId(i_id, false);
	}
	dialog->deleteLater();
	return split;
}

EditScheduledMultiAccountDialog::EditScheduledMultiAccountDialog(bool extra_parameters, Budget *budg, QWidget *parent, QString title, bool create_expenses, bool allow_account_creation) : QDialog(parent), budget(budg), b_extra(extra_parameters) {
	
	setWindowTitle(title);
	setModal(true);
	
	QVBoxLayout *box1 = new QVBoxLayout(this);
	
	tabs = new QTabWidget();
	
	transactionEditWidget = new EditMultiAccountWidget(budget, NULL, create_expenses, b_extra, allow_account_creation); 
	tabs->addTab(transactionEditWidget, tr("Transactions")); 

	recurrenceEditWidget = new RecurrenceEditWidget(transactionEditWidget->date(), budget);
	tabs->addTab(recurrenceEditWidget, tr("Recurrence"));
	
	box1->addWidget(tabs);
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);
	
	connect(transactionEditWidget, SIGNAL(dateChanged(const QDate&)), recurrenceEditWidget, SLOT(setStartDate(const QDate&)));
	transactionEditWidget->focusDescription();

}

bool EditScheduledMultiAccountDialog::checkAccounts() {
	return transactionEditWidget->checkAccounts();
}
void EditScheduledMultiAccountDialog::accept() {
	if(!transactionEditWidget->validValues()) {tabs->setCurrentIndex(0); return;}
	recurrenceEditWidget->setStartDate(transactionEditWidget->date());
	if(!recurrenceEditWidget->validValues()) {tabs->setCurrentIndex(1); return;}
	QDialog::accept();
}
void EditScheduledMultiAccountDialog::reject() {
	transactionEditWidget->reject();
	QDialog::reject();
}
void EditScheduledMultiAccountDialog::setTransaction(MultiAccountTransaction *split) {
	transactionEditWidget->setTransaction(split);
	recurrenceEditWidget->setRecurrence(NULL);
}
void EditScheduledMultiAccountDialog::setValues(QString description_string, CategoryAccount *category_account, double quantity_value, QString comment_string) {
	transactionEditWidget->setValues(description_string, category_account, quantity_value, comment_string);
	recurrenceEditWidget->setRecurrence(NULL);
}
void EditScheduledMultiAccountDialog::setScheduledTransaction(ScheduledTransaction *strans) {
	transactionEditWidget->setTransaction((MultiAccountTransaction*) strans->transaction());
	recurrenceEditWidget->setRecurrence(strans->recurrence());
}
ScheduledTransaction *EditScheduledMultiAccountDialog::createScheduledTransaction() {
	MultiAccountTransaction *split = transactionEditWidget->createTransaction();
	if(!split) {tabs->setCurrentIndex(0); return NULL;}
	recurrenceEditWidget->setStartDate(split->date());
	return new ScheduledTransaction(budget, split, recurrenceEditWidget->createRecurrence());
}
MultiAccountTransaction *EditScheduledMultiAccountDialog::createTransaction(Recurrence *&rec) {
	MultiAccountTransaction *split = transactionEditWidget->createTransaction();
	if(!split) {tabs->setCurrentIndex(0); return NULL;}
	recurrenceEditWidget->setStartDate(split->date());
	rec = recurrenceEditWidget->createRecurrence();
	return split;
}
ScheduledTransaction *EditScheduledMultiAccountDialog::newScheduledTransaction(Budget *budg, QWidget *parent, bool create_expenses, bool extra_parameters, bool allow_account_creation) {
	EditScheduledMultiAccountDialog *dialog = new EditScheduledMultiAccountDialog(extra_parameters, budg, parent, create_expenses ? tr("New Expense with Multiple Payments") : tr("New Income with Multiple Payments"), create_expenses, allow_account_creation);
	ScheduledTransaction *strans = NULL;
	if((allow_account_creation || dialog->checkAccounts()) && dialog->exec() == QDialog::Accepted) {
		strans = dialog->createScheduledTransaction();
	}
	dialog->deleteLater();
	return strans;
}
ScheduledTransaction *EditScheduledMultiAccountDialog::newScheduledTransaction(QString description_string, CategoryAccount *category_account, double quantity_value, QString comment_string, Budget *budg, QWidget *parent, bool create_expenses, bool extra_parameters, bool allow_account_creation) {
	EditScheduledMultiAccountDialog *dialog = new EditScheduledMultiAccountDialog(extra_parameters, budg, parent, create_expenses ? tr("New Expense with Multiple Payments") : tr("New Income with Multiple Payments"), create_expenses, allow_account_creation);
	dialog->setValues(description_string, category_account, quantity_value, comment_string);
	ScheduledTransaction *strans = NULL;
	if((allow_account_creation || dialog->checkAccounts()) && dialog->exec() == QDialog::Accepted) {
		strans = dialog->createScheduledTransaction();
	}
	dialog->deleteLater();
	return strans;
}
ScheduledTransaction *EditScheduledMultiAccountDialog::editScheduledTransaction(ScheduledTransaction *strans, QWidget *parent, bool extra_parameters, bool allow_account_creation) {
	EditScheduledMultiAccountDialog *dialog = new EditScheduledMultiAccountDialog(extra_parameters, strans->budget(), parent, ((MultiAccountTransaction*) strans->transaction())->transactiontype() == TRANSACTION_TYPE_EXPENSE ? tr("Edit Expense with Multiple Payments") : tr("Edit Income with Multiple Payments"), ((MultiAccountTransaction*) strans->transaction())->transactiontype() == TRANSACTION_TYPE_EXPENSE, allow_account_creation);
	qint64 i_time = strans->timestamp();
	qlonglong i_id = strans->id();
	dialog->setScheduledTransaction(strans);
	strans = NULL;
	if((allow_account_creation || dialog->checkAccounts()) && dialog->exec() == QDialog::Accepted) {
		strans = dialog->createScheduledTransaction();
		strans->setTimestamp(i_time);
		strans->setId(i_id, false);
	}
	dialog->deleteLater();
	return strans;
}
MultiAccountTransaction *EditScheduledMultiAccountDialog::editTransaction(MultiAccountTransaction *split, Recurrence *&rec, QWidget *parent, bool extra_parameters, bool allow_account_creation) {
	EditScheduledMultiAccountDialog *dialog = new EditScheduledMultiAccountDialog(extra_parameters, split->budget(), parent, split->transactiontype() == TRANSACTION_TYPE_EXPENSE ? tr("Edit Expense with Multiple Payments") : tr("Edit Income with Multiple Payments"), split->transactiontype() == TRANSACTION_TYPE_EXPENSE, allow_account_creation);
	qint64 i_time = split->timestamp();
	qlonglong i_id = split->id();
	dialog->setTransaction(split);
	split = NULL;
	if((allow_account_creation || dialog->checkAccounts()) && dialog->exec() == QDialog::Accepted) {
		split = dialog->createTransaction(rec);
		split->setTimestamp(i_time);
		split->setId(i_id, false);
	}
	dialog->deleteLater();
	return split;
}


EditScheduledDebtPaymentDialog::EditScheduledDebtPaymentDialog(bool extra_parameters, Budget *budg, QWidget *parent, QString title, AssetsAccount *loan, bool allow_account_creation, bool only_interest) : QDialog(parent), budget(budg), b_extra(extra_parameters) {
	
	setWindowTitle(title);
	setModal(true);
	
	QVBoxLayout *box1 = new QVBoxLayout(this);
	
	tabs = new QTabWidget();
	
	transactionEditWidget = new EditDebtPaymentWidget(budget, NULL, loan, allow_account_creation, only_interest); 
	tabs->addTab(transactionEditWidget, tr("Transaction")); 

	recurrenceEditWidget = new RecurrenceEditWidget(transactionEditWidget->date(), budget);
	tabs->addTab(recurrenceEditWidget, tr("Recurrence"));
	
	box1->addWidget(tabs);
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);
	
	connect(transactionEditWidget, SIGNAL(dateChanged(const QDate&)), recurrenceEditWidget, SLOT(setStartDate(const QDate&)));

}

bool EditScheduledDebtPaymentDialog::checkAccounts() {
	return transactionEditWidget->checkAccounts();
}
void EditScheduledDebtPaymentDialog::accept() {
	if(!transactionEditWidget->validValues()) {tabs->setCurrentIndex(0); return;}
	recurrenceEditWidget->setStartDate(transactionEditWidget->date());
	if(!recurrenceEditWidget->validValues()) {tabs->setCurrentIndex(1); return;}
	QDialog::accept();
}
void EditScheduledDebtPaymentDialog::reject() {
	QDialog::reject();
}
void EditScheduledDebtPaymentDialog::setTransaction(DebtPayment *split) {
	transactionEditWidget->setTransaction(split);
	recurrenceEditWidget->setRecurrence(NULL);
}
void EditScheduledDebtPaymentDialog::setScheduledTransaction(ScheduledTransaction *strans) {
	transactionEditWidget->setTransaction((DebtPayment*) strans->transaction());
	recurrenceEditWidget->setRecurrence(strans->recurrence());
}
ScheduledTransaction *EditScheduledDebtPaymentDialog::createScheduledTransaction() {
	DebtPayment *split = transactionEditWidget->createTransaction();
	if(!split) {tabs->setCurrentIndex(0); return NULL;}
	recurrenceEditWidget->setStartDate(split->date());
	return new ScheduledTransaction(budget, split, recurrenceEditWidget->createRecurrence());
}
DebtPayment *EditScheduledDebtPaymentDialog::createTransaction(Recurrence *&rec) {
	DebtPayment *split = transactionEditWidget->createTransaction();
	if(!split) {tabs->setCurrentIndex(0); return NULL;}
	recurrenceEditWidget->setStartDate(split->date());
	rec = recurrenceEditWidget->createRecurrence();
	return split;
}
ScheduledTransaction *EditScheduledDebtPaymentDialog::newScheduledTransaction(Budget *budg, QWidget *parent, AssetsAccount *account, bool extra_parameters, bool allow_account_creation, bool only_interest) {
	EditScheduledDebtPaymentDialog *dialog = new EditScheduledDebtPaymentDialog(extra_parameters, budg, parent, tr("New Debt Payment"), account, allow_account_creation, only_interest);
	ScheduledTransaction *strans = NULL;
	if((allow_account_creation || dialog->checkAccounts()) && dialog->exec() == QDialog::Accepted) {
		strans = dialog->createScheduledTransaction();
	}
	dialog->deleteLater();
	return strans;
}
ScheduledTransaction *EditScheduledDebtPaymentDialog::editScheduledTransaction(ScheduledTransaction *strans, QWidget *parent, bool extra_parameters, bool allow_account_creation) {
	EditScheduledDebtPaymentDialog *dialog = new EditScheduledDebtPaymentDialog(extra_parameters, strans->budget(), parent, tr("Edit Debt Payment"), NULL, allow_account_creation);
	qint64 i_time = strans->timestamp();
	qlonglong i_id = strans->id();
	dialog->setScheduledTransaction(strans);
	strans = NULL;
	if((allow_account_creation || dialog->checkAccounts()) && dialog->exec() == QDialog::Accepted) {
		strans = dialog->createScheduledTransaction();
		strans->setTimestamp(i_time);
		strans->setId(i_id, false);
	}
	dialog->deleteLater();
	return strans;
}
DebtPayment *EditScheduledDebtPaymentDialog::editTransaction(DebtPayment *split, Recurrence *&rec, QWidget *parent, bool extra_parameters, bool allow_account_creation) {
	EditScheduledDebtPaymentDialog *dialog = new EditScheduledDebtPaymentDialog(extra_parameters, split->budget(), parent, tr("Edit Debt Payment"), NULL, allow_account_creation);
	qint64 i_time = split->timestamp();
	qlonglong i_id = split->id();
	dialog->setTransaction(split);
	split = NULL;
	if((allow_account_creation || dialog->checkAccounts()) && dialog->exec() == QDialog::Accepted) {
		split = dialog->createTransaction(rec);
		split->setTimestamp(i_time);
		split->setId(i_id, false);
	}
	dialog->deleteLater();
	return split;
}

