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

#include <QDebug>

#include <cmath>

extern void setColumnTextWidth(QTreeWidget *w, int i, QString str);
extern void setColumnDateWidth(QTreeWidget *w, int i);
extern void setColumnMoneyWidth(QTreeWidget *w, int i, double v = 9999999.99);
extern void setColumnValueWidth(QTreeWidget *w, int i, double v, int d = -1);
extern void setColumnStrlenWidth(QTreeWidget *w, int i, int l);

extern QColor createExpenseColor(QColor base_color);
extern QColor createIncomeColor(QColor base_color);
extern QColor createTransferColor(QColor base_color);

class TransactionListViewItem : public QTreeWidgetItem {
	protected:
		Transaction *o_trans;
		ScheduledTransaction *o_strans;
		MultiAccountTransaction *o_split;
		QDate d_date;
	public:
		TransactionListViewItem(const QDate &trans_date, Transaction *trans, ScheduledTransaction *strans, MultiAccountTransaction *split, QString, QString=QString::null, QString=QString::null, QString=QString::null, QString=QString::null, QString=QString::null, QString=QString::null, QString=QString::null);
		bool operator<(const QTreeWidgetItem &i_pre) const;
		Transaction *transaction() const;
		ScheduledTransaction *scheduledTransaction() const;
		MultiAccountTransaction *splitTransaction() const;
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
	headers << tr("Description", "Transaction description property (transaction title/generic article name)");
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
	setColumnDateWidth(transactionsView, 0);
	setColumnStrlenWidth(transactionsView, 1, 25);
	setColumnMoneyWidth(transactionsView, 2);
	setColumnStrlenWidth(transactionsView, 3, 20);
	setColumnStrlenWidth(transactionsView, 4, 20);
	if(comments_col > 5) setColumnStrlenWidth(transactionsView, 5, 15);
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

	editWidget = new TransactionEditWidget(true, b_extra, transtype, false, false, NULL, SECURITY_SHARES_AND_QUOTATION, false, budget, this, true);	
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

	connect(transactionsView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(transactionExecuted(QTreeWidgetItem*)));
	connect(transactionsView, SIGNAL(returnPressed(QTreeWidgetItem*)), this, SLOT(transactionExecuted(QTreeWidgetItem*)));
	
	connect(editWidget, SIGNAL(accountAdded(Account*)), this, SIGNAL(accountAdded(Account*)));
	connect(editWidget, SIGNAL(newLoanRequested()), this, SLOT(newTransactionWithLoan()));
	connect(editWidget, SIGNAL(multipleAccountsRequested()), this, SLOT(newMultiAccountTransaction()));

}

//QSize TransactionListWidget::minimumSizeHint() const {return filterWidget->minimumSizeHint().expandedTo(editWidget->minimumSizeHint());}
//QSize TransactionListWidget::sizeHint() const {return QSize(filterWidget->sizeHint().expandedTo(editWidget->sizeHint()).width() + 12, QWidget::sizeHint().height());}

QSize TransactionListWidget::minimumSizeHint() const {return QWidget::minimumSizeHint();}
QSize TransactionListWidget::sizeHint() const {return minimumSizeHint();}

