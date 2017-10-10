/***************************************************************************
 *   Copyright (C) 2017 by Hanna Knutsson                                  *
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

#ifndef CURRENCY_H
#define CURRENCY_H

#include <QString>
#include <QDate>
#include <QMap>
#include <QCoreApplication>

#include "eqonomizelist.h"

class QXmlStreamReader;
class QXmlStreamWriter;
class QXmlStreamAttributes;

class Budget;

typedef enum {
	EXCHANGE_RATE_SOURCE_NONE,
	EXCHANGE_RATE_SOURCE_ECB,
	EXCHANGE_RATE_SOURCE_MYCURRENCY_NET,
} ExchangeRateSource;

class Currency {
	
	Q_DECLARE_TR_FUNCTIONS(Currency)
	
	protected:
			
		int i_decimals;
		int b_precedes;
		QString s_code, s_symbol, s_name;
		ExchangeRateSource r_source;
		Budget *o_budget;
		bool b_local_rate, b_local_name, b_local_symbol, b_local_format;
	
	public:
	
		QMap<QDate, double> rates;
		
		Currency();
		Currency(Budget *parent_budget);
		Currency(Budget *parent_budget, QString initial_code, QString initial_symbol = QString(), QString initial_name = QString(), double initial_rate = 1.0, QDate date = QDate(), int initial_decimals = -1, int initial_precedes = -1);
		Currency(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		~Currency();
		Currency *copy() const;
		
		bool merge(Currency *currency, bool keep_rates = true);
		
		void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		bool readElement(QXmlStreamReader *xml, bool *valid);
		bool readElements(QXmlStreamReader *xml, bool *valid);
		void save(QXmlStreamWriter *xml, bool local_save = true);
		void writeAttributes(QXmlStreamAttributes *attr, bool local_save = true);
		void writeElements(QXmlStreamWriter *xml, bool local_save = true);
	
		double exchangeRate(QDate date = QDate(), bool exact_match = false) const;
		QDate lastExchangeRateDate() const;
		void setExchangeRate(double new_rate, QDate date = QDate());
		
		ExchangeRateSource exchangeRateSource() const;
		void setExchangeRateSource(ExchangeRateSource source);
		
		double convertTo(double value, const Currency *to_currency) const;
		double convertFrom(double value, const Currency *from_currency) const;
		double convertTo(double value, const Currency *to_currency, const QDate &date) const;
		double convertFrom(double value, const Currency *from_currency, const QDate &date) const;
		
		QString formatValue(double value, int nr_of_decimals = -1, bool show_currency = true) const;

		const QString &code() const;
		const QString &symbol(bool return_code_if_empty = false) const;
		const QString &name(bool return_code_if_empty = false) const;
		void setCode(QString new_code);
		void setSymbol(QString new_symbol);
		void setName(QString new_name);
		
		int symbolPrecedes(bool return_default_if_unset = true) const;
		void setSymbolPrecedes(int new_precedes);
		
		int fractionalDigits(bool return_default_if_unset = true) const;
		void setFractionalDigits(int new_frac_digits);
		
		bool hasLocalChanges() const;
		void setAsLocal(bool is_local = true);
		
		bool exchangeRateIsUpdated() const;
		void setExchangeRateIsUpdated(bool exchange_rate_is_updated = true);
		
		bool formatHasChanged() const;
		void setFormatHasChanged(bool format_has_changed = true);
		
		bool nameHasChanged() const;
		void setNameHasChanged(bool name_has_changed = true);
		
		bool symbolHasChanged() const;
		void setSymbolHasChanged(bool symbol_has_changed = true);
	
};

bool currency_list_less_than(Currency *c1, Currency *c2);
template<class type> class CurrencyList : public EqonomizeList<type> {
	public:
		CurrencyList() : EqonomizeList<type>() {};
		void sort() {
			qSort(QList<type>::begin(), QList<type>::end(), currency_list_less_than);
		}
		void inSort(type value) {
			QList<type>::insert(qLowerBound(QList<type>::begin(), QList<type>::end(), value, currency_list_less_than), value);
		}
};


#endif

