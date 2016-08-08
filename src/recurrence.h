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

#ifndef RECURRENCE_H
#define RECURRENCE_H

#include <QDateTime>
#include <QString>
#include <QVector>

class QXmlStreamReader;
class QXmlStreamWriter;
class QXmlStreamAttributes;

class Budget;

typedef enum {
	RECURRENCE_TYPE_DAILY,
	RECURRENCE_TYPE_WEEKLY,
	RECURRENCE_TYPE_MONTHLY,
	RECURRENCE_TYPE_YEARLY
} RecurrenceType;

class Recurrence {

	protected:

		Budget *o_budget;
		QDate d_startdate, d_enddate;
		int i_count;

	public:

		Recurrence(Budget *parent_budget);
		Recurrence(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		Recurrence(const Recurrence *rec);
		virtual ~Recurrence();
		virtual Recurrence *copy() const = 0;
		
		virtual void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		virtual bool readElement(QXmlStreamReader *xml, bool *valid);
		virtual bool readElements(QXmlStreamReader *xml, bool *valid);
		
		virtual void save(QXmlStreamWriter *xml);
		virtual void writeAttributes(QXmlStreamAttributes *attr);
		virtual void writeElements(QXmlStreamWriter *xml);

		void updateDates();
		virtual QDate nextOccurrence(const QDate &date, bool include_equals = false) const = 0;
		virtual QDate prevOccurrence(const QDate &date, bool include_equals = false) const = 0;
		const QDate &firstOccurrence() const;
		const QDate &lastOccurrence() const;
		int countOccurrences(const QDate &startdate, const QDate &enddate) const;
		int countOccurrences(const QDate &enddate) const;
		bool removeOccurrence(const QDate &date);
		const QDate &endDate() const;
		const QDate &startDate() const;
		int fixedOccurrenceCount() const;
		void setFixedOccurrenceCount(int new_count);
		void setEndDate(const QDate &new_end_date);
		void setStartDate(const QDate &new_start_date);
		virtual RecurrenceType type() const = 0;
		void addException(const QDate &date);
		int findException(const QDate &date) const;
		bool hasException(const QDate &date) const;
		bool removeException(const QDate &date);
		void clearExceptions();
		Budget *budget() const;

		QVector<QDate> exceptions;

};

class DailyRecurrence : public Recurrence {

	protected:

		int i_frequency;

	public:

		DailyRecurrence(Budget *parent_budget);
		DailyRecurrence(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		DailyRecurrence(const DailyRecurrence *rec);
		virtual ~DailyRecurrence();
		Recurrence *copy() const;

		virtual void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		virtual void writeAttributes(QXmlStreamAttributes *attr);
		
		QDate nextOccurrence(const QDate &date, bool include_equals = false) const;
		QDate prevOccurrence(const QDate &date, bool include_equals = false) const;
		RecurrenceType type() const;
		int frequency() const;
		void set(const QDate &new_start_date, const QDate &new_end_date, int new_frequency, int occurrences = -1);

};

class WeeklyRecurrence : public Recurrence {

	protected:

		int i_frequency;
		bool b_daysofweek[7];

	public:

		WeeklyRecurrence(Budget *parent_budget);
		WeeklyRecurrence(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		WeeklyRecurrence(const WeeklyRecurrence *rec);
		virtual ~WeeklyRecurrence();
		Recurrence *copy() const;

		virtual void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		virtual void writeAttributes(QXmlStreamAttributes *attr);
		
		QDate nextOccurrence(const QDate &date, bool include_equals = false) const;
		QDate prevOccurrence(const QDate &date, bool include_equals = false) const;
		RecurrenceType type() const;
		int frequency() const;
		bool dayOfWeek(int i) const;
		void set(const QDate &new_start_date, const QDate &new_end_date, bool d1, bool d2, bool d3, bool d4, bool d5, bool d6, bool d7, int new_frequency, int occurrences = -1);

};

typedef enum {
	WEEKEND_HANDLING_NONE = 0,
	WEEKEND_HANDLING_BEFORE = 1,
	WEEKEND_HANDLING_AFTER = 2,
	WEEKEND_HANDLING_NEAREST = 3
} WeekendHandling;

class MonthlyRecurrence : public Recurrence {

	protected:

		int i_frequency;
		int i_day;
		int i_week;
		int i_dayofweek;
		WeekendHandling wh_weekendhandling;

	public:

		MonthlyRecurrence(Budget *parent_budget);
		MonthlyRecurrence(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		MonthlyRecurrence(const MonthlyRecurrence *rec);
		virtual ~MonthlyRecurrence();
		Recurrence *copy() const;

		virtual void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		virtual void writeAttributes(QXmlStreamAttributes *attr);
		
		QDate nextOccurrence(const QDate &date, bool include_equals = false) const;
		QDate prevOccurrence(const QDate &date, bool include_equals = false) const;
		RecurrenceType type() const;
		int frequency() const;
		int day() const;
		WeekendHandling weekendHandling() const;
		int week() const;
		int dayOfWeek() const;
		void setOnDayOfWeek(const QDate &new_start_date, const QDate &new_end_date, int new_dayofweek, int new_week, int new_frequency, int occurrences = -1);
		void setOnDay(const QDate &new_start_date, const QDate &new_end_date, int new_day, WeekendHandling new_weekendhandling, int new_frequency, int occurrences = -1);

};

class YearlyRecurrence : public Recurrence {

	protected:

		int i_frequency;
		int i_dayofmonth;
		int i_month;
		int i_week;
		int i_dayofweek;
		int i_dayofyear;
		WeekendHandling wh_weekendhandling;

	public:

		YearlyRecurrence(Budget *parent_budget);
		YearlyRecurrence(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		YearlyRecurrence(const YearlyRecurrence *rec);
		virtual ~YearlyRecurrence();
		Recurrence *copy() const;

		virtual void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		virtual void writeAttributes(QXmlStreamAttributes *attr);
		
		QDate nextOccurrence(const QDate &date, bool include_equals = false) const;
		QDate prevOccurrence(const QDate &date, bool include_equals = false) const;
		RecurrenceType type() const;
		int frequency() const;
		int month() const;
		int dayOfMonth() const;
		int week() const;
		int dayOfWeek() const;
		int dayOfYear() const;
		WeekendHandling weekendHandling() const;
		void setOnDayOfWeek(const QDate &new_start_date, const QDate &new_end_date, int new_month, int new_dayofweek, int new_week, int new_frequency, int occurrences = -1);
		void setOnDayOfMonth(const QDate &new_start_date, const QDate &new_end_date, int new_month, int new_day, WeekendHandling new_weekendhandling, int new_frequency, int occurrences = -1);
		void setOnDayOfYear(const QDate &new_start_date, const QDate &new_end_date, int new_day, WeekendHandling new_weekendhandling, int new_frequency, int occurrences = -1);

};

#endif
