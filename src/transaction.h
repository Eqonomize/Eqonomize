/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016 by Hanna Knutsson                 *
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

#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <QDateTime>
#include <QString>
#include <QVector>
#include <QCoreApplication>

class QDomElement;

class Account;
class AssetsAccount;
class Budget;
class ExpensesAccount;
class IncomesAccount;
class Recurrence;
class ScheduledTransaction;
class Security;
class SplitTransaction;

extern QString emptystr;

typedef enum {
	TRANSACTION_TYPE_EXPENSE,
	TRANSACTION_TYPE_INCOME,
	TRANSACTION_TYPE_TRANSFER,
	TRANSACTION_TYPE_SECURITY_BUY,
	TRANSACTION_TYPE_SECURITY_SELL
} TransactionType;

typedef enum {
	TRANSACTION_STATUS_NONE,
	TRANSACTION_STATUS_CLEARED,
	TRANSACTION_STATUS_RECONCILED
} TransactionStatus;

class Transaction {

	Q_DECLARE_TR_FUNCTIONS(Transaction)

	protected:

		Budget *o_budget;
		
		double d_value;

		QDate d_date;
		
		Account *o_from;
		Account *o_to;

		QString s_description;
		QString s_comment;

		double d_quantity;

		SplitTransaction *o_split;

		//TransactionStatus ts_status;

	public:

		Transaction(Budget *parent_budget, double initial_value, QDate initial_date, Account *from, Account *to, QString initial_description = emptystr, QString initial_comment = emptystr);
		Transaction(Budget *parent_budget, QDomElement *e, bool *valid);
		Transaction();
		Transaction(const Transaction *transaction);
		virtual ~Transaction();
		virtual Transaction *copy() const = 0;

		virtual bool equals(const Transaction *transaction) const;

		SplitTransaction *parentSplit() const;
		void setParentSplit(SplitTransaction *parent);
		virtual double value() const;
		void setValue(double new_value);
		virtual double quantity() const;
		void setQuantity(double new_quantity);
		const QDate &date() const;
		void setDate(QDate new_date);
		virtual const QString &description() const;
		void setDescription(QString new_description);
		virtual const QString &comment() const;
		void setComment(QString new_comment);
		virtual Account *fromAccount() const;
		void setFromAccount(Account *new_from);
		virtual Account *toAccount() const;
		void setToAccount(Account *new_to);
		Budget *budget() const;
		virtual TransactionType type() const = 0;
		virtual void save(QDomElement *e) const;
		
};

class Expense : public Transaction {

	protected:

		QString s_payee;

	public:

		Expense(Budget *parent_budget, double initial_cost, QDate initial_date, ExpensesAccount *initial_category, AssetsAccount *initial_from, QString initial_description = emptystr, QString initial_comment = emptystr);
		Expense(Budget *parent_budget, QDomElement *e, bool *valid);
		Expense();
		Expense(const Expense *expense);
		virtual ~Expense();
		Transaction *copy() const;

		bool equals(const Transaction *transaction) const;
		
		ExpensesAccount *category() const;
		void setCategory(ExpensesAccount *new_category);
		AssetsAccount *from() const;
		void setFrom(AssetsAccount *new_from);
		double cost() const;
		void setCost(double new_cost);
		const QString &payee() const;
		void setPayee(QString new_payee);
		TransactionType type() const;
		void save(QDomElement *e) const;
		
};

class Income : public Transaction {

	protected:

		Security *o_security;
		QString s_payer;

	public:

		Income(Budget *parent_budget, double initial_income, QDate initial_date, IncomesAccount *initial_category, AssetsAccount *initial_to, QString initial_description = emptystr, QString initial_comment = emptystr);
		Income(Budget *parent_budget, QDomElement *e, bool *valid);
		Income();
		Income(const Income *income_);
		virtual ~Income();
		Transaction *copy() const;

		bool equals(const Transaction *transaction) const;

		IncomesAccount *category() const;
		void setCategory(IncomesAccount *new_category);
		AssetsAccount *to() const;
		void setTo(AssetsAccount *new_to);
		double income() const;
		void setIncome(double new_income);
		const QString &payer() const;
		void setPayer(QString new_payer);
		TransactionType type() const;
		void save(QDomElement *e) const;
		void setSecurity(Security *parent_security);
		Security *security() const;
		
};

class Transfer : public Transaction {

	public:

		Transfer(Budget *parent_budget, double initial_amount, QDate initial_date, AssetsAccount *initial_from, AssetsAccount *initial_to, QString initial_description = emptystr, QString initial_comment = emptystr);
		Transfer(Budget *parent_budget, QDomElement *e, bool *valid, bool internal_is_balancing = false);
		Transfer();
		Transfer(const Transfer *transfer);
		virtual ~Transfer();
		virtual Transaction *copy() const;

		AssetsAccount *to() const;
		void setTo(AssetsAccount *new_to);
		AssetsAccount *from() const;
		void setFrom(AssetsAccount *new_from);
		double amount() const;
		void setAmount(double new_amount);
		TransactionType type() const;
		virtual void save(QDomElement *e) const;
		
};

class Balancing : public Transfer {

	public:

		Balancing(Budget *parent_budget, double initial_amount, QDate initial_date, AssetsAccount *initial_account, QString initial_comment = emptystr);
		Balancing(Budget *parent_budget, QDomElement *e, bool *valid);
		Balancing();
		Balancing(const Balancing *balancing);
		virtual ~Balancing();
		Transaction *copy() const;

		AssetsAccount *account() const;
		void setAccount(AssetsAccount *new_account);
		void save(QDomElement *e) const;

};

class SecurityTransaction : public Transaction {

	protected:

		Security *o_security;
		
		double d_shares, d_share_value;

	public:

		SecurityTransaction(Security *parent_security, double initial_value, double initial_shares, double initial_share_value, QDate initial_date, QString initial_comment = emptystr);
		SecurityTransaction(Budget *parent_budget, QDomElement *e, bool *valid);
		SecurityTransaction();
		SecurityTransaction(const SecurityTransaction *transaction);
		virtual ~SecurityTransaction();
		virtual Transaction *copy() const = 0;

		bool equals(const Transaction *transaction) const;

		virtual Account *fromAccount() const;
		virtual Account *toAccount() const;
		virtual Account *account() const = 0;
		virtual void setAccount(Account *account) = 0;
		virtual TransactionType type() const = 0;
		void save(QDomElement *e) const;
		double shareValue() const;
		double shares() const;
		void setShareValue(double new_share_value);
		void setShares(double new_shares);
		void setSecurity(Security *parent_security);
		Security *security() const;
		
};

class SecurityBuy : public SecurityTransaction {

	public:

		SecurityBuy(Security *parent_security, double initial_value, double initial_shares, double initial_share_value, QDate initial_date, Account *from_account, QString initial_comment = emptystr);
		SecurityBuy(Budget *parent_budget, QDomElement *e, bool *valid);
		SecurityBuy();
		SecurityBuy(const SecurityBuy *transaction);
		virtual ~SecurityBuy();
		Transaction *copy() const;

		Account *account() const;
		void setAccount(Account *account);		
		Account *fromAccount() const;
		TransactionType type() const;
		void save(QDomElement *e) const;

};

class SecuritySell : public SecurityTransaction {

	public:

		SecuritySell(Security *parent_security, double initial_value, double initial_shares, double initial_share_value, QDate initial_date, Account *to_account, QString initial_comment = emptystr);
		SecuritySell(Budget *parent_budget, QDomElement *e, bool *valid);
		SecuritySell();
		SecuritySell(const SecuritySell *transaction);
		virtual ~SecuritySell();
		Transaction *copy() const;

		Account *account() const;
		void setAccount(Account *account);
		Account *toAccount() const;
		TransactionType type() const;
		void save(QDomElement *e) const;

};

class ScheduledTransaction {

	protected:

		Budget *o_budget;
		Transaction *o_trans;
		Recurrence *o_rec;

	public:

		ScheduledTransaction(Budget *parent_budget);
		ScheduledTransaction(Budget *parent_budget, Transaction *trans, Recurrence *rec);
		ScheduledTransaction(Budget *parent_budget, QDomElement *e, bool *valid);
		ScheduledTransaction(const ScheduledTransaction *strans);
		virtual ~ScheduledTransaction();
		ScheduledTransaction *copy() const;

		Transaction *realize(const QDate &date);
		Transaction *transaction() const;
		Recurrence *recurrence() const;
		void setRecurrence(Recurrence *rec, bool delete_old = true);
		void setTransaction(Transaction *trans, bool delete_old = true);
		Budget *budget() const;
		virtual void save(QDomElement *e) const;
		QDate firstOccurrence() const;
		bool isOneTimeTransaction() const;
		void setDate(const QDate &newdate);
		void addException(const QDate &exceptiondate);
	
};

class SplitTransaction {
	
	protected:

		Budget *o_budget;
		
		QDate d_date;
		AssetsAccount *o_account;
		QString s_description;
		QString s_comment;

	public:

		SplitTransaction(Budget *parent_budget, QDate initial_date, AssetsAccount *initial_account, QString initial_description = emptystr);
		SplitTransaction(Budget *parent_budget, QDomElement *e, bool *valid);
		SplitTransaction();
		~SplitTransaction();

		double value() const;
		void addTransaction(Transaction *trans);
		void removeTransaction(Transaction *trans, bool keep = false);
		void clear(bool keep = false);
		const QDate &date() const;
		void setDate(QDate new_date);
		const QString &description() const;
		void setDescription(QString new_description);
		const QString &comment() const;
		void setComment(QString new_comment);
		AssetsAccount *account() const;
		void setAccount(AssetsAccount *new_account);
		Budget *budget() const;
		void save(QDomElement *e) const;

		QVector<Transaction*> splits;

};

class SecurityTrade {
	public:
		SecurityTrade(const QDate &date_, double value_, double from_shares_, Security *from_security_, double to_shares_, Security *to_security_) : date(date_), value(value_), from_shares(from_shares_), to_shares(to_shares_), from_security(from_security_), to_security(to_security_) {}
		QDate date;
		double value, from_shares, to_shares;
		Security *from_security, *to_security;
};


#endif
