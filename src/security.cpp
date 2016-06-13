/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016 by Hanna Knutsson                                        *
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

#include <QDomElement>
#include <QDomNode>

#include <klocalizedstring.h>

#include "budget.h"
#include "recurrence.h"
#include "security.h"

#include <cmath>

extern double yearsBetweenDates(const QDate &date1, const QDate &date2);

void Security::init() {
	transactions.setAutoDelete(false);
	dividends.setAutoDelete(false);
	scheduledTransactions.setAutoDelete(false);
	scheduledDividends.setAutoDelete(false);
	tradedShares.setAutoDelete(false);
	reinvestedDividends.setAutoDelete(true);
}
Security::Security(Budget *parent_budget, AssetsAccount *parent_account, SecurityType initial_type, double initial_shares, int initial_decimals, QString initial_name, QString initial_description) : o_budget(parent_budget), i_id(-1), o_account(parent_account), st_type(initial_type), d_initial_shares(initial_shares), i_decimals(initial_decimals), s_name(initial_name.trimmed()), s_description(initial_description) {
	init();
}
Security::Security(Budget *parent_budget, QDomElement *e, bool *valid) : o_budget(parent_budget) {
	init();
	i_id = e->attribute("id").toInt();
	QString type = e->attribute("type");
	if(type == "bond") {
		st_type = SECURITY_TYPE_BOND;
	} else if(type == "stock") {
		st_type = SECURITY_TYPE_STOCK;
	} else if(type == "mutual fund") {
		st_type = SECURITY_TYPE_MUTUAL_FUND;
	} else {
		st_type = SECURITY_TYPE_OTHER;
	}
	d_initial_shares = e->attribute("initialshares").toDouble();
	i_decimals = e->attribute("decimals", "4").toInt();
	s_name = e->attribute("name").trimmed();
	s_description = e->attribute("description");
	int id_account = e->attribute("account").toInt();
	if(parent_budget->assetsAccounts_id.contains(id_account)) {
		o_account = parent_budget->assetsAccounts_id[id_account];
		if(valid) *valid = (o_account->accountType() == ASSETS_TYPE_SECURITIES);
	} else {
		o_account = NULL;
		if(valid) *valid =false;
	}
	for(QDomNode n = e->firstChild(); !n.isNull(); n = n.nextSibling()) {
		if(n.isElement()) {
			QDomElement e2 = n.toElement();
			if(e2.tagName() == "quotation") {
				QDate date = QDate::fromString(e2.attribute("date"), Qt::ISODate);
				quotations[date] = e2.attribute("value").toDouble();
				quotations_auto[date] = e2.attribute("auto").toInt();
			}
		}
	}
}
Security::Security() : o_budget(NULL), i_id(-1), o_account(NULL), st_type(SECURITY_TYPE_STOCK), d_initial_shares(0.0), i_decimals(4) {init();}
Security::Security(const Security *security) : o_budget(security->budget()), i_id(security->id()), o_account(security->account()), st_type(security->type()), d_initial_shares(security->initialShares()), i_decimals(security->decimals()), s_name(security->name()), s_description(security->description()) {init();}
Security::~Security() {}

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
int Security::decimals() const {return i_decimals;}
void Security::setDecimals(int new_decimals) {i_decimals = new_decimals;}
void Security::save(QDomElement *e) const {
	e->setAttribute("id", i_id);
	e->setAttribute("name", s_name);
	switch(st_type) {
		case SECURITY_TYPE_BOND: {e->setAttribute("type", "bond"); break;}
		case SECURITY_TYPE_STOCK: {e->setAttribute("type", "stock"); break;}
		case SECURITY_TYPE_MUTUAL_FUND: {e->setAttribute("type", "mutual fund"); break;}
		case SECURITY_TYPE_OTHER: {e->setAttribute("type", "other"); break;}
	}
	e->setAttribute("decimals", i_decimals);
	e->setAttribute("initialshares", QString::number(d_initial_shares, 'f', i_decimals));
	if(!s_description.isEmpty()) e->setAttribute("description", s_description);
	e->setAttribute("account", o_account->id());
	QMap<QDate, double>::const_iterator it_end = quotations.end();
	QMap<QDate, double>::const_iterator it = quotations.begin();
	QMap<QDate, bool>::const_iterator it_auto = quotations_auto.begin();
	for(; it != it_end; ++it, ++it_auto) {
		QDomElement e2 = e->ownerDocument().createElement("quotation");
		e2.setAttribute("value", QString::number(it.value(), 'f', MONETARY_DECIMAL_PLACES));
		e2.setAttribute("date", it.key().toString(Qt::ISODate));
		if(it_auto.value()) e2.setAttribute("auto", it_auto.value());
		e->appendChild(e2);
	}
}

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
	if(no_scheduled_shares) {
		ScheduledTransaction *strans = scheduledTransactions.first();
		while(trans && trans->date() <= date) {
			int no = strans->recurrence()->countOccurrences(date);
			if(no > 0) {
				if(strans->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY) n += ((SecurityTransaction*) strans->transaction())->shares() * no;
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
double Security::cost(const QDate &date, bool no_scheduled_shares) {
	double c = d_initial_shares;
	QMap<QDate, double>::const_iterator it = quotations.begin();
	if(it == quotations.end() || date < it.key()) c = 0.0;
	else c *= it.value();
	SecurityTransaction *trans = transactions.first();
	while(trans && trans->date() <= date) {
		if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY) c += trans->value();
		else c -= trans->value();
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
		while(strans && strans->transaction()->date() <= date) {
			int n = strans->recurrence()->countOccurrences(date);
			if(n > 0) {
				if(strans->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY) c += strans->transaction()->value() * n;
				else c -= strans->transaction()->value() * n;
			}
			strans = scheduledTransactions.next();
		}
	}
	return c;
}
double Security::cost() {
	double c = d_initial_shares;
	QMap<QDate, double>::const_iterator it = quotations.begin();
	if(it == quotations.end()) c = 0.0;
	else c *= it.value();
	SecurityTransaction *trans = transactions.first();
	while(trans) {
		if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY) c += trans->value();
		else c -= trans->value();
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
double Security::profit() {
	double p = value() - cost();
	Income *trans = dividends.first();
	while(trans) {
		p += trans->income();
		trans = dividends.next();
	}
	return p;
}
double Security::profit(const QDate &date, bool estimate, bool no_scheduled_shares) {
	double p = value(date, estimate, no_scheduled_shares) - cost(date, no_scheduled_shares);
	Income *trans = dividends.first();
	while(trans && trans->date() <= date) {
		p += trans->income();
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
				p2 += trans->income();
			}
			trans = dividends.next();
		}
		if(p2 > 0.0) {
			p += (p2 * days) / date2.daysTo(date1);
		}
	} else {
		ScheduledTransaction *strans = scheduledDividends.first();
		while(strans && strans->transaction()->date() <= date) {
			int n = strans->recurrence()->countOccurrences(date);
			if(n > 0) {
				p += strans->transaction()->value() * n;
			}
			strans = scheduledDividends.next();
		}
	}
	return p;
}
double Security::profit(const QDate &date1, const QDate &date2, bool estimate, bool no_scheduled_shares) {
	return profit(date2, estimate, no_scheduled_shares) - profit(date1, estimate, no_scheduled_shares);
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
	return pow(change, 1 / yearsBetweenDates(date1, date2)) - 1;
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
	return pow(change, 1 / yearsBetweenDates(date1, date2)) - 1;
}
double Security::yearlyRate(const QDate &date1, const QDate &date2) {
	if(date1 > date2) return yearlyRate(date2, date1);
	if(date1 == date2) return 0.0;
	QMap<QDate, double>::const_iterator it_begin = quotations.begin();
	if(it_begin == quotations.end()) return 0.0;
	QDate curdate = QDate::currentDate();
	if(date1 >= curdate) {
		return pow(1 + (profit(date1, date2, true, true) / value(date1, true, true)), 1 / (yearsBetweenDates(date1, date2))) - 1;
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
	return pow(change, 1 / yearsBetweenDates(date1, date2)) - 1;
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

