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


#ifndef LEDGER_DIALOG_H
#define LEDGER_DIALOG_H

#include <qtextstream.h>

#include <qdialog.h>

class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QComboBox;

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

		bool exportList(QTextStream &outf, int fileformat);
		
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
		void joinTransactions();
		void splitUpTransaction();
		void saveView();
		void printView();
		void accountChanged(int);
		void reject();
		
};

#endif
