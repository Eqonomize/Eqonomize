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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QXmlStreamAttribute>

#include <QDebug>

#include "account.h"
#include "budget.h"
#include "recurrence.h"
#include "security.h"
#include "transaction.h"

static const QString emptystr;
static const QDate emptydate;
static qint64 zero_timestamp;

Transactions::Transactions() {}
QString Transactions::valueString(int precision) const {
	return valueString(value(), precision);
}
QString Transactions::valueString(double value_, int precision) const {
	Currency *cur = currency();
	if(!cur) cur = budget()->defaultCurrency();
	return cur->formatValue(value_, precision);
}
void Transactions::setTimestamp() {
	setTimestamp(QDateTime::currentMSecsSinceEpoch() * 1000);
}

Transaction::Transaction(Budget *parent_budget, double initial_value, QDate initial_date, Account *from, Account *to, QString initial_description, QString initial_comment) : o_budget(parent_budget), d_value(initial_value), d_date(initial_date), o_from(from), o_to(to), s_description(initial_description.trimmed()), s_comment(initial_comment.trimmed()), d_quantity(1.0), o_split(NULL), i_time(QDateTime::currentMSecsSinceEpoch() * 1000) {}
Transaction::Transaction(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : o_budget(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
Transaction::Transaction(Budget *parent_budget) : o_budget(parent_budget), d_value(0.0), o_from(NULL), o_to(NULL), d_quantity(1.0), o_split(NULL), i_time(QDateTime::currentMSecsSinceEpoch() * 1000) {}
Transaction::Transaction() : o_budget(NULL), d_value(0.0), o_from(NULL), o_to(NULL), d_quantity(1.0), o_split(NULL), i_time(QDateTime::currentMSecsSinceEpoch() * 1000) {}
Transaction::Transaction(const Transaction *transaction) : o_budget(transaction->budget()), d_value(transaction->value()), d_date(transaction->date()), o_from(transaction->fromAccount()), o_to(transaction->toAccount()), s_description(transaction->description()), s_comment(transaction->comment()), s_file(transaction->associatedFile()), d_quantity(transaction->quantity()), o_split(NULL), i_time(transaction->timestamp()) {}
Transaction::~Transaction() {}

void Transaction::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	o_split = NULL;
	o_from = NULL; o_to = NULL;
	d_date = QDate::fromString(attr->value("date").toString(), Qt::ISODate);
	i_time = attr->value("timestamp").toString().toLongLong();
	s_description = attr->value("description").trimmed().toString();
	s_comment = attr->value("comment").trimmed().toString();
	s_file = attr->value("file").trimmed().toString();
	if(attr->hasAttribute("quantity")) d_quantity = attr->value("quantity").toDouble();
	else d_quantity = 1.0;
	if(valid && (*valid)) *valid = d_date.isValid();
}
bool Transaction::readElement(QXmlStreamReader*, bool*) {return false;}
bool Transaction::readElements(QXmlStreamReader *xml, bool *valid) {
	while(xml->readNextStartElement()) {
		if(!readElement(xml, valid)) xml->skipCurrentElement();
	}
	return true;
}
void Transaction::save(QXmlStreamWriter *xml) {
	QXmlStreamAttributes attr;
	writeAttributes(&attr);
	xml->writeAttributes(attr);
	writeElements(xml);
}
void Transaction::writeAttributes(QXmlStreamAttributes *attr) {
	attr->append("date", d_date.toString(Qt::ISODate));
	if(i_time != 0) attr->append("timestamp", QString::number(i_time));
	if(!s_description.isEmpty()) attr->append("description", s_description);
	if(!s_comment.isEmpty()) attr->append("comment", s_comment);
	if(!s_file.isEmpty()) attr->append("file", s_file);
	if(d_quantity != 1.0) attr->append("quantity", QString::number(d_quantity, 'f', QUANTITY_DECIMAL_PLACES));
}
void Transaction::writeElements(QXmlStreamWriter*) {}

bool Transaction::equals(const Transactions *trans, bool strict_comparison) const {
	if(trans->generaltype() != GENERAL_TRANSACTION_TYPE_SINGLE) return false;
	Transaction *transaction = (Transaction*) trans;
	if(type() != transaction->type()) return false;
	if(fromAccount() != transaction->fromAccount()) return false;
	if(toAccount() != transaction->toAccount()) return false;
	if(value() != transaction->value()) return false;
	if(date() != transaction->date()) return false;
	if(description() != transaction->description()) return false;
	if(strict_comparison && quantity() != transaction->quantity()) return false;
	if(comment() != transaction->comment() && (strict_comparison || comment().isEmpty() == transaction->comment().isEmpty())) return false;
	if(associatedFile() != transaction->associatedFile() && (strict_comparison || associatedFile().isEmpty() == transaction->associatedFile().isEmpty())) return false;
	if(budget() != transaction->budget()) return false;
	return true;
}

SplitTransaction *Transaction::parentSplit() const {return o_split;}
void Transaction::setParentSplit(SplitTransaction *parent) {
	if(o_split == parent) return;
	o_split = parent;
	if(o_split) i_time = o_split->timestamp();
	o_budget->transactionSortModified(this);
}
double Transaction::value(bool convert) const {
	if(convert && currency() && currency() != budget()->defaultCurrency()) {
		if(budget()->defaultTransactionConversionRateDate() == TRANSACTION_CONVERSION_RATE_AT_DATE) return currency()->convertTo(d_value, budget()->defaultCurrency(), d_date);
		else return currency()->convertTo(d_value, budget()->defaultCurrency());
	}
	return d_value;
}
double Transaction::fromValue(bool convert) const {return value(convert);}
double Transaction::toValue(bool convert) const {return value(convert);}
Currency *Transaction::currency() const {
	if(fromAccount()->type() == ACCOUNT_TYPE_ASSETS && ((AssetsAccount*) fromAccount())->currency()) return ((AssetsAccount*) fromAccount())->currency();
	if(toAccount()->type() == ACCOUNT_TYPE_ASSETS) return ((AssetsAccount*) toAccount())->currency();
	return NULL;
}
Currency *Transaction::fromCurrency() const {
	if(fromAccount()->type() == ACCOUNT_TYPE_ASSETS && ((AssetsAccount*) fromAccount())->currency()) return ((AssetsAccount*) fromAccount())->currency();
	if(toAccount()->type() == ACCOUNT_TYPE_ASSETS) return ((AssetsAccount*) toAccount())->currency();
	return NULL;
}
Currency *Transaction::toCurrency() const {
	if(toAccount()->type() == ACCOUNT_TYPE_ASSETS && ((AssetsAccount*) toAccount())->currency()) return ((AssetsAccount*) toAccount())->currency();
	if(fromAccount()->type() == ACCOUNT_TYPE_ASSETS) return ((AssetsAccount*) fromAccount())->currency();
	return NULL;
}
void Transaction::setValue(double new_value) {d_value = new_value;}
double Transaction::quantity() const {return d_quantity;}
void Transaction::setQuantity(double new_quantity) {d_quantity = new_quantity;}
const QDate &Transaction::date() const {return d_date;}
void Transaction::setDate(QDate new_date) {
	if(new_date == d_date) return;
	QDate old_date = d_date; d_date = new_date;
	o_budget->transactionSortModified(this);
	o_budget->transactionDateModified(this, old_date);
}
const qint64 &Transaction::timestamp() const {return i_time;}
void Transaction::setTimestamp(qint64 cr_time) {
	if(i_time == cr_time) return;
	i_time = cr_time; o_budget->transactionSortModified(this);
}
QString Transaction::description() const {return s_description;}
void Transaction::setDescription(QString new_description) {
	if(new_description == s_description) return;
	s_description = new_description.trimmed(); 
	o_budget->transactionSortModified(this);
}
const QString &Transaction::comment() const {return s_comment;}
void Transaction::setComment(QString new_comment) {s_comment = new_comment.trimmed();}
const QString &Transaction::associatedFile() const {return s_file;}
void Transaction::setAssociatedFile(QString new_attachment) {s_file = new_attachment.trimmed();}
Account *Transaction::fromAccount() const {return o_from;}
void Transaction::setFromAccount(Account *new_from) {o_from = new_from;}
Account *Transaction::toAccount() const {return o_to;}
void Transaction::setToAccount(Account *new_to) {o_to = new_to;}
Budget *Transaction::budget() const {return o_budget;}
GeneralTransactionType Transaction::generaltype() const {return GENERAL_TRANSACTION_TYPE_SINGLE;}
TransactionSubType Transaction::subtype() const {return (TransactionSubType) type();}
bool Transaction::relatesToAccount(Account *account, bool include_subs, bool) const {return o_from == account || o_to == account || (include_subs && (o_from->topAccount() == account || o_to->topAccount() == account));}
void Transaction::replaceAccount(Account *old_account, Account *new_account) {
	if(o_from == old_account) o_from = new_account;
	if(o_to == old_account) o_to = new_account;
}
double Transaction::accountChange(Account *account, bool include_subs, bool convert) const {
	if(o_from == account || (include_subs && o_from->topAccount() == account)) return -fromValue(convert);
	if(o_to == account || (include_subs && o_to->topAccount() == account)) return toValue(convert);
	return 0.0;
}

Expense::Expense(Budget *parent_budget, double initial_cost, QDate initial_date, ExpensesAccount *initial_category, AssetsAccount *initial_from, QString initial_description, QString initial_comment) : Transaction(parent_budget, initial_cost, initial_date, initial_from, initial_category, initial_description, initial_comment), b_reconciled(false) {}
Expense::Expense(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : Transaction(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
Expense::Expense(Budget *parent_budget) : Transaction(parent_budget), b_reconciled(false) {}
Expense::Expense() : Transaction() {}
Expense::Expense(const Expense *expense) : Transaction(expense), s_payee(expense->payee()), b_reconciled(expense->isReconciled(expense->from())) {}
Expense::~Expense() {}
Transaction *Expense::copy() const {return new Expense(this);}

void Expense::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	Transaction::readAttributes(attr, valid);
	int id_category = attr->value("category").toInt();
	int id_from = attr->value("from").toInt();
	if(budget()->expensesAccounts_id.contains(id_category) && budget()->assetsAccounts_id.contains(id_from)) {
		setCategory(budget()->expensesAccounts_id[id_category]);
		setFrom(budget()->assetsAccounts_id[id_from]);
		if(attr->hasAttribute("income")) setCost(-attr->value("income").toDouble());
		else setCost(attr->value("cost").toDouble());
		s_payee = attr->value("payee").trimmed().toString();
		b_reconciled = attr->value("reconciled").toInt();
	} else {
		if(valid) *valid = false;
	}
}
void Expense::writeAttributes(QXmlStreamAttributes *attr) {
	Transaction::writeAttributes(attr);
	if(cost() < 0.0) attr->append("income", QString::number(-cost(), 'f', SAVE_MONETARY_DECIMAL_PLACES));
	else attr->append("cost", QString::number(cost(), 'f', SAVE_MONETARY_DECIMAL_PLACES));
	attr->append("category", QString::number(category()->id()));
	attr->append("from", QString::number(from()->id()));
	if(!s_payee.isEmpty()) attr->append("payee", s_payee);
	if(b_reconciled) attr->append("reconciled", QString::number(b_reconciled));
}

bool Expense::equals(const Transactions *transaction, bool strict_comparison) const {
	if(!Transaction::equals(transaction, strict_comparison)) return false;
	Expense *expense = (Expense*) transaction;
	if(s_payee != expense->payee() && (strict_comparison || s_payee.isEmpty() == expense->payee().isEmpty())) return false;
	return true;
}

ExpensesAccount *Expense::category() const {return (ExpensesAccount*) toAccount();}
void Expense::setCategory(ExpensesAccount *new_category) {setToAccount(new_category);}
AssetsAccount *Expense::from() const {return (AssetsAccount*) fromAccount();}
void Expense::setFrom(AssetsAccount *new_from) {setFromAccount(new_from);}
double Expense::cost(bool convert) const {return value(convert);}
void Expense::setCost(double new_cost) {setValue(new_cost);}
const QString &Expense::payee() const {return s_payee;}
void Expense::setPayee(QString new_payee) {s_payee = new_payee.trimmed();}
QString Expense::description() const {return Transaction::description();}
TransactionType Expense::type() const {return TRANSACTION_TYPE_EXPENSE;}
TransactionSubType Expense::subtype() const {return TRANSACTION_SUBTYPE_EXPENSE;}
bool Expense::relatesToAccount(Account *account, bool include_subs, bool include_non_value) const {return Transaction::relatesToAccount(account, include_subs, include_non_value);}
void Expense::replaceAccount(Account *old_account, Account *new_account) {Transaction::replaceAccount(old_account, new_account);}
bool Expense::isReconciled(AssetsAccount *account) const {
	if(account == from()) return b_reconciled;
	return false;
}
void Expense::setReconciled(AssetsAccount *account, bool is_reconciled) {
	if(account == from()) b_reconciled = is_reconciled;
}

DebtFee::DebtFee(Budget *parent_budget, double initial_cost, QDate initial_date, ExpensesAccount *initial_category, AssetsAccount *initial_from, AssetsAccount *initial_loan, QString initial_comment) : Expense(parent_budget, initial_cost, initial_date, initial_category, initial_from, QString(), initial_comment), o_loan(initial_loan) {
}
DebtFee::DebtFee(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : Expense(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
DebtFee::DebtFee(Budget *parent_budget) : Expense(parent_budget), o_loan(NULL) {}
DebtFee::DebtFee() : Expense(), o_loan(NULL) {}
DebtFee::DebtFee(const DebtFee *loanfee) : Expense(loanfee) {
	o_loan = loanfee->loan();
}
bool DebtFee::relatesToAccount(Account *account, bool include_subs, bool include_non_value) const {return (include_non_value && o_loan == account) || Expense::relatesToAccount(account, include_subs, include_non_value);}
DebtFee::~DebtFee() {}
Transaction *DebtFee::copy() const {return new DebtFee(this);}

void DebtFee::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	Expense::readAttributes(attr, valid);
	int id_loan = attr->value("debt").toInt();
	if(budget()->assetsAccounts_id.contains(id_loan)) {
		o_loan = budget()->assetsAccounts_id[id_loan];
	} else {
		if(valid) *valid = false;
	}
}
void DebtFee::writeAttributes(QXmlStreamAttributes *attr) {
	Expense::writeAttributes(attr);
	attr->append("debt", QString::number(o_loan->id()));
}

AssetsAccount *DebtFee::loan() const {return o_loan;}
void DebtFee::setLoan(AssetsAccount *new_loan) {o_loan = new_loan;}
const QString &DebtFee::payee() const {
	if(o_loan) return o_loan->maintainer();
	return s_payee;
}
QString DebtFee::description() const {return tr("Debt payment: %1 (fee)").arg(o_loan->name());}
TransactionSubType DebtFee::subtype() const {return TRANSACTION_SUBTYPE_DEBT_FEE;}
void DebtFee::replaceAccount(Account *old_account, Account *new_account) {
	if(o_loan == old_account && new_account->type() == ACCOUNT_TYPE_ASSETS) o_loan = (AssetsAccount*) new_account;
	Transaction::replaceAccount(old_account, new_account);
}

DebtInterest::DebtInterest(Budget *parent_budget, double initial_cost, QDate initial_date, ExpensesAccount *initial_category, AssetsAccount *initial_from, AssetsAccount *initial_loan, QString initial_comment) : Expense(parent_budget, initial_cost, initial_date, initial_category, initial_from, QString(), initial_comment), o_loan(initial_loan) {
}
DebtInterest::DebtInterest(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : Expense(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
DebtInterest::DebtInterest(Budget *parent_budget) : Expense(parent_budget), o_loan(NULL) {}
DebtInterest::DebtInterest() : Expense(), o_loan(NULL) {}
DebtInterest::DebtInterest(const DebtInterest *loaninterest) : Expense(loaninterest) {
	o_loan = loaninterest->loan();
}
DebtInterest::~DebtInterest() {}
Transaction *DebtInterest::copy() const {return new DebtInterest(this);}

void DebtInterest::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	Expense::readAttributes(attr, valid);
	int id_loan = attr->value("debt").toInt();
	if(budget()->assetsAccounts_id.contains(id_loan)) {
		o_loan = budget()->assetsAccounts_id[id_loan];
	} else {
		if(valid) *valid = false;
	}
}
void DebtInterest::writeAttributes(QXmlStreamAttributes *attr) {
	Expense::writeAttributes(attr);
	attr->append("debt", QString::number(o_loan->id()));
}

AssetsAccount *DebtInterest::loan() const {return o_loan;}
void DebtInterest::setLoan(AssetsAccount *new_loan) {o_loan = new_loan;}
const QString &DebtInterest::payee() const {
	if(o_loan) return o_loan->maintainer();
	return s_payee;
}
QString DebtInterest::description() const {return tr("Debt payment: %1 (interest)").arg(o_loan->name());}
TransactionSubType DebtInterest::subtype() const {return TRANSACTION_SUBTYPE_DEBT_INTEREST;}
bool DebtInterest::relatesToAccount(Account *account, bool include_subs, bool include_non_value) const {return (include_non_value && o_loan == account) || Expense::relatesToAccount(account, include_subs, include_non_value);}
void DebtInterest::replaceAccount(Account *old_account, Account *new_account) {
	if(o_loan == old_account && new_account->type() == ACCOUNT_TYPE_ASSETS) o_loan = (AssetsAccount*) new_account;
	Transaction::replaceAccount(old_account, new_account);
}

Income::Income(Budget *parent_budget, double initial_income, QDate initial_date, IncomesAccount *initial_category, AssetsAccount *initial_to, QString initial_description, QString initial_comment) : Transaction(parent_budget, initial_income, initial_date, initial_category, initial_to, initial_description, initial_comment), o_security(NULL), b_reconciled(false) {}
Income::Income(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : Transaction(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
Income::Income(Budget *parent_budget) : Transaction(parent_budget), o_security(NULL), b_reconciled(false) {}
Income::Income() : Transaction(), o_security(NULL), b_reconciled(false) {}
Income::Income(const Income *income_) : Transaction(income_), o_security(income_->security()), s_payer(income_->payer()), b_reconciled(income_->isReconciled(income_->to())) {}
Income::~Income() {}
Transaction *Income::copy() const {return new Income(this);}

void Income::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	Transaction::readAttributes(attr, valid);
	int id_category = attr->value("category").toInt();
	int id_to = attr->value("to").toInt();
	if(budget()->incomesAccounts_id.contains(id_category) && budget()->assetsAccounts_id.contains(id_to)) {
		setCategory(budget()->incomesAccounts_id[id_category]);
		setTo(budget()->assetsAccounts_id[id_to]);
		if(attr->hasAttribute("cost")) setIncome(-attr->value("cost").toDouble());
		else setIncome(attr->value("income").toDouble());
		b_reconciled = attr->value("reconciled").toInt();
	} else {
		if(valid) *valid = false;
	}
	int id = -1;
	if(attr->hasAttribute("security")) id = attr->value("security").toInt();
	if(id >= 0 && budget()->securities_id.contains(id)) {
		o_security = budget()->securities_id[id];
	} else {
		o_security = NULL;
	}
	if(!o_security) {
		s_payer = attr->value("payer").trimmed().toString();
	}
}
void Income::writeAttributes(QXmlStreamAttributes *attr) {
	Transaction::writeAttributes(attr);
	if(income() < 0.0 && !o_security) attr->append("cost", QString::number(-income(), 'f', SAVE_MONETARY_DECIMAL_PLACES));
	else attr->append("income", QString::number(income(), 'f', SAVE_MONETARY_DECIMAL_PLACES));
	attr->append("category", QString::number(category()->id()));
	attr->append("to", QString::number(to()->id()));
	if(o_security) attr->append("security", QString::number(o_security->id()));
	else if(!s_payer.isEmpty()) attr->append("payer", s_payer);
	if(b_reconciled) attr->append("reconciled", QString::number(b_reconciled));
}

bool Income::equals(const Transactions *transaction, bool strict_comparison) const {
	if(!Transaction::equals(transaction, strict_comparison)) return false;
	Income *income = (Income*) transaction;
	if(s_payer != income->payer() && (strict_comparison || s_payer.isEmpty() == income->payer().isEmpty())) return false;
	if(o_security != income->security()) return false;
	return true;
}

IncomesAccount *Income::category() const {return (IncomesAccount*) fromAccount();}
void Income::setCategory(IncomesAccount *new_category) {setFromAccount(new_category);}
AssetsAccount *Income::to() const {return (AssetsAccount*) toAccount();}
void Income::setTo(AssetsAccount *new_to) {setToAccount(new_to);}
double Income::income(bool convert) const {return value(convert);}
void Income::setIncome(double new_income) {setValue(new_income);}
const QString &Income::payer() const {
	if(o_security) return o_security->name(); 
	return s_payer;
}
void Income::setPayer(QString new_payer) {s_payer = new_payer.trimmed();}
QString Income::description() const {
	if(o_security) return tr("Dividend: %1").arg(o_security->name());
	return Transaction::description();
}
TransactionType Income::type() const {return TRANSACTION_TYPE_INCOME;}
TransactionSubType Income::subtype() const {return TRANSACTION_SUBTYPE_INCOME;}
void Income::setSecurity(Security *parent_security) {
	o_security = parent_security;
	if(o_security) {
		setDescription(QString());
	}
}
Security *Income::security() const {return o_security;}
bool Income::isReconciled(AssetsAccount *account) const {
	if(account == to()) return b_reconciled;
	return false;
}
void Income::setReconciled(AssetsAccount *account, bool is_reconciled) {
	if(account == to()) b_reconciled = is_reconciled;
}

ReinvestedDividend::ReinvestedDividend(Budget *parent_budget, double initial_value, double initial_shares, QDate initial_date, Security *initial_security, IncomesAccount *initial_category, QString initial_comment) : Income(parent_budget, initial_value, initial_date, initial_category, initial_security ? initial_security->account() : NULL, QString::null, initial_comment), d_shares(initial_shares) {
	o_security = initial_security;
}
ReinvestedDividend::ReinvestedDividend(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : Income(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
ReinvestedDividend::ReinvestedDividend(Budget *parent_budget) : Income(parent_budget), d_shares(0.0) {}
ReinvestedDividend::ReinvestedDividend() : Income(), d_shares(0.0) {}
ReinvestedDividend::ReinvestedDividend(const ReinvestedDividend *reinv) : Income(reinv), d_shares(reinv->shares()) {}
ReinvestedDividend::~ReinvestedDividend() {}
Transaction *ReinvestedDividend::copy() const {return new ReinvestedDividend(this);}

void ReinvestedDividend::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	Transaction::readAttributes(attr, valid);
	int id_category = attr->value("category").toInt();
	int id_sec = attr->value("security").toInt();
	if(budget()->securities_id.contains(id_sec)) {
		if(budget()->incomesAccounts_id.contains(id_category)) setCategory(budget()->incomesAccounts_id[id_category]);
		else setCategory(budget()->null_incomes_account);
		o_security = budget()->securities_id[id_sec];
		setTo(o_security->account());
		d_value = attr->value("value").toDouble();
		d_shares = attr->value("shares").toDouble();
		if(attr->hasAttribute("sharevalue")) {
			double v = attr->value("sharevalue").toDouble();
			if(d_shares <= 0.0 && v != 0.0) d_shares = d_value / v;
			else if(d_value == 0.0) d_value = d_shares * v;
		}
	} else {
		if(valid) *valid = false;
	}
}
void ReinvestedDividend::writeAttributes(QXmlStreamAttributes *attr) {
	Transaction::writeAttributes(attr);
	attr->append("value", QString::number(income(), 'f', SAVE_MONETARY_DECIMAL_PLACES));
	attr->append("shares", QString::number(d_shares, 'f', o_security->decimals()));
	if(category() && category() != budget()->null_incomes_account) attr->append("category", QString::number(category()->id()));
	attr->append("security", QString::number(o_security->id()));
}

bool ReinvestedDividend::equals(const Transactions *transaction, bool strict_comparison) const {
	if(!Income::equals(transaction, strict_comparison) || ((Income*) transaction)->subtype() == subtype()) return false;
	ReinvestedDividend *reinv = (ReinvestedDividend*) transaction;
	if(d_shares != reinv->shares()) return false;
	return true;
}
double ReinvestedDividend::shares() const {return d_shares;}
double ReinvestedDividend::shareValue(bool convert) const {
	double v = 0.0;
	if(o_security) v = o_security->getQuotation(date());
	if(convert && o_security && o_security->currency()) {
		if(budget()->defaultTransactionConversionRateDate() == TRANSACTION_CONVERSION_RATE_AT_DATE) return o_security->currency()->convertTo(v, budget()->defaultCurrency(), date());
		else return o_security->currency()->convertTo(v, budget()->defaultCurrency());
	}
	return v;
}
void ReinvestedDividend::setShares(double new_shares) {
	d_shares = new_shares;
}
QString ReinvestedDividend::description() const {
	return tr("Reinvested dividend: %1").arg(o_security->name());
}
TransactionSubType ReinvestedDividend::subtype() const {return TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND;}
void ReinvestedDividend::setSecurity(Security *parent_security) {
	o_security = parent_security;
	setTo(o_security->account());
}

Transfer::Transfer(Budget *parent_budget, double initial_amount, QDate initial_date, AssetsAccount *initial_from, AssetsAccount *initial_to, QString initial_description, QString initial_comment) : Transaction(parent_budget, initial_amount < 0.0 ? -initial_amount : initial_amount, initial_date, initial_amount < 0.0 ? initial_to : initial_from, initial_amount < 0.0 ? initial_from : initial_to, initial_description, initial_comment), b_from_reconciled(false), b_to_reconciled(false) {
	d_deposit = amount();
}
Transfer::Transfer(Budget *parent_budget, double initial_withdrawal, double initial_deposit, QDate initial_date, AssetsAccount *initial_from, AssetsAccount *initial_to, QString initial_description, QString initial_comment) : Transaction(parent_budget, initial_withdrawal < 0.0 ? -initial_deposit : initial_withdrawal, initial_date, initial_withdrawal < 0.0 ? initial_to : initial_from, initial_withdrawal < 0.0 ? initial_from : initial_to, initial_description, initial_comment), b_from_reconciled(false), b_to_reconciled(false) {
	if(initial_withdrawal < 0.0) {
		d_deposit = -initial_withdrawal;
	} else {
		d_deposit = initial_deposit;
	}
}
Transfer::Transfer(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : Transaction(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
Transfer::Transfer(Budget *parent_budget) : Transaction(parent_budget), b_from_reconciled(false), b_to_reconciled(false) {}
Transfer::Transfer() : Transaction() {}
Transfer::Transfer(const Transfer *transfer) : Transaction(transfer), b_from_reconciled(transfer->isReconciled(transfer->from())), b_to_reconciled(transfer->isReconciled(transfer->to())) {
	d_deposit = transfer->deposit();
}
Transfer::~Transfer() {}
Transaction *Transfer::copy() const {return new Transfer(this);}

void Transfer::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	Transaction::readAttributes(attr, valid);
	int id_from = attr->value("from").toInt();
	int id_to = attr->value("to").toInt();
	if(budget()->assetsAccounts_id.contains(id_from) && budget()->assetsAccounts_id.contains(id_to)) {
		setFrom(budget()->assetsAccounts_id[id_from]);
		setTo(budget()->assetsAccounts_id[id_to]);
		if(attr->hasAttribute("amount")) {
			setAmount(attr->value("amount").toDouble());
		} else {
			setAmount(attr->value("withdrawal").toDouble(), attr->value("deposit").toDouble());
		}
		b_from_reconciled = attr->value("fromreconciled").toInt();
		b_to_reconciled = attr->value("toreconciled").toInt();
	} else {
		if(valid) *valid =false;
	}
}
void Transfer::writeAttributes(QXmlStreamAttributes *attr) {
	Transaction::writeAttributes(attr);
	if(d_deposit != amount()) {
		attr->append("withdrawal", QString::number(amount(), 'f', SAVE_MONETARY_DECIMAL_PLACES));
		attr->append("deposit", QString::number(d_deposit, 'f', SAVE_MONETARY_DECIMAL_PLACES));
	} else {
		attr->append("amount", QString::number(amount(), 'f', SAVE_MONETARY_DECIMAL_PLACES));
	}
	attr->append("from", QString::number(from()->id()));
	attr->append("to", QString::number(to()->id()));
	if(b_from_reconciled) attr->append("fromreconciled", QString::number(b_from_reconciled));
	if(b_to_reconciled) attr->append("toreconciled", QString::number(b_to_reconciled));
}

AssetsAccount *Transfer::to() const {return (AssetsAccount*) toAccount();}
void Transfer::setTo(AssetsAccount *new_to) {setToAccount(new_to);}
AssetsAccount *Transfer::from() const {return (AssetsAccount*) fromAccount();}
void Transfer::setFrom(AssetsAccount *new_from) {setFromAccount(new_from);}
double Transfer::amount(bool convert) const {return value(convert);}
void Transfer::setValue(double new_value) {
	Transaction::setValue(new_value);
	d_deposit = new_value;
}
void Transfer::setAmount(double new_amount) {
	if(new_amount < 0.0) {
		setValue(-new_amount);
		AssetsAccount *from_bak = from();
		setFrom(to());
		setTo(from_bak);
	} else {
		setValue(new_amount);
	}
}
void Transfer::setAmount(double withdrawal_amount, double deposit_amount) {
	if(withdrawal_amount < 0.0) {
		setValue(-withdrawal_amount);
		AssetsAccount *from_bak = from();
		setFrom(to());
		setTo(from_bak);
		d_deposit = -deposit_amount;
	} else {
		setValue(withdrawal_amount);
		d_deposit = deposit_amount;
	}
}
Currency *Transfer::withdrawalCurrency() const {
	return from()->currency();
}
Currency *Transfer::depositCurrency() const {
	if(!to()->currency()) return from()->currency();
	return to()->currency();
}
double Transfer::withdrawal(bool convert) const {
	return value(convert);
}
double Transfer::deposit(bool convert) const {
	if(convert && to() && to()->currency()) {
		if(budget()->defaultTransactionConversionRateDate() == TRANSACTION_CONVERSION_RATE_AT_DATE) return to()->currency()->convertTo(d_deposit, budget()->defaultCurrency(), date());
		else return to()->currency()->convertTo(d_deposit, budget()->defaultCurrency());
	} else if(convert) return withdrawal(true);
	return d_deposit;
}
double Transfer::fromValue(bool convert) const {return withdrawal(convert);}
double Transfer::toValue(bool convert) const {return deposit(convert);}
QString Transfer::description() const {return Transaction::description();}
TransactionType Transfer::type() const {return TRANSACTION_TYPE_TRANSFER;}
TransactionSubType Transfer::subtype() const {return TRANSACTION_SUBTYPE_TRANSFER;}
bool Transfer::isReconciled(AssetsAccount *account) const {
	if(account == from()) return b_from_reconciled;
	if(account == to()) return b_to_reconciled;
	return false;
}
void Transfer::setReconciled(AssetsAccount *account, bool is_reconciled) {
	if(account == from()) b_from_reconciled = is_reconciled;
	if(account == to()) b_to_reconciled = is_reconciled;
}


DebtReduction::DebtReduction(Budget *parent_budget, double initial_amount, QDate initial_date, AssetsAccount *initial_from, AssetsAccount *initial_loan, QString initial_comment) : Transfer(parent_budget, initial_amount, initial_date, initial_from, initial_loan, QString(), initial_comment) {}
DebtReduction::DebtReduction(Budget *parent_budget, double initial_payment, double initial_reduction, QDate initial_date, AssetsAccount *initial_from, AssetsAccount *initial_loan, QString initial_comment) : Transfer(parent_budget, initial_payment, initial_reduction, initial_date, initial_from, initial_loan, QString(), initial_comment) {}
DebtReduction::DebtReduction(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : Transfer(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
DebtReduction::DebtReduction(Budget *parent_budget) : Transfer(parent_budget) {}
DebtReduction::DebtReduction() : Transfer() {}
DebtReduction::DebtReduction(const DebtReduction *loanpayment) : Transfer(loanpayment) {}
DebtReduction::~DebtReduction() {}
Transaction *DebtReduction::copy() const {return new DebtReduction(this);}

void DebtReduction::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	Transfer::readAttributes(attr, valid);
	int id_loan = attr->value("debt").toInt();
	if(budget()->assetsAccounts_id.contains(id_loan)) {
		setTo(budget()->assetsAccounts_id[id_loan]);
	} else {
		if(valid) *valid = false;
	}
}
void DebtReduction::writeAttributes(QXmlStreamAttributes *attr) {
	Transfer::writeAttributes(attr);
	attr->append("debt", QString::number(loan()->id()));
}

AssetsAccount *DebtReduction::loan() const {return (AssetsAccount*) to();}
void DebtReduction::setLoan(AssetsAccount *new_loan) {setTo(new_loan);}
QString DebtReduction::description() const {return tr("Debt payment: %1 (reduction)").arg(loan()->name());}
TransactionSubType DebtReduction::subtype() const {return TRANSACTION_SUBTYPE_DEBT_REDUCTION;}

Balancing::Balancing(Budget *parent_budget, double initial_amount, QDate initial_date, AssetsAccount *initial_account, QString initial_comment) : Transfer(parent_budget) {
	d_value = -initial_amount;
	d_deposit = -initial_amount;
	d_date = initial_date;
	s_comment = initial_comment;
	o_from = initial_account;
	o_to = budget()->balancingAccount;
	
}
Balancing::Balancing(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : Transfer(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
Balancing::Balancing(Budget *parent_budget) : Transfer(parent_budget) {}
Balancing::Balancing() : Transfer() {}
Balancing::Balancing(const Balancing *balancing) : Transfer(balancing) {}
Balancing::~Balancing() {}
Transaction *Balancing::copy() const {return new Balancing(this);}
QString Balancing::description() const {
	if(s_description.isEmpty()) return tr("Account Balance Adjustment");
	return s_description;
}
TransactionSubType Balancing::subtype() const {return TRANSACTION_SUBTYPE_BALANCING;}

void Balancing::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	Transaction::readAttributes(attr, valid);
	d_value = -attr->value("amount").toDouble();
	setToAccount(budget()->balancingAccount);
	setFromAccount(NULL);
	int id_account = attr->value("account").toInt();
	if(budget()->assetsAccounts_id.contains(id_account)) {
		setFromAccount(budget()->assetsAccounts_id[id_account]);
	} else {
		if(valid) *valid = false;
	}
}
void Balancing::writeAttributes(QXmlStreamAttributes *attr) {
	Transaction::writeAttributes(attr);
	attr->append("amount", QString::number(-amount(), 'f', SAVE_MONETARY_DECIMAL_PLACES));
	attr->append("account", QString::number(account()->id()));
}
void Balancing::setAmount(double new_amount) {
	setValue(-new_amount);
}
void Balancing::setAmount(double withdrawal_amount, double) {
	setValue(withdrawal_amount);
}

AssetsAccount *Balancing::account() const {return toAccount() == o_budget->balancingAccount ? (AssetsAccount*) fromAccount() : (AssetsAccount*) toAccount();}
void Balancing::setAccount(AssetsAccount *new_account) {toAccount() == o_budget->balancingAccount ? setFromAccount(new_account) : setToAccount(new_account);}

SecurityTransaction::SecurityTransaction(Security *parent_security, double initial_value, double initial_shares, QDate initial_date, QString initial_comment) : Transaction(parent_security->budget(), initial_value, initial_date, NULL, NULL, QString::null, initial_comment), o_security(parent_security), d_shares(initial_shares), b_reconciled(false) {
}
SecurityTransaction::SecurityTransaction(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : Transaction(parent_budget, xml, valid) {}
SecurityTransaction::SecurityTransaction(Budget *parent_budget) : Transaction(parent_budget), b_reconciled(false) {}
SecurityTransaction::SecurityTransaction() : Transaction(), o_security(NULL), d_shares(0.0), b_reconciled(false) {}
SecurityTransaction::SecurityTransaction(const SecurityTransaction *transaction) : Transaction(transaction), o_security(transaction->security()), d_shares(transaction->shares()), b_reconciled(transaction->account() && transaction->account()->type() == ACCOUNT_TYPE_ASSETS ? transaction->isReconciled((AssetsAccount*) transaction->account()) : false) {}
SecurityTransaction::~SecurityTransaction() {}

void SecurityTransaction::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	Transaction::readAttributes(attr, valid);
	d_shares = attr->value("shares").toDouble();
	if(attr->hasAttribute("sharevalue")) {
		double v = attr->value("sharevalue").toDouble();
		if(d_shares <= 0.0 && v != 0.0) d_shares = d_value / v;
		else if(d_value == 0.0) d_value = d_shares * v;
	}
	b_reconciled = attr->value("reconciled").toInt();
	int id = attr->value("security").toInt();
	if(budget()->securities_id.contains(id)) {
		o_security = budget()->securities_id[id];
	} else {
		if(valid) *valid = false;
	}
}
void SecurityTransaction::writeAttributes(QXmlStreamAttributes *attr) {
	Transaction::writeAttributes(attr);
	attr->append("shares", QString::number(d_shares, 'f', o_security->decimals()));
	attr->append("security", QString::number(o_security->id()));
	if(b_reconciled) attr->append("reconciled", QString::number(b_reconciled));
}

bool SecurityTransaction::equals(const Transactions *transaction, bool strict_comparison) const {
	if(!Transaction::equals(transaction, strict_comparison)) return false;
	SecurityTransaction *sectrans = (SecurityTransaction*) transaction;
	if(d_shares != sectrans->shares()) return false;
	if(o_security != sectrans->security()) return false;
	return true;
}

double SecurityTransaction::shares() const {return d_shares;}
double SecurityTransaction::shareValue(bool convert) const {
	double v = 0.0;
	if(o_security) v = o_security->getQuotation(date());
	if(convert && o_security && o_security->currency()) {
		if(budget()->defaultTransactionConversionRateDate() == TRANSACTION_CONVERSION_RATE_AT_DATE) return o_security->currency()->convertTo(v, budget()->defaultCurrency(), date());
		else return o_security->currency()->convertTo(v, budget()->defaultCurrency());
	}
	return v;
}
void SecurityTransaction::setShares(double new_shares) {
	d_shares = new_shares;
}
double SecurityTransaction::value(bool convert) const {
	return Transaction::value(convert);
}
Currency *SecurityTransaction::currency() const {
	return account()->currency();
}
double SecurityTransaction::fromValue(bool convert) const {
	return Transaction::value(convert);
}
double SecurityTransaction::toValue(bool convert) const {
	return Transaction::value(convert);
}
Account *SecurityTransaction::fromAccount() const {return o_security->account();}
Account *SecurityTransaction::toAccount() const {return o_security->account();}
QString SecurityTransaction::description() const {return Transaction::description();}
void SecurityTransaction::setSecurity(Security *parent_security) {
	o_security = parent_security;
}
Security *SecurityTransaction::security() const {return o_security;}
bool SecurityTransaction::relatesToAccount(Account *account, bool, bool) const {return fromAccount() == account || toAccount() == account;}
double SecurityTransaction::accountChange(Account *account, bool, bool convert) const {
	if(fromAccount() == account) return -fromValue(convert);
	if(toAccount() == account) return toValue(convert);
	return 0.0;
}
bool SecurityTransaction::isReconciled(AssetsAccount *account_) const {
	if(account_ == account()) return b_reconciled;
	return false;
}
void SecurityTransaction::setReconciled(AssetsAccount *account_, bool is_reconciled) {
	if(account_ == account()) b_reconciled = is_reconciled;
}

SecurityBuy::SecurityBuy(Security *parent_security, double initial_value, double initial_shares, QDate initial_date, Account *from_account, QString initial_comment) : SecurityTransaction(parent_security, initial_value, initial_shares, initial_date, initial_comment) {
	setAccount(from_account);
}
SecurityBuy::SecurityBuy(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : SecurityTransaction(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
SecurityBuy::SecurityBuy(Budget *parent_budget) : SecurityTransaction(parent_budget) {}
SecurityBuy::SecurityBuy() : SecurityTransaction() {}
SecurityBuy::SecurityBuy(const SecurityBuy *transaction) : SecurityTransaction(transaction) {
	setAccount(transaction->account());
}
SecurityBuy::~SecurityBuy() {}
Transaction *SecurityBuy::copy() const {return new SecurityBuy(this);}

void SecurityBuy::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	d_value = attr->value("cost").toDouble();
	SecurityTransaction::readAttributes(attr, valid);
	int id_account = attr->value("account").toInt();
	if(budget()->assetsAccounts_id.contains(id_account)) {
		setAccount(budget()->assetsAccounts_id[id_account]);
	} else if(budget()->incomesAccounts_id.contains(id_account)) {
		setAccount(budget()->incomesAccounts_id[id_account]);
	} else {
		if(valid) *valid = false;
	}
}
void SecurityBuy::writeAttributes(QXmlStreamAttributes *attr) {
	SecurityTransaction::writeAttributes(attr);
	attr->append("cost", QString::number(d_value, 'f', SAVE_MONETARY_DECIMAL_PLACES));
	attr->append("account", QString::number(account()->id()));
}

double SecurityBuy::toValue(bool convert) const {
	return shareValue(convert) * d_shares;
}

QString SecurityBuy::description() const {return tr("Security: %1 (bought)", "Financial security (e.g. stock, mutual fund)").arg(o_security->name());}
Account *SecurityBuy::fromAccount() const {return Transaction::fromAccount();}
Account *SecurityBuy::account() const {return fromAccount();}
void SecurityBuy::setAccount(Account *new_account) {setFromAccount(new_account);}
TransactionType SecurityBuy::type() const {return TRANSACTION_TYPE_SECURITY_BUY;}

SecuritySell::SecuritySell(Security *parent_security, double initial_value, double initial_shares, QDate initial_date, Account *to_account, QString initial_comment) : SecurityTransaction(parent_security, initial_value, initial_shares, initial_date, initial_comment) {
	setAccount(to_account);
}
SecuritySell::SecuritySell(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : SecurityTransaction(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
SecuritySell::SecuritySell(Budget *parent_budget) : SecurityTransaction(parent_budget) {}
SecuritySell::SecuritySell() : SecurityTransaction() {}
SecuritySell::SecuritySell(const SecuritySell *transaction) : SecurityTransaction(transaction) {
	setAccount(transaction->account());
}
SecuritySell::~SecuritySell() {}
Transaction *SecuritySell::copy() const {return new SecuritySell(this);}

void SecuritySell::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	d_value = attr->value("income").toDouble();
	SecurityTransaction::readAttributes(attr, valid);
	int id_account = attr->value("account").toInt();	
	if(budget()->assetsAccounts_id.contains(id_account)) {
		setAccount(budget()->assetsAccounts_id[id_account]);
	} else if(budget()->expensesAccounts_id.contains(id_account)) {
		setAccount(budget()->expensesAccounts_id[id_account]);
	} else {
		if(valid) *valid = false;
	}
}
void SecuritySell::writeAttributes(QXmlStreamAttributes *attr) {
	SecurityTransaction::writeAttributes(attr);
	attr->append("income", QString::number(d_value, 'f', SAVE_MONETARY_DECIMAL_PLACES));
	attr->append("account", QString::number(account()->id()));
}

double SecuritySell::fromValue(bool convert) const {
	return shareValue(convert) * d_shares;
}

QString SecuritySell::description() const {return tr("Security: %1 (sold)", "Financial security (e.g. stock, mutual fund)").arg(o_security->name());}
Account *SecuritySell::toAccount() const {return Transaction::toAccount();}
Account *SecuritySell::account() const {return toAccount();}
void SecuritySell::setAccount(Account *new_account) {setToAccount(new_account);}
TransactionType SecuritySell::type() const {return TRANSACTION_TYPE_SECURITY_SELL;}

ScheduledTransaction::ScheduledTransaction(Budget *parent_budget) : o_budget(parent_budget) {
	o_rec = NULL;
}
ScheduledTransaction::ScheduledTransaction(Budget *parent_budget, Transactions *trans, Recurrence *rec) : o_budget(parent_budget) {
	o_rec = rec;
	o_trans = trans;
	if(o_trans && o_rec) o_trans->setDate(o_rec->startDate());
}
ScheduledTransaction::ScheduledTransaction(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : o_budget(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
ScheduledTransaction::ScheduledTransaction(const ScheduledTransaction *strans) : o_budget(strans->budget()), o_rec(NULL) {
	if(strans->recurrence()) o_rec = strans->recurrence()->copy();
	if(strans->transaction()) o_trans = strans->transaction()->copy();
}
ScheduledTransaction::~ScheduledTransaction() {
	if(o_rec) delete o_rec;
	if(o_trans) delete o_trans;
}
ScheduledTransaction *ScheduledTransaction::copy() const {return new ScheduledTransaction(this);}

void ScheduledTransaction::readAttributes(QXmlStreamAttributes*, bool*) {}
bool ScheduledTransaction::readElement(QXmlStreamReader *xml, bool*) {
	if(xml->name() == "recurrence") {
		QStringRef type = xml->attributes().value("type");
		bool valid2 = true;
		if(type == "daily") {
			if(o_rec) delete o_rec;
			o_rec = new DailyRecurrence(budget(), xml, &valid2);
		} else if(type == "weekly") {
			if(o_rec) delete o_rec;
			o_rec = new WeeklyRecurrence(budget(), xml, &valid2);
		} else if(type == "monthly") {
			if(o_rec) delete o_rec;
			o_rec = new MonthlyRecurrence(budget(), xml, &valid2);
		} else if(type == "yearly") {
			if(o_rec) delete o_rec;
			o_rec = new YearlyRecurrence(budget(), xml, &valid2);
		}
		if(!valid2) {
			delete o_rec;
			o_rec = NULL;
		}
		return true;
	} else if(xml->name() == "transaction") {
		QStringRef type = xml->attributes().value("type");
		bool valid2 = true;
		if(type == "expense") {
			if(o_trans) delete o_trans;
			o_trans = new Expense(budget(), xml, &valid2);
		} else if(type == "income") {
			if(o_trans) delete o_trans;
			o_trans = new Income(budget(), xml, &valid2);
		} else if(type == "dividend") {
			if(o_trans) delete o_trans;
			o_trans = new Income(budget(), xml, &valid2);
			if(!((Income*) o_trans)->security()) valid2 = false;
		} else if(type == "transfer") {
			if(o_trans) delete o_trans;
			o_trans = new Transfer(budget(), xml, &valid2);
		} else if(type == "balancing") {
			if(o_trans) delete o_trans;
			o_trans = new Balancing(budget(), xml, &valid2);
		} else if(type == "security_buy") {
			if(o_trans) delete o_trans;
			o_trans = new SecurityBuy(budget(), xml, &valid2);
		} else if(type == "security_sell") {
			if(o_trans) delete o_trans;
			o_trans = new SecuritySell(budget(), xml, &valid2);
		} else if(type == "multiitem") {
			if(o_trans) delete o_trans;
			o_trans = new MultiItemTransaction(budget(), xml, &valid2);
		} else if(type == "multiaccount") {
			if(o_trans) delete o_trans;
			o_trans = new MultiAccountTransaction(budget(), xml, &valid2);
		} else if(type == "debtpayment") {
			if(o_trans) delete o_trans;
			o_trans = new DebtPayment(budget(), xml, &valid2);
		}
		if(!valid2) {
			delete o_trans;
			o_trans = NULL;
		}
		return true;
	}
	return false;
}
bool ScheduledTransaction::readElements(QXmlStreamReader *xml, bool *valid) {
	o_rec = NULL;
	o_trans = NULL;
	while(xml->readNextStartElement()) {
		if(!readElement(xml, valid)) xml->skipCurrentElement();
	}	
	if(!o_trans && valid) *valid = false;
	if(o_rec && o_trans) {
		o_trans->setDate(o_rec->startDate());
	}
	return true;
}
void ScheduledTransaction::save(QXmlStreamWriter *xml) {
	QXmlStreamAttributes attr;
	writeAttributes(&attr);
	if(attr.isEmpty()) xml->writeAttributes(attr);
	writeElements(xml);
}
void ScheduledTransaction::writeAttributes(QXmlStreamAttributes*) {}
void ScheduledTransaction::writeElements(QXmlStreamWriter *xml) {
	if(!o_trans) return;
	if(o_trans->generaltype() == GENERAL_TRANSACTION_TYPE_SINGLE) {
		Transaction *trans = (Transaction*) o_trans;
		xml->writeStartElement("transaction");
		switch(trans->type()) {
			case TRANSACTION_TYPE_TRANSFER: {
				if(trans->fromAccount() == o_budget->balancingAccount) xml->writeAttribute("type", "balancing");
				else xml->writeAttribute("type", "transfer");
				break;
			}
			case TRANSACTION_TYPE_INCOME: {
				if(((Income*) trans)->security()) {
					xml->writeAttribute("type", "dividend");
				} else {
					xml->writeAttribute("type", "income");
				}
				break;
			}
			case TRANSACTION_TYPE_EXPENSE: {
				xml->writeAttribute("type", "expense");
				break;
			}
			case TRANSACTION_TYPE_SECURITY_BUY: {
				xml->writeAttribute("type", "security_buy");
				break;
			}
			case TRANSACTION_TYPE_SECURITY_SELL: {
				xml->writeAttribute("type", "security_sell");
				break;
			}
		}
		trans->save(xml);
		xml->writeEndElement();
	} else if(o_trans->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT) {
		SplitTransaction *o_split = (SplitTransaction*) o_trans;
		xml->writeStartElement("transaction");
		switch(o_split->type()) {
			case SPLIT_TRANSACTION_TYPE_MULTIPLE_ITEMS: {
				xml->writeAttribute("type", "multiitem");
				break;
			}
			case SPLIT_TRANSACTION_TYPE_MULTIPLE_ACCOUNTS: {
				xml->writeAttribute("type", "mulltiaccount");
				break;
			}
			case SPLIT_TRANSACTION_TYPE_LOAN: {
				xml->writeAttribute("type", "debtpayment");
				break;
			}
		}
		o_split->save(xml);
		xml->writeEndElement();
	}	
	if(o_rec) {
		xml->writeStartElement("recurrence");
		switch(o_rec->type()) {
			case RECURRENCE_TYPE_DAILY: {
				xml->writeAttribute("type", "daily");
				break;
			}
			case RECURRENCE_TYPE_WEEKLY: {
				xml->writeAttribute("type", "weekly");
				break;
			}
			case RECURRENCE_TYPE_MONTHLY: {
				xml->writeAttribute("type", "monthly");
				break;
			}
			case RECURRENCE_TYPE_YEARLY: {
				xml->writeAttribute("type", "yearly");
				break;
			}
		}
		o_rec->save(xml);
		xml->writeEndElement();
	}
}

bool ScheduledTransaction::equals(const Transactions *transaction, bool strict_comparison) const {
	if(transaction->generaltype() != GENERAL_TRANSACTION_TYPE_SCHEDULE) return false;
	ScheduledTransaction *strans = (ScheduledTransaction*) transaction;
	if(!o_trans || strict_comparison || !strans->transaction() || !o_trans->equals(strans->transaction(), strict_comparison)) return false;
	if(budget() != strans->budget()) return false;
	return true;
}

Recurrence *ScheduledTransaction::recurrence() const {
	return o_rec;
}
void ScheduledTransaction::setRecurrence(Recurrence *rec, bool delete_old) {
	if(o_rec && delete_old) delete o_rec;
	o_rec = rec;
	if(o_trans && o_rec) o_trans->setDate(o_rec->startDate());
	if(o_rec) {
		o_budget->scheduledTransactionSortModified(this);
		o_budget->scheduledTransactionDateModified(this);
	}
}
Budget *ScheduledTransaction::budget() const {return o_budget;}
const QDate &ScheduledTransaction::firstOccurrence() const {
	if(o_rec) return o_rec->firstOccurrence();
	if(o_trans) return o_trans->date();
	return emptydate;
}
bool ScheduledTransaction::isOneTimeTransaction() const {
	return !o_rec || o_rec->firstOccurrence() == o_rec->lastOccurrence();
}
const QDate &ScheduledTransaction::date() const {
	return firstOccurrence();
}
void ScheduledTransaction::setDate(QDate newdate) {
	if(o_rec) {
		o_rec->setStartDate(newdate);
		if(o_trans) {
			o_trans->setDate(o_rec->startDate());
		}
		o_budget->scheduledTransactionSortModified(this);
		o_budget->scheduledTransactionDateModified(this);
	} else if(o_trans && newdate != o_trans->date()) {
		o_trans->setDate(newdate);
		o_budget->scheduledTransactionSortModified(this);
		o_budget->scheduledTransactionDateModified(this);
	}
}
const qint64 &ScheduledTransaction::timestamp() const {
	if(o_trans) return o_trans->timestamp();
	return zero_timestamp;
}
void ScheduledTransaction::setTimestamp(qint64 cr_time) {
	if(!o_trans || cr_time == o_trans->timestamp()) return;
	o_budget->scheduledTransactionSortModified(this);
	o_trans->setTimestamp(cr_time);
}
void ScheduledTransaction::addException(QDate exceptiondate) {
	if(!o_rec) return;
	if(o_trans && exceptiondate == o_rec->startDate()) {
		o_rec->addException(exceptiondate);
		o_trans->setDate(o_rec->startDate());
		o_budget->scheduledTransactionSortModified(this);
		o_budget->scheduledTransactionDateModified(this);
	} else {
		o_rec->addException(exceptiondate);
	}
}
Transactions *ScheduledTransaction::realize(QDate date) {
	if(!o_trans) return NULL;
	if(o_rec && !o_rec->removeOccurrence(date)) return NULL;
	if(!o_rec && date != o_trans->date()) return NULL;
	Transactions *trans = o_trans->copy();
	trans->setTimestamp();
	if(o_rec) {
		o_trans->setDate(o_rec->startDate());
		o_budget->scheduledTransactionSortModified(this);
		o_budget->scheduledTransactionDateModified(this);
	}
	trans->setDate(date);
	return trans;
}
Transactions *ScheduledTransaction::transaction() const {
	return o_trans;
}
void ScheduledTransaction::setTransaction(Transactions *trans, bool delete_old) {
	if(o_trans && delete_old) delete o_trans;
	o_trans = trans;
	if(o_rec && o_trans) {
		o_trans->setDate(o_rec->startDate());
	} else if(o_trans) {
		o_budget->scheduledTransactionSortModified(this);
		o_budget->scheduledTransactionDateModified(this);
	}
}
double ScheduledTransaction::value(bool convert) const {
	if(!o_trans) return 0.0;
	return o_trans->value(convert);
}
Currency *ScheduledTransaction::currency() const {
	if(!o_trans) return NULL;
	return o_trans->currency();
}
double ScheduledTransaction::quantity() const {
	if(!o_trans) return 0.0;
	return o_trans->quantity();
}
QString ScheduledTransaction::description() const {
	if(o_trans) return o_trans->description();
	return QString();
}
const QString &ScheduledTransaction::comment() const {
	if(o_trans) return o_trans->comment();
	return emptystr;
}
const QString &ScheduledTransaction::associatedFile() const {
	if(o_trans) return o_trans->associatedFile();
	return emptystr;
}
void ScheduledTransaction::setAssociatedFile(QString new_attachment) {
	if(o_trans) o_trans->setAssociatedFile(new_attachment);
}
GeneralTransactionType ScheduledTransaction::generaltype() const {return GENERAL_TRANSACTION_TYPE_SCHEDULE;}
int ScheduledTransaction::transactiontype() const {
	if(o_trans && o_trans->generaltype() == GENERAL_TRANSACTION_TYPE_SINGLE) return ((Transaction*) o_trans)->type();
	return -1;
}
int ScheduledTransaction::transactionsubtype() const {
	if(o_trans && o_trans->generaltype() == GENERAL_TRANSACTION_TYPE_SINGLE) return ((Transaction*) o_trans)->subtype();
	return -1;
}
bool ScheduledTransaction::relatesToAccount(Account *account, bool include_subs, bool include_non_value) const {return o_trans && o_trans->relatesToAccount(account, include_subs, include_non_value);}
void ScheduledTransaction::replaceAccount(Account *old_account, Account *new_account) {if(o_trans) o_trans->replaceAccount(old_account, new_account);}
double ScheduledTransaction::accountChange(Account *account, bool include_subs, bool convert) const {
	if(o_trans) return o_trans->accountChange(account, include_subs, convert);
	return 0.0;
}
bool ScheduledTransaction::isReconciled(AssetsAccount*) const {return false;}
void ScheduledTransaction::setReconciled(AssetsAccount*, bool) {return;}


SplitTransaction::SplitTransaction(Budget *parent_budget, QDate initial_date, QString initial_description) : o_budget(parent_budget), d_date(initial_date), s_description(initial_description.trimmed()), i_time(QDateTime::currentMSecsSinceEpoch() * 1000), b_reconciled(false) {}
SplitTransaction::SplitTransaction(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : o_budget(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
SplitTransaction::SplitTransaction(const SplitTransaction *split) : o_budget(split->budget()), d_date(split->date()), s_description(split->description()), s_comment(split->comment()), s_file(split->associatedFile()), i_time(split->timestamp()), b_reconciled(false) {
	for(int i = 0; i < split->count(); i++) {
		Transaction *trans = split->at(i)->copy();
		trans->setParentSplit(this);
		splits.push_back(trans);
	}
}
SplitTransaction::SplitTransaction(Budget *parent_budget) : o_budget(parent_budget), i_time(QDateTime::currentMSecsSinceEpoch() * 1000), b_reconciled(false) {}
SplitTransaction::SplitTransaction() : o_budget(NULL), i_time(QDateTime::currentMSecsSinceEpoch() * 1000), b_reconciled(false) {}
SplitTransaction::~SplitTransaction() {
	clear();
}

void SplitTransaction::readAttributes(QXmlStreamAttributes *attr, bool*) {
	if(attr->hasAttribute("date")) d_date = QDate::fromString(attr->value("date").toString(), Qt::ISODate);
	i_time = attr->value("timestamp").toString().toLongLong();
	s_description = attr->value("description").trimmed().toString();
	s_comment = attr->value("comment").trimmed().toString();
	s_file = attr->value("file").trimmed().toString();
	b_reconciled = attr->value("reconciled").toInt();
}
bool SplitTransaction::readElement(QXmlStreamReader*, bool*) {
	return false;
}
bool SplitTransaction::readElements(QXmlStreamReader *xml, bool *valid) {
	while(xml->readNextStartElement()) {
		if(!readElement(xml, valid)) xml->skipCurrentElement();
	}
	return true;
}
void SplitTransaction::save(QXmlStreamWriter *xml) {
	QXmlStreamAttributes attr;
	writeAttributes(&attr);
	xml->writeAttributes(attr);
	writeElements(xml);
}
void SplitTransaction::writeAttributes(QXmlStreamAttributes *attr) {
	if(d_date.isValid()) attr->append("date", d_date.toString(Qt::ISODate));
	if(i_time != 0) attr->append("timestamp", QString::number(i_time));
	if(!s_description.isEmpty()) attr->append("description", s_description);
	if(!s_comment.isEmpty()) attr->append("comment", s_comment);
	if(!s_file.isEmpty()) attr->append("file", s_file);
	if(b_reconciled) attr->append("reconciled", QString::number(b_reconciled));
}

void SplitTransaction::writeElements(QXmlStreamWriter*) {}

bool SplitTransaction::equals(const Transactions *transaction, bool strict_comparison) const {
	if(transaction->generaltype() != GENERAL_TRANSACTION_TYPE_SPLIT) return false;
	SplitTransaction *split = (SplitTransaction*) transaction;
	if(split->type() != type()) return false;
	if(count() != split->count()) return false;
	for(int i = 0; i < count(); i++) {
		if(!at(i)->equals(split->at(i), strict_comparison)) return false;
	}	
	if(date() != split->date()) return false;
	if(description() != split->description()) return false;
	if(comment() != split->comment() && (strict_comparison || comment().isEmpty() == split->comment().isEmpty())) return false;
	if(associatedFile() != split->associatedFile() && (strict_comparison || associatedFile().isEmpty() == split->associatedFile().isEmpty())) return false;
	if(budget() != split->budget()) return false;
	return true;
}

double SplitTransaction::income() const {
	return -cost();
}
void SplitTransaction::addTransaction(Transaction *trans) {
	trans->setDate(d_date);
	splits.push_back(trans);
	trans->setParentSplit(this);
}
void SplitTransaction::removeTransaction(Transaction *trans, bool keep) {
	QVector<Transaction*>::iterator it_e = splits.end();
	for(QVector<Transaction*>::iterator it = splits.begin(); it != it_e; ++it) {
		if(*it == trans) {
			splits.erase(it);
			break;
		}
	}
	trans->setParentSplit(NULL);
	o_budget->removeTransaction(trans, keep);
}
void SplitTransaction::clear(bool keep) {
	QVector<Transaction*>::iterator it_e = splits.end();
	for(QVector<Transaction*>::iterator it = splits.begin(); it != it_e; ++it) {
		(*it)->setParentSplit(NULL);
		if(!keep) o_budget->removeTransaction(*it);
	}
	splits.clear();
}
const QDate &SplitTransaction::date() const {return d_date;}
void SplitTransaction::setDate(QDate new_date) {
	if(new_date != d_date) {
		QDate old_date = d_date; d_date = new_date; 
		o_budget->splitTransactionSortModified(this);
		o_budget->splitTransactionDateModified(this, old_date);
	}
	QVector<Transaction*>::size_type c = splits.count();
	for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
		splits[i]->setDate(d_date);
	}
}
const qint64 &SplitTransaction::timestamp() const {return i_time;}
void SplitTransaction::setTimestamp(qint64 cr_time) {
	if(i_time == cr_time) return;
	i_time = cr_time;
	o_budget->splitTransactionSortModified(this);
	QVector<Transaction*>::size_type c = splits.count();
	for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
		splits[i]->setTimestamp(i_time);
	}
}
QString SplitTransaction::description() const {return s_description;}
void SplitTransaction::setDescription(QString new_description) {s_description = new_description.trimmed();}
const QString &SplitTransaction::comment() const {return s_comment;}
void SplitTransaction::setComment(QString new_comment) {s_comment = new_comment;}
const QString &SplitTransaction::associatedFile() const {return s_file;}
void SplitTransaction::setAssociatedFile(QString new_attachment) {s_file = new_attachment;}
Budget *SplitTransaction::budget() const {return o_budget;}

int SplitTransaction::count() const {return splits.count();}
Transaction *SplitTransaction::operator[] (int index) const {return splits[index];}
Transaction *SplitTransaction::at(int index) const {return splits.at(index);}

GeneralTransactionType SplitTransaction::generaltype() const {return GENERAL_TRANSACTION_TYPE_SPLIT;}
bool SplitTransaction::isIncomesAndExpenses() const {
	for(int i = 0; i < splits.count(); i++) {
		if(splits[i]->type() != TRANSACTION_TYPE_EXPENSE && splits[i]->type() != TRANSACTION_TYPE_INCOME) return false;
	}
	return true;
}
bool SplitTransaction::relatesToAccount(Account *account, bool include_subs, bool include_non_value) const {
	int c = splits.count();
	for(int i = 0; i < c; i++) {
		if(splits[i]->relatesToAccount(account, include_subs, include_non_value)) return true;
	}
	return false;
}
void SplitTransaction::replaceAccount(Account *old_account, Account *new_account) {
	int c = splits.count();
	for(int i = 0; i < c; i++) {
		splits[i]->replaceAccount(old_account, new_account);
	}
}
double SplitTransaction::accountChange(Account *account, bool include_subs, bool convert) const {
	double v = 0.0;
	int c = splits.count();
	for(int i = 0; i < c; i++) {
		v += splits[i]->accountChange(account, include_subs, convert);
	}
	return v;
}
bool SplitTransaction::isReconciled(AssetsAccount*) const {
	return b_reconciled;
}
void SplitTransaction::setReconciled(AssetsAccount*, bool is_reconciled) {
	b_reconciled = is_reconciled;
}


MultiItemTransaction::MultiItemTransaction(Budget *parent_budget, QDate initial_date, AssetsAccount *initial_account, QString initial_description) : SplitTransaction(parent_budget, initial_date, initial_description), o_account(initial_account) {}
MultiItemTransaction::MultiItemTransaction(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : SplitTransaction(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
	if(valid && splits.count() == 0) *valid = false;
}
MultiItemTransaction::MultiItemTransaction(const MultiItemTransaction *split) : SplitTransaction(split), o_account(split->account()), s_payee(split->payee()) {
	b_reconciled = split->isReconciled(split->account());
}
MultiItemTransaction::MultiItemTransaction(Budget *parent_budget) : SplitTransaction(parent_budget), o_account(NULL) {}
MultiItemTransaction::MultiItemTransaction() : SplitTransaction(), o_account(NULL) {}
MultiItemTransaction::~MultiItemTransaction() {}
SplitTransaction *MultiItemTransaction::copy() const {return new MultiItemTransaction(this);}

void MultiItemTransaction::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	o_account = NULL;
	SplitTransaction::readAttributes(attr, valid);
	s_payee = attr->value("payee").trimmed().toString(); 
	int id = attr->value("account").toInt();
	if(d_date.isValid() && budget()->assetsAccounts_id.contains(id)) {
		o_account = budget()->assetsAccounts_id[id];
	} else {
		if(valid) *valid = false;
	}
}
bool MultiItemTransaction::readElement(QXmlStreamReader *xml, bool*) {
	if(!o_account) return false;
	if(xml->name() == "transaction") {
		QString id = QString::number(o_account->id());
		QXmlStreamAttributes attr = xml->attributes();
		QStringRef type = attr.value("type");
		attr.append("date", d_date.toString(Qt::ISODate));
		bool valid2 = true;
		bool is_dividend = false;
		Transaction *trans = NULL;
		if(type == "expense") {
			attr.append("from", id);
			if(!s_payee.isEmpty()) attr.append("payee", s_payee);
			trans = new Expense(budget());
		} else if(type == "income") {
			attr.append("to", id);
			if(!s_payee.isEmpty()) attr.append("payer", s_payee);
			trans = new Income(budget());
		} else if(type == "dividend") {
			attr.append("to", id);
			trans = new Income(budget());
			is_dividend = true;
		} else if(type == "balancing") {
			attr.append("account", id);
			trans = new Balancing(budget());
		} else if(type == "transfer") {
			if(attr.hasAttribute("to")) attr.append("from", id);
			else attr.append("to", id);
			trans = new Transfer(budget());
		} else if(type == "security_buy") {
			attr.append("account", id);
			trans = new SecurityBuy(budget());
		} else if(type == "security_sell") {
			attr.append("account", id);
			trans = new SecuritySell(budget());
		}
		if(trans) {
			trans->readAttributes(&attr, &valid2);
			trans->readElements(xml, &valid2);
			if(is_dividend && !((Income*) trans)->security()) valid2 = false;
			if(!valid2) {
				delete trans;
				trans = NULL;
			}
			if(trans) {
				trans->setParentSplit(this);
				if(trans->type() == TRANSACTION_TYPE_EXPENSE) {
					if(s_payee.isEmpty() && !((Expense*) trans)->payee().isEmpty()) s_payee = ((Expense*) trans)->payee();
				} else if(trans->type() == TRANSACTION_TYPE_INCOME) {
					if(s_payee.isEmpty() && !((Income*) trans)->payer().isEmpty()) s_payee = ((Income*) trans)->payer();
				}
				splits.push_back(trans);
			}
			return true;
		}
	}
	return false;
}
void MultiItemTransaction::writeAttributes(QXmlStreamAttributes *attr) {
	SplitTransaction::writeAttributes(attr);
	attr->append("account", QString::number(o_account->id()));
	if(!s_payee.isEmpty()) attr->append("payee", s_payee);
}
void remove_attributes(QXmlStreamAttributes *attr, QString s1, QString s2 = QString(), QString s3 = QString()) {
	bool b1 = false, b2 = false, b3 = false;
	if(s2.isEmpty()) b2 = true;
	if(s3.isEmpty()) b3 = true;
	for(int i = 0; i < attr->count(); i++) {
		if(!b1 && attr->at(i).name() == s1) {
			attr->remove(i);
			if(b2 && b3) break;
			b1 = true;
			i--;
		} else if(!b2 && attr->at(i).name() == s2) {
			attr->remove(i);
			if(b1 && b3) break;
			b2 = true;
			i--;
		} else if(!b3 && attr->at(i).name() == s3) {
			attr->remove(i);
			if(b1 && b2) break;
			b3 = true;
			i--;
		}
	}
}
void MultiItemTransaction::writeElements(QXmlStreamWriter *xml) {
	QVector<Transaction*>::iterator it_end = splits.end();
	for(QVector<Transaction*>::iterator it = splits.begin(); it != it_end; ++it) {
		xml->writeStartElement("transaction");
		Transaction *trans = *it;
		QXmlStreamAttributes attr;
		switch(trans->type()) {
			case TRANSACTION_TYPE_TRANSFER: {
				if(trans->fromAccount() == o_budget->balancingAccount) {
					attr.append("type", "balancing");
					trans->writeAttributes(&attr);
					remove_attributes(&attr, "date", "account");
				} else {
					attr.append("type", "transfer");
					trans->writeAttributes(&attr);
					if(((Transfer*) trans)->from() == o_account) {
						remove_attributes(&attr, "date", "from");
					} else {
						remove_attributes(&attr, "date", "to");
					}
				}
				break;
			}
			case TRANSACTION_TYPE_INCOME: {
				if(((Income*) trans)->security()) {
					attr.append("type", "dividend");
				} else {
					attr.append("type", "income");
				}
				trans->writeAttributes(&attr);
				remove_attributes(&attr, "date", "to", "payer");
				break;
			}
			case TRANSACTION_TYPE_EXPENSE: {
				attr.append("type", "expense");
				trans->writeAttributes(&attr);
				remove_attributes(&attr, "date", "from", "payee");
				break;
			}
			case TRANSACTION_TYPE_SECURITY_BUY: {
				attr.append("type", "security_buy");
				trans->writeAttributes(&attr);
				remove_attributes(&attr, "date", "account");
				break;
			}
			case TRANSACTION_TYPE_SECURITY_SELL: {
				attr.append("type", "security_sell");
				trans->writeAttributes(&attr);
				remove_attributes(&attr, "date", "account");
				break;
			}
			default: {}
		}
		xml->writeAttributes(attr);
		trans->writeElements(xml);
		xml->writeEndElement();
	}
}

double MultiItemTransaction::value(bool convert) const {
	double d_value = 0.0;
	QVector<Transaction*>::size_type c = splits.size();
	for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
		d_value += splits[i]->accountChange(o_account, false, convert);
	}
	return d_value;
}
Currency *MultiItemTransaction::currency() const {
	if(!o_account) return NULL;
	return o_account->currency();
}
double MultiItemTransaction::quantity() const {
	double d_quantity = 0.0;
	QVector<Transaction*>::size_type c = splits.size();
	for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
		d_quantity += splits[i]->quantity();
	}
	return d_quantity;
}
double MultiItemTransaction::cost(bool convert) const {
	double d_value = 0.0;
	QVector<Transaction*>::size_type c = splits.size();
	for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
		Transaction *trans = splits[i];
		if(trans->type() == TRANSACTION_TYPE_INCOME) d_value -= trans->value(convert);
		if(trans->type() == TRANSACTION_TYPE_EXPENSE) d_value += trans->value(convert);
	}
	return d_value;
}

void MultiItemTransaction::addTransaction(Transaction *trans) {
	trans->setDate(d_date);
	switch(trans->type()) {
		case TRANSACTION_TYPE_EXPENSE: {
			((Expense*) trans)->setFrom(o_account);
			((Expense*) trans)->setPayee(s_payee);
			break;
		}
		case TRANSACTION_TYPE_INCOME: {
			((Income*) trans)->setTo(o_account);
			((Income*) trans)->setPayer(s_payee);
			break;
		}
		case TRANSACTION_TYPE_TRANSFER: {
			if(!((Transfer*) trans)->from()) {
				((Transfer*) trans)->setFrom(o_account);
			} else {
				((Transfer*) trans)->setTo(o_account);
			}
			break;
		}
		case TRANSACTION_TYPE_SECURITY_BUY: {}
		case TRANSACTION_TYPE_SECURITY_SELL: {
			((SecurityTransaction*) trans)->setAccount(o_account);
			break;
		}
	}
	splits.push_back(trans);
	trans->setParentSplit(this);
}
AssetsAccount *MultiItemTransaction::account() const {return o_account;}
void MultiItemTransaction::setAccount(AssetsAccount *new_account) {
	QVector<Transaction*>::size_type c = splits.count();
	for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
		Transaction *trans = splits[i];
		switch(trans->type()) {
			case TRANSACTION_TYPE_EXPENSE: {
				((Expense*) trans)->setFrom(new_account);
				break;
			}
			case TRANSACTION_TYPE_INCOME: {
				((Income*) trans)->setTo(new_account);
				break;
			}
			case TRANSACTION_TYPE_TRANSFER: {
				if(((Transfer*) trans)->from() == o_account) {
					((Transfer*) trans)->setFrom(new_account);
				} else {
					((Transfer*) trans)->setTo(new_account);
				}
				break;
			}
			case TRANSACTION_TYPE_SECURITY_BUY: {}
			case TRANSACTION_TYPE_SECURITY_SELL: {
				((SecurityTransaction*) trans)->setAccount(new_account);
				break;
			}
		}
	}
	o_account = new_account;
}
const QString &MultiItemTransaction::payee() const {return s_payee;}
void MultiItemTransaction::setPayee(QString new_payee) {
	QVector<Transaction*>::size_type c = splits.count();
	for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
		Transaction *trans = splits[i];
		switch(trans->type()) {
			case TRANSACTION_TYPE_EXPENSE: {
				((Expense*) trans)->setPayee(new_payee);
				break;
			}
			case TRANSACTION_TYPE_INCOME: {
				((Income*) trans)->setPayer(new_payee);
				break;
			}
			default: {}
		}
	}
	s_payee = new_payee.trimmed();
}
QString MultiItemTransaction::fromAccountsString() const {
	QVector<Transaction*>::size_type c = splits.count();
	if(c == 0) return "";
	QList<Account*> account_list;
	for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
		if(splits[i]->fromAccount() == o_account && !account_list.contains(splits[i]->toAccount())) account_list << splits[i]->toAccount();
		if(splits[i]->toAccount() == o_account && !account_list.contains(splits[i]->fromAccount())) account_list << splits[i]->fromAccount();
	}
	if(account_list.count() == 1) return account_list.first()->name();
	QString account_string = account_list.first()->name();
	for(int i = 1; i < account_list.size(); i++) {
		account_string += " / ";
		account_string += account_list[i]->name();
	}
	return account_string;
}
int MultiItemTransaction::transactiontype() const {
	if(splits.count() == 0) return -1;
	TransactionType transtype = splits[0]->type();
	for(int i = 1; i < splits.count(); i++) {
		if(splits[i]->type() != transtype) return -1;
	}
	return transtype;
}
SplitTransactionType MultiItemTransaction::type() const {
	return SPLIT_TRANSACTION_TYPE_MULTIPLE_ITEMS;
}
bool MultiItemTransaction::relatesToAccount(Account *account, bool include_subs, bool include_non_value) const {
	return (include_non_value && o_account == account) || SplitTransaction::relatesToAccount(account, include_subs, include_non_value);
}
void MultiItemTransaction::replaceAccount(Account *old_account, Account *new_account) {
	if(o_account == old_account && new_account->type() == ACCOUNT_TYPE_ASSETS) o_account = (AssetsAccount*) new_account;
	SplitTransaction::replaceAccount(old_account, new_account);
}
bool MultiItemTransaction::isReconciled(AssetsAccount *account) const {
	if(account == o_account) return b_reconciled;
	return false;
}
void MultiItemTransaction::setReconciled(AssetsAccount *account, bool is_reconciled) {
	if(account == o_account) b_reconciled = is_reconciled;
}

MultiAccountTransaction::MultiAccountTransaction(Budget *parent_budget, CategoryAccount *initial_category, QString initial_description) : SplitTransaction(parent_budget, QDate(), initial_description), o_category(initial_category), d_quantity(1.0) {}
MultiAccountTransaction::MultiAccountTransaction(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : SplitTransaction(parent_budget), d_quantity(1.0) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
	QVector<Transaction*>::size_type c = splits.size();
	for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
		if(i == 0) d_date = splits[i]->date();
		else if(splits[i]->date() < d_date) d_date = splits[i]->date();
		splits[i]->setQuantity(d_quantity / c);
	}
	if(valid && splits.count() == 0) *valid = false;
}
MultiAccountTransaction::MultiAccountTransaction() : SplitTransaction(), o_category(NULL) {}
MultiAccountTransaction::MultiAccountTransaction(const MultiAccountTransaction *split) : SplitTransaction(split), o_category(split->category()), d_quantity(split->quantity()) {}
MultiAccountTransaction::MultiAccountTransaction(Budget *parent_budget) : SplitTransaction(parent_budget), o_category(NULL), d_quantity(1.0) {}
MultiAccountTransaction::~MultiAccountTransaction() {}
SplitTransaction *MultiAccountTransaction::copy() const {return new MultiAccountTransaction(this);}

void MultiAccountTransaction::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	o_category = NULL;
	SplitTransaction::readAttributes(attr, valid);
	int id = attr->value("category").toInt();
	if(budget()->expensesAccounts_id.contains(id)) {
		o_category = budget()->expensesAccounts_id[id];
	} else if(budget()->incomesAccounts_id.contains(id)) {
		o_category = budget()->incomesAccounts_id[id];
	} else {
		if(valid) *valid = false;
	}
	if(attr->hasAttribute("quantity")) d_quantity = attr->value("quantity").toDouble();
	else d_quantity = 1.0;
}
bool MultiAccountTransaction::readElement(QXmlStreamReader *xml, bool*) {
	if(!o_category) return false;
	if(xml->name() == "transaction") {
		QString id = QString::number(o_category->id());
		QXmlStreamAttributes attr = xml->attributes();
		QStringRef type = attr.value("type");
		bool valid2 = true;
		Transaction *trans = NULL;
		if(o_category->type() == ACCOUNT_TYPE_EXPENSES && type == "expense") {
			attr.append("category", id);
			attr.append("description", s_description);
			trans = new Expense(budget());
		} else if(o_category->type() == ACCOUNT_TYPE_INCOMES && type == "income") {
			attr.append("category", id);
			attr.append("description", s_description);
			trans = new Income(budget());
		}
		if(trans) {
			trans->readAttributes(&attr, &valid2);
			trans->readElements(xml, &valid2);
			if(!valid2) {
				delete trans;
				trans = NULL;
			}
			if(trans) {
				trans->setParentSplit(this);
				splits.push_back(trans);
			}
			return true;
		}
	}
	return false;
}
void MultiAccountTransaction::writeAttributes(QXmlStreamAttributes *attr) {
	if(!s_description.isEmpty()) attr->append("description", s_description);
	if(!s_comment.isEmpty())  attr->append("comment", s_comment);
	attr->append("category", QString::number(o_category->id()));
	if(d_quantity != 1.0) attr->append("quantity", QString::number(d_quantity, 'f', QUANTITY_DECIMAL_PLACES));
}
void MultiAccountTransaction::writeElements(QXmlStreamWriter *xml) {
	QVector<Transaction*>::iterator it_end = splits.end();
	for(QVector<Transaction*>::iterator it = splits.begin(); it != it_end; ++it) {
		xml->writeStartElement("transaction");
		Transaction *trans = *it;
		QXmlStreamAttributes attr;
		if(trans->type() == TRANSACTION_TYPE_INCOME) {
			attr.append("type", "income");
		} else {
			attr.append("type", "expense");
		}
		trans->writeAttributes(&attr);
		remove_attributes(&attr, "description", "category", "quantity");
		xml->writeAttributes(attr);
		trans->writeElements(xml);
		xml->writeEndElement();
	}
}
double MultiAccountTransaction::value(bool convert) const {
	double d_value = 0.0;
	Currency *cur = currency();
	QVector<Transaction*>::size_type c = splits.size();
	for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
		Transaction *trans = splits[i];
		if(!convert && cur != trans->currency()) {
			if(budget()->defaultTransactionConversionRateDate() == TRANSACTION_CONVERSION_RATE_AT_DATE) d_value += cur->convertFrom(trans->value(), trans->currency(), trans->date());
			else d_value += cur->convertFrom(trans->value(), trans->currency());
		} else {
			d_value += trans->value(convert);
		}
	}
	return d_value;
}
Currency *MultiAccountTransaction::currency() const {
	if(splits.isEmpty()) return budget()->defaultCurrency();
	Currency *cur = splits[0]->currency();
	if(cur != budget()->defaultCurrency()) {
		for(QVector<Transaction*>::size_type i = 1; i < splits.size(); i++) {
			if(splits[i]->currency() == budget()->defaultCurrency()) return budget()->defaultCurrency();
		}
	}
	return cur;
}
double MultiAccountTransaction::quantity() const {return d_quantity;}
void MultiAccountTransaction::setQuantity(double new_quantity) {
	d_quantity = new_quantity;
	QVector<Transaction*>::size_type c = splits.size();
	for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
		splits[i]->setQuantity(d_quantity / c);
	}
}
double MultiAccountTransaction::cost(bool convert) const {
	if(o_category->type() == ACCOUNT_TYPE_INCOMES) return -value(convert);
	return value(convert);
}
void MultiAccountTransaction::addTransaction(Transaction *trans) {
	if(o_category->type() == ACCOUNT_TYPE_EXPENSES && trans->type() == TRANSACTION_TYPE_EXPENSE) {
		if(!d_date.isValid() || trans->date() < d_date) d_date = trans->date();
		((Expense*) trans)->setCategory((ExpensesAccount*) o_category);
		splits.push_back(trans);
		trans->setParentSplit(this);
	} else if(o_category->type() == ACCOUNT_TYPE_INCOMES && trans->type() == TRANSACTION_TYPE_INCOME) {
		if(!d_date.isValid() || trans->date() < d_date) d_date = trans->date();
		((Income*) trans)->setCategory((IncomesAccount*) o_category);
		splits.push_back(trans);
		trans->setParentSplit(this);
	}
	trans->setDescription(s_description);
	QVector<Transaction*>::size_type c = splits.size();
	for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
		splits[i]->setQuantity(d_quantity / c);
	}
}
CategoryAccount *MultiAccountTransaction::category() const {return o_category;}
void MultiAccountTransaction::setCategory(CategoryAccount *new_category) {
	QVector<Transaction*>::size_type c = splits.count();
	for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
		Transaction *trans = splits[i];
		if(trans->type() == TRANSACTION_TYPE_EXPENSE) {
			((Expense*) trans)->setCategory((ExpensesAccount*) new_category);
		} else if(trans->type() == TRANSACTION_TYPE_INCOME) {
			((Income*) trans)->setCategory((IncomesAccount*) new_category);
		}
	}
	o_category = new_category;
}

AssetsAccount *MultiAccountTransaction::account() const {
	QVector<Transaction*>::size_type c = splits.count();
	if(c == 0) return NULL;	
	if(splits[0]->type() == TRANSACTION_TYPE_EXPENSE) {
		AssetsAccount *acc = (AssetsAccount*) splits[0]->fromAccount();
		for(QVector<Transaction*>::size_type i = 1; i < c; i++) {
			if(splits[i]->fromAccount() != acc) return NULL;
		}
		return acc;
	} else {
		AssetsAccount *acc = (AssetsAccount*) splits[0]->toAccount();
		for(QVector<Transaction*>::size_type i = 1; i < c; i++) {
			if(splits[i]->toAccount() != acc) return NULL;
		}
		return acc;
	}
}
QString MultiAccountTransaction::accountsString() const {
	QVector<Transaction*>::size_type c = splits.count();
	if(c == 0) return "";
	QList<Account*> account_list;
	if(splits[0]->type() == TRANSACTION_TYPE_EXPENSE) {
		for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
			if(!account_list.contains(splits[i]->fromAccount())) account_list << splits[i]->fromAccount();
		}
	} else {
		for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
			if(!account_list.contains(splits[i]->toAccount())) account_list << splits[i]->toAccount();
		}
	}
	if(account_list.count() == 1) return account_list.first()->name();
	QString account_string = account_list.first()->name();
	for(int i = 1; i < account_list.size(); i++) {
		account_string += " / ";
		account_string += account_list[i]->name();
	}
	return account_string;
}
QString MultiAccountTransaction::payees() const {
	QVector<Transaction*>::size_type c = splits.count();
	if(c == 0) return "";
	QStringList payee_list;
	if(splits[0]->type() == TRANSACTION_TYPE_EXPENSE) {
		for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
			if(!((Expense*) splits[i])->payee().isEmpty() && !payee_list.contains(((Expense*) splits[i])->payee())) payee_list << ((Expense*) splits[i])->payee();
		}
	} else {
		for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
			if(!((Income*) splits[i])->payer().isEmpty() && !payee_list.contains(((Income*) splits[i])->payer())) payee_list << ((Income*) splits[i])->payer();
		}
	}
	if(payee_list.isEmpty()) return "";
	if(payee_list.count() == 1) return payee_list.first();
	QString payee_string = payee_list.first();
	for(int i = 1; i < payee_list.count(); i++) {
		payee_string += " / ";
		payee_string += payee_list[i];
	}
	return payee_string;
}

