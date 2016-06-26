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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <QButtonGroup>
#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QListWidget>
#include <QLineEdit>
#include <QObject>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QDateEdit>
#include <QMessageBox>

#include "budget.h"
#include "recurrence.h"
#include "recurrenceeditwidget.h"

EditExceptionsDialog::EditExceptionsDialog(QWidget *parent)  : QDialog(parent, 0) {

	setWindowTitle(tr("Edit Exceptions"));
	setModal(true);

	QVBoxLayout *box1 = new QVBoxLayout(this);
	
	QHBoxLayout *exceptionsLayout = new QHBoxLayout();
	box1->addLayout(exceptionsLayout);

	exceptionsList = new QListWidget(this);
	exceptionsLayout->addWidget(exceptionsList);

	QVBoxLayout *buttonsLayout = new QVBoxLayout();
	exceptionsLayout->addLayout(buttonsLayout);
	dateEdit = new QDateEdit(this);
	dateEdit->setCalendarPopup(true);
	dateEdit->setDate(QDate::currentDate());
	buttonsLayout->addWidget(dateEdit);
	addButton = new QPushButton(tr("Add"), this);
	buttonsLayout->addWidget(addButton);
	changeButton = new QPushButton(tr("Apply"), this);
	changeButton->setEnabled(false);
	buttonsLayout->addWidget(changeButton);
	deleteButton = new QPushButton(tr("Delete"), this);
	deleteButton->setEnabled(false);
	buttonsLayout->addWidget(deleteButton);
	buttonsLayout->addStretch(1);
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Cancel)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Cancel)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);

	connect(exceptionsList, SIGNAL(itemSelectionChanged()), this, SLOT(onSelectionChanged()));
	connect(addButton, SIGNAL(clicked()), this, SLOT(addException()));
	connect(changeButton, SIGNAL(clicked()), this, SLOT(changeException()));
	connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteException()));

}
EditExceptionsDialog::~EditExceptionsDialog() {
}
void EditExceptionsDialog::setRecurrence(Recurrence *rec) {
	exceptionsList->clear();
	if(rec) {
		for(QVector<QDate>::size_type i = 0; i < rec->exceptions.size(); i++) {
			exceptionsList->addItem(QLocale().toString(rec->exceptions[i], QLocale::ShortFormat));
		}
		exceptionsList->sortItems();
	}
	saveValues();
}
void EditExceptionsDialog::modifyExceptions(Recurrence *rec) {
	for(int i = 0; i < exceptionsList->count(); i++) {
		rec->addException(QLocale().toDate(exceptionsList->item(i)->text()));
	}
}
bool EditExceptionsDialog::validValues() {
	return true;
}
void EditExceptionsDialog::saveValues() {
	savedExceptions.clear();
	for(int i = 0; i < exceptionsList->count(); i++) {
		savedExceptions.append(exceptionsList->item(i)->text());
	}
}
void EditExceptionsDialog::restoreValues() {
	exceptionsList->clear();
	exceptionsList->addItems(savedExceptions);
}
void EditExceptionsDialog::accept() {
	if(!validValues()) return;
	saveValues();
	QDialog::accept();
}
void EditExceptionsDialog::reject() {
	restoreValues();
	QDialog::reject();
}
void EditExceptionsDialog::onSelectionChanged() {
	QList<QListWidgetItem*> list = exceptionsList->selectedItems();
	if(!list.isEmpty()) {
		changeButton->setEnabled(true);
		deleteButton->setEnabled(true);
		dateEdit->setDate(QLocale().toDate(list.first()->text()));
	} else {
		changeButton->setEnabled(false);
		deleteButton->setEnabled(false);
	}
}
void EditExceptionsDialog::addException() {
	QDate date = dateEdit->date();
	if(!date.isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		return;
	}
	QString sdate = QLocale().toString(date, QLocale::ShortFormat);
	QList<QListWidgetItem*> list = exceptionsList->findItems(sdate, Qt::MatchExactly);
	if(list.isEmpty()) {
		exceptionsList->addItem(sdate);
		exceptionsList->sortItems();
	}
}
void EditExceptionsDialog::changeException() {
	QList<QListWidgetItem*> list = exceptionsList->selectedItems();
	if(list.isEmpty()) return;
	QDate date = dateEdit->date();
	if(!date.isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		return;
	}
	QString sdate = QLocale().toString(date, QLocale::ShortFormat);
	list.first()->setText(sdate);
	exceptionsList->sortItems();
}
void EditExceptionsDialog::deleteException() {
	QList<QListWidgetItem*> list = exceptionsList->selectedItems();
	if(list.isEmpty()) return;
	delete list.first();
}

