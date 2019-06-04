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

class QCheckBox;
class QComboBox;
class QPushButton;
class QTextEdit;
class QLabel;
class QRadioButton;

class Account;
class AssetsAccount;
class Budget;

class OverTimeReport : public QWidget {

	Q_OBJECT

	public:

		OverTimeReport(Budget *budg, QWidget *parent);

	protected:

		Budget *budget;
		QString source;

		Account *current_account;
		QString current_description, current_tag;
		int current_source;
		bool has_empty_description;
		
		QTextEdit *htmlview;
		QComboBox *sourceCombo, *categoryCombo, *descriptionCombo, *accountCombo;
		QPushButton *saveButton, *printButton;
		QCheckBox *valueButton, *dailyButton, *monthlyButton, *yearlyButton, *countButton, *perButton;
		QRadioButton *catsButton, *tagsButton, *totalButton;
		QLabel *columnsLabel;
		
		bool block_display_update;

	public slots:

		void resetOptions();
		void sourceChanged(int);
		void categoryChanged(int);
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
