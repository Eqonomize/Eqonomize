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

#include <QAction>
#include <QCheckBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QObject>
#include <QPushButton>
#include <QRadioButton>
#include <QTextStream>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QFileDialog>
#include <QUrl>
#include <QTabWidget>
#include <QMessageBox>

#include "budget.h"
#include "editscheduledtransactiondialog.h"
#include "eqonomize.h"
#include "recurrence.h"
#include "transactioneditwidget.h"
#include "transactionfilterwidget.h"
#include "transactionlistwidget.h"

#include <cmath>


extern double monthsBetweenDates(const QDate &date1, const QDate &date2);

class TransactionListViewItem : public QTreeWidgetItem {
	protected:
		Transaction *o_trans;
		ScheduledTransaction *o_strans;
		QDate d_date;
	public:
		TransactionListViewItem(const QDate &trans_date, Transaction *trans, ScheduledTransaction *strans, QString, QString=QString::null, QString=QString::null, QString=QString::null, QString=QString::null, QString=QString::null, QString=QString::null, QString=QString::null);
		bool operator<(const QTreeWidgetItem &i_pre) const;
		Transaction *transaction() const;
		ScheduledTransaction *scheduledTransaction() const;
		const QDate &date() const;
		void setDate(const QDate &newdate);
};

TransactionListWidget::TransactionListWidget(bool extra_parameters, int transaction_type, Budget *budg, Eqonomize *main_win, QWidget *parent) : QWidget(parent), transtype(transaction_type), budget(budg), mainWin(main_win), b_extra(extra_parameters) {

	current_value = 0.0;
	current_quantity = 0.0;

	selected_trans = NULL;

	listPopupMenu = NULL;

	QVBoxLayout *transactionsLayout = new QVBoxLayout(this);
	transactionsLayout->setContentsMargins(0, 0, 0, 0);

	QVBoxLayout *transactionsViewLayout = new QVBoxLayout();
	transactionsLayout->addLayout(transactionsViewLayout);

	transactionsView = new EqonomizeTreeWidget(this);
	transactionsView->setSortingEnabled(true);
	transactionsView->sortByColumn(0, Qt::DescendingOrder);
	transactionsView->setAllColumnsShowFocus(true);
	QStringList headers;
	headers << tr("Date");
	headers << tr("Name");
	comments_col = 5;
	switch(transtype) {
		case TRANSACTION_TYPE_EXPENSE: {
			headers << tr("Cost");
			headers << tr("Category");
			headers << tr("From Account");
			if(b_extra) {
				headers << tr("Payee");
				comments_col = 6;
			}
			from_col = 4; to_col = 3;
			break;
		}
		case TRANSACTION_TYPE_INCOME: {
			headers << tr("Income");
			headers << tr("Category");
			headers << tr("To Account");
			if(b_extra) {
				headers << tr("Payer");
				comments_col = 6;
			}
			from_col = 3; to_col = 4;
			break;
		}
		default: {
			headers << tr("Amount");
			headers << tr("From");
			headers << tr("To");
			from_col = 3; to_col = 4;
			break;
		}
	}
	headers << tr("Comments");
	transactionsView->setColumnCount(comments_col + 1);
	transactionsView->setHeaderLabels(headers);
	transactionsView->setRootIsDecorated(false);
	transactionsView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	transactionsViewLayout->addWidget(transactionsView);
	statLabel = new QLabel(this);
	transactionsViewLayout->addWidget(statLabel);
	QSizePolicy sp = transactionsView->sizePolicy();
	sp.setVerticalPolicy(QSizePolicy::MinimumExpanding);
	transactionsView->setSizePolicy(sp);

	tabs = new QTabWidget(this);
	tabs->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

	editWidget = new TransactionEditWidget(true, b_extra, transtype, false, false, NULL, SECURITY_SHARES_AND_QUOTATION, false, budget, this);
	editInfoLabel = new QLabel(QString::null);
	editWidget->bottomLayout()->addWidget(editInfoLabel);
	QDialogButtonBox *buttons = new QDialogButtonBox();
	editWidget->bottomLayout()->addWidget(buttons);
	addButton = buttons->addButton(tr("Add"), QDialogButtonBox::ActionRole);
	modifyButton = buttons->addButton(tr("Apply"), QDialogButtonBox::ActionRole);
	removeButton = buttons->addButton(tr("Delete"), QDialogButtonBox::ActionRole);
	modifyButton->setEnabled(false);
	removeButton->setEnabled(false);

	filterWidget = new TransactionFilterWidget(b_extra, transtype, budget, this);
	QString editTabTitle;
	switch (transtype) {
		case TRANSACTION_TYPE_EXPENSE: {editTabTitle = tr("New/Edit Expense"); break;}
		case TRANSACTION_TYPE_INCOME: editTabTitle = tr("New/Edit Income"); break;
		case TRANSACTION_TYPE_TRANSFER: editTabTitle = tr("New/Edit Transfer"); break;
	}
	tabs->addTab(editWidget, editTabTitle);
	tabs->addTab(filterWidget, tr("Filter"));
	transactionsLayout->addWidget(tabs);

	updateStatistics();

	connect(addButton, SIGNAL(clicked()), this, SLOT(addTransaction()));
	connect(modifyButton, SIGNAL(clicked()), this, SLOT(modifyTransaction()));
	connect(editWidget, SIGNAL(addmodify()), this, SLOT(addModifyTransaction()));
	connect(removeButton, SIGNAL(clicked()), this, SLOT(removeTransaction()));
	connect(filterWidget, SIGNAL(filter()), this, SLOT(filterTransactions()));
	connect(filterWidget, SIGNAL(toActivated(int)), this, SLOT(filterCategoryActivated(int)));
	connect(filterWidget, SIGNAL(fromActivated(int)), this, SLOT(filterFromActivated(int)));
	connect(transactionsView, SIGNAL(itemSelectionChanged()), this, SLOT(transactionSelectionChanged()));
	transactionsView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(transactionsView, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(popupListMenu(const QPoint&)));

	connect(transactionsView, SIGNAL(doubleClicked(QTreeWidgetItem*, int)), this, SLOT(transactionExecuted(QTreeWidgetItem*)));
	connect(transactionsView, SIGNAL(returnPressed(QTreeWidgetItem*)), this, SLOT(transactionExecuted(QTreeWidgetItem*)));

}

