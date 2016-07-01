/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016 by Hanna Knutsson                 *
 *   hanna_k@fmgirl.com                                                    *
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

#ifndef CATEGORIES_COMPARISON_CHART_H
#define CATEGORIES_COMPARISON_CHART_H

#include <QDateTime>
#include <QResizeEvent>
#include <QWidget>

#ifdef QT_CHARTS_LIB
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QPieSeries>
QT_CHARTS_USE_NAMESPACE
#else
class QGraphicsScene;
class QGraphicsView;
#endif
class QButtonGroup;
class QCheckBox;
class QComboBox;
class QPushButton;

class QDateEdit;

class CategoryAccount;
class Budget;

class CategoriesComparisonChart : public QWidget {

	Q_OBJECT

	public:

		CategoriesComparisonChart(Budget *budg, QWidget *parent);
		~CategoriesComparisonChart();

	protected:

		Budget *budget;
		QDate from_date, to_date;
		CategoryAccount *current_account;
		
		QCheckBox *fromButton;
		QDateEdit *fromEdit, *toEdit;
		QPushButton *nextYearButton, *prevYearButton, *nextMonthButton, *prevMonthButton;
		QPushButton *saveButton, *printButton;
#ifdef QT_CHARTS_LIB
		QChartView *view;
		QChart *chart;
		QAbstractSeries *series;
		QComboBox *themeCombo;
#else
		QGraphicsScene *scene;
		QGraphicsView *view;
#endif		
		QButtonGroup *typeGroup;
		QComboBox *sourceCombo;
#ifndef QT_CHARTS_LIB
		void resizeEvent(QResizeEvent*);
#endif

	public slots:

		void resetOptions();
		void updateTransactions();
		void updateAccounts();
		void updateDisplay();
		void onFilterSelected(QString);
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
#ifdef QT_CHARTS_LIB
		void themeChanged(int);
		void sliceHovered(QPieSlice*, bool);
		void sliceClicked(QPieSlice*);
#endif

};

#endif
