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
#include <QDebug>

#include "budget.h"
#include "currency.h"

Currency::Currency(Budget *parent_budget) {
	o_budget = parent_budget;
	i_decimals = -1;
	b_precedes = -1;
	r_source = EXCHANGE_RATE_SOURCE_NONE;
	b_local_rate = true; b_local_name = true; b_local_symbol = true; b_local_format = true;
}
Currency::Currency() {
	o_budget = NULL;
	i_decimals = -1;
	b_precedes = -1;
	r_source = EXCHANGE_RATE_SOURCE_NONE;
	b_local_rate = true; b_local_name = true; b_local_symbol = true; b_local_format = true;
}
Currency::Currency(Budget *parent_budget, QString initial_code, QString initial_symbol, QString initial_name, double initial_rate, QDate date, int initial_decimals, int initial_precedes) {
	o_budget = parent_budget;
	s_code = initial_code;
	s_symbol = initial_symbol;
	s_name = initial_name;
	if(!date.isValid() && initial_rate != 1.0) date = QDate::currentDate();
	if(date.isValid()) rates[date] = initial_rate;
	i_decimals = initial_decimals;
	b_precedes = initial_precedes;
	if(i_decimals < 0) i_decimals = -1;
	b_local_rate = true; b_local_name = true; b_local_symbol = true; b_local_format = true;
	r_source = EXCHANGE_RATE_SOURCE_NONE;
}
Currency::Currency(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) {
	o_budget = parent_budget;
	i_decimals = -1;
	b_precedes = -1;
	b_local_rate = false; b_local_name = false; b_local_symbol = false; b_local_format = false;
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
Currency::~Currency() {}
Currency *Currency::copy() const {
	Currency *this_copy = new Currency(o_budget, s_code, s_symbol, s_name, 1.0, QDate(), b_precedes, i_decimals);
	this_copy->rates = rates;
	this_copy->setExchangeRateIsUpdated(b_local_rate);
	this_copy->setNameHasChanged(b_local_name);
	this_copy->setSymbolHasChanged(b_local_symbol);
	this_copy->setFormatHasChanged(b_local_format);
	return this_copy;
}

bool Currency::merge(Currency *currency) {
	bool has_changed = false;
	if(!currency->name().isEmpty() && currency->name() != s_name) {
		has_changed = true;
		b_local_name = true;
		s_name = currency->name();
	}
	if(!currency->symbol().isEmpty() && currency->symbol() != s_symbol) {
		has_changed = true;
		b_local_symbol = true;
		s_symbol = currency->symbol();
	}
	QMap<QDate, double>::const_iterator it = currency->rates.constBegin();
	while (it != currency->rates.constEnd()) {
		rates[it.key()] = it.value();
		++it;
	}
	b_local_rate = true;
	has_changed = true;
	if(currency->fractionalDigits(false) >= 0 && currency->fractionalDigits(false) != i_decimals) {
		has_changed = true;
		b_local_format = true;
		i_decimals = currency->fractionalDigits(false);
	}
	if(currency->symbolPrecedes(false) >= 0 && currency->symbolPrecedes(false) != b_precedes) {
		has_changed = true;
		b_local_format = true;
		b_precedes = currency->symbolPrecedes(false);
	}
	return has_changed;
}

void Currency::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	s_name = attr->value("name").trimmed().toString();
	s_code = attr->value("code").trimmed().toString();
	if(s_code.isEmpty() && valid) *valid = false;
	s_symbol = attr->value("symbol").trimmed().toString();
	QString s_source = attr->value("source").trimmed().toString();
	if(s_source == "ECB") r_source = EXCHANGE_RATE_SOURCE_ECB;
	else r_source = EXCHANGE_RATE_SOURCE_NONE;
	if(attr->hasAttribute("decimals")) i_decimals = attr->value("decimals").toInt();
	if(attr->hasAttribute("precedes")) b_precedes = attr->value("precedes").toInt();
	if(i_decimals < 0) i_decimals = -1;
}
bool Currency::readElement(QXmlStreamReader *xml, bool*) {
	if(xml->name() == "rate") {
		QXmlStreamAttributes attr = xml->attributes();
		QDate date = QDate::fromString(attr.value("date").toString(), Qt::ISODate);
		if(date.isValid()) rates[date] = attr.value("value").toDouble();
		return false;
	}
	return false;
}
bool Currency::readElements(QXmlStreamReader *xml, bool *valid) {
	while(xml->readNextStartElement()) {
		if(!readElement(xml, valid)) xml->skipCurrentElement();
	}
	return true;
}
void Currency::save(QXmlStreamWriter *xml, bool local_save) {
	QXmlStreamAttributes attr;
	writeAttributes(&attr, local_save);
	xml->writeAttributes(attr);
	writeElements(xml, local_save);
}
void Currency::writeAttributes(QXmlStreamAttributes *attr, bool local_save) {
	attr->append("code", s_code);
	if((!local_save || b_local_symbol) && !s_symbol.isEmpty()) attr->append("symbol", s_symbol);
	if((!local_save || b_local_name) && !s_name.isEmpty()) attr->append("name", s_name);
	if((!local_save || b_local_format) && i_decimals >= 0) attr->append("decimals", QString::number(i_decimals));
	if((!local_save || b_local_format) && b_precedes >= 0) attr->append("precedes", QString::number(b_precedes));
	if(!local_save && r_source == EXCHANGE_RATE_SOURCE_ECB) attr->append("source", "ECB");
}
void Currency::writeElements(QXmlStreamWriter *xml, bool) {
	QMap<QDate, double>::const_iterator it = rates.constBegin();
	while (it != rates.constEnd()) {
		xml->writeStartElement("rate");
		xml->writeAttribute("value", QString::number(it.value(), 'f', 5));
		xml->writeAttribute("date", it.key().toString(Qt::ISODate));
		xml->writeEndElement();
		++it;
	}
}

