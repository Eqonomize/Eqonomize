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

#include <QLabel>
#include <QLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <QComboBox>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QLocale>
#include <QDateEdit>
#include <QMessageBox>

#include "budget.h"
#include "editsplitdialog.h"
#include "eqonomize.h"
#include "transactioneditwidget.h"

class SplitListViewItem : public QTreeWidgetItem {

	Q_DECLARE_TR_FUNCTIONS(SplitListViewItem)
	
	protected:
		Transaction *o_trans;
		bool b_deposit;
	public:
		SplitListViewItem(Transaction *trans, bool deposit);
		bool operator<(const QTreeWidgetItem&) const;
		Transaction *transaction() const;
		bool isDeposit() const;
		void setTransaction(Transaction *trans, bool deposit);
};

SplitListViewItem::SplitListViewItem(Transaction *trans, bool deposit) : QTreeWidgetItem() {
	setTransaction(trans, deposit);
	setTextAlignment(3, Qt::AlignRight);
	setTextAlignment(4, Qt::AlignRight);
}
bool SplitListViewItem::operator<(const QTreeWidgetItem &i_pre) const {
	int col = 0;
	if(treeWidget()) col = treeWidget()->sortColumn();
	SplitListViewItem *i = (SplitListViewItem*) &i_pre;
	if(col == 3) {
		double value1 = 0.0;
		double value2 = 0.0;
		if(b_deposit && o_trans->value() < 0.0) value1 = -o_trans->value();
		else if(!b_deposit && o_trans->value() >= 0.0) value1 = o_trans->value();
		if(i->isDeposit() && i->transaction()->value() < 0.0) value2 = -i->transaction()->value();
		else if(!i->isDeposit() && i->transaction()->value() >= 0.0) value2 = i->transaction()->value();
		return value1 < value2;
	} else if(col == 4) {
		double value1 = 0.0;
		double value2 = 0.0;
		if(!b_deposit && o_trans->value() < 0.0) value1 = -o_trans->value();
		else if(b_deposit && o_trans->value() >= 0.0) value1 = o_trans->value();
		if(!i->isDeposit() && i->transaction()->value() < 0.0) value2 = -i->transaction()->value();
		else if(i->isDeposit() && i->transaction()->value() >= 0.0) value2 = i->transaction()->value();
		return value1 < value2;
	}
	return QTreeWidgetItem::operator<(i_pre);
}
Transaction *SplitListViewItem::transaction() const {
	return o_trans;
}
bool SplitListViewItem::isDeposit() const {
	return b_deposit;
}
void SplitListViewItem::setTransaction(Transaction *trans, bool deposit) {
	o_trans = trans;
	b_deposit = deposit;
	Budget *budget = trans->budget();
	double value = trans->value();
	setText(1, trans->description());
	setText(2, deposit ? trans->fromAccount()->name() : trans->toAccount()->name());
	if(deposit) setText(3, value >= 0.0 ? QString::null : QLocale().toCurrencyString(-value));
	else setText(3, value < 0.0 ? QString::null : QLocale().toCurrencyString(value));
	if(!deposit) setText(4, value >= 0.0 ? QString::null : QLocale().toCurrencyString(-value));
	else setText(4, value < 0.0 ? QString::null : QLocale().toCurrencyString(value));
	if(trans->type() == TRANSACTION_TYPE_INCOME) {
		if(((Income*) trans)->security()) setText(0, tr("Dividend"));
		else if(value >= 0) setText(0, tr("Income"));
		else setText(0, tr("Repayment"));
	} else if(trans->type() == TRANSACTION_TYPE_EXPENSE) {
		if(value >= 0) setText(0, tr("Expense"));
		else setText(0, tr("Refund"));
	} else if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY) {
		setText(0, tr("Security Buy"));
	} else if(trans->type() == TRANSACTION_TYPE_SECURITY_SELL) {
		setText(0, tr("Security Sell"));
	} else if(trans->toAccount() == budget->balancingAccount || trans->fromAccount() == budget->balancingAccount) {
		setText(0, tr("Balancing"));
	} else {
		setText(0, tr("Transfer"));
	}
}

