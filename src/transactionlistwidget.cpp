/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016-2020 by Hanna Knutsson            *
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
#include <QSettings>

#include "budget.h"
#include "editscheduledtransactiondialog.h"
#include "eqonomize.h"
#include "recurrence.h"
#include "transactioneditwidget.h"
#include "transactionfilterwidget.h"
#include "transactionlistwidget.h"

#include <QDebug>

#include <cmath>

extern QString last_associated_file_directory;

extern void setColumnTextWidth(QTreeWidget *w, int i, QString str);
extern void setColumnDateWidth(QTreeWidget *w, int i);
void setColumnMoneyWidth(QTreeWidget *w, int i, Budget *budget, double v = 9999999.99, int d = -1);
extern void setColumnStrlenWidth(QTreeWidget *w, int i, int l);
extern void setColumnValueWidth(QTreeWidget *w, int i, double v, int d, Budget *budget);

extern QColor createExpenseColor(QTreeWidgetItem *i, int = 0);
extern QColor createIncomeColor(QTreeWidgetItem *i, int = 0);
extern QColor createTransferColor(QTreeWidgetItem *i, int = 0);

class TransactionListViewItem : public QTreeWidgetItem {
	protected:
		Transaction *o_trans;
		ScheduledTransaction *o_strans;
		MultiAccountTransaction *o_split;
		QDate d_date;
	public:
		TransactionListViewItem(const QDate &trans_date, Transaction *trans, ScheduledTransaction *strans, MultiAccountTransaction *split, QString, QString = QString(), QString = QString(), QString = QString(), QString = QString(), QString = QString(), QString = QString(), QString = QString());
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

	right_align_values = true;

	key_event = NULL;

	selected_trans = NULL;

	listPopupMenu = NULL;
	headerPopupMenu = NULL;

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
	tags_col = -1;
	payee_col = -1;
	quantity_col = -1;
	switch(transtype) {
		case TRANSACTION_TYPE_EXPENSE: {
			headers << tr("Cost");
			headers << tr("Category");
			headers << tr("From Account");
			headers << tr("Payee");
			headers << tr("Quantity");
			quantity_col = 6;
			payee_col = 5;
			comments_col = 8;
			tags_col = 7;
			headers << tr("Tags");
			from_col = 4; to_col = 3;
			break;
		}
		case TRANSACTION_TYPE_INCOME: {
			headers << tr("Income");
			headers << tr("Category");
			headers << tr("To Account");
			headers << tr("Payer");
			payee_col = 5;
			comments_col = 7;
			tags_col = 6;
			headers << tr("Tags");
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
	headers << "t";
	transactionsView->setColumnCount(comments_col + 2);
	transactionsView->setHeaderLabels(headers);
	transactionsView->setColumnHidden(transactionsView->columnCount() - 1, true);
	updateColumnWidths();
	if(payee_col >= 0) transactionsView->setColumnHidden(payee_col, !b_extra);
	if(tags_col >= 0) transactionsView->setColumnHidden(tags_col, true);
	if(quantity_col >= 0) transactionsView->setColumnHidden(quantity_col, true);
	transactionsView->setRootIsDecorated(false);
	transactionsView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	transactionsViewLayout->addWidget(transactionsView);
	statLabel = new QLabel(this);
	statLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	transactionsViewLayout->addWidget(statLabel);
	QSizePolicy sp = transactionsView->sizePolicy();
	sp.setVerticalPolicy(QSizePolicy::MinimumExpanding);
	transactionsView->setSizePolicy(sp);

	tabs = new QTabWidget(this);
	tabs->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

	editWidget = new TransactionEditWidget(true, b_extra, transtype, NULL, false, NULL, SECURITY_SHARES_AND_QUOTATION, false, budget, this, true);
	editInfoLabel = new QLabel(QString());
	editWidget->bottomLayout()->addWidget(editInfoLabel, 1);
	QDialogButtonBox *buttons = new QDialogButtonBox();
	editWidget->bottomLayout()->addWidget(buttons);
	addButton = buttons->addButton(tr("Add"), QDialogButtonBox::ActionRole);
	modifyButton = buttons->addButton(tr("Apply"), QDialogButtonBox::ActionRole);
	clearButton = buttons->addButton(tr("Clear"), QDialogButtonBox::ActionRole);
	removeButton = buttons->addButton(tr("Delete"), QDialogButtonBox::ActionRole);
	modifyButton->setEnabled(false);
	removeButton->setEnabled(false);
	clearButton->setEnabled(false);

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

	connect(tabs, SIGNAL(currentChanged(int)), this, SLOT(currentTabChanged(int)));
	connect(addButton, SIGNAL(clicked()), this, SLOT(addTransaction()));
	connect(modifyButton, SIGNAL(clicked()), this, SLOT(modifyTransaction()));
	connect(editWidget, SIGNAL(addmodify()), this, SLOT(addModifyTransaction()));
	connect(removeButton, SIGNAL(clicked()), this, SLOT(removeTransaction()));
	connect(clearButton, SIGNAL(clicked()), this, SLOT(editClear()));
	connect(filterWidget, SIGNAL(filter()), this, SLOT(filterTransactions()));
	connect(filterWidget, SIGNAL(toActivated(Account*)), this, SLOT(filterToActivated(Account*)));
	connect(filterWidget, SIGNAL(fromActivated(Account*)), this, SLOT(filterFromActivated(Account*)));
	connect(transactionsView, SIGNAL(itemSelectionChanged()), this, SLOT(transactionSelectionChanged()));
	transactionsView->header()->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(transactionsView->header(), SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(popupHeaderMenu(const QPoint&)));
	transactionsView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(transactionsView, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(popupListMenu(const QPoint&)));
	connect(transactionsView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(transactionExecuted(QTreeWidgetItem*)));
	connect(transactionsView, SIGNAL(returnPressed(QTreeWidgetItem*)), this, SLOT(transactionExecuted(QTreeWidgetItem*)));

	connect(editWidget, SIGNAL(accountAdded(Account*)), this, SIGNAL(accountAdded(Account*)));
	connect(editWidget, SIGNAL(currenciesModified()), this, SIGNAL(currenciesModified()));
	connect(editWidget, SIGNAL(newLoanRequested()), this, SLOT(newTransactionWithLoan()));
	connect(editWidget, SIGNAL(multipleAccountsRequested()), this, SLOT(newMultiAccountTransaction()));
	connect(editWidget, SIGNAL(propertyChanged()), this, SLOT(updateClearButton()));
	connect(editWidget, SIGNAL(tagAdded(QString)), this, SIGNAL(tagAdded(QString)));
	connect(editInfoLabel, SIGNAL(linkActivated(const QString&)), this, SLOT(editSplitTransaction()));

}

//QSize TransactionListWidget::minimumSizeHint() const {return filterWidget->minimumSizeHint().expandedTo(editWidget->minimumSizeHint());}
//QSize TransactionListWidget::sizeHint() const {return QSize(filterWidget->sizeHint().expandedTo(editWidget->sizeHint()).width() + 12, QWidget::sizeHint().height());}
QSize TransactionListWidget::minimumSizeHint() const {return QWidget::minimumSizeHint();}
QSize TransactionListWidget::sizeHint() const {return minimumSizeHint();}

void TransactionListWidget::updateColumnWidths() {
	comments_col = 5;
	tags_col = -1;
	payee_col = -1;
	quantity_col = -1;
	switch(transtype) {
		case TRANSACTION_TYPE_EXPENSE: {
			quantity_col = 6;
			payee_col = 5;
			comments_col = 8;
			tags_col = 7;
			from_col = 4; to_col = 3;
			break;
		}
		case TRANSACTION_TYPE_INCOME: {
			payee_col = 5;
			comments_col = 7;
			tags_col = 6;
			from_col = 3; to_col = 4;
			break;
		}
		default: {
			from_col = 3; to_col = 4;
			break;
		}
	}
	setColumnDateWidth(transactionsView, 0);
	setColumnStrlenWidth(transactionsView, 1, 25);
	setColumnMoneyWidth(transactionsView, 2, budget);
	setColumnStrlenWidth(transactionsView, from_col, 20);
	setColumnStrlenWidth(transactionsView, to_col, 20);
	if(payee_col >= 0) {
		setColumnStrlenWidth(transactionsView, payee_col, 15);
	}
	if(tags_col >= 0) {
		setColumnStrlenWidth(transactionsView, tags_col, 15);
	}
}
QByteArray TransactionListWidget::saveState() {
	return transactionsView->header()->saveState();
}
void TransactionListWidget::restoreState(const QByteArray &state) {
	QSettings settings;
	if(settings.value("GeneralOptions/version", 0).toInt() >= 140) {
		transactionsView->header()->restoreState(state);
	}
	transactionsView->sortByColumn(0, Qt::DescendingOrder);
	transactionsView->setColumnHidden(transactionsView->columnCount() - 1, true);
}

void TransactionListWidget::updateClearButton() {
	clearButton->setEnabled(!editWidget->isCleared());
}

void TransactionListWidget::selectAssociatedFile() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() > 0) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.first();
		Transactions *transs = i->transaction();
		if(i->splitTransaction()) transs = i->splitTransaction();
		else if(i->transaction()->parentSplit()) transs = i->transaction()->parentSplit();
		if(transs) {
			QStringList urls = QFileDialog::getOpenFileNames(this, QString(), (transs->associatedFile().isEmpty() || transs->associatedFile().contains(",")) ? last_associated_file_directory : transs->associatedFile());
			if(!urls.isEmpty()) {
				QFileInfo fileInfo(urls[0]);
				last_associated_file_directory = fileInfo.absoluteDir().absolutePath();
				if(urls.size() == 1) {
					transs->setAssociatedFile(urls[0]);
				} else {
					QString url;
					for(int i = 0; i < urls.size(); i++) {
						if(i > 0) url += ", ";
						if(urls[i].contains("\"")) {url += "\'"; url += urls[i]; url += "\'";}
						else {url += "\""; url += urls[i]; url += "\"";}
					}
					transs->setAssociatedFile(url);
				}
				mainWin->ActionOpenAssociatedFile->setEnabled(true);
				if(transs->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT) {
					mainWin->transactionRemoved(transs);
					mainWin->transactionAdded(transs);
				} else {
					mainWin->transactionModified(transs, transs);
				}
			}
		}
	}
}
void TransactionListWidget::openAssociatedFile() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() > 0) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.first();
		Transactions *transs = i->transaction();
		if(i->splitTransaction()) transs = i->splitTransaction();
		else if(i->transaction()->parentSplit() && transs->associatedFile().isEmpty()) transs = i->transaction()->parentSplit();
		if(transs) {
			open_file_list(transs->associatedFile());
		}
	}
}
void TransactionListWidget::useMultipleCurrencies(bool b) {
	editWidget->useMultipleCurrencies(b);
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
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if((transtype == TRANSACTION_TYPE_EXPENSE || transtype == TRANSACTION_TYPE_INCOME) && selection.count() > 1) {
		double value = 0.0, quantity = 0.0;
		for(int index = 0; index < selection.count(); index++) {
			TransactionListViewItem *i = (TransactionListViewItem*) selection.at(index);
			Transactions *transs = i->splitTransaction();
			if(!transs) transs = i->transaction();
			value += transs->value(true);
			quantity += transs->quantity();
		}
		int i_count_frac = 0;
		double intpart = 0.0;
		if(modf(quantity, &intpart) != 0.0) i_count_frac = 2;
		statLabel->setText(QString("<div align=\"right\"><b>%1</b> %4 &nbsp; <b>%2</b> %5 &nbsp; <b>%3</b> %6</div>").arg(tr("Quantity:")).arg(transtype == TRANSACTION_TYPE_INCOME ? tr("Income") : tr("Cost:")).arg(tr("Average:")).arg(budget->formatValue(quantity, i_count_frac)).arg(budget->formatMoney(value)).arg(budget->formatMoney(quantity == 0.0 ? 0.0 : value / quantity)));
	} else {
		int i_count_frac = 0;
		double intpart = 0.0;
		if(modf(current_quantity, &intpart) != 0.0) i_count_frac = 2;
		statLabel->setText(QString("<div align=\"right\"><b>%1</b> %5 &nbsp; <b>%2</b> %6 &nbsp; <b>%3</b> %7 &nbsp; <b>%4</b> %8</div>").arg(tr("Quantity:")).arg(tr("Total:")).arg(tr("Average:")).arg(tr("Monthly:")).arg(budget->formatValue(current_quantity, i_count_frac)).arg(budget->formatMoney(current_value)).arg(budget->formatMoney(current_quantity == 0.0 ? 0.0 : current_value / current_quantity)).arg(budget->formatMoney(current_value == 0.0 ? current_value : current_value / filterWidget->countMonths())));
	}
}