//QSize TransactionListWidget::minimumSizeHint() const {return filterWidget->minimumSizeHint().expandedTo(editWidget->minimumSizeHint());}
//QSize TransactionListWidget::sizeHint() const {return QSize(filterWidget->sizeHint().expandedTo(editWidget->sizeHint()).width() + 12, QWidget::sizeHint().height());}

QSize TransactionListWidget::minimumSizeHint() const {return QWidget::minimumSizeHint();}
QSize TransactionListWidget::sizeHint() const {return minimumSizeHint();}

void TransactionListWidget::currentDateChanged(const QDate &olddate, const QDate &newdate) {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.isEmpty()) editWidget->currentDateChanged(olddate, newdate);
	filterWidget->currentDateChanged(olddate, newdate);
}
void TransactionListWidget::transactionExecuted(QTreeWidgetItem*) {
	editTransaction();
}
void TransactionListWidget::updateStatistics() {
	int i_count_frac = 0;
	double intpart = 0.0;
	if(modf(current_quantity, &intpart) != 0.0) i_count_frac = 2;
	statLabel->setText(QString("<div align=\"right\"><b>%1</b> %5 &nbsp; <b>%2</b> %6 &nbsp; <b>%3</b> %7 &nbsp; <b>%4</b> %8</div>").arg(tr("Quantity:")).arg(tr("Total:")).arg(tr("Average:")).arg(tr("Monthly:")).arg(QLocale().toString(current_quantity, 'f', i_count_frac)).arg(QLocale().toCurrencyString(current_value)).arg(QLocale().toCurrencyString(current_quantity == 0.0 ? 0.0 : current_value / current_quantity)).arg(QLocale().toCurrencyString(current_value == 0.0 ? current_value : current_value / filterWidget->countMonths())));
}

void TransactionListWidget::popupListMenu(const QPoint &p) {
	if(!listPopupMenu) {
		listPopupMenu = new QMenu(this);
		switch(transtype) {
			case TRANSACTION_TYPE_EXPENSE: {listPopupMenu->addAction(mainWin->ActionNewExpense); listPopupMenu->addAction(mainWin->ActionNewRefund); break;}
			case TRANSACTION_TYPE_INCOME: {listPopupMenu->addAction(mainWin->ActionNewIncome); listPopupMenu->addAction(mainWin->ActionNewRepayment); break;}
			case TRANSACTION_TYPE_TRANSFER: {listPopupMenu->addAction(mainWin->ActionNewTransfer); break;}
		}
		listPopupMenu->addAction(mainWin->ActionNewSplitTransaction);
		listPopupMenu->addSeparator();
		listPopupMenu->addAction(mainWin->ActionEditTransaction);
		listPopupMenu->addAction(mainWin->ActionEditScheduledTransaction);
		listPopupMenu->addAction(mainWin->ActionEditSplitTransaction);
		listPopupMenu->addAction(mainWin->ActionJoinTransactions);
		listPopupMenu->addAction(mainWin->ActionSplitUpTransaction);
		listPopupMenu->addSeparator();
		listPopupMenu->addAction(mainWin->ActionDeleteTransaction);
		listPopupMenu->addAction(mainWin->ActionDeleteScheduledTransaction);
		listPopupMenu->addAction(mainWin->ActionDeleteSplitTransaction);
	}
	listPopupMenu->popup(transactionsView->viewport()->mapToGlobal(p));
}

extern QString htmlize_string(QString str);

bool TransactionListWidget::isEmpty() {
	return transactionsView->topLevelItemCount() == 0;
}

