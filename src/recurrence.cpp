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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QXmlStreamAttribute>

#include "budget.h"
#include "recurrence.h"

int months_between_dates(const QDate &date1, const QDate &date2) {
	if(date1.year() == date2.year()) {
		return date2.month() - date1.month();
	}
	int months = 12 - date1.month();
	months += (date2.year() - (date1.year() + 1)) * 12;
	months += date2.month();
	return months;
}

int weeks_between_dates(const QDate &date1, const QDate &date2) {
	return (date1.dayOfWeek() - date2.dayOfWeek() + date1.daysTo(date2)) / 7;
}

int get_day_in_month(const QDate &date, int week, int day_of_week) {
	if(week > 0) {
		int fwd = date.dayOfWeek();
		fwd -= date.day() % 7 - 1;
		if(fwd <= 0) fwd = 7 + fwd;
		int day = 1;
		if(fwd < day_of_week) {
			day += day_of_week - fwd;
		} else if(fwd > day_of_week) {
			day += 7 - (fwd - day_of_week);
		}
		day += (week - 1) * 7;
		if(day > date.daysInMonth()) return 0;
		return day;
	} else {
		int lwd = date.dayOfWeek();
		int day = date.daysInMonth();
		lwd += (day - date.day()) % 7;
		if(lwd > 7) lwd = lwd - 7;
		if(lwd > day_of_week) {
			day -= lwd - day_of_week;
		} else if(lwd < day_of_week) {
			day -= 7 - (day_of_week - lwd);
		}
		day -= (-week) * 7;
		if(day < 1) return 0;
		return day;
	}
}
int get_day_in_month(int year, int month, int week, int day_of_week) {
	QDate date;
	date.setDate(year, month, 1);
	return get_day_in_month(date, week, day_of_week);
}