EditRangeDialog::EditRangeDialog(const QDate &startdate, QWidget *parent) : QDialog(parent, 0), date(startdate) {

	setWindowTitle(tr("Edit Recurrence Range"));
	setModal(true);

	QVBoxLayout *box1 = new QVBoxLayout(this);
	
	QGridLayout *rangeLayout = new QGridLayout();
	box1->addLayout(rangeLayout);

	startLabel = new QLabel(tr("Begins on: %1").arg(QLocale().toString(startdate)), this);
	rangeLayout->addWidget(startLabel, 0, 0, 1, 3);

	rangeButtonGroup = new QButtonGroup(this);

	foreverButton = new QRadioButton(tr("No ending date"), this);
	rangeButtonGroup->addButton(foreverButton);
	rangeLayout->addWidget(foreverButton, 1, 0, 1, 3);

	fixedCountButton = new QRadioButton(tr("End after"), this);
	rangeButtonGroup->addButton(fixedCountButton);
	rangeLayout->addWidget(fixedCountButton, 2, 0);
	fixedCountEdit = new QSpinBox(this);
	fixedCountEdit->setRange(1, 9999);
	rangeLayout->addWidget(fixedCountEdit, 2, 1);
	rangeLayout ->addWidget(new QLabel(tr("occurrence(s)"), this), 2, 2);

	endDateButton = new QRadioButton(tr("End on"), this);
	rangeButtonGroup->addButton(endDateButton);
	rangeLayout->addWidget(endDateButton, 3, 0);
	endDateEdit = new QDateEdit(startdate, this);
	endDateEdit->setCalendarPopup(true);
	rangeLayout->addWidget(endDateEdit, 3, 1, 1, 2);

	setRecurrence(NULL);
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Cancel)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Cancel)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);

	connect(fixedCountButton, SIGNAL(toggled(bool)), fixedCountEdit, SLOT(setEnabled(bool)));
	connect(endDateButton, SIGNAL(toggled(bool)), endDateEdit, SLOT(setEnabled(bool)));

}

EditRangeDialog::~EditRangeDialog() {
}

void EditRangeDialog::setStartDate(const QDate &startdate) {
	startLabel->setText(tr("Begins on: %1").arg(QLocale().toString(startdate)));
	date = startdate;
	if(!endDateButton->isChecked() && date > endDateEdit->date()) {
		endDateEdit->setDate(date);
	}
}
void EditRangeDialog::setRecurrence(Recurrence *rec) {
	if(rec && rec->fixedOccurrenceCount() > 0) {
		fixedCountButton->setChecked(true);
		fixedCountEdit->setValue(rec->fixedOccurrenceCount());
		endDateEdit->setEnabled(false);
		fixedCountEdit->setEnabled(true);
		endDateEdit->setDate(date);
	} else if(rec && !rec->endDate().isNull()) {
		endDateButton->setChecked(true);
		endDateEdit->setDate(rec->endDate());
		endDateEdit->setEnabled(true);
		fixedCountEdit->setEnabled(false);
		fixedCountEdit->setValue(1);
	} else {
		foreverButton->setChecked(true);
		endDateEdit->setEnabled(false);
		fixedCountEdit->setEnabled(false);
		endDateEdit->setDate(date);
		fixedCountEdit->setValue(1);
	}
	saveValues();
}
int EditRangeDialog::fixedCount() {
	if(fixedCountButton->isChecked()) {
		return fixedCountEdit->value();
	}
	return -1;
}
QDate EditRangeDialog::endDate() {
	if(endDateButton->isChecked()) {
		return endDateEdit->date();
	}
	return QDate();
}
void EditRangeDialog::accept() {
	if(!validValues()) return;
	saveValues();
	QDialog::accept();
}
void EditRangeDialog::reject() {
	restoreValues();
	QDialog::reject();
}
void EditRangeDialog::saveValues() {
	fixedCountEdit_value = fixedCountEdit->value();
	endDateEdit_value = endDateEdit->date();
	if(foreverButton->isChecked()) checkedButton = foreverButton;
	else if(endDateButton->isChecked()) checkedButton = endDateButton;
	else checkedButton = fixedCountButton;
}
void EditRangeDialog::restoreValues() {
	fixedCountEdit->setValue(fixedCountEdit_value);
	endDateEdit->setDate(endDateEdit_value);
	checkedButton->setChecked(true);
}
bool EditRangeDialog::validValues() {
	if(endDateButton->isChecked()) {
		if(!endDateEdit->date().isValid()) {
			QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
			endDateEdit->setFocus();
			endDateEdit->setCurrentSection(QDateTimeEdit::DaySection);
			return false;
		}
		if(endDateEdit->date() < date) {
			QMessageBox::critical(this, tr("Error"), tr("End date before start date."));
			endDateEdit->setFocus();
			endDateEdit->setCurrentSection(QDateTimeEdit::DaySection);
			return false;
		}
	}
	return true;
}


