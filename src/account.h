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

#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <QDateTime>
#include <QMap>
#include <QString>

#include "eqonomizelist.h"

class QXmlStreamReader;
class QXmlStreamWriter;
class QXmlStreamAttributes;

class Budget;
class Currency;

typedef enum {
	ACCOUNT_TYPE_ASSETS,
	ACCOUNT_TYPE_INCOMES,
	ACCOUNT_TYPE_EXPENSES
} AccountType;

typedef enum {
	ASSETS_TYPE_CASH,
	ASSETS_TYPE_CURRENT,
	ASSETS_TYPE_SAVINGS,
	ASSETS_TYPE_CREDIT_CARD,
	ASSETS_TYPE_LIABILITIES,
	ASSETS_TYPE_SECURITIES,
	ASSETS_TYPE_BALANCING,
	ASSETS_TYPE_OTHER
} AssetsType;

class Account {

	protected:

		Budget *o_budget;
		int i_id;
		QString s_name, s_description;

	public:

		Account(Budget *parent_budget, QString initial_name, QString initial_description);
		Account(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		Account(Budget *parent_budget);
		Account();
		Account(const Account *account);
		virtual ~Account();

		virtual void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		virtual bool readElement(QXmlStreamReader *xml, bool *valid);
		virtual bool readElements(QXmlStreamReader *xml, bool *valid);
		virtual void save(QXmlStreamWriter *xml);
		virtual void writeAttributes(QXmlStreamAttributes *attr);
		virtual void writeElements(QXmlStreamWriter *xml);
		const QString &name() const;
		virtual QString nameWithParent(bool formatted = true) const;
		virtual Account *topAccount();
		void setName(QString new_name);
		const QString &description() const;
		void setDescription(QString new_description);
		virtual bool isClosed() const;
		Budget *budget() const;
		int id() const;
		void setId(int new_id);
		virtual AccountType type() const = 0;

};

class AssetsAccount : public Account {
	
	protected:

		AssetsType at_type;
		double d_initbal;
		bool b_closed;
		QString s_maintainer;
		Currency *o_currency;
		
	public:

		AssetsAccount(Budget *parent_budget, AssetsType initial_type, QString initial_name, double initial_balance = 0.0, QString initial_description = QString::null);
		AssetsAccount(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		AssetsAccount(Budget *parent_budget);
		AssetsAccount();
		AssetsAccount(const AssetsAccount *account);
		virtual ~AssetsAccount();

		virtual void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		virtual void writeAttributes(QXmlStreamAttributes *attr);
		bool isBudgetAccount() const;
		void setAsBudgetAccount(bool will_be = true);
		double initialBalance(bool calculate_for_securities = true) const;
		void setInitialBalance(double new_initial_balance);
		bool isClosed() const;
		void setClosed(bool close_account = true);
		const QString &maintainer() const;
		void setMaintainer(QString maintainer_name);
		virtual AccountType type() const;
		void setAccountType(AssetsType new_type);
		virtual AssetsType accountType() const;
		Currency *currency() const;
		void setCurrency(Currency *new_currency);

};

bool account_list_less_than(Account *t1, Account *t2);
template<class type> class AccountList : public EqonomizeList<type> {
	public:
		AccountList() : EqonomizeList<type>() {};
		void sort() {
			qSort(QList<type>::begin(), QList<type>::end(), account_list_less_than);
		}
		void inSort(type value) {
			QList<type>::insert(qLowerBound(QList<type>::begin(), QList<type>::end(), value, account_list_less_than), value);
		}
};

class CategoryAccount : public Account {

	public:

		CategoryAccount(Budget *parent_budget, QString initial_name, QString initial_description = QString::null);
		CategoryAccount(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		CategoryAccount(Budget *parent_budget);
		CategoryAccount();
		CategoryAccount(const CategoryAccount *account);
		virtual ~CategoryAccount();

		QMap<QDate, double> mbudgets;

		virtual void readAttributes(QXmlStreamAttributes *attr, bool *valid);
		virtual bool readElement(QXmlStreamReader *xml, bool *valid);
		virtual void writeAttributes(QXmlStreamAttributes *attr);
		virtual void writeElements(QXmlStreamWriter *xml);
		
		double monthlyBudget(int year, int month, bool no_default = false) const;
		void setMonthlyBudget(int year, int month, double new_monthly_budget);
		double monthlyBudget(const QDate &date, bool no_default = false) const;
		void setMonthlyBudget(const QDate &date, double new_monthly_budget);
		QString nameWithParent(bool formatted = true) const;
		virtual Account *topAccount();
		virtual AccountType type() const = 0;
		
		bool removeSubCategory(CategoryAccount *sub_account, bool set_parent = true);
		bool addSubCategory(CategoryAccount *sub_account, bool set_parent = true);
		CategoryAccount *parentCategory() const;
		bool setParentCategory(CategoryAccount *parent_account, bool add_child = true);
		
		AccountList<CategoryAccount*> subCategories;
		CategoryAccount *o_parent;

};

class IncomesAccount : public CategoryAccount {
	
	public:

		IncomesAccount(Budget *parent_budget, QString initial_name, QString initial_description = QString::null);
		IncomesAccount(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		IncomesAccount(Budget *parent_budget);
		IncomesAccount();
		IncomesAccount(const IncomesAccount *account);
		virtual ~IncomesAccount();

		virtual AccountType type() const;
		IncomesAccount *parentAccount() const;
		void setParentAccount(IncomesAccount *account);

};

class ExpensesAccount : public CategoryAccount {
	
	public:

		ExpensesAccount(Budget *parent_budget, QString initial_name, QString initial_description = QString::null);
		ExpensesAccount(Budget *parent_budget, QXmlStreamReader *xml, bool *valid);
		ExpensesAccount(Budget *parent_budget);
		ExpensesAccount();
		ExpensesAccount(const ExpensesAccount *account);
		virtual ~ExpensesAccount();

		virtual AccountType type() const;

};


#endif
