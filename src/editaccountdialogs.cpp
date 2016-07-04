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

#include <QComboBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QGridLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QMessageBox>
#include <QPushButton>
#include <QLocale>

#include "budget.h"
#include "editaccountdialogs.h"
#include "eqonomizevalueedit.h"

EditAssetsAccountDialog::EditAssetsAccountDialog(Budget *budg, QWidget *parent, QString title) : QDialog(parent, 0), budget(budg) {

	setWindowTitle(title);
	setModal(true);

	QVBoxLayout *box1 = new QVBoxLayout(this);
	
	QGridLayout *grid = new QGridLayout();
	box1->addLayout(grid);
	grid->addWidget(new QLabel(tr("Type:"), this), 0, 0);
	typeCombo = new QComboBox(this);
	typeCombo->setEditable(false);
	typeCombo->addItem(tr("Cash"));
	typeCombo->addItem(tr("Current Account"));
	typeCombo->addItem(tr("Savings Account"));
	typeCombo->addItem(tr("Credit Card"));
	typeCombo->addItem(tr("Liabilities"));
	typeCombo->addItem(tr("Securities"));
	grid->addWidget(typeCombo, 0, 1);
	grid->addWidget(new QLabel(tr("Name:"), this), 1, 0);
	nameEdit = new QLineEdit(this);
	grid->addWidget(nameEdit, 1, 1);
	grid->addWidget(new QLabel(tr("Initial balance:"), this), 2, 0);
	valueEdit = new EqonomizeValueEdit(true, this);
	grid->addWidget(valueEdit, 2, 1);
	budgetButton = new QCheckBox(tr("Default account for budgeted transactions"), this);
	budgetButton->setChecked(false);
	grid->addWidget(budgetButton, 3, 0, 1, 2);
	grid->addWidget(new QLabel(tr("Description:"), this), 4, 0);
	descriptionEdit = new QTextEdit(this);
	grid->addWidget(descriptionEdit, 5, 0, 1, 2);
	nameEdit->setFocus();
	current_account = NULL;
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);
	
	connect(typeCombo, SIGNAL(activated(int)), this, SLOT(typeActivated(int)));

}
void EditAssetsAccountDialog::typeActivated(int index) {
	valueEdit->setEnabled(index != 5);
	//budgetButton->setChecked(index == 1);
	budgetButton->setEnabled(index != 5);
}
AssetsAccount *EditAssetsAccountDialog::newAccount() {
	AssetsType type;
	switch(typeCombo->currentIndex()) {
		case 1: {type = ASSETS_TYPE_CURRENT; break;}
		case 2: {type = ASSETS_TYPE_SAVINGS; break;}
		case 3: {type = ASSETS_TYPE_CREDIT_CARD; break;}
		case 4: {type = ASSETS_TYPE_LIABILITIES; break;}
		case 5: {type = ASSETS_TYPE_SECURITIES;  break;}
		default: {type = ASSETS_TYPE_CASH; break;}
	}
	AssetsAccount *account = new AssetsAccount(budget, type, nameEdit->text(), valueEdit->value(), descriptionEdit->toPlainText());
	account->setAsBudgetAccount(budgetButton->isChecked());
	return account;
}
void EditAssetsAccountDialog::modifyAccount(AssetsAccount *account) {
	account->setName(nameEdit->text());
	account->setInitialBalance(valueEdit->value());
	account->setDescription(descriptionEdit->toPlainText());
	account->setAsBudgetAccount(budgetButton->isChecked());
	switch(typeCombo->currentIndex()) {
		case 1: {account->setAccountType(ASSETS_TYPE_CURRENT); break;}
		case 2: {account->setAccountType(ASSETS_TYPE_SAVINGS); break;}
		case 3: {account->setAccountType(ASSETS_TYPE_CREDIT_CARD); break;}
		case 4: {account->setAccountType(ASSETS_TYPE_LIABILITIES); break;}
		case 5: {account->setAccountType(ASSETS_TYPE_SECURITIES);  break;}
		default: {account->setAccountType(ASSETS_TYPE_CASH); break;}
	}
}
void EditAssetsAccountDialog::setAccount(AssetsAccount *account) {
	current_account = account;
	nameEdit->setText(account->name());
	valueEdit->setValue(account->initialBalance());
	descriptionEdit->setPlainText(account->description());
	budgetButton->setChecked(account->isBudgetAccount());
	switch(account->accountType()) {
		case ASSETS_TYPE_CURRENT: {typeCombo->setCurrentIndex(1); break;}
		case ASSETS_TYPE_SAVINGS: {typeCombo->setCurrentIndex(2); break;}
		case ASSETS_TYPE_CREDIT_CARD: {typeCombo->setCurrentIndex(3); break;}
		case ASSETS_TYPE_LIABILITIES: {typeCombo->setCurrentIndex(4); break;}
		case ASSETS_TYPE_SECURITIES: {typeCombo->setCurrentIndex(5);  break;}
		default: {typeCombo->setCurrentIndex(0); break;}
	}
	typeActivated(typeCombo->currentIndex());
}
void EditAssetsAccountDialog::accept() {
	QString sname = nameEdit->text().trimmed();
	if(sname.isEmpty()) {
		nameEdit->setFocus();
		QMessageBox::critical(this, tr("Error"), tr("Empty name."));
		return;
	}
	AssetsAccount *aaccount = budget->findAssetsAccount(sname);
	if(aaccount && aaccount != current_account) {
		nameEdit->setFocus();
		QMessageBox::critical(this, tr("Error"), tr("The entered name is used by another account."));
		return;
	}
	QDialog::accept();
}


