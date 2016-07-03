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

#include "ledgerdialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QList>
#include <QMenu>
#include <QCheckBox>
#include <QPushButton>
#include <QScrollBar>
#include <QStringList>
#include <QTextStream>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QAction>
#include <QUrl>
#include <QFileDialog>
#include <QSaveFile>
#include <QTemporaryFile>
#include <QMimeDatabase>
#include <QMessageBox>
#include <QPrinter>
#include <QPrintDialog>
#include <QTextDocument>
#include <QSettings>

#include "budget.h"
#include "eqonomize.h"
#include "transactioneditwidget.h"

extern QString htmlize_string(QString str);

class LedgerListViewItem : public QTreeWidgetItem {
	protected:
		Transaction *o_trans;
		SplitTransaction *o_split;
	public:
		LedgerListViewItem(Transaction *trans, SplitTransaction *split, QTreeWidget *parent, QString, QString=QString::null, QString=QString::null, QString=QString::null, QString=QString::null, QString=QString::null, QString=QString::null, QString=QString::null);
		Transaction *transaction() const;
		SplitTransaction *splitTransaction() const;
};

LedgerListViewItem::LedgerListViewItem(Transaction *trans, SplitTransaction *split, QTreeWidget *parent, QString s1, QString s2, QString s3, QString s4, QString s5, QString s6, QString s7, QString s8) : QTreeWidgetItem(parent), o_trans(trans), o_split(split) {
	setText(0, s1);
	setText(1, s2);
	setText(2, s3);
	setText(3, s4);
	setText(4, s5);
	setText(5, s6);
	setText(6, s7);
	setText(7, s8);
	setTextAlignment(4, Qt::AlignRight | Qt::AlignVCenter);
	setTextAlignment(5, Qt::AlignRight | Qt::AlignVCenter);
	setTextAlignment(6, Qt::AlignRight | Qt::AlignVCenter);
}
Transaction *LedgerListViewItem::transaction() const {
	return o_trans;
}
SplitTransaction *LedgerListViewItem::splitTransaction() const {
	return o_split;
}