double Currency::exchangeRate(QDate date, bool exact_match) const {
	if(exact_match) {
		QMap<QDate, double>::const_iterator it = rates.find(date);
		if(it == rates.constEnd()) return -1.0;
		return it.value();
	}
	if(rates.isEmpty()) return 1.0;
	if(!date.isValid()) return rates.last();
	QMap<QDate, double>::const_iterator it = rates.lowerBound(date);
	if(it == rates.constEnd()) return rates.last();
	if(it.key() != date && it != rates.constBegin()) {
		QMap<QDate, double>::const_iterator it2 = it;
		--it2;
		if(date.daysTo(it.key()) >= it2.key().daysTo(date)) it = it2;
	}
	return it.value();
}
QDate Currency::lastExchangeRateDate() const {
	if(rates.isEmpty()) return QDate();
	return rates.lastKey();
}
void Currency::setExchangeRate(double new_rate, QDate date) {
	if(!date.isValid()) date = QDate::currentDate();
	rates[date] = new_rate;
	b_local_rate = true;
}

ExchangeRateSource Currency::exchangeRateSource() const {
	return r_source;
}
void Currency::setExchangeRateSource(ExchangeRateSource source) {
	r_source = source;
}

double Currency::convertTo(double value, const Currency *to_currency) const {
	if(to_currency == this) return value;
	if(rates.isEmpty()) return value * to_currency->exchangeRate();
	return value / rates.last() * to_currency->exchangeRate();
}
double Currency::convertFrom(double value, const Currency *from_currency) const {
	if(from_currency == this) return value;
	return from_currency->convertTo(value, this);
}
double Currency::convertTo(double value, const Currency *to_currency, const QDate &date) const {
	if(to_currency == this) return value;
	if(rates.isEmpty()) return value * to_currency->exchangeRate(date);
	if(!date.isValid()) return value / rates.last() * to_currency->exchangeRate(date);
	QMap<QDate, double>::const_iterator it = rates.lowerBound(date);	
	if(it == rates.constEnd()) return value / rates.last() * to_currency->exchangeRate(date);
	if(it.key() != date && it != rates.constBegin()) {
		QMap<QDate, double>::const_iterator it2 = it;
		--it2;
		if(date.daysTo(it.key()) >= it2.key().daysTo(date)) it = it2;
	}
	return value / it.value() * to_currency->exchangeRate(date);
}
double Currency::convertFrom(double value, const Currency *from_currency, const QDate &date) const {
	if(from_currency == this) return value;
	return from_currency->convertTo(value, this, date);
}