RecurrenceEditWidget::RecurrenceEditWidget(const QDate &startdate, Budget *budg, QWidget *parent) : QWidget(parent), date(startdate), budget(budg) {

	QVBoxLayout *recurrenceLayout = new QVBoxLayout(this);
	recurrenceLayout->setContentsMargins(0, 0, 0, 0);

	recurrenceButton = new QCheckBox(tr("Enable recurrance"), this);
	recurrenceLayout->addWidget(recurrenceButton);

	ruleGroup = new QGroupBox(tr("Recurrence Rule"), this);
	QVBoxLayout *ruleGroup_layout = new QVBoxLayout(ruleGroup);

	typeCombo = new QComboBox(ruleGroup);
	typeCombo->setEditable(false);
	typeCombo->addItem(tr("Daily"));
	typeCombo->addItem(tr("Weekly"));
	typeCombo->addItem(tr("Monthly"));
	typeCombo->addItem(tr("Yearly"));
	typeCombo->setCurrentIndex(2);
	ruleGroup_layout->addWidget(typeCombo);

	ruleStack = new QStackedWidget(ruleGroup);
	ruleGroup_layout->addWidget(ruleStack);
	QWidget *dailyRuleWidget = new QWidget(ruleStack);
	ruleStack->addWidget(dailyRuleWidget);
	QWidget *weeklyRuleWidget = new QWidget(ruleStack);
	ruleStack->addWidget(weeklyRuleWidget);
	QWidget *monthlyRuleWidget = new QWidget(ruleStack);
	ruleStack->addWidget(monthlyRuleWidget);
	QWidget *yearlyRuleWidget = new QWidget(ruleStack);
	ruleStack->addWidget(yearlyRuleWidget);

	QVBoxLayout *dailyRuleLayout = new QVBoxLayout(dailyRuleWidget);
	QHBoxLayout *dailyFrequencyLayout = new QHBoxLayout();
	dailyRuleLayout->addLayout(dailyFrequencyLayout);
	dailyFrequencyLayout->addWidget(new QLabel(tr("Recur every"), dailyRuleWidget));
	dailyFrequencyEdit = new QSpinBox(dailyRuleWidget);
	dailyFrequencyEdit->setRange(1, 9999);
	dailyFrequencyLayout->addWidget(dailyFrequencyEdit);
	dailyFrequencyLayout->addWidget(new QLabel(tr("day(s)"), dailyRuleWidget));
	dailyFrequencyLayout->addStretch(1);
	dailyRuleLayout->addStretch(1);

	QVBoxLayout *weeklyRuleLayout = new QVBoxLayout(weeklyRuleWidget);
	QHBoxLayout *weeklyFrequencyLayout = new QHBoxLayout();
	weeklyRuleLayout->addLayout(weeklyFrequencyLayout);
	weeklyFrequencyLayout->addWidget(new QLabel(tr("Recur every"), weeklyRuleWidget));
	weeklyFrequencyEdit = new QSpinBox(weeklyRuleWidget);
	weeklyFrequencyEdit->setRange(1, 9999);
	weeklyFrequencyLayout->addWidget(weeklyFrequencyEdit);
	weeklyFrequencyLayout->addWidget(new QLabel(tr("week(s) on:"), weeklyRuleWidget));
	weeklyFrequencyLayout->addStretch(1);
	QHBoxLayout *weeklyDaysLayout = new QHBoxLayout();
	weeklyRuleLayout->addLayout(weeklyDaysLayout);
	int weekStart = QLocale().firstDayOfWeek();	
	for(int i = 0; i < 7; ++i ) {
		weeklyButtons[i] = new QCheckBox(QDate::shortDayName(i + 1, QDate::StandaloneFormat), weeklyRuleWidget);
	}
	for(int i = weekStart - 1; i < 7; ++i ) {
		weeklyDaysLayout->addWidget(weeklyButtons[i]);
	}
	for(int i = 0; i < weekStart - 1; ++i ) {
		weeklyDaysLayout->addWidget(weeklyButtons[i]);
	}
	weeklyRuleLayout->addStretch(1);

	QVBoxLayout *monthlyRuleLayout = new QVBoxLayout(monthlyRuleWidget);
	QHBoxLayout *monthlyFrequencyLayout = new QHBoxLayout();
	monthlyRuleLayout->addLayout(monthlyFrequencyLayout);
	monthlyFrequencyLayout->addWidget(new QLabel(tr("Recur every"), monthlyRuleWidget));
	monthlyFrequencyEdit = new QSpinBox(monthlyRuleWidget);
	monthlyFrequencyEdit->setRange(1, 9999);
	monthlyFrequencyLayout->addWidget(monthlyFrequencyEdit);
	monthlyFrequencyLayout->addWidget(new QLabel(tr("month(s), after the start month"), monthlyRuleWidget));
	monthlyFrequencyLayout->addStretch(1);
	QButtonGroup *monthlyButtonGroup = new QButtonGroup(this);
	QGridLayout *monthlyButtonLayout = new QGridLayout();
	monthlyRuleLayout->addLayout(monthlyButtonLayout, 1);
	monthlyOnDayButton = new QRadioButton(tr("Recur on the"), monthlyRuleWidget);
	monthlyOnDayButton->setChecked(true);
	monthlyButtonGroup->addButton(monthlyOnDayButton);
	monthlyButtonLayout->addWidget(monthlyOnDayButton, 0, 0);
	QBoxLayout *monthlyDayLayout = new QHBoxLayout();
	monthlyDayCombo = new QComboBox(monthlyRuleWidget);
	monthlyDayCombo->addItem(tr("1st"));
	monthlyDayCombo->addItem(tr("2nd"));
	monthlyDayCombo->addItem(tr("3rd"));
	monthlyDayCombo->addItem(tr("4th"));
	monthlyDayCombo->addItem(tr("5th"));
	monthlyDayCombo->addItem(tr("6th"));
	monthlyDayCombo->addItem(tr("7th"));
	monthlyDayCombo->addItem(tr("8th"));
	monthlyDayCombo->addItem(tr("9th"));
	monthlyDayCombo->addItem(tr("10th"));
	monthlyDayCombo->addItem(tr("11th"));
	monthlyDayCombo->addItem(tr("12th"));
	monthlyDayCombo->addItem(tr("13th"));
	monthlyDayCombo->addItem(tr("14th"));
	monthlyDayCombo->addItem(tr("15th"));
	monthlyDayCombo->addItem(tr("16th"));
	monthlyDayCombo->addItem(tr("17th"));
	monthlyDayCombo->addItem(tr("18th"));
	monthlyDayCombo->addItem(tr("19th"));
	monthlyDayCombo->addItem(tr("20th"));
	monthlyDayCombo->addItem(tr("21st"));
	monthlyDayCombo->addItem(tr("22nd"));
	monthlyDayCombo->addItem(tr("23rd"));
	monthlyDayCombo->addItem(tr("24th"));
	monthlyDayCombo->addItem(tr("25th"));
	monthlyDayCombo->addItem(tr("26th"));
	monthlyDayCombo->addItem(tr("27th"));
	monthlyDayCombo->addItem(tr("28th"));
	monthlyDayCombo->addItem(tr("29th"));
	monthlyDayCombo->addItem(tr("30th"));
	monthlyDayCombo->addItem(tr("31st"));
	monthlyDayCombo->addItem(tr("Last"));
	monthlyDayCombo->addItem(tr("2nd Last"));
	monthlyDayCombo->addItem(tr("3rd Last"));
	monthlyDayCombo->addItem(tr("4th Last"));
	monthlyDayCombo->addItem(tr("5th Last"));
	monthlyDayCombo->setMaxVisibleItems(7);
	monthlyDayLayout->addWidget(monthlyDayCombo);
	monthlyDayLayout->addWidget(new QLabel(tr("day"), monthlyRuleWidget));
	monthlyWeekendCombo = new QComboBox(monthlyRuleWidget);
	monthlyWeekendCombo->addItem(tr("possibly on weekend"));
	monthlyWeekendCombo->addItem(tr("but before weekend"));
	monthlyWeekendCombo->addItem(tr("but after weekend"));
	monthlyWeekendCombo->addItem(tr("nearest weekend day"));
	monthlyDayLayout->addWidget(monthlyWeekendCombo);
	monthlyDayLayout->addStretch(1);
	monthlyButtonLayout->addLayout(monthlyDayLayout, 0, 1);
	monthlyOnDayOfWeekButton = new QRadioButton(tr("Recur on the"), monthlyRuleWidget);
	monthlyButtonGroup->addButton(monthlyOnDayOfWeekButton);
	monthlyButtonLayout->addWidget(monthlyOnDayOfWeekButton, 1, 0);
	QBoxLayout *monthlyWeekLayout = new QHBoxLayout();
	monthlyWeekCombo = new QComboBox(monthlyRuleWidget);
	monthlyWeekCombo->addItem(tr("1st"));
	monthlyWeekCombo->addItem(tr("2nd"));
	monthlyWeekCombo->addItem(tr("3rd"));
	monthlyWeekCombo->addItem(tr("4th"));
	monthlyWeekCombo->addItem(tr("5th"));
	monthlyWeekCombo->addItem(tr("Last"));
	monthlyWeekCombo->addItem(tr("2nd Last"));
	monthlyWeekCombo->addItem(tr("3rd Last"));
	monthlyWeekCombo->addItem(tr("4th Last"));
	monthlyWeekCombo->addItem(tr("5th Last"));
	monthlyWeekLayout->addWidget(monthlyWeekCombo);
	monthlyDayOfWeekCombo = new QComboBox(monthlyRuleWidget);
	monthlyWeekLayout->addWidget(monthlyDayOfWeekCombo);
	monthlyWeekLayout->addStretch(1);
	monthlyButtonLayout->addLayout(monthlyWeekLayout, 1, 1);
	monthlyRuleLayout->addStretch(1);

	QVBoxLayout *yearlyRuleLayout = new QVBoxLayout(yearlyRuleWidget);
	QHBoxLayout *yearlyFrequencyLayout = new QHBoxLayout();
	yearlyRuleLayout->addLayout(yearlyFrequencyLayout);
	yearlyFrequencyLayout->addWidget(new QLabel(tr("Recur every"), yearlyRuleWidget));
	yearlyFrequencyEdit = new QSpinBox(yearlyRuleWidget);
	yearlyFrequencyEdit->setRange(1, 9999);
	yearlyFrequencyLayout->addWidget(yearlyFrequencyEdit);
	yearlyFrequencyLayout->addWidget(new QLabel(tr("year(s), after the start year"), yearlyRuleWidget));
	yearlyFrequencyLayout->addStretch(1);
	QButtonGroup *yearlyButtonGroup = new QButtonGroup(this);
	QGridLayout *yearlyButtonLayout = new QGridLayout();
	yearlyRuleLayout->addLayout(yearlyButtonLayout, 1);
	yearlyOnDayOfMonthButton = new QRadioButton(tr("Recur on day", "part before XXX of 'Recur on day XXX of month YYY'"), yearlyRuleWidget);
	yearlyButtonGroup->addButton(yearlyOnDayOfMonthButton);
	yearlyOnDayOfMonthButton->setChecked(true);
	yearlyButtonLayout->addWidget(yearlyOnDayOfMonthButton, 0, 0);
	QBoxLayout *yearlyMonthLayout = new QHBoxLayout();
	yearlyDayOfMonthEdit = new QSpinBox(yearlyRuleWidget);
	yearlyDayOfMonthEdit->setRange(1, 31);
	yearlyMonthLayout->addWidget(yearlyDayOfMonthEdit);
	yearlyMonthLayout->addWidget(new QLabel(tr("of", "part between XXX and YYY of 'Recur on day XXX of month YYY'"), yearlyRuleWidget));
	yearlyMonthCombo = new QComboBox(yearlyRuleWidget);
	yearlyMonthLayout->addWidget(yearlyMonthCombo);
	yearlyWeekendCombo_month = new QComboBox(yearlyRuleWidget);
	yearlyWeekendCombo_month->addItem(tr("possibly on weekend"));
	yearlyWeekendCombo_month->addItem(tr("but before weekend"));
	yearlyWeekendCombo_month->addItem(tr("but after weekend"));
	yearlyWeekendCombo_month->addItem(tr("nearest weekend day"));
	yearlyMonthLayout->addWidget(yearlyWeekendCombo_month);
	yearlyMonthLayout->addStretch(1);
	yearlyButtonLayout->addLayout(yearlyMonthLayout, 0, 1);
	yearlyOnDayOfWeekButton = new QRadioButton(tr("On the", "Part before NNN in 'Recur on the NNN. WEEKDAY of MONTH'"), yearlyRuleWidget);
	yearlyButtonGroup->addButton(yearlyOnDayOfWeekButton);
	yearlyButtonLayout->addWidget(yearlyOnDayOfWeekButton, 1, 0);
	QBoxLayout *yearlyWeekLayout = new QHBoxLayout();
	yearlyWeekCombo = new QComboBox(yearlyRuleWidget);
	yearlyWeekCombo->addItem(tr("1st"));
	yearlyWeekCombo->addItem(tr("2nd"));
	yearlyWeekCombo->addItem(tr("3rd"));
	yearlyWeekCombo->addItem(tr("4th"));
	yearlyWeekCombo->addItem(tr("5th"));
	yearlyWeekCombo->addItem(tr("Last"));
	yearlyWeekCombo->addItem(tr("2nd Last"));
	yearlyWeekCombo->addItem(tr("3rd Last"));
	yearlyWeekCombo->addItem(tr("4th Last"));
	yearlyWeekCombo->addItem(tr("5th Last"));
	yearlyWeekLayout->addWidget(yearlyWeekCombo);
	yearlyDayOfWeekCombo = new QComboBox(yearlyRuleWidget);
	yearlyWeekLayout->addWidget(yearlyDayOfWeekCombo);
	yearlyWeekLayout->addWidget(new QLabel(tr("of", "part between WEEKDAY and MONTH in 'Recur on NNN. WEEKDAY of MONTH'"), yearlyRuleWidget));
	yearlyMonthCombo_week = new QComboBox(yearlyRuleWidget);
	yearlyWeekLayout->addWidget(yearlyMonthCombo_week);
	yearlyWeekLayout->addStretch(1);
	yearlyButtonLayout->addLayout(yearlyWeekLayout, 1, 1);
	yearlyOnDayOfYearButton = new QRadioButton(tr("Recur on day #"), yearlyRuleWidget);
	yearlyButtonGroup->addButton(yearlyOnDayOfYearButton);
	yearlyButtonLayout->addWidget(yearlyOnDayOfYearButton, 2, 0);
	QBoxLayout *yearlyDayLayout = new QHBoxLayout();
	yearlyDayOfYearEdit = new QSpinBox(yearlyRuleWidget);
	yearlyDayOfYearEdit->setRange(1, 366);
	yearlyDayLayout->addWidget(yearlyDayOfYearEdit);
	yearlyDayLayout->addWidget(new QLabel(tr(" of the year", "part after NNN of 'Recur on day #NNN of the year'"), yearlyRuleWidget));
	yearlyWeekendCombo_day = new QComboBox(yearlyRuleWidget);
	yearlyWeekendCombo_day->addItem(tr("possibly on weekend"));
	yearlyWeekendCombo_day->addItem(tr("but before weekend"));
	yearlyWeekendCombo_day->addItem(tr("but after weekend"));
	yearlyWeekendCombo_day->addItem(tr("nearest weekend day"));
	yearlyDayLayout->addWidget(yearlyWeekendCombo_day);
	yearlyDayLayout->addStretch(1);
	yearlyButtonLayout->addLayout(yearlyDayLayout, 2, 1);
	yearlyRuleLayout->addStretch(1);

	recurrenceLayout->addWidget(ruleGroup);

	QHBoxLayout *buttonLayout = new QHBoxLayout();
	recurrenceLayout->addLayout(buttonLayout);
	rangeButton = new QPushButton(tr("Range…"), this);
	buttonLayout->addWidget(rangeButton);
	exceptionsButton = new QPushButton(tr("Exceptions…"), this);
	buttonLayout->addWidget(exceptionsButton);

	recurrenceLayout->addStretch(1);

	ruleStack->setCurrentIndex(2);
	recurrenceButton->setChecked(false);
	ruleGroup->setEnabled(false);
	rangeButton->setEnabled(false);
	exceptionsButton->setEnabled(false);

	rangeDialog = new EditRangeDialog(date, this);
	rangeDialog->hide();

	exceptionsDialog = new EditExceptionsDialog(this);
	exceptionsDialog->hide();

	int months = 0;
	QDate date = QDate::currentDate();
	for(int i = 0; i < 10; i++) {
		int months2 = 12;
		if(months2 > months) {
			for(int i2 = months + 1; i2 <= months2; i2++) {
				yearlyMonthCombo_week->addItem(QDate::longMonthName(i2, QDate::StandaloneFormat));
				yearlyMonthCombo->addItem(QDate::longMonthName(i2, QDate::StandaloneFormat));
			}
			months = months2;
		}
		date = date.addYears(1);
	}
	for(int i = 1; i <= 7; i++) {
		yearlyDayOfWeekCombo->addItem(QDate::longDayName(i, QDate::StandaloneFormat));
		monthlyDayOfWeekCombo->addItem(QDate::longDayName(i, QDate::StandaloneFormat));
	}

	connect(typeCombo, SIGNAL(activated(int)), ruleStack, SLOT(setCurrentIndex(int)));
	connect(rangeButton, SIGNAL(clicked()), this, SLOT(editRange()));
	connect(exceptionsButton, SIGNAL(clicked()), this, SLOT(editExceptions()));
	connect(recurrenceButton, SIGNAL(toggled(bool)), ruleGroup, SLOT(setEnabled(bool)));
	connect(recurrenceButton, SIGNAL(toggled(bool)), rangeButton, SLOT(setEnabled(bool)));
	connect(recurrenceButton, SIGNAL(toggled(bool)), exceptionsButton, SLOT(setEnabled(bool)));

}

