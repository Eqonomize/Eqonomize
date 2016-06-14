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

#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <QDateTime>
#include <QMap>
#include <QString>

class QDomElement;

class Budget;

typedef enum {
	ACCOUNT_TYPE_INCOMES,
	ACCOUNT_TYPE_ASSETS,
	ACCOUNT_TYPE_EXPENSES
} AccountType;

typedef enum {
	ASSETS_TYPE_CASH,
	ASSETS_TYPE_CURRENT,
	ASSETS_TYPE_SAVINGS,
	ASSETS_TYPE_CREDIT_CARD,
	ASSETS_TYPE_LIABILITIES,
	ASSETS_TYPE_SECURITIES,
	ASSETS_TYPE_BALANCING
} AssetsType;

class Account {

	protected:

		Budget *o_budget;
		int i_id;
		QString s_name, s_description;

	public:

		Account(Budget *parent_budget, QString initial_name, QString initial_description);
		Account(Budget *parent_budget, QDomElement *e, bool *valid);
		Account();
		Account(const Account *account);
		virtual ~Account();

		const QString &name() const;
		void setName(QString new_name);
		const QString &description() const;
		void setDescription(QString new_description);
		Budget *budget() const;
		int id() const;
		void setId(int new_id);
		virtual AccountType type() const = 0;
		virtual void save(QDomElement *e) const;

};

class AssetsAccount : public Account {
	
	protected:

		AssetsType at_type;
		double d_initbal;

	public:

		AssetsAccount(Budget *parent_budget, AssetsType initial_type, QString initial_name, double initial_balance = 0.0, QString initial_description = QString::null);
		AssetsAccount(Budget *parent_budget, QDomElement *e, bool *valid);
		AssetsAccount();
		AssetsAccount(const AssetsAccount *account);
		virtual ~AssetsAccount();

		bool isBudgetAccount() const;
		void setAsBudgetAccount(bool will_be = true);
		double initialBalance() const;
		void setInitialBalance(double new_initial_balance);
		virtual AccountType type() const;
		void setAccountType(AssetsType new_type);
		virtual AssetsType accountType() const;
		void save(QDomElement *e) const;

};

class CategoryAccount : public Account {
	
	public:

		CategoryAccount(Budget *parent_budget, QString initial_name, QString initial_description = QString::null);
		CategoryAccount(Budget *parent_budget, QDomElement *e, bool *valid);
		CategoryAccount();
		CategoryAccount(const CategoryAccount *account);
		virtual ~CategoryAccount();

		QMap<QDate, double> mbudgets;
		
		double monthlyBudget(int year, int month, bool no_default = false) const;
		void setMonthlyBudget(int year, int month, double new_monthly_budget);
		double monthlyBudget(const QDate &date, bool no_default = false) const;
		void setMonthlyBudget(const QDate &date, double new_monthly_budget);
		virtual AccountType type() const = 0;
		virtual void save(QDomElement *e) const;

};

class IncomesAccount : public CategoryAccount {
	
	public:

		IncomesAccount(Budget *parent_budget, QString initial_name, QString initial_description = QString::null);
		IncomesAccount(Budget *parent_budget, QDomElement *e, bool *valid);
		IncomesAccount();
		IncomesAccount(const IncomesAccount *account);
		virtual ~IncomesAccount();

		virtual AccountType type() const;

};

class ExpensesAccount : public CategoryAccount {
	
	public:

		ExpensesAccount(Budget *parent_budget, QString initial_name, QString initial_description = QString::null);
		ExpensesAccount(Budget *parent_budget, QDomElement *e, bool *valid);
		ExpensesAccount();
		ExpensesAccount(const ExpensesAccount *account);
		virtual ~ExpensesAccount();

		virtual AccountType type() const;

};

#endif
