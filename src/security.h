/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016 by Hanna Knutsson                 *
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

#ifndef SECURITY_H
#define SECURITY_H

#include <QString>
#include <QDateTime>
#include <QMap>
#include <QList>

#include "transaction.h"
#include "eqonomizelist.h"

class AssetsAccount;
class QXmlStreamReader;
class QXmlStreamWriter;
class QXmlStreamAttributes;
class Security;
class Budget;
class Currency;

typedef enum {
	SECURITY_TYPE_BOND,
	SECURITY_TYPE_STOCK,
	SECURITY_TYPE_MUTUAL_FUND,
	SECURITY_TYPE_OTHER
} SecurityType;

template<class type> class SecurityTransactionList : public EqonomizeList<type> {
	public:
		SecurityTransactionList() : EqonomizeList<type>() {};
		static bool security_transaction_list_less_than(Transaction *t1, Transaction *t2) {
			return t1->date() < t2->date();
		}
		void sort() {
			std::sort(QList<type>::begin(), QList<type>::end(), security_transaction_list_less_than);
		}
		void inSort(type value) {
			QList<type>::insert(std::lower_bound(QList<type>::begin(), QList<type>::end(), value, security_transaction_list_less_than), value);
		}
};

template<class type> class ScheduledSecurityTransactionList : public EqonomizeList<type> {
	public:
		ScheduledSecurityTransactionList() : EqonomizeList<type>() {};
		static bool scheduled_security_list_less_than(ScheduledTransaction *t1, ScheduledTransaction *t2) {
			return t1->date() < t2->date();
		}
		void sort() {
			std::sort(QList<type>::begin(), QList<type>::end(), scheduled_security_list_less_than);
		}
		void inSort(type value) {
			QList<type>::insert(std::lower_bound(QList<type>::begin(), QList<type>::end(), value, scheduled_security_list_less_than), value);
		}
};
template<class type> class TradedSharesList : public EqonomizeList<type> {
	public:
		TradedSharesList() : EqonomizeList<type>() {};
		static bool traded_shares_list_less_than(SecurityTrade *t1, SecurityTrade *t2) {
			return t1->date < t2->date;
		}
		void sort() {
			std::sort(QList<type>::begin(), QList<type>::end(), traded_shares_list_less_than);
		}
		void inSort(type value) {
			QList<type>::insert(std::lower_bound(QList<type>::begin(), QList<type>::end(), value, traded_shares_list_less_than), value);
		}
};

class Security {

	protected:

		Budget *o_budget;

		qlonglong i_id;
		int i_first_revision, i_last_revision;

		AssetsAccount *o_account;

		SecurityType st_type;

		double d_initial_shares;

		int i_decimals, i_quotation_decimals;

		QString s_name;
		QString s_description;

		void init();

	public:

		Security(Budget *parent_budget, AssetsAccount *parent_account, SecurityType initial_type, double initial_shares = 0.0, int initial_decimals = -1, int initial_quotation_decimals = -1, QString initial_name = QString(), QString initial_description = QString());
		Security(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		Security(Budget *parent_budget);
		Security();
		Security(const Security *security);
		virtual ~Security();

		void set(const Security *security);
		void setMergeQuotes(const Security *security);
		void mergeQuotes(const Security *security, bool keep = true);

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
		qlonglong id() const;
		void setId(qlonglong new_id);
		int firstRevision() const;
		void setFirstRevision(int new_rev);
		int lastRevision() const;
		void setLastRevision(int new_rev);
		bool hasQuotation(const QDate &date) const;
		void setQuotation(const QDate &date, double value, bool auto_added = false);
		void removeQuotation(const QDate &date, bool auto_added = false);
		void clearQuotations();
		double getQuotation(const QDate &date, QDate *actual_date = NULL) const;
		AssetsAccount *account() const;
		Currency *currency() const;
		int decimals() const;
		int quotationDecimals() const;
		void setDecimals(int new_decimals);
		void setQuotationDecimals(int new_decimals);
		void setAccount(AssetsAccount *new_account);

		double shares();
		double shares(const QDate &date, bool estimate = false, bool no_scheduled_shares = false);
		double value();
		// estime > 0: estimate future value; estimate < 0: estimate value between quotations
		double value(const QDate &date, int estimate = 0, bool no_scheduled_shares = false);
		double cost(Currency *cur = NULL);
		double cost(const QDate &date, bool no_scheduled_shares = false, Currency *cur = NULL);
		double profit(Currency *cur = NULL);
		double profit(const QDate &date, bool estimate = false, bool no_scheduled_shares = false, Currency *cur = NULL);
		double profit(const QDate &date1, const QDate &date2, bool estimate = false, bool no_scheduled_shares = false, Currency *cur = NULL);
		double yearlyRate();
		double yearlyRate(const QDate &date);
		double yearlyRate(const QDate &date_from, const QDate &date_to);
		double expectedQuotation(const QDate &date);

		QMap<QDate, double> quotations;
		QMap<QDate, bool> quotations_auto;
		TradedSharesList<SecurityTrade*> tradedShares;
		SecurityTransactionList<SecurityTransaction*> transactions;
		SecurityTransactionList<Income*> dividends;
		SecurityTransactionList<ReinvestedDividend*> reinvestedDividends;
		ScheduledSecurityTransactionList<ScheduledTransaction*> scheduledTransactions;
		ScheduledSecurityTransactionList<ScheduledTransaction*> scheduledDividends;
		ScheduledSecurityTransactionList<ScheduledTransaction*> scheduledReinvestedDividends;

};

#endif
