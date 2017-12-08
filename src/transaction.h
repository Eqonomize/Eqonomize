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

#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <QDateTime>
#include <QString>
#include <QVector>
#include <QCoreApplication>

class QXmlStreamReader;
class QXmlStreamWriter;
class QXmlStreamAttributes;

class Account;
class AssetsAccount;
class CategoryAccount;
class Budget;
class ExpensesAccount;
class IncomesAccount;
class Recurrence;
class ScheduledTransaction;
class Security;
class SplitTransaction;
class Currency;

typedef enum {
	TRANSACTION_TYPE_EXPENSE,
	TRANSACTION_TYPE_INCOME,
	TRANSACTION_TYPE_TRANSFER,
	TRANSACTION_TYPE_SECURITY_BUY,
	TRANSACTION_TYPE_SECURITY_SELL
} TransactionType;

typedef enum {
	TRANSACTION_SUBTYPE_EXPENSE = 100,
	TRANSACTION_SUBTYPE_INCOME = 101,
	TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND = 102,
	TRANSACTION_SUBTYPE_TRANSFER = 103,
	TRANSACTION_SUBTYPE_SECURITY_BUY = 104,
	TRANSACTION_SUBTYPE_SECURITY_SELL = 105,
	TRANSACTION_SUBTYPE_DEBT_REDUCTION = 106,
	TRANSACTION_SUBTYPE_DEBT_INTEREST = 107,
	TRANSACTION_SUBTYPE_DEBT_FEE = 108,
	TRANSACTION_SUBTYPE_BALANCING = 109
} TransactionSubType;

typedef enum {
	SPLIT_TRANSACTION_TYPE_MULTIPLE_ITEMS,
	SPLIT_TRANSACTION_TYPE_MULTIPLE_ACCOUNTS,
	SPLIT_TRANSACTION_TYPE_LOAN
} SplitTransactionType;

typedef enum {
	GENERAL_TRANSACTION_TYPE_SINGLE,
	GENERAL_TRANSACTION_TYPE_SPLIT,
	GENERAL_TRANSACTION_TYPE_SCHEDULE
} GeneralTransactionType;

/*typedef enum {
	TRANSACTION_STATUS_NONE,
	TRANSACTION_STATUS_CLEARED,
	TRANSACTION_STATUS_RECONCILED
} TransactionStatus;*/

class Transactions {
	
	Q_DECLARE_TR_FUNCTIONS(Transactions)
	
	public:
		
		Transactions() {}
		virtual ~Transactions() {}
		virtual Transactions *copy() const = 0;
		virtual bool equals(const Transactions *transaction, bool strict_comparison = true) const = 0;
	
		virtual double value(bool convert = false) const = 0;
		virtual Currency *currency() const = 0;
		virtual QString valueString(int precision = -1) const;
		virtual QString valueString(double value_, int precision = -1) const;
		virtual double quantity() const = 0;
		virtual const QDate &date() const = 0;
		virtual const qint64 &timestamp() const = 0;
		virtual void setTimestamp(qint64 cr_time) = 0;
		virtual void setTimestamp();
		virtual void setDate(QDate new_date) = 0;
		virtual QString description() const = 0;
		virtual const QString &comment() const = 0;
		virtual const QString &associatedFile() const = 0;
		virtual void setAssociatedFile(QString new_attachment) = 0;
		virtual Budget *budget() const = 0;
		virtual GeneralTransactionType generaltype() const = 0;
		virtual bool relatesToAccount(Account *account, bool include_subs = true, bool include_non_value = false) const = 0;
		virtual void replaceAccount(Account *old_account, Account *new_account) = 0;
		virtual double accountChange(Account *account, bool include_subs = true, bool convert = false) const = 0;
	
};

class Transaction : public Transactions {

	Q_DECLARE_TR_FUNCTIONS(Transaction)

	protected:

		Budget *o_budget;
		
		double d_value;

		QDate d_date;		
		
		Account *o_from;
		Account *o_to;

		QString s_description;
		QString s_comment;
		
		QString s_file;

		double d_quantity;

		SplitTransaction *o_split;
		
		qint64 i_time;

		//TransactionStatus ts_status;

	public:

		Transaction(Budget *parent_budget, double initial_value, QDate initial_date, Account *from, Account *to, QString initial_description = QString(), QString initial_comment = QString());
		Transaction(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		Transaction(Budget *parent_budget);
		Transaction();
		Transaction(const Transaction *transaction);
		virtual ~Transaction();
		virtual Transaction *copy() const = 0;

		virtual bool equals(const Transactions *transaction, bool strict_comparison = true) const;

		virtual void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		virtual bool readElement(QXmlStreamReader *xml, bool *valid);
		virtual bool readElements(QXmlStreamReader *xml, bool *valid);
		virtual void save(QXmlStreamWriter *xml);
		virtual void writeAttributes(QXmlStreamAttributes *attr);
		virtual void writeElements(QXmlStreamWriter *xml);
		
		SplitTransaction *parentSplit() const;
		void setParentSplit(SplitTransaction *parent);
		virtual double value(bool convert = false) const;
		virtual double fromValue(bool convert = false) const;
		virtual double toValue(bool convert = false) const;
		virtual Currency *currency() const;
		virtual Currency *fromCurrency() const;
		virtual Currency *toCurrency() const;
		virtual void setValue(double new_value);
		virtual double quantity() const;
		void setQuantity(double new_quantity);
		const QDate &date() const;
		void setDate(QDate new_date);
		const qint64 &timestamp() const;
		void setTimestamp(qint64 cr_time);
		virtual QString description() const;
		void setDescription(QString new_description);
		virtual const QString &comment() const;
		void setComment(QString new_comment);
		const QString &associatedFile() const;
		void setAssociatedFile(QString new_attachment);
		virtual Account *fromAccount() const;
		void setFromAccount(Account *new_from);
		virtual Account *toAccount() const;
		void setToAccount(Account *new_to);
		Budget *budget() const;
		virtual GeneralTransactionType generaltype() const;
		virtual TransactionType type() const = 0;
		virtual TransactionSubType subtype() const;
		virtual bool relatesToAccount(Account *account, bool include_subs = true, bool include_non_value = false) const;
		virtual void replaceAccount(Account *old_account, Account *new_account);
		virtual double accountChange(Account *account, bool include_subs = true, bool convert = false) const;
		
};

class Expense : public Transaction {

	Q_DECLARE_TR_FUNCTIONS(Expense)

	protected:

		QString s_payee;

	public:

		Expense(Budget *parent_budget, double initial_cost, QDate initial_date, ExpensesAccount *initial_category, AssetsAccount *initial_from, QString initial_description = QString(), QString initial_comment = QString());
		Expense(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		Expense(Budget *parent_budget);
		Expense();
		Expense(const Expense *expense);
		virtual ~Expense();
		Transaction *copy() const;

		virtual void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		virtual void writeAttributes(QXmlStreamAttributes *attr);

		bool equals(const Transactions *transactions, bool strict_comparison = true) const;
		
		ExpensesAccount *category() const;
		void setCategory(ExpensesAccount *new_category);
		AssetsAccount *from() const;
		void setFrom(AssetsAccount *new_from);
		double cost(bool convert = false) const;
		void setCost(double new_cost);
		virtual const QString &payee() const;
		void setPayee(QString new_payee);
		virtual QString description() const;
		TransactionType type() const;
		virtual TransactionSubType subtype() const;
		virtual bool relatesToAccount(Account *account, bool include_subs = true, bool include_non_value = false) const;
		virtual void replaceAccount(Account *old_account, Account *new_account);
		
};

class DebtFee : public Expense {

	Q_DECLARE_TR_FUNCTIONS(DebtFee)

	protected:
	
		AssetsAccount *o_loan;

	public:
	
		DebtFee(Budget *parent_budget, double initial_cost, QDate initial_date, ExpensesAccount *initial_category, AssetsAccount *initial_from, AssetsAccount *initial_loan, QString initial_comment = QString());
		DebtFee(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		DebtFee(Budget *parent_budget);
		DebtFee();
		DebtFee(const DebtFee *loanfee);
		virtual ~DebtFee();
		Transaction *copy() const;
		
		virtual void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		virtual void writeAttributes(QXmlStreamAttributes *attr);
	
		AssetsAccount *loan() const;
		void setLoan(AssetsAccount *new_loan);
		const QString &payee() const;
		virtual QString description() const;
		TransactionSubType subtype() const;
		virtual bool relatesToAccount(Account *account, bool include_subs = true, bool include_non_value = false) const;
		virtual void replaceAccount(Account *old_account, Account *new_account);
	
};

class DebtInterest : public Expense {

