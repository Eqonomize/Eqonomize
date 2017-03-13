/***************************************************************************
 *   Copyright (C) 2017 by Hanna Knutsson                                  *
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

#ifndef CURRENCY_H
#define CURRENCY_H

#include <QString>
#include <QDate>
#include <QCoreApplication>

#include "eqonomizelist.h"

class QXmlStreamReader;
class QXmlStreamWriter;
class QXmlStreamAttributes;

class Budget;

typedef enum {
	EXCHANGE_RATE_SOURCE_NONE,
	EXCHANGE_RATE_SOURCE_ECB
} ExchangeRateSource;

class Currency {
	
	Q_DECLARE_TR_FUNCTIONS(Currency)
	
	protected:
	
		double d_rate;
		QDate rate_date;
		int i_decimals;
		int b_precedes;
		QString s_code, s_symbol, s_name;
		ExchangeRateSource r_source;
		Budget *o_budget;
	
	public:
		
		Currency();
		Currency(Budget *parent_budget);
		Currency(Budget *parent_budget, QString initial_code, QString initial_symbol, QString initial_name, double initial_rate, QDate date = QDate());
		Currency(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		~Currency();
		Currency *copy() const;
		
		void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		bool readElement(QXmlStreamReader *xml, bool *valid);
		bool readElements(QXmlStreamReader *xml, bool *valid);
		void save(QXmlStreamWriter *xml);
		void writeAttributes(QXmlStreamAttributes *attr);
		void writeElements(QXmlStreamWriter *xml);
	
		double exchangeRate() const;
		void setExchangeRate(double new_rate, QDate date = QDate());
		
		ExchangeRateSource exchangeRateSource() const;
		void setExchangeRateSource(ExchangeRateSource source);
		
		QDate exchangeRateDate() const;
		
		double convertTo(double value, const Currency *to_currency) const;
		double convertFrom(double value, const Currency *from_currency) const;
		
		QString formatValue(double value, int nr_of_decimals = -1) const;

		const QString &code() const;
		const QString &symbol() const;
		const QString &name() const;
		void setCode(QString new_code);
		void setSymbol(QString new_symbol);
		void setName(QString new_name);
		
		bool symbolPrecedes() const;
		void setSymbolPrecedes(int new_precedes);
		
		int fractionalDigits() const;
		void setFractionalDigits(int new_frac_digits);
	
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
