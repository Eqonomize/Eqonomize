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

#ifndef TRANSACTION_FILTER_WIDGET_H
#define TRANSACTION_FILTER_WIDGET_H

#include <QVector>
#include <QWidget>
#include <QDateTime>

class QButtonGroup;
class QCheckBox;
class QLabel;
class QPushButton;
class QRadioButton;
class QCheckBox;

class QComboBox;
class QDateEdit;
class QLineEdit;

class Account;
class Budget;
class Transaction;
class Transactions;
class EqonomizeValueEdit;

class TransactionFilterWidget : public QWidget {

	Q_OBJECT

	public:

		TransactionFilterWidget(bool extra_parameters, int transaction_type, Budget *budg, QWidget *parent = 0);
		~TransactionFilterWidget();
		bool filterTransaction(Transactions *transs, bool checkdate = true);
		void updateFromAccounts();
		void updateToAccounts();
		void updateAccounts();
		void transactionsReset();
		void transactionAdded(Transaction*);
		void transactionModified(Transaction*);
		double countYears();
		double countMonths();
		int countDays();
		QDate startDate();
		QDate endDate();
		void currentDateChanged(const QDate &olddate, const QDate &newdate);
		void focusFirst();
		void updateTags();

		void setFilter(QDate fromdate, QDate todate, double min = -1.0, double max = -1.0, Account *from_account = NULL, Account *to_account = NULL, QString description = QString(), QString payee = QString(), bool exclude = false, bool exact_match = false, bool exclude_subs = false);

	protected:

		QDate firstDate();
		int transtype;
		Budget *budget;
		bool b_extra;
		QComboBox *fromCombo, *toCombo, *tagCombo;
		QCheckBox *minButton, *maxButton, *dateFromButton;
		EqonomizeValueEdit *minEdit, *maxEdit;
		QDateEdit *dateFromEdit, *dateToEdit;
		QLineEdit *descriptionEdit;
		QDate from_date, to_date;
		QRadioButton *includeButton, *excludeButton;
		QCheckBox *exactMatchButton, *excludeSubsButton;
		QPushButton *clearButton;
		QButtonGroup *group;

	protected slots:

		void toChanged(const QDate&);
		void fromChanged(const QDate&);
		void clearFilter();
		void checkEnableClear();
		void onToActivated(int);
		void onFromActivated(int);

	signals:

		void filter();
		void toActivated(Account*);
		void fromActivated(Account*);

};

#endif

