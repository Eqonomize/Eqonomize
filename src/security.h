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

#ifndef SECURITY_H
#define SECURITY_H

#include <qstring.h>
#include <qdatetime.h>
#include <qmap.h>
#include <QList>

#include "transaction.h"

class AssetsAccount;
class QXmlStreamReader;
class QXmlStreamWriter;
class QXmlStreamAttributes;
class Security;
class Budget;

typedef enum {
	SECURITY_TYPE_BOND,
	SECURITY_TYPE_STOCK,
	SECURITY_TYPE_MUTUAL_FUND,
	SECURITY_TYPE_OTHER
} SecurityType;

class ReinvestedDividend {
	public:
		ReinvestedDividend(const QDate &date_, double shares_) : date(date_), shares(shares_) {}
		QDate date;
		double shares;
};

template<class type> class EqonomizeList : public QList<type> {
	protected:
		bool b_auto_delete;
		int i_index;
	public:
		EqonomizeList() : QList<type>(), b_auto_delete(false), i_index(0) {};
		virtual ~EqonomizeList () {}
		void setAutoDelete(bool b) {
			b_auto_delete = b;
		}
		void clear() {
			if(b_auto_delete) {
				while(!QList<type>::isEmpty()) delete QList<type>::takeFirst();
			} else {
				QList<type>::clear();
			}
		}		
		bool removeRef(type value) {
			if(b_auto_delete) delete value;
			return QList<type>::removeOne(value);
		}		
		type first() {
			i_index = 0;
			if(QList<type>::isEmpty()) return NULL;
			return QList<type>::first();
		}
		type last() {
			i_index = QList<type>::count();
			if(QList<type>::isEmpty()) return NULL;
			i_index--;
			return QList<type>::at(i_index);
		}
		type getFirst() {
			return QList<type>::first();
		}
		type next() {
			i_index++;
			if(i_index >= QList<type>::count()) return NULL;
			return QList<type>::at(i_index);
		}
		type current() {
			if(i_index >= QList<type>::count()) return NULL;
			return QList<type>::at(i_index);
		}
		type previous() {			
			if(i_index == 0) return NULL;
			i_index--;
			return QList<type>::at(i_index);
		}
};
static bool security_transaction_list_less_than(Transaction *t1, Transaction *t2) {
	return t1->date() < t2->date();
}
template<class type> class SecurityTransactionList : public EqonomizeList<type> {
	public:
		SecurityTransactionList() : EqonomizeList<type>() {};
		void sort() {
			qSort(QList<type>::begin(), QList<type>::end(), security_transaction_list_less_than);
		}
		void inSort(type value) {
			QList<type>::append(value);
			sort();
		}
};
static bool scheduled_security_list_less_than(ScheduledTransaction *t1, ScheduledTransaction *t2) {
	return t1->date() < t2->date();
}

template<class type> class ScheduledSecurityTransactionList : public EqonomizeList<type> {
	public:
		ScheduledSecurityTransactionList() : EqonomizeList<type>() {};
		void sort() {
			qSort(QList<type>::begin(), QList<type>::end(), scheduled_security_list_less_than);
		}
		void inSort(type value) {
			QList<type>::append(value);
			sort();
		}
};
static bool reinvested_dividend_list_less_than(ReinvestedDividend *t1, ReinvestedDividend *t2) {
	return t1->date < t2->date;
}
template<class type> class ReinvestedDividendList : public EqonomizeList<type> {
	public:
		ReinvestedDividendList() : EqonomizeList<type>() {};
		void sort() {
			qSort(QList<type>::begin(), QList<type>::end(), reinvested_dividend_list_less_than);
		}
		void inSort(type value) {
			QList<type>::append(value);
			sort();
		}
};
static bool traded_shares_list_less_than(SecurityTrade *t1, SecurityTrade *t2) {
	return t1->date < t2->date;
}
template<class type> class TradedSharesList : public EqonomizeList<type> {
	public:
		TradedSharesList() : EqonomizeList<type>() {};
		void sort() {
			qSort(QList<type>::begin(), QList<type>::end(), traded_shares_list_less_than);
		}
		void inSort(type value) {
			QList<type>::append(value);
			sort();
		}
};

class Security {

	protected:

		Budget *o_budget;

		int i_id;

		AssetsAccount *o_account;

		SecurityType st_type;

		double d_initial_shares;

		int i_decimals, i_quotation_decimals;

		QString s_name;
		QString s_description;

		void init();

	public:

		Security(Budget *parent_budget, AssetsAccount *parent_account, SecurityType initial_type, double initial_shares = 0.0, int initial_decimals = -1, int initial_quotation_decimals = -1, QString initial_name = QString::null, QString initial_description = QString::null);
		Security(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		Security(Budget *parent_budget);
		Security();
		Security(const Security *security);
		virtual ~Security();

		virtual void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		virtual bool readElement(QXmlStreamReader *xml, bool *valid);
		virtual bool readElements(QXmlStreamReader *xml, bool *valid);
		virtual void save(QXmlStreamWriter *xml);
		virtual void writeAttributes(QXmlStreamAttributes *attr);
		virtual void writeElements(QXmlStreamWriter *xml);
		const QString &name() const;
		void setName(QString new_name);
		const QString &description() const;
		void setDescription(QString new_description);
		Budget *budget() const;
		double initialBalance() const;
		double initialShares() const;
		void setInitialShares(double initial_shares);
		virtual SecurityType type() const;
		void setType(SecurityType new_type);
		int id() const;
		void setId(int new_id);
		bool hasQuotation(const QDate &date) const;
		void setQuotation(const QDate &date, double value, bool auto_added = false);
		void removeQuotation(const QDate &date, bool auto_added = false);
		void clearQuotations();
		double getQuotation(const QDate &date, QDate *actual_date = NULL) const;
		AssetsAccount *account() const;
		void addReinvestedDividend(const QDate &date, double added_shares);
		int decimals() const;
		int quotationDecimals() const;
		void setDecimals(int new_decimals);
		void setQuotationDecimals(int new_decimals);
		void setAccount(AssetsAccount *new_account);

		double shares();
		double shares(const QDate &date, bool estimate = false, bool no_scheduled_shares = false);
		double value();
		double value(const QDate &date, bool estimate = false, bool no_scheduled_shares = false);
		double cost();
		double cost(const QDate &date, bool no_scheduled_shares = false);
		double profit();
		double profit(const QDate &date, bool estimate = false, bool no_scheduled_shares = false);
		double profit(const QDate &date1, const QDate &date2, bool estimate = false, bool no_scheduled_shares = false);
		double yearlyRate();
		double yearlyRate(const QDate &date);
		double yearlyRate(const QDate &date_from, const QDate &date_to);
		double expectedQuotation(const QDate &date);

		QMap<QDate, double> quotations;
		QMap<QDate, bool> quotations_auto;
		TradedSharesList<SecurityTrade*> tradedShares;
		ReinvestedDividendList<ReinvestedDividend*> reinvestedDividends;
		SecurityTransactionList<SecurityTransaction*> transactions;
		SecurityTransactionList<Income*> dividends;
		ScheduledSecurityTransactionList<ScheduledTransaction*> scheduledTransactions;
		ScheduledSecurityTransactionList<ScheduledTransaction*> scheduledDividends;

};

#endif
