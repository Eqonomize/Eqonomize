/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016 by Hanna Knutsson                 *
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

class Account;
class Budget;

class CategoriesComparisonReport : public QWidget {

	Q_OBJECT

	public:

		CategoriesComparisonReport(Budget *budg, QWidget *parent, bool extra_parameters);

	protected:

		Budget *budget;
		QString source;
		QDate from_date, to_date;
		Account *current_account;
		QString current_description, current_payee;
		bool has_empty_description, has_empty_payee;
		bool b_extra;
		
		QTextEdit *htmlview;
		QCheckBox *fromButton;
		QDateEdit *fromEdit, *toEdit;
		QPushButton *nextYearButton, *prevYearButton, *nextMonthButton, *prevMonthButton;
		QPushButton *saveButton, *printButton;
		QCheckBox *valueButton, *dailyButton, *monthlyButton, *yearlyButton, *countButton, *perButton;
		QComboBox *sourceCombo, *descriptionCombo, *payeeCombo;
		QRadioButton *descriptionButton, *payeeButton;
		QWidget *payeeDescriptionWidget;

	public slots:

		void resetOptions();
		void updateTransactions();
		void updateAccounts();
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
		void descriptionChanged(int);
		void payeeChanged(int);
		void descriptionToggled(bool);
		void payeeToggled(bool);

};

#endif