void MultiAccountTransaction::setDescription(QString new_description) {
	QVector<Transaction*>::size_type c = splits.count();
	for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
		splits[i]->setDescription(new_description);
	}
	s_description = new_description;
}
SplitTransactionType MultiAccountTransaction::type() const {
	return SPLIT_TRANSACTION_TYPE_MULTIPLE_ACCOUNTS;
}
TransactionType MultiAccountTransaction::transactiontype() const {
	if(o_category->type() == ACCOUNT_TYPE_INCOMES) return TRANSACTION_TYPE_INCOME;
	return TRANSACTION_TYPE_EXPENSE;
}
bool MultiAccountTransaction::isIncomesAndExpenses() const {return true;}
bool MultiAccountTransaction::relatesToAccount(Account *account, bool include_subs, bool include_non_value) const {
	return (include_non_value && o_category && (o_category == account || (include_subs && o_category->topAccount() == account))) || SplitTransaction::relatesToAccount(account, include_subs, include_non_value);
}
void MultiAccountTransaction::replaceAccount(Account *old_account, Account *new_account) {
	if(o_category == old_account && (new_account->type() == ACCOUNT_TYPE_INCOMES || new_account->type() == ACCOUNT_TYPE_EXPENSES)) o_category = (CategoryAccount*) new_account;
	if(new_account->type() == ACCOUNT_TYPE_ASSETS && ((AssetsAccount*) new_account)->currency() != currency()) return;
	SplitTransaction::replaceAccount(old_account, new_account);
}


