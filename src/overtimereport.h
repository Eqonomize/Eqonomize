/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016-2020 by Hanna Knutsson            *
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

#ifndef OVER_TIME_REPORT_H
#define OVER_TIME_REPORT_H

#include <QWidget>
#include <QPushButton>
#include <QMenu>
#include <QHash>
#include <QList>
#include <QStringList>

class QCheckBox;
class QComboBox;
class QTextEdit;
class QLabel;
class QRadioButton;
class QAction;
class QKeyEvent;

class Account;
class AssetsAccount;
class Budget;
class Transactions;

class AccountsMenu : public QMenu {

	Q_OBJECT

	public:

		AccountsMenu(Budget*, QWidget *parent = NULL, bool shows_assets = false);

		bool accountSelected(Account *account);
		bool allAccountsSelected();
		void setAccountSelected(Account *account, bool selected);
		int selectedAccountsCount();
		QString selectedAccountsText(int type);
		void updateAccounts(int type);

		QList<Account*> selected_accounts;

	protected:

		QHash<Account*, QAction*> account_actions;

		void keyPressEvent(QKeyEvent *e);
		void mouseReleaseEvent(QMouseEvent *e);

		Budget *budget;

		bool b_assets;

	protected slots:

		void accountToggled();

	public slots:

		void selectAll();
		void deselectAll();
		void toggleAll();

	signals:

		void selectedAccountsChanged();

};

class AccountsCombo : public QPushButton {

	Q_OBJECT

	public:

		AccountsCombo(Budget *budg, QWidget *parent = NULL, bool shows_assets = false);

		bool accountSelected(Account *account);
		bool allAccountsSelected();
		void setAccountSelected(Account *account, bool selected);
		void updateAccounts(int type);
		QString selectedAccountsText(int type = 1);
		QList<Account*> &selectedAccounts();
		bool testTransactionRelation(Transactions *trans, bool exclude_securities = false);
		double transactionChange(Transactions *trans, bool exclude_securities = false);

	public slots:

		void resizeAccountsMenu();
		void updateText();
		void selectAll();
		void deselectAll();
		void clear();

	signals:

		void selectedAccountsChanged();

	protected:

		AccountsMenu *accountsMenu;

};

class DescriptionsMenu : public QMenu {

	Q_OBJECT

	public:

		DescriptionsMenu(int type, Budget*, QWidget *parent = NULL, bool show_all = true);

		bool itemSelected(const QString &str);
		bool allItemsSelected();
		void setItemSelected(const QString &str, bool selected);
		int selectedItemsCount();
		QString selectedItemsText(int type);
		void addItem(QString str);
		void updateItems(const QStringList &list);
		void clearItems();
		bool testTransaction(Transactions *trans);
		void setItemType(int type);
		int itemType();

		QStringList selected_items;

	protected:

		QHash<QString, QAction*> item_actions;

		void keyPressEvent(QKeyEvent *e);
		void mouseReleaseEvent(QMouseEvent *e);

		Budget *budget;

		bool b_all;
		int i_type;

	protected slots:

		void itemToggled();

	public slots:

		void selectAll();
		void deselectAll();
		void toggleAll();

	signals:

		void selectedItemsChanged();

};

class DescriptionsCombo : public QPushButton {

	Q_OBJECT

	public:

		DescriptionsCombo(int type, Budget *budg, QWidget *parent = NULL, bool show_all = true);

		bool itemSelected(const QString &str);
		bool allItemsSelected();
		void setItemSelected(const QString &str, bool selected);
		void addItem(QString str);
		void updateItems(const QStringList &list);
		QString selectedItemsText(int type = 1);
		QStringList &selectedItems();
		bool testTransaction(Transactions *trans);
		void setItemType(int type);
		int itemType();

	public slots:

		void resizeDescriptionsMenu();
		void updateText();
		void selectAll();
		void deselectAll();
		void clear();

	signals:

		void selectedItemsChanged();

	protected:

		DescriptionsMenu *itemsMenu;

};

class OverTimeReport : public QWidget {

	Q_OBJECT

	public:

		OverTimeReport(Budget *budg, QWidget *parent);

	protected:

		Budget *budget;
		QString source;

		int current_source;

		QTextEdit *htmlview;
		QComboBox *sourceCombo;
		DescriptionsCombo *tagCombo, *descriptionCombo;
		AccountsCombo *categoryCombo, *accountCombo;
		QPushButton *saveButton, *printButton;
		QCheckBox *valueButton, *dailyButton, *monthlyButton, *yearlyButton, *countButton, *perButton;
		QRadioButton *catsButton, *tagsButton, *totalButton;
		QLabel *columnsLabel;

		bool block_display_update;

	public slots:

		void resetOptions();
		void sourceChanged(int);
		void categoryChanged();
		void tagChanged();
		void descriptionChanged();
		void updateTransactions();
		void updateAccounts();
		void updateTags();
		void updateDisplay();
		void columnsToggled(int, bool);
		void save();
		void print();
		void saveConfig();

};

#endif