void TransactionListWidget::keyPressEvent(QKeyEvent *e) {
	if(e == key_event) return;
	if(e->key() == Qt::Key_Escape && (e->modifiers() == Qt::NoModifier || e->modifiers() == Qt::KeypadModifier)) {
		if(!transactionsView->selectedItems().isEmpty()) {
			transactionsView->clearSelection();
			e->accept();
			return;
		} else if(tabs->currentWidget() == filterWidget) {
			filterWidget->clearFilter();
			e->accept();
			return;
		}
	}
	QWidget::keyPressEvent(e);
	if(!e->isAccepted() && editWidget->firstHasFocus() && e->key() != Qt::Key_Enter && e->key() != Qt::Key_Return) {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
		key_event = new QKeyEvent(e->type(), e->key(), e->modifiers(), e->nativeScanCode(), e->nativeVirtualKey(), e->nativeModifiers(), e->text(), e->isAutoRepeat(), e->count());
#else
		key_event = new QKeyEvent(*e);
#endif
		QApplication::sendEvent(transactionsView, key_event);
		delete key_event;
	}
}
void TransactionListWidget::tagsModified() {
	editWidget->tagsModified();
	filterWidget->updateTags();
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
		listPopupMenu->addAction(mainWin->ActionCloneTransaction);
		listPopupMenu->addSeparator();
		listPopupMenu->addAction(mainWin->ActionEditTransaction);
		listPopupMenu->addAction(mainWin->ActionEditScheduledTransaction);
		listPopupMenu->addAction(mainWin->ActionEditSplitTransaction);
		listPopupMenu->addAction(mainWin->ActionJoinTransactions);
		listPopupMenu->addAction(mainWin->ActionSplitUpTransaction);
		listPopupMenu->addSeparator();
		listPopupMenu->addAction(mainWin->ActionTags);
		listPopupMenu->addAction(mainWin->ActionLinks);
		listPopupMenu->addAction(mainWin->ActionCreateLink);
		listPopupMenu->addAction(mainWin->ActionSelectAssociatedFile);
		listPopupMenu->addAction(mainWin->ActionOpenAssociatedFile);
		listPopupMenu->addAction(mainWin->ActionEditTimestamp);
		listPopupMenu->addSeparator();
		listPopupMenu->addAction(mainWin->ActionDeleteTransaction);
		listPopupMenu->addAction(mainWin->ActionDeleteScheduledTransaction);
		listPopupMenu->addAction(mainWin->ActionDeleteSplitTransaction);
	}
	listPopupMenu->popup(transactionsView->viewport()->mapToGlobal(p));
}
void TransactionListWidget::hideColumn(bool do_show) {
	transactionsView->setColumnHidden(sender()->property("column_index").toInt(), !do_show);
}
void TransactionListWidget::popupHeaderMenu(const QPoint &p) {
	if(!headerPopupMenu) {
		headerPopupMenu = new QMenu(this);
		QTreeWidgetItem *header = transactionsView->headerItem();
		QAction *a = NULL;
		a = headerPopupMenu->addAction(header->text(3));
		a->setProperty("column_index", QVariant::fromValue(3));
		a->setCheckable(true);
		a->setChecked(!transactionsView->isColumnHidden(3));
		connect(a, SIGNAL(toggled(bool)), this, SLOT(hideColumn(bool)));
		a = headerPopupMenu->addAction(header->text(4));
		a->setProperty("column_index", QVariant::fromValue(4));
		a->setCheckable(true);
		a->setChecked(!transactionsView->isColumnHidden(4));
		connect(a, SIGNAL(toggled(bool)), this, SLOT(hideColumn(bool)));
		if(quantity_col >= 0) {
			a = headerPopupMenu->addAction(header->text(quantity_col));
			a->setProperty("column_index", QVariant::fromValue(quantity_col));
			a->setCheckable(true);
			a->setChecked(!transactionsView->isColumnHidden(quantity_col));
			connect(a, SIGNAL(toggled(bool)), this, SLOT(hideColumn(bool)));
		}
		if(payee_col >= 0) {
			a = headerPopupMenu->addAction(header->text(payee_col));
			a->setProperty("column_index", QVariant::fromValue(payee_col));
			a->setCheckable(true);
			a->setChecked(!transactionsView->isColumnHidden(payee_col));
			connect(a, SIGNAL(toggled(bool)), this, SLOT(hideColumn(bool)));
		}
		if(tags_col >= 0) {
			a = headerPopupMenu->addAction(header->text(tags_col));
			a->setProperty("column_index", QVariant::fromValue(tags_col));
			a->setCheckable(true);
			a->setChecked(!transactionsView->isColumnHidden(tags_col));
			connect(a, SIGNAL(toggled(bool)), this, SLOT(hideColumn(bool)));
		}
		if(comments_col >= 0) {
			a = headerPopupMenu->addAction(header->text(comments_col));
			a->setProperty("column_index", QVariant::fromValue(comments_col));
			a->setCheckable(true);
			a->setChecked(!transactionsView->isColumnHidden(comments_col));
			connect(a, SIGNAL(toggled(bool)), this, SLOT(hideColumn(bool)));
		}
		headerPopupMenu->addSeparator();
		ActionSortByCreationTime = headerPopupMenu->addAction(tr("Sort by creation time"));
		ActionSortByCreationTime->setCheckable(true);
		connect(ActionSortByCreationTime, SIGNAL(toggled(bool)), this, SLOT(sortByCreationTime(bool)));
		SeparatorRightAlignValues = headerPopupMenu->addSeparator();
		ActionRightAlignValues = headerPopupMenu->addAction(tr("Right align"));
		ActionRightAlignValues->setCheckable(true);
		connect(ActionRightAlignValues, SIGNAL(toggled(bool)), this, SLOT(rightAlignValues(bool)));
	}
	int c = transactionsView->columnAt(p.x());
	SeparatorRightAlignValues->setVisible(c == 2);
	ActionRightAlignValues->setVisible(c == 2);
	if(c == 2) {
		QSettings settings;
		ActionRightAlignValues->blockSignals(true);
		ActionRightAlignValues->setChecked(settings.value("GeneralOptions/rightAlignValues", true).toBool());
		ActionRightAlignValues->blockSignals(false);
	}
	ActionSortByCreationTime->blockSignals(true);
	ActionSortByCreationTime->setChecked(transactionsView->sortColumn() == transactionsView->columnCount() - 1);
	ActionSortByCreationTime->blockSignals(false);
	headerPopupMenu->popup(transactionsView->header()->viewport()->mapToGlobal(p));
}
void TransactionListWidget::sortByCreationTime(bool b) {
	if(b) transactionsView->sortByColumn(transactionsView->columnCount() - 1, Qt::DescendingOrder);
	else transactionsView->sortByColumn(0, Qt::DescendingOrder);
}
void TransactionListWidget::rightAlignValues(bool b) {
	emit valueAlignmentUpdated(b);
}

