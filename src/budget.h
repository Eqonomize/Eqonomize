/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016-2017 by Hanna Knutsson            *
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

#ifndef BUDGET_H
#define BUDGET_H

#include <QList>
#include <QMap>
#include <QString>
#include <QCoreApplication>
#include <QFile>
#include <QVector>
#include <QByteArray>

#include "eqonomizelist.h"
#include "account.h"
#include "transaction.h"
#include "security.h"
#include "currency.h"

#define MONETARY_DECIMAL_PLACES 2
#define SAVE_MONETARY_DECIMAL_PLACES 4
#define QUANTITY_DECIMAL_PLACES 2
#define CURRENCY_IS_PREFIX currency_symbol_precedes()
#define IS_GREGORIAN_CALENDAR true

int currency_frac_digits();
bool currency_symbol_precedes();
bool is_zero(double);
QString format_money(double v, int precision);

static bool transaction_list_less_than(Transaction *t1, Transaction *t2) {
	return t1->date() < t2->date();
}
static bool split_list_less_than(SplitTransaction *t1, SplitTransaction *t2) {
	return t1->date() < t2->date();
}
static bool schedule_list_less_than(ScheduledTransaction *t1, ScheduledTransaction *t2) {
	return t1->date() < t2->date();
}
static bool trade_list_less_than(SecurityTrade *t1, SecurityTrade *t2) {
	return t1->date < t2->date;
}
static bool security_list_less_than(Security *t1, Security *t2) {
	return QString::localeAwareCompare(t1->name(), t2->name()) < 0;
}

template<class type> class TransactionList : public EqonomizeList<type> {
	public:
		TransactionList() : EqonomizeList<type>() {};
		void sort() {
			qSort(QList<type>::begin(), QList<type>::end(), transaction_list_less_than);
		}
		void inSort(type value) {
			QList<type>::insert(qLowerBound(QList<type>::begin(), QList<type>::end(), value, transaction_list_less_than), value);
		}
};
template<class type> class SplitTransactionList : public EqonomizeList<type> {
	public:
		SplitTransactionList() : EqonomizeList<type>() {};
		void sort() {
			qSort(QList<type>::begin(), QList<type>::end(), split_list_less_than);
		}
		void inSort(type value) {
			QList<type>::insert(qLowerBound(QList<type>::begin(), QList<type>::end(), value, split_list_less_than), value);
		}
};
template<class type> class ScheduledTransactionList : public EqonomizeList<type> {
	public:
		ScheduledTransactionList() : EqonomizeList<type>() {};
		void sort() {
			qSort(QList<type>::begin(), QList<type>::end(), schedule_list_less_than);
		}
		void inSort(type value) {
			QList<type>::insert(qLowerBound(QList<type>::begin(), QList<type>::end(), value, schedule_list_less_than), value);
		}

};
template<class type> class SecurityList : public EqonomizeList<type> {
	public:
		SecurityList() : EqonomizeList<type>() {};
		void sort() {
			qSort(QList<type>::begin(), QList<type>::end(), security_list_less_than);
		}
		void inSort(type value) {
			QList<type>::insert(qLowerBound(QList<type>::begin(), QList<type>::end(), value, security_list_less_than), value);
		}
};
template<class type> class SecurityTradeList : public EqonomizeList<type> {
	public:
		SecurityTradeList() : EqonomizeList<type>() {};
		void sort() {
			qSort(QList<type>::begin(), QList<type>::end(), trade_list_less_than);
		}
		void inSort(type value) {
			QList<type>::insert(qLowerBound(QList<type>::begin(), QList<type>::end(), value, trade_list_less_than), value);
		}
};

class Budget {

	Q_DECLARE_TR_FUNCTIONS(Budget)
	
	protected:
	
		int i_quotation_decimals, i_share_decimals, i_budget_day;
		bool b_record_new_accounts, b_default_currency_changed, b_currency_modified;
		Currency *default_currency;

	public:
	
		Currency *currency_euro;

		Budget();
		~Budget();
		
		void loadCurrencies();
		void loadGlobalCurrencies();
		void loadLocalCurrencies();
		void loadCurrenciesFile(QString filename, bool is_local);
		QString loadECBData(QByteArray data);
		bool saveCurrencies();

		QString loadFile(QString filename, QString &errors, bool *default_currency_created = NULL, bool merge = false);
		QString saveFile(QString filename, QFile::Permissions permissions = QFile::ReadUser | QFile::WriteUser);
		
		void clear();
		
		QString formatMoney(double v, int precision = -1, bool show_currency = true);

		void addTransaction(Transaction*);
		void removeTransactions(Transactions*, bool keep = false);
		
		void addTransactions(Transactions*);
		void removeTransaction(Transaction*, bool keep = false);

		void addScheduledTransaction(ScheduledTransaction*);
		void removeScheduledTransaction(ScheduledTransaction*, bool keep = false);

		void addSplitTransaction(SplitTransaction*);
		void removeSplitTransaction(SplitTransaction*, bool keep = false);
		
