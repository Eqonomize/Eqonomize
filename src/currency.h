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
#include <QCoreApplication>

class QXmlStreamReader;
class QXmlStreamWriter;
class QXmlStreamAttributes;

class Currency {
	
	Q_DECLARE_TR_FUNCTIONS(Currency)
	
	protected:
	
		double d_rate;
		int i_decimals;
		int b_precedes;
		QString s_code, s_symbol, s_name;
	
	public:
		
		Currency();
		Currency(QString initial_code, QString initial_symbol, QString initial_name, double initial_rate);
		~Currency();
		Currency *copy() const;
	
		double exchangeRate() const;
		void setExchangeRate(double new_rate);
		
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
		void setSymbolPrecedes(bool new_precedes);
		
		int fractionalDigits() const;
		void setFractionalDigits(int new_frac_digits);
	
};

#endif