QByteArray TransactionListWidget::saveState() {
	return transactionsView->header()->saveState();
}
void TransactionListWidget::restoreState(const QByteArray &state) {
	transactionsView->header()->restoreState(state);
	transactionsView->sortByColumn(0, Qt::DescendingOrder);
}

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
		listPopupMenu->addAction(mainWin->ActionNewMultiItemTransaction);
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
			outf << "\"" << header->text(0) << "\",\"" << header->text(1) << "\",\"" << header->text(2) << " (" << QLocale().currencySymbol() << ")" << "\",\"" << header->text(3) << "\",\"" << header->text(4);
			if(comments_col == 6) outf << "\",\"" << tr("Quantity");
			outf << "\",\"" << header->text(5);
			if(comments_col == 6) outf << "\",\"" << header->text(6);
			outf << "\"\n";
			QTreeWidgetItemIterator it(transactionsView);
			TransactionListViewItem *i = (TransactionListViewItem*) *it;
			while(i) {
				Transaction *trans = i->transaction();
				outf << "\"" << QLocale().toString(trans->date(), QLocale::ShortFormat) << "\",\"" << trans->description() << "\",\"" << QLocale().toString(trans->value(), 'f', MONETARY_DECIMAL_PLACES).replace("−","-").remove(" ") << "\",\"" << ((trans->type() == TRANSACTION_TYPE_EXPENSE) ? trans->toAccount()->nameWithParent() : trans->fromAccount()->nameWithParent()) << "\",\"" << ((trans->type() == TRANSACTION_TYPE_EXPENSE) ? trans->fromAccount()->nameWithParent() : trans->toAccount()->nameWithParent());
				if(comments_col == 6) outf << "\",\"" << QLocale().toString(trans->quantity(), 'f', 2).replace("−","-").remove(" ");
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
	if(strans) mainWin->transactionAdded(strans);
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
		} else if(i->splitTransaction()) {
			if(mainWin->editSplitTransaction(i->splitTransaction())) transactionSelectionChanged();
		} else {
			if(mainWin->editTransaction(i->transaction())) transactionSelectionChanged();
		}
	} else if(selection.count() > 1) {
		budget->setRecordNewAccounts(true);
		bool warned1 = false, warned2 = false, warned3 = false, warned4 = false, warned5 = false;
		MultipleTransactionsEditDialog *dialog = new MultipleTransactionsEditDialog(b_extra, transtype, budget, this, true);
		TransactionListViewItem *i = (TransactionListViewItem*) transactionsView->currentItem();
		if(!i->isSelected()) i = (TransactionListViewItem*) selection.first();
		if(i) {
			if(i->scheduledTransaction()) dialog->setTransaction(i->transaction(), i->date());
			else dialog->setTransaction(i->transaction());
		}
		bool equal_date = true, equal_description = true, equal_value = true, equal_category = (transtype != TRANSACTION_TYPE_TRANSFER), equal_payee = (dialog->payeeButton != NULL);
		Transaction *comptrans = NULL;
		Account *compcat = NULL;
		QDate compdate;
		QString compdesc;
		double compvalue = 0.0;
		for(int index = 0; index < selection.size(); index++) {
			TransactionListViewItem *i = (TransactionListViewItem*) selection.at(index);
			if(!comptrans) {
				if(i->splitTransaction()) {
					comptrans = i->splitTransaction()->at(0);
					compvalue = i->splitTransaction()->value();
					for(int split_i = 1; split_i < i->splitTransaction()->count(); split_i++) {
						Transaction *i_trans = i->splitTransaction()->at(split_i);
						if((comptrans->type() == TRANSACTION_TYPE_EXPENSE && ((Expense*) comptrans)->payee() != ((Expense*) i_trans)->payee()) || (comptrans->type() == TRANSACTION_TYPE_INCOME && ((Income*) comptrans)->payer() != ((Income*) i_trans)->payer())) {
							equal_payee = false;
						}
						if(i_trans->date() != comptrans->date()) {
							equal_date = false;
						}
					}
					compcat = i->splitTransaction()->category();
					compdesc = i->splitTransaction()->description();
				} else {
					comptrans = i->transaction();
					compvalue = comptrans->value();
					if(i->transaction()->parentSplit()) {
						if(i->transaction()->parentSplit()->type() == SPLIT_TRANSACTION_TYPE_LOAN) {
							equal_category = false;							
							equal_description = false;
						}
						equal_payee = false;
						equal_date = false;
					}
					if(i->transaction()->type() != TRANSACTION_TYPE_EXPENSE && i->transaction()->type() != TRANSACTION_TYPE_INCOME) equal_payee = false;
					if(i->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || i->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL) {
						equal_value = false;
						equal_description = false;
						compcat = ((SecurityTransaction*) i->transaction())->account();
						if(compcat->type() == ACCOUNT_TYPE_ASSETS) {
							equal_category = false;
						}
					}
					if(i->transaction()->type() == TRANSACTION_TYPE_INCOME) {
						compcat = ((Income*) i->transaction())->category();
					} else if(i->transaction()->type() == TRANSACTION_TYPE_EXPENSE) {
						compcat = ((Expense*) i->transaction())->category();
					}
					compdesc != i->transaction()->description();
				}
				compdate = i->date();
			} else {
				Transaction *trans = i->transaction();
				if(i->splitTransaction()) {
					trans = i->splitTransaction()->at(0);
					for(int split_i = 0; split_i < i->splitTransaction()->count(); split_i++) {
						Transaction *i_trans = i->splitTransaction()->at(split_i);
						if(equal_payee && ((comptrans->type() == TRANSACTION_TYPE_EXPENSE && ((Expense*) comptrans)->payee() != ((Expense*) i_trans)->payee()) || (comptrans->type() == TRANSACTION_TYPE_INCOME && ((Income*) comptrans)->payer() != ((Income*) i_trans)->payer()))) {
							equal_payee = false;
						}
						if(equal_date && i_trans->date() != comptrans->date()) {
							equal_date = false;
						}
					}
					if(equal_value && i->splitTransaction()->value() != compvalue) equal_value = false;
					if(equal_category && i->splitTransaction()->category() != compcat) equal_category = false;
					if(equal_description && compdesc != i->splitTransaction()->description()) equal_description = false;
				} else {
					if(equal_date && (compdate != i->date() || trans->parentSplit())) {
						equal_date = false;
					}
					if(equal_payee && (trans->parentSplit() || trans->type() != comptrans->type() || (comptrans->type() == TRANSACTION_TYPE_EXPENSE && ((Expense*) comptrans)->payee() != ((Expense*) trans)->payee()) || (comptrans->type() == TRANSACTION_TYPE_INCOME && ((Income*) comptrans)->payer() != ((Income*) trans)->payer()))) {
						equal_payee = false;
					}
					if(equal_value && (trans->type() == TRANSACTION_TYPE_SECURITY_BUY || trans->type() == TRANSACTION_TYPE_SECURITY_SELL || compvalue != trans->value())) {
						equal_value = false;
					}
					if(equal_category) {
						if(trans->type() == TRANSACTION_TYPE_INCOME) {
							if(compcat != ((Income*) trans)->category()) {
								equal_category = false;
							}
						} else if(trans->type() == TRANSACTION_TYPE_EXPENSE) {
							if(compcat != ((Expense*) trans)->category()) {
								equal_category = false;
							}
						} else if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY || trans->type() == TRANSACTION_TYPE_SECURITY_SELL) {
							if(compcat != ((SecurityTransaction*) trans)->account()) {
								equal_category = false;
							}
						}
					}
					if(equal_description && (trans->type() == TRANSACTION_TYPE_SECURITY_BUY || trans->type() == TRANSACTION_TYPE_SECURITY_SELL || compdesc != trans->description())) {
						equal_description = false;
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
			foreach(Account* acc, budget->newAccounts) emit accountAdded(acc);
			budget->newAccounts.clear();
			QDate date = dialog->date();
			bool future = !date.isNull() && date > QDate::currentDate();
			mainWin->startBatchEdit();
			for(int index = 0; index < selection.size(); index++) {
				TransactionListViewItem *i = (TransactionListViewItem*) selection.at(index);
				if(i->scheduledTransaction() && (i->splitTransaction() || (i->transaction() && i->transaction()->parentSplit()))) {
				} else if(i->splitTransaction()) {
					bool b = false;
					SplitTransaction *new_split = i->splitTransaction()->copy();
					for(int split_i = 0; split_i < new_split->count(); split_i++) {
						Transaction *i_trans = new_split->at(split_i);
						b = dialog->modifyTransaction(i_trans, split_i == 0);
					}
					if(b) {
						SplitTransaction *old_split = i->splitTransaction();
						budget->removeSplitTransaction(old_split, true);
						mainWin->transactionRemoved(old_split);
						delete old_split;
						budget->addSplitTransaction(new_split);
						mainWin->transactionAdded(new_split);
					} else {
						delete new_split;
					}
				} else {
					if(!warned1 && (i->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || i->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL)) {
						if(dialog->valueButton->isChecked()) {
							QMessageBox::critical(this, tr("Error"), tr("Cannot set the value of security transactions using the dialog for modifying multiple transactions.", "Financial security (e.g. stock, mutual fund)"));
							warned1 = true;
						}
					}
					if(!warned2 && (i->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || i->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL || (i->transaction()->type() == TRANSACTION_TYPE_INCOME && ((Income*) i->transaction())->security()))) {
						if(dialog->descriptionButton->isChecked()) {
							QMessageBox::critical(this, tr("Error"), tr("Cannot change description of dividends and security transactions.", "Referring to the transaction description property (transaction title/generic article name); Financial security (e.g. stock, mutual fund)"));
							warned2 = true;
						}
					}
					if(!warned3 && dialog->payeeButton && (i->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || i->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL || (i->transaction()->type() == TRANSACTION_TYPE_INCOME && ((Income*) i->transaction())->security()))) {
						if(dialog->payeeButton->isChecked()) {
							QMessageBox::critical(this, tr("Error"), tr("Cannot change payer of dividends and security transactions.", "Financial security (e.g. stock, mutual fund)"));
							warned3 = true;
						}
					}
					if(i->transaction()->parentSplit()) {
						if(i->transaction()->parentSplit()->type() == SPLIT_TRANSACTION_TYPE_LOAN) {
							if(!warned5) {
								if(dialog->dateButton->isChecked() || dialog->descriptionButton->isChecked() || dialog->categoryButton->isChecked() || (dialog->payeeButton && dialog->payeeButton->isChecked())) {
									QMessageBox::critical(this, tr("Error"), tr("Cannot change date, description, expense category or payee of transactions that are part of a debt payment using the dialog for modifying multiple transactions.", "Referring to the transaction description property (transaction title/generic article name)"));
									warned5 = true;
								}
							}
						} else if(!warned4) {
							if(dialog->dateButton->isChecked() || (dialog->payeeButton && dialog->payeeButton->isChecked())) {
								QMessageBox::critical(this, tr("Error"), tr("Cannot change date or payee/payer of transactions that are part of a split transaction."));
								warned4 = true;
							}
						}
					}
					ScheduledTransaction *strans = i->scheduledTransaction();
					if(strans && strans->isOneTimeTransaction() && (date.isNull() || date == strans->transaction()->date())) {
						ScheduledTransaction *old_strans = strans->copy();
						Transaction *trans = (Transaction*) strans->transaction();
						if(dialog->modifyTransaction(trans)) {
							mainWin->transactionModified(strans, old_strans);
						}
						delete old_strans;
					} else if(strans) {
						if(strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SINGLE) {
							date = i->date();
							Transaction *trans = (Transaction*) strans->transaction()->copy();
							trans->setDate(date);
							if(dialog->modifyTransaction(trans)) {
								if(future) {
									ScheduledTransaction *strans_new = new ScheduledTransaction(budget, trans, NULL);
									budget->addScheduledTransaction(strans_new);
									mainWin->transactionAdded(strans_new);
								} else {
									budget->addTransaction(trans);
									mainWin->transactionAdded(trans);
								}
								if(strans->isOneTimeTransaction()) {
									budget->removeScheduledTransaction(strans, true);
									mainWin->transactionRemoved(strans);
									delete strans;
								} else {
									ScheduledTransaction *old_strans = strans->copy();
									strans->addException(date);
									mainWin->transactionModified(strans, old_strans);
									delete old_strans;
								}
							} else {
								delete trans;
							}
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
								mainWin->transactionAdded(strans);
							} else {
								mainWin->transactionModified(trans, oldtrans);
							}
						}
						delete oldtrans;
					}
				}
			}
			mainWin->endBatchEdit();
			transactionSelectionChanged();
		} else {
			foreach(Account* acc, budget->newAccounts) emit accountAdded(acc);
			budget->newAccounts.clear();
		}
		budget->setRecordNewAccounts(false);
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
	if(((TransactionListViewItem*) i)->splitTransaction()) {
		editTransaction();
		return;
	}
	if(((TransactionListViewItem*) i)->transaction()->parentSplit()) {
		editSplitTransaction();
		return;
	}
	if(((TransactionListViewItem*) i)->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || ((TransactionListViewItem*) i)->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL) {
		editTransaction();
		return;
	}
	if(i->scheduledTransaction() && i->scheduledTransaction()->transaction()->generaltype() != GENERAL_TRANSACTION_TYPE_SINGLE) {
		editTransaction();
		return;
	}
	if(!editWidget->validValues(true)) return;
	if(i->scheduledTransaction()) {
		if(i->scheduledTransaction()->isOneTimeTransaction()) {
			Transaction *trans = (Transaction*) i->scheduledTransaction()->transaction()->copy();
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
			if(strans) mainWin->transactionAdded(strans);
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
				mainWin->transactionModified(curstranscopy, oldstrans);
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
			mainWin->transactionAdded(strans);
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
		Transaction *oldtrans = (Transaction*) i->transaction()->copy();
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
		if(!i->scheduledTransaction()) return;
		if(i->transaction() && i->transaction()->parentSplit()) {
			ScheduledTransaction *strans = i->scheduledTransaction();
			Transaction *trans = i->transaction();
			budget->removeScheduledTransaction(i->scheduledTransaction(), true);
			mainWin->transactionRemoved(i->scheduledTransaction());
			SplitTransaction *split = (SplitTransaction*) strans->transaction();
			if(split->count() == 1) {
				delete strans;
			} else if(split->type() != SPLIT_TRANSACTION_TYPE_LOAN && split->count() == 2) {
				split->removeTransaction(trans);
				ScheduledTransaction *strans_new = new ScheduledTransaction(budget, split->at(0)->copy(), strans->recurrence()->copy());
				delete strans;
				budget->addScheduledTransaction(strans_new);
				mainWin->transactionAdded(strans_new);
			} else {
				split->removeTransaction(trans);
				budget->addScheduledTransaction(strans);
				mainWin->transactionAdded(strans);
			}
		} else {
			mainWin->removeScheduledTransaction(i->scheduledTransaction());
			editClear();
		}
	}
}
void TransactionListWidget::removeSplitTransaction() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() >= 1) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.first();
		if(i->splitTransaction() || i->transaction()->parentSplit()) {
			SplitTransaction *split = i->splitTransaction();
			if(!split) split = i->transaction()->parentSplit();
			if(QMessageBox::warning(this, tr("Delete transactions?"), tr("Are you sure you want to delete all (%1) transactions in the selected split transaction?").arg(split->count()), QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel) {
				return;
			}
			budget->removeSplitTransaction(split, true);
			mainWin->transactionRemoved(split);
			delete split;
			editClear();
		}
	}
}
void TransactionListWidget::splitUpTransaction() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() >= 1) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.first();
		if(i->transaction() && i->transaction()->parentSplit()) {
			mainWin->splitUpTransaction(i->transaction()->parentSplit());
			transactionSelectionChanged();
		}
	}
}
void TransactionListWidget::joinTransactions() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	MultiItemTransaction *split = NULL;
	QString payee;
	for(int index = 0; index < selection.size(); index++) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.at(index);
		Transaction *trans = i->transaction();
		if(trans && !i->scheduledTransaction() && !i->splitTransaction() && !trans->parentSplit()) {
			if(!split) {
				if((trans->type() == TRANSACTION_TYPE_SECURITY_BUY || trans->type() == TRANSACTION_TYPE_SECURITY_SELL) && ((SecurityTransaction*) trans)->account()->type() == ACCOUNT_TYPE_ASSETS) split = new MultiItemTransaction(budget, i->transaction()->date(), (AssetsAccount*) ((SecurityTransaction*) trans)->account());
				else if(trans->fromAccount()->type() == ACCOUNT_TYPE_ASSETS) split = new MultiItemTransaction(budget, i->transaction()->date(), (AssetsAccount*) trans->fromAccount());
				else split = new MultiItemTransaction(budget, i->transaction()->date(), (AssetsAccount*) trans->toAccount());
			}
			if(payee.isEmpty()) {
				if(trans->type() == TRANSACTION_TYPE_EXPENSE) payee = ((Expense*) trans)->payee();
				else if(trans->type() == TRANSACTION_TYPE_INCOME) payee = ((Income*) trans)->payer();
			}
			split->addTransaction(trans);
		}
	}
	if(!split) return;
	if(!payee.isEmpty()) split->setPayee(payee);
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
		mainWin->startBatchEdit();
	}
	transactionsView->clearSelection();
	for(int index = 0; index < selection.count(); index++) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.at(index);
		if(i->scheduledTransaction()) {
			ScheduledTransaction *strans = i->scheduledTransaction();
			if(i->splitTransaction()) {
				if(strans->isOneTimeTransaction()) {
					budget->removeScheduledTransaction(strans, true);
					mainWin->transactionRemoved(strans);
					delete strans;
				} else {
					QList<QDate> exception_dates;
					exception_dates << i->date();
					for(int index2 = index; index2 < selection.count(); index2++) {
						TransactionListViewItem *i = (TransactionListViewItem*) selection.at(index);
						if(i->scheduledTransaction() == strans) {
							exception_dates << i->date();
						}
					}
					mainWin->transactionRemoved(strans);
					for(int date_index = 0; date_index < exception_dates.count(); date_index++) {
						if(strans->isOneTimeTransaction()) {
							budget->removeScheduledTransaction(strans, true);
							delete strans;
							strans = NULL;
							break;
						}
						strans->addException(exception_dates[date_index]);
					}
					if(strans) mainWin->transactionAdded(strans);
				}
			} else if(i->transaction() && i->transaction()->parentSplit()) {
				if(index == 0 && selection.count() == 1) {
					QDate date = i->date();
					SplitTransaction *split = (SplitTransaction*) strans->transaction()->copy();
					split->setDate(date);
					for(int split_i = 0; split_i < split->count(); split_i++) {
						if(((SplitTransaction*) strans->transaction())->at(split_i) == i->transaction()) {
							split->removeTransaction(split->at(split_i));
							break;
						}
					}
					ScheduledTransaction *strans_new = new ScheduledTransaction(budget, split, NULL);
					mainWin->transactionAdded(strans_new);
					if(strans->isOneTimeTransaction()) {
						mainWin->transactionRemoved(strans);
						budget->removeScheduledTransaction(strans, true);
						delete strans;
					} else {
						mainWin->transactionRemoved(strans);
						strans->addException(date);
						mainWin->transactionAdded(strans);
					}
				}
			} else {
				if(strans->isOneTimeTransaction()) {
					budget->removeScheduledTransaction(strans, true);
					mainWin->transactionRemoved(strans);
					delete strans;
				} else {
					ScheduledTransaction *oldstrans = strans->copy();
					strans->addException(i->date());
					mainWin->transactionModified(strans, oldstrans);
					delete oldstrans;
				}
			}
		} else if(i->splitTransaction()) {
			MultiAccountTransaction *split = i->splitTransaction();
			budget->removeSplitTransaction(split, true);
			mainWin->transactionRemoved(split);
			delete split;
		} else {		
			Transaction *trans = i->transaction();
			if(trans->parentSplit() && trans->parentSplit()->count() == 1) {
				SplitTransaction *split = trans->parentSplit();
				budget->removeSplitTransaction(split, true);
				mainWin->transactionRemoved(split);
				delete split;
			} else if(trans->parentSplit() && trans->parentSplit()->type() != SPLIT_TRANSACTION_TYPE_LOAN && trans->parentSplit()->count() == 2) {
				SplitTransaction *split = trans->parentSplit();
				mainWin->splitUpTransaction(split);
				budget->removeTransaction(trans, true);
				mainWin->transactionRemoved(trans);
				delete trans;
			} else {
				budget->removeTransaction(trans, true);
				mainWin->transactionRemoved(trans);
				delete trans;
			}
		}
	}
	mainWin->endBatchEdit();
}
void TransactionListWidget::addModifyTransaction() {
	addTransaction();
}