bool TransactionListWidget::exportList(QTextStream &outf, int fileformat) {

	switch(fileformat) {
		case 'h': {
			outf.setCodec("UTF-8");
			outf << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">" << '\n';
			outf << "<html>" << '\n';
			outf << "\t<head>" << '\n';
			outf << "\t\t<title>";
			switch(transtype) {
				case TRANSACTION_TYPE_EXPENSE: {outf << htmlize_string(tr("Expenses")); break;}
				case TRANSACTION_TYPE_INCOME: {outf << htmlize_string(tr("Incomes")); break;}
				default: {outf << htmlize_string(tr("Transfers")); break;}
			}
			outf << "</title>" << '\n';
			outf << "\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" << '\n';
			outf << "\t\t<meta name=\"GENERATOR\" content=\"" << qApp->applicationDisplayName() << "\">" << '\n';
			outf << "\t</head>" << '\n';
			outf << "\t<body>" << '\n';
			outf << "\t\t<table border=\"1\" cellspacing=\"0\" cellpadding=\"5\">" << '\n';
			outf << "\t\t\t<caption>";
			switch(transtype) {
				case TRANSACTION_TYPE_EXPENSE: {outf << htmlize_string(tr("Expenses")); break;}
				case TRANSACTION_TYPE_INCOME: {outf << htmlize_string(tr("Incomes")); break;}
				default: {outf << htmlize_string(tr("Transfers")); break;}
			}
			outf << "</caption>" << '\n';
			outf << "\t\t\t<thead>" << '\n';
			outf << "\t\t\t\t<tr>" << '\n';
			QTreeWidgetItem *header = transactionsView->headerItem();
			outf << "\t\t\t\t\t<th>" << htmlize_string(header->text(0)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(1)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(2)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(3)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(4)) << "</th>";
			if(comments_col == 6) outf << "<th>" << htmlize_string(tr("Quantity")) << "</th>";
			outf << "<th>" << htmlize_string(header->text(5)) << "</th>";
			if(comments_col == 6) outf << "<th>" << htmlize_string(header->text(6)) << "</th>";
			outf << "\n";
			outf << "\t\t\t\t</tr>" << '\n';
			outf << "\t\t\t</thead>" << '\n';
			outf << "\t\t\t<tbody>" << '\n';
			QTreeWidgetItemIterator it(transactionsView);
			TransactionListViewItem *i = (TransactionListViewItem*) *it;
			while(i) {
				Transaction *trans = i->transaction();
				outf << "\t\t\t\t<tr>" << '\n';
				outf << "\t\t\t\t\t<td nowrap align=\"right\">" << htmlize_string(QLocale().toString(trans->date(), QLocale::ShortFormat)) << "</td>";
				outf << "<td>" << htmlize_string(trans->description()) << "</td>";
				outf << "<td nowrap align=\"right\">" << htmlize_string(i->text(2)) << "</td>";
				outf << "<td align=\"center\">" << htmlize_string(i->text(3)) << "</td>";
				outf << "<td align=\"center\">" << htmlize_string(i->text(4)) << "</td>";
				int i_count_frac = 0;
				double intpart = 0.0;
				if(modf(trans->quantity(), &intpart) != 0.0) i_count_frac = 2;
				if(comments_col == 6) outf << "<td>" << htmlize_string(QLocale().toString(trans->quantity(), 'f', i_count_frac)) << "</td>";
				outf << "<td>" << htmlize_string(i->text(5)) << "</td>";
				if(comments_col == 6) outf << "<td>" << htmlize_string(i->text(6)) << "</td>";
				outf << "\n";
				outf << "\t\t\t\t</tr>" << '\n';
				++it;
				i = (TransactionListViewItem*) *it;
			}
			outf << "\t\t\t</tbody>" << '\n';
			outf << "\t\t</table>" << '\n';
			outf << "\t\t<div>";
			int i_count_frac = 0;
			double intpart = 0.0;
			if(modf(current_quantity, &intpart) != 0.0) i_count_frac = 2;
			outf << htmlize_string(tr("Quantity:")) << " " << htmlize_string(QLocale().toString(current_quantity, 'f', i_count_frac));
			outf << ", ";
			switch(transtype) {
				case TRANSACTION_TYPE_EXPENSE: {
					outf << htmlize_string(tr("Total cost:")) << " ";
					break;
				}
				case TRANSACTION_TYPE_INCOME: {
					outf << htmlize_string(tr("Total income:")) << " ";
					break;
				}
				default: {
					outf << htmlize_string(tr("Total amount:")) << " ";
					break;
				}
			}
			outf << htmlize_string(QLocale().toCurrencyString(current_value));
			outf << ", ";
			outf << htmlize_string(tr("Average:")) << " " << htmlize_string(QLocale().toCurrencyString(current_value == 0.0 ? current_value : current_value / current_quantity));
			outf << ", ";
			outf << htmlize_string(tr("Monthly average:")) << " " << htmlize_string(QLocale().toCurrencyString(current_value == 0.0 ? current_value : current_value / filterWidget->countMonths()));
			outf << "</div>\n";
			outf << "\t</body>" << '\n';
			outf << "</html>" << '\n';
			break;
		}
		case 'c': {
			//outf.setEncoding(Q3TextStream::Locale);
			QTreeWidgetItem *header = transactionsView->headerItem();
			outf << "\"" << header->text(0) << "\",\"" << header->text(1) << "\",\"" << header->text(2) << "\",\"" << header->text(3) << "\",\"" << header->text(4);
			if(comments_col == 6) outf << "\",\"" << tr("Quantity");
			outf << "\",\"" << header->text(5);
			if(comments_col == 6) outf << "\",\"" << header->text(6);
			outf << "\"\n";
			QTreeWidgetItemIterator it(transactionsView);
			TransactionListViewItem *i = (TransactionListViewItem*) *it;
			while(i) {
				Transaction *trans = i->transaction();
				outf << "\"" << QLocale().toString(trans->date(), QLocale::ShortFormat) << "\",\"" << trans->description() << "\",\"" << i->text(2) << "\",\"" << i->text(3) << "\",\"" << i->text(4);
				if(comments_col == 6) outf << "\",\"" << QLocale().toString(trans->quantity(), 'f', 2);
				outf << "\",\"" << i->text(5);
				if(comments_col == 6) outf << "\",\"" << i->text(6);
				outf << "\"\n";
				++it;
				i = (TransactionListViewItem*) *it;
			}
			break;
		}
	}

	return true;

}

