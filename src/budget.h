/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016-2020 by Hanna Knutsson            *
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

#ifndef BUDGET_H
#define BUDGET_H

#include <QList>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QCoreApplication>
#include <QFile>
#include <QVector>
#include <QByteArray>
#include <QNetworkAccessManager>

#include "eqonomizelist.h"
#include "account.h"
#include "transaction.h"
#include "security.h"
#include "currency.h"

#define MONETARY_DECIMAL_PLACES 2
#define SAVE_MONETARY_DECIMAL_PLACES 4
#define QUANTITY_DECIMAL_PLACES 2
#define IS_GREGORIAN_CALENDAR true

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#	define XML_COMPARE_CONST_CHAR(x) QLatin1String(x)
#else
#	define XML_COMPARE_CONST_CHAR(x) x
#endif

class QProcess;
class QNetworkReply;

typedef enum {
	TRANSACTION_CONVERSION_RATE_AT_DATE,
	TRANSACTION_CONVERSION_LATEST_RATE
} TransactionConversionRateDate;

bool is_zero(double);

void read_id(QXmlStreamAttributes *attr, qlonglong &id, int &rev1, int &rev2);
void write_id(QXmlStreamAttributes *attr, qlonglong &id, int &rev1, int &rev2);
void write_id(QXmlStreamWriter *writer, qlonglong &id, int &rev1, int &rev2);

bool transaction_list_less_than(Transaction *t1, Transaction *t2);
bool split_list_less_than(SplitTransaction *t1, SplitTransaction *t2);
bool schedule_list_less_than(ScheduledTransaction *t1, ScheduledTransaction *t2);
bool trade_list_less_than(SecurityTrade *t1, SecurityTrade *t2);
bool security_list_less_than(Security *t1, Security *t2);

template<class type> class TransactionList : public EqonomizeList<type> {
	public:
		TransactionList() : EqonomizeList<type>() {};
		void sort() {
			std::sort(QList<type>::begin(), QList<type>::end(), transaction_list_less_than);
		}
		void inSort(type value) {
			QList<type>::insert(std::lower_bound(QList<type>::begin(), QList<type>::end(), value, transaction_list_less_than), value);
		}
};
template<class type> class SplitTransactionList : public EqonomizeList<type> {
	public:
		SplitTransactionList() : EqonomizeList<type>() {};
		void sort() {
			std::sort(QList<type>::begin(), QList<type>::end(), split_list_less_than);
		}
		void inSort(type value) {
			QList<type>::insert(std::lower_bound(QList<type>::begin(), QList<type>::end(), value, split_list_less_than), value);
		}
};
template<class type> class ScheduledTransactionList : public EqonomizeList<type> {
	public:
		ScheduledTransactionList() : EqonomizeList<type>() {};
		void sort() {
			std::sort(QList<type>::begin(), QList<type>::end(), schedule_list_less_than);
		}
		void inSort(type value) {
			QList<type>::insert(std::lower_bound(QList<type>::begin(), QList<type>::end(), value, schedule_list_less_than), value);
		}

};
template<class type> class SecurityList : public EqonomizeList<type> {
	public:
		SecurityList() : EqonomizeList<type>() {};
		void sort() {
			std::sort(QList<type>::begin(), QList<type>::end(), security_list_less_than);
		}
		void inSort(type value) {
			QList<type>::insert(std::lower_bound(QList<type>::begin(), QList<type>::end(), value, security_list_less_than), value);
		}
};
template<class type> class SecurityTradeList : public EqonomizeList<type> {
	public:
		SecurityTradeList() : EqonomizeList<type>() {};
		void sort() {
			std::sort(QList<type>::begin(), QList<type>::end(), trade_list_less_than);
		}
		void inSort(type value) {
			QList<type>::insert(std::lower_bound(QList<type>::begin(), QList<type>::end(), value, trade_list_less_than), value);
		}
};

struct BudgetSynchronization {
	QString url, download, upload;
	bool autosync;
	int revision;
	void clear() {
		autosync = false;
		url = QString();
		download = QString();
		upload = QString();
		revision = 0;
	}
	bool isComplete() {
		return !upload.isEmpty() && (!download.isEmpty() || !url.isEmpty());
	}
};

class Budget {

	Q_DECLARE_TR_FUNCTIONS(Budget)

	protected:

		int i_quotation_decimals, i_share_decimals, i_budget_day, i_budget_week, i_budget_month, i_opened_revision, i_revision;
		bool b_record_new_tags, b_record_new_accounts, b_record_new_securities, b_default_currency_changed, b_currency_modified;
		TransactionConversionRateDate i_tcrd;

		qlonglong last_id;

		Currency *default_currency;