DebtPayment::DebtPayment(Budget *parent_budget, QDate initial_date, AssetsAccount *initial_loan, AssetsAccount *initial_account) : SplitTransaction(parent_budget, initial_date), o_loan(initial_loan), o_fee(NULL), o_interest(NULL), o_payment(NULL), o_account(initial_account) {}
DebtPayment::DebtPayment(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : SplitTransaction(parent_budget), o_loan(NULL), o_fee(NULL), o_interest(NULL), o_payment(NULL), o_account(NULL) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
DebtPayment::DebtPayment(const DebtPayment *split) : SplitTransaction(split), o_loan(split->loan()), o_fee(NULL), o_interest(NULL), o_payment(NULL), o_account(split->account()) {
	if(split->fee() != 0.0) setFee(split->fee());
	if(split->interest() != 0.0) setInterest(split->interest(), split->interestPayed());
	if(split->payment() != 0.0) setPayment(split->payment(), split->reduction());
	setExpenseCategory(split->expenseCategory());
	b_reconciled = split->isReconciled(split->account());
}
DebtPayment::DebtPayment(Budget *parent_budget) : SplitTransaction(parent_budget), o_loan(NULL), o_fee(NULL), o_interest(NULL), o_payment(NULL), o_account(NULL) {}
DebtPayment::DebtPayment() : SplitTransaction(), o_loan(NULL), o_fee(NULL), o_interest(NULL), o_payment(NULL), o_account(NULL) {}
DebtPayment::~DebtPayment() {}
SplitTransaction *DebtPayment::copy() const {return new DebtPayment(this);}

double DebtPayment::value(bool convert) const {return interest(convert) + fee(convert) + payment(convert);}
Currency *DebtPayment::currency() const {
	if(!o_loan) return NULL;
	return o_loan->currency();
}
double DebtPayment::quantity() const {
	return 1.0;
}
double DebtPayment::cost(bool convert) const {return interest(convert) + fee(convert);}
void DebtPayment::setInterest(double new_value, bool payed_from_account) {
	if(!o_interest) {
		if(new_value != 0.0) {
			o_interest = new DebtInterest(o_budget, new_value, d_date, o_fee ? o_fee->category() : NULL, payed_from_account ? o_account : o_loan, o_loan);
			o_interest->setParentSplit(this);
		}
	} else {
		o_interest->setValue(new_value);
	}
}
void DebtPayment::setInterestPayed(bool payed_from_account) {
	if(o_interest) {
		if(payed_from_account) o_interest->setFrom(o_account);
		else o_interest->setFrom(o_loan);
	}
}
void DebtPayment::setFee(double new_value) {
	if(!o_fee) {
		if(new_value != 0.0) {
			o_fee = new DebtFee(o_budget, new_value, d_date, o_interest ? o_interest->category() : NULL, o_account, o_loan);
			o_fee->setParentSplit(this);
		}
	} else {
		o_fee->setValue(new_value);
	}
}
void DebtPayment::setPayment(double new_value) {
	if(!o_payment) {
		if(new_value != 0.0) {
			o_payment = new DebtReduction(o_budget, new_value, d_date, o_account, o_loan);
			o_payment->setParentSplit(this);
		}
	} else {
		o_payment->setValue(new_value);
	}
}
void DebtPayment::setPayment(double new_payment, double new_reduction) {
	if(!o_payment) {
		if(new_payment != 0.0 || new_reduction != 0.0) {
			o_payment = new DebtReduction(o_budget, new_payment, new_reduction, d_date, o_account, o_loan);
			o_payment->setParentSplit(this);
		}
	} else {
		o_payment->setAmount(new_payment, new_reduction);
	}
}
double DebtPayment::interest(bool convert) const {
	if(o_interest) return o_interest->value(convert);
	return 0.0;
}
bool DebtPayment::interestPayed() const {
	return !o_interest || o_interest->from() != o_loan;
}
double DebtPayment::fee(bool convert) const {
	if(o_fee) return o_fee->value(convert);
	return 0.0;
}
double DebtPayment::payment(bool convert) const {
	if(o_payment) return o_payment->fromValue(convert);
	return 0.0;
}
double DebtPayment::reduction(bool convert) const {
	if(o_payment) return o_payment->toValue(convert);
	return 0.0;
}

DebtFee *DebtPayment::feeTransaction() const {return o_fee;}
DebtInterest *DebtPayment::interestTransaction() const {return o_interest;}
DebtReduction *DebtPayment::paymentTransaction() const {return o_payment;}

void DebtPayment::clear(bool keep) {
	if(o_fee) {
		o_fee->setParentSplit(NULL);
		if(!keep) o_budget->removeTransaction(o_fee);
		o_fee = NULL;
	}
	if(o_interest) {
		o_interest->setParentSplit(NULL);
		if(!keep) o_budget->removeTransaction(o_interest);
		o_interest = NULL;
	}
	if(o_payment) {
		o_payment->setParentSplit(NULL);
		if(!keep) o_budget->removeTransaction(o_payment);
		o_payment = NULL;
	}
}

void DebtPayment::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	SplitTransaction::readAttributes(attr, valid);
	o_loan = NULL;
	int loan_id = attr->value("debt").toInt();
	if(budget()->assetsAccounts_id.contains(loan_id)) {
		o_loan = budget()->assetsAccounts_id[loan_id];
	} else {
		if(valid) *valid = false;
		return;
	}
	if(attr->hasAttribute("from")) {
		int account_id = attr->value("from").toInt();
		if(budget()->assetsAccounts_id.contains(account_id)) {
			o_account = budget()->assetsAccounts_id[account_id];
		} else {
			if(valid) *valid = false;
			return;
		}
	} else {
		o_account = o_loan;
	}
	ExpensesAccount *cat = NULL;
	if(attr->hasAttribute("expensecategory")) {
		int category_id = attr->value("expensecategory").toInt();
		if(budget()->expensesAccounts_id.contains(category_id)) {
			cat = budget()->expensesAccounts_id[category_id];
		}
	}
	if(attr->hasAttribute("reduction")) {
		if(attr->hasAttribute("payment")) {
			o_payment = new DebtReduction(o_budget, attr->value("payment").toDouble(), attr->value("reduction").toDouble(), d_date, o_account, o_loan);
		} else {
			o_payment = new DebtReduction(o_budget, attr->value("reduction").toDouble(), d_date, o_account, o_loan);
		}
		o_payment->setParentSplit(this);
	}
	if(attr->hasAttribute("interest")) {
		bool interest_payed = true;
		if(attr->hasAttribute("interestpayed")) interest_payed = attr->value("interestpayed").toInt();
		o_interest = new DebtInterest(o_budget, attr->value("interest").toDouble(), d_date, cat, interest_payed ? o_account : o_loan, o_loan);
		o_interest->setParentSplit(this);
		if(valid && !cat) *valid = false;
	}
	if(attr->hasAttribute("fee")) {
		o_fee = new DebtFee(o_budget, attr->value("fee").toDouble(), d_date, cat, o_account, o_loan);
		o_fee->setParentSplit(this);
		if(valid && !cat) *valid = false;
	}
	if(valid && !o_fee && !o_interest && !o_payment) *valid = false;
}
bool DebtPayment::readElement(QXmlStreamReader*, bool*) {
	return false;
}
void DebtPayment::writeAttributes(QXmlStreamAttributes *attr) {
	SplitTransaction::writeAttributes(attr);
	attr->append("debt", QString::number(o_loan->id()));
	if(o_account && o_account != o_loan && (o_payment || o_fee || (o_interest && o_interest->from() != o_loan))) {
		attr->append("from", QString::number(o_account->id()));
		if(o_interest && o_interest->from() == o_loan) {
			attr->append("interestpayed", QString::number(false));
		}
	}
	if(expenseCategory()) attr->append("expensecategory", QString::number(expenseCategory()->id()));
	if(o_payment) {
		attr->append("reduction", QString::number(o_payment->toValue(), 'f', SAVE_MONETARY_DECIMAL_PLACES));
		if(o_payment->toValue() != o_payment->fromValue()) attr->append("payment", QString::number(o_payment->fromValue(), 'f', SAVE_MONETARY_DECIMAL_PLACES));
	}
	if(o_interest) attr->append("interest", QString::number(o_interest->value(), 'f', SAVE_MONETARY_DECIMAL_PLACES));
	if(o_fee) attr->append("fee", QString::number(o_fee->value(), 'f', SAVE_MONETARY_DECIMAL_PLACES));
}
void DebtPayment::writeElements(QXmlStreamWriter*) {}

