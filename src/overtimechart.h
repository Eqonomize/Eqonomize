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

#ifndef OVER_TIME_CHART_H
#define OVER_TIME_CHART_H

#include <QDateTime>
#include <QResizeEvent>
#include <QWidget>

class QButtonGroup;
class QComboBox;
#ifdef QT_CHARTS_LIB
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QBarSet>
#include <QtCharts/QValueAxis>
#include <QtCharts/QAbstractAxis>
class QGraphicsItem;
QT_CHARTS_USE_NAMESPACE
#else
class QGraphicsScene;
class QGraphicsView;
#endif
class QPushButton;
class QRadioButton;

class CategoryAccount;
class Budget;
class EqonomizeMonthSelector;

class OverTimeChart : public QWidget {

	Q_OBJECT

	public:

		OverTimeChart(Budget *budg, QWidget *parent, bool extra_parameters);
		~OverTimeChart();

	protected:

		Budget *budget;
		bool b_extra;

		QComboBox *sourceCombo, *categoryCombo, *descriptionCombo, *payeeCombo;
		EqonomizeMonthSelector *startDateEdit, *endDateEdit;
		QPushButton *saveButton, *printButton;
		QRadioButton *dailyButton, *valueButton, *countButton, *perButton;
#ifdef QT_CHARTS_LIB
		QChartView *view;
		QChart *chart;
		QComboBox *themeCombo;
		QComboBox *typeCombo;
		QAbstractAxis *axisX;
		QValueAxis *axisY;
		QGraphicsItem *point_label;
#else
		QGraphicsScene *scene;
		QGraphicsView *view;
#endif		
		QButtonGroup *valueGroup;
		
		QDate start_date, end_date;

		CategoryAccount *current_account;
		QString current_description, current_payee;
		int current_source;
		bool has_empty_description, has_empty_payee;
#ifndef QT_CHARTS_LIB
		void resizeEvent(QResizeEvent*);
#endif

	public slots:

		void resetOptions();
		void sourceChanged(int);
		void categoryChanged(int);
		void descriptionChanged(int);
		void payeeChanged(int);
		void updateTransactions();
		void updateAccounts();
		void updateDisplay();
		void onFilterSelected(QString);
		void save();
		void print();
		void saveConfig();
		void startMonthChanged(const QDate&);
		void startYearChanged(const QDate&);
		void endMonthChanged(const QDate&);
		void endYearChanged(const QDate&);
		void valueTypeToggled(bool);
#ifdef QT_CHARTS_LIB
		void themeChanged(int);
		void typeChanged(int);
		void legendClicked();
		void onSeriesHovered(const QPointF&, bool);
		void onSeriesHovered(bool, int, QBarSet*);
#endif

};

#endif