EditSplitDialog::EditSplitDialog(Budget *budg, QWidget *parent, AssetsAccount *default_account, bool extra_parameters) : QDialog(parent, 0), budget(budg), b_extra(extra_parameters) {

	setWindowTitle(tr("Split Transaction"));
	setModal(true);

	QVBoxLayout *box1 = new QVBoxLayout(this);

	QGridLayout *grid = new QGridLayout();
	box1->addLayout(grid);

	grid->addWidget(new QLabel(tr("Generic Description:")), 0, 0);
	descriptionEdit = new QLineEdit();
	grid->addWidget(descriptionEdit, 0, 1);
	descriptionEdit->setFocus();

	grid->addWidget(new QLabel(tr("Date:")), 1, 0);
	dateEdit = new QDateEdit(QDate::currentDate());
	dateEdit->setCalendarPopup(true);
	grid->addWidget(dateEdit, 1, 1);

	grid->addWidget(new QLabel(tr("Account:")), 2, 0);
	accountCombo = new QComboBox();
	accountCombo->setEditable(false);
	int i = 0;
	AssetsAccount *account = budget->assetsAccounts.first();
	while(account) {
		if(account != budget->balancingAccount && account->accountType() != ASSETS_TYPE_SECURITIES) {
			accountCombo->addItem(account->name());
			if(account == default_account) accountCombo->setCurrentIndex(i);
			i++;
		}
		account = budget->assetsAccounts.next();
	}
	grid->addWidget(accountCombo, 2, 1);

	box1->addWidget(new QLabel(tr("Transactions:")));
	QHBoxLayout *box2 = new QHBoxLayout();
	box1->addLayout(box2);
	transactionsView = new EqonomizeTreeWidget();
	transactionsView->setSortingEnabled(true);
	transactionsView->sortByColumn(0, Qt::AscendingOrder);
	transactionsView->setAllColumnsShowFocus(true);
	transactionsView->setColumnCount(5);
	QStringList headers;
	headers << tr("Type");
	headers << tr("Description", "Generic Description");
	headers << tr("Account/Category");
	headers << tr("Payment");
	headers << tr("Deposit");
	transactionsView->setHeaderLabels(headers);
	transactionsView->setRootIsDecorated(false);
	box2->addWidget(transactionsView);
	QDialogButtonBox *buttons = new QDialogButtonBox(0, Qt::Vertical);
	QPushButton *newButton = buttons->addButton(tr("New"), QDialogButtonBox::ActionRole);
	QMenu *newMenu = new QMenu(this);
	newButton->setMenu(newMenu);
	connect(newMenu->addAction(QIcon::fromTheme("document-new"), tr("New Expense…")), SIGNAL(triggered()), this, SLOT(newExpense()));
	connect(newMenu->addAction(QIcon::fromTheme("document-new"), tr("New Income…")), SIGNAL(triggered()), this, SLOT(newIncome()));
	connect(newMenu->addAction(QIcon::fromTheme("document-new"), tr("New Deposit…")), SIGNAL(triggered()), this, SLOT(newTransferTo()));
	connect(newMenu->addAction(QIcon::fromTheme("document-new"), tr("New Withdrawal…")), SIGNAL(triggered()), this, SLOT(newTransferFrom()));
	connect(newMenu->addAction(QIcon::fromTheme("document-new"), tr("Shares Bought…")), SIGNAL(triggered()), this, SLOT(newSecurityBuy()));
	connect(newMenu->addAction(QIcon::fromTheme("document-new"), tr("Shares Sold…")), SIGNAL(triggered()), this, SLOT(newSecuritySell()));
	connect(newMenu->addAction(QIcon::fromTheme("document-new"), tr("New Dividend…")), SIGNAL(triggered()), this, SLOT(newDividend()));
	editButton = buttons->addButton(tr("Edit…"), QDialogButtonBox::ActionRole);
	editButton->setEnabled(false);
	removeButton = buttons->addButton(tr("Delete"), QDialogButtonBox::ActionRole);
	removeButton->setEnabled(false);
	box2->addWidget(buttons);
	totalLabel = new QLabel();
	updateTotalValue();
	box1->addWidget(totalLabel);
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Cancel)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Cancel)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);

	connect(transactionsView, SIGNAL(itemSelectionChanged()), this, SLOT(transactionSelectionChanged()));
	connect(transactionsView, SIGNAL(doubleClicked(QTreeWidgetItem*, int)), this, SLOT(edit(QTreeWidgetItem*)));
	connect(removeButton, SIGNAL(clicked()), this, SLOT(remove()));
	connect(editButton, SIGNAL(clicked()), this, SLOT(edit()));

}
EditSplitDialog::~EditSplitDialog() {}

