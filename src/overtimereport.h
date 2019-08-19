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

#ifndef OVER_TIME_REPORT_H
#define OVER_TIME_REPORT_H

#include <QWidget>
#include <QPushButton>
#include <QMenu>
#include <QHash>
#include <QList>

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

class AccountsButton : public QPushButton {
	
	Q_OBJECT
	
	public:

		AccountsButton(Budget *budg, QWidget *parent = NULL, bool shows_assets = false);
		
		bool accountSelected(Account *account);
		bool allAccountsSelected();
		void setAccountSelected(Account *account, bool selected);
		void updateAccounts(int type);
		QString selectedAccountsText(int type = 1);
		QList<Account*> &selectedAccounts();

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

class OverTimeReport : public QWidget {

	Q_OBJECT

	public:

		OverTimeReport(Budget *budg, QWidget *parent);

	protected:

		Budget *budget;
		QString source;

		QString current_description, current_tag;
		int current_source;
		bool has_empty_description;
		
		QTextEdit *htmlview;
		QComboBox *sourceCombo, *tagCombo, *descriptionCombo, *accountCombo;
		AccountsButton *categoryCombo;
		QPushButton *saveButton, *printButton;
		QCheckBox *valueButton, *dailyButton, *monthlyButton, *yearlyButton, *countButton, *perButton;
		QRadioButton *catsButton, *tagsButton, *totalButton;
		QLabel *columnsLabel;
		
		bool block_display_update;

	public slots:

		void resetOptions();
		void sourceChanged(int);
		void categoryChanged();
		void tagChanged(int);
		void descriptionChanged(int);
		void updateTransactions();
		void updateAccounts();
		void updateTags();
		void updateDisplay();
		void columnsToggled(int, bool);
		void save();
		void print();
		void saveConfig();
		AssetsAccount *selectedAccount();

};

#endif