LedgerDialog::LedgerDialog(AssetsAccount *acc, Eqonomize *parent, QString title, bool extra_parameters) : QDialog(NULL, 0), account(acc), mainWin(parent), b_extra(extra_parameters) {

	setWindowTitle(title);
	setModal(true);

	setAttribute(Qt::WA_DeleteOnClose, true);

	budget = account->budget();

	QVBoxLayout *box1 = new QVBoxLayout(this);

	QHBoxLayout *topbox = new QHBoxLayout();
	box1->addLayout(topbox);
	topbox->addWidget(new QLabel(tr("Account:"), this));
	accountCombo = new QComboBox(this);
	accountCombo->setEditable(false);
	int i = 0;
	AssetsAccount *aaccount = budget->assetsAccounts.first();
	while(aaccount) {
		if(aaccount != budget->balancingAccount && aaccount->accountType() != ASSETS_TYPE_SECURITIES) {
			accountCombo->addItem(aaccount->name());
			if(aaccount == account) accountCombo->setCurrentIndex(i);
			i++;
		}
		aaccount = budget->assetsAccounts.next();
	}
	topbox->addWidget(accountCombo);
	QDialogButtonBox *topbuttons = new QDialogButtonBox(this);
	exportButton = topbuttons->addButton(tr("Export…"), QDialogButtonBox::ActionRole);
	printButton = topbuttons->addButton(tr("Print…"), QDialogButtonBox::ActionRole);
	topbox->addWidget(topbuttons);
	topbox->addStretch(1);

	QHBoxLayout *box2 = new QHBoxLayout();
	box1->addLayout(box2);
	transactionsView = new EqonomizeTreeWidget(this);
	transactionsView->setSortingEnabled(false);
	transactionsView->setAllColumnsShowFocus(true);
	transactionsView->setColumnCount(7);
	QStringList headers;
	headers << tr("Date");
	headers << tr("Type");
	headers << tr("Description", "Generic Description");
	headers << tr("Account/Category");
	headers << tr("Deposit");
	headers << tr("Withdrawal");
	headers << tr("Balance");
	transactionsView->setHeaderLabels(headers);
	transactionsView->setRootIsDecorated(false);
	//transactionsView->setItemMargin(3);
	transactionsView->setMinimumHeight(450);
	transactionsView->setSelectionMode(QTreeWidget::ExtendedSelection);
	QSizePolicy sp = transactionsView->sizePolicy();
	sp.setHorizontalPolicy(QSizePolicy::MinimumExpanding);
	transactionsView->setSizePolicy(sp);
	box2->addWidget(transactionsView);
	QDialogButtonBox *buttons = new QDialogButtonBox(Qt::Vertical, this);
	QPushButton *newButton = buttons->addButton(tr("New"), QDialogButtonBox::ActionRole);
	QMenu *newMenu = new QMenu(this);
	newButton->setMenu(newMenu);
	connect(newMenu->addAction(mainWin->ActionNewExpense->icon(), mainWin->ActionNewExpense->text()), SIGNAL(triggered()), this, SLOT(newExpense()));
	connect(newMenu->addAction(mainWin->ActionNewIncome->icon(), mainWin->ActionNewIncome->text()), SIGNAL(triggered()), this, SLOT(newIncome()));
	connect(newMenu->addAction(mainWin->ActionNewTransfer->icon(), mainWin->ActionNewTransfer->text()), SIGNAL(triggered()), this, SLOT(newTransfer()));
	connect(newMenu->addAction(mainWin->ActionNewSplitTransaction->icon(), mainWin->ActionNewSplitTransaction->text()), SIGNAL(triggered()), this, SLOT(newSplit()));
	editButton = buttons->addButton(tr("Edit…"), QDialogButtonBox::ActionRole);
	editButton->setEnabled(false);
	removeButton = buttons->addButton(tr("Delete"), QDialogButtonBox::ActionRole);
	removeButton->setEnabled(false);
	joinButton = buttons->addButton(tr("Join…"), QDialogButtonBox::ActionRole);
	joinButton->setEnabled(false);
	splitUpButton = buttons->addButton(tr("Split Up"), QDialogButtonBox::ActionRole);
	splitUpButton->setEnabled(false);
	box2->addWidget(buttons);
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
	buttonBox->button(QDialogButtonBox::Close)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Close)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(reject()));
	box1->addWidget(buttonBox);

	connect(transactionsView, SIGNAL(itemSelectionChanged()), this, SLOT(transactionSelectionChanged()));
	connect(transactionsView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(edit(QTreeWidgetItem*)));
	connect(removeButton, SIGNAL(clicked()), this, SLOT(remove()));
	connect(editButton, SIGNAL(clicked()), this, SLOT(edit()));
	connect(joinButton, SIGNAL(clicked()), this, SLOT(joinTransactions()));
	connect(splitUpButton, SIGNAL(clicked()), this, SLOT(splitUpTransaction()));
	connect(exportButton, SIGNAL(clicked()), this, SLOT(saveView()));
	connect(printButton, SIGNAL(clicked()), this, SLOT(printView()));
	connect(accountCombo, SIGNAL(activated(int)), this, SLOT(accountChanged(int)));
	connect(mainWin, SIGNAL(transactionsModified()), this, SLOT(updateTransactions()));
	connect(mainWin, SIGNAL(accountsModified()), this, SLOT(updateAccounts()));

	updateTransactions();

}
LedgerDialog::~LedgerDialog() {}

void LedgerDialog::saveConfig() {
	QSettings settings;
	settings.setValue("Ledger/size", size());
}