	Q_DECLARE_TR_FUNCTIONS(DebtInterest)

	protected:
	
		AssetsAccount *o_loan;

	public:
	
		DebtInterest(Budget *parent_budget, double initial_cost, QDate initial_date, ExpensesAccount *initial_category, AssetsAccount *initial_from, AssetsAccount *initial_loan, QString initial_comment = QString());
		DebtInterest(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		DebtInterest(Budget *parent_budget);
		DebtInterest();
		DebtInterest(const DebtInterest *interest);
		virtual ~DebtInterest();
		Transaction *copy() const;
		
		virtual void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		virtual void writeAttributes(QXmlStreamAttributes *attr);
	
		AssetsAccount *loan() const;
		void setLoan(AssetsAccount *new_loan);
		const QString &payee() const;
		virtual QString description() const;
		TransactionSubType subtype() const;
		virtual bool relatesToAccount(Account *account, bool include_subs = true, bool include_non_value = false) const;
		virtual void replaceAccount(Account *old_account, Account *new_account);
	
};


class Income : public Transaction {

	Q_DECLARE_TR_FUNCTIONS(Income)

	protected:

		Security *o_security;
		QString s_payer;

	public:

		Income(Budget *parent_budget, double initial_income, QDate initial_date, IncomesAccount *initial_category, AssetsAccount *initial_to, QString initial_description = QString(), QString initial_comment = QString());
		Income(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		Income(Budget *parent_budget);
		Income();
		Income(const Income *income_);
		virtual ~Income();
		Transaction *copy() const;

		virtual void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		virtual void writeAttributes(QXmlStreamAttributes *attr);
	
		bool equals(const Transactions *transaction, bool strict_comparison = true) const;

		IncomesAccount *category() const;
		void setCategory(IncomesAccount *new_category);
		AssetsAccount *to() const;
		void setTo(AssetsAccount *new_to);
		double income(bool convert = false) const;
		void setIncome(double new_income);
		const QString &payer() const;
		void setPayer(QString new_payer);
		TransactionType type() const;
		void setSecurity(Security *parent_security);
		Security *security() const;
		virtual QString description() const;
		virtual TransactionSubType subtype() const;
		
};

class ReinvestedDividend : public Income {
	
	Q_DECLARE_TR_FUNCTIONS(Income)

	protected:

		double d_shares;

	public:

		ReinvestedDividend(Budget *parent_budget, double initial_value, double initial_shares, QDate initial_date, Security *initial_security, IncomesAccount *initial_category, QString initial_comment = QString());
		ReinvestedDividend(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		ReinvestedDividend(Budget *parent_budget);
		ReinvestedDividend();
		ReinvestedDividend(const ReinvestedDividend *dividend_);
		virtual ~ReinvestedDividend();
		Transaction *copy() const;
		
		virtual void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		virtual void writeAttributes(QXmlStreamAttributes *attr);
		
		bool equals(const Transactions *transaction, bool strict_comparison = true) const;
		
		void setSecurity(Security *parent_security);
		
		double shareValue(bool convert = false) const;
		double shares() const;
		void setShares(double new_shares);
		
		virtual QString description() const;
		virtual TransactionSubType subtype() const;
		
};

class Transfer : public Transaction {

	Q_DECLARE_TR_FUNCTIONS(Transfer)
	
	protected:
	
		double d_deposit;

	public:
	