void TransactionListWidget::transactionsReset() {
	filterWidget->transactionsReset();
	editWidget->transactionsReset();
	editClear();
	filterTransactions();
}
void TransactionListWidget::addTransaction() {
	Transaction *trans = editWidget->createTransaction();
	if(!trans) return;
	editClear();
	ScheduledTransaction *strans = NULL;
	if(trans->date() > QDate::currentDate()) {
		strans = new ScheduledTransaction(budget, trans, NULL);
		budget->addScheduledTransaction(strans);
	} else {
		budget->addTransaction(trans);
	}
	if(strans) mainWin->scheduledTransactionAdded(strans);
	else mainWin->transactionAdded(trans);
}
void TransactionListWidget::editScheduledTransaction() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() == 1) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.first();
		if(i->scheduledTransaction()) {
			mainWin->editScheduledTransaction(i->scheduledTransaction());
		}
	}
}
void TransactionListWidget::editSplitTransaction() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() >= 1) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.first();
		if(i->transaction()->parentSplit()) {
			if(mainWin->editSplitTransaction(i->transaction()->parentSplit())) editClear();
		}
	}
}
void TransactionListWidget::editTransaction() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() == 1) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.first();
		if(i->scheduledTransaction()) {
			if(i->scheduledTransaction()->isOneTimeTransaction()) {
				if(mainWin->editScheduledTransaction(i->scheduledTransaction())) editClear();
			} else {
				if(mainWin->editOccurrence(i->scheduledTransaction(), i->date())) transactionSelectionChanged();
			}
		} else {
			if(mainWin->editTransaction(i->transaction())) transactionSelectionChanged();
		}
	} else if(selection.count() > 1) {
		bool warned1 = false, warned2 = false, warned3 = false, warned4 = false;
		MultipleTransactionsEditDialog *dialog = new MultipleTransactionsEditDialog(b_extra, transtype, budget, this);
		TransactionListViewItem *i = (TransactionListViewItem*) transactionsView->currentItem();
		if(!i->isSelected()) i = (TransactionListViewItem*) selection.first();
		if(i) {
			if(i->scheduledTransaction()) dialog->setScheduledTransaction(i->scheduledTransaction(), i->date());
			else dialog->setTransaction(i->transaction());
		}
		bool equal_date = true, equal_description = true, equal_value = true, equal_category = (transtype != TRANSACTION_TYPE_TRANSFER), equal_payee = (dialog->payeeButton != NULL);
		Transaction *comptrans = NULL;
		Account *compcat = NULL;
		QDate compdate;
		for(int index = 0; index < selection.size(); index++) {
			TransactionListViewItem *i = (TransactionListViewItem*) selection.at(index);
			if(!comptrans) {
				comptrans = i->transaction();
				compdate = i->date();
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
				if(equal_date && (compdate != i->date() || i->transaction()->parentSplit())) {
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
		if(equal_description) dialog->descriptionButton->setChecked(true);
		if(equal_payee) dialog->payeeButton->setChecked(true);
		if(equal_value) dialog->valueButton->setChecked(true);
		if(equal_date) dialog->dateButton->setChecked(true);
		if(equal_category && dialog->categoryButton) dialog->categoryButton->setChecked(true);
		if(dialog->exec() == QDialog::Accepted) {
			QDate date = dialog->date();
			bool future = !date.isNull() && date > QDate::currentDate();
			for(int index = 0; index < selection.size(); index++) {
				TransactionListViewItem *i = (TransactionListViewItem*) selection.at(index);
				if(!warned1 && (i->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || i->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL)) {
					if(dialog->valueButton->isChecked()) {
						QMessageBox::critical(this, tr("Error"), tr("Cannot set the value of security transactions using the dialog for modifying multiple transactions."));
						warned1 = true;
					}
				}
				if(!warned2 && (i->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || i->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL || (i->transaction()->type() == TRANSACTION_TYPE_INCOME && ((Income*) i->transaction())->security()))) {
					if(dialog->descriptionButton->isChecked()) {
						QMessageBox::critical(this, tr("Error"), tr("Cannot change description of dividends and security transactions."));
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
				ScheduledTransaction *strans = i->scheduledTransaction();
				if(strans && strans->isOneTimeTransaction() && (date.isNull() || date == strans->transaction()->date())) {
					ScheduledTransaction *old_strans = strans->copy();
					Transaction *trans = strans->transaction();
					if(dialog->modifyTransaction(trans)) {
						mainWin->scheduledTransactionModified(strans, old_strans);
					}
					delete old_strans;
				} else if(strans) {
					date = i->date();
					Transaction *trans = strans->transaction()->copy();
					trans->setDate(date);
					if(dialog->modifyTransaction(trans)) {
						if(future) {
							ScheduledTransaction *strans_new = new ScheduledTransaction(budget, trans, NULL);
							budget->addScheduledTransaction(strans_new);
							mainWin->scheduledTransactionAdded(strans_new);
						} else {
							budget->addTransaction(trans);
							mainWin->transactionAdded(trans);
						}
						if(strans->isOneTimeTransaction()) {
							budget->removeScheduledTransaction(strans, true);
							mainWin->scheduledTransactionRemoved(strans);
							delete strans;
						} else {
							ScheduledTransaction *old_strans = strans->copy();
							strans->addException(date);
							mainWin->scheduledTransactionModified(strans, old_strans);
							delete old_strans;
						}
					} else {
						delete trans;
					}
				} else {
					Transaction *trans = i->transaction();
					Transaction *oldtrans = trans->copy();
					if(dialog->modifyTransaction(trans)) {
						if(future && !trans->parentSplit()) {
							budget->removeTransaction(trans, true);
							mainWin->transactionRemoved(trans);
							strans = new ScheduledTransaction(budget, trans, NULL);
							budget->addScheduledTransaction(strans);
							mainWin->scheduledTransactionAdded(strans);
						} else {
							mainWin->transactionModified(trans, oldtrans);
						}
					}
					delete oldtrans;
				}
			}
			transactionSelectionChanged();
		}
		dialog->deleteLater();
	}
}
void TransactionListWidget::modifyTransaction() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.isEmpty()) return;
	if(selection.count() > 1) {
		editTransaction();
		return;
	}
	TransactionListViewItem *i = (TransactionListViewItem*) selection.first();
	if(!i) return;
	if(((TransactionListViewItem*) i)->transaction()->parentSplit()) {
		editSplitTransaction();
		return;
	}
	if(((TransactionListViewItem*) i)->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || ((TransactionListViewItem*) i)->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL) {
		editTransaction();
		return;
	}
	if(!editWidget->validValues(true)) return;
	if(i->scheduledTransaction()) {
		if(i->scheduledTransaction()->isOneTimeTransaction()) {
			Transaction *trans = i->scheduledTransaction()->transaction()->copy();
			if(!editWidget->modifyTransaction(trans)) {
				delete trans;
				return;
			}
			mainWin->removeScheduledTransaction(i->scheduledTransaction());
			ScheduledTransaction *strans = NULL;
			if(trans->date() > QDate::currentDate()) {
				strans = new ScheduledTransaction(budget, trans, NULL);
				budget->addScheduledTransaction(strans);
			} else {
				budget->addTransaction(trans);
			}
			if(strans) mainWin->scheduledTransactionAdded(strans);
			else mainWin->transactionAdded(trans);
			QTreeWidgetItemIterator it(transactionsView);
			TransactionListViewItem *i = (TransactionListViewItem*) *it;
			while(i) {
				if(i->transaction() == trans) {
					transactionsView->setCurrentItem(i);
					i->setSelected(true);
					break;
				}
				++it;
				i = (TransactionListViewItem*) *it;
			}
			return;
		} else {
			if(editWidget->validValues()) {
				ScheduledTransaction *curstranscopy = i->scheduledTransaction();
				QDate curdate_copy = i->date();
				ScheduledTransaction *oldstrans = curstranscopy->copy();
				curstranscopy->addException(curdate_copy);
				mainWin->scheduledTransactionModified(curstranscopy, oldstrans);
				delete oldstrans;
			}
			return;
		}
	}
	if(editWidget->date() > QDate::currentDate()) {
		Transaction *newtrans = i->transaction()->copy();
		if(editWidget->modifyTransaction(newtrans)) {
			ScheduledTransaction *strans = new ScheduledTransaction(budget, newtrans, NULL);
			removeTransaction();
			budget->addScheduledTransaction(strans);
			mainWin->scheduledTransactionAdded(strans);
			QTreeWidgetItemIterator it(transactionsView);
			TransactionListViewItem *i = (TransactionListViewItem*) *it;
			while(i) {
				if(i->scheduledTransaction() == strans) {
					transactionsView->setCurrentItem(i);
					i->setSelected(true);
					break;
				}
				++it;
				i = (TransactionListViewItem*) *it;
			}
		} else {
			delete newtrans;
		}
	} else {
		Transaction *oldtrans = i->transaction()->copy();
		if(editWidget->modifyTransaction(i->transaction())) {
			mainWin->transactionModified(i->transaction(), oldtrans);
		}
		delete oldtrans;
	}
}
void TransactionListWidget::removeScheduledTransaction() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() == 1) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.first();
		if(i->scheduledTransaction()) {
			mainWin->removeScheduledTransaction(i->scheduledTransaction());
			editClear();
		}
	}
}
void TransactionListWidget::removeSplitTransaction() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() >= 1) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.first();
		if(i->transaction()->parentSplit()) {
			SplitTransaction *split = i->transaction()->parentSplit();
			if(QMessageBox::warning(this, tr("Delete transactions?"), tr("Are you sure you want to delete all (%1) transactions in the selected split transaction?").arg(split->splits.count()), QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel) {
				return;
			}
			budget->removeSplitTransaction(split, true);
			mainWin->splitTransactionRemoved(split);
			delete split;
			editClear();
		}
	}
}
void TransactionListWidget::splitUpTransaction() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() >= 1) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.first();
		if(i->transaction()->parentSplit()) {
			mainWin->splitUpTransaction(i->transaction()->parentSplit());
			transactionSelectionChanged();
		}
	}
}
void TransactionListWidget::joinTransactions() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	SplitTransaction *split = NULL;
	for(int index = 0; index < selection.size(); index++) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.at(index);
		Transaction *trans = i->transaction();
		if(trans && (!i->scheduledTransaction() && !trans->parentSplit())) {
			if(!split) {
				if((trans->type() == TRANSACTION_TYPE_SECURITY_BUY || trans->type() == TRANSACTION_TYPE_SECURITY_SELL) && ((SecurityTransaction*) trans)->account()->type() == ACCOUNT_TYPE_ASSETS) split = new SplitTransaction(budget, i->transaction()->date(), (AssetsAccount*) ((SecurityTransaction*) trans)->account());
				else if(trans->fromAccount()->type() == ACCOUNT_TYPE_ASSETS) split = new SplitTransaction(budget, i->transaction()->date(), (AssetsAccount*) trans->fromAccount());
				else split = new SplitTransaction(budget, i->transaction()->date(), (AssetsAccount*) trans->toAccount());
			}
			split->splits.push_back(trans);
		}
	}
	if(!split) return;
	if(mainWin->editSplitTransaction(split)) {
		editClear();
	} else {
		split->clear(true);
		delete split;
	}
}
void TransactionListWidget::removeTransaction() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() > 1) {
		if(QMessageBox::warning(this, tr("Delete transactions?"), tr("Are you sure you want to delete all (%1) selected transactions?").arg(selection.count()), QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel) {
			return;
		}
	}
	transactionsView->clearSelection();
	for(int index = 0; index < selection.size(); index++) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.at(index);
		if(i->scheduledTransaction()) {
			ScheduledTransaction *strans = i->scheduledTransaction();
			if(strans->isOneTimeTransaction()) {
				budget->removeScheduledTransaction(strans, true);
				mainWin->scheduledTransactionRemoved(strans);
				delete strans;
			} else {
				ScheduledTransaction *oldstrans = strans->copy();
				strans->addException(i->date());
				mainWin->scheduledTransactionModified(strans, oldstrans);
				delete oldstrans;
			}
		} else {
			Transaction *trans = i->transaction();
			budget->removeTransaction(trans, true);
			mainWin->transactionRemoved(trans);
			delete trans;
		}
	}
}
void TransactionListWidget::addModifyTransaction() {
	addTransaction();
}

