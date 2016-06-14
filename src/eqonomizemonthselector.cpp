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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <QHBoxLayout>
#include <QLayout>
#include <QSpinBox>
#include <QObject>
#include <QComboBox>

#include "budget.h"

#include "eqonomizemonthselector.h"

EqonomizeMonthSelector::EqonomizeMonthSelector(QWidget *parent) : QWidget(parent) {	
	d_date = QDate::currentDate();
	d_date.setDate(d_date.year(), d_date.month(), 1);
	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	monthCombo = new QComboBox(this);
	monthCombo->setEditable(false);
	layout->addWidget(monthCombo);
	yearEdit = new QSpinBox(this);
	yearEdit->setRange(0, 4000);
	yearEdit->setValue(d_date.year());
	layout->addWidget(yearEdit);
	updateMonths();
	monthCombo->setCurrentIndex(d_date.month() - 1);
	connect(yearEdit, SIGNAL(valueChanged(int)), this, SLOT(onYearChanged(int)));
	connect(monthCombo, SIGNAL(activated(int)), this, SLOT(onMonthChanged(int)));
}
EqonomizeMonthSelector::~EqonomizeMonthSelector() {}
		
QDate EqonomizeMonthSelector::date() const {return d_date;}

void EqonomizeMonthSelector::onYearChanged(int year) {	
	d_date.setDate(year, d_date.month(), 1);
	updateMonths();
	emit yearChanged(d_date);
	emit dateChanged(d_date);
}
void EqonomizeMonthSelector::onMonthChanged(int month) {	
	d_date.setDate(d_date.year(), month + 1, 1);
	emit monthChanged(d_date);
	emit dateChanged(d_date);
}

void EqonomizeMonthSelector::setDate(const QDate &newdate) {	
	if(newdate.isValid()) {
		d_date.setDate(newdate.year(), newdate.month(), 1);
		yearEdit->blockSignals(true);
		yearEdit->setValue(newdate.year());
		yearEdit->blockSignals(false);
		monthCombo->blockSignals(true);
		updateMonths();
		monthCombo->setCurrentIndex(d_date.month() - 1);
		monthCombo->blockSignals(false);
	}
}

void EqonomizeMonthSelector::updateMonths() {	
	int months = 12;
	monthCombo->blockSignals(true);
	if(monthCombo->count() != 12 || !IS_GREGORIAN_CALENDAR) {
		monthCombo->clear();
		for(int i = 1; i <= months; i++) {
			monthCombo->addItem(QDate::longMonthName(i, QDate::StandaloneFormat));
		}
		monthCombo->setCurrentIndex(d_date.month() - 1);
	}
	monthCombo->blockSignals(false);
}
	
void EqonomizeMonthSelector::focusMonth() {monthCombo->setFocus();}
void EqonomizeMonthSelector::focusYear() {yearEdit->setFocus();}
	
#include "eqonomizemonthselector.moc"