extern QString htmlize_string(QString str);

bool TransactionListWidget::isEmpty() {
	return transactionsView->topLevelItemCount() == 0;
}

bool TransactionListWidget::exportList(QTextStream &outf, int fileformat) {

	switch(fileformat) {
		case 'h': {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
			outf.setCodec("UTF-8");
#endif
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
			outf << "\t\t\t\t\t";
			for(int index_v = 0; index_v <= comments_col; index_v++) {
				int index = transactionsView->header()->logicalIndex(index_v);
				if(index >= 0 && !transactionsView->isColumnHidden(index)) {
					outf << "<th>" << htmlize_string(header->text(index)) << "</th>";
				}
			}
			outf << "\n";
			outf << "\t\t\t\t</tr>" << '\n';
			outf << "\t\t\t</thead>" << '\n';
			outf << "\t\t\t<tbody>" << '\n';
			QTreeWidgetItemIterator it(transactionsView);
			TransactionListViewItem *i = (TransactionListViewItem*) *it;
			while(i) {
				Transactions *trans = i->transaction();
				if(!trans) trans = i->splitTransaction();
				outf << "\t\t\t\t<tr>" << '\n';
				outf << "\t\t\t\t\t";
				for(int index_v = 0; index_v <= comments_col; index_v++) {
					int index = transactionsView->header()->logicalIndex(index_v);
					if(index >= 0 && !transactionsView->isColumnHidden(index)) {
						if(index == 0) {
							outf << "<td nowrap>" << htmlize_string(QLocale().toString(trans->date(), QLocale::ShortFormat)) << "</td>";
						} else if(index == 1) {
							outf << "<td>" << htmlize_string(trans->description()) << "</td>";
						} else if(index == 2) {
							outf << "<td nowrap align=\"right\">" << htmlize_string(trans->valueString()) << "</td>";
						} else {
							outf << "<td>" << htmlize_string(i->text(index)) << "</td>";
						}
					}
				}
				outf << "\n";
				outf << "\t\t\t\t</tr>" << '\n';
				++it;
				i = (TransactionListViewItem*) *it;
			}
			outf << "\t\t\t</tbody>" << '\n';
			outf << "\t\t</table>" << '\n';
			outf << "\t\t<div>";
			double intpart = 0.0;
			outf << htmlize_string(tr("Quantity:")) << " " << htmlize_string(budget->formatValue(current_quantity, modf(current_quantity, &intpart) != 0.0 ? 2 : 0));
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
			outf << htmlize_string(budget->formatMoney(current_value));
			outf << ", ";
			outf << htmlize_string(tr("Average:")) << " " << htmlize_string(budget->formatMoney(current_value == 0.0 ? current_value : current_value / current_quantity));
			outf << ", ";
			outf << htmlize_string(tr("Monthly average:")) << " " << htmlize_string(budget->formatMoney(current_value == 0.0 ? current_value : current_value / filterWidget->countMonths()));
			outf << "</div>\n";
			outf << "\t</body>" << '\n';
			outf << "</html>" << '\n';
			break;
		}
		case 'c': {
			//outf.setEncoding(Q3TextStream::Locale);
			QTreeWidgetItem *header = transactionsView->headerItem();
			outf << "\"" << header->text(0) << "\",\"" << header->text(1) << "\",\"" << header->text(2) << "\",\"" << header->text(3) << "\",\"" << header->text(4);
			if(quantity_col >= 0 && b_extra) outf << "\",\"" << header->text(quantity_col);
			if(payee_col >= 0 && b_extra) outf << "\",\"" << header->text(payee_col);
			if(tags_col >= 0) outf << "\",\"" << header->text(tags_col);
			outf << "\",\"" << header->text(comments_col);
			outf << "\"\n";
			QTreeWidgetItemIterator it(transactionsView);
			TransactionListViewItem *i = (TransactionListViewItem*) *it;
			while(i) {
				if(i->transaction()) {
					Transaction *trans = i->transaction();
					outf << "\"" << QLocale().toString(trans->date(), QLocale::ShortFormat) << "\",\"" << trans->description() << "\",\"" << trans->valueString().replace("−", "-").remove(" ") << "\",\"" << ((trans->type() == TRANSACTION_TYPE_EXPENSE) ? trans->toAccount()->nameWithParent() : trans->fromAccount()->nameWithParent()) << "\",\"" << ((trans->type() == TRANSACTION_TYPE_EXPENSE) ? trans->fromAccount()->nameWithParent() : trans->toAccount()->nameWithParent());
					if(b_extra && transtype == TRANSACTION_TYPE_EXPENSE) outf << "\",\"" << budget->formatValue(trans->quantity(), 2).replace("−", "-").remove(" ");
				} else {
					MultiAccountTransaction *trans = i->splitTransaction();
					outf << "\"" << QLocale().toString(trans->date(), QLocale::ShortFormat) << "\",\"" << trans->description() << "\",\"" << trans->valueString().replace("−", "-").remove(" ") << "\",\"" << trans->category()->nameWithParent() << "\",\"" << trans->accountsString();
					if(b_extra && transtype == TRANSACTION_TYPE_EXPENSE) outf << "\",\"" << budget->formatValue(trans->quantity(), 2).replace("−", "-").remove(" ");
				}
				if(payee_col >= 0 && b_extra) outf << "\",\"" << i->text(payee_col);
				if(tags_col >= 0) outf << "\",\"" << i->text(tags_col).replace("\"", "\'");
				outf << "\",\"" << i->text(comments_col);
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
	clearTransaction();
	filterTransactions();
}
void TransactionListWidget::addTransaction() {
	Transaction *trans = editWidget->createTransaction();
	if(!trans) return;
	clearTransaction();
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
			transactionsView->scrollToItem(i);
			break;
		}
		++it;
		i = (TransactionListViewItem*) *it;
	}
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
void TransactionListWidget::cloneTransaction() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() == 1) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.first();
		if(i->scheduledTransaction()) {
			mainWin->editScheduledTransaction(i->scheduledTransaction(), mainWin, true);
		} else if(i->splitTransaction()) {
			mainWin->editSplitTransaction(i->splitTransaction(), mainWin, false, true);
		} else if(i->transaction()) {
			if(i->transaction()->parentSplit()) mainWin->editSplitTransaction(i->transaction()->parentSplit(), mainWin, false, true);
			else mainWin->editTransaction(i->transaction(), mainWin, true);
		}
	}
}
void TransactionListWidget::editSplitTransaction() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() >= 1) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.first();
		if(!i->scheduledTransaction() && i->transaction()->parentSplit()) {
			if(mainWin->editSplitTransaction(i->transaction()->parentSplit())) clearTransaction();
		}
	}
}
void TransactionListWidget::editTimestamp() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() >= 1) {
		QList<Transactions*> trans;
		for(int index = 0; index < selection.size(); index++) {
			TransactionListViewItem *i = (TransactionListViewItem*) selection.at(index);
			if(i->scheduledTransaction()) {
				trans << i->scheduledTransaction();
			} else if(i->splitTransaction()) {
				trans << i->splitTransaction();
			} else {
				trans << i->transaction();
			}
		}
		if(mainWin->editTimestamp(trans)) {
			transactionSelectionChanged();
			transactionsView->setSortingEnabled(false);
			transactionsView->setSortingEnabled(true);
		}
	}
}
void TransactionListWidget::createLink(bool link_to) {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() >= 1) {
		QList<Transactions*> transactions;
		for(int index = 0; index < selection.count(); index++) {
			TransactionListViewItem *i = (TransactionListViewItem*) selection.at(index);
			if(i->scheduledTransaction()) transactions << i->scheduledTransaction();
			else if(i->splitTransaction()) transactions << i->splitTransaction();
			else if(i->transaction()) transactions << i->transaction();
		}
		for(int index = 0; index < transactions.count(); index++) {
			Transactions *itrans = transactions.at(index);
			if(itrans->generaltype() == GENERAL_TRANSACTION_TYPE_SINGLE) {
				Transaction *trans = (Transaction*) itrans;
				if(trans->parentSplit()) {
					SplitTransaction *split = trans->parentSplit();
					if(split->type() == SPLIT_TRANSACTION_TYPE_LOAN) {
						transactions.replace(index, split);
						for(int i = index + 1; i < transactions.count();) {
							if(transactions.at(i)->generaltype() == GENERAL_TRANSACTION_TYPE_SINGLE && ((Transaction*) transactions.at(i))->parentSplit() == split) {
								transactions.removeAt(i);
							} else {
								i++;
							}
						}
					} else {
						int n = split->count();
						bool b = true;
						for(int i = 0; i < n; i++) {
							if(!transactions.contains(split->at(i))) {
								b = false;
								break;
							}
						}
						if(b) {
							transactions.replace(index, split);
							for(int i = 0; i < n; i++) {
								transactions.removeAll(split->at(i));
							}
						}
					}
				}
			}
		}
		mainWin->createLink(transactions, link_to);
	}
}
void TransactionListWidget::modifyTags() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() >= 1) {
		QList<Transactions*> trans;
		if(selection.count() > 1) mainWin->startBatchEdit();
		for(int index = 0; index < selection.size(); index++) {
			TransactionListViewItem *i = (TransactionListViewItem*) selection.at(index);
			Transactions *trans = i->splitTransaction();
			if(!trans) {
				if(i->transaction()->parentSplit() && i->transaction()->parentSplit()->type() == SPLIT_TRANSACTION_TYPE_LOAN) trans = i->transaction()->parentSplit();
				else trans = i->transaction();
			}
			if(i->scheduledTransaction()) {
				Transactions *oldtrans = i->scheduledTransaction()->copy();
				mainWin->tagMenu->modifyTransaction(trans);
				mainWin->transactionModified(i->scheduledTransaction(), oldtrans);
				delete oldtrans;
			} else {
				Transactions *oldtrans = trans->copy();
				mainWin->tagMenu->modifyTransaction(trans);
				mainWin->transactionModified(trans, oldtrans);
				delete oldtrans;
			}
		}
		if(selection.count() > 1) mainWin->endBatchEdit();
		transactionSelectionChanged();
	}
}
void TransactionListWidget::editTransaction() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() == 1) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.first();
		if(i->scheduledTransaction()) {
			if(i->scheduledTransaction()->isOneTimeTransaction()) {
				if(mainWin->editScheduledTransaction(i->scheduledTransaction())) clearTransaction();
			} else {
				if(mainWin->editOccurrence(i->scheduledTransaction(), i->date())) clearTransaction();
			}
		} else if(i->splitTransaction()) {
			if(mainWin->editSplitTransaction(i->splitTransaction())) clearTransaction();
		} else {
			if(mainWin->editTransaction(i->transaction())) clearTransaction();
		}
	} else if(selection.count() > 1) {
		budget->setRecordNewAccounts(true);
		budget->resetDefaultCurrencyChanged();
		budget->resetCurrenciesModified();

		bool warned1 = false, warned2 = false, warned3 = false, warned4 = false, warned5 = false;
		MultipleTransactionsEditDialog *dialog = new MultipleTransactionsEditDialog(b_extra, transtype, budget, this, true);
		TransactionListViewItem *i = (TransactionListViewItem*) transactionsView->currentItem();
		if(!i->isSelected()) i = (TransactionListViewItem*) selection.first();
		if(i) {
			if(i->scheduledTransaction()) dialog->setTransaction(i->transaction(), i->date());
			else dialog->setTransaction(i->transaction());
		}
		bool equal_date = true, equal_description = true, equal_value = true, equal_category = (transtype != TRANSACTION_TYPE_TRANSFER), equal_payee = (dialog->payeeButton != NULL), equal_currency = true;
		Transaction *comptrans = NULL;
		Account *compcat = NULL;
		Currency *compcur = NULL;
		QDate compdate;
		QString compdesc;
		double compvalue = 0.0;
		bool incomplete_split = false;
		QList<MultiItemTransaction*> splits;
		for(int index = 0; index < selection.size(); index++) {
			TransactionListViewItem *i = (TransactionListViewItem*) selection.at(index);
			if(!comptrans) {
				if(i->splitTransaction()) {
					comptrans = i->splitTransaction()->at(0);
					compvalue = i->splitTransaction()->value();
					compcur = i->splitTransaction()->currency();
					for(int split_i = 1; split_i < i->splitTransaction()->count(); split_i++) {
						Transaction *i_trans = i->splitTransaction()->at(split_i);
						if((comptrans->type() == TRANSACTION_TYPE_EXPENSE && ((Expense*) comptrans)->payee() != ((Expense*) i_trans)->payee()) || (comptrans->type() == TRANSACTION_TYPE_INCOME && ((Income*) comptrans)->payer() != ((Income*) i_trans)->payer())) {
							equal_payee = false;
						}
						if(i_trans->date() != comptrans->date()) {
							equal_date = false;
						}
						if(equal_currency && (i_trans->fromCurrency() != compcur || i_trans->toCurrency() != compcur)) equal_currency = false;
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
							equal_payee = false;
							equal_date = false;
						} else if(i->transaction()->parentSplit()->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ITEMS) {
							MultiItemTransaction *split = (MultiItemTransaction*) i->transaction()->parentSplit();
							for(int index2 = 0; index2 < split->count(); index2++) {
								Transaction *split_trans = split->at(index2);
								if(split_trans != i->transaction()) {
									bool b_match = false;
									for(int index3 = index + 1; index3 < selection.size(); index3++) {
										 if(((TransactionListViewItem*) selection.at(index3))->transaction() == split_trans) {
										 	b_match = true;
										 	break;
										 }
									}
									if(b_match) {
										if(!i->scheduledTransaction()) splits << split;
									} else {
										equal_payee = false;
										equal_date = false;
										incomplete_split = true;
										break;
									}
								}
							}
						}
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
					compcur = i->transaction()->fromCurrency();
					if(i->transaction()->toCurrency() != compcur) equal_currency = false;
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
						if(equal_currency && (i_trans->fromCurrency() != compcur || i_trans->toCurrency() != compcur)) equal_currency = false;
					}
					if(equal_value && i->splitTransaction()->value() != compvalue) equal_value = false;
					if(equal_category && i->splitTransaction()->category() != compcat) equal_category = false;
					if(equal_description && compdesc != i->splitTransaction()->description()) equal_description = false;
				} else {
					if(equal_date && compdate != i->date()) {
						equal_date = false;
					}
					if(equal_payee && (trans->type() != comptrans->type() || (comptrans->type() == TRANSACTION_TYPE_EXPENSE && ((Expense*) comptrans)->payee() != ((Expense*) trans)->payee()) || (comptrans->type() == TRANSACTION_TYPE_INCOME && ((Income*) comptrans)->payer() != ((Income*) trans)->payer()))) {
						equal_payee = false;
					}
					if(i->transaction()->parentSplit()) {
						if(i->transaction()->parentSplit()->type() == SPLIT_TRANSACTION_TYPE_LOAN) {
							equal_payee = false;
							equal_date = false;
							equal_payee = false;
							equal_date = false;
						} else if(i->transaction()->parentSplit()->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ITEMS) {
							MultiItemTransaction *split = (MultiItemTransaction*) i->transaction()->parentSplit();
							if(!splits.contains(split)) {
								for(int index2 = 0; index2 < split->count(); index2++) {
									Transaction *split_trans = split->at(index2);
									if(split_trans != i->transaction()) {
										bool b_match = false;
										for(int index3 = index + 1; index3 < selection.size(); index3++) {
											 if(((TransactionListViewItem*) selection.at(index3))->transaction() == split_trans) {
											 	b_match = true;
											 	break;
											 }
										}
										if(b_match) {
											if(!i->scheduledTransaction()) splits << split;
										} else {
											equal_date = false;
											incomplete_split = true;
											break;
										}
									}
								}
							}
						}
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
					if(equal_currency && (i->transaction()->toCurrency() != compcur || i->transaction()->fromCurrency() != compcur)) equal_currency = false;
				}
			}
		}
		if(equal_description) dialog->descriptionButton->setChecked(true);
		if(equal_payee) dialog->payeeButton->setChecked(true);
		if(equal_value && equal_currency) dialog->valueButton->setChecked(true);
		if(!equal_currency) dialog->valueButton->setEnabled(false);
		if(equal_date) dialog->dateButton->setChecked(true);
		if(equal_category && dialog->categoryButton) dialog->categoryButton->setChecked(true);
		if(dialog->exec() == QDialog::Accepted) {
			foreach(Account* acc, budget->newAccounts) emit accountAdded(acc);
			if(!budget->newAccounts.isEmpty()) updateAccounts();
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
						} else if(incomplete_split && !warned4) {
							if(dialog->dateButton->isChecked()) {
								QMessageBox::critical(this, tr("Error"), tr("Cannot change date of transactions that are part of a split transaction, unless all individual transactions are selected."));
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
			for(int index = 0; index < splits.size(); index++) {
				MultiItemTransaction *old_split = splits.at(index);
				SplitTransaction *new_split = old_split->copy();
				if(dialog->modifySplitTransaction(new_split)) {
					budget->removeSplitTransaction(old_split, true);
					mainWin->transactionRemoved(old_split);
					delete old_split;
					if(!future) {
						budget->addSplitTransaction(new_split);
						mainWin->transactionAdded(new_split);
					} else {
						ScheduledTransaction *strans = new ScheduledTransaction(budget, new_split, NULL);
						budget->addScheduledTransaction(strans);
						mainWin->transactionAdded(strans);
					}
				} else {
					delete new_split;
				}
			}
			mainWin->endBatchEdit();
			transactionSelectionChanged();
		} else {
			foreach(Account* acc, budget->newAccounts) emit accountAdded(acc);
			if(!budget->newAccounts.isEmpty()) updateAccounts();
			budget->newAccounts.clear();
		}
		if(budget->currenciesModified() || budget->defaultCurrencyChanged()) emit currenciesModified();
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
		Transaction *trans = ((TransactionListViewItem*) i)->transaction();
		if(trans->parentSplit()->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ITEMS && trans->type() == transtype) {
			MultiItemTransaction *split = (MultiItemTransaction*) trans->parentSplit();
			int index = 0;
			for(; index < split->count(); index++) {
				if(split->at(index) == trans) break;
			}
			MultiItemTransaction *split_new = NULL;
			if(transtype == TRANSACTION_TYPE_EXPENSE && trans->subtype() == TRANSACTION_SUBTYPE_EXPENSE) {
				split_new = (MultiItemTransaction*) split->copy();
				Expense *expense = (Expense*) split_new->at(index);
				editWidget->modifyTransaction(expense);
				split_new->setAccount(expense->from());
				split_new->setDate(expense->date());
			} else if(transtype == TRANSACTION_TYPE_INCOME && trans->subtype() == TRANSACTION_SUBTYPE_INCOME && !((Income*) trans)->security()) {
				split_new = (MultiItemTransaction*) split->copy();
				Income *income = (Income*) split_new->at(index);
				editWidget->modifyTransaction(income);
				split_new->setAccount(income->to());
				split_new->setDate(income->date());
			} else if(transtype == TRANSACTION_TYPE_TRANSFER && trans->subtype() == TRANSACTION_SUBTYPE_TRANSFER) {
				split_new = (MultiItemTransaction*) split->copy();
				Transfer *transfer = (Transfer*) split_new->at(index);
				bool b_from = (transfer->from() == split->account());
				editWidget->modifyTransaction(transfer);
				if(b_from) {
					AssetsAccount *new_account = transfer->from();
					transfer->setFrom(split->account());
					split_new->setAccount(new_account);
				} else {
					AssetsAccount *new_account = transfer->to();
					transfer->setTo(split->account());
					split_new->setAccount(new_account);
				}
				split_new->setDate(transfer->date());
			}
			if(split_new) {
				if(mainWin->editSplitTransaction(split_new, this, true)) {
					budget->removeSplitTransaction(split, true);
					mainWin->transactionRemoved(split);
					delete split;
					clearTransaction();
				}
				delete split_new;
				return;
			}
		}
		editSplitTransaction();
		return;
	}
	if(((TransactionListViewItem*) i)->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || ((TransactionListViewItem*) i)->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL || (((TransactionListViewItem*) i)->transaction()->type() == TRANSACTION_TYPE_INCOME && ((Income*) ((TransactionListViewItem*) i)->transaction())->security())) {
		editTransaction();
		return;
	}
	if(i->scheduledTransaction() && i->scheduledTransaction()->transaction()->generaltype() != GENERAL_TRANSACTION_TYPE_SINGLE) {
		editTransaction();
		return;
	}
	if(!editWidget->validValues(true)) return;
	ScheduledTransaction *curstranscopy = i->scheduledTransaction();
	QDate curdate_copy = i->date();
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
			clearTransaction();
			return;
		}
	}
	if(editWidget->date() > QDate::currentDate() || curstranscopy) {
		Transaction *newtrans = i->transaction()->copy();
		if(editWidget->modifyTransaction(newtrans)) {
			Transactions *trans = newtrans;
			if(editWidget->date() > QDate::currentDate()) trans = new ScheduledTransaction(budget, newtrans, NULL);
			removeTransaction();
			budget->addTransactions(trans);
			mainWin->transactionAdded(trans);
			QTreeWidgetItemIterator it(transactionsView);
			TransactionListViewItem *i = (TransactionListViewItem*) *it;
			while(i) {
				if(i->scheduledTransaction() == trans || i->transaction() == trans) {
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
		transactionsView->scrollToItem(i);
	}
	if(curstranscopy) {
		ScheduledTransaction *oldstrans = curstranscopy->copy();
		curstranscopy->addException(curdate_copy);
		mainWin->transactionModified(curstranscopy, oldstrans);
		delete oldstrans;
	}
	clearTransaction();
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
			clearTransaction();
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
			clearTransaction();
		}
	}
}
void TransactionListWidget::splitUpTransaction() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() >= 1) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.first();
		if(i->splitTransaction()) {
			mainWin->splitUpTransaction(i->splitTransaction());
			transactionSelectionChanged();
		} else if(i->transaction() && i->transaction()->parentSplit()) {
			mainWin->splitUpTransaction(i->transaction()->parentSplit());
			transactionSelectionChanged();
		}
	}
}
void TransactionListWidget::joinTransactions() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	SplitTransaction *split = NULL;
	QString payee;
	bool use_payee = true;
	QList<Transaction*> sel_bak;
	int b_multiaccount = -1;
	QString description;
	bool different_descriptions = false, different_dates = false;
	Account *account = NULL, *category = NULL;
	QDate date;
	for(int index = 0; index < selection.size(); index++) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.at(index);
		Transaction *trans = i->transaction();
		if(trans && !i->scheduledTransaction() && !i->splitTransaction() && !trans->parentSplit()) {
			if(trans->type() != transtype || (trans->type() != TRANSACTION_TYPE_EXPENSE && trans->type() != TRANSACTION_TYPE_INCOME)) {
				b_multiaccount = 0;
				break;
			} else if(!account) {
				if(trans->fromAccount()->type() == ACCOUNT_TYPE_ASSETS) {account = trans->fromAccount(); category = trans->toAccount();}
				else {account = trans->toAccount(); category = trans->fromAccount();}
				date = trans->date();
				description = trans->description();
			} else {
				if(trans->fromAccount()->type() == ACCOUNT_TYPE_ASSETS) {
					if(trans->toAccount() != category) {
						b_multiaccount = 0;
						break;
					} else if(b_multiaccount < 1 && trans->fromAccount() != account) {
						b_multiaccount = 1;
					}
				} else {
					if(trans->fromAccount() != category) {
						b_multiaccount = 0;
						break;
					} else if(b_multiaccount < 1 && trans->toAccount() != account) {
						b_multiaccount = 1;
					}
				}
				if(!different_dates && trans->date() != date) different_dates = true;
				if(description.isEmpty()) description = trans->description();
				else if(!different_descriptions && !trans->description().isEmpty() && description != trans->description()) different_descriptions = true;
			}
		}
	}
	if(b_multiaccount != 0) {
		if(different_descriptions && b_multiaccount == 1) b_multiaccount = -1;
		else if(b_multiaccount < 0 && different_descriptions && !different_dates) b_multiaccount = 0;
	}
	if(b_multiaccount < 0) {
		QMessageBox::StandardButton b = QMessageBox::question(this, tr("Join as multiple accounts/payments?"), transtype == TRANSACTION_TYPE_EXPENSE ? tr("Do you wish join the selected expenses as an expense with multiple accounts/payments?") : tr("Do you wish join the selected incomes as an income with multiple accounts/payments?"), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
		if(b == QMessageBox::Yes) b_multiaccount = 1;
		else if(b == QMessageBox::Cancel) return;
		else b_multiaccount = 0;
	}
	if(b_multiaccount > 0) {
		for(int index = 0; index < selection.size(); index++) {
			TransactionListViewItem *i = (TransactionListViewItem*) selection.at(index);
			Transaction *trans = i->transaction();
			if(trans && !i->scheduledTransaction() && !i->splitTransaction() && !trans->parentSplit()) {
				sel_bak << trans;
				if(!split) {
					if(trans->fromAccount()->type() == ACCOUNT_TYPE_ASSETS) split = new MultiAccountTransaction(budget, (CategoryAccount*) trans->toAccount(), trans->description());
					else split = new MultiAccountTransaction(budget, (CategoryAccount*) trans->fromAccount(), trans->description());
				} else {
					if(((MultiAccountTransaction*) split)->description().isEmpty() && !trans->description().isEmpty()) {
						((MultiAccountTransaction*) split)->setDescription(trans->description());
					}
				}
				if(!trans->associatedFile().isEmpty()) {
					if(split->associatedFile().isEmpty()) {
						split->setAssociatedFile(trans->associatedFile());
					} else {
						QString str;
						if((split->associatedFile().startsWith("\"") && split->associatedFile().endsWith("\"")) || (split->associatedFile().startsWith("\'") && split->associatedFile().endsWith("\'"))) str += split->associatedFile();
						else if(split->associatedFile().contains("\"")) {str += "\'"; str += split->associatedFile(); str += "\'";}
						else {str += "\""; str += split->associatedFile(); str += "\"";}
						str += ", ";
						if((trans->associatedFile().startsWith("\"") && trans->associatedFile().endsWith("\"")) || (trans->associatedFile().startsWith("\'") && trans->associatedFile().endsWith("\'"))) str += trans->associatedFile();
						else if(trans->associatedFile().contains("\"")) {str += "\'"; str += trans->associatedFile(); str += "\'";}
						else {str += "\""; str += trans->associatedFile(); str += "\"";}
						split->setAssociatedFile(str);
					}
				}
				if(!trans->comment().isEmpty()) {
					if(split->comment().isEmpty()) split->setComment(trans->comment());
					else split->setComment(split->comment() + " / " + trans->comment());
				}
				split->addTransaction(trans->copy());
			}
		}
	} else {
		for(int index = 0; index < selection.size(); index++) {
			TransactionListViewItem *i = (TransactionListViewItem*) selection.at(index);
			Transaction *trans = i->transaction();
			if(trans && !i->scheduledTransaction() && !i->splitTransaction() && !trans->parentSplit()) {
				sel_bak << trans;
				if(!split) {
					if((trans->type() == TRANSACTION_TYPE_SECURITY_BUY || trans->type() == TRANSACTION_TYPE_SECURITY_SELL) && ((SecurityTransaction*) trans)->account()->type() == ACCOUNT_TYPE_ASSETS) split = new MultiItemTransaction(budget, i->transaction()->date(), (AssetsAccount*) ((SecurityTransaction*) trans)->account());
					else if(trans->type() == TRANSACTION_TYPE_TRANSFER && selection.size() > index + 1) {
						TransactionListViewItem *i_next = (TransactionListViewItem*) selection.at(index + 1);
						Transaction *trans_next = i_next->transaction();
						if(trans_next && !i_next->scheduledTransaction() && !i_next->splitTransaction() && !trans_next->parentSplit()) {
							if((trans_next->type() == TRANSACTION_TYPE_SECURITY_BUY || trans_next->type() == TRANSACTION_TYPE_SECURITY_SELL) && ((SecurityTransaction*) trans_next)->account()->type() == ACCOUNT_TYPE_ASSETS) split = new MultiItemTransaction(budget, i->transaction()->date(), (AssetsAccount*) ((SecurityTransaction*) trans_next)->account());
							else if(trans->type() == TRANSACTION_TYPE_TRANSFER) {
								if(trans->fromAccount() == trans_next->toAccount() || trans->fromAccount() == trans_next->fromAccount()) split = new MultiItemTransaction(budget, i->transaction()->date(), (AssetsAccount*) trans->fromAccount());
								else if(trans->toAccount() == trans_next->toAccount() || trans->toAccount() == trans_next->fromAccount()) split = new MultiItemTransaction(budget, i->transaction()->date(), (AssetsAccount*) trans->toAccount());
								else split = new MultiItemTransaction(budget, i->transaction()->date(), (AssetsAccount*) trans->fromAccount());
							} else if(trans_next->fromAccount()->type() == ACCOUNT_TYPE_ASSETS) split = new MultiItemTransaction(budget, i->transaction()->date(), (AssetsAccount*) trans_next->fromAccount());
							else split = new MultiItemTransaction(budget, i->transaction()->date(), (AssetsAccount*) trans_next->toAccount());
						}
						if(!split) split = new MultiItemTransaction(budget, i->transaction()->date(), (AssetsAccount*) trans->fromAccount());
					} else if(trans->fromAccount()->type() == ACCOUNT_TYPE_ASSETS) split = new MultiItemTransaction(budget, i->transaction()->date(), (AssetsAccount*) trans->fromAccount());
					else split = new MultiItemTransaction(budget, i->transaction()->date(), (AssetsAccount*) trans->toAccount());
				}
				if(!trans->associatedFile().isEmpty()) {
					if(split->associatedFile().isEmpty()) {
						split->setAssociatedFile(trans->associatedFile());
					} else {
						QString str;
						if((split->associatedFile().startsWith("\"") && split->associatedFile().endsWith("\"")) || (split->associatedFile().startsWith("\'") && split->associatedFile().endsWith("\'"))) str += split->associatedFile();
						else if(split->associatedFile().contains("\"")) {str += "\'"; str += split->associatedFile(); str += "\'";}
						else {str += "\""; str += split->associatedFile(); str += "\"";}
						str += ", ";
						if((trans->associatedFile().startsWith("\"") && trans->associatedFile().endsWith("\"")) || (trans->associatedFile().startsWith("\'") && trans->associatedFile().endsWith("\'"))) str += trans->associatedFile();
						else if(trans->associatedFile().contains("\"")) {str += "\'"; str += trans->associatedFile(); str += "\'";}
						else {str += "\""; str += trans->associatedFile(); str += "\"";}
						split->setAssociatedFile(str);
					}
				}
				if(use_payee) {
					if(payee.isEmpty()) {
						if(trans->type() == TRANSACTION_TYPE_EXPENSE) payee = ((Expense*) trans)->payee();
						else if(trans->type() == TRANSACTION_TYPE_INCOME) payee = ((Income*) trans)->payer();
					} else {
						if(trans->type() == TRANSACTION_TYPE_EXPENSE && !((Expense*) trans)->payee().isEmpty() && payee != ((Expense*) trans)->payee()) use_payee = false;
						else if(trans->type() == TRANSACTION_TYPE_INCOME && !((Income*) trans)->payer().isEmpty() && payee != ((Income*) trans)->payer()) use_payee = false;
					}
				}
				split->addTransaction(trans->copy());
			}
		}
	}
	if(!split) return;
	if(!b_multiaccount && use_payee && !payee.isEmpty()) ((MultiItemTransaction*) split)->setPayee(payee);
	if(mainWin->editSplitTransaction(split, mainWin, true)) {
		delete split;
		for(int index = 0; index < sel_bak.size(); index++) {
			Transaction *trans = sel_bak.at(index);
			budget->removeTransaction(trans, true);
			mainWin->transactionRemoved(trans);
			delete trans;
		}
		clearTransaction();
	} else {
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
					mainWin->transactionRemoved(strans, NULL, true);
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
							mainWin->removeTransactionLinks(strans);
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
						mainWin->transactionRemoved(strans, NULL, true);
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
					mainWin->transactionRemoved(strans, NULL, true);
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
			mainWin->transactionRemoved(split, NULL, true);
			delete split;
		} else {
			Transaction *trans = i->transaction();
			if(trans->parentSplit() && trans->parentSplit()->count() == 1) {
				SplitTransaction *split = trans->parentSplit();
				budget->removeSplitTransaction(split, true);
				mainWin->transactionRemoved(split, NULL, true);
				delete split;
			} else if(trans->parentSplit() && trans->parentSplit()->type() != SPLIT_TRANSACTION_TYPE_LOAN && trans->parentSplit()->count() == 2) {
				SplitTransaction *split = trans->parentSplit();
				mainWin->splitUpTransaction(split);
				budget->removeTransaction(trans, true);
				mainWin->transactionRemoved(trans, NULL, true);
				delete trans;
			} else {
				budget->removeTransaction(trans, true);
				mainWin->transactionRemoved(trans, NULL, true);
				delete trans;
			}
		}
	}
	mainWin->endBatchEdit();
}
void TransactionListWidget::editClear() {
	editWidget->setTransaction(NULL);
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
	QSettings settings;
	right_align_values = settings.value("GeneralOptions/rightAlignValues", true).toBool();
	while(!strans || (!date.isNull() && date <= enddate)) {

		QTreeWidgetItem *i = new TransactionListViewItem(date, trans, strans, split, QString(), QString(), right_align_values ? transs->valueString() + " " : transs->valueString());

		if(strans && strans->recurrence()) i->setText(0, QLocale().toString(date, QLocale::ShortFormat) + "**");
		else i->setText(0, QLocale().toString(date, QLocale::ShortFormat));
		transactionsView->insertTopLevelItem(0, i);
		if(right_align_values) i->setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);

		if((split && split->cost() > 0.0) || (trans && ((trans->type() == TRANSACTION_TYPE_EXPENSE && trans->value() > 0.0) || (trans->type() == TRANSACTION_TYPE_INCOME && trans->value() < 0.0)))) {
			if(!expenseColor.isValid()) expenseColor = createExpenseColor(i, 2);
			i->setForeground(2, expenseColor);
		} else if((split && split->cost() < 0.0) || (trans && ((trans->type() == TRANSACTION_TYPE_EXPENSE && trans->value() < 0.0) || (trans->type() == TRANSACTION_TYPE_INCOME && trans->value() > 0.0)))) {
			if(!incomeColor.isValid()) incomeColor = createIncomeColor(i, 2);
			i->setForeground(2, incomeColor);
		} else {
			if(!transferColor.isValid()) transferColor = createTransferColor(i, 2);
			i->setForeground(2, transferColor);
		}
		if(strans) {
			QFont font = i->font(0);
			font.setItalic(true);
			i->setFont(0, font);
			i->setFont(1, font);
			i->setFont(2, font);
			i->setFont(3, font);
			i->setFont(4, font);
			i->setFont(5, font);
			i->setFont(6, font);
			i->setFont(7, font);
		}
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
			if(payee_col >= 0) {
				if(trans->type() == TRANSACTION_TYPE_EXPENSE) i->setText(payee_col, ((Expense*) trans)->payee());
				else if(trans->type() == TRANSACTION_TYPE_INCOME) i->setText(payee_col, ((Income*) trans)->payer());
			}
			if(trans->parentSplit() && trans->comment().isEmpty()) i->setText(comments_col, trans->parentSplit()->comment());
			else i->setText(comments_col, trans->comment());
			if(quantity_col >= 0) i->setText(quantity_col, budget->formatValue(trans->quantity()));
			if(tags_col >= 0) i->setText(tags_col, trans->tagsText(true));
			if(!trans->associatedFile().isEmpty() || (trans->parentSplit() && !trans->parentSplit()->associatedFile().isEmpty())) i->setIcon(2, LOAD_ICON_STATUS("mail-attachment"));
			if(trans->linksCount(true) > 0) i->setIcon(comments_col, LOAD_ICON_STATUS("go-jump"));
		} else if(split) {
			i->setText(3, split->category()->name());
			i->setText(4, split->accountsString());
			if(payee_col >= 0) i->setText(payee_col, split->payeeText());
			if(quantity_col >= 0) i->setText(quantity_col, budget->formatValue(split->quantity()));
			i->setText(comments_col, transs->comment());
			if(tags_col >= 0) i->setText(tags_col, transs->tagsText());
			if(!split->associatedFile().isEmpty()) i->setIcon(2, LOAD_ICON_STATUS("mail-attachment"));
			if(split->linksCount() > 0) i->setIcon(comments_col, LOAD_ICON_STATUS("go-jump"));
		}
		current_value += transs->value(true);
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
			if(i->transaction() && i->transaction()->parentSplit() == split) {
				i->setText(1, i->transaction()->description());
				if(!split->associatedFile().isEmpty() && i->transaction()->associatedFile().isEmpty()) i->setIcon(2, QIcon());
				if(split->linksCount() > 0 && i->transaction()->linksCount(true) == 0) i->setIcon(comments_col, QIcon());
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
					current_value -= oldtrans->value(true);
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
					current_value += trans->value(true);
					current_quantity += trans->quantity();
					i->setDate(trans->date());
					i->setText(0, QLocale().toString(trans->date(), QLocale::ShortFormat));
					if(trans->parentSplit()) i->setText(1, trans->description() + "*");
					else i->setText(1, trans->description());
					i->setText(2, right_align_values ? trans->valueString() + " " : transs->valueString());
					i->setText(from_col, trans->fromAccount()->name());
					i->setText(to_col, trans->toAccount()->name());
					if(payee_col >= 0) {
						if(trans->type() == TRANSACTION_TYPE_EXPENSE) i->setText(payee_col, ((Expense*) trans)->payee());
						else if(trans->type() == TRANSACTION_TYPE_INCOME) i->setText(payee_col, ((Income*) trans)->payer());
					}
					if(trans->parentSplit() && trans->comment().isEmpty()) i->setText(comments_col, trans->parentSplit()->comment());
					else i->setText(comments_col, trans->comment());
					if(tags_col >= 0) i->setText(tags_col, transs->tagsText(true));
					if(quantity_col >= 0) i->setText(quantity_col, budget->formatValue(transs->quantity()));
					if(!trans->associatedFile().isEmpty() || (trans->parentSplit() && !trans->parentSplit()->associatedFile().isEmpty())) i->setIcon(2, LOAD_ICON_STATUS("mail-attachment"));
					else i->setIcon(2, QIcon());
					if(trans->linksCount(true) > 0) i->setIcon(comments_col, LOAD_ICON_STATUS("go-jump"));
					else i->setIcon(comments_col, QIcon());
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
					current_value -= oldstrans->transaction()->value(true);
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
			if(((SplitTransaction*) transs)->type() != SPLIT_TRANSACTION_TYPE_MULTIPLE_ACCOUNTS) {
				if(((SplitTransaction*) transs)->count() == ((SplitTransaction*) oldtranss)->count()) {
					for(int i = 0; i < ((SplitTransaction*) transs)->count(); i++) {
						onTransactionModified(((SplitTransaction*) transs)->at(i), ((SplitTransaction*) oldtranss)->at(i));
					}
				} else {
					for(int i = 0; i < ((SplitTransaction*) transs)->count(); i++) {
						onTransactionRemoved(((SplitTransaction*) oldtranss)->at(i));
						appendFilterTransaction(((SplitTransaction*) transs)->at(i), true);
					}
				}
				break;
			}
			MultiAccountTransaction *split = (MultiAccountTransaction*) transs;
			MultiAccountTransaction *oldsplit = (MultiAccountTransaction*) oldtranss;
			QTreeWidgetItemIterator it(transactionsView);
			TransactionListViewItem *i = (TransactionListViewItem*) *it;
			while(i) {
				if(i->splitTransaction() == split) {
					current_value -= oldsplit->value(true);
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
					current_value += split->value(true);
					current_quantity += split->quantity();
					i->setDate(split->date());
					i->setText(0, QLocale().toString(split->date(), QLocale::ShortFormat));
					i->setText(1, split->description() + "*");
					i->setText(2, right_align_values ? split->valueString() + " " : split->valueString());
					i->setText(3, split->category()->name());
					i->setText(4, split->accountsString());
					if(payee_col >= 0) i->setText(payee_col, split->payeeText());
					i->setText(comments_col, split->comment());
					if(tags_col >= 0) i->setText(tags_col, split->tagsText());
					if(quantity_col >= 0) i->setText(quantity_col, budget->formatValue(split->quantity()));
					if(!split->associatedFile().isEmpty()) i->setIcon(2, LOAD_ICON_STATUS("mail-attachment"));
					else i->setIcon(2, QIcon());
					if(split->linksCount() > 0) i->setIcon(comments_col, LOAD_ICON_STATUS("go-jump"));
					else i->setIcon(comments_col, QIcon());
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
					current_value -= trans->value(true);
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
					current_value -= strans->transaction()->value(true);
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
					current_value -= split->value(true);
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
void TransactionListWidget::clearTransaction() {
	transactionsView->clearSelection();
	editInfoLabel->setText(QString());
	editWidget->setTransaction(NULL);
}
void TransactionListWidget::filterTransactions() {
	expenseColor = QColor();
	incomeColor = QColor();
	transferColor = QColor();
	//QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	selected_trans = NULL;
	/*if(selection.count() == 1) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.first();
		if(!i->scheduledTransaction()) {
			if(i->splitTransaction()) selected_trans = i->splitTransaction();
			else selected_trans = i->transaction();
		}
	}*/
	transactionsView->clear();
	current_value = 0.0;
	current_quantity = 0.0;
	switch(transtype) {
		case TRANSACTION_TYPE_EXPENSE: {
			for(TransactionList<Expense*>::const_iterator it = budget->expenses.constBegin(); it != budget->expenses.constEnd(); ++it) {
				Expense *expense = *it;
				appendFilterTransaction(expense, false);
			}
			for(SecurityTransactionList<SecurityTransaction*>::const_iterator it = budget->securityTransactions.constBegin(); it != budget->securityTransactions.constEnd(); ++it) {
				SecurityTransaction *sectrans = *it;
				if(sectrans->account()->type() == ACCOUNT_TYPE_EXPENSES) {
					appendFilterTransaction(sectrans, false);
				}
			}
			break;
		}
		case TRANSACTION_TYPE_INCOME: {
			for(TransactionList<Income*>::const_iterator it = budget->incomes.constBegin(); it != budget->incomes.constEnd(); ++it) {
				Income *income = *it;
				appendFilterTransaction(income, false);
			}
			for(SecurityTransactionList<SecurityTransaction*>::const_iterator it = budget->securityTransactions.constBegin(); it != budget->securityTransactions.constEnd(); ++it) {
				SecurityTransaction *sectrans = *it;
				if(sectrans->account()->type() == ACCOUNT_TYPE_INCOMES) {
					appendFilterTransaction(sectrans, false);
				}
			}
			break;
		}
		default: {
			for(TransactionList<Transfer*>::const_iterator it = budget->transfers.constBegin(); it != budget->transfers.constEnd(); ++it) {
				Transfer *transfer = *it;
				appendFilterTransaction(transfer, false);
			}
			for(SecurityTransactionList<SecurityTransaction*>::const_iterator it = budget->securityTransactions.constBegin(); it != budget->securityTransactions.constEnd(); ++it) {
				SecurityTransaction *sectrans = *it;
				if(sectrans->account()->type() == ACCOUNT_TYPE_ASSETS) {
					appendFilterTransaction(sectrans, false);
				}
			}
			break;
		}
	}
	for(ScheduledTransactionList<ScheduledTransaction*>::const_iterator it = budget->scheduledTransactions.constBegin(); it != budget->scheduledTransactions.constEnd(); ++it) {
		ScheduledTransaction *strans = *it;
		appendFilterTransaction(strans, false);
	}
	for(SplitTransactionList<SplitTransaction*>::const_iterator it = budget->splitTransactions.constBegin(); it != budget->splitTransactions.constEnd(); ++it) {
		SplitTransaction *split = *it;
		appendFilterTransaction(split, false);
	}
	/*selected_trans = NULL;
	selection = transactionsView->selectedItems();
	if(selection.count() == 0) {*/
		editInfoLabel->setText("");
	//}
	updateStatistics();
	transactionsView->setSortingEnabled(true);
}

void TransactionListWidget::currentTransactionChanged(QTreeWidgetItem *i) {
	if(i == NULL) {
		editWidget->setTransaction(NULL);
		editInfoLabel->setText(QString());
	} else if(((TransactionListViewItem*) i)->scheduledTransaction()) {
		if(((TransactionListViewItem*) i)->splitTransaction()) {
			editWidget->setMultiAccountTransaction(((TransactionListViewItem*) i)->splitTransaction(), ((TransactionListViewItem*) i)->date() == ((TransactionListViewItem*) i)->splitTransaction()->date() ? QDate() : ((TransactionListViewItem*) i)->date());
		} else {
			editWidget->setTransaction(((TransactionListViewItem*) i)->transaction(), ((TransactionListViewItem*) i)->date());
		}
		if(((TransactionListViewItem*) i)->scheduledTransaction()->isOneTimeTransaction()) editInfoLabel->setText(QString());
		else editInfoLabel->setText(tr("** Recurring (editing occurrence)"));
	} else if(((TransactionListViewItem*) i)->splitTransaction()) {
		editWidget->setMultiAccountTransaction(((TransactionListViewItem*) i)->splitTransaction());
		editInfoLabel->setText(QString());
	} else if(((TransactionListViewItem*) i)->transaction()->parentSplit()) {
		editWidget->setTransaction(((TransactionListViewItem*) i)->transaction());
		SplitTransaction *split = ((TransactionListViewItem*) i)->transaction()->parentSplit();
		QFontMetrics fm(editInfoLabel->font());
		int tw = fm.boundingRect(tr("* Part of split (%1)").arg(split->description())).width() + 20;
		if(split->description().isEmpty() || tw > editInfoLabel->width()) editInfoLabel->setText(tr("* Part of <a href=\"%1\">split transaction</a>").arg("split transaction"));
		else editInfoLabel->setText(tr("* Part of split (%1)").arg("<a href=\"split transaction\">" + split->description() + "</a>"));
	} else {
		editWidget->setTransaction(((TransactionListViewItem*) i)->transaction());
		editInfoLabel->setText(QString());
	}
}
void TransactionListWidget::transactionSelectionChanged() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() == 0) {
		editInfoLabel->setText(QString());
		modifyButton->setEnabled(false);
		removeButton->setEnabled(false);
		currentTransactionChanged(NULL);
	} else {
		QTreeWidgetItem *i = selection.first();
		if(selection.count() > 1) {
			modifyButton->setText(tr("Modify…"));
		} else if(((TransactionListViewItem*) i)->splitTransaction() || ((TransactionListViewItem*) i)->transaction()->parentSplit() || ((TransactionListViewItem*) i)->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || ((TransactionListViewItem*) i)->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL || (((TransactionListViewItem*) i)->transaction()->type() == TRANSACTION_TYPE_INCOME && ((Income*) ((TransactionListViewItem*) i)->transaction())->security())) {
			modifyButton->setText(tr("Edit…"));
		} else {
			modifyButton->setText(tr("Apply"));
		}
		modifyButton->setEnabled(true);
		removeButton->setEnabled(true);
		if(selection.count() > 1) {
			for(int index = 0; index < selection.size(); index++) {
				TransactionListViewItem *i = (TransactionListViewItem*) selection.at(index);
				if(i->transaction() && i->transaction()->subtype() == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND) {
					modifyButton->setEnabled(false);
					break;
				}
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
	if(transtype == TRANSACTION_TYPE_INCOME || transtype == TRANSACTION_TYPE_EXPENSE) updateStatistics();
}
void TransactionListWidget::newMultiAccountTransaction() {
	if(mainWin->newMultiAccountTransaction(transtype == TRANSACTION_TYPE_EXPENSE, editWidget->description(), transtype == TRANSACTION_TYPE_EXPENSE ? (CategoryAccount*) editWidget->toAccount() : (CategoryAccount*) editWidget->fromAccount(), editWidget->quantity(), editWidget->comments())) {
		clearTransaction();
	}
}
void TransactionListWidget::newTransactionWithLoan() {
	if(mainWin->newExpenseWithLoan(editWidget->description(), editWidget->value(), editWidget->quantity(), editWidget->date(), (ExpensesAccount*) editWidget->toAccount(), editWidget->payee(), editWidget->comments())) {
		clearTransaction();
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
	bool b_transaction = false, b_scheduledtransaction = false, b_split = false, b_split2 = false, b_join = false, b_delete = false, b_attachment = false, b_select = false, b_time = false, b_tags = false, b_clone = false, b_link = false, b_link_to = false, b_edit = true;
	bool refundable = false, repayable = false;
	QList<Transactions*> list;
	Transactions *link_trans = mainWin->getLinkTransaction();
	SplitTransaction *link_parent = NULL;
	if(link_trans && link_trans->generaltype() == GENERAL_TRANSACTION_TYPE_SINGLE) link_parent = ((Transaction*) link_trans)->parentSplit();
	if(selection.count() == 1) {
		TransactionListViewItem *i = (TransactionListViewItem*) selection.first();
		b_scheduledtransaction = i->scheduledTransaction() && i->scheduledTransaction()->recurrence();
		b_split = !b_scheduledtransaction && (i->transaction() && i->transaction()->parentSplit());
		b_split2 = i->splitTransaction();
		b_transaction = !b_scheduledtransaction || !i->scheduledTransaction()->isOneTimeTransaction();
		b_link = !i->scheduledTransaction() || i->scheduledTransaction()->isOneTimeTransaction();
		b_link_to = link_trans && b_link && i->transaction() != link_trans && i->splitTransaction() != link_trans && (!b_split || (i->transaction()->parentSplit() != link_trans && i->transaction()->parentSplit() != link_parent));
		b_time = !i->scheduledTransaction();
		b_delete = b_transaction;
		refundable = (i->splitTransaction() || (i->transaction()->type() == TRANSACTION_TYPE_EXPENSE && i->transaction()->value() > 0.0));
		repayable = (i->splitTransaction() || (i->transaction()->type() == TRANSACTION_TYPE_INCOME && i->transaction()->value() > 0.0 && !((Income*) i->transaction())->security()));
		if(i->splitTransaction()) {
			list << i->splitTransaction();
			b_attachment = !i->splitTransaction()->associatedFile().isEmpty();
		} else {
			if(b_split) {
				b_attachment = !i->transaction()->parentSplit()->associatedFile().isEmpty();
			}
			if(!b_attachment && i->transaction()) {
				b_attachment = !i->transaction()->associatedFile().isEmpty();
			}
			list << i->transaction();
		}
		b_clone = true;
		b_select = true;
		mainWin->updateLinksAction(list[0], !b_scheduledtransaction || i->scheduledTransaction()->isOneTimeTransaction());
	} else if(selection.count() > 1) {
		b_transaction = true;
		b_delete = true;
		b_join = true;
		b_split = true;
		b_time = true;
		b_link = true;
		b_link_to = (link_trans != NULL);
		SplitTransaction *split = NULL;
		TransactionListViewItem *i = (TransactionListViewItem*) selection.first();
		if(i->transaction() && i->transaction()->parentSplit() && selection.size() < i->transaction()->parentSplit()->count()) {
			split = i->transaction()->parentSplit();
			b_link = false;
			for(int index = 1; index < selection.size(); index++) {
				i = (TransactionListViewItem*) selection.at(index);
				if(!i->transaction() || i->transaction()->parentSplit() != split) {
					b_link = true;
					break;
				}
			}
			split = NULL;
		}
		for(int index = 0; index < selection.size(); index++) {
			if(index >= 10) b_link = false;
			i = (TransactionListViewItem*) selection.at(index);
			if((b_edit || b_join) && i->transaction() && i->transaction()->subtype() == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND) {
				b_join = false;
				b_edit = false;
			}
			if(b_join && (i->splitTransaction() || i->scheduledTransaction() || i->transaction()->parentSplit())) {
				b_join = false;
			}
			if(i->scheduledTransaction()) {
				if((b_link || b_link_to) && !i->scheduledTransaction()->isOneTimeTransaction()) {
					b_link = false;
					b_link_to = false;
				}
				b_time = false;
			}
			if(b_transaction && i->scheduledTransaction() && (i->splitTransaction() || (i->transaction() && i->transaction()->parentSplit()))) {
				b_transaction = false;
				if(!i->splitTransaction()) b_delete = false;
			}
			Transaction *trans = i->transaction();
			if(b_link_to && i->transaction() != link_trans && i->splitTransaction() != link_trans && trans && trans->parentSplit() && (trans->parentSplit() == link_trans || trans->parentSplit() == link_parent)) b_link_to = false;
			if(b_split) {
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
			if(i->splitTransaction()) list << i->splitTransaction();
			else if(i->transaction()) list << i->transaction();
		}
		b_select = b_split && split;
		b_attachment = b_select && !split->associatedFile().isEmpty();
		mainWin->updateLinksAction(NULL);
	} else {
		mainWin->updateLinksAction(NULL);
	}
	b_tags = b_transaction;
	if(b_tags) {
		mainWin->tagMenu->setTransactions(list);
		mainWin->ActionTags->setText(tr("Tags") + QString(" (") + QString::number(mainWin->tagMenu->selectedTagsCount()) + ")");
	}
	if(b_link && list.count() >= 2) {
		b_link = false;
		for(int i = 0; i < list.count() - 1; i++) {
			for(int i2 = i + 1; i2 < list.count(); i2++) {
				if(!list.at(i)->hasLink(list.at(i2), true)) {
					b_link = true;
					break;
				}
			}
		}
	}
	if(b_link_to && list.count() >= 1) {
		b_link_to = false;
		for(int i = 0; i < list.count(); i++) {
			if(!list.at(i)->hasLink(link_trans, true)) {
				b_link_to = true;
				break;
			}
		}
	}
	if(!b_transaction) b_edit = false;
	mainWin->ActionCreateLink->setEnabled(b_link);
	mainWin->ActionLinkTo->setEnabled(b_link_to);
	mainWin->ActionNewRefund->setEnabled(refundable);
	mainWin->ActionNewRepayment->setEnabled(repayable);
	mainWin->ActionNewRefundRepayment->setEnabled(refundable || repayable);
	mainWin->ActionEditTransaction->setEnabled(b_edit);
	mainWin->ActionEditTimestamp->setEnabled(b_time);
	mainWin->ActionDeleteTransaction->setEnabled(b_transaction);
	mainWin->ActionSelectAssociatedFile->setEnabled(b_select);
	mainWin->ActionOpenAssociatedFile->setEnabled(b_attachment);
	mainWin->ActionEditScheduledTransaction->setEnabled(b_scheduledtransaction);
	mainWin->ActionDeleteScheduledTransaction->setEnabled(b_scheduledtransaction);
	mainWin->ActionEditSplitTransaction->setEnabled(b_split);
	mainWin->ActionDeleteSplitTransaction->setEnabled(b_split);
	mainWin->ActionJoinTransactions->setEnabled(b_join);
	mainWin->ActionSplitUpTransaction->setEnabled(b_split || b_split2);
	mainWin->ActionCloneTransaction->setEnabled(b_clone);
	if(b_tags) {
		mainWin->ActionTags->setEnabled(true);
	} else {
		mainWin->ActionTags->setText(tr("Tags"));
		mainWin->ActionTags->setEnabled(false);
	}
}
void TransactionListWidget::filterToActivated(Account *acc) {
	if(acc && (acc->type() != ACCOUNT_TYPE_ASSETS || (acc != budget->balancingAccount && ((AssetsAccount*) acc)->accountType() != ASSETS_TYPE_SECURITIES && !((AssetsAccount*) acc)->isClosed()))) editWidget->setToAccount(acc);
}
void TransactionListWidget::filterFromActivated(Account *acc) {
	if(acc && (acc->type() != ACCOUNT_TYPE_ASSETS || (acc != budget->balancingAccount && ((AssetsAccount*) acc)->accountType() != ASSETS_TYPE_SECURITIES && !((AssetsAccount*) acc)->isClosed()))) editWidget->setFromAccount(acc);
}
void TransactionListWidget::onDisplay() {
	if(tabs->currentWidget() == editWidget) editWidget->focusFirst();
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
void TransactionListWidget::setDefaultAccounts() {
	editWidget->setDefaultAccounts();
}
void TransactionListWidget::showFilter(bool focus_description) {
	tabs->setCurrentWidget(filterWidget);
	if(focus_description) filterWidget->focusFirst();
}
void TransactionListWidget::showEdit() {tabs->setCurrentWidget(editWidget);}
void TransactionListWidget::setFilter(QDate fromdate, QDate todate, double min, double max, Account *from_account, Account *to_account, QString description, QString tag, bool exclude, bool exact_match) {
	filterWidget->setFilter(fromdate, todate, min, max, from_account, to_account, description, tag, exclude, exact_match);
}
void TransactionListWidget::currentTabChanged(int index) {
	if(index == 0) editWidget->focusFirst();
	else if(index == 1) filterWidget->focusFirst();
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
	Transactions *t1 = o_split;
	if(!t1) t1 = o_trans;
	Transactions *t2 = i->splitTransaction();
	if(!t2) t2 = i->transaction();
	if(treeWidget() && col == treeWidget()->columnCount() - 1) {
		if(t1->timestamp() < t2->timestamp()) return true;
		if(t1->timestamp() > t2->timestamp()) return false;
		col = 0;
	}
	if(col == 0) {
		if(d_date < i->date()) return true;
		if(d_date > i->date()) return false;
		if((o_strans == NULL) != (i->scheduledTransaction() == NULL)) return o_strans == NULL;
		if(t1->timestamp() < t2->timestamp()) return true;
		if(t1->timestamp() > t2->timestamp()) return false;
	} else if(col == 2) {
		double d1 = t1->value(true), d2 = t2->value(true);
		if(d1 < d2) return true;
		if(d1 > d2) return false;
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

