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
#include <QHeaderView>
#include <QKeyEvent>
#include <QRadioButton>
#include <QCompleter>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QFileSystemModel>
#include <QFileDialog>
#include <QInputDialog>

#include "accountcombobox.h"
#include "budget.h"
#include "editsplitdialog.h"
#include "editaccountdialogs.h"
#include "eqonomize.h"
#include "eqonomizevalueedit.h"
#include "transactioneditwidget.h"

#include <QDebug>

extern QString last_associated_file_directory;

extern void setColumnTextWidth(QTreeWidget *w, int i, QString str);
extern void setColumnDateWidth(QTreeWidget *w, int i);
extern void setColumnMoneyWidth(QTreeWidget *w, int i, Budget *budget, double v = 9999999.99, int d = -1);
extern void setColumnStrlenWidth(QTreeWidget *w, int i, int l);

EqonomizeRadioButton::EqonomizeRadioButton(const QString &text, QWidget *parent) : QRadioButton(text, parent) {}
void EqonomizeRadioButton::keyPressEvent(QKeyEvent *event) {
	QRadioButton::keyPressEvent(event);
	if(event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
		emit returnPressed();
	}
}

class MultiItemListViewItem : public QTreeWidgetItem {

	Q_DECLARE_TR_FUNCTIONS(MultiItemListViewItem)
	
	protected:
	
		Transaction *o_trans;
		bool b_deposit;
		
	public:
	
		MultiItemListViewItem(Transaction *trans, Currency *cur, bool deposit);
		bool operator<(const QTreeWidgetItem&) const;
		Transaction *transaction() const;
		bool isDeposit() const;
		void setTransaction(Transaction *trans, Currency *cur, bool deposit);
		void currencyChanged(Currency *cur);

};

MultiItemListViewItem::MultiItemListViewItem(Transaction *trans, Currency *cur, bool deposit) : QTreeWidgetItem() {
	setTransaction(trans, cur, deposit);
	setTextAlignment(3, Qt::AlignRight | Qt::AlignVCenter);
}
bool MultiItemListViewItem::operator<(const QTreeWidgetItem &i_pre) const {
	int col = 0;
	if(treeWidget()) col = treeWidget()->sortColumn();
	MultiItemListViewItem *i = (MultiItemListViewItem*) &i_pre;
	if(col == 3) {
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
Transaction *MultiItemListViewItem::transaction() const {
	return o_trans;
}
bool MultiItemListViewItem::isDeposit() const {
	return b_deposit;
}
void MultiItemListViewItem::setTransaction(Transaction *trans, Currency *cur, bool deposit) {	
	o_trans = trans;
	b_deposit = deposit;
	Budget *budget = trans->budget();
	if(!cur) cur = budget->defaultCurrency();
	double value = 0.0;
	if(deposit) value = trans->toValue();
	else value = trans->fromValue();
	setText(1, trans->description());
	setText(2, deposit ? trans->fromAccount()->nameWithParent() : trans->toAccount()->nameWithParent());
	setText(3, cur->formatValue(deposit ? value : -value));
	if(trans->type() == TRANSACTION_TYPE_INCOME) {
		if(((Income*) trans)->security()) setText(0, tr("Dividend"));
		else if(value >= 0) setText(0, tr("Income"));
		else setText(0, tr("Repayment"));
	} else if(trans->type() == TRANSACTION_TYPE_EXPENSE) {
		if(value >= 0) setText(0, tr("Expense"));
		else setText(0, tr("Refund"));
	} else if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY) {
		setText(0, tr("Securities Purchase", "Financial security (e.g. stock, mutual fund)"));
	} else if(trans->type() == TRANSACTION_TYPE_SECURITY_SELL) {
		setText(0, tr("Securities Sale", "Financial security (e.g. stock, mutual fund)"));
	} else if(trans->toAccount() == budget->balancingAccount || trans->fromAccount() == budget->balancingAccount) {
		setText(0, tr("Account Balance Adjustment"));
	} else {
		setText(0, tr("Transfer"));
	}
}
void MultiItemListViewItem::currencyChanged(Currency *cur) {
	setTransaction(o_trans, cur, b_deposit);
}

class MultiAccountListViewItem : public QTreeWidgetItem {

	Q_DECLARE_TR_FUNCTIONS(MultiAccountListViewItem)
	
	protected:
	
		Transaction *o_trans;
		
	public:
	
		MultiAccountListViewItem(Transaction *trans);
		Transaction *transaction() const;
		void setTransaction(Transaction *trans);

};

MultiAccountListViewItem::MultiAccountListViewItem(Transaction *trans) : QTreeWidgetItem() {
	setTransaction(trans);
	setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);
}
Transaction *MultiAccountListViewItem::transaction() const {
	return o_trans;
}
void MultiAccountListViewItem::setTransaction(Transaction *trans) {
	o_trans = trans;
	setText(0, QLocale().toString(trans->date(), QLocale::ShortFormat));
	if(trans->type() == TRANSACTION_TYPE_INCOME) {
		setText(1, trans->toAccount()->nameWithParent());
	} else {
		setText(1, trans->fromAccount()->nameWithParent());
	}
	setText(2, trans->valueString());
}

EditDebtPaymentDialog::EditDebtPaymentDialog(Budget *budg, QWidget *parent, AssetsAccount *default_loan, bool allow_account_creation, bool only_interest) : QDialog(parent) {
	setWindowTitle(tr("Debt Payment"));
	setModal(true);
	QVBoxLayout *box1 = new QVBoxLayout(this);
	editWidget = new EditDebtPaymentWidget(budg, this, default_loan, allow_account_creation, only_interest);
	box1->addWidget(editWidget);
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Cancel)->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);
	connect(editWidget, SIGNAL(addmodify()), this, SLOT(accept()));
	
	editWidget->focusFirst();
	
}
EditDebtPaymentDialog::~EditDebtPaymentDialog() {}
void EditDebtPaymentDialog::accept() {
	if(!editWidget->validValues()) return;
	QDialog::accept();
}
void EditDebtPaymentDialog::keyPressEvent(QKeyEvent *e) {
	if(e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) return;
	QDialog::keyPressEvent(e);
}

EditMultiAccountDialog::EditMultiAccountDialog(Budget *budg, QWidget *parent, bool create_expenses, bool extra_parameters, bool allow_account_creation) : QDialog(parent) {

	if(create_expenses) setWindowTitle(tr("Expense with Multiple Payments"));
	else setWindowTitle(tr("Income with Multiple Payments"));
	
	setModal(true);
	QVBoxLayout *box1 = new QVBoxLayout(this);
	editWidget = new EditMultiAccountWidget(budg, this, create_expenses, extra_parameters, allow_account_creation);
	box1->addWidget(editWidget);
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Cancel)->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);
	
	editWidget->focusFirst();
}
EditMultiAccountDialog::~EditMultiAccountDialog() {}
void EditMultiAccountDialog::accept() {
	if(!editWidget->validValues()) return;
	QDialog::accept();
}
void EditMultiAccountDialog::reject() {
	editWidget->reject();
	QDialog::reject();
}
void EditMultiAccountDialog::keyPressEvent(QKeyEvent *e) {
	if(e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) return;
	QDialog::keyPressEvent(e);
}

EditMultiItemDialog::EditMultiItemDialog(Budget *budg, QWidget *parent, AssetsAccount *default_account, bool extra_parameters, bool allow_account_creation) : QDialog(parent) {
	setWindowTitle(tr("Split Transaction"));
	setModal(true);
	QVBoxLayout *box1 = new QVBoxLayout(this);
	editWidget = new EditMultiItemWidget(budg, this, default_account, extra_parameters, allow_account_creation);
	box1->addWidget(editWidget);
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Cancel)->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);
	
	editWidget->focusFirst();
	
}
EditMultiItemDialog::~EditMultiItemDialog() {}
void EditMultiItemDialog::accept() {
	if(!editWidget->validValues()) return;
	QDialog::accept();
}
void EditMultiItemDialog::reject() {
	editWidget->reject();
	QDialog::reject();
}
void EditMultiItemDialog::keyPressEvent(QKeyEvent *e) {
	if(e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) return;
	QDialog::keyPressEvent(e);
}