void TransactionListWidget::appendFilterTransaction(Transaction *trans, bool update_total_value) {
	if(!filterWidget->filterTransaction(trans)) {
		QTreeWidgetItem *i = new TransactionListViewItem(trans->date(), trans, NULL, QLocale().toString(trans->date(), QLocale::ShortFormat), QString::null, QLocale().toCurrencyString(trans->value()));
		transactionsView->insertTopLevelItem(0, i);
		i->setTextAlignment(2, Qt::AlignRight);
		i->setTextAlignment(3, Qt::AlignCenter);
		i->setTextAlignment(4, Qt::AlignCenter);
		if(trans == selected_trans) {
			transactionsView->blockSignals(true);
			i->setSelected(true);
			transactionsView->blockSignals(false);
		}
		if(trans->parentSplit()) i->setText(1, trans->description() + "*");
		else i->setText(1, trans->description());
		i->setText(from_col, trans->fromAccount()->name());
		i->setText(to_col, trans->toAccount()->name());
		if(comments_col == 6 && trans->type() == TRANSACTION_TYPE_EXPENSE) i->setText(5, ((Expense*) trans)->payee());
		else if(comments_col == 6 && trans->type() == TRANSACTION_TYPE_INCOME) i->setText(5, ((Income*) trans)->payer());
		i->setText(comments_col, trans->comment());
		current_value += trans->value();
		current_quantity += trans->quantity();
		if(update_total_value) {
			updateStatistics();
			transactionsView->setSortingEnabled(true);
		}
	}
}
void TransactionListWidget::appendFilterScheduledTransaction(ScheduledTransaction *strans, bool update_total_value) {
	if(!filterWidget->filterTransaction(strans->transaction(), false)) {
		QDate date = filterWidget->startDate();
		QDate enddate = filterWidget->endDate();
		if(strans->isOneTimeTransaction()) {
			if(date.isNull()) date = strans->firstOccurrence();
			else if(date > strans->firstOccurrence()) date = QDate();
			else date = strans->firstOccurrence();
		} else {
			if(date.isNull()) date = strans->recurrence()->firstOccurrence();
			else date = strans->recurrence()->nextOccurrence(date, true);
		}
		if(date.isNull() || date > enddate) update_total_value = false;
		Transaction *trans = strans->transaction();
		while(!date.isNull() && date <= enddate) {
			QTreeWidgetItem *i = NULL;
			i = new TransactionListViewItem(date, trans, strans, QString::null, trans->description(), QLocale().toCurrencyString(trans->value()));
			transactionsView->insertTopLevelItem(0, i);
			i->setTextAlignment(2, Qt::AlignRight);
			i->setTextAlignment(3, Qt::AlignCenter);
			i->setTextAlignment(4, Qt::AlignCenter);
			if(strans->recurrence()) i->setText(0, QLocale().toString(date, QLocale::ShortFormat) + "**");
			else i->setText(0, QLocale().toString(date, QLocale::ShortFormat));
			i->setText(from_col, trans->fromAccount()->name());
			i->setText(to_col, trans->toAccount()->name());
			if(comments_col == 6 && trans->type() == TRANSACTION_TYPE_EXPENSE) i->setText(5, ((Expense*) trans)->payee());
			else if(comments_col == 6 && trans->type() == TRANSACTION_TYPE_INCOME) i->setText(5, ((Income*) trans)->payer());
			i->setText(comments_col, trans->comment());
			current_value += trans->value();
			current_quantity += trans->quantity();
			if(!strans->isOneTimeTransaction()) date = strans->recurrence()->nextOccurrence(date);
			else break;
		}
		if(update_total_value) {
			updateStatistics();
			transactionsView->setSortingEnabled(true);
		}
	}
}