void EditSplitDialog::updateTotalValue() {
	double total_value = 0.0;
	QTreeWidgetItemIterator it(transactionsView);
	QTreeWidgetItem *i = *it;
	while(i) {
		Transaction *trans = ((SplitListViewItem*) i)->transaction();
		if(trans) {
			if(trans->fromAccount()) total_value += trans->value();
			else total_value -= trans->value();
		}
		++it;
		i = *it;
	}
	totalLabel->setText(QString("<div align=\"left\"><b>%1</b> %2</div>").arg(tr("Total value:"), QLocale().toCurrencyString(total_value)));
}
AssetsAccount *EditSplitDialog::selectedAccount() {
	int index = 0;
	int cur_index = accountCombo->currentIndex();
	AssetsAccount *account = budget->assetsAccounts.first();
	while(account) {
		if(account != budget->balancingAccount && account->accountType() != ASSETS_TYPE_SECURITIES) {
			if(index == cur_index) {
				break;
			}
			index++;
		}
		account = budget->assetsAccounts.next();
	}
	return account;
}
void EditSplitDialog::transactionSelectionChanged() {
	QList<QTreeWidgetItem*> list = transactionsView->selectedItems();
	SplitListViewItem *i = NULL;
	if(!list.isEmpty()) i = (SplitListViewItem*) list.first();
	editButton->setEnabled(i && i->transaction());
	removeButton->setEnabled(i && i->transaction());
}
void EditSplitDialog::newTransaction(int transtype, bool select_security, bool transfer_to, Account *exclude_account) {
	TransactionEditDialog *dialog = new TransactionEditDialog(b_extra, transtype, true, transfer_to, NULL, SECURITY_ALL_VALUES, select_security, budget, this);
        dialog->editWidget->focusDescription();
	dialog->editWidget->updateAccounts(exclude_account);
	if(dialog->editWidget->checkAccounts() && dialog->exec() == QDialog::Accepted) {
		Transaction *trans = dialog->editWidget->createTransaction();
		if(trans) {
			appendTransaction(trans, (trans->toAccount() == NULL));
		}
		updateTotalValue();
	}
	dialog->deleteLater();
}
void EditSplitDialog::newExpense() {
	newTransaction(TRANSACTION_TYPE_EXPENSE);
}
void EditSplitDialog::newDividend() {
	newTransaction(TRANSACTION_TYPE_INCOME, true);
}
void EditSplitDialog::newSecurityBuy() {
	newTransaction(TRANSACTION_TYPE_SECURITY_BUY, true);
}
void EditSplitDialog::newSecuritySell() {
	newTransaction(TRANSACTION_TYPE_SECURITY_SELL, true);
}
void EditSplitDialog::newIncome() {
	newTransaction(TRANSACTION_TYPE_INCOME);
}
void EditSplitDialog::newTransferFrom() {
	newTransaction(TRANSACTION_TYPE_TRANSFER, false, false, selectedAccount());
}
void EditSplitDialog::newTransferTo() {
	newTransaction(TRANSACTION_TYPE_TRANSFER, false, true, selectedAccount());
}
void EditSplitDialog::remove() {
	QList<QTreeWidgetItem*> list = transactionsView->selectedItems();
	if(list.isEmpty()) return;
	SplitListViewItem *i = (SplitListViewItem*) list.first();
	if(i->transaction()) {
		delete i->transaction();
	}
	delete i;
	updateTotalValue();
}
void EditSplitDialog::edit() {
	QList<QTreeWidgetItem*> list = transactionsView->selectedItems();
	if(list.isEmpty()) return;
	edit(list.first());
}
void EditSplitDialog::edit(QTreeWidgetItem *i_pre) {
	if(i_pre == NULL) return;
	SplitListViewItem *i = (SplitListViewItem*) i_pre;
	Transaction *trans = i->transaction();
	if(trans) {
		AssetsAccount *account = selectedAccount();
		Security *security = NULL;
		if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY || trans->type() == TRANSACTION_TYPE_SECURITY_SELL) {
			security = ((SecurityTransaction*) trans)->security();
		} else if(trans->type() == TRANSACTION_TYPE_INCOME && ((Income*) trans)->security()) {
			security = ((Income*) trans)->security();
		}
		TransactionEditDialog *dialog = new TransactionEditDialog(b_extra, trans->type(), true, trans->toAccount() == NULL, security, SECURITY_ALL_VALUES, security != NULL, budget, this);
		dialog->editWidget->updateAccounts(account);
		dialog->editWidget->setTransaction(trans);
		if(dialog->exec() == QDialog::Accepted) {
			if(dialog->editWidget->modifyTransaction(trans)) {
				i->setTransaction(trans, trans->toAccount() == NULL);
			}
			updateTotalValue();
		}
		dialog->deleteLater();
	}
}
SplitTransaction *EditSplitDialog::createSplitTransaction() {
	if(!validValues()) return NULL;
	AssetsAccount *account = selectedAccount();
	SplitTransaction *split = new SplitTransaction(budget, dateEdit->date(), account, descriptionEdit->text());
	QTreeWidgetItemIterator it(transactionsView);
	QTreeWidgetItem *i = *it;
	while(i) {
		Transaction *trans = ((SplitListViewItem*) i)->transaction();
		if(trans) split->addTransaction(trans);
		++it;
		i = *it;
	}
	return split;
}
void EditSplitDialog::setSplitTransaction(SplitTransaction *split) {
	descriptionEdit->setText(split->description());
	dateEdit->setDate(split->date());
	int index = 0;
	AssetsAccount *account = budget->assetsAccounts.first();
	while(account) {
		if(account != budget->balancingAccount && account->accountType() != ASSETS_TYPE_SECURITIES) {
			if(account == split->account()) {
				accountCombo->setCurrentIndex(index);
				break;
			}
			index++;
		}
		account = budget->assetsAccounts.next();
	}
	transactionsView->clear();
	QList<QTreeWidgetItem *> items;
	QVector<Transaction*>::size_type c = split->splits.count();
	for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
		Transaction *trans = split->splits[i]->copy();
		trans->setDate(QDate());
		items.append(new SplitListViewItem(trans, (trans->toAccount() == split->account())));
		switch(trans->type()) {
			case TRANSACTION_TYPE_EXPENSE: {
				((Expense*) trans)->setFrom(NULL);
				break;
			}
			case TRANSACTION_TYPE_INCOME: {
				((Income*) trans)->setTo(NULL);
				break;
			}
			case TRANSACTION_TYPE_TRANSFER: {
				if(((Transfer*) trans)->from() == split->account()) {
					((Transfer*) trans)->setFrom(NULL);
				} else {
					((Transfer*) trans)->setTo(NULL);
				}
				break;
			}
			case TRANSACTION_TYPE_SECURITY_BUY: {}
			case TRANSACTION_TYPE_SECURITY_SELL: {
				((SecurityTransaction*) trans)->setAccount(NULL);
				break;
			}
		}
	}
	transactionsView->addTopLevelItems(items);
	updateTotalValue();
	transactionsView->setSortingEnabled(true);
}
void EditSplitDialog::appendTransaction(Transaction *trans, bool deposit) {
	SplitListViewItem *i = new SplitListViewItem(trans, deposit);
	transactionsView->insertTopLevelItem(transactionsView->topLevelItemCount(), i);
	transactionsView->setSortingEnabled(true);
}