EditMultiItemWidget::EditMultiItemWidget(Budget *budg, QWidget *parent, AssetsAccount *default_account, bool extra_parameters, bool allow_account_creation) : QWidget(parent), budget(budg), b_extra(extra_parameters), b_create_accounts(allow_account_creation) {

	QVBoxLayout *box1 = new QVBoxLayout(this);

	QGridLayout *grid = new QGridLayout();
	box1->addLayout(grid);

	grid->addWidget(new QLabel(tr("Description:", "Transaction description property (transaction title/generic article name)")), 0, 0);
	descriptionEdit = new QLineEdit();
	grid->addWidget(descriptionEdit, 0, 1);
	descriptionEdit->setFocus();
	descriptionEdit->setCompleter(new QCompleter(this));
	descriptionEdit->completer()->setModel(new QStandardItemModel(this));
	descriptionEdit->completer()->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
	descriptionEdit->completer()->setCaseSensitivity(Qt::CaseInsensitive);
	
	QStringList descr_list;
	QString descr;
	for(SplitTransactionList<SplitTransaction*>::const_iterator it = budget->splitTransactions.constEnd(); it != budget->splitTransactions.constBegin();) {
		--it;
		SplitTransaction *split = *it;
		descr = split->description();
		if(split->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ITEMS && !descr.isEmpty() && !descr_list.contains(descr, Qt::CaseInsensitive)) {
			QList<QStandardItem*> row;
			row << new QStandardItem(descr);
			row << new QStandardItem(descr.toLower());
			((QStandardItemModel*) descriptionEdit->completer()->model())->appendRow(row);
			descr_list << descr.toLower();
		}
	}
	((QStandardItemModel*) descriptionEdit->completer()->model())->sort(1);

	grid->addWidget(new QLabel(tr("Date:")), 1, 0);
	dateEdit = new EqonomizeDateEdit(this);
	dateEdit->setCalendarPopup(true);
	grid->addWidget(dateEdit, 1, 1);

	grid->addWidget(new QLabel(tr("Account:")), 2, 0);
	accountCombo = new AccountComboBox(ACCOUNT_TYPE_ASSETS, budget, b_create_accounts, false);
	accountCombo->updateAccounts();
	accountCombo->setCurrentAccount(default_account);
	grid->addWidget(accountCombo, 2, 1);
	
	if(b_extra) {
		grid->addWidget(new QLabel(tr("Payee/Payer:")), 3, 0);
		payeeEdit = new QLineEdit();
		grid->addWidget(payeeEdit, 3, 1);
		payeeEdit->setCompleter(new QCompleter(this));
		payeeEdit->completer()->setModel(new QStandardItemModel(this));
		payeeEdit->completer()->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
		payeeEdit->completer()->setCaseSensitivity(Qt::CaseInsensitive);
		QStringList payee_list;
		for(TransactionList<Expense*>::const_iterator it = budget->expenses.constEnd(); it != budget->expenses.constBegin();) {
			--it;
			Expense *expense = *it;
			if(expense->subtype() != TRANSACTION_SUBTYPE_DEBT_FEE && expense->subtype() != TRANSACTION_SUBTYPE_DEBT_INTEREST) {
				if(!expense->payee().isEmpty() && !payee_list.contains(expense->payee(), Qt::CaseInsensitive)) {
					QList<QStandardItem*> row;
					row << new QStandardItem(expense->payee());
					row << new QStandardItem(expense->payee().toLower());
					((QStandardItemModel*) payeeEdit->completer()->model())->appendRow(row);
					payee_list << expense->payee().toLower();
				}
			}
		}
		for(TransactionList<Income*>::const_iterator it = budget->incomes.constEnd(); it != budget->incomes.constBegin();) {
			--it;
			Income *income = *it;
			if(!income->security()) {
				if(!income->payer().isEmpty() && !payee_list.contains(income->payer(), Qt::CaseInsensitive)) {
					QList<QStandardItem*> row;
					row << new QStandardItem(income->payer());
					row << new QStandardItem(income->payer().toLower());
					((QStandardItemModel*) payeeEdit->completer()->model())->appendRow(row);
					payee_list << income->payer().toLower();
				}
			}
		}
		((QStandardItemModel*) payeeEdit->completer()->model())->sort(1);
	} else {
		payeeEdit = NULL;
	}
	
	grid->addWidget(new QLabel(tr("Tags:")), b_extra ? 4 : 3, 0);
	tagButton = new TagButton(false, allow_account_creation, budget, this);
	grid->addWidget(tagButton, b_extra ? 4 : 3, 1);
	tagButton->updateTags();
	connect(tagButton, SIGNAL(newTagRequested()), this, SLOT(newTag()));
	
	grid->addWidget(new QLabel(tr("Associated file:"), this), b_extra ? 5 : 4, 0);
	QHBoxLayout *fileLayout = new QHBoxLayout();
	fileEdit = new QLineEdit(this);
	QCompleter *completer = new QCompleter(this);
	QFileSystemModel *fsModel = new QFileSystemModel(completer);
	fsModel->setRootPath(QString());
	completer->setModel(fsModel);
	fileEdit->setCompleter(completer);
	fileLayout->addWidget(fileEdit);
	QPushButton *selectFileButton = new QPushButton(LOAD_ICON("document-open"), QString(), this);
	selectFileButton->setToolTip(tr("Select a file"));
	selectFileButton->setAutoDefault(false);
	fileLayout->addWidget(selectFileButton);
	QPushButton *openFileButton = new QPushButton(LOAD_ICON("system-run"), QString(), this);
	openFileButton->setToolTip(tr("Open the file"));
	openFileButton->setAutoDefault(false);
	fileLayout->addWidget(openFileButton);
	openFileButton->setFocusPolicy(Qt::ClickFocus);
	grid->addLayout(fileLayout, b_extra ? 5 : 4, 1);
	
	grid->addWidget(new QLabel(tr("Comments:")), b_extra ? 6 : 5, 0);
	commentEdit = new QLineEdit();
	grid->addWidget(commentEdit, b_extra ? 6 : 5, 1);

	box1->addWidget(new QLabel(tr("Transactions:")));
	QHBoxLayout *box2 = new QHBoxLayout();
	box1->addLayout(box2);
	transactionsView = new EqonomizeTreeWidget(this);
	transactionsView->setSortingEnabled(true);
	transactionsView->sortByColumn(0, Qt::AscendingOrder);
	transactionsView->setAllColumnsShowFocus(true);
	transactionsView->setColumnCount(4);
	QStringList headers;
	headers << tr("Type");
	headers << tr("Description", "Transaction description property (transaction title/generic article name)");
	headers << tr("Account/Category");
	headers << tr("Value");
	transactionsView->setHeaderLabels(headers);
	transactionsView->setRootIsDecorated(false);
	QFontMetrics fm(transactionsView->font());
	int w1 = fm.boundingRect(tr("Type")).width() + 26;
	int w2 = fm.boundingRect(tr("Income")).width() + 10;
	int w3 = fm.boundingRect(tr("Expense")).width() + 10;
	if(w2 > w1) w1 = w2;
	if(w3 > w1) w1 = w3;
	transactionsView->setColumnWidth(0, w1);
	setColumnStrlenWidth(transactionsView, 1, 15);
	w1 = fm.boundingRect(tr("Account/Category")).width() + 26;
	transactionsView->setColumnWidth(2, w1);
	setColumnMoneyWidth(transactionsView, 3, budget);
	transactionsView->setMinimumWidth(transactionsView->columnWidth(0) + transactionsView->columnWidth(1) + transactionsView->columnWidth(2) +  transactionsView->columnWidth(3) + 10);
	box2->addWidget(transactionsView);
	QVBoxLayout *buttons = new QVBoxLayout();
	QPushButton *newButton = new QPushButton(tr("New"));
	buttons->addWidget(newButton);
	QMenu *newMenu = new QMenu(this);
	newButton->setMenu(newMenu);
	connect(newMenu->addAction(LOAD_ICON("document-new"), tr("New Expense…")), SIGNAL(triggered()), this, SLOT(newExpense()));
	connect(newMenu->addAction(LOAD_ICON("document-new"), tr("New Income…")), SIGNAL(triggered()), this, SLOT(newIncome()));
	connect(newMenu->addAction(LOAD_ICON("document-new"), tr("New Deposit…")), SIGNAL(triggered()), this, SLOT(newTransferTo()));
	connect(newMenu->addAction(LOAD_ICON("document-new"), tr("New Withdrawal…")), SIGNAL(triggered()), this, SLOT(newTransferFrom()));
	connect(newMenu->addAction(LOAD_ICON("document-new"), tr("New Securities Purchase…", "Financial security (e.g. stock, mutual fund)")), SIGNAL(triggered()), this, SLOT(newSecurityBuy()));
	connect(newMenu->addAction(LOAD_ICON("document-new"), tr("New Securities Sale…", "Financial security (e.g. stock, mutual fund)")), SIGNAL(triggered()), this, SLOT(newSecuritySell()));
	connect(newMenu->addAction(LOAD_ICON("document-new"), tr("New Dividend…")), SIGNAL(triggered()), this, SLOT(newDividend()));
	editButton = new QPushButton(tr("Edit…"));
	buttons->addWidget(editButton);
	editButton->setEnabled(false);
	removeButton = new QPushButton(tr("Delete"));
	buttons->addWidget(removeButton);
	buttons->addStretch(1);
	removeButton->setEnabled(false);
	box2->addLayout(buttons);
	totalLabel = new QLabel();
	updateTotalValue();
	box1->addWidget(totalLabel);
	
	connect(descriptionEdit, SIGNAL(returnPressed()), this, SLOT(focusDate()));
	connect(dateEdit, SIGNAL(returnPressed()), accountCombo, SLOT(focusAndSelectAll()));
	if(payeeEdit) {
		connect(accountCombo, SIGNAL(accountSelected(Account*)), payeeEdit, SLOT(setFocus()));
		connect(accountCombo, SIGNAL(returnPressed()), payeeEdit, SLOT(setFocus()));
		connect(payeeEdit, SIGNAL(returnPressed()), tagButton, SLOT(setFocus()));
	} else {
		connect(accountCombo, SIGNAL(accountSelected(Account*)), tagButton, SLOT(setFocus()));
		connect(accountCombo, SIGNAL(returnPressed()), tagButton, SLOT(setFocus()));
	}
	connect(tagButton, SIGNAL(returnPressed()), fileEdit, SLOT(setFocus()));
	connect(fileEdit, SIGNAL(returnPressed()), commentEdit, SLOT(setFocus()));
	connect(commentEdit, SIGNAL(returnPressed()), newButton, SLOT(setFocus()));

	connect(selectFileButton, SIGNAL(clicked()), this, SLOT(selectFile()));
	connect(openFileButton, SIGNAL(clicked()), this, SLOT(openFile()));
	connect(transactionsView, SIGNAL(itemSelectionChanged()), this, SLOT(transactionSelectionChanged()));
	connect(transactionsView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(edit(QTreeWidgetItem*)));
	connect(removeButton, SIGNAL(clicked()), this, SLOT(remove()));
	connect(editButton, SIGNAL(clicked()), this, SLOT(edit()));
	connect(dateEdit, SIGNAL(dateChanged(const QDate&)), this, SIGNAL(dateChanged(const QDate&)));
	connect(accountCombo, SIGNAL(newAccountRequested()), this, SLOT(newAccount()));
	connect(accountCombo, SIGNAL(currentAccountChanged(Account*)), this, SLOT(accountChanged()));

}
EditMultiItemWidget::~EditMultiItemWidget() {}