EditIncomesAccountDialog::EditIncomesAccountDialog(Budget *budg, IncomesAccount *default_parent, QWidget *parent, QString title) : QDialog(parent, 0), budget(budg) {

	setWindowTitle(title);
	setModal(true);
	
	if(default_parent && default_parent->parentCategory()) default_parent = NULL;

	QVBoxLayout *box1 = new QVBoxLayout(this);
	
	QGridLayout *grid = new QGridLayout();
	box1->addLayout(grid);
	grid->addWidget(new QLabel(tr("Name:"), this), 0, 0);
	nameEdit = new QLineEdit(this);
	grid->addWidget(nameEdit, 0, 1);
	grid->addWidget(new QLabel(tr("Parent category:"), this), 1, 0);
	parentCombo = new QComboBox(this);
	parentCombo->setEditable(false);
	grid->addWidget(parentCombo, 1, 1);
	parentCombo->addItem(tr("None"));
	parentCombo->setCurrentIndex(0);
	IncomesAccount *account = budget->incomesAccounts.first();
	int i = 1;
	while(account) {
		if(!account->parentCategory()) {
			parentCombo->addItem(account->name());
			parentCombo->setItemData(i, qVariantFromValue((void*) account));
			if(account == default_parent) parentCombo->setCurrentIndex(i);
			i++;
		}
		account = budget->incomesAccounts.next();
	}
	
	budgetButton = new QCheckBox(tr("Monthly budget:"), this);
	budgetButton->setChecked(false);
	grid->addWidget(budgetButton, 2, 0);
	budgetEdit = new EqonomizeValueEdit(false, this);
	budgetEdit->setEnabled(false);
	grid->addWidget(budgetEdit, 2, 1);
	grid->addWidget(new QLabel(tr("Description:"), this), 3, 0);
	descriptionEdit = new QTextEdit(this);
	grid->addWidget(descriptionEdit, 4, 0, 1, 2);
	nameEdit->setFocus();
	current_account = NULL;
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);
	
	connect(budgetButton, SIGNAL(toggled(bool)), this, SLOT(budgetEnabled(bool)));
}
IncomesAccount *EditIncomesAccountDialog::newAccount() {
	IncomesAccount *account = new IncomesAccount(budget, nameEdit->text(), descriptionEdit->toPlainText());
	if(budgetButton->isChecked()) {
		account->setMonthlyBudget(QDate::currentDate().year(), QDate::currentDate().month(), budgetEdit->value());
	}
	int i = parentCombo->currentIndex();
	if(i > 0) {
		account->setParentCategory((IncomesAccount*) parentCombo->itemData(i).value<void*>());
	}
	return account;
}
void EditIncomesAccountDialog::modifyAccount(IncomesAccount *account) {
	account->setName(nameEdit->text());
	account->setDescription(descriptionEdit->toPlainText());
	int i = parentCombo->currentIndex();
	account->setParentCategory((IncomesAccount*) parentCombo->itemData(i).value<void*>());
}
void EditIncomesAccountDialog::setAccount(IncomesAccount *account) {
	current_account = account;
	nameEdit->setText(account->name());
	if(!account->subCategories.isEmpty()) {
		parentCombo->setEnabled(false);
	}
	for(int i = 1; i < parentCombo->count(); i++) {
		if(parentCombo->itemData(i).value<void*>() == account) {
			parentCombo->removeItem(i);
			break;
		} else if(parentCombo->itemData(i).value<void*>() == account->parentCategory()) {
			parentCombo->setCurrentIndex(i);
			break;
		}
	}
	budgetEdit->hide();
	budgetButton->hide();
	descriptionEdit->setPlainText(account->description());
}
void EditIncomesAccountDialog::budgetEnabled(bool b) {
	budgetEdit->setEnabled(b);
}
void EditIncomesAccountDialog::accept() {
	QString sname = nameEdit->text().trimmed();
	if(sname.isEmpty()) {
		nameEdit->setFocus();
		QMessageBox::critical(this, tr("Error"), tr("Empty name."));
		return;
	}
	IncomesAccount *iaccount = budget->findIncomesAccount(sname);
	if(iaccount && iaccount != current_account) {
		nameEdit->setFocus();
		QMessageBox::critical(this, tr("Error"), tr("The entered name is used by another income category."));
		return;
	}
	QDialog::accept();
}

