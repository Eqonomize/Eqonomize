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

#ifndef BUDGET_H
#define BUDGET_H

#include <QList>
#include <QMap>
#include <QString>
#include <QCoreApplication>
#include <QFile>

#include "account.h"
#include "transaction.h"
#include "security.h"

#define MONETARY_DECIMAL_PLACES currency_frac_digits()
#define CURRENCY_IS_PREFIX currency_symbol_precedes()
#define IS_GREGORIAN_CALENDAR true

int currency_frac_digits();
bool currency_symbol_precedes();
QString format_money(double v, int precision);

static bool transaction_list_less_than(Transaction *t1, Transaction *t2) {
	return t1->date() < t2->date();
}
static bool split_list_less_than(SplitTransaction *t1, SplitTransaction *t2) {
	return t1->date() < t2->date();
}
static bool schedule_list_less_than(ScheduledTransaction *t1, ScheduledTransaction *t2) {
	return t1->transaction()->date() < t2->transaction()->date();
}
static bool trade_list_less_than(SecurityTrade *t1, SecurityTrade *t2) {
	return t1->date < t2->date;
}
static bool security_list_less_than(Security *t1, Security *t2) {
	return QString::localeAwareCompare(t1->name(), t2->name()) < 0;
}
static bool account_list_less_than(Account *t1, Account *t2) {
	return QString::localeAwareCompare(t1->name(), t2->name()) < 0;
}

template<class type> class TransactionList : public EqonomizeList<type> {
	public:
		TransactionList() : EqonomizeList<type>() {};
		void sort() {
			qSort(QList<type>::begin(), QList<type>::end(), transaction_list_less_than);
		}
		void inSort(type value) {
			QList<type>::append(value);
			sort();
		}
};
template<class type> class SplitTransactionList : public EqonomizeList<type> {
	public:
		SplitTransactionList() : EqonomizeList<type>() {};
		void sort() {
			qSort(QList<type>::begin(), QList<type>::end(), split_list_less_than);
		}
		void inSort(type value) {
			QList<type>::append(value);
			sort();
		}
};
template<class type> class ScheduledTransactionList : public EqonomizeList<type> {
	public:
		ScheduledTransactionList() : EqonomizeList<type>() {};
		void sort() {
			qSort(QList<type>::begin(), QList<type>::end(), schedule_list_less_than);
		}
		void inSort(type value) {
			QList<type>::append(value);
			sort();
		}

};
template<class type> class AccountList : public EqonomizeList<type> {
	public:
		AccountList() : EqonomizeList<type>() {};
		void sort() {
			qSort(QList<type>::begin(), QList<type>::end(), account_list_less_than);
		}
		void inSort(type value) {
			QList<type>::append(value);
			sort();
		}
};
template<class type> class SecurityList : public EqonomizeList<type> {
	public:
		SecurityList() : EqonomizeList<type>() {};
		void sort() {
			qSort(QList<type>::begin(), QList<type>::end(), security_list_less_than);
		}
		void inSort(type value) {
			QList<type>::append(value);
			sort();
		}
};
template<class type> class SecurityTradeList : public EqonomizeList<type> {
	public:
		SecurityTradeList() : EqonomizeList<type>() {};
		void sort() {
			qSort(QList<type>::begin(), QList<type>::end(), trade_list_less_than);
		}
		void inSort(type value) {
			QList<type>::append(value);
			sort();
		}
};

class Budget {

	Q_DECLARE_TR_FUNCTIONS(Budget)
	
	protected:
	
		int i_quotation_decimals, i_share_decimals;

	public:

		Budget();
		~Budget();

		QString loadFile(QString filename, QString &errors);
		QString saveFile(QString filename, QFile::Permissions permissions = QFile::ReadUser | QFile::WriteUser);
		
		void clear();

		void addTransaction(Transaction*);
		void removeTransaction(Transaction*, bool keep = false);

		void addScheduledTransaction(ScheduledTransaction*);
		void removeScheduledTransaction(ScheduledTransaction*, bool keep = false);

		void addSplitTransaction(SplitTransaction*);
		void removeSplitTransaction(SplitTransaction*, bool keep = false);
		
		void addAccount(Account*);
		void removeAccount(Account*, bool keep = false);

		bool accountHasTransactions(Account*);
		void moveTransactions(Account*, Account*);

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

		QMap<int, IncomesAccount*> incomesAccounts_id;
		QMap<int, ExpensesAccount*> expensesAccounts_id;
		QMap<int, AssetsAccount*> assetsAccounts_id;
		QMap<int, Security*> securities_id;

};

#endif