QString Currency::formatValue(double value, int nr_of_decimals, bool show_currency) const {
	if(nr_of_decimals < 0) {
		if(i_decimals < 0) nr_of_decimals = MONETARY_DECIMAL_PLACES;
		else nr_of_decimals = i_decimals;
	}
	if(!show_currency) return QLocale().toString(value, 'f', nr_of_decimals);
	if((this == o_budget->defaultCurrency()) && !s_symbol.isEmpty()) {
		if((b_precedes < 0 && currency_symbol_precedes()) || b_precedes > 0) {
			return s_symbol + QLocale().toString(value, 'f', nr_of_decimals);
		}
		return QLocale().toString(value, 'f', nr_of_decimals) + " " + s_symbol;
	}	
	return QLocale().toString(value, 'f', nr_of_decimals) + " " + ((this != o_budget->defaultCurrency() || s_symbol.isEmpty()) ? s_code : s_symbol);
}

const QString &Currency::code() const {
	return s_code;
}
const QString &Currency::symbol(bool return_code_if_empty) const {
	if(return_code_if_empty && s_symbol.isEmpty())  return s_code;
	return s_symbol;
}
const QString &Currency::name(bool return_code_if_empty) const {
	if(return_code_if_empty && s_name.isEmpty()) return s_code;
	return s_name;
}
void Currency::setCode(QString new_code) {
	s_code = new_code;
}
void Currency::setSymbol(QString new_symbol) {
	s_symbol = new_symbol;
	b_local_symbol = true;
}
void Currency::setName(QString new_name) {
	s_name = new_name;
	b_local_name = true;
}

int Currency::symbolPrecedes(bool return_default_if_unset) const {
	if(return_default_if_unset && b_precedes < 0) return currency_symbol_precedes();
	return b_precedes;
}
void Currency::setSymbolPrecedes(int new_precedes) {
	b_precedes = new_precedes;
	b_local_format = true;
}

int Currency::fractionalDigits(bool return_default_if_unset) const {
	if(return_default_if_unset && i_decimals < 0) return MONETARY_DECIMAL_PLACES;
	return i_decimals;
}
void Currency::setFractionalDigits(int new_frac_digits) {
	i_decimals = new_frac_digits;
	if(i_decimals < 0) i_decimals = -1;
	b_local_format = true;
}
bool Currency::hasLocalChanges() const {return b_local_format || b_local_rate || b_local_name || b_local_symbol;}
void Currency::setAsLocal() {b_local_format = true; b_local_rate = true; b_local_name = true; b_local_symbol = true;}
bool Currency::exchangeRateIsUpdated() const {return b_local_rate;}
void Currency::setExchangeRateIsUpdated(bool exchange_rate_is_updated) {b_local_rate = exchange_rate_is_updated;}
bool Currency::formatHasChanged() const {return b_local_format;}
void Currency::setFormatHasChanged(bool format_has_changed) {b_local_format = format_has_changed;}
bool Currency::nameHasChanged() const {return b_local_name;}
void Currency::setNameHasChanged(bool name_has_changed) {b_local_name = name_has_changed;}
bool Currency::symbolHasChanged() const {return b_local_symbol;}
void Currency::setSymbolHasChanged(bool symbol_has_changed) {b_local_symbol = symbol_has_changed;}

bool currency_list_less_than(Currency *c1, Currency *c2) {
	return QString::localeAwareCompare(c1->code(), c2->code()) < 0;
}

