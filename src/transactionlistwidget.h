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

#ifndef TRANSACTION_LIST_WIDGET_H
#define TRANSACTION_LIST_WIDGET_H

#include <QDateTime>
#include <QLabel>
#include <QTextStream>
#include <QWidget>
#include <QColor>

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
class Transactions;
class TransactionEditWidget;
class TransactionFilterWidget;

class TransactionListWidget : public QWidget {
	
	Q_OBJECT
	
	public:
		
		TransactionListWidget(bool extra_parameters, int transaction_type, Budget *budg, Eqonomize *main_win, QWidget *parent = 0);
		
		void useMultipleCurrencies(bool b);
		void transactionsReset();
		void updateFromAccounts();
		void updateToAccounts();
		void updateAccounts();
		void onDisplay();
		void appendFilterTransaction(Transactions*, bool, ScheduledTransaction* = NULL);
		void setDefaultAccounts();
		QSize minimumSizeHint() const;
		QSize sizeHint() const;
		void setFilter(QDate fromdate, QDate todate, double min = -1.0, double max = -1.0, Account *from_account = NULL, Account *to_account = NULL, QString description = QString(), QString tag = QString(), bool exclude = false, bool exact_match = false);
		bool exportList(QTextStream &outf, int fileformat);
		bool isEmpty();
		void currentDateChanged(const QDate &olddate, const QDate &newdate);
		QByteArray saveState();
		void restoreState(const QByteArray&);
		void createLink();

	protected:

		int transtype;
		int from_col, to_col, comments_col, tags_col, payee_col, quantity_col;
		Budget *budget;
		Eqonomize *mainWin;
		bool b_extra;
		QTabWidget *tabs;
		QTreeWidget *transactionsView;
		QLabel *statLabel;
		QPushButton *addButton, *modifyButton, *removeButton, *clearButton;
		QMenu *listPopupMenu, *headerPopupMenu;
		TransactionFilterWidget *filterWidget;
		TransactionEditWidget *editWidget;
		QLabel *editInfoLabel;
		double current_value, current_quantity;
		Transactions *selected_trans;
		QColor expenseColor, incomeColor, transferColor;
		QAction *ActionSortByCreationTime;
		QKeyEvent *key_event;
		
		void keyPressEvent(QKeyEvent*);
		
	signals:
		
		void accountAdded(Account*);
		void currenciesModified();
		void tagAdded(QString);

	public slots:

		void selectAssociatedFile();
		void openAssociatedFile();
		void editClear();
		void updateStatistics();
		void updateTransactionActions();
		void editTimestamp();
		void editTransaction();
		void newMultiAccountTransaction();
		void newTransactionWithLoan();
		void newRefundRepayment();
		void cloneTransaction();
		void editScheduledTransaction();
		void editSplitTransaction();
		void splitUpTransaction();
		void joinTransactions();
		void modifyTags();
		void onTransactionSplitUp(SplitTransaction*);
		void onTransactionAdded(Transactions*);
		void onTransactionModified(Transactions*, Transactions*);
		void onTransactionRemoved(Transactions*);
		void filterTransactions();
		void currentTransactionChanged(QTreeWidgetItem*);
		void transactionSelectionChanged();
		void filterToActivated(Account*);
		void filterFromActivated(Account*);
		void addTransaction();
		void modifyTransaction();
		void removeTransaction();
		void clearTransaction();
		void removeSplitTransaction();
		void removeScheduledTransaction();
		void addModifyTransaction();
		void popupHeaderMenu(const QPoint&);
		void popupListMenu(const QPoint&);
		void sortByCreationTime(bool = true);
		void showFilter(bool focus_description = false);
		void showEdit();
		void transactionExecuted(QTreeWidgetItem*);
		void currentTabChanged(int);
		void updateClearButton();
		void tagsModified();
		void hideColumn(bool);

};

#endif

