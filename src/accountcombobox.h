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

#ifndef ACCOUNT_COMBO_BOX_H
#define ACCOUNT_COMBO_BOX_H

#include <QComboBox>
#include <QListView>
#include <QStyledItemDelegate>

class Account;
class Budget;
class Currency;
class QSortFilterProxyModel;
class QStandardItemModel;

class AccountComboBox : public QComboBox {

	Q_OBJECT

	protected:

		int firstAccountIndex() const;

		int i_type;
		Budget *budget;
		bool new_account_action, new_loan_action, multiple_accounts_action;
		bool b_exclude_securities, b_exclude_balancing;
		bool block_account_selected;
		Account *added_account, *exclude_account;
		QSortFilterProxyModel *filterModel;
		QStandardItemModel *sourceModel;

		void keyPressEvent(QKeyEvent *e);

	public:

		AccountComboBox(int account_type, Budget *budg, bool add_new_account_action = false, bool add_new_loan_action = false, bool add_multiple_accounts_action = false, bool exclude_securities_accounts = true, bool exclude_balancing_account = true, QWidget *parent = NULL);
		~AccountComboBox();

		Account *currentAccount() const;
		void setCurrentAccount(Account *account);
		int currentAccountIndex() const;
		void setCurrentAccountIndex(int index);
		void updateAccounts(Account *exclude_account = NULL, Currency *force_currency = NULL);
		bool hasAccount() const;
		Account *createAccount();
		void hidePopup();

	signals:

		void newLoanRequested();
		void newAccountRequested();
		void multipleAccountsRequested();
		void accountSelected(Account*);
		void currentAccountChanged(Account*);
		void returnPressed();

	public slots:

		void focusAndSelectAll();

	protected slots:

		void accountActivated(int);
		void onCurrentIndexChanged(int);

};

class QComboBoxListViewEq : public QListView {

	Q_OBJECT

	public:
		QComboBoxListViewEq(AccountComboBox *cmb = NULL);
		QString filter_str;
	protected:
		void resizeEvent(QResizeEvent *event);
		QStyleOptionViewItem viewOptions() const;
		void paintEvent(QPaintEvent *e);
		void keyPressEvent(QKeyEvent *e);
	private:
		AccountComboBox *combo;
};

class QComboBoxDelegateEq : public QStyledItemDelegate {

	Q_OBJECT

	public:
		QComboBoxDelegateEq(QObject *parent, QComboBox *cmb);
		static void setSeparator(QAbstractItemModel *model, const QModelIndex &index);
		static bool isSeparator(const QModelIndex &index);
	protected:
		void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
		QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
	private:
		QComboBox *mCombo;
};

#endif