void EditSplitDialog::accept() {
	if(!validValues()) return;
	QDialog::accept();
}
void EditSplitDialog::reject() {
	QTreeWidgetItemIterator it(transactionsView);
	QTreeWidgetItem *i = *it;
	while(i) {
		Transaction *trans = ((SplitListViewItem*) i)->transaction();
		if(trans) delete trans;
		++it;
		i = *it;
	}
	QDialog::reject();
}
bool EditSplitDialog::checkAccounts() {
	if(accountCombo->count() == 0) {
		QMessageBox::critical(this, tr("Error"), tr("No suitable account available."));
		return false;
	}
	return true;
}
bool EditSplitDialog::validValues() {
	if(!checkAccounts()) return false;
	if(!dateEdit->date().isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		return false;
	}
	if(dateEdit->date() > QDate::currentDate()) {
		QMessageBox::critical(this, tr("Error"), tr("Future dates is not allowed."));
		return false;
	}
	if(transactionsView->topLevelItemCount() < 2) {
		QMessageBox::critical(this, tr("Error"), tr("A split must contain at least two transactions."));
		return false;
	}
	AssetsAccount *account = selectedAccount();
	QTreeWidgetItemIterator it(transactionsView);
	QTreeWidgetItem *i = *it;
	while(i) {
		Transaction *trans = ((SplitListViewItem*) i)->transaction();
		if(trans && (trans->fromAccount() == account || trans->toAccount() == account)) {
			QMessageBox::critical(this, tr("Error"), tr("Cannot transfer money to and from the same account."));
			return false;
		}
		++it;
		i = *it;
	}
	return true;
}


