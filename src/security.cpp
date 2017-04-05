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

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QXmlStreamAttribute>
#include <QDebug>

#include "budget.h"
#include "recurrence.h"
#include "security.h"

#include <cmath>

void Security::init() {
	transactions.setAutoDelete(false);
	dividends.setAutoDelete(false);
	scheduledTransactions.setAutoDelete(false);
	scheduledDividends.setAutoDelete(false);
	tradedShares.setAutoDelete(false);
	reinvestedDividends.setAutoDelete(true);
}
Security::Security(Budget *parent_budget, AssetsAccount *parent_account, SecurityType initial_type, double initial_shares, int initial_decimals, int initial_quotation_decimals, QString initial_name, QString initial_description) : o_budget(parent_budget), i_id(-1), o_account(parent_account), st_type(initial_type), d_initial_shares(initial_shares), i_decimals(initial_decimals), i_quotation_decimals(initial_quotation_decimals), s_name(initial_name.trimmed()), s_description(initial_description) {
	init();
}
Security::Security(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : o_budget(parent_budget) {
	init();
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
Security::Security(Budget *parent_budget) : o_budget(parent_budget) {}
Security::Security() : o_budget(NULL), i_id(-1), o_account(NULL), st_type(SECURITY_TYPE_STOCK), d_initial_shares(0.0), i_decimals(-1), i_quotation_decimals(-1) {init();}
Security::Security(const Security *security) : o_budget(security->budget()), i_id(security->id()), o_account(security->account()), st_type(security->type()), d_initial_shares(security->initialShares()), i_decimals(security->decimals()), i_quotation_decimals(security->quotationDecimals()), s_name(security->name()), s_description(security->description()) {init();}
Security::~Security() {}

void Security::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	i_id = attr->value("id").toInt();
	QStringRef type = attr->value("type");
	if(type == "bond") {
		st_type = SECURITY_TYPE_BOND;
	} else if(type == "stock") {
		st_type = SECURITY_TYPE_STOCK;
	} else if(type == "mutual fund") {
		st_type = SECURITY_TYPE_MUTUAL_FUND;
	} else {
		st_type = SECURITY_TYPE_OTHER;
	}
	d_initial_shares = attr->value("initialshares").toDouble();
	if(attr->hasAttribute("decimals")) i_decimals = attr->value("decimals").toInt();
	else i_decimals = -1;
	if(attr->hasAttribute("quotationdecimals")) i_quotation_decimals = attr->value("quotationdecimals").toInt();
	else i_quotation_decimals = -1;
	s_name = attr->value("name").trimmed().toString();
	s_description = attr->value("description").toString();
	int id_account = attr->value("account").toInt();
	if(budget()->assetsAccounts_id.contains(id_account)) {
		o_account = budget()->assetsAccounts_id[id_account];
		if(valid && (*valid)) *valid = (o_account->accountType() == ASSETS_TYPE_SECURITIES);
	} else {
		o_account = NULL;
		if(valid) *valid = false;
	}
}
bool Security::readElement(QXmlStreamReader *xml, bool*) {
	if(xml->name() == "quotation") {
		QXmlStreamAttributes attr = xml->attributes();
		QDate date = QDate::fromString(attr.value("date").toString(), Qt::ISODate);
		quotations[date] = attr.value("value").toDouble();
		quotations_auto[date] = attr.value("auto").toInt();
	}
	return false;
}
bool Security::readElements(QXmlStreamReader *xml, bool *valid) {
	while(xml->readNextStartElement()) {
		if(!readElement(xml, valid)) xml->skipCurrentElement();
	}
	return true;
}
void Security::save(QXmlStreamWriter *xml) {
	QXmlStreamAttributes attr;
	writeAttributes(&attr);
	xml->writeAttributes(attr);
	writeElements(xml);
}
void Security::writeAttributes(QXmlStreamAttributes *attr) {
	attr->append("id", QString::number(i_id));
	attr->append("name", s_name);
	switch(st_type) {
		case SECURITY_TYPE_BOND: {attr->append("type", "bond"); break;}
		case SECURITY_TYPE_STOCK: {attr->append("type", "stock"); break;}
		case SECURITY_TYPE_MUTUAL_FUND: {attr->append("type", "mutual fund"); break;}
		case SECURITY_TYPE_OTHER: {attr->append("type", "other"); break;}
	}
	if(i_decimals >= 0) attr->append("decimals", QString::number(i_decimals));
	if(i_quotation_decimals >= 0) attr->append("quotationdecimals", QString::number(i_quotation_decimals));
	attr->append("initialshares", QString::number(d_initial_shares, 'f', i_decimals));
	if(!s_description.isEmpty()) attr->append("description", s_description);
	attr->append("account", QString::number(o_account->id()));
}
void Security::writeElements(QXmlStreamWriter *xml) {
	QMap<QDate, double>::const_iterator it_end = quotations.end();
	QMap<QDate, double>::const_iterator it = quotations.begin();
	QMap<QDate, bool>::const_iterator it_auto = quotations_auto.begin();
	for(; it != it_end; ++it, ++it_auto) {
		xml->writeStartElement("quotation");
		xml->writeAttribute("value", QString::number(it.value(), 'f', quotationDecimals() > SAVE_MONETARY_DECIMAL_PLACES ?  quotationDecimals() : SAVE_MONETARY_DECIMAL_PLACES));
		xml->writeAttribute("date", it.key().toString(Qt::ISODate));
		if(it_auto.value()) xml->writeAttribute("auto", QString::number(it_auto.value()));
		xml->writeEndElement();
	}
}
const QString &Security::name() const {return s_name;}
void Security::setName(QString new_name) {s_name = new_name.trimmed(); o_budget->securityNameModified(this);}
const QString &Security::description() const {return s_description;}
void Security::setDescription(QString new_description) {s_description = new_description;}
Budget *Security::budget() const {return o_budget;}
double Security::initialBalance() const {
	QMap<QDate, double>::const_iterator it = quotations.begin();
	if(it == quotations.end()) return 0.0;
	return it.value() * d_initial_shares;
}
double Security::initialShares() const {return d_initial_shares;}
void Security::setInitialShares(double initial_shares) {d_initial_shares = initial_shares;}
SecurityType Security::type() const {return st_type;}
void Security::setType(SecurityType new_type) {st_type = new_type;}
AssetsAccount *Security::account() const {return o_account;}
Currency *Security::currency() const {
	if(o_account) return o_account->currency();
	return budget()->defaultCurrency();
}
void Security::setAccount(AssetsAccount *new_account) {o_account = new_account;}
int Security::id() const {return i_id;}
void Security::setId(int new_id) {i_id = new_id;}
void Security::setQuotation(const QDate &date, double value, bool auto_added) {
	if(!auto_added) {
		quotations[date] = value;
		quotations_auto[date] = false;
	} else if(!quotations.count(date) || quotations_auto[date]) {
		quotations[date] = value;
		quotations_auto[date] = true;
	}
}
void Security::removeQuotation(const QDate &date, bool auto_added) {
	if(quotations.count(date) && (!auto_added || quotations_auto[date])) {
		quotations.remove(date);
		quotations_auto.remove(date);
	}
}
void Security::clearQuotations() {
	quotations.clear();
	quotations_auto.clear();
}
double Security::getQuotation(const QDate &date, QDate *actual_date) const {
	if(quotations.contains(date)) {
		if(actual_date) *actual_date = date;
		return quotations[date];
	}
	QMap<QDate, double>::const_iterator it_begin = quotations.begin();
	QMap<QDate, double>::const_iterator it = quotations.end();
	if(it != it_begin) {
		--it;
		while(true) {
			if(it.key() <= date) {
				if(actual_date) *actual_date = it.key();
				return it.value();
			}
			if(it == it_begin) break;
			--it;
		}
	}
	if(actual_date) *actual_date = QDate();
	return 0.0;
}
bool Security::hasQuotation(const QDate &date) const {
	return quotations.contains(date);
}
void Security::addReinvestedDividend(const QDate &date, double added_shares) {
	reinvestedDividends.inSort(new ReinvestedDividend(date, added_shares));
}
int Security::decimals() const {
	if(i_decimals < 0) return o_budget->defaultShareDecimals();
	return i_decimals;
}
int Security::quotationDecimals() const {
	if(i_quotation_decimals < 0) return o_budget->defaultQuotationDecimals();
	return i_quotation_decimals;
}
void Security::setDecimals(int new_decimals) {i_decimals = new_decimals;}
void Security::setQuotationDecimals(int new_decimals) {i_quotation_decimals = new_decimals;}

double Security::shares() {
	double n = d_initial_shares;
	SecurityTransaction *trans = transactions.first();
	while(trans) {
		if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY) n += trans->shares();
		else n -= trans->shares();
		trans = transactions.next();
	}
	SecurityTrade *ts = tradedShares.first();
	while(ts) {
		if(ts->from_security == this) n -= ts->from_shares;
		else n += ts->to_shares;
		ts = tradedShares.next();
	}
	ReinvestedDividend *rediv = reinvestedDividends.first();
	while(rediv) {
		n += rediv->shares;
		rediv = reinvestedDividends.next();
	}
	return n;
}
double Security::shares(const QDate &date, bool estimate, bool no_scheduled_shares) {
	double n = 0.0;
	if(quotations.begin() == quotations.end() || quotations.begin().key() <= date) n += d_initial_shares;
	SecurityTransaction *trans = transactions.first();
	while(trans && trans->date() <= date) {
		if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY) n += trans->shares();
		else n -= trans->shares();
		trans = transactions.next();
	}
	SecurityTrade *ts = tradedShares.first();
	while(ts && ts->date <= date) {
		if(ts->from_security == this) n -= ts->from_shares;
		else n += ts->to_shares;
		ts = tradedShares.next();
	}
	if(!no_scheduled_shares) {
		ScheduledTransaction *strans = scheduledTransactions.first();
		while(strans && strans->date() <= date) {
			int no = strans->recurrence()->countOccurrences(date);
			if(no > 0) {
				if(strans->transactiontype() == TRANSACTION_TYPE_SECURITY_BUY) n += ((SecurityTransaction*) strans->transaction())->shares() * no;
				else n -= ((SecurityTransaction*) strans->transaction())->shares() * no;
			}
			strans = scheduledTransactions.next();
		}
	}
	ReinvestedDividend *rediv = reinvestedDividends.first();
	while(rediv && rediv->date <= date) {
		n += rediv->shares;
		rediv = reinvestedDividends.next();
	}
	if(estimate && date > QDate::currentDate() && reinvestedDividends.count() > 0) {		
		QDate date1 = QDate::currentDate();
		int days = date1.daysTo(date);
		int days2 = 0;
		if(quotations.begin() != quotations.end()) days2 = quotations.begin().key().daysTo(date1);
		if(days2 > days) {
			days2 = days;
		}
		int years = days2 / date1.daysInYear();
		if(years == 0) years = 1;
		QDate date2 = date1.addYears(-years);
		double n2 = 0.0;
		rediv = reinvestedDividends.first();
		while(trans) {
			if(rediv->date > date2) {
				n2 += rediv->shares;
			}
			rediv = reinvestedDividends.next();
		}
		if(n2 > 0.0) {
			n += (n2 * days) / date2.daysTo(date1);
		}
	}
	return n;
}
double Security::value() {
	return shares() * getQuotation(QDate::currentDate());
}
double Security::value(const QDate &date, bool estimate, bool no_scheduled_shares) {
	if(estimate && date > QDate::currentDate()) return shares(date, estimate, no_scheduled_shares) * expectedQuotation(date);
	return shares(date, estimate, no_scheduled_shares) * getQuotation(date);
}
double Security::cost(const QDate &date, bool no_scheduled_shares, Currency *cur) {
	if(!cur) cur = currency();
	double c = d_initial_shares;
	QMap<QDate, double>::const_iterator it = quotations.begin();
	if(it == quotations.end() || date < it.key()) {
		c = 0.0;
	} else {
		c *= it.value();
		if(cur != currency()) {
			if(budget()->defaultTransactionConversionRateDate() == TRANSACTION_CONVERSION_RATE_AT_DATE) {
				c = currency()->convertTo(c, cur, it.key());
			} else {
				c = currency()->convertTo(c, cur, date);
			}
		}
	}
	SecurityTransaction *trans = transactions.first();
	while(trans && trans->date() <= date) {
		double v = trans->value();
		if(cur != trans->currency()) {
			if(budget()->defaultTransactionConversionRateDate() == TRANSACTION_CONVERSION_RATE_AT_DATE) {
				v = trans->currency()->convertTo(v, cur, trans->date());
			} else {
				v = trans->currency()->convertTo(v, cur, date);
			}
		}
		if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY) c += v;
		else c -= v;
		trans = transactions.next();
	}
	SecurityTrade *ts = tradedShares.first();
	while(ts && ts->date <= date) {
		if(ts->from_security == this) c -= ts->value;
		else c += ts->value;
		ts = tradedShares.next();
	}
	if(!no_scheduled_shares) {
		ScheduledTransaction *strans = scheduledTransactions.first();
		while(strans && strans->date() <= date) {
			int n = strans->recurrence()->countOccurrences(date);
			if(n > 0) {
				double v = strans->value();
				if(cur != strans->currency()) {
					v = strans->currency()->convertTo(v, cur);
				}
				if(strans->transactiontype() == TRANSACTION_TYPE_SECURITY_BUY) c += v * n;
				else c -= v * n;
			}
			strans = scheduledTransactions.next();
		}
	}
	return c;
}
double Security::cost(Currency *cur) {
	if(!cur) cur = currency();
	double c = d_initial_shares;
	QMap<QDate, double>::const_iterator it = quotations.begin();
	if(it == quotations.end()) {
		c = 0.0;
	} else {
		c *= it.value();
		if(cur != currency()) {
			if(budget()->defaultTransactionConversionRateDate() == TRANSACTION_CONVERSION_RATE_AT_DATE) {
				c = currency()->convertTo(c, cur, it.key());
			} else {
				c = currency()->convertTo(c, cur);
			}
		}
	}
	SecurityTransaction *trans = transactions.first();
	while(trans) {
		double v = trans->value();
		if(cur != trans->currency()) {
			if(budget()->defaultTransactionConversionRateDate() == TRANSACTION_CONVERSION_RATE_AT_DATE) {
				v = trans->currency()->convertTo(v, cur, trans->date());
			} else {
				v = trans->currency()->convertTo(v, cur);
			}
		}
		if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY) c += v;
		else c -= v;
		trans = transactions.next();
	}
	SecurityTrade *ts = tradedShares.first();
	while(ts) {
		if(ts->from_security == this) c -= ts->value;
		else c += ts->value;
		ts = tradedShares.next();
	}
	return c;
}
double Security::profit(Currency *cur) {
	if(!cur) cur = currency();
	double p = value();
	if(cur != currency()) {
		p = currency()->convertTo(p, cur);
	}
	p -= cost(cur);
	Income *trans = dividends.first();
	while(trans) {
		if(cur != trans->currency()) {
			if(budget()->defaultTransactionConversionRateDate() == TRANSACTION_CONVERSION_RATE_AT_DATE) {
				p += trans->currency()->convertTo(trans->income(), cur, trans->date());
			} else {
				p += trans->currency()->convertTo(trans->income(), cur);
			}
		} else {
			p += trans->income();
		}
		trans = dividends.next();
	}
	return p;
}
double Security::profit(const QDate &date, bool estimate, bool no_scheduled_shares, Currency *cur) {
	if(!cur) cur = currency();
	double p = value(date, estimate, no_scheduled_shares);
	if(cur != currency()) {
		p = currency()->convertTo(p, cur, date);
	}
	p -= cost(date, no_scheduled_shares, cur);
	Income *trans = dividends.first();
	while(trans && trans->date() <= date) {
		if(cur != trans->currency()) {
			if(budget()->defaultTransactionConversionRateDate() == TRANSACTION_CONVERSION_RATE_AT_DATE) {
				p += trans->currency()->convertTo(trans->income(), cur, trans->date());
			} else {
				p += trans->currency()->convertTo(trans->income(), cur, date);
			}
		} else {
			p += trans->income();
		}
		trans = dividends.next();
	}
	if(estimate && date > QDate::currentDate() && dividends.count() > 0) {
		QDate date1 = QDate::currentDate();
		int days = date1.daysTo(date);
		int days2 = 0;
		if(quotations.begin() != quotations.end()) days2 = quotations.begin().key().daysTo(date1);
		if(days2 > days) {
			days2 = days;
		}
		int years = days2 / date1.daysInYear();
		if(years == 0) years = 1;
		QDate date2 = date1.addYears(-years);
		double p2 = 0.0;
		trans = dividends.first();
		while(trans) {
			if(trans->date() > date2) {
				if(cur != trans->currency()) {
					p2 += trans->currency()->convertTo(trans->income(), cur);
				} else {
					p2 += trans->income();
				}
			}
			trans = dividends.next();
		}
		if(p2 > 0.0) {
			p += (p2 * days) / date2.daysTo(date1);
		}
	} else if(!no_scheduled_shares) {
		ScheduledTransaction *strans = scheduledDividends.first();
		while(strans && strans->date() <= date) {
			int n = strans->recurrence()->countOccurrences(date);
			if(n > 0) {
				if(cur != strans->currency()) {
					p += strans->currency()->convertTo(strans->value() * n, cur);
				} else {
					p += strans->value() * n;
				}
			}
			strans = scheduledDividends.next();
		}
	}
	return p;
}
double Security::profit(const QDate &date1, const QDate &date2, bool estimate, bool no_scheduled_shares, Currency *cur) {
	return profit(date2, estimate, no_scheduled_shares) - profit(date1, estimate, no_scheduled_shares, cur);
}
double Security::yearlyRate() {
	QMap<QDate, double>::const_iterator it_begin = quotations.begin();
	QMap<QDate, double>::const_iterator it_end = quotations.end();
	if(it_end == it_begin) return 0.0;
	it_end--;
	QDate date1 = it_begin.key(), date2 = QDate::currentDate();
	double q1 = it_begin.value(), q2 = it_end.value();
	int days = date1.daysTo(date2);
	if(it_end.key() != date2 && q1 != q2) {
		int days2 = it_end.key().daysTo(date2);
		q2 *= pow(q2 / q1, days2 / (days - days2));
	}
	Income *trans = dividends.first();
	while(trans) {
		double s = shares(trans->date());
		if(s > 0.0) q2 += trans->income() / s;
		trans = dividends.next();
	}
	double shares_change = 0.0;
	ReinvestedDividend *rediv = reinvestedDividends.first();
	while(rediv) {
		double s = shares(rediv->date);
		if(s > 0.0) shares_change += rediv->shares / (s - rediv->shares);
		rediv = reinvestedDividends.next();
	}
	if(date1 == date2) return 0.0;
	double change = q2 / q1 + shares_change;
	return pow(change, 1 / o_budget->yearsBetweenDates(date1, date2, false)) - 1;
}
double Security::yearlyRate(const QDate &date) {
	QMap<QDate, double>::const_iterator it_begin = quotations.begin();
	if(it_begin == quotations.end()) return 0.0;
	QDate date1 = it_begin.key();
	QDate date2 = date;
	if(date1 >= date2) return 0.0;
	QDate curdate = QDate::currentDate();
	if(date2 > curdate) {
		double rate1 = yearlyRate();
		double rate2 = yearlyRate(curdate, date2);
		int days1 = date1.daysTo(curdate);
		int days2 = curdate.daysTo(date2);
		return ((rate1 * days1) + (rate2 * days2)) / (days1 + days2);
	}
	QDate date2_q;
	double q1 = it_begin.value(), q2 = getQuotation(date2, &date2_q);
	int days = date1.daysTo(date2);
	if(date2 != date2_q && q1 != q2) {
		int days2 = date2_q.daysTo(date2);
		q2 *= pow(q2 / q1, days2 / (days - days2));
	}
	Income *trans = dividends.first();
	while(trans && trans->date() <= date2) {
		double s = shares(trans->date());
		if(s > 0.0) q2 += trans->income() / s;
		trans = dividends.next();
	}
	double shares_change = 0.0;
	ReinvestedDividend *rediv = reinvestedDividends.first();
	while(rediv && rediv->date <= date2) {
		double s = shares(rediv->date);
		if(s > 0.0) shares_change += rediv->shares / (s - rediv->shares);
		rediv = reinvestedDividends.next();
	}
	double change = q2 / q1 + shares_change;
	return pow(change, 1 / o_budget->yearsBetweenDates(date1, date2, false)) - 1;
}
double Security::yearlyRate(const QDate &date1, const QDate &date2) {
	if(date1 > date2) return yearlyRate(date2, date1);
	if(date1 == date2) return 0.0;
	QMap<QDate, double>::const_iterator it_begin = quotations.begin();
	if(it_begin == quotations.end()) return 0.0;
	QDate curdate = QDate::currentDate();
	if(date1 >= curdate) {
		return pow(1 + (profit(date1, date2, true, true) / value(date1, true, true)), 1 / (o_budget->yearsBetweenDates(date1, date2, false))) - 1;
	}
	if(date2 > curdate) {
		double rate1 = yearlyRate(date1, curdate);
		double rate2 = yearlyRate(curdate, date2);
		int days1 = date1.daysTo(curdate);
		int days2 = curdate.daysTo(date2);
		return ((rate1 * days1) + (rate2 * days2)) / (days1 + days2);
	}
	if(date2 < it_begin.key()) {
		return 0.0;
	}
	if(date1 < it_begin.key()) {
		return yearlyRate(it_begin.key(), date2);
	}
	QDate date1_q, date2_q;
	double q1 = getQuotation(date1, &date1_q);
	double q2 = getQuotation(date2, &date2_q);
	int days = date1.daysTo(date2);
	if(q1 != q2) {
		double q1_bak = q1;
		if(date1 != date1_q) {
			double rate = q1 / q2;
			int days1 = date1_q.daysTo(date1);
			q1 *= pow(rate, days1 / (days - days1));
		}
		if(date2 != date2_q) {
			double rate = q2 / q1_bak;
			int days2 = date2_q.daysTo(date2);
			q2 *= pow(rate, days2 / (days - days2));
		}
	}
	Income *trans = dividends.first();
	while(trans && trans->date() <= date2) {
		if(trans->date() >= date1) {
			double s = shares(trans->date());
			if(s > 0.0) q2 += trans->income() / s;
		}
		trans = dividends.next();
	}
	double shares_change = 0.0;
	ReinvestedDividend *rediv = reinvestedDividends.first();
	while(rediv && rediv->date <= date2) {
		if(rediv->date >= date1) {
			double s = shares(rediv->date);
			if(s > 0.0) shares_change += rediv->shares / (s - rediv->shares);
		}
		rediv = reinvestedDividends.next();
	}
	double change = q2 / q1 + shares_change;
	return pow(change, 1 / o_budget->yearsBetweenDates(date1, date2, false)) - 1;
}
double Security::expectedQuotation(const QDate &date) {
	if(quotations.contains(date)) {
		return quotations[date];
	}
	QMap<QDate, double>::const_iterator it_begin = quotations.begin();
	QMap<QDate, double>::const_iterator it = quotations.end();
	if(it == it_begin) return 0.0;
	--it;
	if(it == it_begin) return it_begin.value();
	if(date < it_begin.key()) {		
		int days = date.daysTo(it_begin.key());
		double q2 = expectedQuotation(it_begin.key().addDays(days));
		return it_begin.value() * (it_begin.value() / q2);
	}
	if(it.key() < date) {		
		int days = it.key().daysTo(date);
		double q1 = expectedQuotation(it.key().addDays(-days));
		return it.value() * (it.value() / q1);
	}
	--it;
	while(true) {
		if(it.key() < date) {
			it_begin = it;
			++it;
			int days = it_begin.key().daysTo(date), days2 = it_begin.key().daysTo(it.key());
			if(it.value() == it_begin.value()) return it.value();
			double rate = it.value() / it_begin.value();
			return it_begin.value() * pow(rate, days / days2);
		}
		if(it == it_begin) break;
		--it;
	}
	return 0.0;
}

