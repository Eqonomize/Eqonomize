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


#ifndef RECURRENCE_EDIT_WIDGET_H
#define RECURRENCE_EDIT_WIDGET_H

#include <QDateTime>
#include <QStringList>
#include <QWidget>
#include <QDialog>

class QButtonGroup;
class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QListWidget;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QStackedWidget;

class KDateEdit;

class Budget;
class EditRangeDialog;
class EditExceptionsDialog;
class Recurrence;

class RecurrenceEditWidget : public QWidget {

	Q_OBJECT
	
	public:

		RecurrenceEditWidget(const QDate &startdate, Budget *budg, QWidget *parent = 0);
		~RecurrenceEditWidget();
		void setRecurrence(Recurrence *rec);
		Recurrence *createRecurrence();
		bool validValues();
	
	protected:

		QDate date;
		Budget *budget;
		QGroupBox *ruleGroup;
		QStackedWidget *ruleStack;
		QCheckBox *recurrenceButton;
		QComboBox *typeCombo;
		QPushButton *exceptionsButton, *rangeButton;

		QSpinBox *dailyFrequencyEdit;
		
		QCheckBox *weeklyButtons[7];
		QSpinBox *weeklyFrequencyEdit;

		QRadioButton *monthlyOnDayButton, *monthlyOnDayOfWeekButton;
		QComboBox *monthlyDayCombo, *monthlyWeekendCombo, *monthlyWeekCombo, *monthlyDayOfWeekCombo;
		QSpinBox *monthlyFrequencyEdit;

		QSpinBox *yearlyFrequencyEdit, *yearlyDayOfYearEdit, *yearlyDayOfMonthEdit;
		QRadioButton *yearlyOnDayOfYearButton, *yearlyOnDayOfWeekButton, *yearlyOnDayOfMonthButton;
		QComboBox *yearlyMonthCombo_week, *yearlyWeekendCombo_month, *yearlyWeekendCombo_day, *yearlyDayOfWeekCombo, *yearlyWeekCombo, *yearlyMonthCombo;

		EditRangeDialog *rangeDialog;
		EditExceptionsDialog *exceptionsDialog;

	public slots:

		void editExceptions();
		void editRange();
		void setStartDate(const QDate&);
		
};

class EditRangeDialog : public QDialog {

	Q_OBJECT
	
	protected:

		KDateEdit *endDateEdit;
		QSpinBox *fixedCountEdit;
		QRadioButton *foreverButton, *endDateButton, *fixedCountButton;
		QLabel *startLabel;
		QDate date;
		QButtonGroup *rangeButtonGroup;

		QRadioButton *checkedButton;
		QDate endDateEdit_value;
		int fixedCountEdit_value;
	
	public:

		EditRangeDialog(const QDate &startdate, QWidget *parent);
		~EditRangeDialog();
		void setStartDate(const QDate &startdate);
		void setRecurrence(Recurrence *rec);
		int fixedCount();
		QDate endDate();
		bool validValues();
		void saveValues();
		void restoreValues();

	protected slots:

		void accept();
		void reject();
		
};

class EditExceptionsDialog : public QDialog {

	Q_OBJECT
	
	protected:

		KDateEdit *dateEdit;
		QPushButton *changeButton, *addButton, *deleteButton;
		QListWidget *exceptionsList;
		QStringList savedExceptions;
	
	public:

		EditExceptionsDialog(QWidget *parent);
		~EditExceptionsDialog();
		void setRecurrence(Recurrence *rec);
		void modifyExceptions(Recurrence *rec);
		bool validValues();
		void saveValues();
		void restoreValues();

	protected slots:

		void accept();
		void reject();
		void onSelectionChanged();
		void addException();
		void changeException();
		void deleteException();
		
};

#endif

