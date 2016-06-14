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


#ifndef EDIT_SPLIT_DIALOG_H
#define EDIT_SPLIT_DIALOG_H

#include <QDialog>

class QLabel;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QLineEdit;
class QComboBox;

class QDateEdit;

class Account;
class AssetsAccount;
class Budget;
class SplitTransaction;
class Transaction;

class EditSplitDialog : public QDialog {

	Q_OBJECT

	protected:

		Budget *budget;
		bool b_extra;
		
		QDateEdit *dateEdit;
		QComboBox *accountCombo;
		QLineEdit *descriptionEdit;
		QTreeWidget *transactionsView;
		QPushButton *editButton, *removeButton;
		QLabel *totalLabel;

		void appendTransaction(Transaction *trans, bool deposit);
		void newTransaction(int transtype, bool select_security = false, bool transfer_to = false, Account *exclude_account = NULL);
		void updateTotalValue();
		AssetsAccount *selectedAccount();

	public:

		EditSplitDialog(Budget *budg, QWidget *parent, AssetsAccount *default_account = NULL, bool extra_parameters = false);
		~EditSplitDialog();

		SplitTransaction *createSplitTransaction();
		void setSplitTransaction(SplitTransaction *split);
		bool validValues();
		bool checkAccounts();

	protected slots:
		
		void accept();
		void reject();
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

};

#endif

