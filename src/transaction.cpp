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

#include "account.h"
#include "budget.h"
#include "recurrence.h"
#include "security.h"
#include "transaction.h"

Transaction::Transaction(Budget *parent_budget, double initial_value, QDate initial_date, Account *from, Account *to, QString initial_description, QString initial_comment) : o_budget(parent_budget), d_value(initial_value), d_date(initial_date), o_from(from), o_to(to), s_description(initial_description.trimmed()), s_comment(initial_comment), d_quantity(1.0), o_split(NULL) {
	if(s_description.isNull()) s_description = emptystr;
}
Transaction::Transaction(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : o_budget(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
Transaction::Transaction(Budget *parent_budget) : o_budget(parent_budget) {}
Transaction::Transaction() : o_budget(NULL), d_value(0.0), o_from(NULL), o_to(NULL), s_description(emptystr), d_quantity(1.0), o_split(NULL) {}
Transaction::Transaction(const Transaction *transaction) : o_budget(transaction->budget()), d_value(transaction->value()), d_date(transaction->date()), o_from(transaction->fromAccount()), o_to(transaction->toAccount()), s_description(transaction->description()), s_comment(transaction->comment()), d_quantity(transaction->quantity()), o_split(NULL) {}
Transaction::~Transaction() {}

void Transaction::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	o_split = NULL;
	o_from = NULL; o_to = NULL;
	d_date = QDate::fromString(attr->value("date").toString(), Qt::ISODate);
	if(attr->hasAttribute("description")) s_description = attr->value("description").trimmed().toString();
	else s_description = emptystr;
	if(attr->hasAttribute("comment")) s_comment = attr->value("comment").toString();
	else s_comment = emptystr;
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
	if(!s_comment.isEmpty())  attr->append("comment", s_comment);
	if(d_quantity != 1.0) attr->append("quantity", QString::number(d_quantity, 'f', MONETARY_DECIMAL_PLACES));
}
void Transaction::writeElements(QXmlStreamWriter*) {}

bool Transaction::equals(const Transaction *transaction) const {
	if(type() != transaction->type()) return false;
	if(fromAccount() != transaction->fromAccount()) return false;
	if(toAccount() != transaction->toAccount()) return false;
	if(value() != transaction->value()) return false;
	if(date() != transaction->date()) return false;
	if(quantity() != transaction->quantity()) return false;
	if(description() != transaction->description()) return false;
	if(comment() != transaction->comment() && comment().isEmpty() != transaction->comment().isEmpty()) return false;
	if(budget() != transaction->budget()) return false;
	return true;
}

SplitTransaction *Transaction::parentSplit() const {return o_split;}
void Transaction::setParentSplit(SplitTransaction *parent) {o_split = parent;}
double Transaction::value() const {return d_value;}
void Transaction::setValue(double new_value) {d_value = new_value;}
double Transaction::quantity() const {return d_quantity;}
void Transaction::setQuantity(double new_quantity) {d_quantity = new_quantity;}
const QDate &Transaction::date() const {return d_date;}
void Transaction::setDate(QDate new_date) {QDate old_date = d_date; d_date = new_date; o_budget->transactionDateModified(this, old_date);}
const QString &Transaction::description() const {return s_description;}
void Transaction::setDescription(QString new_description) {s_description = new_description.trimmed();}
const QString &Transaction::comment() const {return s_comment;}
void Transaction::setComment(QString new_comment) {s_comment = new_comment;}
Account *Transaction::fromAccount() const {return o_from;}
void Transaction::setFromAccount(Account *new_from) {o_from = new_from;}
Account *Transaction::toAccount() const {return o_to;}
void Transaction::setToAccount(Account *new_to) {o_to = new_to;}
Budget *Transaction::budget() const {return o_budget;}

Expense::Expense(Budget *parent_budget, double initial_cost, QDate initial_date, ExpensesAccount *initial_category, AssetsAccount *initial_from, QString initial_description, QString initial_comment) : Transaction(parent_budget, initial_cost, initial_date, initial_from, initial_category, initial_description, initial_comment), s_payee(emptystr) {
	if(s_payee.isNull()) s_payee = emptystr;
}
Expense::Expense(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : Transaction(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
Expense::Expense(Budget *parent_budget) : Transaction(parent_budget) {}
Expense::Expense() : Transaction(), s_payee(emptystr) {}
Expense::Expense(const Expense *expense) : Transaction(expense), s_payee(expense->payee()) {}
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
		if(attr->hasAttribute("payee")) s_payee = attr->value("payee").toString();
		else s_payee = emptystr;
	} else {
		if(valid) *valid =false;
	}
}
void Expense::writeAttributes(QXmlStreamAttributes *attr) {
	Transaction::writeAttributes(attr);
	if(cost() < 0.0) attr->append("income", QString::number(-cost(), 'f', MONETARY_DECIMAL_PLACES));
	else attr->append("cost", QString::number(cost(), 'f', MONETARY_DECIMAL_PLACES));
	attr->append("category", QString::number(category()->id()));
	attr->append("from", QString::number(from()->id()));
	if(!s_description.isEmpty()) attr->append("description", s_description);
	if(!s_payee.isEmpty()) attr->append("payee", s_payee);
}

bool Expense::equals(const Transaction *transaction) const {
	if(!Transaction::equals(transaction)) return false;
	Expense *expense = (Expense*) transaction;
	if(s_payee != expense->payee()) return false;
	return true;
}

ExpensesAccount *Expense::category() const {return (ExpensesAccount*) toAccount();}
void Expense::setCategory(ExpensesAccount *new_category) {setToAccount(new_category);}
AssetsAccount *Expense::from() const {return (AssetsAccount*) fromAccount();}
void Expense::setFrom(AssetsAccount *new_from) {setFromAccount(new_from);}
double Expense::cost() const {return value();}
void Expense::setCost(double new_cost) {setValue(new_cost);}
const QString &Expense::payee() const {return s_payee;}
void Expense::setPayee(QString new_payee) {s_payee = new_payee;}
TransactionType Expense::type() const {return TRANSACTION_TYPE_EXPENSE;}

Income::Income(Budget *parent_budget, double initial_income, QDate initial_date, IncomesAccount *initial_category, AssetsAccount *initial_to, QString initial_description, QString initial_comment) : Transaction(parent_budget, initial_income, initial_date, initial_category, initial_to, initial_description, initial_comment), o_security(NULL), s_payer(emptystr) {
	if(s_payer.isNull()) s_payer = emptystr;
}
Income::Income(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : Transaction(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
Income::Income(Budget *parent_budget) : Transaction(parent_budget) {}
Income::Income() : Transaction(), o_security(NULL), s_payer(emptystr) {}
Income::Income(const Income *income_) : Transaction(income_), o_security(income_->security()), s_payer(income_->payer()) {}
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
	if(o_security) {
		setDescription(tr("Dividend: %1").arg(o_security->name()));
		s_payer = o_security->name();
	} else {
		if(attr->hasAttribute("payer")) s_payer = attr->value("payer").toString();
		else s_payer = emptystr;
	}
}
void Income::writeAttributes(QXmlStreamAttributes *attr) {
	Transaction::writeAttributes(attr);
	if(income() < 0.0 && !o_security) attr->append("cost", QString::number(-income(), 'f', MONETARY_DECIMAL_PLACES));
	else attr->append("income", QString::number(income(), 'f', MONETARY_DECIMAL_PLACES));
	attr->append("category", QString::number(category()->id()));
	attr->append("to", QString::number(to()->id()));
	if(o_security) {
		attr->append("security", QString::number(o_security->id()));
	} else {
		if(!s_description.isEmpty()) attr->append("description", s_description);
		if(!s_payer.isEmpty()) attr->append("payer", s_payer);
	}
}

bool Income::equals(const Transaction *transaction) const {
	if(!Transaction::equals(transaction)) return false;
	Income *income = (Income*) transaction;
	if(s_payer != income->payer()) return false;
	if(o_security != income->security()) return false;
	return true;
}

IncomesAccount *Income::category() const {return (IncomesAccount*) fromAccount();}
void Income::setCategory(IncomesAccount *new_category) {setFromAccount(new_category);}
AssetsAccount *Income::to() const {return (AssetsAccount*) toAccount();}
void Income::setTo(AssetsAccount *new_to) {setToAccount(new_to);}
double Income::income() const {return value();}
void Income::setIncome(double new_income) {setValue(new_income);}
const QString &Income::payer() const {return s_payer;}
void Income::setPayer(QString new_payer) {s_payer = new_payer;}
TransactionType Income::type() const {return TRANSACTION_TYPE_INCOME;}
void Income::setSecurity(Security *parent_security) {
	o_security = parent_security;
	if(o_security) {
		setDescription(tr("Dividend: %1").arg(o_security->name()));
	}
}
Security *Income::security() const {return o_security;}

Transfer::Transfer(Budget *parent_budget, double initial_amount, QDate initial_date, AssetsAccount *initial_from, AssetsAccount *initial_to, QString initial_description, QString initial_comment) : Transaction(parent_budget, initial_amount < 0.0 ? -initial_amount : initial_amount, initial_date, initial_amount < 0.0 ? initial_to : initial_from, initial_amount < 0.0 ? initial_from : initial_to, initial_description, initial_comment) {}
Transfer::Transfer(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : Transaction(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
Transfer::Transfer(Budget *parent_budget) : Transaction(parent_budget) {}
Transfer::Transfer() : Transaction() {}
Transfer::Transfer(const Transfer *transfer) : Transaction(transfer) {}
Transfer::~Transfer() {}
Transaction *Transfer::copy() const {return new Transfer(this);}

void Transfer::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	Transaction::readAttributes(attr, valid);
	int id_from = attr->value("from").toInt();
	int id_to = attr->value("to").toInt();
	if(budget()->assetsAccounts_id.contains(id_from) && budget()->assetsAccounts_id.contains(id_to)) {
		setAmount(attr->value("amount").toDouble());
		if(amount() < 0.0) {
			setAmount(-amount());
			setFrom(budget()->assetsAccounts_id[id_to]);
			setTo(budget()->assetsAccounts_id[id_from]);
		} else {
			setFrom(budget()->assetsAccounts_id[id_from]);
			setTo(budget()->assetsAccounts_id[id_to]);
		}
	} else {
		if(valid) *valid =false;
	}
}
void Transfer::writeAttributes(QXmlStreamAttributes *attr) {
	Transaction::writeAttributes(attr);
	attr->append("amount", QString::number(amount(), 'f', MONETARY_DECIMAL_PLACES));
	attr->append("from", QString::number(from()->id()));
	attr->append("to", QString::number(to()->id()));
	if(!s_description.isEmpty()) attr->append("description", s_description);
}

AssetsAccount *Transfer::to() const {return (AssetsAccount*) toAccount();}
void Transfer::setTo(AssetsAccount *new_to) {setToAccount(new_to);}
AssetsAccount *Transfer::from() const {return (AssetsAccount*) fromAccount();}
void Transfer::setFrom(AssetsAccount *new_from) {setFromAccount(new_from);}
double Transfer::amount() const {return value();}
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
TransactionType Transfer::type() const {return TRANSACTION_TYPE_TRANSFER;}

Balancing::Balancing(Budget *parent_budget, double initial_amount, QDate initial_date, AssetsAccount *initial_account, QString initial_comment) : Transfer(parent_budget, initial_amount < 0.0 ? -initial_amount : initial_amount, initial_date, initial_amount < 0.0 ? initial_account : parent_budget->balancingAccount, initial_amount < 0.0 ? parent_budget->balancingAccount : initial_account, initial_comment) {
	setDescription(tr("Account balancing"));
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

void Balancing::readAttributes(QXmlStreamAttributes *attr, bool *valid) {	
	Transaction::readAttributes(attr, valid);
	if(s_description.isEmpty()) setDescription(tr("Account balancing"));
	d_value = attr->value("amount").toDouble();
	if(d_value < 0.0) {
		d_value = -d_value;
		setToAccount(budget()->balancingAccount);
		setFromAccount(NULL);
	} else {
		setFromAccount(budget()->balancingAccount);
		setToAccount(NULL);
	}
	int id_account = attr->value("account").toInt();
	if(budget()->assetsAccounts_id.contains(id_account)) {
		setAccount(budget()->assetsAccounts_id[id_account]);
	} else {
		if(valid) *valid = false;
	}
}
void Balancing::writeAttributes(QXmlStreamAttributes *attr) {
	Transaction::writeAttributes(attr);
	attr->append("amount", QString::number(toAccount() == o_budget->balancingAccount ? -amount() : amount(), 'f', MONETARY_DECIMAL_PLACES));
	attr->append("account", QString::number(account()->id()));
	if(s_description != tr("Account balancing")) attr->append("description", s_description);
}

AssetsAccount *Balancing::account() const {return toAccount() == o_budget->balancingAccount ? (AssetsAccount*) fromAccount() : (AssetsAccount*) toAccount();}
void Balancing::setAccount(AssetsAccount *new_account) {toAccount() == o_budget->balancingAccount ? setFromAccount(new_account) : setToAccount(new_account);}

SecurityTransaction::SecurityTransaction(Security *parent_security, double initial_value, double initial_shares, double initial_share_value, QDate initial_date, QString initial_comment) : Transaction(parent_security->budget(), initial_value, initial_date, NULL, NULL, QString::null, initial_comment), o_security(parent_security), d_shares(initial_shares), d_share_value(initial_share_value) {
}
SecurityTransaction::SecurityTransaction(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : Transaction(parent_budget, xml, valid) {}
SecurityTransaction::SecurityTransaction(Budget *parent_budget) : Transaction(parent_budget) {}
SecurityTransaction::SecurityTransaction() : Transaction(), o_security(NULL), d_shares(0.0), d_share_value(0.0) {}
SecurityTransaction::SecurityTransaction(const SecurityTransaction *transaction) : Transaction(transaction), o_security(transaction->security()), d_shares(transaction->shares()), d_share_value(transaction->shareValue()) {}
SecurityTransaction::~SecurityTransaction() {}

void SecurityTransaction::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	Transaction::readAttributes(attr, valid);
	d_shares = attr->value("shares").toDouble();
	if(attr->hasAttribute("sharevalue")) d_share_value = attr->value("sharevalue").toDouble();
	else d_share_value = -1.0;
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
	attr->append("sharevalue", QString::number(d_share_value, 'f', MONETARY_DECIMAL_PLACES));
	attr->append("security", QString::number(o_security->id()));
}

bool SecurityTransaction::equals(const Transaction *transaction) const {
	if(!Transaction::equals(transaction)) return false;
	SecurityTransaction *sectrans = (SecurityTransaction*) transaction;
	if(d_shares != sectrans->shares()) return false;
	if(d_share_value != sectrans->shareValue()) return false;
	if(o_security != sectrans->security()) return false;
	return true;
}

double SecurityTransaction::shares() const {return d_shares;}
double SecurityTransaction::shareValue() const {return d_share_value;}
void SecurityTransaction::setShares(double new_shares) {
	d_shares = new_shares;
}
void SecurityTransaction::setShareValue(double new_share_value) {
	d_share_value = new_share_value;
	if(o_security) o_security->setQuotation(d_date, d_share_value, true);
}
Account *SecurityTransaction::fromAccount() const {return o_security->account();}
Account *SecurityTransaction::toAccount() const {return o_security->account();}
void SecurityTransaction::setSecurity(Security *parent_security) {
	o_security = parent_security;
}
Security *SecurityTransaction::security() const {return o_security;}

SecurityBuy::SecurityBuy(Security *parent_security, double initial_value, double initial_shares, double initial_share_value, QDate initial_date, Account *from_account, QString initial_comment) : SecurityTransaction(parent_security, initial_value, initial_shares, initial_share_value, initial_date, initial_comment) {
	setAccount(from_account);
	if(o_security) {
		setDescription(tr("Security: %1 (bought)").arg(o_security->name()));
	}
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
	SecurityTransaction::readAttributes(attr, valid);
	int id_account = attr->value("account").toInt();
	d_value = attr->value("cost").toDouble();
	if(d_share_value < 0.0) d_share_value = d_value / d_shares;
	if(budget()->assetsAccounts_id.contains(id_account)) {
		setAccount(budget()->assetsAccounts_id[id_account]);
	} else if(budget()->incomesAccounts_id.contains(id_account)) {
		setAccount(budget()->incomesAccounts_id[id_account]);
	} else {
		if(valid) *valid = false;
	}
	if(o_security) {
		setDescription(tr("Security: %1 (bought)").arg(o_security->name()));
	}
}
void SecurityBuy::writeAttributes(QXmlStreamAttributes *attr) {
	SecurityTransaction::writeAttributes(attr);
	attr->append("cost", QString::number(d_value, 'f', MONETARY_DECIMAL_PLACES));
	attr->append("account", QString::number(account()->id()));
}

Account *SecurityBuy::fromAccount() const {return Transaction::fromAccount();}
Account *SecurityBuy::account() const {return fromAccount();}
void SecurityBuy::setAccount(Account *new_account) {setFromAccount(new_account);}
TransactionType SecurityBuy::type() const {return TRANSACTION_TYPE_SECURITY_BUY;}

SecuritySell::SecuritySell(Security *parent_security, double initial_value, double initial_shares, double initial_share_value, QDate initial_date, Account *to_account, QString initial_comment) : SecurityTransaction(parent_security, initial_value, initial_shares, initial_share_value, initial_date, initial_comment) {
	setAccount(to_account);
	if(o_security) {
		setDescription(tr("Security: %1 (sold)").arg(o_security->name()));
	}
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
	SecurityTransaction::readAttributes(attr, valid);
	int id_account = attr->value("account").toInt();
	d_value = attr->value("income").toDouble();
	if(d_share_value < 0.0) d_share_value = d_value / d_shares;
	if(budget()->assetsAccounts_id.contains(id_account)) {
		setAccount(budget()->assetsAccounts_id[id_account]);
	} else if(budget()->expensesAccounts_id.contains(id_account)) {
		setAccount(budget()->expensesAccounts_id[id_account]);
	} else {
		if(valid) *valid = false;
	}
	if(o_security) {
		setDescription(tr("Security: %1 (sold)").arg(o_security->name()));
	}
}
void SecuritySell::writeAttributes(QXmlStreamAttributes *attr) {
	SecurityTransaction::writeAttributes(attr);
	attr->append("income", QString::number(d_value, 'f', MONETARY_DECIMAL_PLACES));
	attr->append("account", QString::number(account()->id()));
}

Account *SecuritySell::toAccount() const {return Transaction::toAccount();}
Account *SecuritySell::account() const {return toAccount();}
void SecuritySell::setAccount(Account *new_account) {setToAccount(new_account);}
TransactionType SecuritySell::type() const {return TRANSACTION_TYPE_SECURITY_SELL;}

ScheduledTransaction::ScheduledTransaction(Budget *parent_budget) : o_budget(parent_budget) {
	o_trans = NULL;
	o_rec = NULL;
}
ScheduledTransaction::ScheduledTransaction(Budget *parent_budget, Transaction *trans, Recurrence *rec) : o_budget(parent_budget) {
	o_trans = trans;
	o_rec = rec;
	if(o_trans && o_rec) o_trans->setDate(o_rec->startDate());
}
ScheduledTransaction::ScheduledTransaction(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : o_budget(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
ScheduledTransaction::ScheduledTransaction(const ScheduledTransaction *strans) : o_budget(strans->budget()), o_trans(NULL), o_rec(NULL) {
	if(strans->transaction()) o_trans = strans->transaction()->copy();
	if(strans->recurrence()) o_rec = strans->recurrence()->copy();
}
ScheduledTransaction::~ScheduledTransaction() {
	if(o_trans) delete o_trans;
	if(o_rec) delete o_rec;
}
ScheduledTransaction *ScheduledTransaction::copy() const {return new ScheduledTransaction(this);}

void ScheduledTransaction::readAttributes(QXmlStreamAttributes*, bool*) {}

bool ScheduledTransaction::readElement(QXmlStreamReader *xml, bool*) {
	if(xml->name() == "transaction") {
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
		}
		if(!valid2) {
			delete o_trans;
			o_trans = NULL;
		}
		return true;
	} else if(xml->name() == "recurrence") {
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
	}
	return false;
}
bool ScheduledTransaction::readElements(QXmlStreamReader *xml, bool *valid) {
	o_rec = NULL; o_trans = NULL;
	while(xml->readNextStartElement()) {
		if(!readElement(xml, valid)) xml->skipCurrentElement();
	}
	if(!o_trans && valid) *valid = false;
	if(o_rec && o_trans) o_trans->setDate(o_rec->startDate());
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
	xml->writeStartElement("transaction");
	switch(o_trans->type()) {
		case TRANSACTION_TYPE_TRANSFER: {
			if(o_trans->fromAccount() == o_budget->balancingAccount) xml->writeAttribute("type", "balancing");
			else xml->writeAttribute("type", "transfer");
			break;
		}
		case TRANSACTION_TYPE_INCOME: {
			if(((Income*) o_trans)->security()) {
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
	o_trans->save(xml);
	xml->writeEndElement();
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

Transaction *ScheduledTransaction::realize(const QDate &date) {
	if(!o_trans) return NULL;
	if(o_rec && !o_rec->removeOccurrence(date)) return NULL;
	if(!o_rec && date != o_trans->date()) return NULL;
	Transaction *trans = o_trans->copy();
	if(o_rec) {
		o_trans->setDate(o_rec->startDate());
		o_budget->scheduledTransactionDateModified(this);
	}
	trans->setDate(date);
	return trans;
}
Transaction *ScheduledTransaction::transaction() const {
	return o_trans;
}
Recurrence *ScheduledTransaction::recurrence() const {
	return o_rec;
}
void ScheduledTransaction::setRecurrence(Recurrence *rec, bool delete_old) {
	if(o_rec && delete_old) delete o_rec;
	o_rec = rec;
	if(o_trans && o_rec) o_trans->setDate(o_rec->startDate());
	if(o_rec) o_budget->scheduledTransactionDateModified(this);
}
void ScheduledTransaction::setTransaction(Transaction *trans, bool delete_old) {
	if(o_trans && delete_old) delete o_trans;
	o_trans = trans;
	if(o_rec && o_trans) o_trans->setDate(o_rec->startDate());
	else if(o_trans) o_budget->scheduledTransactionDateModified(this);
}
Budget *ScheduledTransaction::budget() const {return o_budget;}
QDate ScheduledTransaction::firstOccurrence() const {
	if(o_rec) return o_rec->firstOccurrence();
	if(o_trans) return o_trans->date();
	return QDate();
}
bool ScheduledTransaction::isOneTimeTransaction() const {
	return !o_rec || o_rec->firstOccurrence() == o_rec->lastOccurrence();
}
void ScheduledTransaction::setDate(const QDate &newdate) {
	if(o_rec) {
		o_rec->setStartDate(newdate);
		if(o_trans) o_trans->setDate(o_rec->startDate());
		o_budget->scheduledTransactionDateModified(this);
	} else if(o_trans && newdate != o_trans->date()) {
		o_trans->setDate(newdate);
		o_budget->scheduledTransactionDateModified(this);
	}
}
void ScheduledTransaction::addException(const QDate &exceptiondate) {
	if(!o_rec) return;
	if(o_trans && exceptiondate == o_rec->startDate()) {
		o_rec->addException(exceptiondate);
		o_trans->setDate(o_rec->startDate());
		o_budget->scheduledTransactionDateModified(this);
	} else {
		o_rec->addException(exceptiondate);
	}
}

SplitTransaction::SplitTransaction(Budget *parent_budget, QDate initial_date, AssetsAccount *initial_account, QString initial_description) : o_budget(parent_budget), d_date(initial_date), o_account(initial_account), s_description(initial_description.trimmed()) {}
SplitTransaction::SplitTransaction(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : o_budget(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
SplitTransaction::SplitTransaction() : o_budget(NULL), o_account(NULL) {}
SplitTransaction::~SplitTransaction() {
	clear();
}

void SplitTransaction::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	o_account = NULL;
	d_date = QDate::fromString(attr->value("date").toString(), Qt::ISODate);
	if(attr->hasAttribute("description")) s_description = attr->value("description").trimmed().toString();
	else s_description = emptystr;
	if(attr->hasAttribute("comment")) s_comment = attr->value("comment").toString();
	else s_comment = emptystr;
	int id = attr->value("account").toInt();
	if(d_date.isValid() && budget()->assetsAccounts_id.contains(id)) {
		o_account = budget()->assetsAccounts_id[id];
	} else {
		if(valid) *valid = false;
	}
}
bool SplitTransaction::readElement(QXmlStreamReader *xml, bool*) {
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
			trans = new Expense(budget());			
		} else if(type == "income") {
			attr.append("to", id);
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
				splits.push_back(trans);
			}
			return true;
		}
	}
	return false;
}
bool SplitTransaction::readElements(QXmlStreamReader *xml, bool *valid) {
	if(!o_account) return false;
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
	attr->append("date", d_date.toString(Qt::ISODate));
	attr->append("account", QString::number(o_account->id()));
	if(!s_description.isEmpty()) attr->append("description", s_description);
	if(!s_comment.isEmpty())  attr->append("comment", s_comment);
	
}
void remove_attributes(QXmlStreamAttributes *attr, QString s1, QString s2) {
	bool b1 = false, b2 = false;
	for(int i = 0; i < attr->count(); i++) {
		if(!b1 && attr->at(i).name() == s1) {
			attr->remove(i);
			if(b2) break;
			b1 = true;
			i--;
		} else if(!b2 && attr->at(i).name() == s2) {
			attr->remove(i);
			if(b1) break;
			b2 = true;
			i--;
		}
	}
}
void SplitTransaction::writeElements(QXmlStreamWriter *xml) {
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
				remove_attributes(&attr, "date", "to");
				break;
			}
			case TRANSACTION_TYPE_EXPENSE: {
				attr.append("type", "expense");
				trans->writeAttributes(&attr);
				remove_attributes(&attr, "date", "from");
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

double SplitTransaction::value() const {
	double d_value = 0.0;
	QVector<Transaction*>::size_type c = splits.size();
	for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
		Transaction *trans = splits[i];
		if(trans->fromAccount() == o_account) d_value -= trans->value();
		if(trans->toAccount() == o_account) d_value += trans->value();
	}
	return d_value;
}
void SplitTransaction::addTransaction(Transaction *trans) {
	trans->setDate(d_date);
	switch(trans->type()) {
		case TRANSACTION_TYPE_EXPENSE: {
			((Expense*) trans)->setFrom(o_account);
			break;
		}
		case TRANSACTION_TYPE_INCOME: {
			((Income*) trans)->setTo(o_account);
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
	QDate old_date = d_date; d_date = new_date; o_budget->splitTransactionDateModified(this, old_date);
	QVector<Transaction*>::size_type c = splits.count();
	for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
		splits[i]->setDate(d_date);
	}
}
const QString &SplitTransaction::description() const {return s_description;}
void SplitTransaction::setDescription(QString new_description) {s_description = new_description.trimmed();}
const QString &SplitTransaction::comment() const {return s_comment;}
void SplitTransaction::setComment(QString new_comment) {s_comment = new_comment;}
AssetsAccount *SplitTransaction::account() const {return o_account;}
void SplitTransaction::setAccount(AssetsAccount *new_account) {
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
Budget *SplitTransaction::budget() const {return o_budget;}