AssetsAccount *DebtPayment::loan() const {return o_loan;}
void DebtPayment::setLoan(AssetsAccount *new_loan) {
	if(o_fee) o_fee->setLoan(new_loan);
	if(o_interest) o_interest->setLoan(new_loan);
	if(o_payment) o_payment->setLoan(new_loan);
	o_loan = new_loan;
}
ExpensesAccount *DebtPayment::expenseCategory() const {
	if(o_interest) return o_interest->category();
	else if(o_fee) return o_fee->category();
	return NULL;
}
void DebtPayment::setExpenseCategory(ExpensesAccount *new_category) {
	if(o_interest) o_interest->setCategory(new_category);
	if(o_fee) o_fee->setCategory(new_category);
}
AssetsAccount *DebtPayment::account() const {return o_account;}
void DebtPayment::setAccount(AssetsAccount *new_account) {
	if(o_fee) o_fee->setFrom(new_account);
	if(o_interest) o_interest->setFrom(new_account);
	if(o_payment) o_payment->setFrom(new_account);
	o_account = new_account;
}
void DebtPayment::setDate(QDate new_date) {
	if(new_date != d_date) {
		QDate old_date = d_date; d_date = new_date; 
		i_time = QDateTime::currentMSecsSinceEpoch() * 1000;
		o_budget->splitTransactionSortModified(this);
		o_budget->splitTransactionDateModified(this, old_date);
	}
	if(o_fee) o_fee->setDate(d_date);
	if(o_interest) o_interest->setDate(d_date);
	if(o_payment) o_payment->setDate(d_date);
}

