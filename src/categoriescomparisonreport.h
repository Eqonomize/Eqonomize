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

#ifndef CATEGORIES_COMPARISON_REPORT_H
#define CATEGORIES_COMPARISON_REPORT_H

#include <QDateTime>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QPushButton;
class QRadioButton;
class QDateEdit;
class QTextEdit;

class AccountsCombo;
class DescriptionsCombo;
class CategoryAccount;
class AssetsAccount;
class Budget;

class CategoriesComparisonReport : public QWidget {

	Q_OBJECT

	public:

		CategoriesComparisonReport(Budget *budg, QWidget *parent, bool extra_parameters);

	protected:

		Budget *budget;
		QString source;
		QDate from_date, to_date;
		CategoryAccount *current_account;
		QString current_tag;
		bool b_extra;
		bool block_display_update;
		int first_source_account_index;

		QTextEdit *htmlview;
		QCheckBox *fromButton;
		QDateEdit *fromEdit, *toEdit;
		QPushButton *nextYearButton, *prevYearButton, *nextMonthButton, *prevMonthButton;
		QPushButton *saveButton, *printButton;
		QCheckBox *valueButton, *dailyButton, *monthlyButton, *yearlyButton, *countButton, *perButton;
		QComboBox *sourceCombo;
		DescriptionsCombo *descriptionCombo, *payeeCombo;
		AccountsCombo *accountCombo;
		QRadioButton *descriptionButton, *payeeButton, *subsButton;
		QRadioButton *monthsButton, *yearsButton, *tagsButton, *totalButton;
		QWidget *payeeDescriptionWidget;

	public slots:

		void resetOptions();
		void updateTransactions();
		void updateAccounts();
		void updateTags();
		void updateDisplay();
		void save();
		void print();
		void saveConfig();
		void fromChanged(const QDate&);
		void toChanged(const QDate&);
		void prevMonth();
		void nextMonth();
		void prevYear();
		void nextYear();
		void sourceChanged(int);
		void descriptionChanged();
		void payeeChanged();
		void descriptionToggled(bool);
		void payeeToggled(bool);
		void subsToggled(bool);
		void columnsToggled(int, bool);

};

#endif