void TransactionListWidget::onSplitRemoved(SplitTransaction *split) {
	QTreeWidgetItemIterator it(transactionsView);
	TransactionListViewItem *i = (TransactionListViewItem*) *it;
	while(i) {
		if(i->transaction()->parentSplit() == split) {
			i->setText(1, i->transaction()->description());
		}
		++it;
		i = (TransactionListViewItem*) *it;
	}
}
void TransactionListWidget::onTransactionAdded(Transaction *trans) {
	appendFilterTransaction(trans, true);
	filterWidget->transactionAdded(trans);
	editWidget->transactionAdded(trans);
}
void TransactionListWidget::onTransactionModified(Transaction *trans, Transaction *oldtrans) {
	current_value -= oldtrans->value();
	current_quantity -= oldtrans->quantity();
	bool b_filter = filterWidget->filterTransaction(trans);
	if(!b_filter) {
		current_value += trans->value();
		current_quantity += trans->quantity();
	}
	QTreeWidgetItemIterator it(transactionsView);
	TransactionListViewItem *i = (TransactionListViewItem*) *it;
	while(i) {
		if(i->transaction() == trans) {
			break;
		}
		++it;
		i = (TransactionListViewItem*) *it;
	}
	if(i && b_filter) {
		delete i;
		i = NULL;
	}
	updateStatistics();
	if(i) {
		i->setDate(trans->date());
		i->setText(0, QLocale().toString(trans->date(), QLocale::ShortFormat));
		if(trans->parentSplit()) i->setText(1, trans->description() + "*");
		else i->setText(1, trans->description());
		i->setText(2, QLocale().toCurrencyString(trans->value()));
		i->setText(from_col, trans->fromAccount()->name());
		i->setText(to_col, trans->toAccount()->name());
		if(comments_col == 6 && trans->type() == TRANSACTION_TYPE_EXPENSE) i->setText(5, ((Expense*) trans)->payee());
		else if(comments_col == 6 && trans->type() == TRANSACTION_TYPE_INCOME) i->setText(5, ((Income*) trans)->payer());
		i->setText(comments_col, trans->comment());
	}
	if(oldtrans->description() != trans->description()) filterWidget->transactionModified(trans);
	editWidget->transactionModified(trans);
}
void TransactionListWidget::onTransactionRemoved(Transaction *trans) {
	QTreeWidgetItemIterator it(transactionsView);
	TransactionListViewItem *i = (TransactionListViewItem*) *it;
	while(i) {
		if(i->transaction() == trans) {
			delete i;
			current_value -= trans->value();
			current_quantity -= trans->quantity();
			updateStatistics();
			break;
		}
		++it;
		i = (TransactionListViewItem*) *it;
	}
	editWidget->transactionRemoved(trans);
}
void TransactionListWidget::onScheduledTransactionAdded(ScheduledTransaction *strans) {
	appendFilterScheduledTransaction(strans, true);
	filterWidget->transactionAdded(strans->transaction());
	editWidget->transactionAdded(strans->transaction());
}
void TransactionListWidget::onScheduledTransactionModified(ScheduledTransaction *strans, ScheduledTransaction *oldstrans) {
	QTreeWidgetItemIterator it(transactionsView);
	TransactionListViewItem *i = (TransactionListViewItem*) *it;
	while(i) {
		if(i->scheduledTransaction() == strans) {
			current_value -= oldstrans->transaction()->value();
			current_quantity -= oldstrans->transaction()->quantity();
			QTreeWidgetItem *i_del = i;
			++it;
			i = (TransactionListViewItem*) *it;
			delete i_del;
		} else {
			++it;
			i = (TransactionListViewItem*) *it;
		}
	}
	appendFilterScheduledTransaction(strans, true);
	updateStatistics();
	if(oldstrans->transaction()->description() != strans->transaction()->description()) filterWidget->transactionModified(strans->transaction());
	editWidget->transactionModified(strans->transaction());
}
void TransactionListWidget::editClear() {
	transactionsView->clearSelection();
	editInfoLabel->setText(QString::null);
	editWidget->setTransaction(NULL);
}
void TransactionListWidget::onScheduledTransactionRemoved(ScheduledTransaction *strans) {
	QTreeWidgetItemIterator it(transactionsView);
	TransactionListViewItem *i = (TransactionListViewItem*) *it;
	while(i) {
		if(i->scheduledTransaction() == strans) {
			current_value -= strans->transaction()->value();
			current_quantity -= strans->transaction()->quantity();
			QTreeWidgetItem *i_del = i;
			++it;
			i = (TransactionListViewItem*) *it;
			delete i_del;
		} else {
			++it;
			i = (TransactionListViewItem*) *it;
		}
	}
	updateStatistics();
	editWidget->transactionRemoved(strans->transaction());
}
void TransactionListWidget::filterTransactions() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	selected_trans = NULL;
	if(selection.count() == 1) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.first();
		if(!i->scheduledTransaction()) {selected_trans = i->transaction();}
	}
	transactionsView->clear();
	current_value = 0.0;
	current_quantity = 0.0;
	switch(transtype) {
		case TRANSACTION_TYPE_EXPENSE: {
			Expense *expense = budget->expenses.first();
			while(expense) {
				appendFilterTransaction(expense, false);
				expense = budget->expenses.next();
			}
			SecurityTransaction *sectrans = budget->securityTransactions.first();
			while(sectrans) {
				if(sectrans->account()->type() == ACCOUNT_TYPE_EXPENSES) {
					appendFilterTransaction(sectrans, false);
				}
				sectrans = budget->securityTransactions.next();
			}
			ScheduledTransaction *strans = budget->scheduledTransactions.first();
			while(strans) {
				if(strans->transaction()->type() == TRANSACTION_TYPE_EXPENSE || ((strans->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || strans->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL) && ((SecurityTransaction*) strans->transaction())->account()->type() == ACCOUNT_TYPE_EXPENSES)) {
					appendFilterScheduledTransaction(strans, false);
				}
				strans = budget->scheduledTransactions.next();
			}
			break;
		}
		case TRANSACTION_TYPE_INCOME: {
			Income *income = budget->incomes.first();
			while(income) {
				appendFilterTransaction(income, false);
				income = budget->incomes.next();
			}
			SecurityTransaction *sectrans = budget->securityTransactions.first();
			while(sectrans) {
				if(sectrans->account()->type() == ACCOUNT_TYPE_INCOMES) {
					appendFilterTransaction(sectrans, false);
				}
				sectrans = budget->securityTransactions.next();
			}
			ScheduledTransaction *strans = budget->scheduledTransactions.first();
			while(strans) {
				if(strans->transaction()->type() == TRANSACTION_TYPE_INCOME || ((strans->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || strans->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL) && ((SecurityTransaction*) strans->transaction())->account()->type() == ACCOUNT_TYPE_INCOMES)) {
					appendFilterScheduledTransaction(strans, false);
				}
				strans = budget->scheduledTransactions.next();
			}
			break;
		}
		default: {
			Transfer *transfer = budget->transfers.first();
			while(transfer) {
				appendFilterTransaction(transfer, false);
				transfer = budget->transfers.next();
			}
			SecurityTransaction *sectrans = budget->securityTransactions.first();
			while(sectrans) {
				if(sectrans->account()->type() == ACCOUNT_TYPE_ASSETS) {
					appendFilterTransaction(sectrans, false);
				}
				sectrans = budget->securityTransactions.next();
			}
			ScheduledTransaction *strans = budget->scheduledTransactions.first();
			while(strans) {
				if(strans->transaction()->type() == TRANSACTION_TYPE_TRANSFER || ((strans->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || strans->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL) && ((SecurityTransaction*) strans->transaction())->account()->type() == ACCOUNT_TYPE_ASSETS)) {
					appendFilterScheduledTransaction(strans, false);
				}
				strans = budget->scheduledTransactions.next();
			}
			break;
		}
	}
	selected_trans = NULL;
	selection = transactionsView->selectedItems();
	if(selection.count() == 0) {
		editInfoLabel->setText("");
	}
	updateStatistics();
	transactionsView->setSortingEnabled(true);
}