void EditMultiItemWidget::selectFile() {
	QStringList urls = QFileDialog::getOpenFileNames(this, QString(), (fileEdit->text().isEmpty() || fileEdit->text().contains(",")) ? last_associated_file_directory : fileEdit->text());
	if(!urls.isEmpty()) {
		QFileInfo fileInfo(urls[0]);
		last_associated_file_directory = fileInfo.absoluteDir().absolutePath();
		if(urls.size() == 1) {
			fileEdit->setText(urls[0]);
		} else {
			QString url;
			for(int i = 0; i < urls.size(); i++) {
				if(i > 0) url += ", ";
				if(urls[i].contains("\"")) {url += "\'"; url += urls[i]; url += "\'";}
				else {url += "\""; url += urls[i]; url += "\"";}
			}
			fileEdit->setText(url);
		}
	}
}
void EditMultiItemWidget::openFile() {
	open_file_list(fileEdit->text());
}
void EditMultiItemWidget::newAccount() {
	accountCombo->createAccount();
}
void EditMultiItemWidget::newTag() {
	tagButton->createTag();
}
void EditMultiItemWidget::accountChanged() {
	Account *account = selectedAccount();
	if(account) {
		updateTotalValue();
		QTreeWidgetItemIterator it(transactionsView);
		QTreeWidgetItem *i = *it;
		while(i) {			
			((MultiItemListViewItem*) i)->currencyChanged(account->currency());
			++it;
			i = *it;
		}
	}
}
void EditMultiItemWidget::focusDate() {
	if(!dateEdit) return;
	dateEdit->setFocus();
	dateEdit->setCurrentSection(QDateTimeEdit::DaySection);
}

