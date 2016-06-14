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

class QComboBox;
class QDateEdit;
class QLineEdit;

class Account;
class Budget;
class Transaction;
class EqonomizeValueEdit;

class TransactionFilterWidget : public QWidget {

	Q_OBJECT

	public:

		TransactionFilterWidget(bool extra_parameters, int transaction_type, Budget *budg, QWidget *parent = 0);
		~TransactionFilterWidget();
		bool filterTransaction(Transaction *trans, bool checkdate = true);
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

		void setFilter(QDate fromdate, QDate todate, double min = -1.0, double max = -1.0, Account *from_account = NULL, Account *to_account = NULL, QString description = QString::null, QString payee = QString::null, bool exclude = false);

	protected:

		QDate firstDate();
		int transtype;
		Budget *budget;
		bool b_extra;
		QVector<Account*> froms, tos;
		QComboBox *fromCombo, *toCombo;
		QCheckBox *minButton, *maxButton, *dateFromButton;
		EqonomizeValueEdit *minEdit, *maxEdit;
		QDateEdit *dateFromEdit, *dateToEdit;
		QLineEdit *descriptionEdit, *payeeEdit;
		QDate from_date, to_date;
		QRadioButton *includeButton, *excludeButton;
		QPushButton *clearButton;
		QButtonGroup *group;

	protected slots:

		void toChanged(const QDate&);
		void fromChanged(const QDate&);
		void clearFilter();
		void checkEnableClear();

	signals:

		void filter();
		void toActivated(int);
		void fromActivated(int);

};

#endif

