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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <QDomElement>
#include <QDomNode>

#include "account.h"
#include "budget.h"



Account::Account(Budget *parent_budget, QString initial_name, QString initial_description) : o_budget(parent_budget), i_id(-1), s_name(initial_name.trimmed()), s_description(initial_description) {}
Account::Account(Budget *parent_budget, QDomElement *e, bool *valid) : o_budget(parent_budget) {
	i_id = e->attribute("id").toInt();
	s_name = e->attribute("name").trimmed();
	s_description = e->attribute("description");
	if(valid) *valid = true;
}
Account::Account() : o_budget(NULL), i_id(-1) {}
Account::Account(const Account *account) : o_budget(account->budget()), i_id(account->id()), s_name(account->name()), s_description(account->description()) {}
Account::~Account() {}

const QString &Account::name() const {return s_name;}
void Account::setName(QString new_name) {s_name = new_name.trimmed(); o_budget->accountNameModified(this);}
const QString &Account::description() const {return s_description;}
void Account::setDescription(QString new_description) {s_description = new_description;}
Budget *Account::budget() const {return o_budget;}
int Account::id() const {return i_id;}
void Account::setId(int new_id) {i_id = new_id;}
void Account::save(QDomElement *e) const {
	e->setAttribute("name", s_name);
	e->setAttribute("id", i_id);
	if(!s_description.isEmpty()) e->setAttribute("description", s_description);
}

AssetsAccount::AssetsAccount(Budget *parent_budget, AssetsType initial_type, QString initial_name, double initial_balance, QString initial_description) : Account(parent_budget, initial_name, initial_description), at_type(initial_type), d_initbal(initial_type == ASSETS_TYPE_SECURITIES ? 0.0 : initial_balance) {}
AssetsAccount::AssetsAccount(Budget *parent_budget, QDomElement *e, bool *valid) : Account(parent_budget, e, valid) {
	QString type = e->attribute("type");
	if(type == "current") {
		at_type = ASSETS_TYPE_CURRENT;
	} else if(type == "savings") {
		at_type = ASSETS_TYPE_SAVINGS;
	} else if(type == "credit card") {
		at_type = ASSETS_TYPE_CREDIT_CARD;
	} else if(type == "liabilities") {
		at_type = ASSETS_TYPE_LIABILITIES;
	} else if(type == "securities") {
		at_type = ASSETS_TYPE_SECURITIES;
	} else if(type == "balancing") {
		at_type = ASSETS_TYPE_BALANCING;
	} else {
		at_type = ASSETS_TYPE_CASH;
	}
	if(at_type != ASSETS_TYPE_SECURITIES) {
		d_initbal = e->attribute("initialbalance").toDouble();
		if(e->hasAttribute("budgetaccount")) {
			bool b_budget = e->attribute("budgetaccount").toInt();
			if(b_budget) {
				o_budget->budgetAccount = this;
			}
		} else if(at_type == ASSETS_TYPE_CURRENT && !o_budget->budgetAccount) {
			o_budget->budgetAccount = this;
		}
	}
}
AssetsAccount::AssetsAccount() : Account(), at_type(ASSETS_TYPE_CASH), d_initbal(0.0) {}
AssetsAccount::AssetsAccount(const AssetsAccount *account) : Account(account), at_type(account->accountType()), d_initbal(account->initialBalance()) {}
AssetsAccount::~AssetsAccount() {if(o_budget->budgetAccount == this) o_budget->budgetAccount = NULL;}

bool AssetsAccount::isBudgetAccount() const {
	return o_budget->budgetAccount == this;
}
void AssetsAccount::setAsBudgetAccount(bool will_be) {
	if(will_be) {
		o_budget->budgetAccount = this;
	} else if(o_budget->budgetAccount == this) {
		o_budget->budgetAccount = NULL;
	}
}
double AssetsAccount::initialBalance() const {
	if(at_type == ASSETS_TYPE_SECURITIES) {
		double d = 0.0;
		Security *sec = o_budget->securities.first();
		while(sec) {
			if(sec->account() == this) {
				d += sec->initialBalance();
			}
			sec = o_budget->securities.next();
		}
		return d;
	}
	return d_initbal;
}
void AssetsAccount::setInitialBalance(double new_initial_balance) {if(at_type != ASSETS_TYPE_SECURITIES) d_initbal = new_initial_balance;}
AccountType AssetsAccount::type() const {return ACCOUNT_TYPE_ASSETS;}
void AssetsAccount::save(QDomElement *e) const {
	Account::save(e);
	if(at_type != ASSETS_TYPE_SECURITIES) {
		e->setAttribute("initialbalance", QString::number(d_initbal, 'f', MONETARY_DECIMAL_PLACES));
		if(at_type == ASSETS_TYPE_CURRENT || o_budget->budgetAccount == this) {
			e->setAttribute("budgetaccount", o_budget->budgetAccount == this);
		}
	}
	switch(at_type) {
		case ASSETS_TYPE_CURRENT: {e->setAttribute("type", "current"); break;}
		case ASSETS_TYPE_SAVINGS: {e->setAttribute("type", "savings"); break;}
		case ASSETS_TYPE_CREDIT_CARD: {e->setAttribute("type", "credit card"); break;}
		case ASSETS_TYPE_LIABILITIES: {e->setAttribute("type", "liabilities"); break;}
		case ASSETS_TYPE_SECURITIES: {e->setAttribute("type", "securities"); break;}
		case ASSETS_TYPE_BALANCING: {e->setAttribute("type", "balancing"); break;}
		case ASSETS_TYPE_CASH: {e->setAttribute("type", "cash"); break;}
	}
}
void AssetsAccount::setAccountType(AssetsType new_type) {
	at_type = new_type;
	if(at_type == ASSETS_TYPE_SECURITIES) d_initbal = 0.0;
}
AssetsType AssetsAccount::accountType() const {return at_type;}

