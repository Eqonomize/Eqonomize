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
class QAction;

class Eqonomize;
class AssetsAccount;
class Budget;

class LedgerDialog : public QDialog {
	
	Q_OBJECT
	
	protected:

		AssetsAccount *account;
		Eqonomize *mainWin;
		Budget *budget;
		bool b_extra;
		
		QTreeWidget *transactionsView;
		QPushButton *editButton, *removeButton, *joinButton, *splitUpButton;
		QPushButton *exportButton, *printButton;
		QComboBox *accountCombo;
		QLabel *statLabel;
		QAction *ActionNewDebtInterest, *ActionNewDebtPayment;

		bool exportList(QTextStream &outf, int fileformat, QDate first_date = QDate(), QDate last_date = QDate());
		void accountChanged();
		
	public:
		
		LedgerDialog(AssetsAccount *acc, Eqonomize *parent, QString title, bool extra_parameters);
		~LedgerDialog();
		
	public slots:
	
		void saveConfig();

	protected slots:
		
		void remove();
		void edit();
		void edit(QTreeWidgetItem*);
		void transactionSelectionChanged();
		void updateTransactions();
		void updateAccounts();
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
		
};

#endif