EditExpensesAccountDialog::EditExpensesAccountDialog(Budget *budg, ExpensesAccount *default_parent, QWidget *parent, QString title) : QDialog(parent, 0), budget(budg) {

	setWindowTitle(title);
	setModal(true);
	
	if(default_parent && default_parent->parentCategory()) default_parent = NULL;

	QVBoxLayout *box1 = new QVBoxLayout(this);
	
	QGridLayout *grid = new QGridLayout();
	box1->addLayout(grid);
	grid->addWidget(new QLabel(tr("Name:"), this), 0, 0);
	nameEdit = new QLineEdit(this);
	grid->addWidget(nameEdit, 0, 1);
	grid->addWidget(new QLabel(tr("Parent category:"), this), 1, 0);
	parentCombo = new QComboBox(this);
	parentCombo->setEditable(false);
	grid->addWidget(parentCombo, 1, 1);
	parentCombo->addItem(tr("None"));
	parentCombo->setCurrentIndex(0);
	ExpensesAccount *account = budget->expensesAccounts.first();
	int i = 1;
	while(account) {
		if(!account->parentCategory()) {
			parentCombo->addItem(account->name());
			parentCombo->setItemData(i, qVariantFromValue((void*) account));
			if(account == default_parent) parentCombo->setCurrentIndex(i);
			i++;
		}
		account = budget->expensesAccounts.next();
	}
	budgetButton = new QCheckBox(tr("Monthly budget:"), this);
	budgetButton->setChecked(false);
	grid->addWidget(budgetButton, 2, 0);
	budgetEdit = new EqonomizeValueEdit(false, this);
	budgetEdit->setEnabled(false);
	grid->addWidget(budgetEdit, 2, 1);
	grid->addWidget(new QLabel(tr("Description:"), this), 3, 0);
	descriptionEdit = new QTextEdit(this);
	grid->addWidget(descriptionEdit, 4, 0, 1, 2);
	nameEdit->setFocus();
	current_account = NULL;
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);
	
	connect(budgetButton, SIGNAL(toggled(bool)), this, SLOT(budgetEnabled(bool)));
}
ExpensesAccount *EditExpensesAccountDialog::newAccount() {
	ExpensesAccount *account = new ExpensesAccount(budget, nameEdit->text(), descriptionEdit->toPlainText());
	if(budgetButton->isChecked()) {
		account->setMonthlyBudget(QDate::currentDate().year(), QDate::currentDate().month(), budgetEdit->value());
	}
	int i = parentCombo->currentIndex();
	if(i > 0) {
		account->setParentCategory((ExpensesAccount*) parentCombo->itemData(i).value<void*>());
	}
	return account;
}
void EditExpensesAccountDialog::modifyAccount(ExpensesAccount *account) {
	account->setName(nameEdit->text());
	account->setDescription(descriptionEdit->toPlainText());
	int i = parentCombo->currentIndex();
	account->setParentCategory((ExpensesAccount*) parentCombo->itemData(i).value<void*>());
}
void EditExpensesAccountDialog::setAccount(ExpensesAccount *account) {
	current_account = account;
	nameEdit->setText(account->name());
	if(!account->subCategories.isEmpty()) {
		parentCombo->setEnabled(false);
	}
	for(int i = 1; i < parentCombo->count(); i++) {
		if(parentCombo->itemData(i).value<void*>() == account) {
			parentCombo->removeItem(i);
			break;
		} else if(parentCombo->itemData(i).value<void*>() == account->parentCategory()) {
			parentCombo->setCurrentIndex(i);
			break;
		}
	}
	budgetEdit->hide();
	budgetButton->hide();
	descriptionEdit->setPlainText(account->description());
}
void EditExpensesAccountDialog::budgetEnabled(bool b) {
	budgetEdit->setEnabled(b);
}
void EditExpensesAccountDialog::accept() {
	QString sname = nameEdit->text().trimmed();
	if(sname.isEmpty()) {
		nameEdit->setFocus();
		QMessageBox::critical(this, tr("Error"), tr("Empty name."));
		return;
	}
	ExpensesAccount *eaccount = budget->findExpensesAccount(sname);
	if(eaccount && eaccount != current_account) {
		nameEdit->setFocus();
		QMessageBox::critical(this, tr("Error"), tr("The entered name is used by another expense category."));
		return;
	}
	QDialog::accept();
}

