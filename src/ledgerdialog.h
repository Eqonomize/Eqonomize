/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016-2017 by Hanna Knutsson            *
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


#ifndef LEDGER_DIALOG_H
#define LEDGER_DIALOG_H

#include <qtextstream.h>

#include <QDialog>
#include <QDate>

class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QComboBox;
class QLabel;
class QLineEdit;
class QAction;
class QMenu;
class QFrame;
class QDateEdit;
class EqonomizeValueEdit;

class Eqonomize;
class AssetsAccount;
class Budget;

class LedgerDialog : public QDialog {

	Q_OBJECT

	protected:

		AssetsAccount *account;
		Eqonomize *mainWin;
		Budget *budget;
		bool b_extra, b_ascending;

		QString stat_total_text;

		double d_rec_cl, d_rec_op;
		double d_book_cl, d_book_op;

		int re1, re2;
		int min_width_1, min_width_2;

		QTreeWidget *transactionsView;
		QPushButton *editButton, *removeButton, *joinButton, *splitUpButton, *reconcileButton, *markReconciledButton;
		QDateEdit *reconcileStartEdit, *reconcileEndEdit;
		EqonomizeValueEdit *reconcileOpeningEdit, *reconcileClosingEdit, *reconcileChangeEdit;
		QLabel *reconcileBOpeningLabel, *reconcileBClosingLabel, *reconcileBChangeLabel, *reconcileROpeningLabel, *reconcileRClosingLabel, *reconcileRChangeLabel;
		QFrame *reconcileWidget;
		QPushButton *exportButton, *printButton, *editAccountButton;
		QComboBox *accountCombo;
		QLabel *statLabel;
		QAction *ActionNewDebtInterest, *ActionNewDebtPayment;
		QAction *ActionMarkReconciled, *ActionEdit, *ActionClone, *ActionSplit, *ActionJoin, *ActionDelete, *ActionOpenFile, *ActionEditTimestamp;
		QMenu *headerMenu, *listMenu;
		QLineEdit *searchEdit;
		QPushButton *searchPreviousButton, *searchNextButton;
		QAction *ActionSearch;

		bool exportList(QTextStream &outf, int fileformat, QDate first_date = QDate(), QDate last_date = QDate());
		void accountChanged();

		void updateReconciliationStats(bool = false, bool = false, bool = false);
		void updateReconciliationStatLabels();

		QKeyEvent *key_event;

		void keyPressEvent(QKeyEvent*);

	public:

		LedgerDialog(AssetsAccount *acc, Budget *budg, Eqonomize *parent, QString title, bool extra_parameters, bool do_reconciliation = false);
		~LedgerDialog();

	public slots:

		void saveConfig(bool only_state = false);
		void updateColumnWidths();
		void updateAccounts();

	protected slots:

		void popupHeaderMenu(const QPoint&);
		void popupListMenu(const QPoint&);
		void toggleReconciliation(bool);
		void reconcileTransactions();
		void markAsReconciled();
		void reconcileStartDateChanged(const QDate&);
		void reconcileEndDateChanged(const QDate&);
		void reconcileOpeningChanged(double);
		void reconcileClosingChanged(double);
		void reconcileChangeChanged(double);
		void hideColumn(bool);
		void remove();
		void cloneTransaction();
		void edit();
		void editTimestamp();
		void openAssociatedFile();
		void edit(QTreeWidgetItem*, int);
		void onTransactionSpacePressed(QTreeWidgetItem*);
		void onTransactionReturnPressed(QTreeWidgetItem*);
		void transactionActivated(QTreeWidgetItem*, int);
		void transactionSelectionChanged();
		void updateTransactions(bool = false);
		void newExpense();
		void newIncome();
		void newTransfer();
		void newSplit();
		void newDebtPayment();
		void newDebtInterest();
		void joinTransactions();
		void splitUpTransaction();
		void saveView();
		void printView();
		void accountActivated(int);
		void reject();
		void editAccount();
		void search();
		void searchPrevious();
		void searchChanged(const QString&);
		void ascendingToggled(bool);
		void onFilterSelected(QString);

};

#endif