void TransactionListWidget::currentTransactionChanged(QTreeWidgetItem *i) {
	if(i == NULL) {
		editWidget->setTransaction(NULL);
		editInfoLabel->setText(QString::null);
	} else if(((TransactionListViewItem*) i)->transaction()->parentSplit()) {
		editWidget->setTransaction(((TransactionListViewItem*) i)->transaction());
		SplitTransaction *split = ((TransactionListViewItem*) i)->transaction()->parentSplit();
		if(split->description().isEmpty() || split->description().length() > 10) editInfoLabel->setText(tr("* Part of split transaction"));
		else editInfoLabel->setText(tr("* Part of split (%1)").arg(split->description()));
	} else if(((TransactionListViewItem*) i)->scheduledTransaction()) {
		editWidget->setScheduledTransaction(((TransactionListViewItem*) i)->scheduledTransaction(), ((TransactionListViewItem*) i)->date());
		if(((TransactionListViewItem*) i)->scheduledTransaction()->isOneTimeTransaction()) editInfoLabel->setText(QString::null);
		else editInfoLabel->setText(tr("** Recurring (editing occurrance)"));
	} else {
		editWidget->setTransaction(((TransactionListViewItem*) i)->transaction());
		editInfoLabel->setText(QString::null);
	}
}
void TransactionListWidget::transactionSelectionChanged() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() == 0) {
		editInfoLabel->setText(QString::null);
		modifyButton->setEnabled(false);
		removeButton->setEnabled(false);
		currentTransactionChanged(NULL);
	} else {
		QTreeWidgetItem *i = selection.first();
		if(selection.count() > 1) {
			modifyButton->setText(tr("Modify…"));
		} else if(((TransactionListViewItem*) i)->transaction()->parentSplit() || ((TransactionListViewItem*) i)->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || ((TransactionListViewItem*) i)->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL) {
			modifyButton->setText(tr("Edit…"));
		} else {
			modifyButton->setText(tr("Apply"));
		}
		modifyButton->setEnabled(true);
		removeButton->setEnabled(true);
		if(transactionsView->currentItem()->isSelected()) {
			currentTransactionChanged(transactionsView->currentItem());
		} else if(selection.count() == 1) {
			currentTransactionChanged(i);
		}
	}
	updateTransactionActions();
}
void TransactionListWidget::newRefundRepayment() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() == 1) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.first();
		if((i->transaction()->type() == TRANSACTION_TYPE_EXPENSE && i->transaction()->value() > 0.0) || (i->transaction()->type() == TRANSACTION_TYPE_INCOME && i->transaction()->value() > 0.0 && !((Income*) i->transaction())->security())) {
			mainWin->newRefundRepayment(i->transaction());
		}
	}
}
void TransactionListWidget::updateTransactionActions() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	bool b_transaction = false, b_scheduledtransaction = false, b_split = false;
	bool refundable = false, repayable = false;
	if(selection.count() == 1) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.first();
		b_split = i->transaction() && i->transaction()->parentSplit();
		b_scheduledtransaction = !b_split && i->scheduledTransaction() && i->scheduledTransaction()->recurrence();
		b_transaction = !b_scheduledtransaction || !i->scheduledTransaction()->isOneTimeTransaction();
		refundable = (i->transaction()->type() == TRANSACTION_TYPE_EXPENSE && i->transaction()->value() > 0.0);
		repayable = (i->transaction()->type() == TRANSACTION_TYPE_INCOME && i->transaction()->value() > 0.0 && !((Income*) i->transaction())->security());
	} else if(selection.count() > 1) {
		b_transaction = true;
	}
	bool b_join = (selection.count() > 0);
	b_split = b_join;
	for(int index = 0; index < selection.size(); index++) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.at(index);
		Transaction *trans = i->transaction();
		if(i->scheduledTransaction() || trans->parentSplit()) {
			b_join = false;
			break;
		}
	}
	SplitTransaction *split = NULL;
	for(int index = 0; index < selection.size(); index++) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.at(index);
		Transaction *trans = i->transaction();
		if(!split) split = trans->parentSplit();
		if(!trans->parentSplit() || trans->parentSplit() != split) {
			b_split = false;
			break;
		}
	}
	mainWin->ActionNewRefund->setEnabled(refundable);
	mainWin->ActionNewRepayment->setEnabled(repayable);
	mainWin->ActionNewRefundRepayment->setEnabled(refundable || repayable);
	mainWin->ActionEditTransaction->setEnabled(b_transaction);
	mainWin->ActionDeleteTransaction->setEnabled(b_transaction);
	mainWin->ActionEditScheduledTransaction->setEnabled(b_scheduledtransaction);
	mainWin->ActionDeleteScheduledTransaction->setEnabled(b_scheduledtransaction);
	mainWin->ActionEditSplitTransaction->setEnabled(b_split);
	mainWin->ActionDeleteSplitTransaction->setEnabled(b_split);
	mainWin->ActionJoinTransactions->setEnabled(b_join);
	mainWin->ActionSplitUpTransaction->setEnabled(b_split);
}
void TransactionListWidget::filterCategoryActivated(int index) {
	if(index > 0) editWidget->setCurrentToItem(index - 1);
}
void TransactionListWidget::filterFromActivated(int index) {
	if(index > 0) editWidget->setCurrentFromItem(index - 1);
}
void TransactionListWidget::onDisplay() {
	if(tabs->currentWidget() == editWidget) editWidget->focusDescription();
}
void TransactionListWidget::updateFromAccounts() {
	editWidget->updateFromAccounts();
	filterWidget->updateFromAccounts();
}
void TransactionListWidget::updateToAccounts() {
	editWidget->updateToAccounts();
	filterWidget->updateToAccounts();
}
void TransactionListWidget::updateAccounts() {
	editWidget->updateAccounts();
	filterWidget->updateAccounts();
}
void TransactionListWidget::setCurrentEditToItem(int index) {
	editWidget->setCurrentToItem(index);
}
void TransactionListWidget::setCurrentEditFromItem(int index) {
	editWidget->setCurrentFromItem(index);
}
int TransactionListWidget::currentEditToItem() {
	return editWidget->currentToItem();
}
int TransactionListWidget::currentEditFromItem() {
	return editWidget->currentFromItem();
}
void TransactionListWidget::showFilter() {tabs->setCurrentWidget(filterWidget);}
void TransactionListWidget::showEdit() {tabs->setCurrentWidget(editWidget);}
void TransactionListWidget::setFilter(QDate fromdate, QDate todate, double min, double max, Account *from_account, Account *to_account, QString description, QString payee, bool exclude, bool exact_match) {
	filterWidget->setFilter(fromdate, todate, min, max, from_account, to_account, description, payee, exclude, exact_match);
}