Recurrence::Recurrence(Budget *parent_budget) : o_budget(parent_budget) {
	i_count = -1;
}
Recurrence::Recurrence(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : o_budget(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
Recurrence::Recurrence(const Recurrence *rec) : o_budget(rec->budget()), d_startdate(rec->startDate()), d_enddate(rec->endDate()), i_count(rec->fixedOccurrenceCount()), exceptions(rec->exceptions) {}
Recurrence::~Recurrence() {}

void Recurrence::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	d_startdate = QDate::fromString(attr->value("startdate").toString(), Qt::ISODate);
	if(!attr->value("enddate").isEmpty()) {
		d_enddate = QDate::fromString(attr->value("enddate").toString(), Qt::ISODate);
	}
	if(attr->hasAttribute("recurrences")) i_count = attr->value("recurrences").toInt();
	else i_count = -1;
	if(valid && !d_startdate.isValid()) {
		*valid = false;
	}
	if(!d_enddate.isValid() && !d_enddate.isNull()) d_enddate = QDate();
}
bool Recurrence::readElement(QXmlStreamReader *xml, bool*) {
	if(xml->name() == "exception") {
		QDate date = QDate::fromString(xml->attributes().value("date").toString(), Qt::ISODate);
		if(date.isValid() && date >= d_startdate && (d_enddate.isNull() || date <= d_enddate)) {
			exceptions.append(date);
		}
	}
	return false;
}
bool Recurrence::readElements(QXmlStreamReader *xml, bool *valid) {
	while(xml->readNextStartElement()) {
		if(!readElement(xml, valid)) xml->skipCurrentElement();
	}
	std::sort(exceptions.begin(), exceptions.end());
	return true;
}

void Recurrence::save(QXmlStreamWriter *xml) {
	QXmlStreamAttributes attr;
	writeAttributes(&attr);
	xml->writeAttributes(attr);
	writeElements(xml);
}
void Recurrence::writeAttributes(QXmlStreamAttributes *attr) {
	attr->append("startdate", d_startdate.toString(Qt::ISODate));
	if(d_enddate.isValid()) attr->append("enddate", d_enddate.toString(Qt::ISODate));
}
void Recurrence::writeElements(QXmlStreamWriter *xml) {
	for(QVector<QDate>::iterator it = exceptions.begin(); it != exceptions.end(); ++it) {
		xml->writeStartElement("exception");
		xml->writeAttribute("date", it->toString(Qt::ISODate));
		xml->writeEndElement();
	}
}

void Recurrence::updateDates() {
	if(!d_startdate.isValid()) return;
	if(!d_enddate.isNull() && i_count <= 0) {
		d_enddate =  prevOccurrence(d_enddate, true);
		while(exceptions.count() > 0) {
			if(exceptions.last() == d_enddate) {
				d_enddate =  prevOccurrence(d_enddate, false);
				exceptions.pop_back();
			} else if(exceptions.last() > d_enddate) {
				exceptions.pop_back();
			} else {
				break;
			}
		}
	}
	//d_startdate =  nextOccurrence(d_startdate, true);
	while(exceptions.count() > 0) {
		if(exceptions.first() == d_startdate) {
			d_startdate =  nextOccurrence(d_startdate, false);
			exceptions.erase(exceptions.begin());
		} else if(exceptions.first() < d_startdate) {
			exceptions.erase(exceptions.begin());
		} else {
			break;
		}
	}
	if(i_count > 0) {
		d_enddate = QDate();
		QDate new_enddate = d_startdate;
		for(int i = 1; i < i_count; i++) {
			new_enddate = nextOccurrence(new_enddate);
			if(new_enddate.isNull()) {
				i_count = i;
				break;
			}
		}
		d_enddate = new_enddate;
		if(d_enddate.isNull()) {
			d_enddate = d_startdate;
		}
	}
	if(d_enddate.isNull() && nextOccurrence(d_startdate).isNull()) {
		d_enddate = d_startdate;
	}
}
const QDate &Recurrence::firstOccurrence() const {
	return d_startdate;
}
const QDate &Recurrence::lastOccurrence() const {
	return d_enddate;
}
int Recurrence::countOccurrences(const QDate &startdate, const QDate &enddate) const {
	if(enddate < d_startdate) return 0;
	if(!d_enddate.isNull() && startdate > d_enddate) return 0;
	if(i_count > 0 && startdate <= d_startdate && enddate <= d_enddate) return i_count;
	QDate date1 = nextOccurrence(startdate, true);
	if(date1.isNull() || date1 > enddate) return 0;
	int n = 0;
	do {
		n++;
		date1 = nextOccurrence(date1);
	} while(!date1.isNull() && date1 <= enddate);
	return n;
}
int Recurrence::countOccurrences(const QDate &enddate) const {
	return countOccurrences(d_startdate, enddate);
}
bool Recurrence::removeOccurrence(const QDate &date) {
	addException(date);
	return true;
}
const QDate &Recurrence::endDate() const {
	return d_enddate;
}
const QDate &Recurrence::startDate() const {
	return d_startdate;
}
void Recurrence::setEndDate(const QDate &new_end_date) {
	i_count = -1;
	d_enddate = new_end_date;
	if(!new_end_date.isNull()) {
		d_enddate =  prevOccurrence(d_enddate, true);
		while(exceptions.count() > 0) {
			if(exceptions.last() == d_enddate) {
				d_enddate =  prevOccurrence(d_enddate, false);
				exceptions.pop_back();
			} else if(exceptions.last() > d_enddate) {
				exceptions.pop_back();
			} else {
				break;
			}
		}
	} else if(nextOccurrence(d_startdate).isNull()) {
		d_enddate = d_startdate;
	}
}
void Recurrence::setStartDate(const QDate &new_start_date) {
	d_startdate = new_start_date;
	bool set_end_date = false;
	if(!d_enddate.isNull() && d_startdate > d_enddate) {
		d_enddate = QDate();
		set_end_date = true;
	}
	//d_startdate =  nextOccurrence(d_startdate, true);
	while(exceptions.count() > 0) {
		if(exceptions.first() == d_startdate) {
			d_startdate =  nextOccurrence(d_startdate, false);
			exceptions.erase(exceptions.begin());
		} else if(exceptions.first() < d_startdate) {
			exceptions.erase(exceptions.begin());
		} else {
			break;
		}
	}
	if(i_count > 0) setFixedOccurrenceCount(i_count);
	else if(set_end_date) d_enddate = d_startdate;
	if(d_enddate.isNull() && nextOccurrence(d_startdate).isNull()) {
		d_enddate = d_startdate;
	}
}
int Recurrence::fixedOccurrenceCount() const {
	return i_count;
}
void Recurrence::setFixedOccurrenceCount(int new_count) {
	if(new_count <= 0) {
		i_count = -1;
		setEndDate(d_enddate);
	} else {
		i_count = new_count;
		d_enddate = QDate();
		QDate new_enddate = d_startdate;
		for(int i = 1; i < i_count; i++) {
			new_enddate = nextOccurrence(new_enddate);
			if(new_enddate.isNull()) {
				i_count = i;
				break;
			}
		}
		d_enddate = new_enddate;
		if(d_enddate.isNull()) {
			d_enddate = d_startdate;
		}
	}
}
void Recurrence::addException(const QDate &date) {
	if(hasException(date) || !date.isValid()) return;
	if(date == d_startdate) {
		d_startdate =  nextOccurrence(d_startdate);
		if(d_startdate.isNull()) d_enddate = QDate();
		return;
	}
	if(date == d_enddate) {
		d_enddate =  prevOccurrence(d_enddate);
		if(d_enddate.isNull()) d_startdate = QDate();
		return;
	}
	exceptions.append(date);
	std::sort(exceptions.begin(), exceptions.end());
}
int Recurrence::findException(const QDate &date) const {
	for(QVector<QDate>::size_type i = 0; i < exceptions.count(); i++) {
		if(exceptions[i] == date) {
			return (int) i;
		}
		if(exceptions[i] > date) break;
	}
	return -1;
}
bool Recurrence::hasException(const QDate &date) const {
	return findException(date) >= 0;
}
bool Recurrence::removeException(const QDate &date) {
	for(QVector<QDate>::iterator it = exceptions.begin(); it != exceptions.end(); ++it) {
		if(*it == date) {
			exceptions.erase(it);
			return true;
		}
		if(*it > date) break;
	}
	return false;
}
void Recurrence::clearExceptions() {
	exceptions.clear();
}
Budget *Recurrence::budget() const {return o_budget;}

DailyRecurrence::DailyRecurrence(Budget *parent_budget) : Recurrence(parent_budget) {
	i_frequency = 1;
}
DailyRecurrence::DailyRecurrence(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : Recurrence(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
DailyRecurrence::DailyRecurrence(const DailyRecurrence *rec) : Recurrence(rec), i_frequency(rec->frequency()) {}
DailyRecurrence::~DailyRecurrence() {}
Recurrence *DailyRecurrence::copy() const {return new DailyRecurrence(this);}

void DailyRecurrence::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	Recurrence::readAttributes(attr, valid);
	if(attr->hasAttribute("frequency")) i_frequency = attr->value("frequency").toInt();
	else i_frequency = 1;
	if(valid && i_frequency < 1) {
		*valid = false;
	}
	updateDates();
}
void DailyRecurrence::writeAttributes(QXmlStreamAttributes *attr) {
	Recurrence::writeAttributes(attr);
	attr->append("frequency", QString::number(i_frequency));
}

QDate DailyRecurrence::nextOccurrence(const QDate &date, bool include_equals) const {
	if(include_equals) {
		if(date == startDate()) return date;
	}
	QDate nextdate = date;
	if(!include_equals) nextdate = nextdate.addDays(1);
	if(!endDate().isNull() && nextdate > endDate()) return QDate();
	if(nextdate <= startDate()) return firstOccurrence();
	if(i_frequency != 1) {
		int days = startDate().daysTo(nextdate);
		if(days % i_frequency != 0) {
			nextdate = nextdate.addDays(i_frequency - (days % i_frequency));
		}
	}
	if(!endDate().isNull() && nextdate > endDate()) return QDate();
	if(hasException(nextdate)) return nextOccurrence(nextdate);
	return nextdate;
}

QDate DailyRecurrence::prevOccurrence(const QDate &date, bool include_equals) const {
	if(!include_equals) {
		if(!endDate().isNull() && date > endDate()) return lastOccurrence();
	}
	QDate prevdate = date;
	if(!include_equals) prevdate = prevdate.addDays(-1);
	if(prevdate < startDate()) return QDate();
	if(prevdate == startDate()) return startDate();
	if(i_frequency != 1) {
		int days = startDate().daysTo(prevdate);
		if(days % i_frequency != 0) {
			prevdate = prevdate.addDays(-(days % i_frequency));
		}
	}
	if(prevdate < startDate()) return QDate();
	if(hasException(prevdate)) return prevOccurrence(prevdate);
	return prevdate;
}
RecurrenceType DailyRecurrence::type() const {
	return RECURRENCE_TYPE_DAILY;
}
int DailyRecurrence::frequency() const {
	return i_frequency;
}
void DailyRecurrence::set(const QDate &new_start_date, const QDate &new_end_date, int new_frequency, int occurrences) {
	i_frequency = new_frequency;
	setStartDate(new_start_date);
	if(occurrences <= 0) setEndDate(new_end_date);
	else setFixedOccurrenceCount(occurrences);
}

WeeklyRecurrence::WeeklyRecurrence(Budget *parent_budget) : Recurrence(parent_budget) {
	i_frequency = 1;
	b_daysofweek[0] = false;
	b_daysofweek[1] = false;
	b_daysofweek[2] = false;
	b_daysofweek[3] = false;
	b_daysofweek[4] = false;
	b_daysofweek[5] = false;
	b_daysofweek[6] = false;
}
WeeklyRecurrence::WeeklyRecurrence(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : Recurrence(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
WeeklyRecurrence::WeeklyRecurrence(const WeeklyRecurrence *rec) : Recurrence(rec), i_frequency(rec->frequency()) {
	b_daysofweek[0] = rec->dayOfWeek(1);
	b_daysofweek[1] = rec->dayOfWeek(2);
	b_daysofweek[2] = rec->dayOfWeek(3);
	b_daysofweek[3] = rec->dayOfWeek(4);
	b_daysofweek[4] = rec->dayOfWeek(5);
	b_daysofweek[5] = rec->dayOfWeek(6);
	b_daysofweek[6] = rec->dayOfWeek(7);
}
WeeklyRecurrence::~WeeklyRecurrence() {}
Recurrence *WeeklyRecurrence::copy() const {return new WeeklyRecurrence(this);}

void WeeklyRecurrence::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	Recurrence::readAttributes(attr, valid);
	if(attr->hasAttribute("frequency")) i_frequency = attr->value("frequency").toInt();
	else i_frequency = 1;
	QStringRef days = attr->value("days");
	b_daysofweek[0] = days.indexOf('1') >= 0;
	b_daysofweek[1] = days.indexOf('2') >= 0;
	b_daysofweek[2] = days.indexOf('3') >= 0;
	b_daysofweek[3] = days.indexOf('4') >= 0;
	b_daysofweek[4] = days.indexOf('5') >= 0;
	b_daysofweek[5] = days.indexOf('6') >= 0;
	b_daysofweek[6] = days.indexOf('7') >= 0;
	if(valid) {
		bool b = false;
		for(int i = 0; i < 7; i++) {
			if(b_daysofweek[i]) {
				b = true;
				break;
			}
		}
		if(!b) *valid = false;
	}
	if(valid && i_frequency < 1) {
		*valid = false;
	}
	updateDates();
}
void WeeklyRecurrence::writeAttributes(QXmlStreamAttributes *attr) {
	Recurrence::writeAttributes(attr);
	attr->append("frequency", QString::number(i_frequency));
	QString days;
	if(b_daysofweek[0]) days += '1';
	if(b_daysofweek[1]) days += '2';
	if(b_daysofweek[2]) days += '3';
	if(b_daysofweek[3]) days += '4';
	if(b_daysofweek[4]) days += '5';
	if(b_daysofweek[5]) days += '6';
	if(b_daysofweek[6]) days += '7';
	attr->append("days", days);
}

QDate WeeklyRecurrence::nextOccurrence(const QDate &date, bool include_equals) const {
	if(!include_equals) {
		if(date < startDate()) return firstOccurrence();
	} else {
		if(date <= startDate()) return firstOccurrence();
	}
	QDate nextdate = date;
	if(!include_equals) nextdate = nextdate.addDays(1);
	if(!endDate().isNull() && nextdate > endDate()) return QDate();
	if(i_frequency != 1 && (nextdate.year() != startDate().year() || nextdate.weekNumber() != startDate().weekNumber())) {
		int i = weeks_between_dates(startDate(), nextdate) % i_frequency;
		if(i != 0) {
			nextdate = nextdate.addDays((i_frequency - i) * 7 - (nextdate.dayOfWeek() - 1));
		}
	}
	int dow = nextdate.dayOfWeek();
	int i = dow;
	for(; i <= 7; i++) {
		if(b_daysofweek[i - 1]) {
			break;
		}
	}
	if(i > 7) {
		for(i = 1; i <= 7; i++) {
			if(b_daysofweek[i - 1]) {
				break;
			}
		}
		if(i > 7) return QDate();
	}
	if(i < dow) {
		nextdate = nextdate.addDays((i_frequency * 7) + i - dow);
	} else if(i > dow) {
		nextdate = nextdate.addDays(i - dow);
	}
	if(!endDate().isNull() && nextdate > endDate()) return QDate();
	if(hasException(nextdate)) return nextOccurrence(nextdate);
	return nextdate;
}

QDate WeeklyRecurrence::prevOccurrence(const QDate &date, bool include_equals) const {
	if(!include_equals) {
		if(!endDate().isNull() && date > endDate()) return lastOccurrence();
	}
	QDate prevdate = date;
	if(!include_equals) prevdate = prevdate.addDays(-1);
	if(prevdate < startDate()) return QDate();
	if(prevdate == startDate()) return startDate();
	if(i_frequency != 1 && (prevdate.year() != startDate().year() || prevdate.weekNumber() != startDate().weekNumber())) {
		int i = weeks_between_dates(startDate(), prevdate) % i_frequency;
		if(i != 0) {
			prevdate = prevdate.addDays(-(i * 7) + 7 - prevdate.dayOfWeek());
		}
	}
	int dow_s = startDate().dayOfWeek();
	bool s_week = prevdate.year() == startDate().year() && prevdate.weekNumber() == startDate().weekNumber();
	int dow = prevdate.dayOfWeek();
	int i = dow;
	for(; i <= 7; i++) {
		if(b_daysofweek[i - 1] || (s_week && dow_s == i)) {
			break;
		}
	}
	if(i > 7) {
		for(i = 1; i <= 7; i++) {
			if(b_daysofweek[i - 1] || (s_week && dow_s == i)) {
				break;
			}
		}
		if(i > 7) return QDate();
	}
	if(i > dow) {
		prevdate = prevdate.addDays(-(i_frequency * 7) + dow - i);
	} else if(i < dow) {
		prevdate = prevdate.addDays(dow - i);
	}
	if(prevdate < startDate()) return QDate();
	if(hasException(prevdate)) return prevOccurrence(prevdate);
	return prevdate;
}
RecurrenceType WeeklyRecurrence::type() const {
	return RECURRENCE_TYPE_WEEKLY;
}
int WeeklyRecurrence::frequency() const {
	return i_frequency;
}
bool WeeklyRecurrence::dayOfWeek(int i) const {
	if(i >= 1 && i <= 7) return b_daysofweek[i - 1];
	return false;
}
void WeeklyRecurrence::set(const QDate &new_start_date, const QDate &new_end_date, bool d1, bool d2, bool d3, bool d4, bool d5, bool d6, bool d7, int new_frequency, int occurrences) {
	b_daysofweek[0] = d1;
	b_daysofweek[1] = d2;
	b_daysofweek[2] = d3;
	b_daysofweek[3] = d4;
	b_daysofweek[4] = d5;
	b_daysofweek[5] = d6;
	b_daysofweek[6] = d7;
	i_frequency = new_frequency;
	setStartDate(new_start_date);
	if(occurrences <= 0) setEndDate(new_end_date);
	else setFixedOccurrenceCount(occurrences);
}

MonthlyRecurrence::MonthlyRecurrence(Budget *parent_budget) : Recurrence(parent_budget) {
	i_day = 1;
	i_frequency = 1;
	i_week = 1;
	i_dayofweek = -1;
	wh_weekendhandling = WEEKEND_HANDLING_NONE;
}
MonthlyRecurrence::MonthlyRecurrence(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : Recurrence(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
MonthlyRecurrence::MonthlyRecurrence(const MonthlyRecurrence *rec) : Recurrence(rec), i_frequency(rec->frequency()), i_day(rec->day()), i_week(rec->week()), i_dayofweek(rec->dayOfWeek()), wh_weekendhandling(rec->weekendHandling()) {}
MonthlyRecurrence::~MonthlyRecurrence() {}
Recurrence *MonthlyRecurrence::copy() const {return new MonthlyRecurrence(this);}

void MonthlyRecurrence::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	Recurrence::readAttributes(attr, valid);
	if(attr->hasAttribute("day")) i_day = attr->value("day").toInt();
	else i_day = 1;
	if(attr->hasAttribute("frequency")) i_frequency = attr->value("frequency").toInt();
	else i_frequency = 1;
	if(attr->hasAttribute("week")) i_week = attr->value("week").toInt();
	else i_week = 1;
	if(attr->hasAttribute("dayofweek")) i_dayofweek = attr->value("dayofweek").toInt();
	else i_dayofweek = -1;
	if(attr->hasAttribute("weekendhandling")) wh_weekendhandling = (WeekendHandling) attr->value("weekendhandling").toInt();
	else wh_weekendhandling = (WeekendHandling) 0;
	if(valid && (i_day > 31 || i_day < -27 || i_frequency < 1 || i_week > 5 || i_week < -4 || i_dayofweek > 7)) {
		*valid = false;
	}
	updateDates();
}
void MonthlyRecurrence::writeAttributes(QXmlStreamAttributes *attr) {
	Recurrence::writeAttributes(attr);
	attr->append("frequency", QString::number(i_frequency));
	if(i_dayofweek <= 0) {
		attr->append("day", QString::number(i_day));
		attr->append("weekendhandling", QString::number(wh_weekendhandling));
	} else {
		attr->append("dayofweek", QString::number(i_dayofweek));
		attr->append("week", QString::number(i_week));
	}
}

QDate MonthlyRecurrence::nextOccurrence(const QDate &date, bool include_equals) const {
	if(!include_equals) {
		if(date < startDate()) return firstOccurrence();
	} else {
		if(date <= startDate()) return firstOccurrence();
	}
	QDate nextdate = date;
	if(!include_equals) nextdate = nextdate.addDays(1);
	if(!endDate().isNull() && nextdate > endDate()) return QDate();
	int prevday = -1;
	if(nextdate.year() == startDate().year() && nextdate.month() == startDate().month()) {
		if(i_frequency > 1) prevday = 1;
		else prevday = nextdate.day();
		nextdate = nextdate.addMonths(i_frequency);
		nextdate.setDate(nextdate.year(), nextdate.month(), 1);
	} else if(i_frequency != 1) {
		int i = months_between_dates(startDate(), nextdate) % i_frequency;
		if(i != 0) {
			if(i_frequency - i > 1) prevday = 1;
			else prevday = nextdate.day();
			nextdate = nextdate.addMonths(i_frequency - i);
			nextdate.setDate(nextdate.year(), nextdate.month(), 1);
		}
	}
	int day = i_day;
	if(i_dayofweek > 0) day = get_day_in_month(nextdate, i_week, i_dayofweek);
	else if(i_day < 1) day = nextdate.daysInMonth() + i_day;
	if(wh_weekendhandling == WEEKEND_HANDLING_BEFORE) {
		QDate date2;
		date2.setDate(nextdate.year(), nextdate.month(), day);
		int wday = date2.dayOfWeek();
		if(wday == 6) day -= 1;
		else if(wday == 7) day -= 2;
		if(day <= 0 && prevday > 0 && prevday <= nextdate.addMonths(-1).daysInMonth() + day) {
			nextdate = nextdate.addMonths(-1);
			day = nextdate.daysInMonth() + day;
		}
	} else if(wh_weekendhandling == WEEKEND_HANDLING_AFTER) {
		QDate date2;
		date2.setDate(nextdate.year(), nextdate.month(), day);
		int wday = date2.dayOfWeek();
		if(wday == 6) day += 2;
		else if(wday == 7) day += 1;
		if(day > nextdate.daysInMonth()) {
			day -= nextdate.daysInMonth();
			nextdate = nextdate.addMonths(1);
			nextdate.setDate(nextdate.year(), nextdate.month(), day);
		}
	} else if(wh_weekendhandling == WEEKEND_HANDLING_NEAREST) {
		QDate date2;
		date2.setDate(nextdate.year(), nextdate.month(), day);
		int wday = date2.dayOfWeek();
		if(wday == 6) {
			day -= 1;
			if(day <= 0 && prevday > 0 && prevday <= nextdate.addMonths(-1).daysInMonth() + day) {
				nextdate = nextdate.addMonths(-1);
				day = nextdate.daysInMonth() + day;
			}
		} else if(wday == 7) {
			day += 1;
			if(day > nextdate.daysInMonth()) {
				day -= nextdate.daysInMonth();
				nextdate = nextdate.addMonths(1);
				nextdate.setDate(nextdate.year(), nextdate.month(), day);
			}
		}
	}
	if(day <= 0 || nextdate.day() > day || day > nextdate.daysInMonth()) {
		int i = 0;
		do {
			if(i >= 1200 / i_frequency) return QDate();
			i++;
			if(i_frequency > 1) prevday = 1;
			else prevday = nextdate.day();
			nextdate = nextdate.addMonths(i_frequency);
			if(!endDate().isNull() && (nextdate.year() > endDate().year() || (nextdate.year() == endDate().year() && nextdate.month() > endDate().month()))) return QDate();
			day = i_day;
			if(i_dayofweek > 0) day = get_day_in_month(nextdate, i_week, i_dayofweek);
			else if(i_day < 1) day = nextdate.daysInMonth() + i_day;
			if(wh_weekendhandling == WEEKEND_HANDLING_BEFORE) {
				QDate date2;
				date2.setDate(nextdate.year(), nextdate.month(), day);
				int wday = date2.dayOfWeek();
				if(wday == 6) day -= 1;
				else if(wday == 7) day -= 2;
				if(day <= 0 && prevday > 0 && prevday <= nextdate.addMonths(-1).daysInMonth() + day) {
					nextdate = nextdate.addMonths(-1);
					day = nextdate.daysInMonth() + day;
				} else {
					nextdate.setDate(nextdate.year(), nextdate.month(), 1);
				}
			} else if(wh_weekendhandling == WEEKEND_HANDLING_AFTER) {
				QDate date2;
				date2.setDate(nextdate.year(), nextdate.month(), day);
				int wday = date2.dayOfWeek();
				if(wday == 6) day += 2;
				else if(wday == 7) day += 1;
				if(day > nextdate.daysInMonth()) {
					day -= nextdate.daysInMonth();
					nextdate = nextdate.addMonths(1);
					nextdate.setDate(nextdate.year(), nextdate.month(), day);
				}
			} else if(wh_weekendhandling == WEEKEND_HANDLING_NEAREST) {
				QDate date2;
				date2.setDate(nextdate.year(), nextdate.month(), day);
				int wday = date2.dayOfWeek();
				if(wday == 6) {
					day -= 1;
					if(day <= 0 && prevday > 0 && prevday <= nextdate.addMonths(-1).daysInMonth() + day) {
						nextdate = nextdate.addMonths(-1);
						day = nextdate.daysInMonth() + day;
					} else {
						nextdate.setDate(nextdate.year(), nextdate.month(), 1);
					}
				} else if(wday == 7) {
					day += 1;
					if(day > nextdate.daysInMonth()) {
						day -= nextdate.daysInMonth();
						nextdate = nextdate.addMonths(1);
						nextdate.setDate(nextdate.year(), nextdate.month(), day);
					}
				}
			}
		} while(day <= 0 || day > nextdate.daysInMonth());
	}
	nextdate.setDate(nextdate.year(), nextdate.month(), day);
	if(!endDate().isNull() && nextdate > endDate()) return QDate();
	if(hasException(nextdate)) return nextOccurrence(nextdate);
	return nextdate;
}

QDate MonthlyRecurrence::prevOccurrence(const QDate &date, bool include_equals) const {
	if(!include_equals) {
		if(!endDate().isNull() && date > endDate()) return lastOccurrence();
	}
	QDate prevdate = date;
	if(!include_equals) prevdate = prevdate.addDays(-1);
	if(prevdate < startDate()) return QDate();
	if(prevdate == startDate()) return startDate();
	int prevday = -1;
	if(i_frequency != 1 && (prevdate.year() != startDate().year() || prevdate.month() != startDate().month())) {
		int i = months_between_dates(startDate(), prevdate) % i_frequency;
		if(i != 0) {
			if(i > 1) prevday = 1;
			else prevday = prevdate.day();
			prevdate = prevdate.addMonths(-i);
			prevdate.setDate(prevdate.year(), prevdate.month(), prevdate.daysInMonth());
		}
	}
	if(prevdate.year() == startDate().year() && prevdate.month() == startDate().month()) return startDate();
	int day = i_day;
	if(i_dayofweek > 0) day = get_day_in_month(prevdate, i_week, i_dayofweek);
	else if(i_day < 1) day = prevdate.daysInMonth() + i_day;
	if(wh_weekendhandling == WEEKEND_HANDLING_BEFORE) {
		QDate date2;
		date2.setDate(prevdate.year(), prevdate.month(), day);
		int wday = date2.dayOfWeek();
		if(wday == 6) day -= 1;
		else if(wday == 7) day -= 2;
		if(day <= 0) {
			prevdate = prevdate.addMonths(-1);
			day = prevdate.daysInMonth() + day;
		}
	} else if(wh_weekendhandling == WEEKEND_HANDLING_AFTER) {
		QDate date2;
		date2.setDate(prevdate.year(), prevdate.month(), day);
		int wday = date2.dayOfWeek();
		if(wday == 6) day += 2;
		else if(wday == 7) day += 1;
		if(day > prevdate.daysInMonth() && prevday > 0 && prevday >= day - prevdate.daysInMonth()) {
			day -= prevdate.daysInMonth();
			prevdate = prevdate.addMonths(1);
			prevdate.setDate(prevdate.year(), prevdate.month(), day);
		}
	} else if(wh_weekendhandling == WEEKEND_HANDLING_NEAREST) {
		QDate date2;
		date2.setDate(prevdate.year(), prevdate.month(), day);
		int wday = date2.dayOfWeek();
		if(wday == 6) {
			day -= 1;
			if(day <= 0) {
				prevdate = prevdate.addMonths(-1);
				day = prevdate.daysInMonth() + day;
			}
		} else if(wday == 7) {
			day += 1;
			if(day > prevdate.daysInMonth() && prevday > 0 && prevday >= day - prevdate.daysInMonth()) {
				day -= prevdate.daysInMonth();
				prevdate = prevdate.addMonths(1);
				prevdate.setDate(prevdate.year(), prevdate.month(), day);
			}
		}
	}
	if(day <= 0 || prevdate.day() < day) {
		int i = 0;
		do {
			if(i >= 1200 / i_frequency) return QDate();
			i++;
			if(i_frequency > 1) prevday = 1;
			else prevday = prevdate.day();
			prevdate = prevdate.addMonths(-i_frequency);
			if(prevdate.year() == startDate().year() && prevdate.month() == startDate().month()) return startDate();
			if(prevdate.year() <= startDate().year() && prevdate.month() < startDate().month()) return QDate();
			day = i_day;
			if(i_dayofweek > 0) day = get_day_in_month(prevdate, i_week, i_dayofweek);
			else if(i_day < 1) day = prevdate.daysInMonth() + i_day;
			if(wh_weekendhandling == WEEKEND_HANDLING_BEFORE) {
				QDate date2;
				date2.setDate(prevdate.year(), prevdate.month(), day);
				int wday = date2.dayOfWeek();
				if(wday == 6) day -= 1;
				else if(wday == 7) day -= 2;
				if(day <= 0) {
					prevdate = prevdate.addMonths(-1);
					day = prevdate.daysInMonth() + day;
				}
			} else if(wh_weekendhandling == WEEKEND_HANDLING_AFTER) {
				QDate date2;
				date2.setDate(prevdate.year(), prevdate.month(), day);
				int wday = date2.dayOfWeek();
				if(wday == 6) day += 2;
				else if(wday == 7) day += 1;
				if(day > prevdate.daysInMonth() && prevday > 0 && prevday >= day - prevdate.daysInMonth()) {
					day -= prevdate.daysInMonth();
					prevdate = prevdate.addMonths(1);
					prevdate.setDate(prevdate.year(), prevdate.month(), day);
				} else {
					prevdate.setDate(prevdate.year(), prevdate.month(), prevdate.daysInMonth());
				}
			} else if(wh_weekendhandling == WEEKEND_HANDLING_NEAREST) {
				QDate date2;
				date2.setDate(prevdate.year(), prevdate.month(), day);
				int wday = date2.dayOfWeek();
				if(wday == 6) {
					day -= 1;
					if(day <= 0) {
						prevdate = prevdate.addMonths(-1);
						day = prevdate.daysInMonth() + day;
					}
				} else if(wday == 7) {
					day += 1;
					if(day > prevdate.daysInMonth() && prevday > 0 && prevday >= day - prevdate.daysInMonth()) {
						day -= prevdate.daysInMonth();
						prevdate = prevdate.addMonths(1);
						prevdate.setDate(prevdate.year(), prevdate.month(), day);
					} else {
						prevdate.setDate(prevdate.year(), prevdate.month(), prevdate.daysInMonth());
					}
				}
			}
		} while(day <= 0 || day > prevdate.daysInMonth());
	}
	prevdate.setDate(prevdate.year(), prevdate.month(), day);
	if(prevdate < startDate()) return QDate();
	if(hasException(prevdate)) return prevOccurrence(prevdate);
	return prevdate;
}
RecurrenceType MonthlyRecurrence::type() const {
	return RECURRENCE_TYPE_MONTHLY;
}
int MonthlyRecurrence::frequency() const {
	return i_frequency;
}
int MonthlyRecurrence::day() const {
	return i_day;
}
WeekendHandling MonthlyRecurrence::weekendHandling() const {
	return wh_weekendhandling;
}
int MonthlyRecurrence::week() const {
	return i_week;
}
int MonthlyRecurrence::dayOfWeek() const {
	return i_dayofweek;
}
void MonthlyRecurrence::setOnDayOfWeek(const QDate &new_start_date, const QDate &new_end_date, int new_dayofweek, int new_week, int new_frequency, int occurrences) {
	i_week = new_week;
	i_frequency = new_frequency;
	i_dayofweek = new_dayofweek;
	wh_weekendhandling = WEEKEND_HANDLING_NONE;
	setStartDate(new_start_date);
	if(occurrences <= 0) setEndDate(new_end_date);
	else setFixedOccurrenceCount(occurrences);
}
void MonthlyRecurrence::setOnDay(const QDate &new_start_date, const QDate &new_end_date, int new_day, WeekendHandling new_weekendhandling, int new_frequency, int occurrences) {
	i_dayofweek = -1;
	i_day = new_day;
	i_frequency = new_frequency;
	wh_weekendhandling = new_weekendhandling;
	setStartDate(new_start_date);
	if(occurrences <= 0) setEndDate(new_end_date);
	else setFixedOccurrenceCount(occurrences);
}

YearlyRecurrence::YearlyRecurrence(Budget *parent_budget) : Recurrence(parent_budget) {
	i_dayofmonth = 1;
	i_month = 1;
	i_frequency = 1;
	i_week = 1;
	i_dayofweek = -1;
	i_dayofyear = -1;
	wh_weekendhandling = WEEKEND_HANDLING_NONE;
}
YearlyRecurrence::YearlyRecurrence(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : Recurrence(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
YearlyRecurrence::YearlyRecurrence(const YearlyRecurrence *rec) : Recurrence(rec), i_frequency(rec->frequency()), i_dayofmonth(rec->dayOfMonth()), i_month(rec->month()), i_week(rec->week()), i_dayofweek(rec->dayOfWeek()), i_dayofyear(rec->dayOfYear()), wh_weekendhandling(rec->weekendHandling()) {}
YearlyRecurrence::~YearlyRecurrence() {}
Recurrence *YearlyRecurrence::copy() const {return new YearlyRecurrence(this);}

void YearlyRecurrence::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	Recurrence::readAttributes(attr, valid);
	if(attr->hasAttribute("frequency")) i_frequency = attr->value("frequency").toInt();
	else i_frequency = 1;
	if(attr->hasAttribute("dayofmonth")) i_dayofmonth = attr->value("dayofmonth").toInt();
	else i_dayofmonth = 1;
	if(attr->hasAttribute("month")) i_month = attr->value("month").toInt();
	else i_month = 1;
	if(attr->hasAttribute("week")) i_week = attr->value("week").toInt();
	else i_week = 1;
	if(attr->hasAttribute("dayofweek")) i_dayofweek = attr->value("dayofweek").toInt();
	else i_dayofweek = -1;
	if(attr->hasAttribute("dayofyear")) i_dayofyear = attr->value("dayofyear").toInt();
	else i_dayofyear = -1;
	if(attr->hasAttribute("weekendhandling")) wh_weekendhandling = (WeekendHandling) attr->value("weekendhandling").toInt();
	else wh_weekendhandling = (WeekendHandling) 0;
	if(valid && (i_dayofmonth > 31 || i_frequency < 1 || i_week > 5 || i_week < -4 || i_dayofweek > 7)) {
		*valid = false;
	}
	updateDates();
}
void YearlyRecurrence::writeAttributes(QXmlStreamAttributes *attr) {
	Recurrence::writeAttributes(attr);
	attr->append("frequency", QString::number(i_frequency));
	if(i_dayofyear > 0) {
		attr->append("dayofyear", QString::number(i_dayofyear));
		attr->append("weekendhandling", QString::number(wh_weekendhandling));
	} else if(i_dayofweek > 0) {
		attr->append("month", QString::number(i_month));
		attr->append("dayofweek", QString::number(i_dayofweek));
		attr->append("week", QString::number(i_week));
	} else {
		attr->append("month", QString::number(i_month));
		attr->append("dayofmonth", QString::number(i_dayofmonth));
		attr->append("weekendhandling", QString::number(wh_weekendhandling));
	}
}
QDate YearlyRecurrence::nextOccurrence(const QDate &date, bool include_equals) const {
	if(!include_equals) {
		if(date < startDate()) return firstOccurrence();
	} else {
		if(date <= startDate()) return firstOccurrence();
	}
	QDate nextdate = date;
	if(!include_equals) nextdate = nextdate.addDays(1);
	if(!endDate().isNull() && nextdate > endDate()) return QDate();
	if(nextdate.year() == startDate().year()) {
		nextdate = nextdate.addYears(i_frequency);
		nextdate.setDate(nextdate.year(), 1, 1);
	} else if(i_frequency != 1) {
		int i = (nextdate.year() - startDate().year()) % i_frequency;
		if(i != 0) {
			nextdate = nextdate.addYears(i_frequency - i);
			nextdate.setDate(nextdate.year(), 1, 1);
		}
	}
	if(i_dayofyear > 0) {
		if(nextdate.dayOfYear() > i_dayofyear) {
			nextdate = nextdate.addYears(i_frequency);
		}
		if(i_dayofyear > nextdate.daysInYear()) {
			int i = 10;
			do {
				if(i == 0) return QDate();
				nextdate = nextdate.addYears(i_frequency);
				i--;
			} while(i_dayofyear > nextdate.daysInYear());
		}
		nextdate = nextdate.addDays(i_dayofyear - nextdate.dayOfYear());
	} else {
		int day = i_dayofmonth;
		if(i_dayofweek > 0) day = get_day_in_month(nextdate.year(), i_month, i_week, i_dayofweek);
		if(day == 0 || nextdate.month() > i_month || (nextdate.month() == i_month && nextdate.day() > day)) {
			do {
				nextdate = nextdate.addYears(i_frequency);
				if(i_dayofweek > 0) day = get_day_in_month(nextdate.year(), i_month, i_week, i_dayofweek);
				if(!endDate().isNull() && nextdate.year() > endDate().year()) return QDate();
			} while(day == 0);
		}
		if(i_dayofweek <= 0) {
			nextdate.setDate(nextdate.year(), i_month, 1);
			if(day > nextdate.daysInMonth()) {
				int i = 10;
				do {
					if(i == 0) return QDate();
					nextdate = nextdate.addYears(i_frequency);
					nextdate.setDate(nextdate.year(), i_month, 1);
					i--;
				} while(day > nextdate.daysInMonth());
			}
		}
		nextdate.setDate(nextdate.year(), i_month, day);
	}
	if(!endDate().isNull() && nextdate > endDate()) return QDate();
	if(hasException(nextdate)) return nextOccurrence(nextdate);
	return nextdate;
}

QDate YearlyRecurrence::prevOccurrence(const QDate &date, bool include_equals) const {
	if(!include_equals) {
		if(!endDate().isNull() && date > endDate()) return lastOccurrence();
	}
	QDate prevdate = date;
	if(!include_equals) prevdate = prevdate.addDays(-1);
	if(prevdate < startDate()) return QDate();
	if(prevdate == startDate()) return startDate();
	if(i_frequency != 1) {
		int i = (prevdate.year() - startDate().year()) % i_frequency;
		if(i != 0) {
			prevdate = prevdate.addYears(-i);
		}
	}
	if(prevdate.year() == startDate().year()) return startDate();
	if(i_dayofyear > 0) {
		if(prevdate.dayOfYear() < i_dayofyear) {
			prevdate = prevdate.addYears(-i_frequency);
			if(prevdate.year() == startDate().year()) return startDate();
		}
		if(i_dayofyear > prevdate.daysInYear()) {
			do {
				prevdate = prevdate.addYears(-i_frequency);
				if(prevdate.year() <= startDate().year()) return startDate();
			} while(i_dayofyear > prevdate.daysInYear());
		}
		prevdate = prevdate.addDays(i_dayofyear - prevdate.dayOfYear());
	} else {
		int day = i_dayofmonth;
		if(i_dayofweek > 0) day = get_day_in_month(prevdate.year(), i_month, i_week, i_dayofweek);
		if(day <= 0 || prevdate.month() < i_month || (prevdate.month() == i_month && prevdate.day() < day)) {
			do {
				prevdate = prevdate.addYears(-i_frequency);
				if(i_dayofweek > 0) day = get_day_in_month(prevdate.year(), i_month, i_week, i_dayofweek);
				if(prevdate.year() == startDate().year()) return startDate();
			} while(day <= 0);
		}
		if(i_dayofweek <= 0) {
			prevdate.setDate(prevdate.year(), i_month, 1);
			if(day > prevdate.daysInMonth()) {
				do {
					prevdate = prevdate.addYears(-i_frequency);
					prevdate.setDate(prevdate.year(), i_month, 1);
					if(prevdate.year() <= startDate().year()) return startDate();
				} while(day > prevdate.daysInMonth());
			}
		}
		prevdate.setDate(prevdate.year(), i_month, day);
	}
	if(prevdate < startDate()) return QDate();
	if(hasException(prevdate)) return prevOccurrence(prevdate);
	return prevdate;
}
RecurrenceType YearlyRecurrence::type() const {
	return RECURRENCE_TYPE_YEARLY;
}
int YearlyRecurrence::frequency() const {
	return i_frequency;
}
int YearlyRecurrence::dayOfYear() const {
	return i_dayofyear;
}
int YearlyRecurrence::month() const {
	return i_month;
}
int YearlyRecurrence::dayOfMonth() const {
	return i_dayofmonth;
}
WeekendHandling YearlyRecurrence::weekendHandling() const {
	return wh_weekendhandling;
}
int YearlyRecurrence::week() const {
	return i_week;
}
int YearlyRecurrence::dayOfWeek() const {
	return i_dayofweek;
}
void YearlyRecurrence::setOnDayOfWeek(const QDate &new_start_date, const QDate &new_end_date, int new_month, int new_dayofweek, int new_week, int new_frequency, int occurrences) {
	i_dayofyear = -1;
	wh_weekendhandling = WEEKEND_HANDLING_NONE;
	i_dayofweek = new_dayofweek;
	i_week = new_week;
	i_month = new_month;
	i_frequency = new_frequency;
	setStartDate(new_start_date);
	if(occurrences <= 0) setEndDate(new_end_date);
	else setFixedOccurrenceCount(occurrences);
}
void YearlyRecurrence::setOnDayOfMonth(const QDate &new_start_date, const QDate &new_end_date, int new_month, int new_day, WeekendHandling new_weekendhandling, int new_frequency, int occurrences) {
	i_dayofweek = -1;
	i_dayofyear = -1;
	i_dayofmonth = new_day;
	i_month = new_month;
	i_frequency = new_frequency;
	wh_weekendhandling = new_weekendhandling;
	setStartDate(new_start_date);
	if(occurrences <= 0) setEndDate(new_end_date);
	else setFixedOccurrenceCount(occurrences);
}
void YearlyRecurrence::setOnDayOfYear(const QDate &new_start_date, const QDate &new_end_date, int new_day, WeekendHandling new_weekendhandling, int new_frequency, int occurrences) {
	i_dayofyear = new_day;
	wh_weekendhandling = new_weekendhandling;
	i_frequency = new_frequency;
	setStartDate(new_start_date);
	if(occurrences <= 0) setEndDate(new_end_date);
	else setFixedOccurrenceCount(occurrences);
}