		Transfer(Budget *parent_budget, double initial_amount, QDate initial_date, AssetsAccount *initial_from, AssetsAccount *initial_to, QString initial_description = QString(), QString initial_comment = QString());
		Transfer(Budget *parent_budget, double initial_withdrawal, double initial_deposit, QDate initial_date, AssetsAccount *initial_from, AssetsAccount *initial_to, QString initial_description = QString(), QString initial_comment = QString());
		Transfer(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		Transfer(Budget *parent_budget);
		Transfer();
		Transfer(const Transfer *transfer);
		virtual ~Transfer();
		virtual Transaction *copy() const;

		virtual void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		virtual void writeAttributes(QXmlStreamAttributes *attr);

		AssetsAccount *to() const;
		void setTo(AssetsAccount *new_to);
		AssetsAccount *from() const;
		void setFrom(AssetsAccount *new_from);
		double amount(bool convert = false) const;
		void setValue(double new_value);
		virtual void setAmount(double new_amount);
		virtual void setAmount(double withdrawal, double deposit);
		Currency *withdrawalCurrency() const;
		Currency *depositCurrency() const;
		double withdrawal(bool convert = false) const;
		double deposit(bool convert = false) const;
		double fromValue(bool convert = false) const;
		double toValue(bool convert = false) const;
		virtual QString description() const;
		TransactionType type() const;
		virtual TransactionSubType subtype() const;
		
};

class DebtReduction : public Transfer {

	Q_DECLARE_TR_FUNCTIONS(DebtReduction)

	public:
	
		DebtReduction(Budget *parent_budget, double initial_amount, QDate initial_date, AssetsAccount *initial_from, AssetsAccount *initial_loan, QString initial_comment = QString());
		DebtReduction(Budget *parent_budget, double initial_payment, double initial_reduction, QDate initial_date, AssetsAccount *initial_from, AssetsAccount *initial_loan, QString initial_comment = QString());
		DebtReduction(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		DebtReduction(Budget *parent_budget);
		DebtReduction();
		DebtReduction(const DebtReduction *loanpayment);
		virtual ~DebtReduction();
		Transaction *copy() const;
		
		virtual void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		virtual void writeAttributes(QXmlStreamAttributes *attr);
	
		AssetsAccount *loan() const;
		void setLoan(AssetsAccount *new_loan);
		virtual QString description() const;
		TransactionSubType subtype() const;
	
};


class Balancing : public Transfer {

	Q_DECLARE_TR_FUNCTIONS(Balancing)

	public:

		Balancing(Budget *parent_budget, double initial_amount, QDate initial_date, AssetsAccount *initial_account, QString initial_comment = QString());
		Balancing(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		Balancing(Budget *parent_budget);
		Balancing();
		Balancing(const Balancing *balancing);
		virtual ~Balancing();
		Transaction *copy() const;

		virtual void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		virtual void writeAttributes(QXmlStreamAttributes *attr);
		
		virtual void setAmount(double new_amount);
		virtual void setAmount(double withdrawal, double deposit);

		AssetsAccount *account() const;
		void setAccount(AssetsAccount *new_account);
		
		virtual QString description() const;
		TransactionSubType subtype() const;

};

class SecurityTransaction : public Transaction {

	Q_DECLARE_TR_FUNCTIONS(SecurityTransaction)

	protected:

		Security *o_security;
		
		double d_shares;

	public:

		SecurityTransaction(Security *parent_security, double initial_value, double initial_shares, QDate initial_date, QString initial_comment = QString());
		SecurityTransaction(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		SecurityTransaction(Budget *parent_budget);
		SecurityTransaction();
		SecurityTransaction(const SecurityTransaction *transaction);
		virtual ~SecurityTransaction();
		virtual Transaction *copy() const = 0;

		virtual void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		virtual void writeAttributes(QXmlStreamAttributes *attr);

		bool equals(const Transactions *transaction, bool strict_comparison = true) const;
		virtual Account *fromAccount() const;
		virtual Account *toAccount() const;
		virtual Account *account() const = 0;
		virtual void setAccount(Account *account) = 0;
		virtual TransactionType type() const = 0;
		virtual QString description() const;
		virtual double value(bool convert = false) const;
		virtual Currency *currency() const;
		virtual double fromValue(bool convert = false) const;
		virtual double toValue(bool convert = false) const;
		double shareValue(bool convert = false) const;
		double shares() const;
		void setShares(double new_shares);
		void setSecurity(Security *parent_security);
		Security *security() const;

		virtual bool relatesToAccount(Account *account, bool include_subs = true, bool include_non_value = false) const;
		virtual double accountChange(Account *account, bool include_subs = true, bool convert = false) const;
		
};

class SecurityBuy : public SecurityTransaction {