QString DebtPayment::description() const {
	return tr("Debt payment: %1").arg(o_loan->name());
}
SplitTransactionType DebtPayment::type() const {
	return SPLIT_TRANSACTION_TYPE_LOAN;
}
bool DebtPayment::isIncomesAndExpenses() const {return true;}

int DebtPayment::count() const {
	int c = 0;
	if(o_fee) c++;
	if(o_interest) c++;
	if(o_payment) c++;
	return c;
}
Transaction *DebtPayment::operator[] (int index) const {
	return at(index);
}
Transaction *DebtPayment::at(int index) const {
	if(index == 0) {
		if(o_payment) return o_payment;
		if(o_interest) return o_interest;
		return o_fee;
	}
	if(index == 1) {
		if(o_payment) {
			if(o_interest) return o_interest;
			return o_fee;
		} 
		if(o_interest) return o_fee;
		return NULL;
	}
	if(index == 2 && o_payment && o_interest) return o_fee;
	return NULL;
}
void DebtPayment::removeTransaction(Transaction *trans, bool keep) {
	if(trans == o_interest) o_interest = NULL;
	else if(trans == o_fee) o_fee = NULL;
	else if(trans == o_payment) o_payment = NULL;
	trans->setParentSplit(NULL);
	o_budget->removeTransaction(trans, keep);
}
bool DebtPayment::relatesToAccount(Account *account, bool include_subs, bool include_non_value) const {
	return (include_non_value && (o_account == account || o_loan == account)) || (o_fee && o_fee->relatesToAccount(account, include_subs, include_non_value)) || (o_interest && o_interest->relatesToAccount(account, include_subs, include_non_value)) || (o_payment && o_payment->relatesToAccount(account, include_subs, include_non_value));
}
void DebtPayment::replaceAccount(Account *old_account, Account *new_account) {
	if(new_account->type() == ACCOUNT_TYPE_ASSETS && ((AssetsAccount*) new_account)->currency() != currency()) return;
	if(o_account == old_account && new_account->type() == ACCOUNT_TYPE_ASSETS) o_account = (AssetsAccount*) new_account;
	if(o_loan == old_account && new_account->type() == ACCOUNT_TYPE_ASSETS) o_loan = (AssetsAccount*) new_account;
	if(o_fee) o_fee->replaceAccount(old_account, new_account);
	if(o_interest) o_interest->replaceAccount(old_account, new_account);
	if(o_payment) o_payment->replaceAccount(old_account, new_account);
}
double DebtPayment::accountChange(Account *account, bool include_subs, bool convert) const {
	double v = 0.0;
	if(o_fee) v += o_fee->accountChange(account, include_subs, convert);
	if(o_interest) v += o_interest->accountChange(account, include_subs, convert);
	if(o_payment) v += o_payment->accountChange(account, include_subs, convert);
	return v;
}
bool DebtPayment::isReconciled(AssetsAccount *account) const {
	if(account == o_account) return b_reconciled;
	return false;
}
void DebtPayment::setReconciled(AssetsAccount *account, bool is_reconciled) {
	if(account == o_account) b_reconciled = is_reconciled;
}