		void addAccount(Account*);
		void removeAccount(Account*, bool keep = false);
		void accountModified(Account*);

		bool accountHasTransactions(Account*, bool check_subs = true);
		void moveTransactions(Account*, Account*, bool move_from_subs = true);
		
		Transaction *findDuplicateTransaction(Transaction *trans); 

		void addSecurity(Security*);
		void removeSecurity(Security*, bool keep = false);
		Security *findSecurity(QString name);
		
		int defaultQuotationDecimals() const;
		int defaultShareDecimals() const;
		void setDefaultQuotationDecimals(int new_decimals);
		void setDefaultShareDecimals(int new_decimals);

		bool securityHasTransactions(Security*);

		void scheduledTransactionDateModified(ScheduledTransaction*);
		void transactionDateModified(Transaction*, const QDate &olddate);
		void splitTransactionDateModified(SplitTransaction*, const QDate &olddate);
		void accountNameModified(Account*);
		void securityNameModified(Security*);
		void securityTradeDateModified(SecurityTrade*, const QDate &olddate);

		void addSecurityTrade(SecurityTrade*);
		void removeSecurityTrade(SecurityTrade*, bool keep = false);

		Account *findAccount(QString name);
		AssetsAccount *findAssetsAccount(QString name);
		IncomesAccount *findIncomesAccount(QString name);
		ExpensesAccount *findExpensesAccount(QString name);
		IncomesAccount *findIncomesAccount(QString name, CategoryAccount *parent_acc);
		ExpensesAccount *findExpensesAccount(QString name, CategoryAccount *parent_acc);
		
		Currency *defaultCurrency();
		void setDefaultCurrency(Currency*);
		bool resetDefaultCurrency();
		void addCurrency(Currency*);
		void removeCurrency(Currency*);
		Currency *findCurrency(QString code);
		Currency *findCurrencySymbol(QString symbol, bool require_unique = false);
		bool usesMultipleCurrencies();
		bool defaultCurrencyChanged();
		void resetDefaultCurrencyChanged();
		bool currenciesModified();
		void resetCurrenciesModified();
		void currencyModified(Currency*);
		
		AccountList<IncomesAccount*> incomesAccounts;
		AccountList<ExpensesAccount*> expensesAccounts;
		AccountList<AssetsAccount*> assetsAccounts;
		AccountList<Account*> accounts;
		AssetsAccount *balancingAccount, *budgetAccount;
		TransactionList<Expense*> expenses;
		TransactionList<Income*> incomes;
		TransactionList<Transfer*> transfers;
		TransactionList<Transaction*> transactions;
		TransactionList<SecurityTransaction*> securityTransactions;
		ScheduledTransactionList<ScheduledTransaction*> scheduledTransactions;
		SplitTransactionList<SplitTransaction*> splitTransactions;
		SecurityList<Security*> securities;
		SecurityTradeList<SecurityTrade*> securityTrades;
		CurrencyList<Currency*> currencies;
		
		void setRecordNewAccounts(bool rna);
		QVector<Account*> newAccounts;

		QMap<int, IncomesAccount*> incomesAccounts_id;
		QMap<int, ExpensesAccount*> expensesAccounts_id;
		QMap<int, AssetsAccount*> assetsAccounts_id;
		QMap<int, Security*> securities_id;
		
		void setBudgetDay(int day_of_month);
		int budgetDay() const;
		
		bool isSameBudgetMonth(const QDate &date1, const QDate &date2) const;
		int daysInBudgetMonth(const QDate &date) const;
		int daysInBudgetYear(const QDate &date) const;
		int dayOfBudgetYear(const QDate &date) const;
		int dayOfBudgetMonth(const QDate &date) const;
		int budgetMonth(const QDate &date) const;
		int budgetYear(const QDate &date) const;
		bool isFirstBudgetDay(const QDate &date) const;
		bool isLastBudgetDay(const QDate &date) const;
		void addBudgetMonthsSetLast(QDate &date, int months) const;
		void addBudgetMonthsSetFirst(QDate &date, int months) const;
		QDate budgetDateToMonth(QDate date) const;
		QDate firstBudgetDay(QDate date) const;
		QDate lastBudgetDay(QDate date) const;
		QDate firstBudgetDayOfYear(QDate date) const;
		QDate lastBudgetDayOfYear(QDate date) const;
		QDate monthToBudgetMonth(QDate date) const;
		void goForwardBudgetMonths(QDate &from_date, QDate &to_date, int months) const;
		double averageYear(const QDate &date1, const QDate &date2, bool use_budget_months = true);
		double averageMonth(const QDate &date1, const QDate &date2, bool use_budget_months = true);
		double yearsBetweenDates(const QDate &date1, const QDate &date2, bool use_budget_months = true);
		double monthsBetweenDates(const QDate &date1, const QDate &date2, bool use_budget_months = true);
		int calendarMonthsBetweenDates(const QDate &date1, const QDate &date2, bool use_budget_months = true);

};

#endif