	Q_DECLARE_TR_FUNCTIONS(SecurityBuy)

	public:

		SecurityBuy(Security *parent_security, double initial_value, double initial_shares, QDate initial_date, Account *from_account, QString initial_comment = QString());
		SecurityBuy(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		SecurityBuy(Budget *parent_budget);
		SecurityBuy();
		SecurityBuy(const SecurityBuy *transaction);
		virtual ~SecurityBuy();
		Transaction *copy() const;

		virtual void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		virtual void writeAttributes(QXmlStreamAttributes *attr);

		Account *account() const;
		void setAccount(Account *account);
		double toValue(bool convert = false) const;
		Account *fromAccount() const;
		QString description() const;
		TransactionType type() const;

};

class SecuritySell : public SecurityTransaction {

	Q_DECLARE_TR_FUNCTIONS(SecuritySell)

	public:

		SecuritySell(Security *parent_security, double initial_value, double initial_shares, QDate initial_date, Account *to_account, QString initial_comment = QString());
		SecuritySell(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		SecuritySell(Budget *parent_budget);
		SecuritySell();
		SecuritySell(const SecuritySell *transaction);
		virtual ~SecuritySell();
		Transaction *copy() const;

		virtual void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		virtual void writeAttributes(QXmlStreamAttributes *attr);

		Account *account() const;
		void setAccount(Account *account);
		double fromValue(bool convert = false) const;
		Account *toAccount() const;
		QString description() const;
		TransactionType type() const;

};

class ScheduledTransaction : public Transactions {

	Q_DECLARE_TR_FUNCTIONS(ScheduledTransaction)

	protected:

		Budget *o_budget;
		Recurrence *o_rec;
		Transactions *o_trans;

	public:

		ScheduledTransaction(Budget *parent_budget);
		ScheduledTransaction(Budget *parent_budget, Transactions *trans, Recurrence *rec);
		ScheduledTransaction(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		ScheduledTransaction(const ScheduledTransaction *strans);
		virtual ~ScheduledTransaction();
		virtual ScheduledTransaction *copy() const;

		virtual void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		virtual bool readElement(QXmlStreamReader *xml, bool *valid);
		virtual bool readElements(QXmlStreamReader *xml, bool *valid);
		virtual void save(QXmlStreamWriter *xml);
		virtual void writeAttributes(QXmlStreamAttributes *attr);
		virtual void writeElements(QXmlStreamWriter *xml);

		virtual bool equals(const Transactions *transaction, bool strict_comparison = true) const;
		virtual Recurrence *recurrence() const;
		virtual void setRecurrence(Recurrence *rec, bool delete_old = true);
		Transactions *realize(QDate date);
		Transactions *transaction() const;
		void setTransaction(Transactions *trans, bool delete_old = true);
		Budget *budget() const;
		virtual const QDate &date() const;
		const qint64 &timestamp() const;
		void setTimestamp(qint64 cr_time);
		virtual const QDate &firstOccurrence() const;
		virtual bool isOneTimeTransaction() const;
		virtual void setDate(QDate newdate);
		virtual void addException(QDate exceptiondate);
		virtual double value(bool convert = false) const;
		virtual Currency *currency() const;
		virtual double quantity() const;
		virtual QString description() const;
		virtual const QString &comment() const;
		virtual const QString &associatedFile() const;
		virtual void setAssociatedFile(QString new_attachment);
		virtual GeneralTransactionType generaltype() const;
		virtual int transactiontype() const;
		virtual int transactionsubtype() const;
		
		virtual bool relatesToAccount(Account *account, bool include_subs = true, bool include_non_value = false) const;
		virtual void replaceAccount(Account *old_account, Account *new_account);
		virtual double accountChange(Account *account, bool include_subs = true, bool convert = false) const;
	
};

class SplitTransaction : public Transactions {

	Q_DECLARE_TR_FUNCTIONS(SplitTransaction)
	
	protected:

		Budget *o_budget;
		QDate d_date;		
		QString s_description;
		QString s_comment;
		QString s_file;
		QVector<Transaction*> splits;
		qint64 i_time;
		