void LedgerDialog::accountChanged(int index) {
	int i = 0;
	AssetsAccount *aaccount = budget->assetsAccounts.first();
	while(aaccount) {
		if(aaccount != budget->balancingAccount && aaccount->accountType() != ASSETS_TYPE_SECURITIES) {
			if(i == index) {
				account = aaccount;
				break;
			}
			i++;
		}
		aaccount = budget->assetsAccounts.next();
	}
	updateTransactions();
}
void LedgerDialog::updateAccounts() {
	accountCombo->blockSignals(true);
	int i = 0;
	bool account_found = false;
	AssetsAccount *aaccount = budget->assetsAccounts.first();
	while(aaccount) {
		if(aaccount != budget->balancingAccount && aaccount->accountType() != ASSETS_TYPE_SECURITIES) {
			accountCombo->addItem(aaccount->name());
			if(aaccount == account) {
				accountCombo->setCurrentIndex(i);
				account_found = true;
			}
			i++;
		}
		aaccount = budget->assetsAccounts.next();
	}
	if(!account_found) {
		if(accountCombo->count() == 0) {
			close();
			return;
		}
		accountChanged(0);
	} else {
		updateTransactions();
	}
	accountCombo->blockSignals(false);
}
void LedgerDialog::saveView() {
	if(transactionsView->topLevelItemCount() == 0) {
		QMessageBox::critical(this, tr("Error"), tr("Empty transaction list."));
		return;
	}
	char filetype = 'h';
	QMimeDatabase db;
	QString html_filter = db.mimeTypeForName("text/html").filterString();
	QString csv_filter = db.mimeTypeForName("text/csv").filterString();
	QString filter = html_filter;
	QUrl url = QFileDialog::getSaveFileUrl(this, QString(), QUrl(), html_filter + ";;" + csv_filter, &filter);
	if(filter == csv_filter) filetype = 'c';
	if(url.isEmpty() || !url.isValid()) return;
	QSaveFile ofile(url.toLocalFile());
	ofile.open(QIODevice::WriteOnly);
	ofile.setPermissions((QFile::Permissions) 0x0660);
	if(!ofile.isOpen()) {
		ofile.cancelWriting();
		QMessageBox::critical(this, tr("Error"), tr("Couldn't open file for writing."));
		return;
	}
	QTextStream stream(&ofile);
	exportList(stream, filetype);
	if(!ofile.commit()) {
		QMessageBox::critical(this, tr("Error"), tr("Error while writing file; file was not saved."));
		return;
	}

}
bool LedgerDialog::exportList(QTextStream &outf, int fileformat) {

	switch(fileformat) {
		case 'h': {
			outf.setCodec("UTF-8");
			outf << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">" << '\n';
			outf << "<html>" << '\n';
			outf << "\t<head>" << '\n';
			outf << "\t\t<title>"; outf << htmlize_string(tr("Ledger")); outf << "</title>" << '\n';
			outf << "\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" << '\n';
			outf << "\t\t<meta name=\"GENERATOR\" content=\"" << qApp->applicationDisplayName() << "\">" << '\n';
			outf << "\t</head>" << '\n';
			outf << "\t<body>" << '\n';
			outf << "\t\t<table border=\"1\" cellspacing=\"0\" cellpadding=\"5\">" << '\n';
			outf << "\t\t\t<caption>"; outf << htmlize_string(tr("Transactions for %1").arg(account->name())); outf << "</caption>" << '\n';
			outf << "\t\t\t<thead>" << '\n';
			outf << "\t\t\t\t<tr>" << '\n';
			QTreeWidgetItem *header = transactionsView->headerItem();
			outf << "\t\t\t\t\t<th>" << htmlize_string(header->text(0)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(1)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(2)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(3)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(4)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(5)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(6)) << "</th>" << "\n";
			outf << "\t\t\t\t</tr>" << '\n';
			outf << "\t\t\t</thead>" << '\n';
			outf << "\t\t\t<tbody>" << '\n';
			QTreeWidgetItemIterator it(transactionsView);
			LedgerListViewItem *i = (LedgerListViewItem*) *it;
			while(i) {
				outf << "\t\t\t\t<tr>" << '\n';
				outf << "\t\t\t\t\t<td nowrap align=\"right\">" << htmlize_string(i->text(0)) << "</td>";
				outf << "<td>" << htmlize_string(i->text(1)) << "</td>";
				outf << "<td>" << htmlize_string(i->text(2)) << "</td>";
				outf << "<td>" << htmlize_string(i->text(3)) << "</td>";
				outf << "<td nowrap align=\"right\">" << htmlize_string(i->text(4)) << "</td>";
				outf << "<td nowrap align=\"right\">" << htmlize_string(i->text(5)) << "</td>";
				outf << "<td nowrap align=\"right\">" << htmlize_string(i->text(6)) << "</td>" << "\n";
				outf << "\t\t\t\t</tr>" << '\n';
				++it;
				i = (LedgerListViewItem*) *it;
			}
			outf << "\t\t\t</tbody>" << '\n';
			outf << "\t\t</table>" << '\n';
			outf << "\t</body>" << '\n';
			outf << "</html>" << '\n';
			break;
		}
		case 'c': {
			//outf.setEncoding(Q3TextStream::Locale);
			QTreeWidgetItem *header = transactionsView->headerItem();
			outf << "\"" << header->text(0) << "\",\"" << header->text(1) << "\",\"" << header->text(2) << "\",\"" << header->text(3) << "\",\"" << header->text(4) << "\",\""<< header->text(5) << "\",\"" << header->text(6) << "\"\n";
			QTreeWidgetItemIterator it(transactionsView);
			LedgerListViewItem *i = (LedgerListViewItem*) *it;
			while(i) {
				outf << "\"" << i->text(0) << "\",\"" << i->text(1) << "\",\"" << i->text(2) << "\",\"" << i->text(3) << "\",\"" << i->text(4) << "\",\"" << i->text(5) << "\",\"" << i->text(6) << "\"\n";
				++it;
				i = (LedgerListViewItem*) *it;
			}
			break;
		}
	}

	return true;

}
void LedgerDialog::printView() {
	if(transactionsView->topLevelItemCount() == 0) {
		QMessageBox::critical(this, tr("Error"), tr("Empty transaction list."));
		return;
	}
	QPrinter printer;
	QPrintDialog print_dialog(&printer, this);
	if(print_dialog.exec() == QDialog::Accepted) {
		QString str;
		QTextStream stream(&str, QIODevice::WriteOnly);
		exportList(stream, 'h');
		QTextDocument htmldoc;
		htmldoc.setHtml(str);
		htmldoc.print(&printer);
	}
}
void LedgerDialog::joinTransactions() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	SplitTransaction *split = NULL;
	for(int index = 0; index < selection.size(); index++) {
		LedgerListViewItem *i = (LedgerListViewItem*) selection[index];
		Transaction *trans = i->transaction();
		if(trans && !i->splitTransaction()) {
			if(!split) {
				split = new SplitTransaction(budget, i->transaction()->date(), account);
			}
			split->splits.push_back(trans);
		}
	}
	if(!split) return;
	if(!mainWin->editSplitTransaction(split, this)) {
		split->clear(true);
		delete split;
	}
}
void LedgerDialog::splitUpTransaction() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	LedgerListViewItem *i = (LedgerListViewItem*) selection.first();
	if(i) {
		if(i->splitTransaction()) mainWin->splitUpTransaction(i->splitTransaction());
		else if(i->transaction() && i->transaction()->parentSplit()) mainWin->splitUpTransaction(i->transaction()->parentSplit());
	}
}
void LedgerDialog::transactionSelectionChanged() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	bool selected = !selection.isEmpty();
	removeButton->setEnabled(selected);
	editButton->setEnabled(selected);
	bool b_join = selected;
	bool b_split = selected;
	SplitTransaction *split = NULL;
	for(int index = 0; index < selection.size(); index++) {
		LedgerListViewItem *i = (LedgerListViewItem*) selection[index];
		Transaction *trans = i->transaction();
		if(!trans) {
			removeButton->setEnabled(false);
			if(selection.count() > 1) editButton->setEnabled(false);
			b_join = false;
			b_split = false;
			break;
		}
		if((b_join || b_split) && i->splitTransaction()) {
			b_join = false;
			b_split = (selection.count() == 1);
		} else if(b_split) {
			if(!split) split = trans->parentSplit();
			if(!trans->parentSplit() || trans->parentSplit() != split) {
				b_split = false;
			}
		}
		if(b_join && trans->parentSplit()) {
			b_join = false;
		}
	}
	joinButton->setEnabled(b_join);
	splitUpButton->setEnabled(b_split);
}
void LedgerDialog::newExpense() {
	mainWin->newScheduledTransaction(TRANSACTION_TYPE_EXPENSE, NULL, false, this, account);
}
void LedgerDialog::newIncome() {
	mainWin->newScheduledTransaction(TRANSACTION_TYPE_INCOME, NULL, false, this, account);
}
void LedgerDialog::newTransfer() {
	mainWin->newScheduledTransaction(TRANSACTION_TYPE_TRANSFER, NULL, false, this, account);
}
void LedgerDialog::newSplit() {
	mainWin->newSplitTransaction(this, account);
}
void LedgerDialog::remove() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() > 1) {
		if(QMessageBox::warning(this, tr("Delete transactions?"), tr("Are you sure you want to delete all (%1) selected transactions?").arg(selection.count()), QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok) {
			return;
		}
	}
	transactionsView->clearSelection();
	for(int index = 0; index < selection.size(); index++) {
		LedgerListViewItem *i = (LedgerListViewItem*) selection[index];
		if(i->splitTransaction()) {
			budget->removeSplitTransaction(i->splitTransaction(), true);
			mainWin->splitTransactionRemoved(i->splitTransaction());
			delete i->splitTransaction();
		} else if(i->transaction()) {
			budget->removeTransaction(i->transaction(), true);
			mainWin->transactionRemoved(i->transaction());
			delete i->transaction();
		}
	}
}
void LedgerDialog::edit() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() == 1) {
		LedgerListViewItem *i = (LedgerListViewItem*) selection.first();
		if(i->splitTransaction()) mainWin->editSplitTransaction(i->splitTransaction(), this);
		else if(!i->transaction()) mainWin->editAccount(account, this);
		else if(i->transaction()->parentSplit()) mainWin->editSplitTransaction(i->transaction()->parentSplit(), this);
		else if(i->transaction()) mainWin->editTransaction(i->transaction(), this);
	} else if(selection.count() > 1) {
		bool warned1 = false, warned2 = false, warned3 = false, warned4 = false;
		bool equal_date = true, equal_description = true, equal_value = true, equal_category = true, equal_payee = b_extra;
		int transtype = -1;
		Transaction *comptrans = NULL;
		Account *compcat = NULL;
		QDate compdate;
		for(int index = 0; index < selection.size(); index++) {
			LedgerListViewItem *i = (LedgerListViewItem*) selection[index];
			if(!comptrans) {
				comptrans = i->transaction();
				compdate = i->transaction()->date();
				transtype = i->transaction()->type();
				if(i->transaction()->parentSplit()) equal_date = false;
				if(i->transaction()->type() != TRANSACTION_TYPE_EXPENSE && i->transaction()->type() != TRANSACTION_TYPE_INCOME) equal_payee = false;
				if(i->transaction()->type() == TRANSACTION_TYPE_INCOME) {
					compcat = ((Income*) i->transaction())->category();
				} else if(i->transaction()->type() == TRANSACTION_TYPE_EXPENSE) {
					compcat = ((Expense*) i->transaction())->category();
				} else if(i->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || i->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL) {
					equal_value = false;
					equal_description = false;
					compcat = ((SecurityTransaction*) i->transaction())->account();
					if(compcat->type() == ACCOUNT_TYPE_ASSETS) {
						equal_category = false;
					}
				}
			} else {
				if(transtype >= 0 && comptrans->type() != transtype) {
					transtype = -1;
				}
				if(equal_date && (compdate != i->transaction()->date() || i->transaction()->parentSplit())) {
					equal_date = false;
				}
				if(equal_description && (i->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || i->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL || comptrans->description() != i->transaction()->description())) {
					equal_description = false;
				}
				if(equal_payee && (i->transaction()->type() != comptrans->type() || (comptrans->type() == TRANSACTION_TYPE_EXPENSE && ((Expense*) comptrans)->payee() != ((Expense*) i->transaction())->payee()) || (comptrans->type() == TRANSACTION_TYPE_INCOME && ((Income*) comptrans)->payer() != ((Income*) i->transaction())->payer()))) {
					equal_payee = false;
				}
				if(equal_value && (i->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || i->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL || comptrans->value() != i->transaction()->value())) {
					equal_value = false;
				}
				if(equal_category) {
					if(i->transaction()->type() == TRANSACTION_TYPE_INCOME) {
						if(compcat != ((Income*) i->transaction())->category()) {
							equal_category = false;
						}
					} else if(i->transaction()->type() == TRANSACTION_TYPE_EXPENSE) {
						if(compcat != ((Expense*) i->transaction())->category()) {
							equal_category = false;
						}
					} else if(i->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || i->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL) {
						if(compcat != ((SecurityTransaction*) i->transaction())->account()) {
							equal_category = false;
						}
					}
				}
			}
		}
		MultipleTransactionsEditDialog *dialog = new MultipleTransactionsEditDialog(b_extra, transtype, budget, this);
		LedgerListViewItem *i = (LedgerListViewItem*) transactionsView->currentItem();
		if(!i->isSelected()) i = (LedgerListViewItem*) selection.first();
		if(i) {
			dialog->setTransaction(i->transaction());
		}
		if(equal_description) dialog->descriptionButton->setChecked(true);
		if(equal_payee && dialog->payeeButton) dialog->payeeButton->setChecked(true);
		if(equal_value && dialog->valueButton) dialog->valueButton->setChecked(true);
		if(equal_date) dialog->dateButton->setChecked(true);
		if(equal_category && dialog->categoryButton) dialog->categoryButton->setChecked(true);
		if(dialog->exec() == QDialog::Accepted) {
			QDate date = dialog->date();
			bool future = !date.isNull() && date > QDate::currentDate();
			for(int index = 0; index < selection.size(); index++) {
				LedgerListViewItem *i = (LedgerListViewItem*) selection[index];
				if(!warned1 && (i->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || i->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL)) {
					if(dialog->valueButton && dialog->valueButton->isChecked()) {
						QMessageBox::critical(this, tr("Error"), tr("Cannot set the value of security transactions using the dialog for modifying multiple transactions."));
						warned1 = true;
					}
				}
				if(!warned2 && (i->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || i->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL || (i->transaction()->type() == TRANSACTION_TYPE_INCOME && ((Income*) i->transaction())->security()))) {
					if(dialog->descriptionButton->isChecked()) {
						QMessageBox::critical(this, tr("Error"), tr("Cannot change description of dividends and security transactions.", "Referring to the generic description property"));
						warned2 = true;
					}
				}
				if(!warned3 && dialog->payeeButton && (i->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || i->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL || (i->transaction()->type() == TRANSACTION_TYPE_INCOME && ((Income*) i->transaction())->security()))) {
					if(dialog->payeeButton->isChecked()) {
						QMessageBox::critical(this, tr("Error"), tr("Cannot change payer of dividends and security transactions."));
						warned3 = true;
					}
				}
				if(!warned4 && i->transaction()->parentSplit()) {
					if(dialog->dateButton->isChecked()) {
						QMessageBox::critical(this, tr("Error"), tr("Cannot change date of transactions that are part of a split transaction."));
						warned4 = true;
					}
				}
				Transaction *trans = i->transaction();
				Transaction *oldtrans = trans->copy();
				if(dialog->modifyTransaction(trans)) {
					if(future && !trans->parentSplit()) {
						budget->removeTransaction(trans, true);
						mainWin->transactionRemoved(trans);
						ScheduledTransaction *strans = new ScheduledTransaction(budget, trans, NULL);
						budget->addScheduledTransaction(strans);
						mainWin->scheduledTransactionAdded(strans);
					} else {
						mainWin->transactionModified(trans, oldtrans);
					}
				}
				delete oldtrans;
			}
		}
		dialog->deleteLater();
	}
}
void LedgerDialog::edit(QTreeWidgetItem*) {
	edit();
}
void LedgerDialog::updateTransactions() {
	/*int contents_x = transactionsView->contentsX();
	int contents_y = transactionsView->contentsY();*/
	int scroll_h = transactionsView->horizontalScrollBar()->value();
	int scroll_v = transactionsView->verticalScrollBar()->value();
	Transaction *selected_transaction = NULL;
	SplitTransaction *selected_split = NULL;
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() == 1) {
		LedgerListViewItem *i = (LedgerListViewItem*) selection.first();
		selected_split = i->splitTransaction();
		if(!selected_split) selected_transaction = i->transaction();
	}
	transactionsView->clear();
	double balance = account->initialBalance();
	if(balance != 0.0) transactionsView->insertTopLevelItem(0, new LedgerListViewItem(NULL, NULL, NULL, "-", "-", "Initial balance", "-", QString::null, QString::null, QLocale().toCurrencyString(balance)));
	Transaction *trans = budget->transactions.first();
	QVector<SplitTransaction*> splits;
	while(trans) {
		SplitTransaction *split_this = trans->parentSplit();
		bool skip = false;
		if(!splits.isEmpty() && split_this) {
			if(splits[splits.size() - 1]->date() != split_this->date()) {
				splits.clear();
			} else if(split_this) {
				for(QVector<SplitTransaction*>::size_type i = 0; i < splits.size(); i++) {
					if(split_this == splits[i]) {
						skip = true;
						break;
					}
				}
			}
		}
		if(!skip && (trans->fromAccount() == account || trans->toAccount() == account)) {
			if(split_this && split_this->account() != account) split_this = NULL;
			if(split_this) splits.push_back(split_this);
			bool deposit = split_this || (trans->toAccount() == account);
			double value = 0.0;
			if(split_this) value = split_this->value();
			else value = trans->value();
			if(deposit) balance += value;
			else balance -= value;
			if(!split_this && value < 0.0) {
				deposit = !deposit;
				value = -value;
			}
			if(split_this) {
				LedgerListViewItem *i = new LedgerListViewItem(trans, split_this, NULL, QLocale().toString(split_this->date(), QLocale::ShortFormat), tr("Split Transaction"), split_this->description(), QString::null, (value >= 0.0) ? QLocale().toCurrencyString(value) : QString::null, (value < 0.0) ? QLocale().toCurrencyString(-value) : QString::null, QLocale().toCurrencyString(balance));
				transactionsView->insertTopLevelItem(0, i);
				if(split_this == selected_split) {
					i->setSelected(true);
				}
			} else {
				LedgerListViewItem *i = new LedgerListViewItem(trans, NULL, NULL, QLocale().toString(trans->date(), QLocale::ShortFormat), QString::null, trans->description(), deposit ? trans->fromAccount()->name() : trans->toAccount()->name(), (deposit && value >= 0.0) ? QLocale().toCurrencyString(value < 0.0 ? -value : value) : QString::null, (deposit && value >= 0.0) ? QString::null : QLocale().toCurrencyString(value < 0.0 ? -value : value), QLocale().toCurrencyString(balance));
				transactionsView->insertTopLevelItem(0, i);
				if(trans->type() == TRANSACTION_TYPE_INCOME) {
					if(value >= 0.0) i->setText(1, tr("Income"));
					else i->setText(1, tr("Repayment"));
				} else if(trans->type() == TRANSACTION_TYPE_EXPENSE) {
					if(value >= 0.0) i->setText(1, tr("Expense"));
					else i->setText(1, tr("Refund"));
				} else if(trans->toAccount() == budget->balancingAccount || trans->fromAccount() == budget->balancingAccount) i->setText(1, tr("Balancing"));
				else i->setText(1, tr("Transfer"));
				if(trans == selected_transaction) {
					i->setSelected(true);
				}
			}
		}
		trans = budget->transactions.next();
	}
	//transactionsView->setContentsPos(contents_x, contents_y);
	transactionsView->horizontalScrollBar()->setValue(scroll_h);
	transactionsView->verticalScrollBar()->setValue(scroll_v);
}
void LedgerDialog::reject() {
	saveConfig();
	QDialog::reject();
}

