/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016-2017 by Hanna Knutsson            *
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

#include "account.h"
#include "budget.h"

Account::Account(Budget *parent_budget, QString initial_name, QString initial_description) : o_budget(parent_budget), i_id(parent_budget->getNewId()), i_first_revision(parent_budget->revision()), i_last_revision(parent_budget->revision()), s_name(initial_name.trimmed()), s_description(initial_description.trimmed()) {}
Account::Account(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : o_budget(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
Account::Account(Budget *parent_budget) : o_budget(parent_budget), i_first_revision(parent_budget->revision()), i_last_revision(parent_budget->revision()) {}
Account::Account() : o_budget(NULL), i_id(0), i_first_revision(1), i_last_revision(1) {}
Account::Account(const Account *account) : o_budget(account->budget()), i_id(account->id()), i_first_revision(account->firstRevision()), i_last_revision(account->lastRevision()), s_name(account->name()), s_description(account->description()) {}
Account::~Account() {}

void Account::set(const Account *account) {
	i_id = account->id();
	i_first_revision = account->firstRevision();
	i_last_revision = account->lastRevision();
	s_name = account->name();
	s_description = account->description();
}
void Account::readAttributes(QXmlStreamAttributes *attr, bool*) {
	read_id(attr, i_id, i_first_revision, i_last_revision);
	s_name = attr->value("name").trimmed().toString();
	s_description = attr->value("description").trimmed().toString();
}
bool Account::readElement(QXmlStreamReader*, bool*) {return false;}
bool Account::readElements(QXmlStreamReader *xml, bool *valid) {
	while(xml->readNextStartElement()) {
		if(!readElement(xml, valid)) xml->skipCurrentElement();
	}
	return true;
}
void Account::save(QXmlStreamWriter *xml) {
	QXmlStreamAttributes attr;
	writeAttributes(&attr);
	xml->writeAttributes(attr);
	writeElements(xml);
}
void Account::writeAttributes(QXmlStreamAttributes *attr) {
	attr->append("name", s_name);
	write_id(attr, i_id, i_first_revision, i_last_revision);
	if(!s_description.isEmpty()) attr->append("description", s_description);
}
void Account::writeElements(QXmlStreamWriter*) {}
const QString &Account::name() const {return s_name;}
QString Account::nameWithParent(bool) const {return s_name;}
Account *Account::topAccount() {return this;}
void Account::setName(QString new_name) {s_name = new_name.trimmed(); o_budget->accountNameModified(this);}
const QString &Account::description() const {return s_description;}
void Account::setDescription(QString new_description) {s_description = new_description.trimmed();}
bool Account::isClosed() const {return false;}
Budget *Account::budget() const {return o_budget;}
qlonglong Account::id() const {return i_id;}
void Account::setId(qlonglong new_id) {i_id = new_id;}
int Account::firstRevision() const {return i_first_revision;}
void Account::setFirstRevision(int new_rev) {i_first_revision = new_rev; if(i_first_revision > i_last_revision) i_last_revision = i_first_revision;}
int Account::lastRevision() const {return i_last_revision;}
void Account::setLastRevision(int new_rev) {i_last_revision = new_rev;}
Currency *Account::currency() const {return o_budget->defaultCurrency();}

AssetsAccount::AssetsAccount(Budget *parent_budget, int initial_type, QString initial_name, double initial_balance, QString initial_description) : Account(parent_budget, initial_name, initial_description), at_type(initial_type), d_initbal(initial_type == ASSETS_TYPE_SECURITIES ? 0.0 : initial_balance), b_closed(false) {
	o_currency = parent_budget->defaultCurrency();
}
AssetsAccount::AssetsAccount(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : Account(parent_budget) {
	o_currency = NULL;
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
AssetsAccount::AssetsAccount(Budget *parent_budget) : Account(parent_budget), at_type(ASSETS_TYPE_CASH), d_initbal(0.0), b_closed(false) {
	o_currency = parent_budget->defaultCurrency();
	i_id = parent_budget->getNewId();
}
AssetsAccount::AssetsAccount() : Account(), at_type(ASSETS_TYPE_CASH), d_initbal(0.0), b_closed(false), o_currency(NULL) {}
AssetsAccount::AssetsAccount(const AssetsAccount *account) : Account(account), at_type(account->accountType()), d_initbal(account->initialBalance()), b_closed(account->isClosed()), o_currency(account->currency()) {}
AssetsAccount::~AssetsAccount() {if(o_budget->budgetAccount == this) o_budget->budgetAccount = NULL;}

void AssetsAccount::set(const AssetsAccount *account) {
	Account::set(account);
	at_type = account->accountType();
	d_initbal = account->initialBalance();
	b_closed = account->isClosed();
	o_currency = account->currency();
}
void AssetsAccount::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	Account::readAttributes(attr, valid);
	QString type = attr->value("type").trimmed().toString();
	at_type = o_budget->getAccountType(type);
	if(attr->hasAttribute("lender")) s_maintainer = attr->value("lender").trimmed().toString();
	else if(attr->hasAttribute("issuer"))  s_maintainer = attr->value("issuer").trimmed().toString();
	else s_maintainer = attr->value("bank").trimmed().toString();
	s_group = attr->value("group").trimmed().toString();
	if(s_group == o_budget->getAccountTypeName(at_type, true, true)) s_group = "";
	QString s_cur = attr->value("currency").trimmed().toString();
	if(!s_cur.isEmpty()) {
		o_currency = o_budget->findCurrency(s_cur);
	}
	if(!o_currency) o_currency = o_budget->defaultCurrency();
	if(!isSecurities()) {
		d_initbal = attr->value("initialbalance").toDouble();
		if(attr->hasAttribute("budgetaccount") && !isLiabilities()) {
			bool b_budget = attr->value("budgetaccount").toInt();
			if(b_budget) {
				o_budget->budgetAccount = this;
			}
		}
	}
	if(!isDebt() && attr->hasAttribute("closed")) {
		b_closed = attr->value("closed").toInt();
	} else {
		b_closed = false;
	}
}
bool AssetsAccount::isDebt() const {
	return o_budget->accountTypeIsDebt(at_type);
}
bool AssetsAccount::isCreditCard() const {
	return o_budget->accountTypeIsCreditCard(at_type);
}
bool AssetsAccount::isSecurities() const {
	return o_budget->accountTypeIsSecurities(at_type);
}
bool AssetsAccount::isLiabilities() const {
	return o_budget->accountTypeIsLiabilities(at_type);
}
bool AssetsAccount::isTypeOther() const {
	return o_budget->accountTypeIsOther(at_type);
}
void AssetsAccount::writeAttributes(QXmlStreamAttributes *attr) {
	Account::writeAttributes(attr);
	if(o_currency) attr->append("currency", o_currency->code());
	if(!isSecurities()) {
		attr->append("initialbalance", QString::number(d_initbal, 'f', SAVE_MONETARY_DECIMAL_PLACES));
		if(o_budget->budgetAccount == this) {
			attr->append("budgetaccount", QString::number(o_budget->budgetAccount == this));
		}
	}
	attr->append("type", o_budget->getAccountTypeName(at_type));
	if(!s_group.isEmpty()) attr->append("group", s_group);
	if(!s_maintainer.isEmpty()) {
		if(isDebt()) attr->append("lender", s_maintainer);
		else if(isCreditCard()) attr->append("issuer", s_maintainer);
		else attr->append("bank", s_maintainer);
	}
	if(b_closed) attr->append("closed", QString::number(b_closed));
}

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
double AssetsAccount::initialBalance(bool calculate_for_securities) const {
	if(isSecurities()) {
		if(!calculate_for_securities) return 0.0;
		double d = 0.0;
		for(SecurityList<Security*>::const_iterator it = o_budget->securities.constBegin(); it != o_budget->securities.constEnd(); ++it) {
			Security *sec = *it;
			if(sec->account() == this) {
				d += sec->initialBalance();
			}
		}
		return d;
	}
	return d_initbal;
}
void AssetsAccount::setInitialBalance(double new_initial_balance) {if(!isSecurities()) d_initbal = new_initial_balance;}
AccountType AssetsAccount::type() const {return ACCOUNT_TYPE_ASSETS;}
void AssetsAccount::setAccountType(int new_type) {
	at_type = new_type;
	if(isDebt()) setAsBudgetAccount(false);
	if(isSecurities()) d_initbal = 0.0;
}
void AssetsAccount::setAccountTypeName(const QString &new_type, bool localized) {
	setAccountType(o_budget->getAccountType(new_type, localized));
}
QString AssetsAccount::accountTypeName(bool localized, bool plural) const {
	return o_budget->getAccountTypeName(at_type, localized, plural);
}
QString AssetsAccount::group() const {
	if(s_group.isEmpty()) return o_budget->getAccountTypeName(at_type, true, true);
	if(s_group == "-") return "";
	return s_group;
}
void AssetsAccount::setGroup(QString group_name) {
	s_group = group_name.trimmed();
	if(s_group.isEmpty()) s_group = "-";
	else if(s_group == o_budget->getAccountTypeName(at_type, true, true)) s_group = "";
}
bool AssetsAccount::isClosed() const {
	if(isDebt()) return true;
	return b_closed;
}
void AssetsAccount::setClosed(bool close_account) {
	b_closed = close_account;
	if(b_closed) setAsBudgetAccount(false);
}
const QString &AssetsAccount::maintainer() const {return s_maintainer;}
void AssetsAccount::setMaintainer(QString maintainer_name) {s_maintainer = maintainer_name.trimmed();}
int AssetsAccount::accountType() const {return at_type;}
Currency *AssetsAccount::currency() const {if(!o_currency) return o_budget->defaultCurrency(); return o_currency;}
void AssetsAccount::setCurrency(Currency *new_currency) {o_currency = new_currency;}

bool account_list_less_than(Account *t1, Account *t2) {
	if(t1->type() != ACCOUNT_TYPE_ASSETS && t2->type() != ACCOUNT_TYPE_ASSETS) {
		CategoryAccount *cat1 = (CategoryAccount*) t1;
		CategoryAccount *cat2 = (CategoryAccount*) t2;
		if(cat1->parentCategory() != cat2->parentCategory()) {
			if(!cat1->parentCategory()) {
				if(cat2->parentCategory() == cat1) return true;
				return QString::localeAwareCompare(cat1->name(), cat2->parentCategory()->name()) < 0;
			} else if(!cat2->parentCategory()) {
				if(cat1->parentCategory() == cat2) return false;
				return QString::localeAwareCompare(cat1->parentCategory()->name(), cat2->name()) < 0;
			} else {
				return QString::localeAwareCompare(cat1->parentCategory()->name(), cat2->parentCategory()->name()) < 0;
			}
		}
	}
	return QString::localeAwareCompare(t1->name(), t2->name()) < 0;
}

CategoryAccount::CategoryAccount(Budget *parent_budget, QString initial_name, QString initial_description) : Account(parent_budget, initial_name, initial_description), o_parent(NULL) {}
CategoryAccount::CategoryAccount(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : Account(parent_budget), o_parent(NULL) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
CategoryAccount::CategoryAccount(Budget *parent_budget) : Account(parent_budget), o_parent(NULL) {}
CategoryAccount::CategoryAccount() : Account(), o_parent(NULL) {}
CategoryAccount::CategoryAccount(const CategoryAccount *account) : Account(account), o_parent(NULL) {}
CategoryAccount::~CategoryAccount() {
	if(o_parent) o_parent->removeSubCategory(this, false);
	for(AccountList<CategoryAccount*>::const_iterator it = subCategories.constBegin(); it != subCategories.constEnd(); ++it) {
		CategoryAccount *ca = *it;
		ca->o_parent = NULL;
		delete ca;
	}
	subCategories.clear();
}

void CategoryAccount::set(const CategoryAccount *account) {
	Account::set(account);
	setParentCategory(account->parentCategory());
	mbudgets = account->mbudgets;
	subCategories = account->subCategories;
}
void CategoryAccount::setMergeBudgets(const CategoryAccount *account) {
	Account::set(account);
	setParentCategory(account->parentCategory());
	mergeBudgets(account, false);
	subCategories = account->subCategories;
}
void CategoryAccount::mergeBudgets(const CategoryAccount *account, bool keep) {
	for(QMap<QDate, double>::const_iterator it = account->mbudgets.begin(); it != account->mbudgets.end(); ++it) {
		if(!keep || !mbudgets.contains(it.key())) mbudgets[it.key()] = it.value();
	}
}
void CategoryAccount::readAttributes(QXmlStreamAttributes *attr, bool *valid) {
	Account::readAttributes(attr, valid);
	if(attr->hasAttribute("monthlybudget")) {
		double d_mbudget = attr->value("monthlybudget").toDouble();
		if(d_mbudget >= 0.0) {
			QDate date = QDate::currentDate();
			date.setDate(date.year(), date.month(), 1);
			mbudgets[date] = d_mbudget;
		}
	}
}
bool CategoryAccount::readElement(QXmlStreamReader *xml, bool *valid) {
	if(xml->name() == "budget") {
		QXmlStreamAttributes attr = xml->attributes();
		QDate date = QDate::fromString(attr.value("date").toString(), Qt::ISODate);
		mbudgets[date] = attr.value("value").toDouble();
		return false;
	} else if(xml->name() == "category") {
		QStringRef ctype = xml->attributes().value("type");
		bool valid2 = true;
		if(type() == ACCOUNT_TYPE_EXPENSES && ctype == "expenses") {
			ExpensesAccount *account = new ExpensesAccount(budget(), xml, &valid2);
			if(valid) {
				budget()->expensesAccounts_id[account->id()] = account;
				budget()->expensesAccounts.append(account);
				budget()->accounts.append(account);
				addSubCategory(account);
			} else {
				delete account;
			}
			return true;
		} else if(type() == ACCOUNT_TYPE_INCOMES && ctype == "incomes") {
			IncomesAccount *account = new IncomesAccount(budget(), xml, &valid2);
			if(valid) {
				budget()->incomesAccounts_id[account->id()] = account;
				budget()->incomesAccounts.append(account);
				budget()->accounts.append(account);
				addSubCategory(account);
			} else {
				delete account;
			}
			return true;
		}
		return false;
	}
	return Account::readElement(xml, valid);
}
void CategoryAccount::writeAttributes(QXmlStreamAttributes *attr) {
	Account::writeAttributes(attr);
}
void CategoryAccount::writeElements(QXmlStreamWriter *xml) {
	Account::writeElements(xml);
	QMap<QDate, double>::const_iterator it_end = mbudgets.end();
	for(QMap<QDate, double>::const_iterator it = mbudgets.begin(); it != it_end; ++it) {
		xml->writeStartElement("budget");
		xml->writeAttribute("value", QString::number(it.value(), 'f', SAVE_MONETARY_DECIMAL_PLACES));
		xml->writeAttribute("date", it.key().toString(Qt::ISODate));
		xml->writeEndElement();
	}
	for(AccountList<CategoryAccount*>::const_iterator it = subCategories.constBegin(); it != subCategories.constEnd(); ++it) {
		CategoryAccount *cat = *it;
		xml->writeStartElement("category");
		if(cat->type() == ACCOUNT_TYPE_INCOMES) xml->writeAttribute("type", "incomes");
		else xml->writeAttribute("type", "expenses");
		cat->save(xml);
		xml->writeEndElement();
	}
}
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
QString CategoryAccount::nameWithParent(bool formatted) const{
	if(o_parent) {
		if(formatted) return o_parent->name() + QString(": ") + name();
		else return o_parent->name() + QString(":") + name();  
	}
	return name();
}
Account *CategoryAccount::topAccount() {
	if(o_parent) return o_parent; 
	return this;
}
void CategoryAccount::setMonthlyBudget(const QDate &date, double new_monthly_budget) {
	mbudgets[date] = new_monthly_budget;
}
bool CategoryAccount::removeSubCategory(CategoryAccount *sub_account, bool set_parent) {
	if(set_parent) return sub_account->setParentCategory(NULL);
	return subCategories.removeAll(sub_account) > 0;
}
bool CategoryAccount::addSubCategory(CategoryAccount *sub_account, bool set_parent) {
	if(o_parent) return false;
	if(set_parent) return sub_account->setParentCategory(this);
	subCategories.inSort(sub_account);
	return true;
}
CategoryAccount *CategoryAccount::parentCategory() const {
	return o_parent;
}
bool CategoryAccount::setParentCategory(CategoryAccount *parent_account, bool add_child) {
	if(parent_account == o_parent) return false;
	if(parent_account) {
		if(type() != parent_account->type()) return false;
		if(!subCategories.isEmpty()) return false;
		if(add_child && !parent_account->addSubCategory(this, false)) return false;
		if(o_parent) o_parent->removeSubCategory(this, false);
	} else if(o_parent) {
		o_parent->removeSubCategory(this, false);
	}
	o_parent = parent_account;
	return true;
}


IncomesAccount::IncomesAccount(Budget *parent_budget, QString initial_name, QString initial_description) : CategoryAccount(parent_budget, initial_name, initial_description) {}
IncomesAccount::IncomesAccount(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : CategoryAccount(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
IncomesAccount::IncomesAccount(Budget *parent_budget) : CategoryAccount(parent_budget) {
	i_id = parent_budget->getNewId();
}
IncomesAccount::IncomesAccount() : CategoryAccount() {}
IncomesAccount::IncomesAccount(const IncomesAccount *account) : CategoryAccount(account) {}
IncomesAccount::~IncomesAccount() {}

AccountType IncomesAccount::type() const {return ACCOUNT_TYPE_INCOMES;}

ExpensesAccount::ExpensesAccount(Budget *parent_budget, QString initial_name, QString initial_description) : CategoryAccount(parent_budget, initial_name, initial_description) {}
ExpensesAccount::ExpensesAccount(Budget *parent_budget, QXmlStreamReader *xml, bool *valid) : CategoryAccount(parent_budget) {
	QXmlStreamAttributes attr = xml->attributes();
	readAttributes(&attr, valid);
	readElements(xml, valid);
}
ExpensesAccount::ExpensesAccount(Budget *parent_budget) : CategoryAccount(parent_budget) {
	i_id = parent_budget->getNewId();
}
ExpensesAccount::ExpensesAccount() : CategoryAccount() {}
ExpensesAccount::ExpensesAccount(const ExpensesAccount *account) : CategoryAccount(account) {}
ExpensesAccount::~ExpensesAccount() {}

AccountType ExpensesAccount::type() const {return ACCOUNT_TYPE_EXPENSES;}

