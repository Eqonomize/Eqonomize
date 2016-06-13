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

#ifndef CATEGORIES_COMPARISON_CHART_H
#define CATEGORIES_COMPARISON_CHART_H

#include <QDateTime>
#include <QResizeEvent>
#include <QWidget>


class QButtonGroup;
class QCheckBox;
class QComboBox;
class QGraphicsScene;
class QGraphicsView;
class QPushButton;

class KDateEdit;

class Account;
class Budget;

class CategoriesComparisonChart : public QWidget {

	Q_OBJECT

	public:

		CategoriesComparisonChart(Budget *budg, QWidget *parent);
		~CategoriesComparisonChart();

	protected:

		Budget *budget;
		QDate from_date, to_date;
		Account *current_account;
		
		QCheckBox *fromButton;
		KDateEdit *fromEdit, *toEdit;
		QPushButton *nextYearButton, *prevYearButton, *nextMonthButton, *prevMonthButton;
		QPushButton *saveButton, *printButton;
		QGraphicsScene *scene;
		QGraphicsView *view;
		QButtonGroup *typeGroup;
		QComboBox *sourceCombo;

		void resizeEvent(QResizeEvent*);

	public slots:

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

};

#endif