void TransactionListWidget::appendFilterTransaction(Transactions *transs, bool update_total_value, ScheduledTransaction *strans) {
	Transaction *trans = NULL;
	MultiAccountTransaction *split = NULL;
	switch(transs->generaltype()) {
		case GENERAL_TRANSACTION_TYPE_SINGLE: {
			if(filterWidget->filterTransaction(transs, !strans)) return;
			trans = (Transaction*) transs;
			break;
		}
		case GENERAL_TRANSACTION_TYPE_SPLIT: {
			if(((SplitTransaction*) transs)->type() != SPLIT_TRANSACTION_TYPE_MULTIPLE_ACCOUNTS) return;
			if(filterWidget->filterTransaction(transs, !strans)) return;
			split = (MultiAccountTransaction*) transs;
			break;
		}
		case GENERAL_TRANSACTION_TYPE_SCHEDULE: {
			strans = (ScheduledTransaction*) transs;
			transs = strans->transaction();
			if(transs->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT) {
				if(((SplitTransaction*) transs)->type() != SPLIT_TRANSACTION_TYPE_MULTIPLE_ACCOUNTS) {
					SplitTransaction *split2 = (SplitTransaction*) transs;
					int c = split2->count();
					for(int i = 0; i < c; i++) {
						appendFilterTransaction(split2->at(i), update_total_value, strans);
					}
					return;
				}
				split = (MultiAccountTransaction*) transs;
			} else {
				trans = (Transaction*) transs;
			}
			if(filterWidget->filterTransaction(transs, false)) return;
			break;
		}
	}
	QDate date = filterWidget->startDate();
	QDate enddate = filterWidget->endDate();
	if(strans) {
		if(strans->isOneTimeTransaction()) {
			if(date.isNull()) date = strans->firstOccurrence();
			else if(date > strans->firstOccurrence()) date = QDate();
			else date = strans->firstOccurrence();
		} else {
			if(date.isNull()) date = strans->recurrence()->firstOccurrence();
			else date = strans->recurrence()->nextOccurrence(date, true);
		}
		if(date.isNull() || date > enddate) update_total_value = false;
	} else {
		date = transs->date();
	}
	while(!strans || (!date.isNull() && date <= enddate)) {

		QTreeWidgetItem *i = new TransactionListViewItem(date, trans, strans, split, QString::null, QString::null, QLocale().toCurrencyString(transs->value()));
		
		if(strans && strans->recurrence()) i->setText(0, QLocale().toString(date, QLocale::ShortFormat) + "**");
		else i->setText(0, QLocale().toString(date, QLocale::ShortFormat));
		transactionsView->insertTopLevelItem(0, i);
		i->setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);
		
		if((split && split->cost() > 0.0) || (trans && ((trans->type() == TRANSACTION_TYPE_EXPENSE && trans->value() > 0.0) || (trans->type() == TRANSACTION_TYPE_INCOME && trans->value() < 0.0)))) {
			if(!expenseColor.isValid()) expenseColor = createExpenseColor(i->foreground(2).color());
			i->setForeground(2, expenseColor);
		} else if((split && split->cost() < 0.0) || (trans && ((trans->type() == TRANSACTION_TYPE_EXPENSE && trans->value() < 0.0) || (trans->type() == TRANSACTION_TYPE_INCOME && trans->value() > 0.0)))) {
			if(!incomeColor.isValid()) incomeColor = createIncomeColor(i->foreground(2).color());
			i->setForeground(2, incomeColor);
		} else {
			if(!transferColor.isValid()) transferColor = createTransferColor(i->foreground(2).color());
			i->setForeground(2, transferColor);
		}
		//i->setTextAlignment(3, Qt::AlignCenter);
		//i->setTextAlignment(4, Qt::AlignCenter);
		if((trans && trans == selected_trans) || (split && split == selected_trans)) {
			transactionsView->blockSignals(true);
			i->setSelected(true);
			transactionsView->blockSignals(false);
		}
		if(trans && trans->parentSplit()) i->setText(1, trans->description() + "*");
		else i->setText(1, transs->description());
		if(trans) {
			i->setText(from_col, trans->fromAccount()->name());
			i->setText(to_col, trans->toAccount()->name());
			if(comments_col == 6 && trans->type() == TRANSACTION_TYPE_EXPENSE) i->setText(5, ((Expense*) trans)->payee());
			else if(comments_col == 6 && trans->type() == TRANSACTION_TYPE_INCOME) i->setText(5, ((Income*) trans)->payer());
		} else if(split) {
			i->setText(3, split->category()->name());
			i->setText(4, split->accountsString());
			if(comments_col == 6) i->setText(5, split->payees());
		}
		i->setText(comments_col, transs->comment());
		current_value += transs->value();
		current_quantity += transs->quantity();
		if(strans && !strans->isOneTimeTransaction()) date = strans->recurrence()->nextOccurrence(date);
		else break;
	}
	if(update_total_value) {
		updateStatistics();
		transactionsView->setSortingEnabled(true);
	}
}