void EditMultiItemWidget::updateTotalValue() {
	double total_value = 0.0;
	QTreeWidgetItemIterator it(transactionsView);
	QTreeWidgetItem *i = *it;
	while(i) {
		Transaction *trans = ((MultiItemListViewItem*) i)->transaction();
		if(trans) {
			if(trans->fromAccount()) total_value += trans->toValue();
			else total_value -= trans->fromValue();
		}
		++it;
		i = *it;
	}
	Currency *cur = budget->defaultCurrency();
	if(selectedAccount()) cur = selectedAccount()->currency();
	totalLabel->setText(QString("<div align=\"left\"><b>%1</b> %2</div>").arg(tr("Total value:"), cur->formatValue(total_value)));
}
AssetsAccount *EditMultiItemWidget::selectedAccount() {
	return (AssetsAccount*) accountCombo->currentAccount();
}
void EditMultiItemWidget::transactionSelectionChanged() {
	QList<QTreeWidgetItem*> list = transactionsView->selectedItems();
	MultiItemListViewItem *i = NULL;
	if(!list.isEmpty()) i = (MultiItemListViewItem*) list.first();
	editButton->setEnabled(i && i->transaction());
	removeButton->setEnabled(i && i->transaction());
}
void EditMultiItemWidget::newTransaction(int transtype, bool select_security, bool transfer_to, Account *exclude_account) {
	Currency *cur = budget->defaultCurrency();
	if(selectedAccount()) cur = selectedAccount()->currency();
	TransactionEditDialog *dialog = new TransactionEditDialog(b_extra, transtype, cur, transfer_to, NULL, SECURITY_ALL_VALUES, select_security, budget, this, b_create_accounts);
	dialog->editWidget->focusFirst();
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
void EditMultiItemWidget::newExpense() {
	newTransaction(TRANSACTION_TYPE_EXPENSE);
}
void EditMultiItemWidget::newDividend() {
	newTransaction(TRANSACTION_TYPE_INCOME, true);
}
void EditMultiItemWidget::newSecurityBuy() {
	newTransaction(TRANSACTION_TYPE_SECURITY_BUY, true);
}
void EditMultiItemWidget::newSecuritySell() {
	newTransaction(TRANSACTION_TYPE_SECURITY_SELL, true);
}
void EditMultiItemWidget::newIncome() {
	newTransaction(TRANSACTION_TYPE_INCOME);
}
void EditMultiItemWidget::newTransferFrom() {
	newTransaction(TRANSACTION_TYPE_TRANSFER, false, false, selectedAccount());
}
void EditMultiItemWidget::newTransferTo() {
	newTransaction(TRANSACTION_TYPE_TRANSFER, false, true, selectedAccount());
}
void EditMultiItemWidget::remove() {
	QList<QTreeWidgetItem*> list = transactionsView->selectedItems();
	if(list.isEmpty()) return;
	MultiItemListViewItem *i = (MultiItemListViewItem*) list.first();
	if(i->transaction()) {
		delete i->transaction();
	}
	delete i;
	updateTotalValue();
}
void EditMultiItemWidget::edit() {
	QList<QTreeWidgetItem*> list = transactionsView->selectedItems();
	if(list.isEmpty()) return;
	edit(list.first());
}
void EditMultiItemWidget::edit(QTreeWidgetItem *i_pre) {
	if(i_pre == NULL) return;
	MultiItemListViewItem *i = (MultiItemListViewItem*) i_pre;
	Transaction *trans = i->transaction();
	if(trans) {
		AssetsAccount *account = selectedAccount();
		Currency *cur = budget->defaultCurrency();
		if(account) cur = account->currency();
		Security *security = NULL;
		if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY || trans->type() == TRANSACTION_TYPE_SECURITY_SELL) {
			security = ((SecurityTransaction*) trans)->security();
		} else if(trans->type() == TRANSACTION_TYPE_INCOME && ((Income*) trans)->security()) {
			security = ((Income*) trans)->security();
		}
		TransactionEditDialog *dialog = new TransactionEditDialog(b_extra, trans->type(), cur, trans->toAccount() == NULL, security, SECURITY_ALL_VALUES, security != NULL, budget, this, b_create_accounts);
		dialog->editWidget->updateAccounts(account);
		dialog->editWidget->setTransaction(trans);
		if((trans->type() == TRANSACTION_TYPE_EXPENSE && ((Expense*) trans)->payee() == payeeEdit->text().trimmed()) || (trans->type() == TRANSACTION_TYPE_INCOME && ((Income*) trans)->payer() == payeeEdit->text().trimmed())) dialog->editWidget->setPayee("");
		if(dialog->exec() == QDialog::Accepted) {
			if(dialog->editWidget->modifyTransaction(trans)) {
				i->setTransaction(trans, cur, trans->toAccount() == NULL);
			}
			updateTotalValue();
		}
		dialog->deleteLater();
	}
}
MultiItemTransaction *EditMultiItemWidget::createTransaction() {
	if(!validValues()) return NULL;
	AssetsAccount *account = selectedAccount();
	MultiItemTransaction *split = new MultiItemTransaction(budget, dateEdit->date(), account, descriptionEdit->text());
	if(payeeEdit) split->setPayee(payeeEdit->text());
	if(fileEdit) split->setAssociatedFile(fileEdit->text());
	split->setComment(commentEdit->text());
	QTreeWidgetItemIterator it(transactionsView);
	QTreeWidgetItem *i = *it;
	if(tagButton) tagButton->modifyTransaction(split);
	while(i) {
		Transaction *trans = ((MultiItemListViewItem*) i)->transaction();
		if(trans) split->addTransaction(trans);
		++it;
		i = *it;
	}
	split->joinTags();
	return split;
}
void EditMultiItemWidget::setTransaction(MultiItemTransaction *split) {
	descriptionEdit->setText(split->description());
	dateEdit->setDate(split->date());
	accountCombo->setCurrentAccount(split->account());
	if(payeeEdit) payeeEdit->setText(split->payee());
	if(fileEdit) fileEdit->setText(split->associatedFile());
	commentEdit->setText(split->comment());
	transactionsView->clear();
	QList<QTreeWidgetItem *> items;
	tagButton->setTransaction(split);
	int c = split->count();
	QStringList tags;
	for(int i = 0; i < c; i++) {
		Transaction *trans = split->at(i);
		if(i == 0) {
			for(int i2 = 0; i2 < trans->tagsCount(false); i2++) {
				tags << trans->getTag(i2, false);
			}
		} else {
			for(int i2 = 0; i2 < tags.count();) {
				if(!trans->hasTag(tags[i2], false)) {
					tags.removeAt(i2);
				} else {
					i2++;
				}
			}
		}
	}
	for(int i = 0; i < tags.count(); i++) {
		tagButton->setTagSelected(tags[i], true);
	}
	for(int i = 0; i < c; i++) {
		Transaction *trans = split->at(i)->copy();
		for(int i2 = 0; i2 < tags.count(); i2++) {
			trans->removeTag(tags[i2]);
		}
		trans->setAssociatedFile(QString());
		trans->setDate(QDate());
		items.append(new MultiItemListViewItem(trans, split->currency(), (trans->toAccount() == split->account())));
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
	emit dateChanged(split->date());
}
void EditMultiItemWidget::setTransaction(MultiItemTransaction *split, const QDate &date) {
	setTransaction(split);
	if(dateEdit) dateEdit->setDate(date);
}

void EditMultiItemWidget::appendTransaction(Transaction *trans, bool deposit) {
	Currency *cur = budget->defaultCurrency();
	if(selectedAccount()) cur = selectedAccount()->currency();
	MultiItemListViewItem *i = new MultiItemListViewItem(trans, cur, deposit);
	transactionsView->insertTopLevelItem(transactionsView->topLevelItemCount(), i);
	transactionsView->setSortingEnabled(true);
}

void EditMultiItemWidget::reject() {
	QTreeWidgetItemIterator it(transactionsView);
	QTreeWidgetItem *i = *it;
	while(i) {
		Transaction *trans = ((MultiItemListViewItem*) i)->transaction();
		if(trans) delete trans;
		++it;
		i = *it;
	}
}
void EditMultiItemWidget::focusFirst() {
	descriptionEdit->setFocus();
}
QDate EditMultiItemWidget::date() {
	return dateEdit->date();
}
bool EditMultiItemWidget::checkAccounts() {
	if(!accountCombo->hasAccount()) {
		QMessageBox::critical(this, tr("Error"), tr("No suitable account available."));
		return false;
	}
	return true;
}
bool EditMultiItemWidget::validValues() {
	if(!checkAccounts()) return false;
	if(!dateEdit->date().isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		return false;
	}
	if(transactionsView->topLevelItemCount() < 2) {
		QMessageBox::critical(this, tr("Error"), tr("A split must contain at least two transactions."));
		return false;
	}
	AssetsAccount *account = selectedAccount();
	if(!account) return false;
	QTreeWidgetItemIterator it(transactionsView);
	QTreeWidgetItem *i = *it;
	while(i) {
		Transaction *trans = ((MultiItemListViewItem*) i)->transaction();
		if(trans && (trans->fromAccount() == account || trans->toAccount() == account)) {
			QMessageBox::critical(this, tr("Error"), tr("Cannot transfer money to and from the same account."));
			return false;
		}
		++it;
		i = *it;
	}
	return true;
}

EditMultiAccountWidget::EditMultiAccountWidget(Budget *budg, QWidget *parent, bool create_expenses, bool extra_parameters, bool allow_account_creation) : QWidget(parent), budget(budg), b_expense(create_expenses), b_extra(extra_parameters), b_create_accounts(allow_account_creation) {

	QVBoxLayout *box1 = new QVBoxLayout(this);
	
	QGridLayout *grid = new QGridLayout();
	box1->addLayout(grid);
	box1->addStretch(1);

	grid->addWidget(new QLabel(tr("Description:", "Transaction description property (transaction title/generic article name)")), 0, 0);
	descriptionEdit = new QLineEdit();
	grid->addWidget(descriptionEdit, 0, 1);
	descriptionEdit->setFocus();
	
	if(b_extra) {
		grid->addWidget(new QLabel(tr("Quantity:")), 1, 0);
		quantityEdit = new EqonomizeValueEdit(1.0, QUANTITY_DECIMAL_PLACES, true, false, this, budget);
		grid->addWidget(quantityEdit, 1, 1);
	} else {
		quantityEdit = NULL;
	}

	grid->addWidget(new QLabel(tr("Category:")), b_extra ? 2 : 1, 0);
	categoryCombo = new AccountComboBox(b_expense ? ACCOUNT_TYPE_EXPENSES : ACCOUNT_TYPE_INCOMES, budget, b_create_accounts);
	categoryCombo->updateAccounts();
	grid->addWidget(categoryCombo, b_extra ? 2 : 1, 1);
	
	grid->addWidget(new QLabel(tr("Tags:")), b_extra ? 3 : 2, 0);
	tagButton = new TagButton(false, allow_account_creation, budget, this);
	grid->addWidget(tagButton, b_extra ? 3 : 2, 1);
	tagButton->updateTags();
	connect(tagButton, SIGNAL(newTagRequested()), this, SLOT(newTag()));
	
	grid->addWidget(new QLabel(tr("Associated file:"), this), b_extra ? 4 : 3, 0);
	QHBoxLayout *fileLayout = new QHBoxLayout();
	fileEdit = new QLineEdit(this);
	QCompleter *completer = new QCompleter(this);
	QFileSystemModel *fsModel = new QFileSystemModel(completer);
	fsModel->setRootPath(QString());
	completer->setModel(fsModel);
	fileEdit->setCompleter(completer);
	fileLayout->addWidget(fileEdit);
	QPushButton *selectFileButton = new QPushButton(LOAD_ICON("document-open"), QString(), this);
	selectFileButton->setToolTip(tr("Select a file"));
	selectFileButton->setAutoDefault(false);
	fileLayout->addWidget(selectFileButton);
	QPushButton *openFileButton = new QPushButton(LOAD_ICON("system-run"), QString(), this);
	openFileButton->setToolTip(tr("Open the file"));
	openFileButton->setAutoDefault(false);
	fileLayout->addWidget(openFileButton);
	openFileButton->setFocusPolicy(Qt::ClickFocus);
	grid->addLayout(fileLayout, b_extra ? 4 : 3, 1);
	
	grid->addWidget(new QLabel(tr("Comments:")), b_extra ? 5 : 4, 0);
	commentEdit = new QLineEdit();
	grid->addWidget(commentEdit, b_extra ? 5 : 4, 1);

	box1->addWidget(new QLabel(tr("Transactions:")));
	QHBoxLayout *box2 = new QHBoxLayout();
	box1->addLayout(box2);
	transactionsView = new EqonomizeTreeWidget(this);
	transactionsView->setSortingEnabled(true);
	transactionsView->sortByColumn(0, Qt::DescendingOrder);
	transactionsView->setAllColumnsShowFocus(true);
	transactionsView->setColumnCount(3);
	QStringList headers;
	headers << tr("Date");
	headers << tr("Account");
	if(b_expense) headers << tr("Cost");
	else headers << tr("Income");
	transactionsView->setHeaderLabels(headers);
	transactionsView->setRootIsDecorated(false);
	setColumnDateWidth(transactionsView, 0);
	setColumnStrlenWidth(transactionsView, 1, 25);
	setColumnMoneyWidth(transactionsView, 2, budget);
	transactionsView->setMinimumWidth(transactionsView->columnWidth(0) + transactionsView->columnWidth(1) + transactionsView->columnWidth(2) + 10);
	box2->addWidget(transactionsView);
	QVBoxLayout *buttons = new QVBoxLayout();
	QPushButton *newButton = new QPushButton(tr("New"));
	buttons->addWidget(newButton);
	editButton = new QPushButton(tr("Edit…"));
	buttons->addWidget(editButton);
	editButton->setEnabled(false);
	removeButton = new QPushButton(tr("Delete"));
	buttons->addWidget(removeButton);
	buttons->addStretch(1);
	removeButton->setEnabled(false);
	box2->addLayout(buttons);
	totalLabel = new QLabel();
	updateTotalValue();
	box1->addWidget(totalLabel);
	
	if(quantityEdit) {
		connect(descriptionEdit, SIGNAL(returnPressed()), quantityEdit, SLOT(enterFocus()));
		connect(quantityEdit, SIGNAL(returnPressed()), categoryCombo, SLOT(focusAndSelectAll()));
	} else {
		connect(descriptionEdit, SIGNAL(returnPressed()), categoryCombo, SLOT(focusAndSelectAll()));
	}
	connect(categoryCombo, SIGNAL(accountSelected(Account*)), fileEdit, SLOT(setFocus()));
	connect(categoryCombo, SIGNAL(returnPressed()), tagButton, SLOT(setFocus()));
	connect(tagButton, SIGNAL(returnPressed()), fileEdit, SLOT(setFocus()));
	connect(fileEdit, SIGNAL(returnPressed()), commentEdit, SLOT(setFocus()));
	connect(commentEdit, SIGNAL(returnPressed()), newButton, SLOT(setFocus()));

	connect(selectFileButton, SIGNAL(clicked()), this, SLOT(selectFile()));
	connect(openFileButton, SIGNAL(clicked()), this, SLOT(openFile()));
	connect(transactionsView, SIGNAL(itemSelectionChanged()), this, SLOT(transactionSelectionChanged()));
	connect(transactionsView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(edit(QTreeWidgetItem*)));
	connect(newButton, SIGNAL(clicked()), this, SLOT(newTransaction()));
	connect(removeButton, SIGNAL(clicked()), this, SLOT(remove()));
	connect(editButton, SIGNAL(clicked()), this, SLOT(edit()));
	connect(categoryCombo, SIGNAL(newAccountRequested()), this, SLOT(newCategory()));

}
EditMultiAccountWidget::~EditMultiAccountWidget() {}

void EditMultiAccountWidget::selectFile() {
	QStringList urls = QFileDialog::getOpenFileNames(this, QString(), (fileEdit->text().isEmpty() || fileEdit->text().contains(",")) ? last_associated_file_directory : fileEdit->text());
	if(!urls.isEmpty()) {
		QFileInfo fileInfo(urls[0]);
		last_associated_file_directory = fileInfo.absoluteDir().absolutePath();
		if(urls.size() == 1) {
			fileEdit->setText(urls[0]);
		} else {
			QString url;
			for(int i = 0; i < urls.size(); i++) {
				if(i > 0) url += ", ";
				if(urls[i].contains("\"")) {url += "\'"; url += urls[i]; url += "\'";}
				else {url += "\""; url += urls[i]; url += "\"";}
			}
			fileEdit->setText(url);
		}
	}
}
void EditMultiAccountWidget::openFile() {
	open_file_list(fileEdit->text());
}

void EditMultiAccountWidget::newCategory() {
	categoryCombo->createAccount();
}

void EditMultiAccountWidget::updateTotalValue() {
	double total_value = 0.0, highest_value = 0.0;
	QTreeWidgetItemIterator it(transactionsView);
	QTreeWidgetItem *i = *it;
	Currency *cur = NULL;
	while(i) {
		Transaction *trans = ((MultiAccountListViewItem*) i)->transaction();
		if(!cur || trans->currency() == budget->defaultCurrency() || trans->value(true) > highest_value) {
			cur = trans->currency();
			if(cur == budget->defaultCurrency()) break;
			highest_value = trans->value(true);
		}
		++it;
		i = *it;
	}
	if(!cur) cur = budget->defaultCurrency();
	QTreeWidgetItemIterator it2(transactionsView);
	i = *it2;
	while(i) {
		Transaction *trans = ((MultiAccountListViewItem*) i)->transaction();
		if(trans) {
			if(budget->defaultTransactionConversionRateDate() == TRANSACTION_CONVERSION_RATE_AT_DATE) total_value += trans->currency()->convertTo(trans->value(), cur, trans->date());
			else total_value += trans->currency()->convertTo(trans->value(), cur);
		}
		++it2;
		i = *it2;
	}	
	totalLabel->setText(QString("<div align=\"left\"><b>%1</b> %2</div>").arg(tr("Total cost:"), cur->formatValue(total_value)));
}
CategoryAccount *EditMultiAccountWidget::selectedCategory() {
	return (CategoryAccount*) categoryCombo->currentAccount();
}
void EditMultiAccountWidget::transactionSelectionChanged() {
	QList<QTreeWidgetItem*> list = transactionsView->selectedItems();
	MultiAccountListViewItem *i = NULL;
	if(!list.isEmpty()) i = (MultiAccountListViewItem*) list.first();
	editButton->setEnabled(i && i->transaction());
	removeButton->setEnabled(i && i->transaction());
}
void EditMultiAccountWidget::newTransaction() {
	TransactionEditDialog *dialog = new TransactionEditDialog(b_extra, b_expense ? TRANSACTION_TYPE_EXPENSE : TRANSACTION_TYPE_INCOME, NULL, false, NULL, SECURITY_ALL_VALUES, false, budget, this, b_create_accounts, true);
	dialog->editWidget->focusFirst();
	dialog->editWidget->updateAccounts();
	if(dialog->editWidget->checkAccounts() && dialog->exec() == QDialog::Accepted) {
		Transaction *trans = dialog->editWidget->createTransaction();
		if(trans) {
			appendTransaction(trans);
		}
		updateTotalValue();
	}
	dialog->deleteLater();
}
void EditMultiAccountWidget::remove() {
	QDate d_date = date();
	QList<QTreeWidgetItem*> list = transactionsView->selectedItems();
	if(list.isEmpty()) return;
	MultiAccountListViewItem *i = (MultiAccountListViewItem*) list.first();
	if(i->transaction()) {
		delete i->transaction();
	}
	delete i;
	updateTotalValue();
	QDate d_date_new = date();
	if(d_date != d_date_new) emit dateChanged(d_date_new);
}
void EditMultiAccountWidget::edit() {
	QList<QTreeWidgetItem*> list = transactionsView->selectedItems();
	if(list.isEmpty()) return;
	edit(list.first());
}
void EditMultiAccountWidget::edit(QTreeWidgetItem *i_pre) {
	if(i_pre == NULL) return;
	MultiAccountListViewItem *i = (MultiAccountListViewItem*) i_pre;
	Transaction *trans = i->transaction();
	if(trans) {
		TransactionEditDialog *dialog = new TransactionEditDialog(b_extra, trans->type(), NULL, false, NULL, SECURITY_ALL_VALUES, false, budget, this, b_create_accounts, true);
		dialog->editWidget->updateAccounts();
		dialog->editWidget->setTransaction(trans);
		if(dialog->exec() == QDialog::Accepted) {
			QDate d_date = date();
			if(dialog->editWidget->modifyTransaction(trans)) {
				i->setTransaction(trans);
			}
			updateTotalValue();
			QDate d_date_new = date();
			if(d_date != d_date_new) emit dateChanged(d_date_new);
		}
		dialog->deleteLater();
	}
}
void EditMultiAccountWidget::newTag() {
	tagButton->createTag();
}
MultiAccountTransaction *EditMultiAccountWidget::createTransaction() {
	if(!validValues()) return NULL;
	CategoryAccount *account = selectedCategory();
	MultiAccountTransaction *split = new MultiAccountTransaction(budget, account, descriptionEdit->text());
	split->setComment(commentEdit->text());
	split->setAssociatedFile(fileEdit->text());
	tagButton->modifyTransaction(split);
	QTreeWidgetItemIterator it(transactionsView);
	QTreeWidgetItem *i = *it;
	while(i) {
		Transaction *trans = ((MultiAccountListViewItem*) i)->transaction();
		if(trans) split->addTransaction(trans);
		++it;
		i = *it;
	}
	return split;
}
void EditMultiAccountWidget::setValues(QString description_string, CategoryAccount *category_account, double quantity_value, QString comment_string) {
	descriptionEdit->setText(description_string);
	commentEdit->setText(comment_string);
	if(quantityEdit) quantityEdit->setValue(quantity_value);
	categoryCombo->setCurrentAccount(category_account);
}
void EditMultiAccountWidget::setTransaction(Transactions *transs) {
	if(transs->generaltype() == GENERAL_TRANSACTION_TYPE_SCHEDULE) {
		setTransaction(((ScheduledTransaction*) transs)->transaction());
		return;
	}
	descriptionEdit->setText(transs->description());
	commentEdit->setText(transs->comment());
	fileEdit->setText(transs->associatedFile());
	if(quantityEdit) quantityEdit->setValue(transs->quantity());
	transactionsView->clear();
	if(transs->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT && ((SplitTransaction*) transs)->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ACCOUNTS) {
		MultiAccountTransaction *split = (MultiAccountTransaction*) transs;
		categoryCombo->setCurrentAccount(split->category());
		QList<QTreeWidgetItem *> items;
		int c = split->count();
		for(int i = 0; i < c; i++) {
			Transaction *trans = split->at(i)->copy();
			trans->setComment(QString());
			trans->setAssociatedFile(QString());
			items.append(new MultiAccountListViewItem(trans));
		}
		transactionsView->addTopLevelItems(items);
	} else if(transs->generaltype() == GENERAL_TRANSACTION_TYPE_SINGLE) {
		Transaction *trans = (Transaction*) transs;
		if(trans->type() == TRANSACTION_TYPE_EXPENSE) {
			categoryCombo->setCurrentAccount(((Expense*) trans)->category());
		} else if(trans->type() == TRANSACTION_TYPE_INCOME) {
			categoryCombo->setCurrentAccount(((Income*) trans)->category());
		}
	}
	tagButton->setTransaction(transs);
	updateTotalValue();
	transactionsView->setSortingEnabled(true);
	emit dateChanged(transs->date());
}
void EditMultiAccountWidget::setTransaction(MultiAccountTransaction *split, const QDate &date) {
	descriptionEdit->setText(split->description());
	categoryCombo->setCurrentAccount(split->category());
	if(quantityEdit) quantityEdit->setValue(split->quantity());
	commentEdit->setText(split->comment());
	fileEdit->setText(split->associatedFile());
	transactionsView->clear();
	QList<QTreeWidgetItem *> items;
	int c = split->count();
	for(int i = 0; i < c; i++) {
		Transaction *trans = split->at(i)->copy();
		trans->setComment(QString());
		trans->setAssociatedFile(QString());
		trans->setDate(date);
		items.append(new MultiAccountListViewItem(trans));
	}
	tagButton->setTransaction(split);
	transactionsView->addTopLevelItems(items);
	updateTotalValue();
	transactionsView->setSortingEnabled(true);
	emit dateChanged(split->date());
}

void EditMultiAccountWidget::appendTransaction(Transaction *trans) {
	QDate d_date = date();
	MultiAccountListViewItem *i = new MultiAccountListViewItem(trans);
	transactionsView->insertTopLevelItem(transactionsView->topLevelItemCount(), i);
	transactionsView->setSortingEnabled(true);
	QDate d_date_new = date();
	if(d_date != d_date_new) emit dateChanged(d_date_new);
}
QDate EditMultiAccountWidget::date() {
	QDate d_date;
	QTreeWidgetItemIterator it(transactionsView);
	QTreeWidgetItem *i = *it;
	while(i) {
		Transaction *trans = ((MultiAccountListViewItem*) i)->transaction();
		if(trans && (d_date.isNull() || trans->date() < d_date)) d_date = trans->date();
		++it;
		i = *it;
	}
	if(d_date.isNull()) d_date = QDate::currentDate();
	return d_date;
}
void EditMultiAccountWidget::reject() {
	QTreeWidgetItemIterator it(transactionsView);
	QTreeWidgetItem *i = *it;
	while(i) {
		Transaction *trans = ((MultiAccountListViewItem*) i)->transaction();
		if(trans) delete trans;
		++it;
		i = *it;
	}
}
void EditMultiAccountWidget::focusFirst() {
	return descriptionEdit->setFocus();
}
bool EditMultiAccountWidget::checkAccounts() {
	if(!categoryCombo->hasAccount()) {
		QMessageBox::critical(this, tr("Error"), tr("No suitable expense categories available."));
		return false;
	}
	return true;
}
bool EditMultiAccountWidget::validValues() {
	if(!checkAccounts()) return false;
	if(transactionsView->topLevelItemCount() < 2) {
		QMessageBox::critical(this, tr("Error"), tr("A split must contain at least two transactions."));
		return false;
	}
	if(!selectedCategory()) return false;
	return true;
}

EditDebtPaymentWidget::EditDebtPaymentWidget(Budget *budg, QWidget *parent, AssetsAccount *default_loan, bool allow_account_creation, bool only_interest) : QWidget(parent), budget(budg), b_create_accounts(allow_account_creation) {

	b_search = true;

	QVBoxLayout *box1 = new QVBoxLayout(this);

	QGridLayout *grid = new QGridLayout();
	box1->addLayout(grid);
	box1->addStretch(1);
	
	int row = 0;
	
	grid->addWidget(new QLabel(tr("Debt:")), row, 0);
	loanCombo = new AccountComboBox(-3, budget, b_create_accounts);
	loanCombo->updateAccounts();
	grid->addWidget(loanCombo, row, 1); row++;

	grid->addWidget(new QLabel(tr("Date:")), row, 0);
	dateEdit = new EqonomizeDateEdit(this);
	dateEdit->setCalendarPopup(true);
	grid->addWidget(dateEdit, row, 1); row++;
	
	if(only_interest) {
		reductionEdit = NULL;
		paymentEdit = NULL;
	} else {
		grid->addWidget(new QLabel(tr("Debt reduction:")), row, 0);
		reductionEdit = new EqonomizeValueEdit(false, this, budget);
		grid->addWidget(reductionEdit, row, 1); row++;		
		paymentLabel = new QLabel(tr("Reduction payment:"));
		grid->addWidget(paymentLabel, row, 0);
		paymentEdit = new EqonomizeValueEdit(false, this, budget);
		grid->addWidget(paymentEdit, row, 1); row++;
		if(!budget->usesMultipleCurrencies()) {
			paymentLabel->setVisible(false);
			paymentEdit->setVisible(false);
		}
	}
	
	grid->addWidget(new QLabel(tr("Interest:")), row, 0);
	interestEdit = new EqonomizeValueEdit(false, this, budget);
	grid->addWidget(interestEdit, row, 1); row++;
	
	if(only_interest) {
		feeEdit = NULL;
		totalLabel = NULL;
		accountCombo = NULL;
		addedInterestButton = NULL;
		paidInterestButton = NULL;
	} else {
		QHBoxLayout *paidAddedLayout = new QHBoxLayout();
		paidAddedLayout->addStretch(1);
		paidInterestButton = new EqonomizeRadioButton(tr("Paid"), this);
		paidAddedLayout->addWidget(paidInterestButton);
		addedInterestButton = new EqonomizeRadioButton(tr("Added to debt"), this);
		paidAddedLayout->addWidget(addedInterestButton);
		paidInterestButton->setChecked(true);
		grid->addLayout(paidAddedLayout, row, 0, 1, 2); row++;
		
		grid->addWidget(new QLabel(tr("Fee:")), row, 0);
		feeEdit = new EqonomizeValueEdit(false, this, budget);
		grid->addWidget(feeEdit, row, 1); row++;

		totalLabel = new QLabel();
		grid->addWidget(totalLabel, row, 0, 1, 2); row++;
	
		grid->addWidget(new QLabel(tr("Account:")), row, 0);
		accountCombo = new AccountComboBox(ACCOUNT_TYPE_ASSETS, budget, b_create_accounts);
		accountCombo->updateAccounts();
		grid->addWidget(accountCombo, row, 1); row++;
	}
	
	grid->addWidget(new QLabel(tr("Expense category:")), row, 0);
	categoryCombo = new AccountComboBox(ACCOUNT_TYPE_EXPENSES, budget, b_create_accounts);
	categoryCombo->updateAccounts();
	grid->addWidget(categoryCombo, row, 1); row++;
	
	if(only_interest) {
		commentEdit = NULL;
		fileEdit = NULL;
	} else {
		grid->addWidget(new QLabel(tr("Associated file:"), this), row, 0);
		QHBoxLayout *fileLayout = new QHBoxLayout();
		fileEdit = new QLineEdit(this);
		QCompleter *completer = new QCompleter(this);
		QFileSystemModel *fsModel = new QFileSystemModel(completer);
		fsModel->setRootPath(QString());
		completer->setModel(fsModel);
		fileEdit->setCompleter(completer);
		fileLayout->addWidget(fileEdit);
		QPushButton *selectFileButton = new QPushButton(LOAD_ICON("document-open"), QString(), this);
		selectFileButton->setToolTip(tr("Select a file"));
		selectFileButton->setAutoDefault(false);
		fileLayout->addWidget(selectFileButton);
		QPushButton *openFileButton = new QPushButton(LOAD_ICON("system-run"), QString(), this);
		openFileButton->setToolTip(tr("Open the file"));
		openFileButton->setAutoDefault(false);
		fileLayout->addWidget(openFileButton);
		openFileButton->setFocusPolicy(Qt::ClickFocus);
		grid->addLayout(fileLayout, row, 1);
		row++;
		connect(selectFileButton, SIGNAL(clicked()), this, SLOT(selectFile()));
		connect(openFileButton, SIGNAL(clicked()), this, SLOT(openFile()));
		grid->addWidget(new QLabel(tr("Comments:")), row, 0);
		commentEdit = new QLineEdit();
		grid->addWidget(commentEdit, row, 1); row++;
	}
	
	loanCombo->focusAndSelectAll();

	if(default_loan) loanCombo->setCurrentAccount(default_loan);

	b_search = true;
	loanActivated(loanCombo->currentAccount());
	accountChanged();
	valueChanged();
	interestSourceChanged();
	b_search = true;
	
	connect(loanCombo, SIGNAL(returnPressed()), this, SLOT(focusDate()));
	connect(loanCombo, SIGNAL(accountSelected(Account*)), this, SLOT(focusDate()));
	if(only_interest) {
		connect(dateEdit, SIGNAL(returnPressed()), interestEdit, SLOT(enterFocus()));
		connect(interestEdit, SIGNAL(returnPressed()), categoryCombo, SLOT(focusAndSelectAll()));
		connect(categoryCombo, SIGNAL(returnPressed()), this, SIGNAL(addmodify()));
	} else {
		connect(dateEdit, SIGNAL(returnPressed()), reductionEdit, SLOT(enterFocus()));
		connect(reductionEdit, SIGNAL(returnPressed()), this, SLOT(reductionFocusNext()));
		connect(paymentEdit, SIGNAL(returnPressed()), interestEdit, SLOT(enterFocus()));
		connect(interestEdit, SIGNAL(returnPressed()), paidInterestButton, SLOT(setFocus()));
		connect(paidInterestButton, SIGNAL(returnPressed()), feeEdit, SLOT(enterFocus()));
		connect(addedInterestButton, SIGNAL(returnPressed()), feeEdit, SLOT(enterFocus()));
		connect(feeEdit, SIGNAL(returnPressed()), this, SLOT(feeFocusNext()));
		connect(accountCombo, SIGNAL(accountSelected(Account*)), this, SLOT(accountFocusNext()));
		connect(accountCombo, SIGNAL(returnPressed()), this, SLOT(accountFocusNext()));
		connect(categoryCombo, SIGNAL(accountSelected(Account*)), fileEdit, SLOT(setFocus()));
		connect(categoryCombo, SIGNAL(returnPressed()), fileEdit, SLOT(setFocus()));
		connect(fileEdit, SIGNAL(returnPressed()), commentEdit, SLOT(setFocus()));
		connect(commentEdit, SIGNAL(returnPressed()), this, SIGNAL(addmodify()));
	}

	if(dateEdit) connect(dateEdit, SIGNAL(dateChanged(const QDate&)), this, SIGNAL(dateChanged(const QDate&)));
	if(dateEdit) connect(dateEdit, SIGNAL(dateChanged(const QDate&)), this, SLOT(hasBeenModified()));
	if(commentEdit) connect(commentEdit, SIGNAL(textEdited(const QString&)), this, SLOT(hasBeenModified()));
	if(fileEdit) connect(fileEdit, SIGNAL(textChanged(const QString&)), this, SLOT(hasBeenModified()));
	if(feeEdit) connect(feeEdit, SIGNAL(valueChanged(double)), this, SLOT(valueChanged()));
	if(paymentEdit) connect(paymentEdit, SIGNAL(valueChanged(double)), this, SLOT(valueChanged()));
	if(reductionEdit && paymentEdit) connect(reductionEdit, SIGNAL(editingFinished()), this, SLOT(reductionEditingFinished()));
	if(reductionEdit) connect(reductionEdit, SIGNAL(valueChanged(double)), this, SLOT(valueChanged()));
	if(interestEdit) connect(interestEdit, SIGNAL(valueChanged(double)), this, SLOT(valueChanged()));
	if(paidInterestButton) connect(paidInterestButton, SIGNAL(toggled(bool)), this, SLOT(interestSourceChanged()));
	if(addedInterestButton) connect(addedInterestButton, SIGNAL(toggled(bool)), this, SLOT(interestSourceChanged()));
	if(loanCombo) connect(loanCombo, SIGNAL(accountSelected(Account*)), this, SLOT(loanActivated(Account*)));
	if(accountCombo) connect(accountCombo, SIGNAL(newAccountRequested()), this, SLOT(newAccount()));
	if(loanCombo) connect(loanCombo, SIGNAL(newAccountRequested()), this, SLOT(newLoan()));
	if(categoryCombo) connect(categoryCombo, SIGNAL(newAccountRequested()), this, SLOT(newCategory()));
	if(accountCombo) connect(accountCombo, SIGNAL(currentAccountChanged(Account*)), this, SLOT(accountChanged()));

}
EditDebtPaymentWidget::~EditDebtPaymentWidget() {}

void EditDebtPaymentWidget::focusFirst() {
	loanCombo->focusAndSelectAll();
}
void EditDebtPaymentWidget::focusDate() {
	if(!dateEdit) return;
	dateEdit->setFocus();
	dateEdit->setCurrentSection(QDateTimeEdit::DaySection);
}
void EditDebtPaymentWidget::reductionFocusNext() {
	if(paymentEdit->isVisible() && paymentEdit->isEnabled()) paymentEdit->enterFocus();
	else interestEdit->enterFocus();
}
void EditDebtPaymentWidget::accountFocusNext() {
	if(categoryCombo->isEnabled()) categoryCombo->focusAndSelectAll();
	else fileEdit->setFocus();
}
void EditDebtPaymentWidget::feeFocusNext() {
	if(accountCombo->isEnabled()) accountCombo->focusAndSelectAll();
	else if(categoryCombo->isEnabled()) categoryCombo->focusAndSelectAll();
	else fileEdit->setFocus();
}

void EditDebtPaymentWidget::selectFile() {
	QStringList urls = QFileDialog::getOpenFileNames(this, QString(), (fileEdit->text().isEmpty() || fileEdit->text().contains(",")) ? last_associated_file_directory : fileEdit->text());
	if(!urls.isEmpty()) {
		QFileInfo fileInfo(urls[0]);
		last_associated_file_directory = fileInfo.absoluteDir().absolutePath();
		if(urls.size() == 1) {
			fileEdit->setText(urls[0]);
		} else {
			QString url;
			for(int i = 0; i < urls.size(); i++) {
				if(i > 0) url += ", ";
				if(urls[i].contains("\"")) {url += "\'"; url += urls[i]; url += "\'";}
				else {url += "\""; url += urls[i]; url += "\"";}
			}
			fileEdit->setText(url);
		}
	}
}
void EditDebtPaymentWidget::openFile() {
	open_file_list(fileEdit->text());
}

void EditDebtPaymentWidget::newAccount() {
	accountCombo->createAccount();
	loanCombo->updateAccounts();
	bool b = budget->usesMultipleCurrencies();
	paymentLabel->setVisible(b);
	paymentEdit->setVisible(b);
}
void EditDebtPaymentWidget::newCategory() {
	categoryCombo->createAccount();
}
void EditDebtPaymentWidget::newLoan() {
	loanCombo->createAccount();
	accountCombo->updateAccounts();
	bool b = budget->usesMultipleCurrencies();
	paymentLabel->setVisible(b);
	paymentEdit->setVisible(b);
}
void EditDebtPaymentWidget::accountChanged() {
	valueChanged();
}
void EditDebtPaymentWidget::loanActivated(Account *account) {
	if(!account) return;
	AssetsAccount *loan = (AssetsAccount*) account;
	if(b_search) {
		DebtPayment *trans = NULL;		
		SplitTransactionList<SplitTransaction*>::const_iterator it = budget->splitTransactions.constEnd();
		while(it != budget->splitTransactions.constBegin()) {
			--it;
			SplitTransaction *split = *it;
			if(split->type() == SPLIT_TRANSACTION_TYPE_LOAN && ((DebtPayment*) split)->loan() == loan) {
				trans = (DebtPayment*) split;
				break;
			}
		}
		if(trans) {
			if(paymentEdit) paymentEdit->setValue(trans->payment());
			if(reductionEdit) reductionEdit->setValue(trans->reduction());
			if(feeEdit) feeEdit->setValue(trans->fee());
			if(interestEdit) interestEdit->setValue(trans->interest());
			if(accountCombo) accountCombo->setCurrentAccount(trans->account());
		}
		if(categoryCombo) {
			if(trans && trans->expenseCategory()) {
				categoryCombo->setCurrentAccount(trans->expenseCategory());
			} else {
				ExpensesAccount *cat = NULL;
				while(it != budget->splitTransactions.constBegin()) {
					--it;
					SplitTransaction *split = *it;
					if(split->type() == SPLIT_TRANSACTION_TYPE_LOAN && ((DebtPayment*) split)->loan() == loan) {
						cat = ((DebtPayment*) split)->expenseCategory();
						if(cat) break;
					}
				}
				if(cat) {
					categoryCombo->setCurrentAccount(cat);
				} else {
					for(TransactionList<Expense*>::const_iterator it = budget->expenses.constEnd(); it != budget->expenses.constBegin();) {
						--it;
						Expense *etrans = *it;
						if(etrans->from() == loan) {
							categoryCombo->setCurrentAccount(etrans->category());
							break;
						}
					}
				}
			}
		}
		valueChanged();
		b_search = true;
	} else {
		valueChanged();
	}
}

void EditDebtPaymentWidget::reductionEditingFinished() {
	if(paymentEdit && paymentEdit->isEnabled() && paymentEdit->value() == 0.0 && paymentEdit->currency() && reductionEdit->currency()) {
		paymentEdit->setValue(reductionEdit->currency()->convertTo(reductionEdit->value(), paymentEdit->currency()));
	}
}
void EditDebtPaymentWidget::valueChanged() {
	if(categoryCombo) categoryCombo->setEnabled((!paymentEdit && !reductionEdit) || (interestEdit && interestEdit->value() > 0.0) || (feeEdit && feeEdit->value() > 0.0));
	if(accountCombo && interestEdit && paidInterestButton) {
		accountCombo->setEnabled(interestEdit->value() == 0.0 || (feeEdit && feeEdit->value() > 0.0) || (paymentEdit && paymentEdit->value() > 0.0) || (reductionEdit && reductionEdit->value() > 0.0) || paidInterestButton->isChecked());
	}
	Account *acc = selectedAccount();
	AssetsAccount *loan = selectedLoan();
	if(!acc || !loan) return;
	if(feeEdit && feeEdit->currency() != acc->currency()) feeEdit->setCurrency(acc->currency());
	if(reductionEdit && reductionEdit->currency() != loan->currency()) reductionEdit->setCurrency(loan->currency());
	if((!addedInterestButton || addedInterestButton->isChecked()) && interestEdit) interestEdit->setCurrency(loan->currency());
	if(addedInterestButton && addedInterestButton->isChecked()) {
		if(selectedLoan() && interestEdit->currency() != loan->currency()) interestEdit->setCurrency(loan->currency());
	} else if(interestEdit) {
		if(selectedAccount() && interestEdit->currency() != acc->currency()) interestEdit->setCurrency(acc->currency());
	}
	if(paymentEdit) {
		if(paymentEdit->currency() != acc->currency()) paymentEdit->setCurrency(acc->currency());
		if(loan && loan->currency() == acc->currency()) {
			paymentEdit->setEnabled(false);
		} else {
			paymentEdit->setEnabled(true);
		}
	}
	if(paymentEdit && !paymentEdit->isEnabled() && paymentEdit->value() != reductionEdit->value()) {
		paymentEdit->setValue(reductionEdit->value());
	}
	updateTotalValue();
	hasBeenModified();
}
void EditDebtPaymentWidget::interestSourceChanged() {
	valueChanged();
}

void EditDebtPaymentWidget::updateTotalValue() {
	if(!totalLabel) return;
	double value = 0.0, v1 = 0.0, v2 = 0.0, v3 = 0.0;
	Currency *cur = budget->defaultCurrency();
	Currency *cur3 = cur;
	QDate date;
	if(budget->defaultTransactionConversionRateDate() == TRANSACTION_CONVERSION_RATE_AT_DATE) {
		if(dateEdit) date = dateEdit->date();
		if(!date.isValid()) date = QDate::currentDate();
	}
	if((!feeEdit || feeEdit->currency() != cur) && (!interestEdit || interestEdit->currency() != cur) && (!paymentEdit || paymentEdit->currency() != cur) && (!reductionEdit || reductionEdit->currency() != cur)) {
		if(feeEdit && feeEdit->currency()) feeEdit->currency()->convertTo(feeEdit->value(), cur, date);
		if(interestEdit && interestEdit->currency()) {
			v2 = interestEdit->currency()->convertTo(interestEdit->value(), cur, date);
			if(feeEdit && feeEdit->currency() == interestEdit->currency()) v1 += v2;
		}
		if(paymentEdit && paymentEdit->currency() && (paymentEdit->value() > 0.0 || (!reductionEdit || !reductionEdit->currency() || reductionEdit->value() <= 0.0))) {
			cur3 = paymentEdit->currency();
			v3 = cur3->convertTo(paymentEdit->value(), cur, date);
			if(feeEdit && feeEdit->currency() == paymentEdit->currency()) v1 += v3;
			else if(interestEdit && interestEdit->currency() == paymentEdit->currency()) v2 += v3;
		} else if(reductionEdit && reductionEdit->currency()) {
			cur3 = reductionEdit->currency();
			v3 = cur3->convertTo(reductionEdit->value(), cur, date);
			if(feeEdit && feeEdit->currency() == reductionEdit->currency()) v1 += v3;
			else if(interestEdit && interestEdit->currency() == reductionEdit->currency()) v2 += v3;
		}
		if(v3 >= v1 && v3 >= v2 && cur3) {
			cur = cur3;
		} else if(v2 >= v1 && interestEdit && interestEdit->currency()) {
			cur = interestEdit->currency();
		} else if(v1 > 0.0 && feeEdit && feeEdit->currency()) {
			cur = feeEdit->currency();
		}
	}
	if(feeEdit) {
		value += feeEdit->currency()->convertTo(feeEdit->value(), cur, date);
	}
	if(interestEdit) {
		value += interestEdit->currency()->convertTo(interestEdit->value(), cur, date);
	}
	if(paymentEdit && paymentEdit->currency() && (paymentEdit->value() > 0.0 || (!reductionEdit || !reductionEdit->currency() || reductionEdit->value() <= 0.0))) {
		value += paymentEdit->currency()->convertTo(paymentEdit->value(), cur, date);
	} else if(reductionEdit && reductionEdit->currency()) {
		value += reductionEdit->currency()->convertTo(reductionEdit->value(), cur, date);
	}
	totalLabel->setText(QString("<div align=\"right\"><b>%1</b> %2</div>").arg(tr("Total value:"), cur->formatValue(value)));
}
AssetsAccount *EditDebtPaymentWidget::selectedLoan() {
	if(!loanCombo) return NULL;
	return (AssetsAccount*) loanCombo->currentAccount();
}
AssetsAccount *EditDebtPaymentWidget::selectedAccount() {
	if(!accountCombo) return NULL;
	return (AssetsAccount*) accountCombo->currentAccount();
}
ExpensesAccount *EditDebtPaymentWidget::selectedCategory() {
	if(!categoryCombo) return NULL;
	return (ExpensesAccount*) categoryCombo->currentAccount();
}
DebtPayment *EditDebtPaymentWidget::createTransaction() {
	if(!validValues()) return NULL;
	AssetsAccount *loan = selectedLoan();
	AssetsAccount *account = selectedAccount();
	ExpensesAccount *category = selectedCategory();
	DebtPayment *split = new DebtPayment(budget, dateEdit->date(), loan, account ? account : loan);
	if((reductionEdit && reductionEdit->value() > 0.0) || (paymentEdit && paymentEdit->value() > 0.0)) split->setPayment(paymentEdit->value(), reductionEdit->value());
	if(feeEdit && feeEdit->value() > 0.0) split->setFee(feeEdit->value());
	if(interestEdit && interestEdit->value() > 0.0) split->setInterest(interestEdit->value(), !(addedInterestButton && addedInterestButton->isChecked()));
	if((feeEdit && feeEdit->value() > 0.0) || (interestEdit && interestEdit->value() > 0.0)) split->setExpenseCategory(category);
	if(commentEdit) split->setComment(commentEdit->text());
	if(fileEdit) split->setAssociatedFile(fileEdit->text());
	return split;
}
void EditDebtPaymentWidget::setTransaction(DebtPayment *split) {
	if(dateEdit) dateEdit->setDate(split->date());
	if(commentEdit) commentEdit->setText(split->comment());
	if(fileEdit) fileEdit->setText(split->associatedFile());
	if(loanCombo) loanCombo->setCurrentAccount(split->loan());
	if(accountCombo) accountCombo->setCurrentAccount(split->account());
	if(categoryCombo && split->expenseCategory()) categoryCombo->setCurrentAccount(split->expenseCategory());
	if(paymentEdit) paymentEdit->setValue(split->payment());
	if(reductionEdit) reductionEdit->setValue(split->reduction());
	if(interestEdit) interestEdit->setValue(split->interest());
	if(paidInterestButton && split->interestPaid()) paidInterestButton->setChecked(true);
	else if(addedInterestButton) addedInterestButton->setChecked(true);
	if(feeEdit) feeEdit->setValue(split->fee());
	valueChanged();
	emit dateChanged(split->date());
}
void EditDebtPaymentWidget::setTransaction(DebtPayment *split, const QDate &date) {
	setTransaction(split);
	if(dateEdit) dateEdit->setDate(date);
}

QDate EditDebtPaymentWidget::date() {
	if(!dateEdit) return QDate::currentDate();
	return dateEdit->date();
}
void EditDebtPaymentWidget::hasBeenModified() {
	b_search = false;
}
bool EditDebtPaymentWidget::checkAccounts() {
	if((loanCombo && !loanCombo->hasAccount()) || (accountCombo && !accountCombo->hasAccount()) || (categoryCombo && !categoryCombo->hasAccount())) {
		QMessageBox::critical(this, tr("Error"), tr("No suitable account available."));
		return false;
	}
	return true;
}
bool EditDebtPaymentWidget::validValues() {
	if(!checkAccounts()) return false;
	if(dateEdit && !dateEdit->date().isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		return false;
	}
	if((!feeEdit || feeEdit->value() <= 0.0) && (!interestEdit || interestEdit->value() <= 0.0) && (!paymentEdit || paymentEdit->value() <= 0.0) && (!reductionEdit || reductionEdit->value() <= 0.0)) {
		if(!feeEdit && !reductionEdit) {
			QMessageBox::critical(this, tr("Error"), tr("Interest must not be zero."));
			interestEdit->setFocus();
		} else {
			QMessageBox::critical(this, tr("Error"), tr("At least one value must non-zero."));
			if(reductionEdit) reductionEdit->setFocus();
		}
		return false;
	}
	if((accountCombo && !selectedAccount()) || (categoryCombo && !selectedCategory()) || (loanCombo && !selectedLoan())) return false;
	return true;
}