	public:
		
		SplitTransaction(Budget *parent_budget, QDate initial_date, QString initial_description = QString());
		SplitTransaction(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		SplitTransaction(const SplitTransaction *split);
		SplitTransaction(Budget *parent_budget);
		SplitTransaction();
		virtual ~SplitTransaction();
		virtual SplitTransaction *copy() const = 0;
		
		virtual double value(bool convert = false) const = 0;
		virtual Currency *currency() const = 0;
		virtual double cost(bool convert = false) const = 0;
		virtual double quantity() const = 0;
		double income() const;
		
		virtual void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		virtual bool readElement(QXmlStreamReader *xml, bool *valid);
		virtual bool readElements(QXmlStreamReader *xml, bool *valid);
		virtual void save(QXmlStreamWriter *xml);
		virtual void writeAttributes(QXmlStreamAttributes *attr);
		virtual void writeElements(QXmlStreamWriter *xml);

		virtual bool equals(const Transactions *transaction, bool strict_comparison = true) const;

		virtual void addTransaction(Transaction *trans);
		virtual void removeTransaction(Transaction *trans, bool keep = false);
		virtual void clear(bool keep = false);
		
		const QDate &date() const;
		virtual void setDate(QDate new_date);
		const qint64 &timestamp() const;
		void setTimestamp(qint64 cr_time);
		QString description() const;
		virtual void setDescription(QString new_description);
		const QString &comment() const;
		virtual void setComment(QString new_comment);
		const QString &associatedFile() const;
		virtual void setAssociatedFile(QString new_attachment);
		
		virtual int count() const;
		virtual Transaction *operator[] (int index) const;
		virtual Transaction *at(int index) const;
		
		Budget *budget() const;
		
		virtual GeneralTransactionType generaltype() const;
		virtual SplitTransactionType type() const = 0;
		virtual bool isIncomesAndExpenses() const;
		
		virtual bool relatesToAccount(Account *account, bool include_subs = true, bool include_non_value = false) const;
		virtual void replaceAccount(Account *old_account, Account *new_account);
		virtual double accountChange(Account *account, bool include_subs = true, bool convert = false) const;

};

class MultiItemTransaction : public SplitTransaction {

	Q_DECLARE_TR_FUNCTIONS(MultiItemTransaction)
	
	protected:

		AssetsAccount *o_account;
		QString s_payee;

	public:

		MultiItemTransaction(Budget *parent_budget, QDate initial_date, AssetsAccount *initial_account, QString initial_description = QString());
		MultiItemTransaction(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		MultiItemTransaction(const MultiItemTransaction *split);
		MultiItemTransaction(Budget *parent_budget);
		MultiItemTransaction();
		virtual ~MultiItemTransaction();
		virtual SplitTransaction *copy() const;

		virtual void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		virtual bool readElement(QXmlStreamReader *xml, bool *valid);
		virtual void writeAttributes(QXmlStreamAttributes *attr);
		virtual void writeElements(QXmlStreamWriter *xml);

		virtual double value(bool convert = false) const;
		virtual Currency *currency() const;
		virtual double quantity() const;
		virtual double cost(bool convert = false) const;
		double income() const;
		AssetsAccount *account() const;
		void setAccount(AssetsAccount *new_account);
		QString fromAccountsString() const;
		virtual const QString &payee() const;
		void setPayee(QString new_payee);
		
		void addTransaction(Transaction *trans);
		
		virtual SplitTransactionType type() const;
		int transactiontype() const;
		
		virtual bool relatesToAccount(Account *account, bool include_subs = true, bool include_non_value = false) const;
		virtual void replaceAccount(Account *old_account, Account *new_account);

};

class MultiAccountTransaction : public SplitTransaction {

	Q_DECLARE_TR_FUNCTIONS(MultiAccountTransaction)

	protected:
	
		CategoryAccount *o_category;
		double d_quantity;

	public:
	