CategoryAccount::CategoryAccount(Budget *parent_budget, QString initial_name, QString initial_description) : Account(parent_budget, initial_name, initial_description) {}
CategoryAccount::CategoryAccount(Budget *parent_budget, QDomElement *e, bool *valid) : Account(parent_budget, e, valid) {
	if(e->hasAttribute("monthlybudget")) {
		double d_mbudget = e->attribute("monthlybudget").toDouble();
		if(d_mbudget >= 0.0) {			
			QDate date = QDate::currentDate();
			date.setDate(date.year(), date.month(), 1);
			mbudgets[date] = d_mbudget;
		}
	}
	for(QDomNode n = e->firstChild(); !n.isNull(); n = n.nextSibling()) {
		if(n.isElement()) {
			QDomElement e2 = n.toElement();
			if(e2.tagName() == "budget") {
				QDate date = QDate::fromString(e2.attribute("date"), Qt::ISODate);
				mbudgets[date] = e2.attribute("value").toDouble();
			}
		}
	}
}
CategoryAccount::CategoryAccount() : Account() {}
CategoryAccount::CategoryAccount(const CategoryAccount *account) : Account(account) {}
CategoryAccount::~CategoryAccount() {}

double CategoryAccount::monthlyBudget(int year, int month, bool no_default) const {	
	QDate date;
	date.setDate(year, month, 1);
	return monthlyBudget(date, no_default);
}
void CategoryAccount::setMonthlyBudget(int year, int month, double new_monthly_budget) {	
	QDate date;
	date.setDate(year, month, 1);
	return setMonthlyBudget(date, new_monthly_budget);
}
double CategoryAccount::monthlyBudget(const QDate &date, bool no_default) const {
	if(mbudgets.isEmpty()) return -1.0;
	QMap<QDate, double>::const_iterator it = mbudgets.find(date);
	if(it != mbudgets.end()) {
		return it.value();
	}
	if(no_default) return -1.0;	
	it = mbudgets.begin();
	if(it.key() > date) return -1.0;
	QMap<QDate, double>::const_iterator it_e = mbudgets.end();
	--it_e;
	while(it_e != it) {
		if(date > it_e.key()) return it_e.value();
		--it_e;
	}
	return it.value();
}
void CategoryAccount::setMonthlyBudget(const QDate &date, double new_monthly_budget) {
	mbudgets[date] = new_monthly_budget;
}
void CategoryAccount::save(QDomElement *e) const {
	Account::save(e);
	QMap<QDate, double>::const_iterator it_end = mbudgets.end();
	for(QMap<QDate, double>::const_iterator it = mbudgets.begin(); it != it_end; ++it) {
		QDomElement e2 = e->ownerDocument().createElement("budget");
		e2.setAttribute("value", QString::number(it.value(), 'f', MONETARY_DECIMAL_PLACES));
		e2.setAttribute("date", it.key().toString(Qt::ISODate));
		e->appendChild(e2);
	}
}

IncomesAccount::IncomesAccount(Budget *parent_budget, QString initial_name, QString initial_description) : CategoryAccount(parent_budget, initial_name, initial_description) {}
IncomesAccount::IncomesAccount(Budget *parent_budget, QDomElement *e, bool *valid) : CategoryAccount(parent_budget, e, valid) {}
IncomesAccount::IncomesAccount() : CategoryAccount() {}
IncomesAccount::IncomesAccount(const IncomesAccount *account) : CategoryAccount(account) {}
IncomesAccount::~IncomesAccount() {}

AccountType IncomesAccount::type() const {return ACCOUNT_TYPE_INCOMES;}

ExpensesAccount::ExpensesAccount(Budget *parent_budget, QString initial_name, QString initial_description) : CategoryAccount(parent_budget, initial_name, initial_description) {}
ExpensesAccount::ExpensesAccount(Budget *parent_budget, QDomElement *e, bool *valid) : CategoryAccount(parent_budget, e, valid) {}
ExpensesAccount::ExpensesAccount() : CategoryAccount() {}
ExpensesAccount::ExpensesAccount(const ExpensesAccount *account) : CategoryAccount(account) {}
ExpensesAccount::~ExpensesAccount() {}

AccountType ExpensesAccount::type() const {return ACCOUNT_TYPE_EXPENSES;}