RecurrenceEditWidget::~RecurrenceEditWidget() {
}

void RecurrenceEditWidget::editExceptions() {
	exceptionsDialog->exec();
	exceptionsDialog->hide();
}
void RecurrenceEditWidget::editRange() {
	rangeDialog->exec();
	rangeDialog->hide();
}

void RecurrenceEditWidget::setRecurrence(Recurrence *rec) {
	rangeDialog->setRecurrence(rec);
	exceptionsDialog->setRecurrence(rec);
	if(!rec) {
		recurrenceButton->setChecked(false);
		recurrenceButton->setChecked(false);
		ruleGroup->setEnabled(false);
		rangeButton->setEnabled(false);
		exceptionsButton->setEnabled(false);
		return;
	}
	switch(rec->type()) {
		case RECURRENCE_TYPE_DAILY: {
			DailyRecurrence *drec = (DailyRecurrence*) rec;
			dailyFrequencyEdit->setValue(drec->frequency());
			typeCombo->setCurrentIndex(0);
			break;
		}
		case RECURRENCE_TYPE_WEEKLY: {
			WeeklyRecurrence *wrec = (WeeklyRecurrence*) rec;
			weeklyFrequencyEdit->setValue(wrec->frequency());
			for(int i = 0; i < 7; i++) {
				weeklyButtons[i]->setChecked(wrec->dayOfWeek(i + 1));
			}
			typeCombo->setCurrentIndex(1);
			break;
		}
		case RECURRENCE_TYPE_MONTHLY: {
			MonthlyRecurrence *mrec = (MonthlyRecurrence*) rec;
			monthlyFrequencyEdit->setValue(mrec->frequency());
			if(mrec->dayOfWeek() > 0) {
				monthlyOnDayOfWeekButton->setChecked(true);
				monthlyDayOfWeekCombo->setCurrentIndex(mrec->dayOfWeek() - 1);
				int week = mrec->week();
				if(week <= 0) week = 6 - week;
				monthlyWeekCombo->setCurrentIndex(week - 1);
			} else {
				monthlyOnDayButton->setChecked(true);
				int day = mrec->day();
				if(day <= 0) day = 32 - day;
				monthlyDayCombo->setCurrentIndex(day - 1);
				monthlyWeekendCombo->setCurrentIndex(mrec->weekendHandling());
			}
			typeCombo->setCurrentIndex(2);
			break;
		}
		case RECURRENCE_TYPE_YEARLY: {
			YearlyRecurrence *yrec = (YearlyRecurrence*) rec;
			yearlyFrequencyEdit->setValue(yrec->frequency());
			if(yrec->dayOfYear() > 0) {
				yearlyOnDayOfYearButton->setChecked(true);
				yearlyDayOfYearEdit->setValue(yrec->dayOfYear());
				yearlyWeekendCombo_day->setCurrentIndex(yrec->weekendHandling());
				yearlyWeekendCombo_month->setCurrentIndex(yrec->weekendHandling());
			} else if(yrec->dayOfWeek() > 0) {
				yearlyOnDayOfWeekButton->setChecked(true);
				yearlyDayOfWeekCombo->setCurrentIndex(yrec->dayOfWeek() - 1);
				int week = yrec->week();
				if(week <= 0) week = 6 - week;
				yearlyWeekCombo->setCurrentIndex(week - 1);
				yearlyMonthCombo_week->setCurrentIndex(yrec->month() - 1);
			} else {
				yearlyOnDayOfMonthButton->setChecked(true);
				yearlyDayOfMonthEdit->setValue(yrec->dayOfMonth());
				yearlyMonthCombo->setCurrentIndex(yrec->month() - 1);
				yearlyWeekendCombo_day->setCurrentIndex(yrec->weekendHandling());
				yearlyWeekendCombo_month->setCurrentIndex(yrec->weekendHandling());
			}
			typeCombo->setCurrentIndex(3);
			break;
		}
	}
	ruleStack->setCurrentIndex(typeCombo->currentIndex());
	recurrenceButton->setChecked(true);
	ruleGroup->setEnabled(true);
	rangeButton->setEnabled(true);
	exceptionsButton->setEnabled(true);
}
Recurrence *RecurrenceEditWidget::createRecurrence() {
	if(!recurrenceButton->isChecked() || !validValues()) return NULL;
	switch(typeCombo->currentIndex()) {
		case 0: {
			DailyRecurrence *rec = new DailyRecurrence(budget);
			rec->set(date, rangeDialog->endDate(), dailyFrequencyEdit->value(), rangeDialog->fixedCount());
			exceptionsDialog->modifyExceptions(rec);
			return rec;
		}
		case 1: {
			WeeklyRecurrence *rec = new WeeklyRecurrence(budget);
			rec->set(date, rangeDialog->endDate(), weeklyButtons[0]->isChecked(), weeklyButtons[1]->isChecked(), weeklyButtons[2]->isChecked(), weeklyButtons[3]->isChecked(), weeklyButtons[4]->isChecked(), weeklyButtons[5]->isChecked(), weeklyButtons[6]->isChecked(), weeklyFrequencyEdit->value(), rangeDialog->fixedCount());
			exceptionsDialog->modifyExceptions(rec);
			return rec;
		}
		case 2: {
			MonthlyRecurrence *rec = new MonthlyRecurrence(budget);
			if(monthlyOnDayButton->isChecked()) {
				int day = monthlyDayCombo->currentIndex() + 1;
				if(day > 31) day = 32 - day;
				rec->setOnDay(date, rangeDialog->endDate(), day, (WeekendHandling) monthlyWeekendCombo->currentIndex(), monthlyFrequencyEdit->value(), rangeDialog->fixedCount());
			} else {
				int week = monthlyWeekCombo->currentIndex() + 1;
				if(week > 5) week = 6 - week;
				rec->setOnDayOfWeek(date, rangeDialog->endDate(), monthlyDayOfWeekCombo->currentIndex() + 1, week, monthlyFrequencyEdit->value(), rangeDialog->fixedCount());
			}
			exceptionsDialog->modifyExceptions(rec);
			return rec;
		}
		case 3: {
			YearlyRecurrence *rec = new YearlyRecurrence(budget);
			if(yearlyOnDayOfMonthButton->isChecked()) {
				rec->setOnDayOfMonth(date, rangeDialog->endDate(), yearlyMonthCombo->currentIndex() + 1, yearlyDayOfMonthEdit->value(), (WeekendHandling) yearlyWeekendCombo_month->currentIndex(), yearlyFrequencyEdit->value(), rangeDialog->fixedCount());
			} else if(yearlyOnDayOfWeekButton->isChecked()) {
				int week = yearlyWeekCombo->currentIndex() + 1;
				if(week > 5) week = 6 - week;
				rec->setOnDayOfWeek(date, rangeDialog->endDate(), yearlyMonthCombo_week->currentIndex() + 1, yearlyDayOfWeekCombo->currentIndex() + 1, week, yearlyFrequencyEdit->value(), rangeDialog->fixedCount());
			} else {
				rec->setOnDayOfYear(date, rangeDialog->endDate(), yearlyDayOfYearEdit->value(), (WeekendHandling) yearlyWeekendCombo_day->currentIndex(), yearlyFrequencyEdit->value(), rangeDialog->fixedCount());
			}
			exceptionsDialog->modifyExceptions(rec);
			return rec;
		}
	}
	return NULL;
}
bool RecurrenceEditWidget::validValues() {
	if(!recurrenceButton->isChecked()) return true;
	switch(typeCombo->currentIndex()) {
		case 0: {
			break;
		}
		case 1: {
			bool b = false;
			for(int i = 0; i < 7; i++) {
				if(weeklyButtons[i]->isChecked()) {
					b = true;
					break;
				}
			}
			if(!b) {
				QMessageBox::critical(this, tr("Error"), tr("No day of week selected for weekly recurrence."));
				weeklyButtons[0]->setFocus();
				return false;
			}
			break;
		}
		case 2: {
			int i_frequency = monthlyFrequencyEdit->value();
			if(i_frequency % 12 == 0) {					
				int i_dayofmonth = monthlyDayCombo->currentIndex() + 1;
				int i_month = date.month();
				if(i_dayofmonth <= 31 && ((i_month == 2 && i_dayofmonth > 29) || (i_dayofmonth > 30 && (i_month == 4 || i_month == 6 || i_month == 9 || i_month == 11)))) {
					QMessageBox::critical(this, tr("Error"), tr("Selected day will never occur with selected frequency and start date."));
					monthlyDayCombo->setFocus();
					return false;
				}
			}
			break;
		}
		case 3: {			
			if(yearlyOnDayOfMonthButton->isChecked()) {
				int i_frequency = yearlyFrequencyEdit->value();
				int i_dayofmonth = yearlyDayOfMonthEdit->value();
				int i_month = yearlyMonthCombo->currentIndex() + 1;
				if(IS_GREGORIAN_CALENDAR) {
					if((i_month == 2 && i_dayofmonth > 29) || (i_dayofmonth > 30 && (i_month == 4 || i_month == 6 || i_month == 9 || i_month == 11))) {
						QMessageBox::critical(this, tr("Error"), tr("Selected day does not exist in selected month."));
						yearlyDayOfMonthEdit->setFocus();
						return false;
					} else if(i_month != 2 || i_dayofmonth < 29) {
						break;
					}
				}
				QDate nextdate;
				nextdate.setDate(date.year(), i_month, 1);
				if(i_dayofmonth > nextdate.daysInMonth()) {
					int i = 10;
					do {
						if(i == 0) {
							QMessageBox::critical(this, tr("Error"), tr("Selected day will never occur with selected frequency and start date."));
							yearlyDayOfMonthEdit->setFocus();
							return false;
						}
						nextdate = nextdate.addYears(i_frequency);
						nextdate.setDate(nextdate.year(), i_month, 1);
						i--;
					} while(i_dayofmonth > nextdate.daysInMonth());
				}
			} else if(yearlyOnDayOfYearButton->isChecked()) {
				int i_frequency = yearlyFrequencyEdit->value();
				int i_dayofyear = yearlyDayOfYearEdit->value();
				if(i_dayofyear > date.daysInYear()) {
					QDate nextdate = date;
					int i = 10;
					do {
						if(i == 0) {
							QMessageBox::critical(this, tr("Error"), tr("Selected day will never occur with selected frequency and start date."));
							yearlyDayOfYearEdit->setFocus();
							return false;
						}
						nextdate = nextdate.addYears(i_frequency);
						i--;
					} while(i_dayofyear > nextdate.daysInYear());
				}
			}
			break;
		}
	}
	if(!rangeDialog->validValues()) return false;
	if(!exceptionsDialog->validValues()) return false;
	return true;
}
void RecurrenceEditWidget::setStartDate(const QDate &startdate) {
	if(!startdate.isValid()) return;
	date = startdate;
	rangeDialog->setStartDate(date);
}

