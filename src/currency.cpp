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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QXmlStreamAttribute>
#include <QLocale>

#include "budget.h"
#include "currency.h"

Currency::Currency() {
	d_rate = 1.0;
	i_decimals = -1;
	b_precedes = -1;
}
Currency::Currency(QString initial_code, QString initial_symbol, QString initial_name, double initial_rate) {
	s_code = initial_code;
	s_symbol = initial_symbol;
	s_name = initial_name;
	d_rate = initial_rate;
	i_decimals = -1;
	b_precedes = -1;
}
Currency::~Currency() {}
Currency *Currency::copy() const {
	Currency *this_copy = new Currency(s_code, s_symbol, s_name, d_rate);
	this_copy->setSymbolPrecedes(b_precedes);
	this_copy->setFractionalDigits(i_decimals);
	return this_copy;
}

double Currency::exchangeRate() const {
	return d_rate;
}
void Currency::setExchangeRate(double new_rate) {
	d_rate = new_rate;
}

double Currency::convertTo(double value, const Currency *to_currency) const {
	return value * d_rate / to_currency->exchangeRate();
}
double Currency::convertFrom(double value, const Currency *from_currency) const {
	return from_currency->convertTo(value, this);
}

QString Currency::formatValue(double value, int nr_of_decimals) const {
	if(nr_of_decimals < 0) {
		if(i_decimals < 0) nr_of_decimals = MONETARY_DECIMAL_PLACES;
		else nr_of_decimals = i_decimals;
	}
	if((b_precedes < 0 && currency_symbol_precedes()) || b_precedes > 0) {
		return (s_symbol.isEmpty() ? s_code : s_symbol) + " " + QLocale().toString(value, 'f', nr_of_decimals);
	}
	return QLocale().toString(value, 'f', nr_of_decimals) + " " + (s_symbol.isEmpty() ? s_code : s_symbol);
}

const QString &Currency::code() const {
	return s_code;
}
const QString &Currency::symbol() const {
	return s_symbol;
}
const QString &Currency::name() const {
	return s_name;
}
void Currency::setCode(QString new_code) {
	s_code = new_code;
}
void Currency::setSymbol(QString new_symbol) {
	s_symbol = new_symbol;
}
void Currency::setName(QString new_name) {
	s_name = new_name;
}

bool Currency::symbolPrecedes() const {
	return b_precedes;
}
void Currency::setSymbolPrecedes(bool new_precedes) {
	b_precedes = new_precedes;
}

int Currency::fractionalDigits() const {
	return i_decimals;
}
void Currency::setFractionalDigits(int new_frac_digits) {
	i_decimals = new_frac_digits;
}