void TransactionListWidget::onTransactionSplitUp(SplitTransaction *split) {
	if(split->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ACCOUNTS) {
		for(int i = 0; i < split->count(); i++) {
			split->at(i)->setParentSplit(NULL);
			appendFilterTransaction(split->at(i), false);
			split->at(i)->setParentSplit(split);
			updateStatistics();
			transactionsView->setSortingEnabled(true);
		}
	} else {
		QTreeWidgetItemIterator it(transactionsView);
		TransactionListViewItem *i = (TransactionListViewItem*) *it;
		while(i) {
			if(i->transaction()->parentSplit() == split) {
				i->setText(1, i->transaction()->description());
			}
			++it;
			i = (TransactionListViewItem*) *it;
		}
		transactionSelectionChanged();
	}
}
void TransactionListWidget::onTransactionAdded(Transactions *trans) {
	appendFilterTransaction(trans, true);
	switch(trans->generaltype()) {
		case GENERAL_TRANSACTION_TYPE_SINGLE: {
			filterWidget->transactionAdded((Transaction*) trans);
			editWidget->transactionAdded((Transaction*) trans);
			break;
		}
		case GENERAL_TRANSACTION_TYPE_SCHEDULE: {
			ScheduledTransaction *strans = (ScheduledTransaction*) trans;
			if(strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SINGLE) {
				editWidget->transactionAdded((Transaction*) strans->transaction());
				filterWidget->transactionAdded((Transaction*) strans->transaction());
			} else if(strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT) {
				SplitTransaction *split = (SplitTransaction*) strans->transaction();
				int n = split->count();
				for(int split_i = 0; split_i < n; split_i++) {
					editWidget->transactionAdded(split->at(split_i));
					filterWidget->transactionAdded(split->at(split_i));
				}
			}
			break;
		}
		case GENERAL_TRANSACTION_TYPE_SPLIT: {
			break;
		}
	}
}
void TransactionListWidget::onTransactionModified(Transactions *transs, Transactions *oldtranss) {
	switch(transs->generaltype()) {
		case GENERAL_TRANSACTION_TYPE_SINGLE: {
			Transaction *trans = (Transaction*) transs;
			Transaction *oldtrans = (Transaction*) oldtranss;
			QTreeWidgetItemIterator it(transactionsView);
			TransactionListViewItem *i = (TransactionListViewItem*) *it;
			while(i) {
				if(i->transaction() == trans) {
					current_value -= oldtrans->value();
					current_quantity -= oldtrans->quantity();
					break;
				}
				++it;
				i = (TransactionListViewItem*) *it;
			}
			if(!i) {
				appendFilterTransaction(trans, true);
			} else {
				if(filterWidget->filterTransaction(trans)) {
					delete i;
				} else {
					current_value += trans->value();
					current_quantity += trans->quantity();
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
				updateStatistics();
			}
			if(oldtrans->description() != trans->description()) filterWidget->transactionModified(trans);
			editWidget->transactionModified(trans);
			break;
		}
		case GENERAL_TRANSACTION_TYPE_SCHEDULE: {
			ScheduledTransaction *strans = (ScheduledTransaction*) transs;
			ScheduledTransaction *oldstrans = (ScheduledTransaction*) oldtranss;
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
			appendFilterTransaction(strans, true);
			updateStatistics();
			if(strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SINGLE) {
				if(oldstrans->transaction()->description() != strans->transaction()->description()) filterWidget->transactionModified((Transaction*) strans->transaction());
				editWidget->transactionModified((Transaction*) strans->transaction());
			} else if(strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT) {
				SplitTransaction *split = (SplitTransaction*) strans->transaction();
				bool b_filter = (oldstrans->transaction()->description() != strans->transaction()->description());
				int n = split->count();
				for(int split_i = 0; split_i < n; split_i++) {
					editWidget->transactionModified(split->at(split_i));
					if(b_filter) filterWidget->transactionModified(split->at(split_i));
				}
			}
			break;
		}
		case GENERAL_TRANSACTION_TYPE_SPLIT: {
			if(((SplitTransaction*) transs)->type() != SPLIT_TRANSACTION_TYPE_MULTIPLE_ACCOUNTS) break;
			MultiAccountTransaction *split = (MultiAccountTransaction*) transs;
			MultiAccountTransaction *oldsplit = (MultiAccountTransaction*) oldtranss;
			QTreeWidgetItemIterator it(transactionsView);
			TransactionListViewItem *i = (TransactionListViewItem*) *it;
			while(i) {
				if(i->splitTransaction() == split) {
					current_value -= oldsplit->value();
					current_quantity -= oldsplit->quantity();
					break;
				}
				++it;
				i = (TransactionListViewItem*) *it;
			}
			if(!i) {
				appendFilterTransaction(split, true);
			} else {
				if(filterWidget->filterTransaction(split)) {
					delete i;
				} else {
					current_value += split->value();
					current_quantity += split->quantity();
					i->setDate(split->date());
					i->setText(0, QLocale().toString(split->date(), QLocale::ShortFormat));
					i->setText(1, split->description() + "*");
					i->setText(2, QLocale().toCurrencyString(split->value()));
					i->setText(3, split->category()->name());
					i->setText(4, split->accountsString());
					if(comments_col == 6) i->setText(5, split->payees());
					i->setText(comments_col, split->comment());
				}
				updateStatistics();
			}
			break;
		}
	}
}
void TransactionListWidget::onTransactionRemoved(Transactions *transs) {
	switch(transs->generaltype()) {
		case GENERAL_TRANSACTION_TYPE_SINGLE: {
			Transaction *trans = (Transaction*) transs;
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
			break;
		}
		case GENERAL_TRANSACTION_TYPE_SCHEDULE: {
			ScheduledTransaction *strans = (ScheduledTransaction*) transs;
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
			if(strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SINGLE) {
				editWidget->transactionRemoved((Transaction*) strans->transaction());
			} else if(strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT) {
				SplitTransaction *split = (SplitTransaction*) strans->transaction();
				int n = split->count();
				for(int split_i = 0; split_i < n; split_i++) {
					editWidget->transactionRemoved(split->at(split_i));
				}
			}
			break;
		}
		case GENERAL_TRANSACTION_TYPE_SPLIT: {
			if(((SplitTransaction*) transs)->type() != SPLIT_TRANSACTION_TYPE_MULTIPLE_ACCOUNTS) break;
			MultiAccountTransaction *split = (MultiAccountTransaction*) transs;
			QTreeWidgetItemIterator it(transactionsView);
			TransactionListViewItem *i = (TransactionListViewItem*) *it;
			while(i) {
				if(i->splitTransaction() == split) {
					delete i;
					current_value -= split->value();
					current_quantity -= split->quantity();
					updateStatistics();
					break;
				}
				++it;
				i = (TransactionListViewItem*) *it;
			}
			break;
		}
	}
}
void TransactionListWidget::editClear() {
	transactionsView->clearSelection();
	editInfoLabel->setText(QString::null);
	editWidget->setTransaction(NULL);
}
void TransactionListWidget::filterTransactions() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	selected_trans = NULL;
	if(selection.count() == 1) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.first();
		if(!i->scheduledTransaction()) {
			if(i->splitTransaction()) selected_trans = i->splitTransaction();
			else selected_trans = i->transaction();
		}
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
			break;
		}
	}
	ScheduledTransaction *strans = budget->scheduledTransactions.first();
	while(strans) {
		appendFilterTransaction(strans, false);
		strans = budget->scheduledTransactions.next();
	}
	SplitTransaction *split = budget->splitTransactions.first();
	while(split) {
		appendFilterTransaction(split, false);
		split = budget->splitTransactions.next();
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
	} else if(((TransactionListViewItem*) i)->splitTransaction()) {
		editWidget->setMultiAccountTransaction(((TransactionListViewItem*) i)->splitTransaction());
	} else if(((TransactionListViewItem*) i)->transaction()->parentSplit()) {
		editWidget->setTransaction(((TransactionListViewItem*) i)->transaction());
		SplitTransaction *split = ((TransactionListViewItem*) i)->transaction()->parentSplit();
		if(split->description().isEmpty() || split->description().length() > 10) editInfoLabel->setText(tr("* Part of split transaction"));
		else editInfoLabel->setText(tr("* Part of split (%1)").arg(split->description()));
	} else if(((TransactionListViewItem*) i)->scheduledTransaction()) {
		editWidget->setTransaction(((TransactionListViewItem*) i)->transaction(), ((TransactionListViewItem*) i)->date());
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
		} else if(((TransactionListViewItem*) i)->splitTransaction() || ((TransactionListViewItem*) i)->transaction()->parentSplit() || ((TransactionListViewItem*) i)->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || ((TransactionListViewItem*) i)->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL) {
			modifyButton->setText(tr("Edit…"));
		} else {
			modifyButton->setText(tr("Apply"));
		}
		modifyButton->setEnabled(true);
		removeButton->setEnabled(true);
		if(selection.count() > 1) {
			for(int index = 0; index < selection.size(); index++) {
				TransactionListViewItem *i = (TransactionListViewItem*) selection.at(index);
				if(i->scheduledTransaction()) {
					if(i->transaction() && i->transaction()->parentSplit()) {
						modifyButton->setEnabled(false);
						removeButton->setEnabled(false);
						break;
					} else if(i->splitTransaction()) {
						modifyButton->setEnabled(false);
					}
				}
			}
		}
		if(transactionsView->currentItem()->isSelected()) {
			currentTransactionChanged(transactionsView->currentItem());
		} else if(selection.count() == 1) {
			currentTransactionChanged(i);
		}
	}
	updateTransactionActions();
}
void TransactionListWidget::newMultiAccountTransaction() {
	if(mainWin->newMultiAccountTransaction(transtype == TRANSACTION_TYPE_EXPENSE, editWidget->description(), transtype == TRANSACTION_TYPE_EXPENSE ? (CategoryAccount*) editWidget->toAccount() : (CategoryAccount*) editWidget->fromAccount(), editWidget->quantity(), editWidget->comments())) {
		editClear();
	}
}
void TransactionListWidget::newTransactionWithLoan() {
	if(mainWin->newExpenseWithLoan(editWidget->description(), editWidget->value(), editWidget->quantity(), editWidget->date(), (ExpensesAccount*) editWidget->toAccount(), editWidget->payee(), editWidget->comments())) {
		editClear();
	}
	
}
void TransactionListWidget::newRefundRepayment() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() == 1) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.first();
		if(i->splitTransaction()) {
			if(i->splitTransaction()->value() > 0.0) mainWin->newRefundRepayment(i->splitTransaction());
		} else if((i->transaction()->type() == TRANSACTION_TYPE_EXPENSE && i->transaction()->value() > 0.0) || (i->transaction()->type() == TRANSACTION_TYPE_INCOME && i->transaction()->value() > 0.0 && !((Income*) i->transaction())->security())) {
			mainWin->newRefundRepayment(i->transaction());
		}
	}
}
void TransactionListWidget::updateTransactionActions() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	bool b_transaction = false, b_scheduledtransaction = false, b_split = false, b_join = false, b_delete = false;
	bool refundable = false, repayable = false;
	if(selection.count() == 1) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.first();
		b_split = (i->transaction() && i->transaction()->parentSplit());
		b_scheduledtransaction = i->scheduledTransaction() && i->scheduledTransaction()->recurrence();
		b_transaction = !b_scheduledtransaction || !i->scheduledTransaction()->isOneTimeTransaction();
		b_delete = b_transaction;
		refundable = (i->splitTransaction() || (i->transaction()->type() == TRANSACTION_TYPE_EXPENSE && i->transaction()->value() > 0.0));
		repayable = (i->splitTransaction() || (i->transaction()->type() == TRANSACTION_TYPE_INCOME && i->transaction()->value() > 0.0 && !((Income*) i->transaction())->security()));
	} else if(selection.count() > 1) {
		b_transaction = true;
		b_delete = true;
		b_join = true;
		b_split = true;
		SplitTransaction *split = NULL;
		if(b_join) {
			for(int index = 0; index < selection.size(); index++) {
				TransactionListViewItem *i = (TransactionListViewItem*) selection.at(index);
				if(b_join && (i->splitTransaction() || i->scheduledTransaction() || i->transaction()->parentSplit())) {
					b_join = false;
				}
				if(b_transaction && i->scheduledTransaction() && (i->splitTransaction() || (i->transaction() && i->transaction()->parentSplit()))) {
					b_transaction = false;
					if(!i->splitTransaction()) b_delete = false;
				}
				if(b_split) {
					Transaction *trans = i->transaction();
					if(!trans) {
						b_split = false;
					} else {
						if(!split) split = trans->parentSplit();
						if(!trans->parentSplit() || trans->parentSplit() != split) {
							b_split = false;							
						}
					}
				}
				if(!b_split && !b_join && !b_transaction && !b_delete) break;
			}
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
void TransactionListWidget::showFilter(bool focus_description) {
	tabs->setCurrentWidget(filterWidget);
	if(focus_description) filterWidget->focusDescription();
}
void TransactionListWidget::showEdit() {tabs->setCurrentWidget(editWidget);}
void TransactionListWidget::setFilter(QDate fromdate, QDate todate, double min, double max, Account *from_account, Account *to_account, QString description, QString payee, bool exclude, bool exact_match) {
	filterWidget->setFilter(fromdate, todate, min, max, from_account, to_account, description, payee, exclude, exact_match);
}

TransactionListViewItem::TransactionListViewItem(const QDate &trans_date, Transaction *trans, ScheduledTransaction *strans, MultiAccountTransaction *split, QString s1, QString s2, QString s3, QString s4, QString s5, QString s6, QString s7, QString s8) : QTreeWidgetItem(), o_trans(trans), o_strans(strans), o_split(split), d_date(trans_date) {
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
		if(o_split && i->splitTransaction()) return o_split->value() < i->splitTransaction()->value();
		if(o_split) return o_split->value() < i->transaction()->value();
		if(i->splitTransaction()) return o_trans->value() < i->splitTransaction()->value();
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
MultiAccountTransaction *TransactionListViewItem::splitTransaction() const {
	return o_split;
}
const QDate &TransactionListViewItem::date() const {
	return d_date;
}
void TransactionListViewItem::setDate(const QDate &newdate) {
	d_date = newdate;
}