		MultiAccountTransaction(Budget *parent_budget, CategoryAccount *initial_category, QString initial_description = QString());
		MultiAccountTransaction(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		MultiAccountTransaction(const MultiAccountTransaction *split);
		MultiAccountTransaction(Budget *parent_budget);
		MultiAccountTransaction();
		virtual ~MultiAccountTransaction();
		virtual SplitTransaction *copy() const;
	
		virtual void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		virtual bool readElement(QXmlStreamReader *xml, bool *valid);
		virtual void writeAttributes(QXmlStreamAttributes *attr);
		virtual void writeElements(QXmlStreamWriter *xml);
		
		virtual double value(bool convert = false) const;
		virtual Currency *currency() const;
		virtual double quantity() const;
		void setQuantity(double new_quantity);
		virtual double cost(bool convert = false) const;
		
		CategoryAccount *category() const;
		void setCategory(CategoryAccount *new_category);
		void setDescription(QString new_description);
		
		AssetsAccount *account() const;
		QString accountsString() const;
		QString payees() const;
		
		void addTransaction(Transaction *trans);
		
		virtual SplitTransactionType type() const;
		virtual TransactionType transactiontype() const;
		virtual bool isIncomesAndExpenses() const;
		
		virtual bool relatesToAccount(Account *account, bool include_subs = true, bool include_non_value = false) const;
		virtual void replaceAccount(Account *old_account, Account *new_account);
	
};

class DebtPayment : public SplitTransaction {

	Q_DECLARE_TR_FUNCTIONS(DebtPayment)
	
	protected:
	
		AssetsAccount *o_loan;
		DebtFee *o_fee;
		DebtInterest *o_interest;
		DebtReduction *o_payment;
		AssetsAccount *o_account;
	
	public:
	
		DebtPayment(Budget *parent_budget, QDate initial_date, AssetsAccount *initial_loan, AssetsAccount *initial_account);
		DebtPayment(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		DebtPayment(const DebtPayment *split);
		DebtPayment(Budget *parent_budget);
		DebtPayment();
		virtual ~DebtPayment();
		virtual SplitTransaction *copy() const;
		
		virtual void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		virtual bool readElement(QXmlStreamReader *xml, bool *valid);
		virtual void writeAttributes(QXmlStreamAttributes *attr);
		virtual void writeElements(QXmlStreamWriter *xml);
		
		virtual double value(bool convert = false) const;
		virtual Currency *currency() const;
		virtual double quantity() const;
		virtual double cost(bool convert = false) const;
		
		DebtFee *feeTransaction() const;
		DebtInterest *interestTransaction() const;
		DebtReduction *paymentTransaction() const;
		
		void setInterest(double new_value, bool payed_from_account = true);
		void setInterestPayed(bool payed_from_account);
		void setFee(double new_value);
		void setPayment(double new_value);
		void setPayment(double new_payment, double new_reduction);
		double interest(bool convert = false) const;
		bool interestPayed() const;
		double fee(bool convert = false) const;
		double payment(bool convert = false) const;
		double reduction(bool convert = false) const;
		
		void clear(bool keep = false);
		
		AssetsAccount *loan() const;
		void setLoan(AssetsAccount *new_loan);
		AssetsAccount *account() const;
		void setAccount(AssetsAccount *new_account);
		ExpensesAccount *expenseCategory() const;
		void setExpenseCategory(ExpensesAccount *new_category);
		
		QString description() const;
		virtual void setDate(QDate new_date);
		
		SplitTransactionType type() const;
		virtual bool isIncomesAndExpenses() const;
		
		int count() const;
		virtual Transaction *operator[] (int index) const;
		virtual Transaction *at(int index) const;
		virtual void removeTransaction(Transaction *trans, bool keep = false);
		
		virtual bool relatesToAccount(Account *account, bool include_subs = true, bool include_non_value = false) const;
		virtual void replaceAccount(Account *old_account, Account *new_account);
		virtual double accountChange(Account *account, bool include_subs = true, bool convert = false) const;
		
};

class SecurityTrade {
	public:
		SecurityTrade(const QDate &date_, double from_shares_, Security *from_security_, double to_shares_, Security *to_security_) : date(date_), from_shares(from_shares_), to_shares(to_shares_), from_security(from_security_), to_security(to_security_), timestamp(QDateTime::currentMSecsSinceEpoch() * 1000) {}
		QDate date;		
		double from_shares, to_shares;
		Security *from_security, *to_security;
		qint64 timestamp;
};


#endif