TransactionListViewItem::TransactionListViewItem(const QDate &trans_date, Transaction *trans, ScheduledTransaction *strans, QString s1, QString s2, QString s3, QString s4, QString s5, QString s6, QString s7, QString s8) : QTreeWidgetItem(), o_trans(trans), o_strans(strans), d_date(trans_date) {
	setText(0, s1);
	setText(1, s2);
	setText(2, s3);
	setText(3, s4);
	setText(4, s5);
	setText(5, s6);
	setText(6, s7);
	setText(7, s8);
}
bool TransactionListViewItem::operator<(const QTreeWidgetItem &i_pre) const {
	int col = 0;
	if(treeWidget()) col = treeWidget()->sortColumn();
	TransactionListViewItem *i = (TransactionListViewItem*) &i_pre;
	if(col == 0) {
		return d_date < i->date();
	} else if(col == 2) {
		return o_trans->value() < i->transaction()->value();
	}
	return QTreeWidgetItem::operator<(i_pre);
}
Transaction *TransactionListViewItem::transaction() const {
	return o_trans;
}
ScheduledTransaction *TransactionListViewItem::scheduledTransaction() const {
	return o_strans;
}
const QDate &TransactionListViewItem::date() const {
	return d_date;
}
void TransactionListViewItem::setDate(const QDate &newdate) {
	d_date = newdate;
}


