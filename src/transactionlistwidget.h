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


#ifndef TRANSACTION_LIST_WIDGET_H
#define TRANSACTION_LIST_WIDGET_H

#include <QDateTime>
#include <QLabel>
#include <QTextStream>
#include <QWidget>

class QLabel;
class QMenu;
class QPushButton;
class QTabWidget;
class QTreeWidget;
class QTreeWidgetItem;

class Account;
class Budget;
class Eqonomize;
class ScheduledTransaction;
class SplitTransaction;
class Transaction;
class TransactionEditWidget;
class TransactionFilterWidget;

class TransactionListWidget : public QWidget {
	
	Q_OBJECT
	
	public:
		
		TransactionListWidget(bool extra_parameters, int transaction_type, Budget *budg, Eqonomize *main_win, QWidget *parent = 0);
		void transactionsReset();
		void updateFromAccounts();
		void updateToAccounts();
		void updateAccounts();
		void onDisplay();
		void appendFilterTransaction(Transaction*, bool);
		void appendFilterScheduledTransaction(ScheduledTransaction *strans, bool update_total_value);
		void setCurrentEditToItem(int index);
		void setCurrentEditFromItem(int index);
		int currentEditToItem();
		int currentEditFromItem();
		QSize minimumSizeHint() const;
		QSize sizeHint() const;
		void setFilter(QDate fromdate, QDate todate, double min = -1.0, double max = -1.0, Account *from_account = NULL, Account *to_account = NULL, QString description = QString::null, QString payee = QString::null, bool exclude = false, bool exact_match = false);
		bool exportList(QTextStream &outf, int fileformat);
		bool isEmpty();
		void currentDateChanged(const QDate &olddate, const QDate &newdate);

	protected:

		int transtype;
		int from_col, to_col, comments_col;
		Budget *budget;
		Eqonomize *mainWin;
		bool b_extra;
		QTabWidget *tabs;
		QTreeWidget *transactionsView;
		QLabel *statLabel;
		QPushButton *addButton, *modifyButton, *removeButton;
		QMenu *listPopupMenu;
		TransactionFilterWidget *filterWidget;
		TransactionEditWidget *editWidget;
		QLabel *editInfoLabel;
		double current_value, current_quantity;
		Transaction *selected_trans;

	public slots:

		void editClear();
		void updateStatistics();
		void updateTransactionActions();
		void editTransaction();
		void newRefundRepayment();
		void editScheduledTransaction();
		void editSplitTransaction();
		void splitUpTransaction();
		void joinTransactions();
		void onSplitRemoved(SplitTransaction*);	
		void onTransactionAdded(Transaction*);
		void onTransactionModified(Transaction*, Transaction*);
		void onTransactionRemoved(Transaction*);
		void onScheduledTransactionAdded(ScheduledTransaction*);
		void onScheduledTransactionModified(ScheduledTransaction*, ScheduledTransaction*);
		void onScheduledTransactionRemoved(ScheduledTransaction*);
		void filterTransactions();
		void currentTransactionChanged(QTreeWidgetItem*);
		void transactionSelectionChanged();
		void filterCategoryActivated(int);
		void filterFromActivated(int);
		void addTransaction();
		void modifyTransaction();
		void removeTransaction();
		void removeSplitTransaction();
		void removeScheduledTransaction();
		void addModifyTransaction();
		void popupListMenu(const QPoint&);
		void showFilter();
		void showEdit();
		void transactionExecuted(QTreeWidgetItem*);

};

#endif