		QNetworkReply *syncReply;
		QProcess *syncProcess;

	public:

		BudgetSynchronization *o_sync;
		Currency *currency_euro;
		QNetworkAccessManager nam;

		QString monetary_decimal_separator, monetary_group_separator, monetary_positive_sign, monetary_negative_sign;
		QByteArray monetary_group_format;
		bool currency_symbol_precedes, currency_symbol_precedes_neg, currency_code_precedes, currency_code_precedes_neg, currency_symbol_space, currency_code_space, currency_symbol_space_neg, currency_code_space_neg;
		int monetary_sign_p_symbol_neg, monetary_sign_p_symbol_pos, monetary_sign_p_code_neg, monetary_sign_p_code_pos, monetary_decimal_places;
		QString decimal_separator, group_separator, positive_sign, negative_sign;
		QByteArray group_format;

		Budget();
		~Budget();

		void loadCurrencies();
		void loadGlobalCurrencies();
		void loadLocalCurrencies();
		void loadCurrenciesFile(QString filename, bool is_local);
		QString loadECBData(QByteArray data);
		QString loadMyCurrencyNetData(QByteArray data);
		QString loadMyCurrencyNetHtml(QByteArray data);
		QString saveCurrencies();

		TransactionConversionRateDate defaultTransactionConversionRateDate() const;
		void setDefaultTransactionConversionRateDate(TransactionConversionRateDate tcrd);

		QString loadFile(QString filename, QString &errors, bool *default_currency_created = NULL, bool merge = false, bool rename_duplicate_accounts = false, bool rename_duplicate_categories = false, bool rename_duplicate_securities = false, bool ignore_duplicate_transactions = false);
		QString saveFile(QString filename, QFile::Permissions permissions = QFile::ReadUser | QFile::WriteUser, bool is_backup = false);
		int fileRevision(QString filename, QString &error) const;
		bool isUnsynced(QString filename, QString &error, int synced_revision = -1) const;
		QString syncFile(QString filename, QString &errors, int revision_synced = -1);
		void cancelSync();
		bool sync(QString &error, QString &errors, bool do_upload = true, bool on_load = false);
		QString syncUpload(QString filename);
		bool autosyncEnabled() const;

		void clear();

		QString formatMoney(double v, int precision = -1, bool show_currency = true);
		QString formatValue(double v, int precision = 2, bool always_show_sign = false);
		QString formatValue(int v, int precision = 0, bool always_show_sign = false);

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
		void transactionsSortModified(Transactions*);
		void scheduledTransactionSortModified(ScheduledTransaction*);
		void transactionSortModified(Transaction*);
		void splitTransactionSortModified(SplitTransaction*);
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

		qlonglong getNewId();
		int revision();

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

		IncomesAccount *null_incomes_account;

		void setRecordNewAccounts(bool rna);
		QVector<Account*> newAccounts;

		void setRecordNewSecurities(bool rns);
		QVector<Security*> newSecurities;

		QHash<qlonglong, IncomesAccount*> incomesAccounts_id;
		QHash<qlonglong, ExpensesAccount*> expensesAccounts_id;
		QHash<qlonglong, AssetsAccount*> assetsAccounts_id;
		QHash<qlonglong, Security*> securities_id;

		Transactions *getTransaction(qlonglong tid);

		void setBudgetDay(int day_of_month);
		int budgetDay() const;
		void setBudgetWeek(int week_of_month);
		int budgetWeek() const;
		void setBudgetMonth(int month_of_year);
		int budgetMonth() const;

		bool isSameBudgetMonth(const QDate &date1, const QDate &date2) const;
		int daysInBudgetMonth(const QDate &date) const;
		int daysInBudgetYear(const QDate &date) const;
		int dayOfBudgetYear(const QDate &date) const;
		int dayOfBudgetMonth(const QDate &date) const;
		int budgetMonth(const QDate &date) const;
		int budgetYear(const QDate &date) const;
		QString budgetYearString(const QDate &date, bool short_format = false) const;
		QString budgetYearString(int year, bool short_format = false) const;
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

		int getAccountType(const QString &type, bool localized = false, bool plural = false);
		QString getAccountTypeName(int at_type, bool localized = false, bool plural = false);
		bool accountTypeIsDebt(int at_type);
		bool accountTypeIsCreditCard(int at_type);
		bool accountTypeIsSecurities(int at_type);
		bool accountTypeIsLiabilities(int at_type);
		bool accountTypeIsOther(int at_type);

		void tagAdded(const QString &tag);
		void tagRemoved(const QString &tag);
		QString findTag(const QString &tag);

		void setRecordNewTags(bool rnt);
		QVector<QString> newTags;

		QStringList tags;

};

#endif
