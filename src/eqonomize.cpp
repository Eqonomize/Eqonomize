/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016-2018 by Hanna Knutsson            *
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

#include <QButtonGroup>
#include <QCheckBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDesktopWidget>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLayout>
#include <QMenu>
#include <QObject>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QRadioButton>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>
#include <QAction>
#include <QActionGroup>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QUrl>
#include <QFileDialog>
#include <QTabWidget>
#include <QSaveFile>
#include <QSpinBox>
#include <QMimeType>
#include <QMimeData>
#include <QMimeDatabase>
#include <QTemporaryFile>
#include <QCommandLineParser>
#include <QDateEdit>
#include <QCalendarWidget>
#include <QMessageBox>
#include <QTextEdit>
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QTextDocument>
#include <QTabWidget>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QSettings>
#include <QWhatsThis>
#include <QDesktopServices>
#include <QLocalSocket>
#include <QLocalServer>
#include <QLocale>
#include <QShortcut>
#include <QTextBrowser>
#include <QToolButton>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QProgressDialog>

#include <QDebug>

#include "accountcombobox.h"
#include "budget.h"
#include "editaccountdialogs.h"
#include "editcurrencydialog.h"
#include "categoriescomparisonchart.h"
#include "categoriescomparisonreport.h"
#include "currencyconversiondialog.h"
#include "editscheduledtransactiondialog.h"
#include "editsplitdialog.h"
#include "eqonomize.h"
#include "eqonomizemonthselector.h"
#include "eqonomizevalueedit.h"
#include "importcsvdialog.h"
#include "ledgerdialog.h"
#include "overtimechart.h"
#include "overtimereport.h"
#include "qifimportexport.h"
#include "recurrence.h"
#include "recurrenceeditwidget.h"
#include "security.h"
#include "transactionlistwidget.h"
#include "transactioneditwidget.h"

#include <cmath>

#define ACCOUNTS_PAGE_INDEX 0
#define EXPENSES_PAGE_INDEX 1
#define INCOMES_PAGE_INDEX 2
#define TRANSFERS_PAGE_INDEX 3
#define SECURITIES_PAGE_INDEX 4
#define SCHEDULE_PAGE_INDEX 5

#define MAX_RECENT_FILES 10

#define NEW_ACCOUNT_TREE_WIDGET_ITEM(i, parent, s1, s2, s3, s4) QTreeWidgetItem *i = new QTreeWidgetItem(parent); i->setText(0, s1); i->setText(1, s2); i->setText(2, s3); i->setText(3, s4); i->setTextAlignment(BUDGET_COLUMN, Qt::AlignRight | Qt::AlignVCenter); i->setTextAlignment(CHANGE_COLUMN, Qt::AlignRight | Qt::AlignVCenter); i->setTextAlignment(VALUE_COLUMN, Qt::AlignRight | Qt::AlignVCenter);

enum {
	BACKUP_NEVER,
	BACKUP_DAILY,
	BACKUP_WEEKLY,
	BACKUP_FORTNIGHTLY,
	BACKUP_MONTHLY
};

#define CHANGE_COLUMN				2
#define BUDGET_COLUMN				1
#define VALUE_COLUMN				3

#define IS_DEBT(account) (account->accountType() == ASSETS_TYPE_LIABILITIES || account->accountType() == ASSETS_TYPE_CREDIT_CARD)

QString last_document_directory, last_associated_file_directory, last_picture_directory;

QColor createExpenseColor(QColor base_color) {
	qreal r, g, b;
	base_color.getRgbF(&r, &g, &b);
	if(r >= 0.8) {
		g /= 1.5;
		b /= 1.5;
		r = 1.0;
	} else {
		if(r >= 0.6) r = 1.0;
		else r += 0.4;
	}
	base_color.setRgbF(r, g, b);
	return base_color;
}
QColor createIncomeColor(QColor base_color) {
	qreal r, g, b;
	base_color.getRgbF(&r, &g, &b);
	if(g >= 0.8) {
		r /= 1.5;
		b /= 1.5;
		g = 1.0;
	} else {
		if(g >= 0.7) g = 1.0;
		else g += 0.3;
	}
	base_color.setRgbF(r, g, b);
	return base_color;
}
QColor createTransferColor(QColor base_color) {
	qreal r, g, b;
	base_color.getRgbF(&r, &g, &b);
	if(b >= 0.8) {
		g /= 1.5;
		r /= 1.5;
		b = 1.0;
	} else {
		if(b >= 0.7) b = 1.0;
		else b += 0.3;
	}
	base_color.setRgbF(r, g, b);
	return base_color;
}

QColor createExpenseColor(QTreeWidgetItem *i, int = 0) {return createExpenseColor(i->treeWidget()->viewport()->palette().color(i->treeWidget()->viewport()->foregroundRole()));}
QColor createIncomeColor(QTreeWidgetItem *i, int = 0) {return createIncomeColor(i->treeWidget()->viewport()->palette().color(i->treeWidget()->viewport()->foregroundRole()));}
QColor createTransferColor(QTreeWidgetItem *i, int = 0) {return createTransferColor(i->treeWidget()->viewport()->palette().color(i->treeWidget()->viewport()->foregroundRole()));}

QColor createExpenseColor(QWidget *w) {return createExpenseColor(w->palette().color(w->foregroundRole()));}
QColor createIncomeColor(QWidget *w) {return createIncomeColor(w->palette().color(w->foregroundRole()));}
QColor createTransferColor(QWidget *w) {return createTransferColor(w->palette().color(w->foregroundRole()));}

void setAccountChangeColor(QTreeWidgetItem *i, double change, bool is_expense) {
	//QColor fg = i->foreground(VALUE_COLUMN).color();
	if((!is_expense && change < 0.0) || (is_expense && change > 0.0)) {
		i->setForeground(CHANGE_COLUMN, createExpenseColor(i, VALUE_COLUMN));
	} else if((is_expense && change < 0.0) || (!is_expense && change > 0.0)) {
		i->setForeground(CHANGE_COLUMN, createIncomeColor(i, VALUE_COLUMN));
	} else {
		i->setForeground(CHANGE_COLUMN, i->treeWidget()->palette().color(i->treeWidget()->foregroundRole()));
	}
}
void setAccountBudgetColor(QTreeWidgetItem *i, double budget_rem, bool is_expense) {
	if(!is_expense) return;
	//QColor fg = i->foreground(VALUE_COLUMN).color();
	if(budget_rem < 0.0) i->setForeground(BUDGET_COLUMN, createExpenseColor(i, VALUE_COLUMN));
	else i->setForeground(BUDGET_COLUMN, createIncomeColor(i, VALUE_COLUMN));
}

void setColumnTextWidth(QTreeWidget *w, int i, QString str) {
	QFontMetrics fm(w->font());
	int tw = fm.width(str) + 10;
	int hw = fm.width(w->headerItem()->text(i)) + 10;
	if(w->columnWidth(i) < tw) w->setColumnWidth(i, tw);
	if(w->columnWidth(i) < hw) w->setColumnWidth(i, hw);
}
void setColumnDateWidth(QTreeWidget *w, int i) {
	setColumnTextWidth(w, i, QLocale().toString(QDate::currentDate(), QLocale::ShortFormat));
}
void setColumnMoneyWidth(QTreeWidget *w, int i, double v = 9999999.99) {
	setColumnTextWidth(w, i, QLocale().toString(v, 'f', MONETARY_DECIMAL_PLACES) + " XXX");
}
void setColumnValueWidth(QTreeWidget *w, int i, double v, int d = -1) {
	setColumnTextWidth(w, i, QLocale().toString(v, 'f', d < 0 ? MONETARY_DECIMAL_PLACES : d));
}
void setColumnStrlenWidth(QTreeWidget *w, int i, int l) {
	setColumnTextWidth(w, i, QString(l, 'h'));
}


QTreeWidgetItem *selectedItem(QTreeWidget *w) {
	QList<QTreeWidgetItem*> list = w->selectedItems();
	if(list.isEmpty()) return NULL;
	return list.first();
}

EqonomizeComboBox::EqonomizeComboBox(QWidget *parent) : QComboBox(parent) {
	block_selected = false;
	connect(this, SIGNAL(activated(int)), this, SLOT(itemActivated(int)));
}
void EqonomizeComboBox::keyPressEvent(QKeyEvent *event) {
	block_selected = true;
	QComboBox::keyPressEvent(event);
	block_selected = false;
	if(!event->isAccepted() && (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)) {
		emit returnPressed();
	}
}
void EqonomizeComboBox::itemActivated(int index) {
	if(!block_selected) {
		emit itemSelected(index);
	}
}


class SecurityListViewItem : public QTreeWidgetItem {
	protected:
		Security *o_security;
	public:
		SecurityListViewItem(Security *sec, QString s1, QString s2 = QString::null, QString s3 = QString::null, QString s4 = QString::null, QString s5 = QString::null, QString s6 = QString::null, QString s7 = QString::null, QString s8 = QString::null, QString s9 = QString::null) : QTreeWidgetItem(UserType), o_security(sec) {
			setText(0, s1);
			setText(1, s2);
			setText(2, s3);
			setText(3, s4);
			setText(4, s5);
			setText(5, s6);
			setText(6, s7);
			setText(7, s8);
			setText(8, s9);
			setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
			setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);
			setTextAlignment(3, Qt::AlignRight | Qt::AlignVCenter);
			setTextAlignment(4, Qt::AlignRight | Qt::AlignVCenter);
			setTextAlignment(5, Qt::AlignRight | Qt::AlignVCenter);
			setTextAlignment(6, Qt::AlignRight | Qt::AlignVCenter);
		}
		bool operator<(const QTreeWidgetItem &i_pre) const {
			int col = 0;
			if(treeWidget()) col = treeWidget()->sortColumn();
			SecurityListViewItem *i = (SecurityListViewItem*) &i_pre;
			switch(col) {
				case 1: {
					if(value < i->value) return true;
					if(value > i->value) return false;
					break;
				}
				case 2: {
					if(shares < i->shares) return true;
					if(shares > i->shares) return false;
					break;
				}
				case 3: {
					if(quote < i->quote) return true;
					if(quote > i->quote) return false;
					break;
				}
				case 4: {
					if(cost < i->cost) return true;
					if(cost > i->cost) return false;
					break;
				}
				case 5: {
					if(profit < i->profit) return true;
					if(profit > i->profit) return false;
					break;
				}
				case 6: {
					if(rate < i->rate) return true;
					if(rate > i->rate) return false;
					break;
				}
				default: {break;}
			}
			return QTreeWidgetItem::operator<(i_pre);
		}
		Security* security() const {return o_security;}
		double cost, value, rate, profit, shares, quote, scost, sprofit;
};

class ScheduleListViewItem : public QTreeWidgetItem {

	Q_DECLARE_TR_FUNCTIONS(ScheduleListViewItem)

	protected:
		ScheduledTransaction *o_strans;
		QDate d_date;
		QColor expenseColor, incomeColor, transferColor;
	public:
		ScheduleListViewItem(QTreeWidget *parent, ScheduledTransaction *strans, const QDate &trans_date);
		bool operator<(const QTreeWidgetItem &i_pre) const;
		ScheduledTransaction *scheduledTransaction() const;
		void setScheduledTransaction(ScheduledTransaction *strans);
		const QDate &date() const;
		void setDate(const QDate &newdate);
};

ScheduleListViewItem::ScheduleListViewItem(QTreeWidget *parent, ScheduledTransaction *strans, const QDate &trans_date) : QTreeWidgetItem(parent, UserType) {
	setTextAlignment(3, Qt::AlignRight | Qt::AlignVCenter);
	//setTextAlignment(4, Qt::AlignCenter);
	//setTextAlignment(5, Qt::AlignCenter);
	setScheduledTransaction(strans);
	setDate(trans_date);
}
bool ScheduleListViewItem::operator<(const QTreeWidgetItem &i_pre) const {
	int col = 0;
	if(treeWidget()) col = treeWidget()->sortColumn();
	ScheduleListViewItem *i = (ScheduleListViewItem*) &i_pre;
	if(col == 0) {
		if(d_date < i->date()) return true;
		if(d_date > i->date()) return false;
		return o_strans->description().localeAwareCompare(i->scheduledTransaction()->description()) < 0;
	} else if(col == 3) {
		double d1 = o_strans->value(true), d2 = i->scheduledTransaction()->value(true);
		if(d1 < d2) return true;
		if(d1 > d2) return false;
	}
	return QTreeWidgetItem::operator<(i_pre);
}
ScheduledTransaction *ScheduleListViewItem::scheduledTransaction() const {
	return o_strans;
}
const QDate &ScheduleListViewItem::date() const {
	return d_date;
}
void ScheduleListViewItem::setDate(const QDate &newdate) {
	d_date = newdate;
	setText(0, QLocale().toString(d_date, QLocale::ShortFormat));
}
void ScheduleListViewItem::setScheduledTransaction(ScheduledTransaction *strans) {
	o_strans = strans;	
	setText(2, strans->description());
	if(strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SINGLE) {
		Transaction *trans = (Transaction*) strans->transaction();
		setText(3, trans->valueString());
		setText(4, trans->fromAccount()->name());
		setText(5, trans->toAccount()->name());
		if((trans->type() == TRANSACTION_TYPE_EXPENSE && trans->value() > 0.0) || (trans->type() == TRANSACTION_TYPE_INCOME && trans->value() < 0.0)) {
			if(!expenseColor.isValid()) expenseColor = createExpenseColor(this, 3);
			setForeground(3, expenseColor);
		} else if((trans->type() == TRANSACTION_TYPE_EXPENSE && trans->value() < 0.0) || (trans->type() == TRANSACTION_TYPE_INCOME && trans->value() > 0.0)) {
			if(!incomeColor.isValid()) incomeColor = createIncomeColor(this, 3);
			setForeground(3, incomeColor);
		} else {		
			if(!transferColor.isValid()) transferColor = createTransferColor(this, 3);
			setForeground(3, transferColor);
		}
		switch(trans->type()) {
			case TRANSACTION_TYPE_TRANSFER: {setText(1, QObject::tr("Transfer")); break;}
			case TRANSACTION_TYPE_INCOME: {
				if(((Income*) trans)->security()) setText(1, QObject::tr("Dividend"));
				else setText(1, QObject::tr("Income"));
				break;
			}
			case TRANSACTION_TYPE_EXPENSE: {setText(1, QObject::tr("Expense")); break;}
			case TRANSACTION_TYPE_SECURITY_BUY: {setText(1, QObject::tr("Securities Purchase", "Financial security (e.g. stock, mutual fund)")); break;}
			case TRANSACTION_TYPE_SECURITY_SELL: {setText(1, QObject::tr("Securities Sale", "Financial security (e.g. stock, mutual fund)")); break;}
		}
	} else {
		SplitTransaction *split = (SplitTransaction*) strans->transaction();
		if(split->type() == SPLIT_TRANSACTION_TYPE_LOAN) {
			setText(1, QObject::tr("Debt Payment"));
			setText(3, split->valueString(-split->value()));
			if(!expenseColor.isValid()) expenseColor = createExpenseColor(this, 3);
			setForeground(3, expenseColor);
			setText(4, ((DebtPayment*) split)->account()->name());
			setText(5, ((DebtPayment*) split)->loan()->name());
		} else {
			bool b_reverse = false;
			if(split->isIncomesAndExpenses()) {
				if(split->cost() > 0.0) {
					if(!expenseColor.isValid()) expenseColor = createExpenseColor(this, 3);
					setForeground(3, expenseColor);
					setText(3, split->valueString(split->cost()));
					b_reverse = true;
				} else {
					if(!incomeColor.isValid()) incomeColor = createIncomeColor(this, 3);
					setForeground(3, incomeColor);
					setText(3, split->valueString(split->income()));
				}
			} else {
				if(!transferColor.isValid()) transferColor = createTransferColor(this, 3);
				setForeground(3, transferColor);
				setText(3, split->valueString());
			}
			if(split->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ITEMS) {
				setText(b_reverse ? 4 : 5, ((MultiItemTransaction*) split)->account()->name());
				setText(b_reverse ? 5 : 4, ((MultiItemTransaction*) split)->fromAccountsString());
				if(((MultiItemTransaction*) split)->transactiontype() == TRANSACTION_TYPE_INCOME) setText(1, QObject::tr("Income"));
				else if(((MultiItemTransaction*) split)->transactiontype() == TRANSACTION_TYPE_EXPENSE) setText(1, QObject::tr("Expense"));
				else setText(1, QObject::tr("Split Transaction"));
			} else {
				setText(b_reverse ? 4 : 5, ((MultiAccountTransaction*) split)->category()->name());
				setText(b_reverse ? 5 : 4, ((MultiAccountTransaction*) split)->accountsString());
				if(((MultiAccountTransaction*) split)->transactiontype() == TRANSACTION_TYPE_INCOME) setText(1, QObject::tr("Income"));
				else setText(1, QObject::tr("Expense"));
			}
		}
	}
	setText(6, strans->comment());
}

class ConfirmScheduleListViewItem : public QTreeWidgetItem {
	Q_DECLARE_TR_FUNCTIONS(ConfirmScheduleListViewItem)
	protected:
		Transactions *o_trans;
		QColor expenseColor, incomeColor, transferColor;
	public:
		ConfirmScheduleListViewItem(QTreeWidget *parent, Transactions *trans);
		bool operator<(const QTreeWidgetItem &i_pre) const;
		Transactions *transaction() const;
		void setTransaction(Transactions *trans);
};

ConfirmScheduleListViewItem::ConfirmScheduleListViewItem(QTreeWidget *parent, Transactions *trans) : QTreeWidgetItem(parent, UserType) {
	setTransaction(trans);
	setTextAlignment(3, Qt::AlignRight | Qt::AlignVCenter);
}
bool ConfirmScheduleListViewItem::operator<(const QTreeWidgetItem &i_pre) const {
	int col = 0;
	if(treeWidget()) col = treeWidget()->sortColumn();
	ConfirmScheduleListViewItem *i = (ConfirmScheduleListViewItem*) &i_pre;
	if(col == 0) {
		return o_trans->date() < i->transaction()->date();
	} else if(col == 3) {
		return o_trans->value(true) < i->transaction()->value(true);
	}
	return QTreeWidgetItem::operator<(i_pre);
}
Transactions *ConfirmScheduleListViewItem::transaction() const {
	return o_trans;
}
void ConfirmScheduleListViewItem::setTransaction(Transactions *transs) {
	o_trans = transs;
	setText(0, QLocale().toString(transs->date(), QLocale::ShortFormat));
	if(transs->generaltype() == GENERAL_TRANSACTION_TYPE_SINGLE) {
		Transaction *trans = (Transaction*) transs;
		switch(trans->type()) {
			case TRANSACTION_TYPE_TRANSFER: {setText(1, tr("Transfer")); break;}
			case TRANSACTION_TYPE_INCOME: {
				if(((Income*) trans)->security()) setText(1, tr("Dividend"));
				else setText(1, tr("Income"));
				break;
			}
			case TRANSACTION_TYPE_EXPENSE: {setText(1, tr("Expense")); break;}
			case TRANSACTION_TYPE_SECURITY_BUY: {setText(1, tr("Securities Purchase", "Financial security (e.g. stock, mutual fund)")); break;}
			case TRANSACTION_TYPE_SECURITY_SELL: {setText(1, tr("Securities Sale", "Financial security (e.g. stock, mutual fund)")); break;}
		}
		if((trans->type() == TRANSACTION_TYPE_EXPENSE && trans->value() > 0.0) || (trans->type() == TRANSACTION_TYPE_INCOME && trans->value() < 0.0)) {
			if(!expenseColor.isValid()) expenseColor = createExpenseColor(this, 3);
			setForeground(3, expenseColor);
		} else if((trans->type() == TRANSACTION_TYPE_EXPENSE && trans->value() < 0.0) || (trans->type() == TRANSACTION_TYPE_INCOME && trans->value() > 0.0)) {
			if(!incomeColor.isValid()) incomeColor = createIncomeColor(this, 3);
			setForeground(3, incomeColor);
		} else {
			if(!transferColor.isValid()) transferColor = createTransferColor(this, 3);
			setForeground(3, transferColor);
		}
		setText(3, trans->valueString());
	} else {
		SplitTransaction *split = (SplitTransaction*) transs;
		if(split->type() == SPLIT_TRANSACTION_TYPE_LOAN) {
			setText(3, split->valueString(-split->value()));
			if(!expenseColor.isValid()) expenseColor = createExpenseColor(this, 3);
			setForeground(3, expenseColor);
			setText(1, tr("Debt Payment"));
		} else {
			if(split->isIncomesAndExpenses()) {
				if(split->cost() > 0.0) {
					if(!expenseColor.isValid()) expenseColor = createExpenseColor(this, 3);
					setForeground(3, expenseColor);
					setText(3, split->valueString(split->cost()));
				} else {
					if(!incomeColor.isValid()) incomeColor = createIncomeColor(this, 3);
					setForeground(3, incomeColor);
					setText(3, split->valueString(split->income()));
				}
			} else {
				if(!transferColor.isValid()) transferColor = createTransferColor(this, 3);
				setForeground(3, transferColor);
				setText(3, split->valueString());
			}
			if(split->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ITEMS) {
				if(((MultiItemTransaction*) split)->transactiontype() == TRANSACTION_TYPE_INCOME) setText(1, QObject::tr("Income"));
				else if(((MultiItemTransaction*) split)->transactiontype() == TRANSACTION_TYPE_EXPENSE) setText(1, QObject::tr("Expense"));
				else setText(1, QObject::tr("Split Transaction"));
			} else {
				if(((MultiAccountTransaction*) split)->transactiontype() == TRANSACTION_TYPE_INCOME) setText(1, QObject::tr("Income"));
				else setText(1, QObject::tr("Expense"));
			}
		}
	}
	setText(2, transs->description());
}


class SecurityTransactionListViewItem : public QTreeWidgetItem {
	public:
		SecurityTransactionListViewItem(QString, QString=QString::null, QString=QString::null, QString=QString::null);
		bool operator<(const QTreeWidgetItem &i_pre) const;
		SecurityTransaction *trans;
		Income *div;
		ReinvestedDividend *rediv;
		ScheduledTransaction *strans, *sdiv, *srediv;
		SecurityTrade *ts;
		QDate date;
		double shares, value;
};

SecurityTransactionListViewItem::SecurityTransactionListViewItem(QString s1, QString s2, QString s3, QString s4) : QTreeWidgetItem(UserType), trans(NULL), div(NULL), rediv(NULL), strans(NULL), sdiv(NULL), srediv(NULL), ts(NULL), shares(-1.0), value(-1.0) {
	setText(0, s1);
	setText(1, s2);
	setText(2, s3);
	setText(3, s4);
	setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
	setTextAlignment(1, Qt::AlignLeft | Qt::AlignVCenter);
	setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);
	setTextAlignment(3, Qt::AlignRight | Qt::AlignVCenter);
}
bool SecurityTransactionListViewItem::operator<(const QTreeWidgetItem &i_pre) const {
	int col = 0;
	if(treeWidget()) col = treeWidget()->sortColumn();
	SecurityTransactionListViewItem *i = (SecurityTransactionListViewItem*) &i_pre;
	if(col == 0) {
		return date < i->date;
	} else if(col == 2) {
		return value < i->value;
	} else if(col == 2) {
		return shares < i->shares;
	}
	return QTreeWidgetItem::operator<(i_pre);
}
class QuotationListViewItem : public QTreeWidgetItem {
	public:
		QuotationListViewItem(const QDate &date_, double value_, int decimals_);
		bool operator<(const QTreeWidgetItem &i_pre) const;
		QDate date;
		double value;
		int decimals;
};

QuotationListViewItem::QuotationListViewItem(const QDate &date_, double value_, int decimals_) : QTreeWidgetItem(UserType), date(date_), value(value_), decimals(decimals_) {
	setText(0, QLocale().toString(date, QLocale::ShortFormat));
	setText(1, format_money(value, decimals));
	setTextAlignment(0, Qt::AlignRight | Qt::AlignVCenter);
	setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
}
bool QuotationListViewItem::operator<(const QTreeWidgetItem &i_pre) const {
	QuotationListViewItem *i = (QuotationListViewItem*) &i_pre;
	if(!treeWidget() || treeWidget()->sortColumn() == 0) {
		return date < i->date;
	} else {
		return value < i->value;
	}
	return QTreeWidgetItem::operator<(i_pre);
}

RefundDialog::RefundDialog(Transactions *trans, QWidget *parent) : QDialog(parent, 0), transaction(trans) {

	setModal(true);

	QVBoxLayout *box1 = new QVBoxLayout(this);
	
	QGridLayout *layout = new QGridLayout();
	box1->addLayout(layout);

	layout->addWidget(new QLabel(tr("Date:"), this), 0, 0);
	dateEdit = new EqonomizeDateEdit(this);
	dateEdit->setCalendarPopup(true);
	layout->addWidget(dateEdit, 0, 1);
	
	TransactionType t_type = TRANSACTION_TYPE_EXPENSE;
	Transaction *curtrans = NULL;
	if(trans->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT) {
		if(((MultiAccountTransaction*) trans)->category()->type() == ACCOUNT_TYPE_INCOMES) t_type = TRANSACTION_TYPE_INCOME;
		curtrans = ((MultiAccountTransaction*) trans)->at(0);
	} else if(trans->generaltype() == GENERAL_TRANSACTION_TYPE_SINGLE) {
		t_type = ((Transaction*) trans)->type();
		curtrans = (Transaction*) trans;
	}
	
	if(t_type == TRANSACTION_TYPE_INCOME) setWindowTitle(tr("Repayment"));
	else setWindowTitle(tr("Refund"));

	if(t_type == TRANSACTION_TYPE_INCOME) layout->addWidget(new QLabel(tr("Cost:"), this), 1, 0);
	else layout->addWidget(new QLabel(tr("Income:"), this), 1, 0);
	valueEdit = new EqonomizeValueEdit(trans->value(), false, true, this, trans->budget());
	valueEdit->setCurrency(trans->currency());
	layout->addWidget(valueEdit, 1, 1);

	layout->addWidget(new QLabel(tr("Quantity:"), this), 2, 0);
	quantityEdit = new EqonomizeValueEdit(trans->quantity(), QUANTITY_DECIMAL_PLACES, false, false, this, trans->budget());
	layout->addWidget(quantityEdit, 2, 1);

	layout->addWidget(new QLabel(tr("Account:"), this), 3, 0);
	accountCombo = new AccountComboBox(ACCOUNT_TYPE_ASSETS, trans->budget());
	accountCombo->updateAccounts();
	if(t_type == TRANSACTION_TYPE_EXPENSE) accountCombo->setCurrentAccount(((Expense*) curtrans)->from());
	else if(t_type == TRANSACTION_TYPE_INCOME) accountCombo->setCurrentAccount(((Income*) curtrans)->to());
	layout->addWidget(accountCombo, 3, 1);

	layout->addWidget(new QLabel(tr("Comments:"), this), 4, 0);
	commentsEdit = new QLineEdit(this);
	if(t_type == TRANSACTION_TYPE_INCOME) commentsEdit->setText(tr("Repayment"));
	else commentsEdit->setText(tr("Refund"));
	layout->addWidget(commentsEdit, 4, 1);
	
	connect(dateEdit, SIGNAL(returnPressed()), valueEdit, SLOT(enterFocus()));
	connect(valueEdit, SIGNAL(returnPressed()), quantityEdit, SLOT(enterFocus()));
	connect(quantityEdit, SIGNAL(returnPressed()), accountCombo, SLOT(setFocus()));
	connect(accountCombo, SIGNAL(accountSelected(Account*)), commentsEdit, SLOT(setFocus()));
	connect(accountCombo, SIGNAL(returnPressed()), commentsEdit, SLOT(setFocus()));
	connect(commentsEdit, SIGNAL(returnPressed()), this, SLOT(accept()));
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	connect(accountCombo, SIGNAL(currentAccountChanged(Account*)), this, SLOT(accountActivated(Account*)));
	box1->addWidget(buttonBox);
	
	dateEdit->setFocus();
	dateEdit->setCurrentSection(QDateTimeEdit::DaySection);

}
void RefundDialog::keyPressEvent(QKeyEvent *e) {
	if(e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) return;
	QDialog::keyPressEvent(e);
}
void RefundDialog::accountActivated(Account *account) {
	if(account) valueEdit->setCurrency(((AssetsAccount*) account)->currency());
}
Transaction *RefundDialog::createRefund() {
	if(!validValues()) return NULL;
	Transaction *trans = NULL;
	AssetsAccount *account = NULL;
	if(accountCombo->currentData().isValid()) account = (AssetsAccount*) accountCombo->currentData().value<void*>();
	if(transaction->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT) {
		trans = ((MultiAccountTransaction*) transaction)->at(0)->copy();
	} else if(transaction->generaltype() == GENERAL_TRANSACTION_TYPE_SINGLE) {
		trans = (Transaction*) transaction->copy();
	}
	if(trans->type() == TRANSACTION_TYPE_EXPENSE) ((Expense*) trans)->setFrom(account);
	else ((Income*) trans)->setTo(account);
	trans->setId(trans->budget()->getNewId());
	trans->setFirstRevision(trans->budget()->revision());
	trans->setTimestamp(QDateTime::currentMSecsSinceEpoch() * 1000);
	trans->setQuantity(-quantityEdit->value());
	trans->setValue(-valueEdit->value());
	trans->setDate(dateEdit->date());
	trans->setComment(commentsEdit->text());
	return trans;
}
void RefundDialog::accept() {
	if(validValues()) {
		QDialog::accept();
	}
}
bool RefundDialog::validValues() {
	if(!dateEdit->date().isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		return false;
	}
	return true;
}

EditSecurityTradeDialog::EditSecurityTradeDialog(Budget *budg, Security *sec, QWidget *parent)  : QDialog(parent, 0), budget(budg) {

	setWindowTitle(tr("Securities Exchange", "Shares of one security directly exchanged for shares of another; Financial security (e.g. stock, mutual fund)"));
	setModal(true);

	QVBoxLayout *box1 = new QVBoxLayout(this);
	QGridLayout *layout = new QGridLayout();
	box1->addLayout(layout);

	layout->addWidget(new QLabel(tr("From security:", "Financial security (e.g. stock, mutual fund)"), this), 0, 0);
	fromSecurityCombo = new EqonomizeComboBox(this);
	fromSecurityCombo->setEditable(false);	
	bool sel = false;
	int i = 0;
	for(SecurityList<Security*>::const_iterator it = budget->securities.constBegin(); it != budget->securities.constEnd(); ++it) {
		Security *c_sec = *it;
		fromSecurityCombo->addItem(c_sec->name(), qVariantFromValue((void*) c_sec));
		if(c_sec == sec) {
			fromSecurityCombo->setCurrentIndex(i);
		} else if(!sel && !sec) {
			for(SecurityList<Security*>::const_iterator it2 = budget->securities.constBegin(); it2 != budget->securities.constEnd(); ++it2) {
				Security *c_sec2 = *it2;
				if(c_sec2 != c_sec && c_sec2->account() == c_sec->account()) {
					fromSecurityCombo->setCurrentIndex(i);
					sel = true;
					break;
				}
			}
		}
		i++;
	}
	layout->addWidget(fromSecurityCombo, 0, 1);

	layout->addWidget(new QLabel(tr("Shares moved:", "Financial shares"), this), 1, 0);
	QHBoxLayout *sharesLayout = new QHBoxLayout();
	fromSharesEdit = new EqonomizeValueEdit(0.0, sec ? sec->shares() : 10000.0, 1.0, 0.0, sec ? sec->decimals() : 4, false, this, budget);
	fromSharesEdit->setSizePolicy(QSizePolicy::Expanding, fromSharesEdit->sizePolicy().verticalPolicy());
	sharesLayout->addWidget(fromSharesEdit);
	QPushButton *maxSharesButton = new QPushButton(tr("All"), this);
	maxSharesButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	sharesLayout->addWidget(maxSharesButton);
	layout->addLayout(sharesLayout, 1, 1);
	fromSharesEdit->setFocus();

	layout->addWidget(new QLabel(tr("To security:", "Financial security (e.g. stock, mutual fund)"), this), 2, 0);
	toSecurityCombo = new EqonomizeComboBox(this);
	toSecurityCombo->setEditable(false);
	layout->addWidget(toSecurityCombo, 2, 1);

	layout->addWidget(new QLabel(tr("Shares received:", "Financial shares"), this), 3, 0);
	toSharesEdit = new EqonomizeValueEdit(0.0, sec ? sec->decimals() : 4, false, false, this, budget);
	layout->addWidget(toSharesEdit, 3, 1);

	layout->addWidget(new QLabel(tr("Date:"), this), 5, 0);
	dateEdit = new EqonomizeDateEdit(this);
	dateEdit->setCalendarPopup(true);
	layout->addWidget(dateEdit, 5, 1);
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);

	fromSecurityChanged(true);
	toSecurityChanged();
	
	connect(fromSecurityCombo, SIGNAL(returnPressed()), fromSharesEdit, SLOT(enterFocus()));
	connect(fromSecurityCombo, SIGNAL(itemSelected(int)), fromSharesEdit, SLOT(enterFocus()));
	connect(fromSharesEdit, SIGNAL(returnPressed()), toSecurityCombo, SLOT(setFocus()));
	connect(toSecurityCombo, SIGNAL(returnPressed()), toSharesEdit, SLOT(enterFocus()));
	connect(toSecurityCombo, SIGNAL(itemSelected(int)), toSharesEdit, SLOT(enterFocus()));
	connect(toSharesEdit, SIGNAL(returnPressed()), this, SLOT(focusDate()));
	connect(dateEdit, SIGNAL(returnPressed()), this, SLOT(accept()));

	connect(maxSharesButton, SIGNAL(clicked()), this, SLOT(maxShares()));
	connect(fromSecurityCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(fromSecurityChanged()));
	connect(toSecurityCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(toSecurityChanged()));

}
void EditSecurityTradeDialog::keyPressEvent(QKeyEvent *e) {
	if(e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) return;
	QDialog::keyPressEvent(e);
}
void EditSecurityTradeDialog::focusDate() {
	if(!dateEdit) return;
	dateEdit->setFocus();
	dateEdit->setCurrentSection(QDateTimeEdit::DaySection);
}
void EditSecurityTradeDialog::maxShares() {
	fromSharesEdit->setValue(fromSharesEdit->maximum());
}
Security *EditSecurityTradeDialog::selectedFromSecurity() {
	if(!fromSecurityCombo->currentData().isValid()) return NULL;
	return (Security*) fromSecurityCombo->currentData().value<void*>();
}
Security *EditSecurityTradeDialog::selectedToSecurity() {
	if(!toSecurityCombo->currentData().isValid()) return NULL;
	return (Security*) toSecurityCombo->currentData().value<void*>();
}
void EditSecurityTradeDialog::fromSecurityChanged(bool in_init) {
	Security *sec = selectedFromSecurity();
	toSecurityCombo->clear();
	if(sec) {
		fromSharesEdit->setPrecision(sec->decimals());
		fromSharesEdit->setMaximum(sec->shares());
		Security *to_sec = selectedToSecurity();
		int i = 0;
		for(SecurityList<Security*>::const_iterator it = budget->securities.constBegin(); it != budget->securities.constEnd(); ++it) {
			Security *c_sec = *it;
			if(c_sec != sec && c_sec->account() == sec->account()) {
				toSecurityCombo->addItem(c_sec->name(), qVariantFromValue((void*) c_sec));
				if(c_sec == to_sec) {
					toSecurityCombo->setCurrentIndex(i);
				}
			}
			i++;
		}
		if(!in_init && !checkSecurities()) {
			fromSecurityCombo->setCurrentIndex(prev_from_index);
		} else {
			prev_from_index = fromSecurityCombo->currentIndex();
		}
	}
}
void EditSecurityTradeDialog::toSecurityChanged() {
	Security *sec = selectedToSecurity();
	if(sec) {
		toSharesEdit->setPrecision(sec->decimals());
	}
}
void EditSecurityTradeDialog::setSecurityTrade(SecurityTrade *ts) {
	dateEdit->setDate(ts->date);
	int index = fromSecurityCombo->findData(qVariantFromValue((void*) ts->from_security));
	if(index >= 0) fromSecurityCombo->setCurrentIndex(index);
	fromSharesEdit->setMaximum(ts->from_security->shares() + ts->from_shares);
	fromSharesEdit->setValue(ts->from_shares);
	toSharesEdit->setValue(ts->to_shares);
	index = fromSecurityCombo->findData(qVariantFromValue((void*) ts->to_security));
	if(index >= 0) toSecurityCombo->setCurrentIndex(index);
}
SecurityTrade *EditSecurityTradeDialog::createSecurityTrade() {
	if(!validValues()) return NULL;
	return new SecurityTrade(dateEdit->date(), fromSharesEdit->value(), selectedFromSecurity(), toSharesEdit->value(), selectedToSecurity(), budget->getNewId(), budget->revision(), budget->revision());
}
bool EditSecurityTradeDialog::checkSecurities() {
	if(toSecurityCombo->count() == 0) {
		QMessageBox::critical(isVisible() ? this : parentWidget(), tr("Error"), tr("No other security available for exchange in the account.", "Shares of one security directly exchanged for shares of another; Financial security (e.g. stock, mutual fund)"));
		return false;
	}
	return true;
}
void EditSecurityTradeDialog::accept() {
	if(validValues()) {
		QDialog::accept();
	}
}
bool EditSecurityTradeDialog::validValues() {
	if(selectedFromSecurity() == selectedToSecurity()) {
		QMessageBox::critical(this, tr("Error"), tr("Selected to and from securities are the same.", "Financial security (e.g. stock, mutual fund)"));
		return false;
	}
	if(!dateEdit->date().isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		return false;
	}
	if(fromSharesEdit->value() == 0.0 || toSharesEdit->value() == 0.0) {
		QMessageBox::critical(this, tr("Error"), tr("Zero shares not allowed.", "Financial shares"));
		return false;
	}
	return true;
}

EditQuotationsDialog::EditQuotationsDialog(Budget *budg, QWidget *parent) : QDialog(parent, 0), budget(budg) {

	setWindowTitle(tr("Quotes", "Financial quote"));
	setModal(true);
	
	i_quotation_decimals = budget->defaultQuotationDecimals();

	QVBoxLayout *quotationsVLayout = new QVBoxLayout(this);

	titleLabel = new QLabel(tr("Quotes", "Financial quote"), this);
	quotationsVLayout->addWidget(titleLabel);
	QHBoxLayout *quotationsLayout = new QHBoxLayout();
	quotationsVLayout->addLayout(quotationsLayout);

	quotationsView = new QTreeWidget(this);
	quotationsView->setSortingEnabled(true);
	quotationsView->sortByColumn(0, Qt::DescendingOrder);
	quotationsView->setAllColumnsShowFocus(true);
	quotationsView->setColumnCount(2);
	quotationsView->setAlternatingRowColors(true);
	QStringList headers;
	headers << tr("Date");
	headers << tr("Price per Share", "Financial Shares");
	quotationsView->header()->setStretchLastSection(true);
	quotationsView->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow);
	quotationsView->setHeaderLabels(headers);
	setColumnDateWidth(quotationsView, 0);
	setColumnMoneyWidth(quotationsView, 1);
	quotationsView->setRootIsDecorated(false);
	quotationsView->header()->setSectionsClickable(false);
	QSizePolicy sp = quotationsView->sizePolicy();
	sp.setHorizontalPolicy(QSizePolicy::MinimumExpanding);
	quotationsView->setSizePolicy(sp);
	quotationsLayout->addWidget(quotationsView);
	QVBoxLayout *buttonsLayout = new QVBoxLayout();
	quotationsLayout->addLayout(buttonsLayout);
	quotationEdit = new EqonomizeValueEdit(0.0, pow(10, -i_quotation_decimals), 0.0, i_quotation_decimals, true, this, budget);
	buttonsLayout->addWidget(quotationEdit);
	dateEdit = new QDateEdit(this);
	dateEdit->setCalendarPopup(true);
	dateEdit->setDate(QDate::currentDate());
	buttonsLayout->addWidget(dateEdit);
	addButton = new QPushButton(tr("Add"), this);
	buttonsLayout->addWidget(addButton);
	changeButton = new QPushButton(tr("Modify"), this);
	changeButton->setEnabled(false);
	buttonsLayout->addWidget(changeButton);
	deleteButton = new QPushButton(tr("Delete"), this);
	deleteButton->setEnabled(false);
	buttonsLayout->addWidget(deleteButton);
	buttonsLayout->addStretch(1);
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	quotationsVLayout->addWidget(buttonBox);

	connect(quotationsView, SIGNAL(itemSelectionChanged()), this, SLOT(onSelectionChanged()));
	connect(addButton, SIGNAL(clicked()), this, SLOT(addQuotation()));
	connect(changeButton, SIGNAL(clicked()), this, SLOT(changeQuotation()));
	connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteQuotation()));

}
void EditQuotationsDialog::setSecurity(Security *security) {
	quotationsView->clear();
	i_quotation_decimals = security->quotationDecimals();
	QList<QTreeWidgetItem *> items;
	QMap<QDate, double>::const_iterator it_end = security->quotations.end();
	for(QMap<QDate, double>::const_iterator it = security->quotations.constBegin(); it != it_end; ++it) {
		items.append(new QuotationListViewItem(it.key(), it.value(), i_quotation_decimals));
	}
	quotationsView->addTopLevelItems(items);
	quotationsView->setSortingEnabled(true);
	titleLabel->setText(tr("Quotes for %1", "Financial quote").arg(security->name()));
	quotationEdit->setRange(0.0, pow(10, -i_quotation_decimals), i_quotation_decimals);
	quotationEdit->setCurrency(security->currency(), true);
	if(items.isEmpty()) quotationsView->setMinimumWidth(quotationsView->columnWidth(0) + quotationsView->columnWidth(1) + 10);
}
void EditQuotationsDialog::modifyQuotations(Security *security) {
	security->quotations.clear();
	QTreeWidgetItemIterator it(quotationsView);
	QuotationListViewItem *i = (QuotationListViewItem*) *it;
	while(i) {
		security->quotations[i->date] = i->value;
		security->quotations_auto[i->date] = false;
		++it;
		i = (QuotationListViewItem*) *it;
	}
}
void EditQuotationsDialog::onSelectionChanged() {
	QuotationListViewItem *i = (QuotationListViewItem*) selectedItem(quotationsView);
	if(i) {
		changeButton->setEnabled(true);
		deleteButton->setEnabled(true);
		dateEdit->setDate(i->date);
		quotationEdit->setValue(i->value);
	} else {
		changeButton->setEnabled(false);
		deleteButton->setEnabled(false);
	}
}
void EditQuotationsDialog::addQuotation() {
	QDate date = dateEdit->date();
	if(!date.isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		return;
	}
	QTreeWidgetItemIterator it(quotationsView);
	QuotationListViewItem *i = (QuotationListViewItem*) *it;
	while(i) {
		if(i->date == date) {
			i->value = quotationEdit->value();
			i->setText(1, budget->formatMoney(i->value, i->decimals));
			return;
		}
		++it;
		i = (QuotationListViewItem*) *it;
	}
	quotationsView->insertTopLevelItem(0, new QuotationListViewItem(date, quotationEdit->value(), i_quotation_decimals));
	quotationsView->setSortingEnabled(true);
}
void EditQuotationsDialog::changeQuotation() {
	QuotationListViewItem *i = (QuotationListViewItem*) selectedItem(quotationsView);
	if(i == NULL) return;
	QDate date = dateEdit->date();
	if(!date.isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		return;
	}
	QTreeWidgetItemIterator it(quotationsView);
	QuotationListViewItem *i2 = (QuotationListViewItem*) *it;
	while(i2) {
		if(i2->date == date) {
			i2->value = quotationEdit->value();
			i2->setText(1, budget->formatMoney(i2->value, i2->decimals));
			return;
		}
		++it;
		i2 = (QuotationListViewItem*) *it;
	}
	i->date = date;
	i->value = quotationEdit->value();
	i->setText(0, QLocale().toString(date, QLocale::ShortFormat));
	i->setText(1, budget->formatMoney(i->value, i->decimals));
}
void EditQuotationsDialog::deleteQuotation() {
	QuotationListViewItem *i = (QuotationListViewItem*) selectedItem(quotationsView);
	if(i == NULL) return;
	delete i;
}


OverTimeReportDialog::OverTimeReportDialog(Budget *budg, QWidget *parent) : QDialog(parent, Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint) {
	setWindowTitle(tr("Report"));
	setModal(false);
	QVBoxLayout *box1 = new QVBoxLayout(this);
	report = new OverTimeReport(budg, this);
	box1->addWidget(report);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
	buttonBox->button(QDialogButtonBox::Close)->setAutoDefault(false);
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(reject()));
	box1->addWidget(buttonBox);
}
void OverTimeReportDialog::reject() {
	report->saveConfig();
	QDialog::reject();
}
CategoriesComparisonReportDialog::CategoriesComparisonReportDialog(bool extra_parameters, Budget *budg, QWidget *parent) : QDialog(parent, Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint) {
	setWindowTitle(tr("Report"));
	setModal(false);
	QVBoxLayout *box1 = new QVBoxLayout(this);
	report = new CategoriesComparisonReport(budg, this, extra_parameters);
	box1->addWidget(report);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
	buttonBox->button(QDialogButtonBox::Close)->setAutoDefault(false);
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(reject()));
	box1->addWidget(buttonBox);
}
void CategoriesComparisonReportDialog::reject() {
	report->saveConfig();
	QDialog::reject();
}
OverTimeChartDialog::OverTimeChartDialog(bool extra_parameters, Budget *budg, QWidget *parent) : QDialog(parent, Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint) {
	setWindowTitle(tr("Chart"));
	setModal(false);
	QVBoxLayout *box1 = new QVBoxLayout(this);
	chart = new OverTimeChart(budg, this, extra_parameters);
	box1->addWidget(chart);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
	buttonBox->button(QDialogButtonBox::Close)->setAutoDefault(false);
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(reject()));
	box1->addWidget(buttonBox);
}
void OverTimeChartDialog::reject() {
	chart->saveConfig();
	QDialog::reject();
}
CategoriesComparisonChartDialog::CategoriesComparisonChartDialog(Budget *budg, QWidget *parent) : QDialog(parent, Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint) {
	setWindowTitle(tr("Chart"));
	setModal(false);
	QVBoxLayout *box1 = new QVBoxLayout(this);
	chart = new CategoriesComparisonChart(budg, this);
	box1->addWidget(chart);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
	buttonBox->button(QDialogButtonBox::Close)->setAutoDefault(false);
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(reject()));
	box1->addWidget(buttonBox);
}
void CategoriesComparisonChartDialog::reject() {
	chart->saveConfig();
	QDialog::reject();
}

ConfirmScheduleDialog::ConfirmScheduleDialog(bool extra_parameters, Budget *budg, QWidget *parent, QString title) : QDialog(parent, 0), budget(budg), b_extra(extra_parameters) {

	setWindowTitle(title);
	setModal(true);

	QVBoxLayout *box1 = new QVBoxLayout(this);
	//box1->setSizeConstraint(QLayout::SetFixedSize);
	QLabel *label = new QLabel(tr("The following transactions was scheduled to occur today or before today.\nConfirm that they have indeed occurred (or will occur today)."), this);
	box1->addWidget(label);
	QHBoxLayout *box2 = new QHBoxLayout();
	box1->addLayout(box2);
	transactionsView = new QTreeWidget(this);
	transactionsView->sortByColumn(0, Qt::AscendingOrder);
	transactionsView->setAllColumnsShowFocus(true);
	transactionsView->setColumnCount(4);
	transactionsView->setAlternatingRowColors(true);
	QStringList headers;
	headers << tr("Date");
	headers << tr("Type");
	headers << tr("Description", "Transaction description property (transaction title/generic article name)");
	headers << tr("Amount");
	transactionsView->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow);
	transactionsView->setHeaderLabels(headers);
	setColumnDateWidth(transactionsView, 0);
	setColumnStrlenWidth(transactionsView, 1, 15);
	setColumnStrlenWidth(transactionsView, 2, 25);
	setColumnMoneyWidth(transactionsView, 3);
	transactionsView->setRootIsDecorated(false);
	QSizePolicy sp = transactionsView->sizePolicy();
	sp.setHorizontalPolicy(QSizePolicy::MinimumExpanding);
	transactionsView->setSizePolicy(sp);
	box2->addWidget(transactionsView);
	QDialogButtonBox *buttons = new QDialogButtonBox(Qt::Vertical, this);
	editButton = buttons->addButton(tr("Edit…"), QDialogButtonBox::ActionRole);
	editButton->setEnabled(false);
	postponeButton = buttons->addButton(tr("Postpone…"), QDialogButtonBox::ActionRole);
	postponeButton->setEnabled(false);
	removeButton = buttons->addButton(tr("Delete"), QDialogButtonBox::ActionRole);
	removeButton->setEnabled(false);
	box2->addWidget(buttons);
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);
	
	connect(transactionsView, SIGNAL(itemSelectionChanged()), this, SLOT(transactionSelectionChanged()));
	connect(removeButton, SIGNAL(clicked()), this, SLOT(remove()));
	connect(editButton, SIGNAL(clicked()), this, SLOT(edit()));
	connect(postponeButton, SIGNAL(clicked()), this, SLOT(postpone()));
	updateTransactions();

}
void ConfirmScheduleDialog::transactionSelectionChanged() {
	QTreeWidgetItem *i = selectedItem(transactionsView);
	if(i == NULL) {
		editButton->setEnabled(false);
		removeButton->setEnabled(false);
		postponeButton->setEnabled(false);
	} else {
		editButton->setEnabled(true);
		removeButton->setEnabled(true);
		postponeButton->setEnabled(true);
	}
}
void ConfirmScheduleDialog::remove() {
	QTreeWidgetItem *i = selectedItem(transactionsView);
	if(i == NULL) return;
	Transactions *trans = ((ConfirmScheduleListViewItem*) i)->transaction();
	delete trans;
	delete i;
	if(transactionsView->topLevelItemCount() == 0) reject();
}
void ConfirmScheduleDialog::postpone() {
	QTreeWidgetItem *i = selectedItem(transactionsView);
	if(i == NULL) return;
	QDialog *dialog = new QDialog(this, 0);
	dialog->setWindowTitle(tr("Date"));
	QVBoxLayout *box1 = new QVBoxLayout(dialog);
	QCalendarWidget *datePicker = new QCalendarWidget(dialog);
	datePicker->setSelectedDate(QDate::currentDate().addDays(1));
	box1->addWidget(datePicker);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), dialog, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(accept()));
	box1->addWidget(buttonBox);
	if(dialog->exec() == QDialog::Accepted) {
		if(datePicker->selectedDate() > QDate::currentDate()) {
			Transactions *trans = ((ConfirmScheduleListViewItem*) i)->transaction();
			trans->setDate(datePicker->selectedDate());
			budget->addScheduledTransaction(new ScheduledTransaction(budget, trans, NULL));
			delete i;
		} else {
			QMessageBox::critical(this, tr("Error"), tr("Can only postpone to future dates."));
		}
	}
	dialog->deleteLater();
	if(transactionsView->topLevelItemCount() == 0) reject();
}
void ConfirmScheduleDialog::edit() {
	QTreeWidgetItem *i = selectedItem(transactionsView);
	if(i == NULL) return;
	Transactions *transs = ((ConfirmScheduleListViewItem*) i)->transaction();
	Security *security = NULL;
	if(transs->generaltype() == GENERAL_TRANSACTION_TYPE_SINGLE) {
		Transaction *trans = (Transaction*) transs;
		if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY || trans->type() == TRANSACTION_TYPE_SECURITY_SELL) {
			security = ((SecurityTransaction*) trans)->security();
		} else if(trans->type() == TRANSACTION_TYPE_INCOME && ((Income*) trans)->security()) {
			security = ((Income*) trans)->security();
		}
		TransactionEditDialog *dialog = new TransactionEditDialog(b_extra, trans->type(), NULL, false, security, SECURITY_ALL_VALUES, security != NULL, budget, this, true);
		dialog->editWidget->updateAccounts();
		dialog->editWidget->setTransaction(trans);
		//if(trans->type() == TRANSACTION_TYPE_SECURITY_SELL) dialog->editWidget->setMaxSharesDate(QDate::currentDate());
		if(dialog->editWidget->checkAccounts() && dialog->exec() == QDialog::Accepted) {
			if(dialog->editWidget->modifyTransaction(trans)) {
				if(trans->date() > QDate::currentDate()) {
					budget->addScheduledTransaction(new ScheduledTransaction(budget, trans, NULL));
					delete i;
				} else {
					((ConfirmScheduleListViewItem*) i)->setTransaction(trans);
				}
			}
		}
		dialog->deleteLater();
	} else if(transs->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT) {
		if(((SplitTransaction*) transs)->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ITEMS) {
			MultiItemTransaction *split = (MultiItemTransaction*) transs;
			EditMultiItemDialog *dialog = new EditMultiItemDialog(budget, this, NULL, b_extra, true);
			dialog->editWidget->setTransaction(split);
			if(dialog->editWidget->checkAccounts() && dialog->exec() == QDialog::Accepted) {
				MultiItemTransaction *split_new = dialog->editWidget->createTransaction();
				if(split_new) {
					delete split;
					if(split_new->date() > QDate::currentDate()) {
						budget->addScheduledTransaction(new ScheduledTransaction(budget, split_new, NULL));
						delete i;
					} else {
						((ConfirmScheduleListViewItem*) i)->setTransaction(split_new);
					}
				}
			}
			dialog->deleteLater();
		} else if(((SplitTransaction*) transs)->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ACCOUNTS) {
			MultiAccountTransaction *split = (MultiAccountTransaction*) transs;
			EditMultiAccountDialog *dialog = new EditMultiAccountDialog(budget, this, split->transactiontype() == TRANSACTION_TYPE_EXPENSE, b_extra, true);
			dialog->editWidget->setTransaction(split);
			if(dialog->editWidget->checkAccounts() && dialog->exec() == QDialog::Accepted) {
				MultiAccountTransaction *split_new = dialog->editWidget->createTransaction();
				if(split_new) {
					delete split;
					if(split_new->date() > QDate::currentDate()) {
						budget->addScheduledTransaction(new ScheduledTransaction(budget, split_new, NULL));
						delete i;
					} else {
						((ConfirmScheduleListViewItem*) i)->setTransaction(split_new);
					}
				}
			}
			dialog->deleteLater();
		} else if(((SplitTransaction*) transs)->type() == SPLIT_TRANSACTION_TYPE_LOAN) {
			DebtPayment *split = (DebtPayment*) transs;
			EditDebtPaymentDialog *dialog = new EditDebtPaymentDialog(budget, this, NULL, true);
			dialog->editWidget->setTransaction(split);
			if(dialog->editWidget->checkAccounts() && dialog->exec() == QDialog::Accepted) {
				DebtPayment *split_new = dialog->editWidget->createTransaction();
				if(split_new) {
					delete split;
					if(split_new->date() > QDate::currentDate()) {
						budget->addScheduledTransaction(new ScheduledTransaction(budget, split_new, NULL));
						delete i;
					} else {
						((ConfirmScheduleListViewItem*) i)->setTransaction(split_new);
					}
				}
			}
			dialog->deleteLater();
		}
	}
	if(transactionsView->topLevelItemCount() == 0) reject();
}
void ConfirmScheduleDialog::updateTransactions() {
	for(ScheduledTransactionList<ScheduledTransaction*>::iterator it = budget->scheduledTransactions.begin(); it != budget->scheduledTransactions.end();) {
		ScheduledTransaction *strans = *it;
		if(strans->firstOccurrence() < QDate::currentDate() || (QTime::currentTime().hour() >= 18 && strans->firstOccurrence() == QDate::currentDate())) {
			bool b = strans->isOneTimeTransaction();
			Transactions *trans = strans->realize(strans->firstOccurrence());
			if(trans) {
				new ConfirmScheduleListViewItem(transactionsView, trans);
			}
			if(b) budget->removeScheduledTransaction(strans);
			it = budget->scheduledTransactions.begin();
		} else {
			++it;
		}
	}
	transactionsView->setSortingEnabled(true);
	QTreeWidgetItemIterator qit(transactionsView);
	QTreeWidgetItem *i = *qit;
	if(i) i->setSelected(true);
}
Transactions *ConfirmScheduleDialog::firstTransaction() {
	current_index = 0;
	ConfirmScheduleListViewItem *current_item = (ConfirmScheduleListViewItem*) transactionsView->topLevelItem(current_index);
	if(current_item) return current_item->transaction();
	return NULL;
}
Transactions *ConfirmScheduleDialog::nextTransaction() {
	current_index++;
	ConfirmScheduleListViewItem *current_item = (ConfirmScheduleListViewItem*) transactionsView->topLevelItem(current_index);
	if(current_item) return current_item->transaction();
	return NULL;
}

SecurityTransactionsDialog::SecurityTransactionsDialog(Security *sec, Eqonomize *parent, QString title) : QDialog(parent, 0), security(sec), mainWin(parent) {

	setWindowTitle(title);
	setModal(true);

	QVBoxLayout *box1 = new QVBoxLayout(this);
	box1->addWidget(new QLabel(tr("Transactions for %1").arg(security->name()), this));
	QHBoxLayout *box2 = new QHBoxLayout();
	box1->addLayout(box2);
	transactionsView = new EqonomizeTreeWidget(this);
	transactionsView->sortByColumn(0, Qt::DescendingOrder);
	transactionsView->setAllColumnsShowFocus(true);
	transactionsView->setColumnCount(4);	
	QStringList headers;
	headers << tr("Date");
	headers << tr("Type");
	headers << tr("Value");
	headers << tr("Shares", "Financial shares");
	transactionsView->setHeaderLabels(headers);
	setColumnDateWidth(transactionsView, 0);
	setColumnStrlenWidth(transactionsView, 1, 25);
	setColumnMoneyWidth(transactionsView, 2);
	setColumnValueWidth(transactionsView, 3, 999999.99, 4);	
	if(security->transactions.isEmpty()) transactionsView->setMinimumWidth(transactionsView->columnWidth(0) + transactionsView->columnWidth(1) + transactionsView->columnWidth(2) +  transactionsView->columnWidth(3) + 10);
	else transactionsView->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow);
	transactionsView->setRootIsDecorated(false);
	QSizePolicy sp = transactionsView->sizePolicy();
	sp.setHorizontalPolicy(QSizePolicy::MinimumExpanding);
	transactionsView->setSizePolicy(sp);
	box2->addWidget(transactionsView);
	QDialogButtonBox *buttons = new QDialogButtonBox(Qt::Vertical, this);
	editButton = buttons->addButton(tr("Edit…"), QDialogButtonBox::ActionRole);
	editButton->setEnabled(false);
	removeButton = buttons->addButton(tr("Delete"), QDialogButtonBox::ActionRole);
	removeButton->setEnabled(false);
	box2->addWidget(buttons);
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
	buttonBox->button(QDialogButtonBox::Close)->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(reject()));
	box1->addWidget(buttonBox);
	
	connect(transactionsView, SIGNAL(itemSelectionChanged()), this, SLOT(transactionSelectionChanged()));
	connect(transactionsView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(edit(QTreeWidgetItem*)));
	connect(removeButton, SIGNAL(clicked()), this, SLOT(remove()));
	connect(editButton, SIGNAL(clicked()), this, SLOT(edit()));
	updateTransactions();
}
void SecurityTransactionsDialog::transactionSelectionChanged() {
	QTreeWidgetItem *i = selectedItem(transactionsView);
	if(i == NULL) {
		editButton->setEnabled(false);
		removeButton->setEnabled(false);
	} else {
		editButton->setEnabled(true);
		removeButton->setEnabled(true);
	}
}
void SecurityTransactionsDialog::remove() {
	SecurityTransactionListViewItem *i = (SecurityTransactionListViewItem*) selectedItem(transactionsView);
	if(i == NULL) return;
	if(i->trans) {
		SecurityTransaction *trans = i->trans;
		security->budget()->removeTransaction(trans, true);
		mainWin->transactionRemoved(trans);
		delete trans;
	} else if(i->div) {
		Income *div = i->div;
		security->budget()->removeTransaction(div, true);
		mainWin->transactionRemoved(div);
		delete div;
	} else if(i->rediv) {
		ReinvestedDividend *rediv = i->rediv;
		security->budget()->removeTransaction(rediv, true);
		mainWin->transactionRemoved(rediv);
		delete rediv;
	} else if(i->strans) {
		ScheduledTransaction *strans = i->strans;
		security->budget()->removeScheduledTransaction(strans, true);
		mainWin->transactionRemoved(strans);
		delete strans;
	} else if(i->sdiv) {
		ScheduledTransaction *sdiv = i->sdiv;
		security->budget()->removeScheduledTransaction(sdiv, true);
		mainWin->transactionRemoved(sdiv);
		delete sdiv;
	} else if(i->srediv) {
		ScheduledTransaction *srediv = i->srediv;
		security->budget()->removeScheduledTransaction(srediv, true);
		mainWin->transactionRemoved(srediv);
		delete srediv;
	} else if(i->ts) {
		SecurityTrade *ts = i->ts;
		security->budget()->removeSecurityTrade(ts, true);
		mainWin->updateSecurity(ts->from_security);
		mainWin->updateSecurity(ts->to_security);
		mainWin->updateSecurityAccount(ts->from_security->account());
		if(ts->to_security->account() != ts->from_security->account()) {
			mainWin->updateSecurityAccount(ts->to_security->account());
		}
		mainWin->setModified(true);
		delete ts;
	}
	delete i;
}
void SecurityTransactionsDialog::edit() {
	edit(selectedItem(transactionsView));
}
void SecurityTransactionsDialog::edit(QTreeWidgetItem *i_pre) {
	if(i_pre == NULL) return;
	SecurityTransactionListViewItem *i = (SecurityTransactionListViewItem*) i_pre;
	bool b = false;
	if(i->trans) b = mainWin->editTransaction(i->trans, this);
	else if(i->div) b = mainWin->editTransaction(i->div, this);
	else if(i->rediv) b = mainWin->editTransaction(i->rediv, this);
	else if(i->strans) b = mainWin->editScheduledTransaction(i->strans, this);
	else if(i->sdiv) b = mainWin->editScheduledTransaction(i->sdiv, this);
	else if(i->srediv) b = mainWin->editScheduledTransaction(i->srediv, this);
	else if(i->ts) b = mainWin->editSecurityTrade(i->ts, this);
	if(b) updateTransactions();
}
void SecurityTransactionsDialog::updateTransactions() {
	transactionsView->clear();
	QList<QTreeWidgetItem *> items;
	for(SecurityTransactionList<SecurityTransaction*>::const_iterator it = security->transactions.constBegin(); it != security->transactions.constEnd(); ++it) {
		SecurityTransaction *trans = *it;
		SecurityTransactionListViewItem *i = new SecurityTransactionListViewItem(QLocale().toString(trans->date(), QLocale::ShortFormat), QString::null, trans->valueString(), QLocale().toString(trans->shares(), 'f', security->decimals()));
		i->trans = trans;
		i->date = trans->date();
		i->value = trans->value();
		i->shares = trans->shares();
		if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY) i->setText(1, tr("Shares Bought", "Financial shares"));
		else i->setText(1, tr("Shares Sold", "Financial shares"));
		items.append(i);
	}
	for(SecurityTransactionList<Income*>::const_iterator it = security->dividends.constBegin(); it != security->dividends.constEnd(); ++it) {
		Income *div = *it;
		SecurityTransactionListViewItem *i = new SecurityTransactionListViewItem(QLocale().toString(div->date(), QLocale::ShortFormat), tr("Dividend"), div->valueString(), "-");
		i->div = div;
		i->date = div->date();
		i->value = div->value();
		items.append(i);
	}
	for(SecurityTransactionList<ReinvestedDividend*>::const_iterator it = security->reinvestedDividends.constBegin(); it != security->reinvestedDividends.constEnd(); ++it) {
		ReinvestedDividend *rediv = *it;
		SecurityTransactionListViewItem *i = new SecurityTransactionListViewItem(QLocale().toString(rediv->date(), QLocale::ShortFormat), tr("Reinvested Dividend"), rediv->valueString(), QLocale().toString(rediv->shares(), 'f', security->decimals()));
		i->rediv = rediv;
		i->date = rediv->date();
		i->shares = rediv->shares();
		i->value = rediv->value();
		items.append(i);
	}
	for(TradedSharesList<SecurityTrade*>::const_iterator it = security->tradedShares.constBegin(); it != security->tradedShares.constEnd(); ++it) {
		SecurityTrade *ts = *it;
		double shares;
		double ts_value;
		if(ts->from_security == security) {
			shares = ts->from_shares;
		} else {
			shares = ts->to_shares;
		}
		ts_value = shares * security->getQuotation(ts->date);
		SecurityTransactionListViewItem *i = new SecurityTransactionListViewItem(QLocale().toString(ts->date, QLocale::ShortFormat), ts->from_security == security ? tr("Shares Sold (Exchanged)", "Shares of one security directly exchanged for shares of another; Financial shares") :  tr("Shares Bought (Exchanged)", "Shares of one security directly exchanged for shares of another; Financial shares"), security->currency()->formatValue(ts_value), QLocale().toString(shares, 'f', security->decimals()));
		i->ts = ts;
		i->date = ts->date;
		i->shares = shares;
		i->value = ts_value;
		items.append(i);
	}
	for(ScheduledSecurityTransactionList<ScheduledTransaction*>::const_iterator it = security->scheduledTransactions.constBegin(); it != security->scheduledTransactions.constEnd(); ++it) {
		ScheduledTransaction *strans = *it;
		SecurityTransactionListViewItem *i = new SecurityTransactionListViewItem(QLocale().toString(strans->date(), QLocale::ShortFormat), QString::null, strans->transaction()->valueString(), QLocale().toString(((SecurityTransaction*) strans->transaction())->shares(), 'f', security->decimals()));
		i->strans = strans;
		i->date = strans->date();
		i->value = strans->transaction()->value();
		i->shares = ((SecurityTransaction*) strans->transaction())->shares();
		if(strans->recurrence()) {
			if(strans->transactiontype() == TRANSACTION_TYPE_SECURITY_BUY) i->setText(1, tr("Shares Bought (Recurring)", "Financial shares"));
			else i->setText(1, tr("Shares Sold (Recurring)", "Financial shares"));
		} else {
			if(strans->transactiontype() == TRANSACTION_TYPE_SECURITY_BUY) i->setText(1, tr("Shares Bought (Scheduled)", "Financial shares"));
			else i->setText(1, tr("Shares Sold (Scheduled)", "Financial shares"));
		}
		items.append(i);
	}
	for(ScheduledSecurityTransactionList<ScheduledTransaction*>::const_iterator it = security->scheduledDividends.constBegin(); it != security->scheduledDividends.constEnd(); ++it) {
		ScheduledTransaction *strans = *it;
		SecurityTransactionListViewItem *i = new SecurityTransactionListViewItem(QLocale().toString(strans->date(), QLocale::ShortFormat), QString::null, strans->transaction()->valueString(), "-");
		i->srediv = strans;
		i->date = strans->date();
		i->value = strans->transaction()->value();
		if(strans->recurrence()) {
			i->setText(1, tr("Dividend (Recurring)"));
		} else {
			i->setText(1, tr("Dividend (Scheduled)"));
		}
		items.append(i);
	}
	for(ScheduledSecurityTransactionList<ScheduledTransaction*>::const_iterator it = security->scheduledReinvestedDividends.constBegin(); it != security->scheduledReinvestedDividends.constEnd(); ++it) {
		ScheduledTransaction *strans = *it;
		SecurityTransactionListViewItem *i = new SecurityTransactionListViewItem(QLocale().toString(strans->date(), QLocale::ShortFormat), QString::null, strans->transaction()->valueString(), QLocale().toString(((ReinvestedDividend*) strans->transaction())->shares(), 'f', security->decimals()));
		i->sdiv = strans;
		i->date = strans->date();
		i->value = strans->transaction()->value();
		i->shares = ((ReinvestedDividend*) strans->transaction())->shares();
		if(strans->recurrence()) {
			i->setText(1, tr("Reinvested Dividend (Recurring)"));
		} else {
			i->setText(1, tr("Reinvested Dividend (Scheduled)"));
		}
		items.append(i);
	}
	transactionsView->addTopLevelItems(items);
	transactionsView->setSortingEnabled(true);
}

EditSecurityDialog::EditSecurityDialog(Budget *budg, QWidget *parent, QString title, bool allow_account_creation) : QDialog(parent, 0), budget(budg), b_create_accounts(allow_account_creation) {

	setWindowTitle(title);
	setModal(true);
	
	QVBoxLayout *box1 = new QVBoxLayout(this);

	QGridLayout *grid = new QGridLayout();
	box1->addLayout(grid);
	grid->addWidget(new QLabel(tr("Type:"), this), 0, 0);
	typeCombo = new QComboBox(this);
	typeCombo->setEditable(false);
	typeCombo->addItem(tr("Mutual Fund"));
	typeCombo->addItem(tr("Stock", "Financial stock"));
	typeCombo->addItem(tr("Other"));
	grid->addWidget(typeCombo, 0, 1);
	typeCombo->setFocus();
	grid->addWidget(new QLabel(tr("Name:"), this), 1, 0);
	nameEdit = new QLineEdit(this);
	grid->addWidget(nameEdit, 1, 1);
	grid->addWidget(new QLabel(tr("Account:"), this), 2, 0);
	accountCombo = new AccountComboBox(ASSETS_TYPE_SECURITIES, budget, b_create_accounts, false, false, false, true, this);
	accountCombo->updateAccounts();
	grid->addWidget(accountCombo, 2, 1);
	grid->addWidget(new QLabel(tr("Decimals in shares:", "Financial shares"), this), 3, 0);
	decimalsEdit = new QSpinBox(this);
	decimalsEdit->setRange(0, 8);
	decimalsEdit->setSingleStep(1);
	decimalsEdit->setValue(budget->defaultShareDecimals());	
	grid->addWidget(decimalsEdit, 3, 1);
	grid->addWidget(new QLabel(tr("Initial shares:", "Financial shares"), this), 4, 0);
	sharesEdit = new EqonomizeValueEdit(0.0, budget->defaultShareDecimals(), false, false, this, budget);
	grid->addWidget(sharesEdit, 4, 1);
	grid->addWidget(new QLabel(tr("Decimals in quotes:", "Financial quote"), this), 5, 0);
	quotationDecimalsEdit = new QSpinBox(this);
	quotationDecimalsEdit->setRange(0, 8);
	quotationDecimalsEdit->setSingleStep(1);
	quotationDecimalsEdit->setValue(budget->defaultQuotationDecimals());
	grid->addWidget(quotationDecimalsEdit, 5, 1);
	quotationLabel = new QLabel(tr("Initial quote:", "Financial quote"), this);
	grid->addWidget(quotationLabel, 6, 0);
	quotationEdit = new EqonomizeValueEdit(false, this, budget);
	quotationDecimalsChanged(budget->defaultQuotationDecimals());
	grid->addWidget(quotationEdit, 6, 1);
	quotationDateLabel = new QLabel(tr("Date:"), this);
	grid->addWidget(quotationDateLabel, 7, 0);
	quotationDateEdit = new QDateEdit(QDate::currentDate(), this);
	quotationDateEdit->setCalendarPopup(true);
	grid->addWidget(quotationDateEdit, 7, 1);
	grid->addWidget(new QLabel(tr("Description:"), this), 8, 0);
	descriptionEdit = new QTextEdit(this);
	grid->addWidget(descriptionEdit, 9, 0, 1, 2);
	nameEdit->setFocus();
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);
	
	if(accountCombo->currentAccount()) accountActivated(accountCombo->currentAccount());

	connect(decimalsEdit, SIGNAL(valueChanged(int)), this, SLOT(decimalsChanged(int)));
	connect(quotationDecimalsEdit, SIGNAL(valueChanged(int)), this, SLOT(quotationDecimalsChanged(int)));
	connect(accountCombo, SIGNAL(currentAccountChanged(Account*)), this, SLOT(accountActivated(Account*)));
	connect(accountCombo, SIGNAL(newAccountRequested()), this, SLOT(newAccount()));

}
void EditSecurityDialog::accept() {
	if(!checkAccount()) return;
	if(!accountCombo->currentAccount()) return;
	if(nameEdit->text().trimmed().isEmpty()) {
		nameEdit->setFocus();
		QMessageBox::critical(this, tr("Error"), tr("Empty name."));
		return;
	}
	QDialog::accept();
}
void EditSecurityDialog::newAccount() {
	accountCombo->createAccount();
}
void EditSecurityDialog::accountActivated(Account *account) {
	quotationEdit->setCurrency(account ? ((AssetsAccount*) account)->currency() : budget->defaultCurrency(), true);
}
bool EditSecurityDialog::checkAccount() {
	if(!accountCombo->hasAccount()) {
		QMessageBox::critical(isVisible() ? this : parentWidget(), tr("Error"), tr("No suitable account available."));
		return false;
	}
	return true;
}
void EditSecurityDialog::decimalsChanged(int i) {
	sharesEdit->setPrecision(i);
}
void EditSecurityDialog::quotationDecimalsChanged(int i) {
	quotationEdit->setRange(0.0, pow(10, -i), i);
}
Security *EditSecurityDialog::newSecurity() {
	if(!checkAccount()) return NULL;
	SecurityType type;
	switch(typeCombo->currentIndex()) {
		case 1: {type = SECURITY_TYPE_STOCK; break;}
		case 2: {type = SECURITY_TYPE_OTHER; break;}
		default: {type = SECURITY_TYPE_MUTUAL_FUND; break;}
	}
	Security *security = new Security(budget, (AssetsAccount*) accountCombo->currentAccount(), type, sharesEdit->value(), decimalsEdit->value(), quotationDecimalsEdit->value(), nameEdit->text(), descriptionEdit->toPlainText());
	if(quotationEdit->value() > 0.0) {
		security->setQuotation(quotationDateEdit->date(), quotationEdit->value());
	}
	return security;
}
bool EditSecurityDialog::modifySecurity(Security *security) {
	if(!checkAccount()) return false;
	security->setName(nameEdit->text());
	security->setInitialShares(sharesEdit->value());
	security->setDescription(descriptionEdit->toPlainText());
	security->setAccount((AssetsAccount*) accountCombo->currentAccount());
	security->setDecimals(decimalsEdit->value());
	security->setQuotationDecimals(quotationDecimalsEdit->value());
	switch(typeCombo->currentIndex()) {
		case 1: {security->setType(SECURITY_TYPE_STOCK); break;}
		case 2: {security->setType(SECURITY_TYPE_OTHER); break;}
		default: {security->setType(SECURITY_TYPE_MUTUAL_FUND); break;}
	}
	return true;
}
void EditSecurityDialog::setSecurity(Security *security) {
	nameEdit->setText(security->name());
	quotationEdit->hide();
	quotationDateEdit->hide();
	quotationLabel->hide();
	quotationDateLabel->hide();
	descriptionEdit->setPlainText(security->description());
	decimalsEdit->setValue(security->decimals());
	decimalsChanged(security->decimals());
	quotationDecimalsEdit->setValue(security->quotationDecimals());
	quotationDecimalsChanged(security->quotationDecimals());
	sharesEdit->setValue(security->initialShares());
	accountCombo->setCurrentAccount(security->account());
	switch(security->type()) {
		case SECURITY_TYPE_BOND: {typeCombo->setCurrentIndex(2); break;}
		case SECURITY_TYPE_STOCK: {typeCombo->setCurrentIndex(1); break;}
		case SECURITY_TYPE_OTHER: {typeCombo->setCurrentIndex(2); break;}
		default: {typeCombo->setCurrentIndex(0); break;}
	}
}

class TotalListViewItem : public QTreeWidgetItem {
	public:
		TotalListViewItem(QTreeWidget *parent, QString label1, QString label2 = QString::null, QString label3 = QString::null, QString label4 = QString::null) : QTreeWidgetItem(parent, UserType) {
			setText(0, label1);
			setText(1, label2);
			setText(2, label3);
			setText(3, label4);
			setTextAlignment(BUDGET_COLUMN, Qt::AlignRight | Qt::AlignVCenter);
			setTextAlignment(CHANGE_COLUMN, Qt::AlignRight | Qt::AlignVCenter);
			setTextAlignment(VALUE_COLUMN, Qt::AlignRight | Qt::AlignVCenter);
			/*setBackground(0, parent->viewport()->palette().alternateBase());
			setBackground(1, parent->viewport()->palette().alternateBase());
			setBackground(2, parent->viewport()->palette().alternateBase());
			setBackground(3, parent->viewport()->palette().alternateBase());*/
		}
		TotalListViewItem(QTreeWidget *parent, QTreeWidgetItem *after, QString label1, QString label2 = QString::null, QString label3 = QString::null, QString label4 = QString::null) : QTreeWidgetItem(parent, after, UserType) {
			setText(0, label1);
			setText(1, label2);
			setText(2, label3);
			setText(3, label4);
			setTextAlignment(BUDGET_COLUMN, Qt::AlignRight | Qt::AlignVCenter);
			setTextAlignment(CHANGE_COLUMN, Qt::AlignRight | Qt::AlignVCenter);
			setTextAlignment(VALUE_COLUMN, Qt::AlignRight | Qt::AlignVCenter);
			/*setBackground(0, parent->palette().alternateBase());
			setBackground(1, parent->palette().alternateBase());
			setBackground(2, parent->palette().alternateBase());
			setBackground(3, parent->palette().alternateBase());*/
		}
};

class AccountsListView : public EqonomizeTreeWidget {
	public:
		AccountsListView(QWidget *parent) : EqonomizeTreeWidget(parent) {
			setContentsMargins(25, 25, 25, 25);
		}
};

Eqonomize::Eqonomize() : QMainWindow() {

	in_batch_edit = false;

	expenses_budget = 0.0;
	expenses_budget_diff = 0.0;
	incomes_budget = 0.0;
	incomes_budget_diff = 0.0;
	incomes_accounts_value = 0.0;
	incomes_accounts_change = 0.0;
	expenses_accounts_value = 0.0;
	expenses_accounts_change = 0.0;
	assets_accounts_value = 0.0;
	assets_accounts_change = 0.0;
	total_value = 0.0;
	total_cost = 0.0;
	total_profit = 0.0;
	total_rate = 0.0;
	
	helpDialog = NULL;
	cccDialog = NULL;
	ccrDialog = NULL;
	otcDialog = NULL;
	otrDialog = NULL;
	
	currencyConversionWindow = NULL;
	
	last_picture_directory = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
	last_document_directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
	last_associated_file_directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

	partial_budget = false;

	modified = false;
	modified_auto_save = false;

	budget = new Budget();
	
	QSettings settings;
	settings.beginGroup("GeneralOptions");
	
	b_extra = settings.value("useExtraProperties", true).toBool();
	int initial_period = settings.value("initialPeriod", int(0)).toInt();
	if(initial_period < 0 || initial_period > 4) initial_period = 0;
	
	bool b_uerftd = settings.value("useExchangeRateForTransactionDate", false).toBool();
	budget->setDefaultTransactionConversionRateDate(b_uerftd ? TRANSACTION_CONVERSION_RATE_AT_DATE : TRANSACTION_CONVERSION_LATEST_RATE);

	prev_cur_date = QDate::currentDate();
	QDate curdate = prev_cur_date;
	bool from_enabled = true;

	switch(initial_period) {
		case 0: {
			from_date = budget->firstBudgetDay(curdate);
			to_date = curdate;
			break;
		}
		case 1: {
			from_date = budget->firstBudgetDayOfYear(curdate);
			to_date = curdate;
			break;
		}
		case 2: {
			from_date = budget->firstBudgetDay(curdate);
			to_date = budget->lastBudgetDay(curdate);
			break;
		}
		case 3: {
			from_date = budget->firstBudgetDayOfYear(curdate);
			to_date = from_date.addDays(curdate.daysInYear() - 1);
			break;
		}
		case 4: {
			from_date = QDate::fromString(settings.value("initialFromDate").toString(), Qt::ISODate);
			if(!from_date.isValid()) from_date = budget->firstBudgetDay(curdate);
			from_enabled = settings.value("initialFromDateEnabled", false).toBool();
			to_date = QDate::fromString(settings.value("initialToDate").toString(), Qt::ISODate);
			if(!to_date.isValid()) to_date = from_date.addDays(from_date.daysInMonth() - 1);
			break;
		}
	}
	
	frommonth_begin = budget->firstBudgetDay(from_date);
	prevmonth_begin = budget->firstBudgetDay(to_date);
	budget->addBudgetMonthsSetFirst(prevmonth_begin, -1);

	setAcceptDrops(true);

	QWidget *w_top = new QWidget(this);
	setCentralWidget(w_top);

	QVBoxLayout *topLayout = new QVBoxLayout(w_top);
	
	tabs = new QTabWidget(w_top);
	topLayout->addWidget(tabs);
	tabs->setDocumentMode(true);
	tabs->setIconSize(tabs->iconSize() * 1.5);

	accounts_page = new QWidget(this);
	tabs->addTab(accounts_page, LOAD_ICON("eqz-account"), tr("Accounts && Categories"));
	expenses_page = new QWidget(this);
	tabs->addTab(expenses_page, LOAD_ICON("eqz-expense"), tr("Expenses"));
	incomes_page = new QWidget(this);
	tabs->addTab(incomes_page, LOAD_ICON("eqz-income"), tr("Incomes"));
	transfers_page = new QWidget(this);
	tabs->addTab(transfers_page, LOAD_ICON("eqz-transfer"), tr("Transfers"));
	securities_page = new QWidget(this);
	tabs->addTab(securities_page, LOAD_ICON("eqz-security"), tr("Securities", "Financial security (e.g. stock, mutual fund)"));
	schedule_page = new QWidget(this);
	tabs->addTab(schedule_page, LOAD_ICON("eqz-schedule"), tr("Schedule"));

	connect(tabs, SIGNAL(currentChanged(int)), this, SLOT(onPageChange(int)));

	QVBoxLayout *accountsLayout = new QVBoxLayout(accounts_page);
	accountsLayout->setContentsMargins(0, 0, 0, 0);

	QVBoxLayout *accountsLayoutView = new QVBoxLayout();
	accountsLayout->addLayout(accountsLayoutView);
	accountsView = new AccountsListView(accounts_page);
	accountsView->setSortingEnabled(false);
	accountsView->setAllColumnsShowFocus(true);
	accountsView->setColumnCount(4);
	QStringList accountsViewHeaders;
	accountsViewHeaders << tr("Account / Category");
	accountsViewHeaders << tr("Remaining Budget");
	//: Noun, how much the account balance has changed
	accountsViewHeaders << tr("Change");
	accountsViewHeaders << tr("Total");
	accountsView->setHeaderLabels(accountsViewHeaders);
	accountsView->setRootIsDecorated(false);
	accountsView->header()->setSectionsClickable(false);
	accountsView->header()->setStretchLastSection(false);
	accountsView->setDragDropMode(QAbstractItemView::InternalMove);
	accountsView->setDragEnabled(true);
	QSizePolicy sp = accountsView->sizePolicy();
	sp.setVerticalPolicy(QSizePolicy::MinimumExpanding);
	accountsView->setSizePolicy(sp);
	QFontMetrics fm(accountsView->font());
	accountsView->header()->setSectionResizeMode(QHeaderView::Fixed);
	accountsView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	setColumnTextWidth(accountsView, BUDGET_COLUMN, tr("%2 of %1", "%1: budget; %2: remaining budget").arg(budget->formatMoney(99999999.99)).arg(budget->formatMoney(99999999.99)));
	setColumnMoneyWidth(accountsView, CHANGE_COLUMN, 999999999999.99);
	setColumnMoneyWidth(accountsView, VALUE_COLUMN, 999999999999.99);
	assetsItem = new TotalListViewItem(accountsView, tr("Assets"), QString::null, budget->formatMoney(0.0), budget->formatMoney(0.0) + " ");
	assetsItem->setIcon(0, LOAD_ICON("eqz-account"));
	liabilitiesItem = new TotalListViewItem(accountsView, assetsItem, tr("Liabilities"), QString::null, budget->formatMoney(0.0), budget->formatMoney(0.0) + " ");
	liabilitiesItem->setIcon(0, LOAD_ICON("eqz-liabilities"));
	incomesItem = new TotalListViewItem(accountsView, liabilitiesItem, tr("Incomes"), "-", budget->formatMoney(0.0), budget->formatMoney(0.0) + " ");
	incomesItem->setIcon(0, LOAD_ICON("eqz-income"));
	expensesItem = new TotalListViewItem(accountsView, incomesItem, tr("Expenses"), "-", budget->formatMoney(0.0), budget->formatMoney(0.0) + " ");
	expensesItem->setIcon(0, LOAD_ICON("eqz-expense"));
	assetsItem->setFlags(assetsItem->flags() & ~Qt::ItemIsDragEnabled);
	assetsItem->setFlags(assetsItem->flags() & ~Qt::ItemIsDropEnabled);
	liabilitiesItem->setFlags(liabilitiesItem->flags() & ~Qt::ItemIsDragEnabled);
	liabilitiesItem->setFlags(liabilitiesItem->flags() & ~Qt::ItemIsDropEnabled);
	expensesItem->setFlags(expensesItem->flags() & ~Qt::ItemIsDragEnabled);
	incomesItem->setFlags(incomesItem->flags() & ~Qt::ItemIsDragEnabled);
	QFont itemfont = assetsItem->font(0);
	itemfont.setWeight(QFont::DemiBold);
	assetsItem->setFont(0, itemfont); liabilitiesItem->setFont(0, itemfont); incomesItem->setFont(0, itemfont); expensesItem->setFont(0, itemfont);
	assetsItem->setFont(1, itemfont); liabilitiesItem->setFont(1, itemfont); incomesItem->setFont(1, itemfont); expensesItem->setFont(1, itemfont);
	assetsItem->setFont(2, itemfont); liabilitiesItem->setFont(2, itemfont); incomesItem->setFont(2, itemfont); expensesItem->setFont(2, itemfont);
	assetsItem->setFont(3, itemfont); liabilitiesItem->setFont(3, itemfont); incomesItem->setFont(3, itemfont); expensesItem->setFont(3, itemfont);
	accountsLayoutView->addWidget(accountsView);
	QHBoxLayout *accountsLayoutFooter = new QHBoxLayout();
	accountsLayoutView->addLayout(accountsLayoutFooter);
	accountsLayoutFooter->addStretch(1);
	footer1 = new QLabel(QString("* ") + tr("Includes budgeted transactions"), accounts_page);
	accountsLayoutFooter->addWidget(footer1);
	footer1->hide();

	accountsTabs = new QTabWidget(accounts_page);
	accountsTabs->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

	QWidget *periodWidget = new QWidget(accounts_page);
	accountsTabs->addTab(periodWidget, tr("Period"));
	QVBoxLayout *accountsPeriodLayout = new QVBoxLayout(periodWidget);
	QHBoxLayout *accountsPeriodLayout2 = new QHBoxLayout();
	accountsPeriodLayout->addLayout(accountsPeriodLayout2);
	accountsPeriodFromButton = new QCheckBox(tr("From"), periodWidget);
	accountsPeriodFromButton->setChecked(from_enabled);
	accountsPeriodLayout2->addWidget(accountsPeriodFromButton);
	accountsPeriodFromEdit = new QDateEdit(periodWidget);
	accountsPeriodFromEdit->setCalendarPopup(true);
	accountsPeriodFromEdit->setDate(from_date);
	accountsPeriodFromEdit->setEnabled(true);
	sp = accountsPeriodFromEdit->sizePolicy();
	sp.setHorizontalPolicy(QSizePolicy::Expanding);
	accountsPeriodFromEdit->setSizePolicy(sp);
	accountsPeriodLayout2->addWidget(accountsPeriodFromEdit);
	accountsPeriodLayout2->addWidget(new QLabel(tr("To"), periodWidget));
	accountsPeriodToEdit = new QDateEdit(periodWidget);
	accountsPeriodToEdit->setCalendarPopup(true);
	accountsPeriodToEdit->setDate(to_date);
	accountsPeriodToEdit->setEnabled(true);
	accountsPeriodToEdit->setSizePolicy(sp);
	accountsPeriodLayout2->addWidget(accountsPeriodToEdit);
	QHBoxLayout *accountsPeriodLayout3 = new QHBoxLayout();
	accountsPeriodLayout->addLayout(accountsPeriodLayout3);
	QPushButton *prevYearButton = new QPushButton(LOAD_ICON("eqz-previous-year"), "", periodWidget);	
	accountsPeriodLayout2->addWidget(prevYearButton);
	QPushButton *prevMonthButton = new QPushButton(LOAD_ICON("eqz-previous-month"), "", periodWidget);
	accountsPeriodLayout2->addWidget(prevMonthButton);
	QPushButton *nextMonthButton = new QPushButton(LOAD_ICON("eqz-next-month"), "", periodWidget);
	accountsPeriodLayout2->addWidget(nextMonthButton);
	QPushButton *nextYearButton = new QPushButton(LOAD_ICON("eqz-next-year"), "", periodWidget);
	accountsPeriodLayout2->addWidget(nextYearButton);
	QPushButton *accountsPeriodButton = new QPushButton(tr("Select Period"), periodWidget);
	QMenu *accountsPeriodMenu = new QMenu(this);	
	ActionAP_1 = accountsPeriodMenu->addAction(tr("Current Month"));
	ActionAP_2 = accountsPeriodMenu->addAction(tr("Current Year"));
	ActionAP_3 = accountsPeriodMenu->addAction(tr("Current Whole Month"));
	ActionAP_4 = accountsPeriodMenu->addAction(tr("Current Whole Year"));
	ActionAP_5 = accountsPeriodMenu->addAction(tr("Whole Past Month"));
	ActionAP_6 = accountsPeriodMenu->addAction(tr("Whole Past Year"));
	ActionAP_7 = accountsPeriodMenu->addAction(tr("Previous Month"));
	ActionAP_8 = accountsPeriodMenu->addAction(tr("Previous Year"));
	accountsPeriodButton->setMenu(accountsPeriodMenu);	
	accountsPeriodLayout3->addWidget(accountsPeriodButton);
	partialBudgetButton = new QCheckBox(tr("Show partial budget"), periodWidget);
	accountsPeriodLayout3->addWidget(partialBudgetButton);
	accountsPeriodLayout3->addStretch(1);

	QWidget *budgetWidget = new QWidget(accounts_page);
	accountsTabs->addTab(budgetWidget, tr("Edit Budget"));
	QVBoxLayout *budgetLayout = new QVBoxLayout(budgetWidget);
	QHBoxLayout *budgetLayout2 = new QHBoxLayout();
	budgetLayout->addLayout(budgetLayout2);
	budgetButton = new QCheckBox(tr("Budget:"), budgetWidget);
	budgetButton->setChecked(false);
	budgetLayout2->addWidget(budgetButton);
	budgetEdit = new EqonomizeValueEdit(false, budgetWidget, budget);
	budgetEdit->setEnabled(false);
	sp = budgetEdit->sizePolicy();
	sp.setHorizontalPolicy(QSizePolicy::Expanding);
	budgetEdit->setSizePolicy(sp);
	budgetEdit->setFocus();
	budgetLayout2->addWidget(budgetEdit);
	budgetLayout2->addWidget(new QLabel(tr("Month:"), budgetWidget));
	budgetMonthEdit = new EqonomizeMonthSelector(budgetWidget);
	sp = budgetMonthEdit->sizePolicy();
	sp.setHorizontalPolicy(QSizePolicy::Expanding);
	budgetMonthEdit->setSizePolicy(sp);
	budgetLayout2->addWidget(budgetMonthEdit);
	QHBoxLayout *budgetLayout3 = new QHBoxLayout();
	budgetLayout->addLayout(budgetLayout3);
	budgetLayout3->addWidget(new QLabel(tr("Result previous month:"), budgetWidget));
	prevMonthBudgetLabel = new QLabel("-", budgetWidget);
	budgetLayout3->addWidget(prevMonthBudgetLabel);
	budgetLayout3->addStretch(1);

	accountsLayout->addWidget(accountsTabs);

	accountPopupMenu = NULL;
	assetsPopupMenu = NULL;

	connect(budgetMonthEdit, SIGNAL(dateChanged(const QDate&)), this, SLOT(budgetMonthChanged(const QDate&)));
	connect(budgetEdit, SIGNAL(valueChanged(double)), this, SLOT(budgetChanged(double)));
	connect(budgetEdit, SIGNAL(returnPressed()), this, SLOT(budgetEditReturnPressed()));
	connect(budgetButton, SIGNAL(toggled(bool)), this, SLOT(budgetToggled(bool)));
	connect(budgetButton, SIGNAL(toggled(bool)), budgetEdit, SLOT(setEnabled(bool)));
	connect(partialBudgetButton, SIGNAL(toggled(bool)), this, SLOT(setPartialBudget(bool)));
	connect(accountsPeriodMenu, SIGNAL(triggered(QAction*)), this, SLOT(periodSelected(QAction*)));
	connect(prevMonthButton, SIGNAL(clicked()), this, SLOT(prevMonth()));
	connect(nextMonthButton, SIGNAL(clicked()), this, SLOT(nextMonth()));
	connect(prevYearButton, SIGNAL(clicked()), this, SLOT(prevYear()));
	connect(nextYearButton, SIGNAL(clicked()), this, SLOT(nextYear()));
	connect(accountsPeriodFromButton, SIGNAL(toggled(bool)), accountsPeriodFromEdit, SLOT(setEnabled(bool)));
	connect(accountsPeriodFromButton, SIGNAL(toggled(bool)), this, SLOT(filterAccounts()));
	connect(accountsPeriodFromEdit, SIGNAL(dateChanged(const QDate&)), this, SLOT(accountsPeriodFromChanged(const QDate&)));
	connect(accountsPeriodToEdit, SIGNAL(dateChanged(const QDate&)), this, SLOT(accountsPeriodToChanged(const QDate&)));
	connect(accountsView, SIGNAL(itemSelectionChanged()), this, SLOT(accountsSelectionChanged()));
	connect(accountsView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(accountExecuted(QTreeWidgetItem*, int)));
	connect(accountsView, SIGNAL(returnPressed(QTreeWidgetItem*)), this, SLOT(accountExecuted(QTreeWidgetItem*)));
	connect(accountsView, SIGNAL(itemMoved(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(accountMoved(QTreeWidgetItem*, QTreeWidgetItem*)));
	accountsView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(accountsView, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(popupAccountsMenu(const QPoint&)));

	expensesLayout = new QVBoxLayout(expenses_page);
	expensesLayout->setContentsMargins(0, 0, 0, 0);
	expensesWidget = new TransactionListWidget(b_extra, TRANSACTION_TYPE_EXPENSE, budget, this, expenses_page);
	connect(expensesWidget, SIGNAL(accountAdded(Account*)), this, SLOT(accountAdded(Account*)));
	connect(expensesWidget, SIGNAL(currenciesModified()), this, SLOT(currenciesModified()));
	expensesLayout->addWidget(expensesWidget);

	incomesLayout = new QVBoxLayout(incomes_page);
	incomesLayout->setContentsMargins(0, 0, 0, 0);
	incomesWidget = new TransactionListWidget(b_extra, TRANSACTION_TYPE_INCOME, budget, this, incomes_page);
	connect(incomesWidget, SIGNAL(accountAdded(Account*)), this, SLOT(accountAdded(Account*)));
	connect(incomesWidget, SIGNAL(currenciesModified()), this, SLOT(currenciesModified()));
	incomesLayout->addWidget(incomesWidget);

	transfersLayout = new QVBoxLayout(transfers_page);
	transfersLayout->setContentsMargins(0, 0, 0, 0);
	transfersWidget = new TransactionListWidget(b_extra, TRANSACTION_TYPE_TRANSFER, budget, this, transfers_page);
	connect(transfersWidget, SIGNAL(accountAdded(Account*)), this, SLOT(accountAdded(Account*)));
	connect(transfersWidget, SIGNAL(currenciesModified()), this, SLOT(currenciesModified()));
	transfersLayout->addWidget(transfersWidget);

	QVBoxLayout *securitiesLayout = new QVBoxLayout(securities_page);
	securitiesLayout->setContentsMargins(0, securitiesLayout->contentsMargins().top(), 0, 0);
	
	QDialogButtonBox *securitiesButtons = new QDialogButtonBox(securities_page);
	newSecurityButton = securitiesButtons->addButton(tr("New Security…", "Financial security (e.g. stock, mutual fund)"), QDialogButtonBox::ActionRole);
	newSecurityButton->setEnabled(true);
	newSecurityTransactionButton = securitiesButtons->addButton(tr("New Transaction"), QDialogButtonBox::ActionRole);
	QMenu *newSecurityTransactionMenu = new QMenu(this);
	newSecurityTransactionButton->setMenu(newSecurityTransactionMenu);
	setQuotationButton = securitiesButtons->addButton(tr("Set Quote…", "Financial quote"), QDialogButtonBox::ActionRole);
	setQuotationButton->setEnabled(false);
	securitiesLayout->addWidget(securitiesButtons);

	QVBoxLayout *securitiesViewLayout = new QVBoxLayout();
	securitiesLayout->addLayout(securitiesViewLayout);
	securitiesView = new EqonomizeTreeWidget(securities_page);
	securitiesView->setSortingEnabled(true);
	securitiesView->sortByColumn(0, Qt::AscendingOrder);
	securitiesView->setAllColumnsShowFocus(true);
	securitiesView->setColumnCount(9);
	QStringList securitiesViewHeaders;
	securitiesViewHeaders << tr("Name");
	securitiesViewHeaders << tr("Value");
	securitiesViewHeaders << tr("Shares", "Financial shares");
	securitiesViewHeaders << tr("Quote", "Financial quote");
	securitiesViewHeaders << tr("Cost");
	securitiesViewHeaders << tr("Profit");
	securitiesViewHeaders << tr("Yearly Rate");
	securitiesViewHeaders << tr("Type");
	securitiesViewHeaders << tr("Account");
	securitiesView->setHeaderLabels(securitiesViewHeaders);
	securitiesView->setRootIsDecorated(false);
	securitiesView->header()->setStretchLastSection(false);
	setColumnStrlenWidth(securitiesView, 0, 25);
	setColumnMoneyWidth(securitiesView, 1);
	setColumnValueWidth(securitiesView, 2, 999999.99, 4);
	setColumnValueWidth(securitiesView, 2, 999999.99, 4);
	setColumnMoneyWidth(securitiesView, 4);
	setColumnMoneyWidth(securitiesView, 5);
	setColumnValueWidth(securitiesView, 6, 99.99);
	setColumnStrlenWidth(securitiesView, 7, 10);
	setColumnStrlenWidth(securitiesView, 8, 10);
	sp = securitiesView->sizePolicy();
	sp.setVerticalPolicy(QSizePolicy::MinimumExpanding);
	securitiesView->setSizePolicy(sp);
	securitiesViewLayout->addWidget(securitiesView);

	securitiesStatLabel = new QLabel(securities_page);
	securitiesViewLayout->addWidget(securitiesStatLabel);

	QGroupBox *periodGroup = new QGroupBox(tr("Statistics Period"), securities_page);
	QVBoxLayout *securitiesPeriodLayout = new QVBoxLayout(periodGroup);
	QHBoxLayout *securitiesPeriodLayout2 = new QHBoxLayout();
	securitiesPeriodLayout->addLayout(securitiesPeriodLayout2);
	securitiesPeriodFromButton = new QCheckBox(tr("From"), periodGroup);
	securitiesPeriodFromButton->setChecked(false);
	securitiesPeriodLayout2->addWidget(securitiesPeriodFromButton);
	securitiesPeriodFromEdit = new QDateEdit(periodGroup);
	securitiesPeriodFromEdit->setCalendarPopup(true);
	securities_from_date.setDate(curdate.year(), 1, 1);
	securitiesPeriodFromEdit->setDate(securities_from_date);
	securitiesPeriodFromEdit->setEnabled(false);
	sp = securitiesPeriodFromEdit->sizePolicy();
	sp.setHorizontalPolicy(QSizePolicy::Expanding);
	securitiesPeriodFromEdit->setSizePolicy(sp);
	securitiesPeriodLayout2->addWidget(securitiesPeriodFromEdit);
	securitiesPeriodLayout2->addWidget(new QLabel(tr("To"), periodGroup));
	securitiesPeriodToEdit = new QDateEdit(periodGroup);
	securitiesPeriodToEdit->setCalendarPopup(true);
	securities_to_date = curdate;
	securitiesPeriodToEdit->setDate(securities_to_date);
	securitiesPeriodToEdit->setEnabled(true);
	securitiesPeriodToEdit->setSizePolicy(sp);
	securitiesPeriodLayout2->addWidget(securitiesPeriodToEdit);
	securitiesLayout->addWidget(periodGroup);
	QHBoxLayout *securitiesPeriodLayout3 = new QHBoxLayout();
	securitiesPeriodLayout2->addLayout(securitiesPeriodLayout3);
	QPushButton *securitiesPrevYearButton = new QPushButton(LOAD_ICON("eqz-previous-year"), "", periodGroup);
	securitiesPeriodLayout3->addWidget(securitiesPrevYearButton);
	QPushButton *securitiesPrevMonthButton = new QPushButton(LOAD_ICON("eqz-previous-month"), "", periodGroup);
	securitiesPeriodLayout3->addWidget(securitiesPrevMonthButton);
	QPushButton *securitiesNextMonthButton = new QPushButton(LOAD_ICON("eqz-next-month"), "", periodGroup);
	securitiesPeriodLayout3->addWidget(securitiesNextMonthButton);
	QPushButton *securitiesNextYearButton = new QPushButton(LOAD_ICON("eqz-next-year"), "", periodGroup);
	securitiesPeriodLayout3->addWidget(securitiesNextYearButton);

	securitiesPopupMenu = NULL;

	updateSecuritiesStatistics();

	connect(securitiesPrevMonthButton, SIGNAL(clicked()), this, SLOT(securitiesPrevMonth()));
	connect(securitiesNextMonthButton, SIGNAL(clicked()), this, SLOT(securitiesNextMonth()));
	connect(securitiesPrevYearButton, SIGNAL(clicked()), this, SLOT(securitiesPrevYear()));
	connect(securitiesNextYearButton, SIGNAL(clicked()), this, SLOT(securitiesNextYear()));
	connect(securitiesPeriodFromButton, SIGNAL(toggled(bool)), securitiesPeriodFromEdit, SLOT(setEnabled(bool)));
	connect(securitiesPeriodFromButton, SIGNAL(toggled(bool)), this, SLOT(updateSecurities()));
	connect(securitiesPeriodFromEdit, SIGNAL(dateChanged(const QDate&)), this, SLOT(securitiesPeriodFromChanged(const QDate&)));
	connect(securitiesPeriodToEdit, SIGNAL(dateChanged(const QDate&)), this, SLOT(securitiesPeriodToChanged(const QDate&)));
	connect(newSecurityButton, SIGNAL(clicked()), this, SLOT(newSecurity()));
	connect(setQuotationButton, SIGNAL(clicked()), this, SLOT(setQuotation()));
	connect(securitiesView, SIGNAL(itemSelectionChanged()), this, SLOT(securitiesSelectionChanged()));
	connect(securitiesView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(securitiesExecuted(QTreeWidgetItem*, int)));
	connect(securitiesView, SIGNAL(returnPressed(QTreeWidgetItem*)), this, SLOT(securitiesExecuted(QTreeWidgetItem*)));
	securitiesView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(securitiesView, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(popupSecuritiesMenu(const QPoint&)));

	QVBoxLayout *scheduleLayout = new QVBoxLayout(schedule_page);
	scheduleLayout->setContentsMargins(0, scheduleLayout->contentsMargins().top(), 0, 0);

	QDialogButtonBox *scheduleButtons = new QDialogButtonBox(schedule_page);
	newScheduleButton = scheduleButtons->addButton(tr("New Schedule"), QDialogButtonBox::ActionRole);
	QMenu *newScheduleMenu = new QMenu(this);
	newScheduleButton->setMenu(newScheduleMenu);
	editScheduleButton = scheduleButtons->addButton(tr("Edit"), QDialogButtonBox::ActionRole);
	editScheduleMenu = new QMenu(this);
	editScheduleButton->setMenu(editScheduleMenu);
	editScheduleButton->setEnabled(false);
	removeScheduleButton = scheduleButtons->addButton(tr("Remove"), QDialogButtonBox::ActionRole);
	removeScheduleMenu = new QMenu(this);
	removeScheduleButton->setMenu(removeScheduleMenu);
	removeScheduleButton->setEnabled(false);
	scheduleLayout->addWidget(scheduleButtons);

	scheduleView = new EqonomizeTreeWidget(schedule_page);
	scheduleView->setAllColumnsShowFocus(true);
	scheduleView->setColumnCount(7);
	QStringList scheduleViewHeaders;
	scheduleViewHeaders << tr("Next Occurrence");
	scheduleViewHeaders << tr("Type");
	scheduleViewHeaders << tr("Description", "Transaction description property (transaction title/generic article name)");
	scheduleViewHeaders << tr("Amount");
	scheduleViewHeaders << tr("From");
	scheduleViewHeaders << tr("To");
	scheduleViewHeaders << tr("Comments");	
	scheduleView->setHeaderLabels(scheduleViewHeaders);
	setColumnDateWidth(scheduleView, 0);
	setColumnStrlenWidth(scheduleView, 1, 15);
	setColumnStrlenWidth(scheduleView, 2, 25);
	setColumnMoneyWidth(scheduleView, 3);
	setColumnStrlenWidth(scheduleView, 4, 15);
	setColumnStrlenWidth(scheduleView, 5, 15);
	scheduleView->setRootIsDecorated(false);
	sp = scheduleView->sizePolicy();
	sp.setVerticalPolicy(QSizePolicy::MinimumExpanding);
	scheduleView->setSizePolicy(sp);
	scheduleLayout->addWidget(scheduleView);
	scheduleView->sortByColumn(0, Qt::AscendingOrder);
	scheduleView->setSortingEnabled(true);

	schedulePopupMenu = NULL;

	connect(scheduleView, SIGNAL(itemSelectionChanged()), this, SLOT(scheduleSelectionChanged()));
	connect(scheduleView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(scheduleExecuted(QTreeWidgetItem*)));
	connect(scheduleView, SIGNAL(returnPressed(QTreeWidgetItem*)), this, SLOT(scheduleExecuted(QTreeWidgetItem*)));
	scheduleView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(scheduleView, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(popupScheduleMenu(const QPoint&)));
	connect(scheduleView, SIGNAL(itemSelectionChanged()), this, SLOT(updateTransactionActions()));

	setupActions();

	switch(initial_period) {
		case 0: {AIPCurrentMonth->setChecked(true); break;}
		case 1: {AIPCurrentYear->setChecked(true); break;}
		case 2: {AIPCurrentWholeMonth->setChecked(true); break;}
		case 3: {AIPCurrentWholeYear->setChecked(true); break;}
		case 4: {AIPRememberLastDates->setChecked(true); break;}
	}
	
	int i_backup_frequency = settings.value("backupFrequency", int(BACKUP_WEEKLY)).toInt();
	
	switch(i_backup_frequency) {
		case BACKUP_NEVER: {ABFNever->setChecked(true); break;}
		case BACKUP_DAILY: {ABFDaily->setChecked(true); break;}		
		case BACKUP_FORTNIGHTLY: {ABFFortnightly->setChecked(true); break;}
		case BACKUP_MONTHLY: {ABFMonthly->setChecked(true); break;}
		default: {ABFWeekly->setChecked(true); break;}
	}

	newScheduleMenu->addAction(ActionNewExpense);
	newScheduleMenu->addAction(ActionNewIncome);
	newScheduleMenu->addAction(ActionNewTransfer);
	newScheduleMenu->addAction(ActionNewDebtPayment);
	newScheduleMenu->addAction(ActionNewMultiItemTransaction);
	editScheduleMenu->addAction(ActionEditSchedule);
	editScheduleMenu->addAction(ActionEditOccurrence);
	removeScheduleMenu->addAction(ActionDeleteSchedule);
	removeScheduleMenu->addAction(ActionDeleteOccurrence);

	newSecurityTransactionMenu->addAction(ActionBuyShares);
	newSecurityTransactionMenu->addAction(ActionSellShares);
	newSecurityTransactionMenu->addAction(ActionNewSecurityTrade);
	newSecurityTransactionMenu->addAction(ActionNewDividend);
	newSecurityTransactionMenu->addAction(ActionNewReinvestedDividend);
	
	settings.endGroup();

	readOptions();

	if(first_run) {
		QFontMetrics fm(accountsView->font());
		int w = fm.width(accountsView->headerItem()->text(0)) + fm.width(QString(15, 'h'));
		accountsView->setMinimumWidth(w + accountsView->columnWidth(BUDGET_COLUMN) + accountsView->columnWidth(CHANGE_COLUMN) + accountsView->columnWidth(VALUE_COLUMN));
		//QDesktopWidget desktop;
		//resize(QSize(750, 650).boundedTo(desktop.availableGeometry(this).size()));
	}
	

	QTimer *scheduleTimer = new QTimer();
	scheduleTimer->setTimerType(Qt::VeryCoarseTimer);
	connect(scheduleTimer, SIGNAL(timeout()), this, SLOT(checkSchedule()));
	scheduleTimer->start(1000 * 60 * 30);
	
	QTimer *updateExchangeRatesTimer = new QTimer();
	updateExchangeRatesTimer->setTimerType(Qt::VeryCoarseTimer);
	connect(scheduleTimer, SIGNAL(timeout()), this, SLOT(checkExchangeRatesTimeOut()));
	updateExchangeRatesTimer->start(1000 * 60 * 60 * 6);

	auto_save_timeout = true;

	QTimer *autoSaveTimer = new QTimer();
	autoSaveTimer->setTimerType(Qt::VeryCoarseTimer);
	connect(autoSaveTimer, SIGNAL(timeout()), this, SLOT(onAutoSaveTimeout()));
	autoSaveTimer->start(1000 * 60 * 1);

	QTimer *dateTimer = new QTimer();
	dateTimer->setTimerType(Qt::VeryCoarseTimer);
	connect(dateTimer, SIGNAL(timeout()), this, SLOT(checkDate()));
	dateTimer->start(1000 * 60);
	
	QLocalServer::removeServer("eqonomize");
	server = new QLocalServer(this);
	server->listen("eqonomize");
	connect(server, SIGNAL(newConnection()), this, SLOT(serverNewConnection()));

}
Eqonomize::~Eqonomize() {}

void Eqonomize::serverNewConnection() {
	socket = server->nextPendingConnection();
	if(socket) {
		connect(socket, SIGNAL(readyRead()), this, SLOT(socketReadyRead()));
	}
}
void Eqonomize::socketReadyRead() {
	setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
	show();
	raise();	
	activateWindow();
	QString command = socket->readAll();
	QChar c = command[0];	
	if(c == 'e') showExpenses();
	else if(c ==  'i') showIncomes();
	else if(c == 't') showTransfers();
	command.remove(0, 1);
	command = command.trimmed();
	if(c == 's') {
		if(command.isEmpty() || QUrl::fromUserInput(command) == current_url) {
			fileSynchronize();
		} else {
			QUrl url = QUrl::fromUserInput(command);
			Budget *budget2 = new Budget();
			QString errors;
			QString error = budget2->loadFile(url.toLocalFile(), errors);
			if(!error.isNull()) {qWarning() << error; return;}
			if(!errors.isNull()) qWarning() << errors;
			errors = QString::null;
			if(budget2->sync(error, errors, true, true)) {
				if(!errors.isNull()) qWarning() << errors;
				error = budget2->saveFile(url.toLocalFile());
				if(!error.isNull()) {qWarning() << error; return;}
			} else {
				if(!error.isNull()) {qWarning() << error; return;}
				if(!errors.isNull()) qWarning() << errors;
			}
		}
	}
	if(!command.isEmpty()) openURL(QUrl::fromUserInput(command));
}

void Eqonomize::useExtraProperties(bool b) {

	b_extra = b;

	delete expensesWidget;
	expensesWidget = new TransactionListWidget(b_extra, TRANSACTION_TYPE_EXPENSE, budget, this, expenses_page);
	connect(expensesWidget, SIGNAL(accountAdded(Account*)), this, SLOT(accountAdded(Account*)));
	connect(expensesWidget, SIGNAL(currenciesModified()), this, SLOT(currenciesModified()));
	expensesLayout->addWidget(expensesWidget);
	expensesWidget->updateAccounts();
	expensesWidget->transactionsReset();
	if(tabs->currentIndex() == EXPENSES_PAGE_INDEX) {
		expensesWidget->onDisplay();
		updateTransactionActions();
	}
	expensesWidget->show();

	delete incomesWidget;
	incomesWidget = new TransactionListWidget(b_extra, TRANSACTION_TYPE_INCOME, budget, this, incomes_page);
	connect(incomesWidget, SIGNAL(accountAdded(Account*)), this, SLOT(accountAdded(Account*)));
	connect(incomesWidget, SIGNAL(currenciesModified()), this, SLOT(currenciesModified()));
	incomesLayout->addWidget(incomesWidget);
	incomesWidget->updateAccounts();
	incomesWidget->transactionsReset();
	if(tabs->currentIndex() == INCOMES_PAGE_INDEX) {
		incomesWidget->onDisplay();
		updateTransactionActions();
	}
	incomesWidget->show();
	
	QSettings settings;
	settings.beginGroup("GeneralOptions");
	settings.setValue("useExtraProperties", b_extra);
	settings.endGroup();

}
void Eqonomize::useExchangeRateForTransactionDate(bool b) {
	if(b != (budget->defaultTransactionConversionRateDate() == TRANSACTION_CONVERSION_RATE_AT_DATE)) {
		if(b) budget->setDefaultTransactionConversionRateDate(TRANSACTION_CONVERSION_RATE_AT_DATE);
		else budget->setDefaultTransactionConversionRateDate(TRANSACTION_CONVERSION_LATEST_RATE);
		expensesWidget->filterTransactions();
		incomesWidget->filterTransactions();
		transfersWidget->filterTransactions();
		filterAccounts();
		updateScheduledTransactions();
		updateSecurities();
		emit transactionsModified();
	
		QSettings settings;
		settings.beginGroup("GeneralOptions");
		settings.setValue("useExchangeRateForTransactionDate", budget->defaultTransactionConversionRateDate() == TRANSACTION_CONVERSION_RATE_AT_DATE);
		settings.endGroup();
	}
}
void Eqonomize::updateBudgetDay() {
	frommonth_begin = budget->firstBudgetDay(from_date);
	prevmonth_begin = budget->firstBudgetDay(to_date);
	budget->addBudgetMonthsSetFirst(prevmonth_begin, -1);
	budgetMonthEdit->blockSignals(true);
	budgetMonthEdit->setDate(budget->budgetDateToMonth(to_date));
	budgetMonthEdit->blockSignals(false);
	if(ActionSelectInitialPeriod->checkedAction() == AIPCurrentMonth) periodSelected(ActionAP_1);
	else if(ActionSelectInitialPeriod->checkedAction() == AIPCurrentYear) periodSelected(ActionAP_2);
	else if(ActionSelectInitialPeriod->checkedAction() == AIPCurrentWholeMonth) periodSelected(ActionAP_3);
	else if(ActionSelectInitialPeriod->checkedAction() == AIPCurrentWholeYear) periodSelected(ActionAP_4);
	else filterAccounts();
	if(otcDialog) ((OverTimeChartDialog*) otcDialog)->chart->resetOptions();
	if(otrDialog) ((OverTimeReportDialog*) otrDialog)->report->resetOptions();
	if(cccDialog) ((CategoriesComparisonChartDialog*) cccDialog)->chart->resetOptions();
	if(ccrDialog) ((CategoriesComparisonReportDialog*) ccrDialog)->report->resetOptions();
}
void Eqonomize::setBudgetPeriod() {
	QDialog *dialog = new QDialog(this, 0);
	dialog->setWindowTitle(tr("Set Budget Period"));
	QVBoxLayout *box1 = new QVBoxLayout(dialog);
	QGridLayout *layout = new QGridLayout();
	layout->addWidget(new QLabel(tr("First day in budget month:"), dialog), 0, 0);
	QComboBox *monthlyDayCombo = new QComboBox(dialog);
	monthlyDayCombo->addItem(tr("1st"));
	monthlyDayCombo->addItem(tr("2nd"));
	monthlyDayCombo->addItem(tr("3rd"));
	monthlyDayCombo->addItem(tr("4th"));
	monthlyDayCombo->addItem(tr("5th"));
	monthlyDayCombo->addItem(tr("6th"));
	monthlyDayCombo->addItem(tr("7th"));
	monthlyDayCombo->addItem(tr("8th"));
	monthlyDayCombo->addItem(tr("9th"));
	monthlyDayCombo->addItem(tr("10th"));
	monthlyDayCombo->addItem(tr("11th"));
	monthlyDayCombo->addItem(tr("12th"));
	monthlyDayCombo->addItem(tr("13th"));
	monthlyDayCombo->addItem(tr("14th"));
	monthlyDayCombo->addItem(tr("15th"));
	monthlyDayCombo->addItem(tr("16th"));
	monthlyDayCombo->addItem(tr("17th"));
	monthlyDayCombo->addItem(tr("18th"));
	monthlyDayCombo->addItem(tr("19th"));
	monthlyDayCombo->addItem(tr("20th"));
	monthlyDayCombo->addItem(tr("21st"));
	monthlyDayCombo->addItem(tr("22nd"));
	monthlyDayCombo->addItem(tr("23rd"));
	monthlyDayCombo->addItem(tr("24th"));
	monthlyDayCombo->addItem(tr("25th"));
	monthlyDayCombo->addItem(tr("26th"));
	monthlyDayCombo->addItem(tr("27th"));
	monthlyDayCombo->addItem(tr("28th"));
	monthlyDayCombo->addItem(tr("Last"));
	monthlyDayCombo->addItem(tr("2nd Last"));
	monthlyDayCombo->addItem(tr("3rd Last"));
	monthlyDayCombo->addItem(tr("4th Last"));
	monthlyDayCombo->addItem(tr("5th Last"));
	if(budget->budgetDay() > 0) monthlyDayCombo->setCurrentIndex(budget->budgetDay() - 1);
	else if(budget->budgetDay() > -5) monthlyDayCombo->setCurrentIndex(28 - budget->budgetDay());
	layout->addWidget(monthlyDayCombo, 0, 1);
	layout->addWidget(new QLabel(tr("First month in budget year:"), dialog), 1, 0);
	QComboBox *yearlyMonthCombo = new QComboBox(dialog);
	for(int i = 1; i <= 12; i++) {
		yearlyMonthCombo->addItem(QDate::longMonthName(i, QDate::StandaloneFormat));
	}
	yearlyMonthCombo->setCurrentIndex(budget->budgetMonth() - 1);
	layout->addWidget(yearlyMonthCombo, 1, 1);
	box1->addLayout(layout);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), dialog, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(accept()));
	box1->addWidget(buttonBox);
	if(dialog->exec() == QDialog::Accepted) {
		budget->setBudgetDay(monthlyDayCombo->currentIndex() >= 28 ? -(monthlyDayCombo->currentIndex() - 28) : monthlyDayCombo->currentIndex() + 1);
		budget->setBudgetMonth(yearlyMonthCombo->currentIndex() + 1);
		updateBudgetDay();
		setModified(true);
	}
	dialog->deleteLater();
}

void Eqonomize::checkDate() {
	if(QDate::currentDate() != prev_cur_date) {
		QDate prev_cur_date_bak = prev_cur_date;
		prev_cur_date = QDate::currentDate();
		if(to_date == prev_cur_date_bak) {
			accountsPeriodToEdit->setDate(prev_cur_date);
			accountsPeriodToChanged(prev_cur_date);
		}
		if(securities_to_date == prev_cur_date_bak) {
			securitiesPeriodToEdit->setDate(prev_cur_date);
			securitiesPeriodToChanged(prev_cur_date);
		}
		expensesWidget->currentDateChanged(prev_cur_date_bak, prev_cur_date);
		incomesWidget->currentDateChanged(prev_cur_date_bak, prev_cur_date);
		transfersWidget->currentDateChanged(prev_cur_date_bak, prev_cur_date);
	}
}

void Eqonomize::showExpenses() {
	tabs->setCurrentIndex(EXPENSES_PAGE_INDEX);
}
void Eqonomize::showIncomes() {
	tabs->setCurrentIndex(INCOMES_PAGE_INDEX);
}
void Eqonomize::showTransfers() {
	tabs->setCurrentIndex(TRANSFERS_PAGE_INDEX);
}

void Eqonomize::setPartialBudget(bool b) {
	partial_budget = b;
	filterAccounts();
}
void Eqonomize::budgetEditReturnPressed() {
	QTreeWidgetItem *i = selectedItem(accountsView);
	if(!i) return;
	int index = accountsView->indexOfTopLevelItem(i);
	index++;
	i = accountsView->topLevelItem(index);
	if(!i && account_items.contains(i) && account_items[i]->type() == ACCOUNT_TYPE_INCOMES) i = expensesItem->child(0);
	if(i) {
		i->setSelected(true);
		if(budgetEdit->isEnabled()) budgetEdit->selectNumber();
		else budgetButton->setFocus();
	}
}
void Eqonomize::budgetMonthChanged(const QDate &date) {
	accountsPeriodFromEdit->blockSignals(true);
	accountsPeriodFromButton->blockSignals(true);
	accountsPeriodToEdit->blockSignals(true);
	from_date = budget->monthToBudgetMonth(date);
	to_date = budget->lastBudgetDay(from_date);
	accountsPeriodFromButton->setChecked(true);
	accountsPeriodFromEdit->setDate(from_date);
	accountsPeriodToEdit->setDate(to_date);
	accountsPeriodFromEdit->blockSignals(false);
	accountsPeriodFromButton->blockSignals(false);
	accountsPeriodToEdit->blockSignals(false);
	filterAccounts();
}
void Eqonomize::budgetChanged(double value) {
	QTreeWidgetItem *i = selectedItem(accountsView);
	if(!account_items.contains(i)) return;
	Account *account = account_items[i];
	if(account->type() == ACCOUNT_TYPE_ASSETS) return;
	CategoryAccount *ca = (CategoryAccount*) account;
	QDate month = budgetMonthEdit->date();
	ca->mbudgets[month] = value;
	for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
		ca = *it;
		if(!ca->mbudgets.contains(month)) {
			value = ca->monthlyBudget(month);
			ca->mbudgets[month] = value;
		}
	}
	for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
		ca = *it;
		if(!ca->mbudgets.contains(month)) {
			value = ca->monthlyBudget(month);
			ca->mbudgets[month] = value;
		}
	}
	setModified(true);
	filterAccounts();
}
void Eqonomize::budgetToggled(bool b) {
	QTreeWidgetItem *i = selectedItem(accountsView);
	if(!account_items.contains(i)) return;
	Account *account = account_items[i];
	if(account->type() == ACCOUNT_TYPE_ASSETS) return;
	CategoryAccount *ca = (CategoryAccount*) account;
	QDate month = budgetMonthEdit->date();
	if(b) ca->mbudgets[month] = budgetEdit->value();
	else ca->mbudgets[month] = -1;
	for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
		ca = *it;
		if(!ca->mbudgets.contains(month)) {
			double value = ca->monthlyBudget(month);
			ca->mbudgets[month] = value;
		}
	}
	for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
		ca = *it;
		if(!ca->mbudgets.contains(month)) {
			double value = ca->monthlyBudget(month);
			ca->mbudgets[month] = value;
		}
	}
	setModified(true);
	filterAccounts();
}
void Eqonomize::periodSelected(QAction *a) {
	accountsPeriodFromEdit->blockSignals(true);
	accountsPeriodFromButton->blockSignals(true);
	accountsPeriodToEdit->blockSignals(true);	
	QDate curdate = QDate::currentDate();
	if(a == ActionAP_1) {
		from_date = budget->firstBudgetDay(curdate);
		to_date = curdate;
	} else if(a == ActionAP_2) {
		from_date = budget->firstBudgetDayOfYear(curdate);
		to_date = curdate;
	} else if(a == ActionAP_3) {
		from_date = budget->firstBudgetDay(curdate);
		to_date = budget->lastBudgetDay(curdate);
	} else if(a == ActionAP_4) {
		from_date = budget->firstBudgetDayOfYear(curdate);
		to_date = budget->lastBudgetDayOfYear(curdate);
	} else if(a == ActionAP_5) {
		to_date = curdate;
		from_date = to_date.addMonths(-1).addDays(1);
	} else if(a == ActionAP_6) {
		to_date = curdate;
		from_date = to_date.addYears(-1).addDays(1);
	} else if(a == ActionAP_7) {
		from_date = budget->firstBudgetDay(curdate);
		budget->addBudgetMonthsSetFirst(from_date, -1);
		to_date = budget->lastBudgetDay(from_date);
	} else if(a == ActionAP_8) {
		from_date = budget->firstBudgetDayOfYear(curdate);
		budget->addBudgetMonthsSetFirst(from_date, -12);
		to_date = budget->lastBudgetDayOfYear(from_date);
	}
	accountsPeriodFromButton->setChecked(true);
	accountsPeriodFromEdit->setEnabled(true);
	accountsPeriodFromEdit->setDate(from_date);
	accountsPeriodToEdit->setDate(to_date);
	accountsPeriodFromEdit->blockSignals(false);
	accountsPeriodFromButton->blockSignals(false);
	accountsPeriodToEdit->blockSignals(false);
	filterAccounts();
}

void Eqonomize::newSecurity() {
	budget->setRecordNewAccounts(true);
	budget->resetDefaultCurrencyChanged();
	budget->resetCurrenciesModified();
	EditSecurityDialog *dialog = new EditSecurityDialog(budget, this, tr("New Security", "Financial security (e.g. stock, mutual fund)"), true);
	if(dialog->exec() == QDialog::Accepted) {
		foreach(Account *acc, budget->newAccounts) accountAdded(acc);
		budget->newAccounts.clear();
		Security *security = dialog->newSecurity();
		if(security) {
			budget->addSecurity(security);
			appendSecurity(security);
			updateSecurityAccount(security->account());
			setModified(true);
		}
	} else {
		foreach(Account *acc, budget->newAccounts) accountAdded(acc);
		budget->newAccounts.clear();
	}
	dialog->deleteLater();
	if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
	budget->setRecordNewAccounts(false);
}
void Eqonomize::securityAdded(Security *security) {
	appendSecurity(security);
	updateSecurityAccount(security->account());
	setModified(true);
}
void Eqonomize::editSecurity(QTreeWidgetItem *i) {
	if(!i) return;
	budget->setRecordNewAccounts(true);
	budget->resetDefaultCurrencyChanged();
	budget->resetCurrenciesModified();
	Security *security = ((SecurityListViewItem*) i)->security();
	EditSecurityDialog *dialog = new EditSecurityDialog(budget, this, tr("Edit Security", "Financial security (e.g. stock, mutual fund)"), true);
	dialog->setSecurity(security);
	if(dialog->exec() == QDialog::Accepted) {
		foreach(Account *acc, budget->newAccounts) accountAdded(acc);
		budget->newAccounts.clear();
		AssetsAccount *prev_acc = security->account();
		if(dialog->modifySecurity(security)) {
			updateSecurity(i);
			updateSecurityAccount(security->account());
			if(prev_acc != security->account()) {
				updateSecurityAccount(prev_acc);
			}
			setModified(true);
			incomesWidget->filterTransactions();
			transfersWidget->filterTransactions();
		}
	} else {
		foreach(Account *acc, budget->newAccounts) accountAdded(acc);
		budget->newAccounts.clear();
	}
	dialog->deleteLater();
	if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
	budget->setRecordNewAccounts(false);
}
void Eqonomize::editSecurity() {
	QTreeWidgetItem *i = selectedItem(securitiesView);
	if(i == NULL) return;
	editSecurity(i);
}
void Eqonomize::updateSecuritiesStatistics() {
	securitiesStatLabel->setText(QString("<div align=\"right\"><b>%1</b> %5 &nbsp; <b>%2</b> %6 &nbsp; <b>%3</b> %7 &nbsp; <b>%4</b> %8%</div>").arg(tr("Total value:")).arg(tr("Cost:")).arg(tr("Profit:")).arg(tr("Rate:")).arg(budget->formatMoney(total_value), budget->formatMoney(total_cost)).arg(budget->formatMoney(total_profit)).arg(QLocale().toString(total_rate * 100)));
}
void Eqonomize::deleteSecurity() {
	SecurityListViewItem *i = (SecurityListViewItem*) selectedItem(securitiesView);
	if(i == NULL) return;
	Security *security = i->security();
	bool has_trans = budget->securityHasTransactions(security);
	if(!has_trans || QMessageBox::warning(this, tr("Delete security?", "Financial security (e.g. stock, mutual fund)"), tr("Are you sure you want to delete the security \"%1\" and all associated transactions?", "Financial security (e.g. stock, mutual fund)").arg(security->name()), QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok) {
		total_value -= i->value;
		total_rate *= total_cost;
		total_cost -= i->scost;
		total_rate -= i->scost * i->rate;
		if(total_cost != 0.0) total_rate /= total_cost;
		total_profit -= i->sprofit;
		updateSecuritiesStatistics();
		delete i;
		budget->removeSecurity(security);
		if(has_trans) {
			filterAccounts();
			updateScheduledTransactions();
			expensesWidget->filterTransactions();
			incomesWidget->filterTransactions();
			transfersWidget->filterTransactions();
			emit transactionsModified();
		} else {
			updateSecurityAccount(security->account());
		}
		setModified(true);
	}
}
void Eqonomize::buySecurities() {
	SecurityListViewItem *i = (SecurityListViewItem*) selectedItem(securitiesView);
	newScheduledTransaction(TRANSACTION_TYPE_SECURITY_BUY, i == NULL ? NULL : i->security(), true);
}
void Eqonomize::sellSecurities() {
	SecurityListViewItem *i = (SecurityListViewItem*) selectedItem(securitiesView);
	newScheduledTransaction(TRANSACTION_TYPE_SECURITY_SELL, i == NULL ? NULL : i->security(), true);
}
void Eqonomize::newDividend() {
	SecurityListViewItem *i = (SecurityListViewItem*) selectedItem(securitiesView);
	newScheduledTransaction(TRANSACTION_TYPE_INCOME, i == NULL ? NULL : i->security(), true);
}
void Eqonomize::newReinvestedDividend() {
	SecurityListViewItem *i = (SecurityListViewItem*) selectedItem(securitiesView);
	newScheduledTransaction(TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND, i == NULL ? NULL : i->security(), true);
}
void Eqonomize::editSecurityTrade(SecurityTrade *ts) {
	editSecurityTrade(ts, this);
}
bool Eqonomize::editSecurityTrade(SecurityTrade *ts, QWidget *parent) {
	EditSecurityTradeDialog *dialog = new EditSecurityTradeDialog(budget, ts->from_security, parent);
	dialog->setSecurityTrade(ts);
	if(dialog->exec() == QDialog::Accepted) {
		SecurityTrade *ts_new = dialog->createSecurityTrade();
		if(ts_new) {
			ts_new->timestamp = ts->timestamp;
			ts_new->id = ts->id;
			ts_new->first_revision = ts->first_revision;
			ts_new->last_revision = budget->revision();
			budget->removeSecurityTrade(ts, true);
			budget->addSecurityTrade(ts_new);
			updateSecurity(ts_new->from_security);
			updateSecurity(ts_new->to_security);
			if(ts->to_security != ts_new->to_security) updateSecurity(ts->to_security);
			if(ts->from_security != ts_new->from_security) updateSecurity(ts->from_security);
			updateSecurityAccount(ts_new->from_security->account());
			if(ts_new->to_security->account() != ts_new->from_security->account()) {
				updateSecurityAccount(ts_new->to_security->account());
			}
			if(ts->to_security->account() != ts_new->to_security->account() && ts->to_security->account() != ts_new->from_security->account()) {
				updateSecurityAccount(ts->to_security->account());
			}
			setModified(true);
			delete ts;
			dialog->deleteLater();
			return true;
		}
	}
	dialog->deleteLater();
	return false;
}
void Eqonomize::newSecurityTrade() {
	SecurityListViewItem *i = (SecurityListViewItem*) selectedItem(securitiesView);
	EditSecurityTradeDialog *dialog = new EditSecurityTradeDialog(budget, i == NULL ? NULL : i->security(), this);
	if(dialog->checkSecurities() && dialog->exec() == QDialog::Accepted) {
		SecurityTrade *ts = dialog->createSecurityTrade();
		if(ts) {
			budget->addSecurityTrade(ts);
			updateSecurity(ts->from_security);
			updateSecurity(ts->to_security);
			updateSecurityAccount(ts->from_security->account());
			if(ts->to_security->account() != ts->from_security->account()) {
				updateSecurityAccount(ts->to_security->account());
			}
			setModified(true);
		}
	}
	dialog->deleteLater();
}
void Eqonomize::setQuotation() {
	SecurityListViewItem *i = (SecurityListViewItem*) selectedItem(securitiesView);
	if(i == NULL) return;
	QDialog *dialog = new QDialog(this, 0);
	dialog->setWindowTitle(tr("Set Quote (%1)", "Financial quote").arg(i->security()->name()));
	dialog->setModal(true);
	QVBoxLayout *box1 = new QVBoxLayout(dialog);
	QGridLayout *grid = new QGridLayout();
	box1->addLayout(grid);
	grid->addWidget(new QLabel(tr("Price per share:", "Financial shares"), dialog), 0, 0);
	EqonomizeValueEdit *quotationEdit = new EqonomizeValueEdit(0.0, pow(10, -i->security()->quotationDecimals()), i->security()->getQuotation(QDate::currentDate()), i->security()->quotationDecimals(), true, dialog, budget);
	quotationEdit->setFocus();
	quotationEdit->setCurrency(i->security()->currency(), true);
	grid->addWidget(quotationEdit, 0, 1);
	grid->addWidget(new QLabel(tr("Date:"), dialog), 1, 0);
	QDateEdit *dateEdit = new QDateEdit(QDate::currentDate(), dialog);
	dateEdit->setCalendarPopup(true);
	grid->addWidget(dateEdit, 1, 1);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), dialog, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(accept()));
	box1->addWidget(buttonBox);
	while(dialog->exec() == QDialog::Accepted) {
		QDate date = dateEdit->date();
		if(!date.isValid()) {
			QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		} else if(date > QDate::currentDate()) {
			QMessageBox::critical(this, tr("Error"), tr("Future dates are not allowed."));
		} else {
			i->security()->setQuotation(date, quotationEdit->value());
			updateSecurity(i);
			updateSecurityAccount(i->security()->account(), true);
			setModified(true);
			break;
		}
	}
	dialog->deleteLater();
}
void Eqonomize::editQuotations() {
	SecurityListViewItem *i = (SecurityListViewItem*) selectedItem(securitiesView);
	if(!i) return;
	Security *security = i->security();
	EditQuotationsDialog *dialog = new EditQuotationsDialog(budget, this);
	dialog->setSecurity(security);
	if(dialog->exec() == QDialog::Accepted) {
		dialog->modifyQuotations(security);
		updateSecurity(i);
		updateSecurityAccount(security->account());
		setModified(true);
	}
	dialog->deleteLater();
}
void Eqonomize::editSecurityTransactions() {
	SecurityListViewItem *i = (SecurityListViewItem*) selectedItem(securitiesView);
	if(!i) return;
	SecurityTransactionsDialog *dialog = new SecurityTransactionsDialog(i->security(), this, tr("Security Transactions", "Financial security (e.g. stock, mutual fund)"));
	dialog->exec();
	dialog->deleteLater();
}
void Eqonomize::securitiesSelectionChanged() {
	SecurityListViewItem *i = (SecurityListViewItem*) selectedItem(securitiesView);
	setQuotationButton->setEnabled(i != NULL);
	ActionEditSecurity->setEnabled(i != NULL);
	ActionDeleteSecurity->setEnabled(i != NULL);
	ActionSetQuotation->setEnabled(i != NULL);
	ActionEditQuotations->setEnabled(i != NULL);
	ActionEditSecurityTransactions->setEnabled(i != NULL);
}
void Eqonomize::securitiesExecuted(QTreeWidgetItem *i) {
	if(i == NULL) return;
	editSecurity(i);
}
void Eqonomize::securitiesExecuted(QTreeWidgetItem *i, int c) {
	if(i == NULL) return;
	switch(c) {
		case 4: {}
		case 5: {}
		case 6: {}
		case 1: {}
		case 2: {
			editSecurityTransactions();
			break;
		}
		case 3: {
			setQuotation();
			break;
		}
		case 8: {
			if(((SecurityListViewItem*) i)->security()) {
				editAccount(((SecurityListViewItem*) i)->security()->account());
			}
			break;
		}
		default: {
			editSecurity(i);
			break;
		}
	}
}

void Eqonomize::popupSecuritiesMenu(const QPoint &p) {
	if(!securitiesPopupMenu) {
		securitiesPopupMenu = new QMenu(this);
		securitiesPopupMenu->addAction(ActionNewSecurity);
		securitiesPopupMenu->addAction(ActionEditSecurity);
		securitiesPopupMenu->addAction(ActionDeleteSecurity);
		securitiesPopupMenu->addSeparator();
		securitiesPopupMenu->addAction(ActionBuyShares);
		securitiesPopupMenu->addAction(ActionSellShares);
		securitiesPopupMenu->addAction(ActionNewSecurityTrade);
		securitiesPopupMenu->addAction(ActionNewDividend);
		securitiesPopupMenu->addAction(ActionNewReinvestedDividend);
		securitiesPopupMenu->addAction(ActionEditSecurityTransactions);
		securitiesPopupMenu->addSeparator();
		securitiesPopupMenu->addAction(ActionSetQuotation);
		securitiesPopupMenu->addAction(ActionEditQuotations);
	}
	securitiesPopupMenu->popup(securitiesView->viewport()->mapToGlobal(p));
}
void Eqonomize::appendSecurity(Security *security) {
	double value = 0.0, cost = 0.0, rate = 0.0, profit = 0.0, quotation = 0.0, shares = 0.0;
	value = security->value(securities_to_date, true);
	cost = security->cost(securities_to_date);
	if(securities_to_date > QDate::currentDate()) quotation = security->expectedQuotation(securities_to_date);
	else quotation = security->getQuotation(securities_to_date);
	shares = security->shares(securities_to_date, true);
	if(securitiesPeriodFromButton->isChecked()) {
		rate = security->yearlyRate(securities_from_date, securities_to_date);
		profit = security->profit(securities_from_date, securities_to_date, true);
	} else {
		rate = security->yearlyRate(securities_to_date);
		profit = security->profit(securities_to_date, true);
	}
	Currency *cur = security->currency();
	if(!cur) cur = budget->defaultCurrency();
	SecurityListViewItem *i = new SecurityListViewItem(security, security->name(), cur->formatValue(value), QLocale().toString(shares, 'f', security->decimals()), cur->formatValue(quotation, security->quotationDecimals()), cur->formatValue(cost), cur->formatValue(profit), QLocale().toString(rate * 100) + "%", QString::null, security->account()->name());	
	i->rate = rate;
	i->shares = shares;
	if(cur != budget->defaultCurrency()) {
		i->sprofit = security->profit(securities_to_date, true, false, budget->defaultCurrency());
		i->scost = security->cost(securities_to_date, false, budget->defaultCurrency());
		i->profit = cur->convertTo(profit, budget->defaultCurrency());
		i->quote = cur->convertTo(quotation, budget->defaultCurrency());
		i->value = cur->convertTo(value, budget->defaultCurrency());
		i->cost = cur->convertTo(cost, budget->defaultCurrency());
	} else {
		i->sprofit = profit;
		i->scost = cost;
		i->profit = profit;
		i->quote = quotation;
		i->value = value;
		i->cost = cost;
	}
	switch(security->type()) {
		case SECURITY_TYPE_BOND: {i->setText(7, tr("Bond")); break;}
		case SECURITY_TYPE_STOCK: {i->setText(7, tr("Stock", "Financial stock")); break;}
		case SECURITY_TYPE_MUTUAL_FUND: {i->setText(7, tr("Mutual Fund")); break;}
		case SECURITY_TYPE_OTHER: {i->setText(7, tr("Other")); break;}
	}
	securitiesView->insertTopLevelItem(securitiesView->topLevelItemCount(), i);
	if(cost > 0.0) i->setForeground(4, createExpenseColor(i, 0));
	else if(cost < 0.0) i->setForeground(4, createIncomeColor(i, 0));
	else i->setForeground(4, createTransferColor(i, 0));
	if(profit < 0.0) i->setForeground(5, createExpenseColor(i, 0));
	else if(profit > 0.0) i->setForeground(5, createIncomeColor(i, 0));
	else i->setForeground(5, createTransferColor(i, 0));
	if(rate < 0.0) i->setForeground(6, createExpenseColor(i, 0));
	else if(rate > 0.0) i->setForeground(6, createIncomeColor(i, 0));
	else i->setForeground(6, createTransferColor(i, 0));
	total_rate *= total_value;
	total_value += i->value;
	total_cost += i->scost;
	total_rate += i->value * rate;
	if(total_cost != 0.0) total_rate /= total_value;
	total_profit += i->sprofit;
	updateSecuritiesStatistics();
	securitiesView->setSortingEnabled(true);
}
void Eqonomize::updateSecurity(Security *security) {
	QTreeWidgetItemIterator it(securitiesView);
	QTreeWidgetItem *i = *it;
	while(i != NULL) {
		if(((SecurityListViewItem*) i)->security() == security) {
			updateSecurity(i);
			break;
		}
		++it;
		i = *it;
	}
}
void Eqonomize::updateSecurity(QTreeWidgetItem *i_pre) {
	SecurityListViewItem* i = (SecurityListViewItem*) i_pre;
	Security *security = i->security();
	Currency *cur = security->currency();
	total_rate *= total_value;
	total_value -= i->value;
	total_cost -= i->scost;
	total_rate -= i->value * i->rate;
	if(total_cost != 0.0) total_rate /= total_value;
	total_profit -= ((SecurityListViewItem*) i)->sprofit;
	double value = 0.0, cost = 0.0, rate = 0.0, profit = 0.0, quotation = 0.0, shares = 0.0;
	value = security->value(securities_to_date, true);
	cost = security->cost(securities_to_date);
	if(securities_to_date > QDate::currentDate()) quotation = security->expectedQuotation(securities_to_date);
	else quotation = security->getQuotation(securities_to_date);
	shares = security->shares(securities_to_date, true);
	if(securitiesPeriodFromButton->isChecked()) {
		rate = security->yearlyRate(securities_from_date, securities_to_date);
		profit = security->profit(securities_from_date, securities_to_date, true);
	} else {
		rate = security->yearlyRate(securities_to_date);
		profit = security->profit(securities_to_date, true);
	}
	i->rate = rate;
	i->shares = shares;
	if(cur != budget->defaultCurrency()) {
		i->sprofit = security->profit(securities_to_date, true, false, budget->defaultCurrency());
		i->scost = security->cost(securities_to_date, false, budget->defaultCurrency());
		i->profit = cur->convertTo(profit, budget->defaultCurrency());
		i->quote = cur->convertTo(quotation, budget->defaultCurrency());
		i->value = cur->convertTo(value, budget->defaultCurrency());
		i->cost = cur->convertTo(cost, budget->defaultCurrency());
	} else {
		i->sprofit = profit;
		i->scost = cost;
		i->profit = profit;
		i->quote = quotation;
		i->value = value;
		i->cost = cost;
	}
	total_rate *= total_value;
	total_value += i->value;
	total_cost += i->scost;
	total_rate += i->value * rate;
	if(total_cost != 0.0) total_rate /= total_value;
	total_profit += i->profit;
	i->setText(0, security->name());
	switch(security->type()) {
		case SECURITY_TYPE_BOND: {i->setText(7, tr("Bond")); break;}
		case SECURITY_TYPE_STOCK: {i->setText(7, tr("Stock", "Financial stock")); break;}
		case SECURITY_TYPE_MUTUAL_FUND: {i->setText(7, tr("Mutual Fund")); break;}
		case SECURITY_TYPE_OTHER: {i->setText(7, tr("Other")); break;}
	}
	i->setText(1, cur->formatValue(value));
	i->setText(2, QLocale().toString(shares, 'f', security->decimals()));
	i->setText(3, cur->formatValue(quotation, security->quotationDecimals()));
	i->setText(4, cur->formatValue(cost));
	i->setText(5, cur->formatValue(profit));
	i->setText(6, QLocale().toString(rate * 100) + "%");
	i->setText(8, security->account()->name());
	if(cost > 0.0) i->setForeground(4, createExpenseColor(i, 0));
	else if(cost < 0.0) i->setForeground(4, createIncomeColor(i, 0));
	else i->setForeground(4, createTransferColor(i, 0));
	if(profit < 0.0) i->setForeground(5, createExpenseColor(i, 0));
	else if(profit > 0.0) i->setForeground(5, createIncomeColor(i, 0));
	else i->setForeground(5, createTransferColor(i, 0));
	if(rate < 0.0) i->setForeground(6, createExpenseColor(i, 0));
	else if(rate > 0.0) i->setForeground(6, createIncomeColor(i, 0));
	else i->setForeground(6, createTransferColor(i, 0));
	updateSecuritiesStatistics();
}
void Eqonomize::updateSecurities() {
	securitiesView->clear();
	total_value = 0.0;
	total_cost = 0.0;
	total_profit = 0.0;
	total_rate = 0.0;
	for(SecurityList<Security*>::const_iterator it = budget->securities.constBegin(); it != budget->securities.constEnd(); ++it) {
		Security *security = *it;
		appendSecurity(security);
	}
}
void Eqonomize::addNewSchedule(ScheduledTransaction *strans, QWidget *parent) {
	if(strans->isOneTimeTransaction() && strans->transaction()->date() <= QDate::currentDate()) {
		Transactions *trans = strans->transaction()->copy();
		delete strans;
		budget->addTransactions(trans);
		transactionAdded(trans);
	} else if(strans->recurrence() && strans->date() <= QDate::currentDate() && ((QTime::currentTime().hour() >= 18 && strans->recurrence()->nextOccurrence(strans->date(), false) > QDate::currentDate()) || (QTime::currentTime().hour() < 18 && strans->recurrence()->nextOccurrence(strans->date(), false) >= QDate::currentDate()))) {
		Transactions *trans = strans->realize(strans->date());
		budget->addTransactions(trans);
		transactionAdded(trans);
		budget->addScheduledTransaction(strans);
		transactionAdded(strans);
	} else {
		budget->addScheduledTransaction(strans);
		transactionAdded(strans);
		checkSchedule(true, parent);
	}
}
void Eqonomize::newExpenseWithLoan() {
	newExpenseWithLoan(this);
}
bool Eqonomize::newExpenseWithLoan(QString description_value, double value_value, double quantity_value, QDate date_value, ExpensesAccount *category_value, QString payee_value, QString comment_value) {
	budget->setRecordNewAccounts(true);
	budget->setRecordNewSecurities(true);
	budget->resetDefaultCurrencyChanged();
	budget->resetCurrenciesModified();
	TransactionEditDialog *dialog = new TransactionEditDialog(b_extra, TRANSACTION_TYPE_EXPENSE, NULL, false, NULL, SECURITY_ALL_VALUES, false, budget, this, true, false, true); 
	dialog->editWidget->updateAccounts();
	dialog->editWidget->setValues(description_value, value_value, quantity_value, date_value, NULL, category_value, payee_value, comment_value);
	if(dialog->editWidget->checkAccounts() && dialog->exec() == QDialog::Accepted) {
		Transactions *trans = dialog->editWidget->createTransactionWithLoan();
		if(trans) {
			foreach(Account* acc, budget->newAccounts) accountAdded(acc);
			budget->newAccounts.clear();
			foreach(Security *sec, budget->newSecurities) securityAdded(sec);
			budget->newSecurities.clear();
			if(trans->date() > QDate::currentDate()) {
				ScheduledTransaction *strans_new = new ScheduledTransaction(budget, trans, NULL);
				budget->addScheduledTransaction(strans_new);
				transactionAdded(strans_new);
			} else {
				budget->addTransactions(trans);
				transactionAdded(trans);
			}
			if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
			budget->setRecordNewAccounts(false);
			budget->setRecordNewSecurities(false);
			dialog->deleteLater();
			return true;
		}
	}
	foreach(Account *acc, budget->newAccounts) accountAdded(acc);
	budget->newAccounts.clear();
	foreach(Security *sec, budget->newSecurities) securityAdded(sec);
	budget->newSecurities.clear();
	if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
	budget->setRecordNewAccounts(false);
	budget->setRecordNewSecurities(false);
	dialog->deleteLater();
	return false;
}
bool Eqonomize::newExpenseWithLoan(QWidget *parent) {
	budget->setRecordNewAccounts(true);
	budget->setRecordNewSecurities(true);
	budget->resetDefaultCurrencyChanged();
	budget->resetCurrenciesModified();
	TransactionEditDialog *dialog = new TransactionEditDialog(b_extra, TRANSACTION_TYPE_EXPENSE, NULL, false, NULL, SECURITY_ALL_VALUES, false, budget, parent, true, false, true); 
	dialog->editWidget->updateAccounts();
	if(dialog->editWidget->checkAccounts() && dialog->exec() == QDialog::Accepted) {
		Transactions *trans = dialog->editWidget->createTransactionWithLoan();
		if(trans) {
			foreach(Account* acc, budget->newAccounts) accountAdded(acc);
			budget->newAccounts.clear();
			foreach(Security *sec, budget->newSecurities) securityAdded(sec);
			budget->newSecurities.clear();
			if(trans->date() > QDate::currentDate()) {
				ScheduledTransaction *strans_new = new ScheduledTransaction(budget, trans, NULL);
				budget->addScheduledTransaction(strans_new);
				transactionAdded(strans_new);
			} else {
				budget->addTransactions(trans);
				transactionAdded(trans);
			}
			if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
			budget->setRecordNewAccounts(false);
			budget->setRecordNewSecurities(false);
			dialog->deleteLater();
			return true;
		}
	}
	foreach(Account *acc, budget->newAccounts) accountAdded(acc);
	budget->newAccounts.clear();
	foreach(Security *sec, budget->newSecurities) securityAdded(sec);
	budget->newSecurities.clear();
	if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
	budget->setRecordNewAccounts(false);
	budget->setRecordNewSecurities(false);
	dialog->deleteLater();
	return false;
}
void Eqonomize::newMultiAccountExpense() {
	newMultiAccountTransaction(this, true);
}
void Eqonomize::newMultiAccountIncome() {
	newMultiAccountTransaction(this, false);
}
bool Eqonomize::newMultiAccountTransaction(bool create_expenses, QString description_string, CategoryAccount *category_account, double quantity_value, QString comment_string) {
	budget->setRecordNewAccounts(true);
	budget->setRecordNewSecurities(true);
	budget->resetDefaultCurrencyChanged();
	budget->resetCurrenciesModified();
	ScheduledTransaction *strans = EditScheduledMultiAccountDialog::newScheduledTransaction(description_string, category_account, quantity_value, comment_string, budget, this, create_expenses, b_extra, true);
	if(strans) {
		foreach(Account* acc, budget->newAccounts) accountAdded(acc);
		budget->newAccounts.clear();
		foreach(Security *sec, budget->newSecurities) securityAdded(sec);
		budget->newSecurities.clear();
		addNewSchedule(strans, this);
		if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
		budget->setRecordNewAccounts(false);
		budget->setRecordNewSecurities(false);
		return true;
	} else {
		foreach(Account* acc, budget->newAccounts) accountAdded(acc);
		budget->newAccounts.clear();
		foreach(Security *sec, budget->newSecurities) securityAdded(sec);
		budget->newSecurities.clear();
	}
	if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
	budget->setRecordNewAccounts(false);
	budget->setRecordNewSecurities(false);
	return false;
}
bool Eqonomize::newMultiAccountTransaction(QWidget *parent, bool create_expenses) {
	budget->setRecordNewAccounts(true);
	budget->setRecordNewSecurities(true);
	budget->resetDefaultCurrencyChanged();
	budget->resetCurrenciesModified();
	ScheduledTransaction *strans = EditScheduledMultiAccountDialog::newScheduledTransaction(budget, parent, create_expenses, b_extra, true);
	if(strans) {
		foreach(Account* acc, budget->newAccounts) accountAdded(acc);
		budget->newAccounts.clear();
		foreach(Security *sec, budget->newSecurities) securityAdded(sec);
		budget->newSecurities.clear();
		addNewSchedule(strans, parent);
		if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
		budget->setRecordNewAccounts(false);
		budget->setRecordNewSecurities(false);
		return true;
	} else {
		foreach(Account* acc, budget->newAccounts) accountAdded(acc);
		budget->newAccounts.clear();
		foreach(Security *sec, budget->newSecurities) securityAdded(sec);
		budget->newSecurities.clear();
	}
	if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
	budget->setRecordNewAccounts(false);
	budget->setRecordNewSecurities(false);
	return false;
}
void  Eqonomize::newMultiItemTransaction() {
	newMultiItemTransaction(this);
}
bool Eqonomize::newMultiItemTransaction(QWidget *parent, AssetsAccount *account) {
	budget->setRecordNewAccounts(true);
	budget->setRecordNewSecurities(true);
	budget->resetDefaultCurrencyChanged();
	budget->resetCurrenciesModified();
	ScheduledTransaction *strans = EditScheduledMultiItemDialog::newScheduledTransaction(budget, parent, account, b_extra, true);
	if(strans) {
		foreach(Account* acc, budget->newAccounts) accountAdded(acc);
		budget->newAccounts.clear();
		foreach(Security *sec, budget->newSecurities) securityAdded(sec);
		budget->newSecurities.clear();
		addNewSchedule(strans, parent);
		budget->setRecordNewAccounts(false);
		budget->setRecordNewSecurities(false);
		return true;
	} else {
		foreach(Account* acc, budget->newAccounts) accountAdded(acc);
		budget->newAccounts.clear();
		foreach(Security *sec, budget->newSecurities) securityAdded(sec);
		budget->newSecurities.clear();
	}
	if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
	budget->setRecordNewAccounts(false);
	budget->setRecordNewSecurities(false);
	return false;
}
void  Eqonomize::newDebtPayment() {
	newDebtPayment(this);
}
void  Eqonomize::newDebtInterest() {
	newDebtPayment(this, NULL, true);
}
bool Eqonomize::newDebtPayment(QWidget *parent, AssetsAccount *loan, bool only_interest) {
	budget->setRecordNewAccounts(true);
	budget->setRecordNewSecurities(true);
	budget->resetDefaultCurrencyChanged();
	budget->resetCurrenciesModified();
	ScheduledTransaction *strans = EditScheduledDebtPaymentDialog::newScheduledTransaction(budget, parent, loan, b_extra, true, only_interest);
	if(strans) {
		foreach(Account* acc, budget->newAccounts) accountAdded(acc);
		budget->newAccounts.clear();
		foreach(Security *sec, budget->newSecurities) securityAdded(sec);
		budget->newSecurities.clear();
		addNewSchedule(strans, parent);
		if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
		budget->setRecordNewAccounts(false);
		budget->setRecordNewSecurities(false);
		return true;
	} else {
		foreach(Account* acc, budget->newAccounts) accountAdded(acc);
		budget->newAccounts.clear();
		foreach(Security *sec, budget->newSecurities) securityAdded(sec);
		budget->newSecurities.clear();
	}
	if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
	budget->setRecordNewAccounts(false);
	budget->setRecordNewSecurities(false);
	return false;
}
bool Eqonomize::editSplitTransaction(SplitTransaction *split) {
	return editSplitTransaction(split, this);
}
bool Eqonomize::editSplitTransaction(SplitTransaction *split, QWidget *parent, bool temporary_split)  {
	Recurrence *rec = NULL;
	budget->setRecordNewAccounts(true);
	budget->setRecordNewSecurities(true);
	budget->resetDefaultCurrencyChanged();
	budget->resetCurrenciesModified();
	SplitTransaction *new_split = NULL;
	if(split->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ITEMS) {
		new_split = EditScheduledMultiItemDialog::editTransaction((MultiItemTransaction*) split, rec, parent, b_extra, true);
	} else if(split->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ACCOUNTS) {
		new_split = EditScheduledMultiAccountDialog::editTransaction((MultiAccountTransaction*) split, rec, parent, b_extra, true);
	} else if(split->type() == SPLIT_TRANSACTION_TYPE_LOAN) {
		new_split = EditScheduledDebtPaymentDialog::editTransaction((DebtPayment*) split, rec, parent, b_extra, true);
	}
	if(new_split) {
		if(!temporary_split) {
			budget->removeSplitTransaction(split, true);
			transactionRemoved(split);
			delete split;
		}
		split = new_split;
		foreach(Account* acc, budget->newAccounts) accountAdded(acc);
		budget->newAccounts.clear();
		foreach(Security *sec, budget->newSecurities) securityAdded(sec);
		budget->newSecurities.clear();
		if((!rec || rec->firstOccurrence() == rec->lastOccurrence()) && split->date() <= QDate::currentDate()) {
			budget->addSplitTransaction(split);
			transactionAdded(split);
		} else {
			ScheduledTransaction *strans = new ScheduledTransaction(budget, split, rec);
			if(strans->recurrence() && strans->date() <= QDate::currentDate() && ((QTime::currentTime().hour() >= 18 && strans->recurrence()->nextOccurrence(strans->date(), false) > QDate::currentDate()) || (QTime::currentTime().hour() < 18 && strans->recurrence()->nextOccurrence(strans->date(), false) >= QDate::currentDate()))) {
				Transactions *new_trans = strans->realize(strans->date());
				budget->addTransactions(new_trans);
				transactionAdded(new_trans);
				budget->addScheduledTransaction(strans);
				transactionAdded(strans);
			} else {
				budget->addScheduledTransaction(strans);
				transactionAdded(strans);
				checkSchedule(true, parent);
			}
		}
		if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
		budget->setRecordNewAccounts(false);
		budget->setRecordNewSecurities(false);
		return true;
	}
	foreach(Account* acc, budget->newAccounts) accountAdded(acc);
	budget->newAccounts.clear();
	foreach(Security *sec, budget->newSecurities) securityAdded(sec);
	budget->newSecurities.clear();
	if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
	budget->setRecordNewAccounts(false);
	budget->setRecordNewSecurities(false);
	return false;
}
bool Eqonomize::splitUpTransaction(SplitTransaction *split) {
	expensesWidget->onTransactionSplitUp(split);
	incomesWidget->onTransactionSplitUp(split);
	transfersWidget->onTransactionSplitUp(split);
	int c = split->count();
	for(int i = 0; i < c; i++) {
		Transaction *trans = split->at(i);
		if(trans->comment().isEmpty()) trans->setComment(split->comment());
		if(trans->associatedFile().isEmpty()) trans->setAssociatedFile(split->associatedFile());
	}
	split->clear(true);
	budget->removeSplitTransaction(split, true);
	transactionRemoved(split);
	delete split;
	setModified(true);
	return true;
}
bool Eqonomize::removeSplitTransaction(SplitTransaction *split) {
	budget->removeSplitTransaction(split, true);
	transactionRemoved(split);
	delete split;
	return true;
}
bool Eqonomize::newScheduledTransaction(int transaction_type, Security *security, bool select_security) {
	return newScheduledTransaction(transaction_type, security, select_security, this);
}
bool Eqonomize::newScheduledTransaction(int transaction_type, Security *security, bool select_security, QWidget *parent, Account *account) {
	budget->setRecordNewAccounts(true);
	budget->setRecordNewSecurities(true);
	budget->resetDefaultCurrencyChanged();
	budget->resetCurrenciesModified();
	ScheduledTransaction *strans = EditScheduledTransactionDialog::newScheduledTransaction(transaction_type, budget, parent, security, select_security, account, b_extra, true);
	if(strans) {
		foreach(Account* acc, budget->newAccounts) accountAdded(acc);
		budget->newAccounts.clear();
		foreach(Security *sec, budget->newSecurities) securityAdded(sec);
		budget->newSecurities.clear();
		addNewSchedule(strans, parent);
		return true;
	} else {
		foreach(Account* acc, budget->newAccounts) accountAdded(acc);
		budget->newAccounts.clear();
		foreach(Security *sec, budget->newSecurities) securityAdded(sec);
		budget->newSecurities.clear();
	}
	if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
	budget->setRecordNewAccounts(false);
	budget->setRecordNewSecurities(false);
	return false;
}
void Eqonomize::newScheduledExpense() {
	newScheduledTransaction(TRANSACTION_TYPE_EXPENSE);
}
void Eqonomize::newScheduledIncome() {
	newScheduledTransaction(TRANSACTION_TYPE_INCOME);
}
void Eqonomize::newScheduledTransfer() {
	newScheduledTransaction(TRANSACTION_TYPE_TRANSFER);
}
bool Eqonomize::editScheduledTransaction(ScheduledTransaction *strans) {
	return editScheduledTransaction(strans, this);
}
bool Eqonomize::editScheduledTransaction(ScheduledTransaction *strans, QWidget *parent) {
	budget->setRecordNewAccounts(true);
	budget->setRecordNewSecurities(true);
	budget->resetDefaultCurrencyChanged();
	budget->resetCurrenciesModified();
	budget->resetDefaultCurrencyChanged();
	budget->resetCurrenciesModified();
	if(strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SINGLE) {
		ScheduledTransaction *old_strans = strans->copy();
		if(EditScheduledTransactionDialog::editScheduledTransaction(strans, parent, true, b_extra, true)) {
			foreach(Account* acc, budget->newAccounts) accountAdded(acc);
			budget->newAccounts.clear();
			foreach(Security *sec, budget->newSecurities) securityAdded(sec);
			budget->newSecurities.clear();
			if(!strans->recurrence() && strans->transaction()->date() <= QDate::currentDate()) {
				Transactions *trans = strans->transaction()->copy();
				budget->removeScheduledTransaction(strans, true);
				transactionModified(strans, old_strans);
				transactionRemoved(strans);
				delete strans;
				budget->addTransactions(trans);
				transactionAdded(trans);
			} else {
				transactionModified(strans, old_strans);
				checkSchedule(true, parent);
			}
			delete old_strans;
			return true;
		} else {
			foreach(Account* acc, budget->newAccounts) accountAdded(acc);
			budget->newAccounts.clear();
			foreach(Security *sec, budget->newSecurities) securityAdded(sec);
			budget->newSecurities.clear();
		}
		delete old_strans;
	} else if(strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT) {
		ScheduledTransaction *strans_new = NULL;
		if(((SplitTransaction*) strans->transaction())->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ITEMS) {
			strans_new = EditScheduledMultiItemDialog::editScheduledTransaction(strans, parent, b_extra, true);
		} else if(((SplitTransaction*) strans->transaction())->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ACCOUNTS) {
			strans_new = EditScheduledMultiAccountDialog::editScheduledTransaction(strans, parent, b_extra, true);
		} else if(((SplitTransaction*) strans->transaction())->type() == SPLIT_TRANSACTION_TYPE_LOAN) {
			strans_new = EditScheduledDebtPaymentDialog::editScheduledTransaction(strans, parent, b_extra, true);
		}
		if(strans_new) {
			foreach(Account* acc, budget->newAccounts) accountAdded(acc);
			budget->newAccounts.clear();
			foreach(Security *sec, budget->newSecurities) securityAdded(sec);
			budget->newSecurities.clear();
			budget->removeScheduledTransaction(strans, true);
			transactionRemoved(strans);
			delete strans;
			strans = strans_new;
			if(!strans->recurrence() && strans->transaction()->date() <= QDate::currentDate()) {
				Transactions *trans = strans->transaction()->copy();
				budget->removeScheduledTransaction(strans, true);
				transactionRemoved(strans);
				delete strans;
				budget->addTransactions(trans);
				transactionAdded(trans);
			} else {
				budget->addScheduledTransaction(strans);
				transactionAdded(strans);
				checkSchedule(true, parent);
			}
			if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
			budget->setRecordNewAccounts(false);
			budget->setRecordNewSecurities(false);
			return true;
		} else {
			foreach(Account* acc, budget->newAccounts) accountAdded(acc);
			budget->newAccounts.clear();
			foreach(Security *sec, budget->newSecurities) securityAdded(sec);
			budget->newSecurities.clear();
		}
	}
	if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
	budget->setRecordNewAccounts(false);
	budget->setRecordNewSecurities(false);
	return false;
}
bool Eqonomize::editOccurrence(ScheduledTransaction *strans, const QDate &date) {
	return editOccurrence(strans, date, this);
}
bool Eqonomize::editOccurrence(ScheduledTransaction *strans, const QDate &date, QWidget *parent) {
	budget->setRecordNewAccounts(true);
	budget->setRecordNewSecurities(true);
	budget->resetDefaultCurrencyChanged();
	budget->resetCurrenciesModified();
	if(strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SINGLE) {
		Security *security = NULL;
		if(strans->transactiontype() == TRANSACTION_TYPE_SECURITY_BUY || strans->transactiontype() == TRANSACTION_TYPE_SECURITY_SELL) {
			security = ((SecurityTransaction*) strans->transaction())->security();
		} else if(strans->transactiontype() == TRANSACTION_TYPE_INCOME && ((Income*) strans->transaction())->security()) {
			security = ((Income*) strans->transaction())->security();
		}
		TransactionEditDialog *dialog = new TransactionEditDialog(b_extra, strans->transactionsubtype() == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND ? TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND : strans->transactiontype(), NULL, false, security, SECURITY_ALL_VALUES, security != NULL, budget, parent, true);
		dialog->editWidget->updateAccounts();
		dialog->editWidget->setTransaction((Transaction*) strans->transaction(), date);
		if(dialog->editWidget->checkAccounts() && dialog->exec() == QDialog::Accepted) {
			foreach(Account* acc, budget->newAccounts) accountAdded(acc);
			budget->newAccounts.clear();
			foreach(Security *sec, budget->newSecurities) securityAdded(sec);
			budget->newSecurities.clear();
			Transaction *trans = dialog->editWidget->createTransaction();
			if(trans) {
				if(trans->date() > QDate::currentDate()) {
					ScheduledTransaction *strans_new = new ScheduledTransaction(budget, trans, NULL);
					budget->addScheduledTransaction(strans_new);
					transactionAdded(strans_new);
				} else {
					budget->addTransaction(trans);
					transactionAdded(trans);
				}
				ScheduledTransaction *old_strans = strans->copy();
				strans->addException(date);
				transactionModified(strans, old_strans);
				delete old_strans;
				dialog->deleteLater();
				if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
				budget->setRecordNewAccounts(false);
				budget->setRecordNewSecurities(false);
				return true;
			}
		}
		dialog->deleteLater();
	} else if(strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT) {
		if(((SplitTransaction*) strans->transaction())->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ITEMS) {
			EditMultiItemDialog *dialog = new EditMultiItemDialog(budget, parent, NULL, b_extra, true);
			dialog->editWidget->setTransaction((MultiItemTransaction*) strans->transaction(), date);
			if(dialog->editWidget->checkAccounts() && dialog->exec() == QDialog::Accepted) {
				foreach(Account* acc, budget->newAccounts) accountAdded(acc);
				budget->newAccounts.clear();
				foreach(Security *sec, budget->newSecurities) securityAdded(sec);
				budget->newSecurities.clear();
				MultiItemTransaction *split = dialog->editWidget->createTransaction();
				if(split) {
					if(split->date() > QDate::currentDate()) {
						ScheduledTransaction *strans_new = new ScheduledTransaction(budget, split, NULL);
						budget->addScheduledTransaction(strans_new);
						transactionAdded(strans_new);
					} else {
						budget->addSplitTransaction(split);
						transactionAdded(split);
					}
					transactionRemoved(strans);
					strans->addException(date);
					transactionAdded(strans);
					dialog->deleteLater();
					if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
					budget->setRecordNewAccounts(false);
					budget->setRecordNewSecurities(false);
					return true;
				}
			}			
			dialog->deleteLater();
		} else if(((SplitTransaction*) strans->transaction())->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ACCOUNTS) {
			EditMultiAccountDialog *dialog = new EditMultiAccountDialog(budget, parent, ((MultiAccountTransaction*) strans->transaction())->transactiontype() == TRANSACTION_TYPE_EXPENSE, b_extra, true);
			dialog->editWidget->setTransaction((MultiAccountTransaction*) strans->transaction(), date);
			if(dialog->editWidget->checkAccounts() && dialog->exec() == QDialog::Accepted) {
				foreach(Account* acc, budget->newAccounts) accountAdded(acc);
				budget->newAccounts.clear();
				foreach(Security *sec, budget->newSecurities) securityAdded(sec);
				budget->newSecurities.clear();
				MultiAccountTransaction *split = dialog->editWidget->createTransaction();
				if(split) {
					if(split->date() > QDate::currentDate()) {
						ScheduledTransaction *strans_new = new ScheduledTransaction(budget, split, NULL);
						budget->addScheduledTransaction(strans_new);
						transactionAdded(strans_new);
					} else {
						budget->addSplitTransaction(split);
						transactionAdded(split);
					}
					transactionRemoved(strans);
					strans->addException(date);
					transactionAdded(strans);
					dialog->deleteLater();
					if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
					budget->setRecordNewAccounts(false);
					budget->setRecordNewSecurities(false);
					return true;
				}
			}			
			dialog->deleteLater();
		} else if(((SplitTransaction*) strans->transaction())->type() == SPLIT_TRANSACTION_TYPE_LOAN) {
			EditDebtPaymentDialog *dialog = new EditDebtPaymentDialog(budget, parent, NULL, true);
			dialog->editWidget->setTransaction((DebtPayment*) strans->transaction(), date);
			if(dialog->editWidget->checkAccounts() && dialog->exec() == QDialog::Accepted) {
				foreach(Account* acc, budget->newAccounts) accountAdded(acc);
				budget->newAccounts.clear();
				foreach(Security *sec, budget->newSecurities) securityAdded(sec);
				budget->newSecurities.clear();
				DebtPayment *split = dialog->editWidget->createTransaction();
				if(split) {
					if(split->date() > QDate::currentDate()) {
						ScheduledTransaction *strans_new = new ScheduledTransaction(budget, split, NULL);
						budget->addScheduledTransaction(strans_new);
						transactionAdded(strans_new);
					} else {
						budget->addSplitTransaction(split);
						transactionAdded(split);
					}
					transactionRemoved(strans);
					strans->addException(date);
					transactionAdded(strans);
					dialog->deleteLater();
					if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
					budget->setRecordNewAccounts(false);
					budget->setRecordNewSecurities(false);
					return true;
				}
			}			
			dialog->deleteLater();
		}
	}
	foreach(Account* acc, budget->newAccounts) accountAdded(acc);
	budget->newAccounts.clear();
	foreach(Security *sec, budget->newSecurities) securityAdded(sec);
	budget->newSecurities.clear();
	if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
	budget->setRecordNewAccounts(false);
	budget->setRecordNewSecurities(false);
	return false;
}
void Eqonomize::editScheduledTransaction() {
	ScheduleListViewItem *i = (ScheduleListViewItem*) selectedItem(scheduleView);
	if(i == NULL) return;
	editScheduledTransaction(i->scheduledTransaction());
}
void Eqonomize::editOccurrence() {
	ScheduleListViewItem *i = (ScheduleListViewItem*) selectedItem(scheduleView);
	if(i == NULL) return;
	editOccurrence(i->scheduledTransaction(), i->date());
}
bool Eqonomize::removeScheduledTransaction(ScheduledTransaction *strans) {
	budget->removeScheduledTransaction(strans, true);
	transactionRemoved(strans);
	delete strans;
	return true;
}
void Eqonomize::removeScheduledTransaction() {
	ScheduleListViewItem *i = (ScheduleListViewItem*) selectedItem(scheduleView);
	if(i == NULL) return;
	removeScheduledTransaction(i->scheduledTransaction());
}
bool Eqonomize::removeOccurrence(ScheduledTransaction *strans, const QDate &date) {
	if(strans->isOneTimeTransaction()) {
		removeScheduledTransaction(strans);
	} else if(strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SINGLE) {
		ScheduledTransaction *oldstrans = strans->copy();
		strans->addException(date);
		transactionModified(strans, oldstrans);
		delete oldstrans;
	} else {
		transactionRemoved(strans);
		strans->addException(date);
		transactionAdded(strans);
	}
	return true;
}
void Eqonomize::removeOccurrence() {
	ScheduleListViewItem *i = (ScheduleListViewItem*) selectedItem(scheduleView);
	if(i == NULL) return;
	removeOccurrence(i->scheduledTransaction(), i->date());
}
void Eqonomize::scheduleSelectionChanged() {
	ScheduleListViewItem *i = (ScheduleListViewItem*) selectedItem(scheduleView);
	if(i == NULL) {
		editScheduleButton->setEnabled(false);
		removeScheduleButton->setEnabled(false);
	} else {
		editScheduleButton->setEnabled(true);
		removeScheduleButton->setEnabled(true);
		ActionEditSchedule->setEnabled(true);
		ActionDeleteSchedule->setEnabled(true);
		ActionEditOccurrence->setEnabled(!i->scheduledTransaction()->isOneTimeTransaction());
		ActionDeleteOccurrence->setEnabled(!i->scheduledTransaction()->isOneTimeTransaction());
	}
}
void Eqonomize::scheduleExecuted(QTreeWidgetItem *i) {
	if(i == NULL) return;
	editScheduledTransaction(((ScheduleListViewItem*) i)->scheduledTransaction());
}
void Eqonomize::popupScheduleMenu(const QPoint &p) {
	if(!schedulePopupMenu) {
		schedulePopupMenu = new QMenu(this);
		schedulePopupMenu->addAction(ActionEditScheduledTransaction);
		schedulePopupMenu->addAction(ActionEditTransaction);
		schedulePopupMenu->addSeparator();
		schedulePopupMenu->addAction(ActionDeleteScheduledTransaction);
		schedulePopupMenu->addAction(ActionDeleteTransaction);
	}
	schedulePopupMenu->popup(scheduleView->viewport()->mapToGlobal(p));
}

void Eqonomize::editSelectedScheduledTransaction() {
	TransactionListWidget *w = NULL;
	if(tabs->currentIndex() == ACCOUNTS_PAGE_INDEX) return;
	else if(tabs->currentIndex() == EXPENSES_PAGE_INDEX) w = expensesWidget;
	else if(tabs->currentIndex() == INCOMES_PAGE_INDEX) w = incomesWidget;
	else if(tabs->currentIndex() == TRANSFERS_PAGE_INDEX) w = transfersWidget;
	else if(tabs->currentIndex() == SECURITIES_PAGE_INDEX) return;
	else if(tabs->currentIndex() == SCHEDULE_PAGE_INDEX) {
		editScheduledTransaction();
		return;
	}
	if(!w) return;
	w->editScheduledTransaction();
}
void Eqonomize::editSelectedSplitTransaction() {
	TransactionListWidget *w = NULL;
	if(tabs->currentIndex() == EXPENSES_PAGE_INDEX) w = expensesWidget;
	else if(tabs->currentIndex() == INCOMES_PAGE_INDEX) w = incomesWidget;
	else if(tabs->currentIndex() == TRANSFERS_PAGE_INDEX) w = transfersWidget;
	else 	return;
	if(!w) return;
	w->editSplitTransaction();
}
void Eqonomize::joinSelectedTransactions() {
	TransactionListWidget *w = NULL;
	if(tabs->currentIndex() == EXPENSES_PAGE_INDEX) w = expensesWidget;
	else if(tabs->currentIndex() == INCOMES_PAGE_INDEX) w = incomesWidget;
	else if(tabs->currentIndex() == TRANSFERS_PAGE_INDEX) w = transfersWidget;
	else return;
	if(!w) return;
	w->joinTransactions();
}
void Eqonomize::splitUpSelectedTransaction() {
	TransactionListWidget *w = NULL;
	if(tabs->currentIndex() == EXPENSES_PAGE_INDEX) w = expensesWidget;
	else if(tabs->currentIndex() == INCOMES_PAGE_INDEX) w = incomesWidget;
	else if(tabs->currentIndex() == TRANSFERS_PAGE_INDEX) w = transfersWidget;
	else return;
	if(!w) return;
	w->splitUpTransaction();
}
bool Eqonomize::editTransaction(Transaction *trans) {
	return editTransaction(trans, this);
}
bool Eqonomize::editTransaction(Transaction *trans, QWidget *parent) {
	Transaction *oldtrans = trans->copy();
	Recurrence *rec = NULL;
	budget->setRecordNewAccounts(true);
	budget->setRecordNewSecurities(true);
	budget->resetDefaultCurrencyChanged();
	budget->resetCurrenciesModified();
	if(trans->parentSplit()) {
		SplitTransaction *split = trans->parentSplit();
		if(split->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ITEMS) {
			Security *security = NULL;
			if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY || trans->type() == TRANSACTION_TYPE_SECURITY_SELL) {
				security = ((SecurityTransaction*) trans)->security();
			} else if(trans->type() == TRANSACTION_TYPE_INCOME && ((Income*) trans)->security()) {
				security = ((Income*) trans)->security();
			}
			TransactionEditDialog *dialog = new TransactionEditDialog(b_extra, trans->type(), split->currency(), trans->fromAccount() != ((MultiItemTransaction*) split)->account(), security, SECURITY_ALL_VALUES, security != NULL, budget, parent, true);
			dialog->editWidget->updateAccounts(((MultiItemTransaction*) split)->account());
			dialog->editWidget->setTransaction(trans);
			if(dialog->exec() == QDialog::Accepted) {
				foreach(Account* acc, budget->newAccounts) accountAdded(acc);
				budget->newAccounts.clear();
				foreach(Security *sec, budget->newSecurities) securityAdded(sec);
				budget->newSecurities.clear();
				if(dialog->editWidget->modifyTransaction(trans)) {
					transactionModified(trans, oldtrans);
					delete oldtrans;
					if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
					budget->setRecordNewAccounts(false);
					budget->setRecordNewSecurities(false);
					return true;
				}
			}
			dialog->deleteLater();
		} else {
			if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
			budget->setRecordNewAccounts(false);
			budget->setRecordNewSecurities(false);
			return editSplitTransaction(split, parent);
		}
	} else if(EditScheduledTransactionDialog::editTransaction(trans, rec, parent, true, b_extra, true)) {
		foreach(Account* acc, budget->newAccounts) accountAdded(acc);
		budget->newAccounts.clear();
		foreach(Security *sec, budget->newSecurities) securityAdded(sec);
		budget->newSecurities.clear();
		if((!rec || rec->firstOccurrence() == rec->lastOccurrence()) && trans->date() <= QDate::currentDate()) {
			transactionModified(trans, oldtrans);
		} else {
			budget->removeTransaction(trans, true);
			transactionRemoved(trans);
			ScheduledTransaction *strans = new ScheduledTransaction(budget, trans, rec);
			if(strans->recurrence() && strans->date() <= QDate::currentDate() && ((QTime::currentTime().hour() >= 18 && strans->recurrence()->nextOccurrence(strans->date(), false) > QDate::currentDate()) || (QTime::currentTime().hour() < 18 && strans->recurrence()->nextOccurrence(strans->date(), false) >= QDate::currentDate()))) {
				Transactions *new_trans = strans->realize(strans->date());
				budget->addTransactions(new_trans);
				transactionAdded(new_trans);
				budget->addScheduledTransaction(strans);
				transactionAdded(strans);
			} else {
				budget->addScheduledTransaction(strans);
				transactionAdded(strans);
				checkSchedule(true, parent);
			}
		}
		delete oldtrans;
		if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
		budget->setRecordNewAccounts(false);
		budget->setRecordNewSecurities(false);
		return true;
	}
	foreach(Account* acc, budget->newAccounts) accountAdded(acc);
	budget->newAccounts.clear();
	foreach(Security *sec, budget->newSecurities) securityAdded(sec);
	budget->newSecurities.clear();
	if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
	budget->setRecordNewAccounts(false);
	budget->setRecordNewSecurities(false);
	delete oldtrans;
	return false;
}
void Eqonomize::newRefund() {
	if(tabs->currentIndex() == EXPENSES_PAGE_INDEX) expensesWidget->newRefundRepayment();
}
void Eqonomize::newRepayment() {
	if(tabs->currentIndex() == INCOMES_PAGE_INDEX) incomesWidget->newRefundRepayment();
}
void Eqonomize::newRefundRepayment() {
	if(tabs->currentIndex() == EXPENSES_PAGE_INDEX) expensesWidget->newRefundRepayment();
	else if(tabs->currentIndex() == INCOMES_PAGE_INDEX) incomesWidget->newRefundRepayment();
}
bool Eqonomize::newRefundRepayment(Transactions *trans) {
	if(!((trans->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT && ((SplitTransaction*) trans)->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ACCOUNTS) || (trans->generaltype() == GENERAL_TRANSACTION_TYPE_SINGLE && (((Transaction*) trans)->type() == TRANSACTION_TYPE_EXPENSE || ((Transaction*) trans)->type() == TRANSACTION_TYPE_INCOME)))) return false;
	RefundDialog *dialog = new RefundDialog(trans, this);
	if(dialog->exec() == QDialog::Accepted) {
		Transaction *new_trans = dialog->createRefund();
		if(new_trans) {
			if(new_trans->date() > QDate::currentDate()) {
				ScheduledTransaction *strans = new ScheduledTransaction(budget, new_trans, NULL);
				budget->addScheduledTransaction(strans);
				transactionAdded(strans);
			} else {
				budget->addTransaction(new_trans);
				transactionAdded(new_trans);
			}
			dialog->deleteLater();
			return true;
		}
	}
	dialog->deleteLater();
	return false;
}
void Eqonomize::editSelectedTransaction() {
	TransactionListWidget *w = NULL;
	if(tabs->currentIndex() == ACCOUNTS_PAGE_INDEX) return;
	else if(tabs->currentIndex() == EXPENSES_PAGE_INDEX) w = expensesWidget;
	else if(tabs->currentIndex() == INCOMES_PAGE_INDEX) w = incomesWidget;
	else if(tabs->currentIndex() == TRANSFERS_PAGE_INDEX) w = transfersWidget;
	else if(tabs->currentIndex() == SECURITIES_PAGE_INDEX) return;
	else if(tabs->currentIndex() == SCHEDULE_PAGE_INDEX) {
		editOccurrence();
		return;
	}
	if(!w) return;
	w->editTransaction();
}
void Eqonomize::selectAssociatedFile() {
	TransactionListWidget *w = NULL;
	if(tabs->currentIndex() == ACCOUNTS_PAGE_INDEX) return;
	else if(tabs->currentIndex() == EXPENSES_PAGE_INDEX) w = expensesWidget;
	else if(tabs->currentIndex() == INCOMES_PAGE_INDEX) w = incomesWidget;
	else if(tabs->currentIndex() == TRANSFERS_PAGE_INDEX) w = transfersWidget;
	else if(tabs->currentIndex() == SECURITIES_PAGE_INDEX) return;
	else if(tabs->currentIndex() == SCHEDULE_PAGE_INDEX) {
		ScheduleListViewItem *i = (ScheduleListViewItem*) selectedItem(scheduleView);
		if(i == NULL) return;
		QString url = QFileDialog::getOpenFileName(this, QString(), i->scheduledTransaction()->associatedFile().isEmpty() ? last_associated_file_directory : i->scheduledTransaction()->associatedFile());
		if(!url.isEmpty()) {
			QFileInfo fileInfo(url);
			last_associated_file_directory = fileInfo.absoluteDir().absolutePath();
			i->scheduledTransaction()->setAssociatedFile(url);
			ActionOpenAssociatedFile->setEnabled(true);
		}
		return;
	}
	if(!w) return;
	w->selectAssociatedFile();
}
void Eqonomize::openAssociatedFile() {
	TransactionListWidget *w = NULL;
	if(tabs->currentIndex() == ACCOUNTS_PAGE_INDEX) return;
	else if(tabs->currentIndex() == EXPENSES_PAGE_INDEX) w = expensesWidget;
	else if(tabs->currentIndex() == INCOMES_PAGE_INDEX) w = incomesWidget;
	else if(tabs->currentIndex() == TRANSFERS_PAGE_INDEX) w = transfersWidget;
	else if(tabs->currentIndex() == SECURITIES_PAGE_INDEX) return;
	else if(tabs->currentIndex() == SCHEDULE_PAGE_INDEX) {
		ScheduleListViewItem *i = (ScheduleListViewItem*) selectedItem(scheduleView);
		if(i == NULL) return;
		QDesktopServices::openUrl(QUrl::fromLocalFile(i->scheduledTransaction()->associatedFile()));
		return;
	}
	if(!w) return;
	w->openAssociatedFile();
}
void Eqonomize::deleteSelectedScheduledTransaction() {
	TransactionListWidget *w = NULL;
	if(tabs->currentIndex() == ACCOUNTS_PAGE_INDEX) return;
	else if(tabs->currentIndex() == EXPENSES_PAGE_INDEX) w = expensesWidget;
	else if(tabs->currentIndex() == INCOMES_PAGE_INDEX) w = incomesWidget;
	else if(tabs->currentIndex() == TRANSFERS_PAGE_INDEX) w = transfersWidget;
	else if(tabs->currentIndex() == SECURITIES_PAGE_INDEX) return;
	else if(tabs->currentIndex() == SCHEDULE_PAGE_INDEX) {
		removeScheduledTransaction();
		return;
	}
	if(!w) return;
	w->removeScheduledTransaction();
}
void Eqonomize::deleteSelectedSplitTransaction() {
	TransactionListWidget *w = NULL;
	if(tabs->currentIndex() == EXPENSES_PAGE_INDEX) w = expensesWidget;
	else if(tabs->currentIndex() == INCOMES_PAGE_INDEX) w = incomesWidget;
	else if(tabs->currentIndex() == TRANSFERS_PAGE_INDEX) w = transfersWidget;
	else return;
	if(!w) return;
	w->removeSplitTransaction();
}
void Eqonomize::deleteSelectedTransaction() {
	TransactionListWidget *w = NULL;
	if(tabs->currentIndex() == ACCOUNTS_PAGE_INDEX) return;
	else if(tabs->currentIndex() == EXPENSES_PAGE_INDEX) w = expensesWidget;
	else if(tabs->currentIndex() == INCOMES_PAGE_INDEX) w = incomesWidget;
	else if(tabs->currentIndex() == TRANSFERS_PAGE_INDEX) w = transfersWidget;
	else if(tabs->currentIndex() == SECURITIES_PAGE_INDEX) return;
	else if(tabs->currentIndex() == SCHEDULE_PAGE_INDEX) {
		removeOccurrence();
		return;
	}
	if(!w) return;
	w->removeTransaction();
}

void Eqonomize::onPageChange(int index) {
	if(index == EXPENSES_PAGE_INDEX) {
		expensesWidget->onDisplay();
	} else if(index == INCOMES_PAGE_INDEX) {
		incomesWidget->onDisplay();
	} else if(index == TRANSFERS_PAGE_INDEX) {
		transfersWidget->onDisplay();
	}
	if(index == ACCOUNTS_PAGE_INDEX) {
		accountsSelectionChanged();
	} else {
		ActionDeleteAccount->setEnabled(false);
		ActionCloseAccount->setEnabled(false);
		ActionEditAccount->setEnabled(false);
		ActionBalanceAccount->setEnabled(false);
		ActionReconcileAccount->setEnabled(false);
		ActionShowAccountTransactions->setEnabled(false);
	}
	if(index == SECURITIES_PAGE_INDEX) {
		securitiesSelectionChanged();
	} else {
		ActionEditSecurity->setEnabled(false);
		ActionDeleteSecurity->setEnabled(false);
		ActionSetQuotation->setEnabled(false);
		ActionEditQuotations->setEnabled(false);
		ActionEditSecurityTransactions->setEnabled(false);
	}
	updateTransactionActions();
}
void Eqonomize::updateTransactionActions() {
	TransactionListWidget *w = NULL;
	bool b_transaction = false, b_scheduledtransaction = false, b_attachment = false;
	if(tabs->currentIndex() == EXPENSES_PAGE_INDEX) w = expensesWidget;
	else if(tabs->currentIndex() == INCOMES_PAGE_INDEX) w = incomesWidget;
	else if(tabs->currentIndex() == TRANSFERS_PAGE_INDEX) w = transfersWidget;
	else if(tabs->currentIndex() == SCHEDULE_PAGE_INDEX) {
		ScheduleListViewItem *i = (ScheduleListViewItem*) selectedItem(scheduleView);
		b_transaction = i && !i->scheduledTransaction()->isOneTimeTransaction();
		b_attachment = i && !i->scheduledTransaction()->associatedFile().isEmpty();
		b_scheduledtransaction = (i != NULL);
	} else {}
	if(w) {
		w->updateTransactionActions();
	} else {
		ActionJoinTransactions->setEnabled(false);
		ActionSplitUpTransaction->setEnabled(false);
		ActionEditSplitTransaction->setEnabled(false);
		ActionDeleteSplitTransaction->setEnabled(false);
		ActionEditTransaction->setEnabled(b_transaction);
		ActionDeleteTransaction->setEnabled(b_transaction);
		ActionSelectAssociatedFile->setEnabled(b_transaction);
		ActionOpenAssociatedFile->setEnabled(b_attachment);
		ActionEditScheduledTransaction->setEnabled(b_scheduledtransaction);
		ActionDeleteScheduledTransaction->setEnabled(b_scheduledtransaction);
		ActionNewRefund->setEnabled(false);
		ActionNewRepayment->setEnabled(false);
		ActionNewRefundRepayment->setEnabled(false);
	}
}

void Eqonomize::popupAccountsMenu(const QPoint &p) {
	QTreeWidgetItem *i = accountsView->itemAt(p);
	if(i == NULL) return;
	if(i == liabilitiesItem || (account_items.contains(i) && (account_items[i]->type() == ACCOUNT_TYPE_ASSETS) && (((AssetsAccount*) account_items[i])->accountType() == ASSETS_TYPE_LIABILITIES))) {
		ActionAddAccount->setText(tr("Add Loan"));
	} else if(i == assetsItem || (account_items.contains(i) && (account_items[i]->type() == ACCOUNT_TYPE_ASSETS))) {
		ActionAddAccount->setText(tr("Add Account"));
	} else {
		ActionAddAccount->setText(tr("Add Category"));
		
	}
	if(i == liabilitiesItem || i == assetsItem || (account_items.contains(i) && account_items[i]->type() == ACCOUNT_TYPE_ASSETS)) {
		if(!assetsPopupMenu) {
			assetsPopupMenu = new QMenu(this);
			assetsPopupMenu->addAction(ActionAddAccount);
			assetsPopupMenu->addAction(ActionEditAccount);
			assetsPopupMenu->addAction(ActionReconcileAccount);
			assetsPopupMenu->addAction(ActionBalanceAccount);
			assetsPopupMenu->addAction(ActionCloseAccount);
			assetsPopupMenu->addAction(ActionDeleteAccount);
			assetsPopupMenu->addSeparator();
			assetsPopupMenu->addAction(ActionShowAccountTransactions);
		}
		assetsPopupMenu->popup(accountsView->viewport()->mapToGlobal(p));
	} else {
		if(!accountPopupMenu) {
			accountPopupMenu = new QMenu(this);
			accountPopupMenu->addAction(ActionAddAccount);
			accountPopupMenu->addAction(ActionEditAccount);
			accountPopupMenu->addAction(ActionDeleteAccount);
			accountPopupMenu->addSeparator();
			accountPopupMenu->addAction(ActionShowAccountTransactions);
		}
		accountPopupMenu->popup(accountsView->viewport()->mapToGlobal(p));
	}

}

void Eqonomize::showLedger() {
	if(budget->assetsAccounts.isEmpty()) return;
	QTreeWidgetItem *i = selectedItem(accountsView);
	Account *account = NULL;
	if(i && i != assetsItem && i != liabilitiesItem && i != incomesItem && i != expensesItem) {
		account = account_items[i];
		if(account && account->type() != ACCOUNT_TYPE_ASSETS) account = NULL;
	}
	if(!account) {
		account = budget->assetsAccounts[0];
		if(account == budget->balancingAccount && budget->assetsAccounts.count() > 1) account = budget->assetsAccounts[1];
	}
	LedgerDialog *dialog = new LedgerDialog((AssetsAccount*) account, this, tr("Ledger"), b_extra);
	dialog->show();
	connect(this, SIGNAL(timeToSaveConfig()), dialog, SLOT(saveConfig()));
}
void Eqonomize::reconcileAccount() {
	if(budget->assetsAccounts.isEmpty()) return;
	QTreeWidgetItem *i = selectedItem(accountsView);
	Account *account = NULL;
	if(i && i != assetsItem && i != liabilitiesItem && i != incomesItem && i != expensesItem) {
		account = account_items[i];
		if(account && account->type() != ACCOUNT_TYPE_ASSETS) account = NULL;
	}
	if(!account) {
		account = budget->assetsAccounts[0];
		if(account == budget->balancingAccount && budget->assetsAccounts.count() > 1) account = budget->assetsAccounts[1];
	}
	LedgerDialog *dialog = new LedgerDialog((AssetsAccount*) account, this, tr("Ledger"), b_extra, true);
	dialog->show();
	connect(this, SIGNAL(timeToSaveConfig()), dialog, SLOT(saveConfig()));
}
void Eqonomize::showAccountTransactions(bool b) {
	QTreeWidgetItem *i = selectedItem(accountsView);
	if(i == NULL || i == assetsItem || i == liabilitiesItem) return;
	if(i == incomesItem) {
		if(b) incomesWidget->setFilter(QDate(), to_date, -1.0, -1.0, NULL, NULL);
		else incomesWidget->setFilter(accountsPeriodFromButton->isChecked() ? from_date : QDate(), to_date, -1.0, -1.0, NULL, NULL);
		incomesWidget->showFilter();
		tabs->setCurrentIndex(INCOMES_PAGE_INDEX);
	} else if(i == expensesItem) {
		if(b) expensesWidget->setFilter(QDate(), to_date, -1.0, -1.0, NULL, NULL);
		else expensesWidget->setFilter(accountsPeriodFromButton->isChecked() ? from_date : QDate(), to_date, -1.0, -1.0, NULL, NULL);
		expensesWidget->showFilter();
		tabs->setCurrentIndex(EXPENSES_PAGE_INDEX);
	} else {
		Account *account = account_items[i];
		AccountType type = account->type();
		if(type == ACCOUNT_TYPE_INCOMES) {
			if(b) incomesWidget->setFilter(QDate(), to_date, -1.0, -1.0, account, NULL);
			else incomesWidget->setFilter(accountsPeriodFromButton->isChecked() ? from_date : QDate(), to_date, -1.0, -1.0, account, NULL);
			incomesWidget->showFilter();
			tabs->setCurrentIndex(INCOMES_PAGE_INDEX);
		} else if(type == ACCOUNT_TYPE_EXPENSES) {
			if(b) expensesWidget->setFilter(QDate(), to_date, -1.0, -1.0, NULL, account);
			else expensesWidget->setFilter(accountsPeriodFromButton->isChecked() ? from_date : QDate(), to_date, -1.0, -1.0, NULL, account);
			expensesWidget->showFilter();
			tabs->setCurrentIndex(EXPENSES_PAGE_INDEX);
		} else if(((AssetsAccount*) account)->accountType() == ASSETS_TYPE_SECURITIES) {
			tabs->setCurrentIndex(SECURITIES_PAGE_INDEX);
		} else {
			LedgerDialog *dialog = new LedgerDialog((AssetsAccount*) account, this, tr("Ledger"), b_extra);
			dialog->show();
			connect(this, SIGNAL(timeToSaveConfig()), dialog, SLOT(saveConfig()));
		}
	}
}
void Eqonomize::accountsPeriodToChanged(const QDate &date) {
	bool error = false;
	if(!date.isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		error = true;
	}
	if(!error && accountsPeriodFromEdit->date() > date) {
		if(accountsPeriodFromButton->isChecked()) {
			QMessageBox::critical(this, tr("Error"), tr("To date is before from date."));
		}
		from_date = date;
		accountsPeriodFromEdit->blockSignals(true);
		accountsPeriodFromEdit->setDate(from_date);
		accountsPeriodFromEdit->blockSignals(false);
	}
	if(error) {
		accountsPeriodToEdit->setFocus();
		accountsPeriodToEdit->blockSignals(true);
		accountsPeriodToEdit->setDate(to_date);
		accountsPeriodToEdit->blockSignals(false);
		accountsPeriodToEdit->selectAll();
		return;
	}
	to_date = date;
	filterAccounts();
}
void Eqonomize::accountsPeriodFromChanged(const QDate &date) {
	bool error = false;
	if(!date.isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		error = true;
	}
	if(!error && date > accountsPeriodToEdit->date()) {
		QMessageBox::critical(this, tr("Error"), tr("From date is after to date."));
		to_date = date;
		accountsPeriodToEdit->blockSignals(true);
		accountsPeriodToEdit->setDate(to_date);
		accountsPeriodToEdit->blockSignals(false);
	}
	if(error) {
		accountsPeriodFromEdit->setFocus();
		accountsPeriodFromEdit->blockSignals(true);
		accountsPeriodFromEdit->setDate(from_date);
		accountsPeriodFromEdit->blockSignals(false);
		accountsPeriodFromEdit->selectAll();
		return;
	}
	from_date = date;
	if(accountsPeriodFromButton->isChecked()) filterAccounts();
}
void Eqonomize::prevMonth() {
	accountsPeriodFromEdit->blockSignals(true);
	accountsPeriodToEdit->blockSignals(true);
	budget->goForwardBudgetMonths(from_date, to_date, -1);
	accountsPeriodFromEdit->setDate(from_date);
	accountsPeriodToEdit->setDate(to_date);
	accountsPeriodFromEdit->blockSignals(false);
	accountsPeriodToEdit->blockSignals(false);
	filterAccounts();
}
void Eqonomize::nextMonth() {
	accountsPeriodFromEdit->blockSignals(true);
	accountsPeriodToEdit->blockSignals(true);
	budget->goForwardBudgetMonths(from_date, to_date, 1);
	accountsPeriodFromEdit->setDate(from_date);
	accountsPeriodToEdit->setDate(to_date);
	accountsPeriodFromEdit->blockSignals(false);
	accountsPeriodToEdit->blockSignals(false);
	filterAccounts();
}
void Eqonomize::currentMonth() {
	accountsPeriodFromEdit->blockSignals(true);
	accountsPeriodToEdit->blockSignals(true);
	QDate curdate = QDate::currentDate();
	from_date == budget->firstBudgetDay(curdate);
	accountsPeriodFromEdit->setDate(from_date);
	to_date = curdate;
	accountsPeriodToEdit->setDate(to_date);
	accountsPeriodFromEdit->blockSignals(false);
	accountsPeriodToEdit->blockSignals(false);
	filterAccounts();
}
void Eqonomize::prevYear() {
	accountsPeriodFromEdit->blockSignals(true);
	accountsPeriodToEdit->blockSignals(true);
	budget->goForwardBudgetMonths(from_date, to_date, -12);
	accountsPeriodFromEdit->setDate(from_date);
	accountsPeriodToEdit->setDate(to_date);
	accountsPeriodFromEdit->blockSignals(false);
	accountsPeriodToEdit->blockSignals(false);
	filterAccounts();
}
void Eqonomize::nextYear() {
	accountsPeriodFromEdit->blockSignals(true);
	accountsPeriodToEdit->blockSignals(true);
	budget->goForwardBudgetMonths(from_date, to_date, 12);
	accountsPeriodFromEdit->setDate(from_date);
	accountsPeriodToEdit->setDate(to_date);
	accountsPeriodFromEdit->blockSignals(false);
	accountsPeriodToEdit->blockSignals(false);
	filterAccounts();
}
void Eqonomize::currentYear() {
	accountsPeriodFromEdit->blockSignals(true);
	accountsPeriodToEdit->blockSignals(true);
	QDate curdate = QDate::currentDate();
	from_date = budget->firstBudgetDayOfYear(curdate);
	accountsPeriodFromEdit->setDate(from_date);
	to_date = curdate;
	accountsPeriodToEdit->setDate(to_date);
	accountsPeriodFromEdit->blockSignals(false);
	accountsPeriodToEdit->blockSignals(false);
	filterAccounts();
}

void Eqonomize::securitiesPeriodToChanged(const QDate &date) {
	bool error = false;
	if(!date.isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		error = true;
	}
	if(!error && securitiesPeriodFromEdit->date() > date) {
		if(securitiesPeriodFromButton->isChecked()) {
			QMessageBox::critical(this, tr("Error"), tr("To date is before from date."));
		}
		securities_from_date = date;
		securitiesPeriodFromEdit->blockSignals(true);
		securitiesPeriodFromEdit->setDate(securities_from_date);
		securitiesPeriodFromEdit->blockSignals(false);
	}
	if(error) {
		securitiesPeriodToEdit->setFocus();
		securitiesPeriodToEdit->blockSignals(true);
		securitiesPeriodToEdit->setDate(securities_to_date);
		securitiesPeriodToEdit->blockSignals(false);
		securitiesPeriodToEdit->selectAll();
		return;
	}
	securities_to_date = date;
	updateSecurities();
}
void Eqonomize::securitiesPeriodFromChanged(const QDate &date) {
	bool error = false;
	if(!date.isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		error = true;
	}
	if(!error && date > securitiesPeriodToEdit->date()) {
		QMessageBox::critical(this, tr("Error"), tr("From date is after to date."));
		securities_to_date = date;
		securitiesPeriodToEdit->blockSignals(true);
		securitiesPeriodToEdit->setDate(securities_to_date);
		securitiesPeriodToEdit->blockSignals(false);
	}
	if(error) {
		securitiesPeriodFromEdit->setFocus();
		securitiesPeriodFromEdit->blockSignals(true);
		securitiesPeriodFromEdit->setDate(securities_from_date);
		securitiesPeriodFromEdit->blockSignals(false);
		securitiesPeriodFromEdit->selectAll();
		return;
	}
	securities_from_date = date;
	if(securitiesPeriodFromButton->isChecked()) updateSecurities();
}
void Eqonomize::securitiesPrevMonth() {
	securitiesPeriodFromEdit->blockSignals(true);
	securitiesPeriodToEdit->blockSignals(true);
	budget->goForwardBudgetMonths(securities_from_date, securities_to_date, -1);
	securitiesPeriodFromEdit->setDate(securities_from_date);
	securitiesPeriodToEdit->setDate(securities_to_date);
	securitiesPeriodFromEdit->blockSignals(false);
	securitiesPeriodToEdit->blockSignals(false);
	updateSecurities();
}
void Eqonomize::securitiesNextMonth() {	
	securitiesPeriodFromEdit->blockSignals(true);
	securitiesPeriodToEdit->blockSignals(true);
	budget->goForwardBudgetMonths(securities_from_date, securities_to_date, 1);
	securitiesPeriodFromEdit->setDate(securities_from_date);
	securitiesPeriodToEdit->setDate(securities_to_date);
	securitiesPeriodFromEdit->blockSignals(false);
	securitiesPeriodToEdit->blockSignals(false);
	updateSecurities();
}
void Eqonomize::securitiesCurrentMonth() {	
	securitiesPeriodFromEdit->blockSignals(true);
	securitiesPeriodToEdit->blockSignals(true);
	QDate curdate = QDate::currentDate();
	securities_from_date == budget->firstBudgetDay(curdate);
	securitiesPeriodFromEdit->setDate(securities_from_date);
	securities_to_date = curdate;
	securitiesPeriodToEdit->setDate(securities_to_date);
	securitiesPeriodFromEdit->blockSignals(false);
	securitiesPeriodToEdit->blockSignals(false);
	updateSecurities();
}
void Eqonomize::securitiesPrevYear() {
	securitiesPeriodFromEdit->blockSignals(true);
	securitiesPeriodToEdit->blockSignals(true);
	budget->goForwardBudgetMonths(securities_from_date, securities_to_date, -12);
	securitiesPeriodFromEdit->setDate(securities_from_date);
	securitiesPeriodToEdit->setDate(securities_to_date);
	securitiesPeriodFromEdit->blockSignals(false);
	securitiesPeriodToEdit->blockSignals(false);
	updateSecurities();
}
void Eqonomize::securitiesNextYear() {	
	securitiesPeriodFromEdit->blockSignals(true);
	securitiesPeriodToEdit->blockSignals(true);
	budget->goForwardBudgetMonths(securities_from_date, securities_to_date, 12);
	securitiesPeriodFromEdit->setDate(securities_from_date);
	securitiesPeriodToEdit->setDate(securities_to_date);
	securitiesPeriodFromEdit->blockSignals(false);
	securitiesPeriodToEdit->blockSignals(false);
	updateSecurities();
}
void Eqonomize::securitiesCurrentYear() {
	securitiesPeriodFromEdit->blockSignals(true);
	securitiesPeriodToEdit->blockSignals(true);
	QDate curdate = QDate::currentDate();
	securities_from_date = budget->firstBudgetDayOfYear(curdate);
	securitiesPeriodFromEdit->setDate(securities_from_date);
	securities_to_date = curdate;
	securitiesPeriodToEdit->setDate(securities_to_date);
	securitiesPeriodFromEdit->blockSignals(false);
	securitiesPeriodToEdit->blockSignals(false);
	updateSecurities();
}

void Eqonomize::setModified(bool has_been_modified) {
	modified_auto_save = has_been_modified;
	if(has_been_modified) autoSave();
	if(modified == has_been_modified) return;
	modified = has_been_modified;
	ActionFileSave->setEnabled(modified || !current_url.isValid());
	setWindowModified(has_been_modified);
	if(modified) {
		if(!in_batch_edit) emit budgetUpdated();
	} else {
		auto_save_timeout = true;
	}
}
void Eqonomize::createDefaultBudget() {
	if(!askSave()) return;
	budget->clear();
	bool new_currency = false, adjust_contents = false;
	if(current_url.isEmpty()) {
		new_currency = budget->resetDefaultCurrency();
		if(first_run) adjust_contents = true;
	}
	current_url = "";
	setWindowTitle(tr("Untitled") + "[*]");
	ActionFileReload->setEnabled(false);
	QSettings settings;
	settings.beginGroup("GeneralOptions");
	settings.setValue("lastURL", current_url.url());
	if(!cr_tmp_file.isEmpty()) {
		QFile autosaveFile(cr_tmp_file);
		autosaveFile.remove();
		cr_tmp_file = "";
	}
	settings.endGroup();
	settings.sync();
	
	budget->addAccount(new AssetsAccount(budget, ASSETS_TYPE_CASH, tr("Cash")));
	budget->addAccount(new AssetsAccount(budget, ASSETS_TYPE_CURRENT, tr("Checking Account", "Transactional account")));
	budget->addAccount(new AssetsAccount(budget, ASSETS_TYPE_SAVINGS, tr("Savings Account")));
	budget->addAccount(new IncomesAccount(budget, tr("Salary")));
	budget->addAccount(new IncomesAccount(budget, tr("Other")));
	budget->addAccount(new ExpensesAccount(budget, tr("Bills")));
	budget->addAccount(new ExpensesAccount(budget, tr("Clothing")));
	budget->addAccount(new ExpensesAccount(budget, tr("Groceries")));
	budget->addAccount(new ExpensesAccount(budget, tr("Leisure")));
	budget->addAccount(new ExpensesAccount(budget, tr("Other")));
	
	if(new_currency) warnAndAskForExchangeRate();

	reloadBudget();
	if(adjust_contents) {
		accountsView->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow);
	}
	setModified(false);
	ActionFileSave->setEnabled(true);
	emit accountsModified();
	emit transactionsModified();
	emit budgetUpdated();

}
void Eqonomize::reloadBudget() {
	incomes_accounts_value = 0.0;
	incomes_accounts_change = 0.0;
	expenses_accounts_value = 0.0;
	expenses_accounts_change = 0.0;
	assets_accounts_value = 0.0;
	assets_accounts_change = 0.0;
	liabilities_accounts_value = 0.0;
	liabilities_accounts_change = 0.0;
	expenses_budget = 0.0;
	expenses_budget_diff = 0.0;
	incomes_budget = 0.0;
	incomes_budget_diff = 0.0;
	account_value.clear();
	account_change.clear();
	while(assetsItem->childCount() > 0) {
		delete assetsItem->child(0);
	}
	while(liabilitiesItem->childCount() > 0) {
		delete liabilitiesItem->child(0);
	}
	while(incomesItem->childCount() > 0) {
		delete incomesItem->child(0);
	}
	while(expensesItem->childCount() > 0) {
		delete expensesItem->child(0);
	}
	account_items.clear();
	item_accounts.clear();
	for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
		AssetsAccount *aaccount = *it;
		if(aaccount != budget->balancingAccount) {
			appendAssetsAccount(aaccount);
		}
	}
	for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
		IncomesAccount *iaccount = *it;
		if(!iaccount->parentCategory()) appendIncomesAccount(iaccount, incomesItem);
	}
	for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
		ExpensesAccount *eaccount = *it;
		if(!eaccount->parentCategory()) appendExpensesAccount(eaccount, expensesItem);
	}
	account_value[budget->balancingAccount] = 0.0;
	account_change[budget->balancingAccount] = 0.0;
	expensesWidget->updateAccounts();
	incomesWidget->updateAccounts();
	transfersWidget->updateAccounts();
	assetsItem->setExpanded(true);
	liabilitiesItem->setExpanded(true);
	incomesItem->setExpanded(true);
	expensesItem->setExpanded(true);
	expensesWidget->transactionsReset();
	incomesWidget->transactionsReset();
	transfersWidget->transactionsReset();
	updateBudgetDay();
	updateScheduledTransactions();
	updateSecurities();
	updateUsesMultipleCurrencies();
	budgetEdit->setCurrency(budget->defaultCurrency());
}
bool Eqonomize::openURL(const QUrl& url, bool merge) {

	if(!merge && url != current_url && crashRecovery(QUrl(url))) return true;

	bool ignore_duplicate_transactions = false, rename_duplicate_accounts = false, rename_duplicate_categories = false, rename_duplicate_securities = false;
	if(merge) {
		QDialog *dialog = new QDialog(this, 0);
		dialog->setWindowTitle(tr("Import Options"));
		dialog->setModal(true);
		QVBoxLayout *box1 = new QVBoxLayout(dialog);
		QCheckBox *idtButton = new QCheckBox(tr("Ignore duplicate transactions"), dialog);
		box1->addWidget(idtButton);
		QCheckBox *rdaButton = new QCheckBox(tr("Rename duplicate accounts"), dialog);
		box1->addWidget(rdaButton);
		QCheckBox *rdcButton = new QCheckBox(tr("Rename duplicate categories"), dialog);
		box1->addWidget(rdcButton);
		QCheckBox *rdsButton = new QCheckBox(tr("Rename duplicate securities"), dialog);
		box1->addWidget(rdsButton);
		QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
		connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), dialog, SLOT(reject()));
		connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(accept()));
		box1->addWidget(buttonBox);
		if(dialog->exec() != QDialog::Accepted) return false;
		ignore_duplicate_transactions = idtButton->isChecked();
		rename_duplicate_accounts = rdaButton->isChecked();
		rename_duplicate_categories = rdcButton->isChecked();
		rename_duplicate_securities = rdsButton->isChecked();
		dialog->deleteLater();
	}

	QString errors;
	bool new_currency = false;
	QString error = budget->loadFile(url.toLocalFile(), errors, &new_currency, merge, rename_duplicate_accounts, rename_duplicate_categories, rename_duplicate_securities, ignore_duplicate_transactions);
	if(!error.isNull()) {
		QMessageBox::critical(this, tr("Couldn't open file"), tr("Error loading %1: %2.").arg(url.toString()).arg(error));
		return false;
	}
	if(!errors.isEmpty()) {
		QMessageBox::critical(this, tr("Error"), errors);
	}
	
	if(!merge) {
		setWindowTitle(url.fileName() + "[*]");
		current_url = url;
		ActionFileReload->setEnabled(true);
		QSettings settings;
		settings.beginGroup("GeneralOptions");
		settings.setValue("lastURL", current_url.url());
		if(!cr_tmp_file.isEmpty()) {
			QFile autosaveFile(cr_tmp_file);
			autosaveFile.remove();
			cr_tmp_file = "";
		}
		settings.endGroup();
		settings.sync();
		updateRecentFiles(url.toLocalFile());
		
		if(budget->autosyncEnabled()) {
			sync(true, true);
		}
	
		if(new_currency) warnAndAskForExchangeRate();
	}
	
	Currency *cur = budget->defaultCurrency();
	if(cur != budget->currency_euro && cur->exchangeRateSource() == EXCHANGE_RATE_SOURCE_NONE) cur = NULL;
	for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
		AssetsAccount *acc = *it;
		if(acc->currency() != NULL && acc->currency() != cur && (acc->currency() == budget->currency_euro || acc->currency()->exchangeRateSource() != EXCHANGE_RATE_SOURCE_NONE)) {
			if(cur) {
				if(timeToUpdateExchangeRates()) updateExchangeRates(false);
				break;
			}
			cur = acc->currency();
		}
	}

	reloadBudget();
	
	if(!merge) {
		expensesWidget->setDefaultAccounts();
		incomesWidget->setDefaultAccounts();
		transfersWidget->setDefaultAccounts();
	}

	emit accountsModified();
	emit transactionsModified();
	emit budgetUpdated();

	setModified(false);
	ActionFileSave->setEnabled(false);
	
	checkSchedule(true, this);
	
	return true;

}

void Eqonomize::openSynchronizationSettings() {
	syncDialog = new QDialog(this, 0);
	syncDialog->setWindowTitle(tr("Synchronization Settings"));
	QVBoxLayout *box1 = new QVBoxLayout(syncDialog);
	QGridLayout *grid = new QGridLayout();
	grid->addWidget(new QLabel(tr("Web address:"), syncDialog), 0, 0);
	syncUrlEdit = new QLineEdit(budget->o_sync->url, syncDialog);
	syncUrlEdit->setPlaceholderText(tr("optional"));
	grid->addWidget(syncUrlEdit, 0, 1);
	grid->addWidget(new QLabel(tr("Download command:"), syncDialog), 1, 0);
	syncDownloadEdit = new QLineEdit(budget->o_sync->download, syncDialog);
	syncDownloadEdit->setPlaceholderText(tr("optional"));
	grid->addWidget(syncDownloadEdit, 1, 1);
	grid->addWidget(new QLabel(tr("Upload command:"), syncDialog), 2, 0);
	syncUploadEdit = new QLineEdit(budget->o_sync->upload, syncDialog);
	syncUploadEdit->setPlaceholderText(tr("mandatory"));
	grid->addWidget(syncUploadEdit, 2, 1);
	grid->addWidget(new QLabel(tr("%f = local file (temporary), %u = url"), syncDialog), 3, 1);
	syncAutoBox = new QCheckBox(tr("Automatic synchronization"), syncDialog);
	syncAutoBox->setChecked(budget->o_sync->autosync);
	grid->addWidget(syncAutoBox, 4, 0, 1, 2);
	box1->addLayout(grid);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	uploadButton = buttonBox->addButton(tr("Upload"), QDialogButtonBox::ActionRole);
	uploadButton->setEnabled(!budget->o_sync->upload.isEmpty());
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), syncDialog, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), syncDialog, SLOT(accept()));
	connect(uploadButton, SIGNAL(clicked()), this, SLOT(uploadClicked()));
	connect(syncUploadEdit, SIGNAL(textChanged(const QString&)), this, SLOT(syncUploadChanged(const QString&)));
	box1->addWidget(buttonBox);
	if(syncDialog->exec() == QDialog::Accepted) {
		budget->o_sync->url = syncUrlEdit->text().trimmed();
		budget->o_sync->download = syncDownloadEdit->text().trimmed();
		budget->o_sync->upload = syncUploadEdit->text().trimmed();
		budget->o_sync->autosync = syncAutoBox->isChecked();
		setModified(true);
	}
	syncDialog->deleteLater();
}
void Eqonomize::syncUploadChanged(const QString &text) {
	uploadButton->setEnabled(!text.trimmed().isEmpty());
}
void Eqonomize::uploadClicked() {
	if(modified) {
		if(!current_url.isValid()) {
			if(saveAs(true, false, syncDialog)) return;
		} else {
			if(!saveURL(current_url, true, false, syncDialog)) return;
		}
	}
	QString upload_bak = budget->o_sync->upload;
	QString download_bak = budget->o_sync->download;
	QString url_bak = budget->o_sync->url;
	bool autosync_bak = budget->o_sync->autosync;
	budget->o_sync->url = syncUrlEdit->text().trimmed();
	budget->o_sync->download = syncDownloadEdit->text().trimmed();
	budget->o_sync->upload = syncUploadEdit->text().trimmed();
	budget->o_sync->autosync = syncAutoBox->isChecked();
	QTemporaryFile file;
	file.open();
	QProgressDialog *syncProgressDialog = new QProgressDialog(tr("Uploading…"), tr("Abort"), 0, 1, syncDialog);
	syncProgressDialog->setWindowModality(Qt::WindowModal);
	syncProgressDialog->setMinimumDuration(200);
	syncProgressDialog->setMaximum(1);
	connect(syncProgressDialog, SIGNAL(canceled()), this, SLOT(cancelSync()));
	syncProgressDialog->setValue(0);
	file.close();
	QString error = budget->syncUpload(file.fileName());
	if(!error.isEmpty()) {
		syncProgressDialog->reset();
		syncProgressDialog->deleteLater();
		QMessageBox::critical(syncDialog, tr("Error uploading file"), tr("Error uploading %1: %2.").arg(current_url.toString()).arg(error));
	} else {
		syncProgressDialog->setValue(1);
		syncProgressDialog->deleteLater();
		setModified(true);
	}
	budget->o_sync->url = url_bak;
	budget->o_sync->upload = upload_bak;
	budget->o_sync->download = download_bak;
	budget->o_sync->autosync = autosync_bak;
}
void Eqonomize::fileSynchronize() {
	if(budget->o_sync->isComplete()) {
		bool b = true;
		if(modified) {
			if(!current_url.isValid()) {
				b = saveAs(true, false, syncDialog);
			} else {
				b = saveURL(current_url, true, false, syncDialog);
			}
		}
		if(b) sync(true, true);
	} else {
		openSynchronizationSettings();
	}
}

void Eqonomize::cancelSync() {
	budget->cancelSync();
}
void Eqonomize::sync(bool do_save, bool on_load, QWidget *parent) {
	if(!parent) parent = this;
	QProgressDialog *syncProgressDialog = new QProgressDialog(tr("Synchronizing…"), tr("Abort"), 0, 1, parent);
	syncProgressDialog->setWindowModality(Qt::WindowModal);
	syncProgressDialog->setMinimumDuration(200);
	syncProgressDialog->setMaximum(1);
	connect(syncProgressDialog, SIGNAL(canceled()), this, SLOT(cancelSync()));
	syncProgressDialog->setValue(0);
	QString error, errors;
	int rev_bak = budget->o_sync->revision;
	if(budget->sync(error, errors, true, on_load)) {
		syncProgressDialog->setValue(1);
		syncProgressDialog->deleteLater();
		if(!error.isEmpty()) QMessageBox::critical(parent, tr("Error synchronizing file"), tr("Error synchronizing %1: %2.").arg(current_url.toString()).arg(error));
		else if(!errors.isEmpty()) QMessageBox::critical(parent, tr("Synchronization error"), errors);
		if(do_save) {
			saveURL(current_url, false, false, parent);
		}
		reloadBudget();
	} else {
		syncProgressDialog->reset();
		syncProgressDialog->deleteLater();
		if(!error.isEmpty()) QMessageBox::critical(parent, tr("Error synchronizing file"), tr("Error synchronizing %1: %2.").arg(current_url.toString()).arg(error));
		if(rev_bak != budget->o_sync->revision) {
			if(do_save) {
				QString error = budget->saveFile(current_url.toLocalFile(), QFile::ReadUser | QFile::WriteUser, true);
				if(!error.isNull()) {
					QMessageBox::critical(parent, tr("Couldn't save file"), tr("Error saving %1: %2.").arg(current_url.toString()).arg(error));
				}
			} else {
				setModified(true);
			}
		}
	}
}

bool Eqonomize::saveURL(const QUrl& url, bool do_local_sync, bool do_cloud_sync, QWidget *parent) {
	if(!parent) parent = this;
	bool exists = QFile::exists(url.toLocalFile());
	if(exists) {
		if(do_local_sync && url == current_url) {
			QString error;
			if(budget->isUnsynced(url.toLocalFile(), error)) {
				switch(QMessageBox::question(parent, tr("Synchronize file?"), tr("The file has been modified by a different user or program. Do you wish to merge changes?"), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes)) {
					case QMessageBox::Yes: {
						QString errors;
						error = budget->syncFile(url.toLocalFile(), errors);
						if(!error.isNull()) {
							QMessageBox::critical(parent, tr("Couldn't open file"), tr("Error loading %1: %2.").arg(url.toString()).arg(error));
							return false;
						}
						if(!errors.isEmpty()) {
							QMessageBox::critical(parent, tr("Error"), errors);
							return false;
						}
						reloadBudget();
						break;
					}
					case QMessageBox::No: {break;}
					default: {
						return false;
					}
				}
			} else if(!error.isNull()) {
				QMessageBox::critical(parent, tr("Couldn't save file"), tr("Error saving %1: %2.").arg(url.toString()).arg(error));
				return false;
			}
		}
		if(!QFile::exists(url.toLocalFile() + "~") || QFile::remove(url.toLocalFile() + "~")) {
			QFile::copy(url.toLocalFile(), url.toLocalFile() + "~");
		}		
		QFileInfo urlinfo(url.toLocalFile());
		QDir urldir(urlinfo.absolutePath());
		int i_backup_frequency = BACKUP_WEEKLY;
		if(ActionSelectBackupFrequency->checkedAction() == ABFNever) {i_backup_frequency = BACKUP_NEVER;}
		else if(ActionSelectBackupFrequency->checkedAction() == ABFDaily) {i_backup_frequency = BACKUP_DAILY;}
		else if(ActionSelectBackupFrequency->checkedAction() == ABFFortnightly) {i_backup_frequency = BACKUP_FORTNIGHTLY;}
		else if(ActionSelectBackupFrequency->checkedAction() == ABFMonthly) {i_backup_frequency = BACKUP_MONTHLY;}
		if(i_backup_frequency > BACKUP_NEVER && (urldir.exists(urlinfo.baseName() + "-backup") || urldir.mkdir(urlinfo.baseName() + "-backup"))) {
			QDir backupdir(urldir.absolutePath() + QString("/") + urlinfo.baseName() + "-backup");
			QStringList backup_files;
			if(urlinfo.suffix().isEmpty()) backup_files = backupdir.entryList(QStringList(urlinfo.baseName() + QString("_\?\?\?\?-\?\?-\?\?")), QDir::Files, QDir::Time);
			else backup_files = backupdir.entryList(QStringList(urlinfo.baseName() + QString("_\?\?\?\?-\?\?-\?\?.") + urlinfo.suffix()), QDir::Files, QDir::Time);
			bool save_backup = backup_files.isEmpty();
			if(!save_backup) {
				QString last_backup = backup_files.first();
				int suffix_length = 0;
				if(!urlinfo.suffix().isEmpty()) suffix_length += urlinfo.suffix().length() + 1;
				QDate last_backup_date = QDate::fromString(last_backup.mid(last_backup.length() - 10 - suffix_length, 10), "yyyy-MM-dd");
				switch(i_backup_frequency) {
					case BACKUP_DAILY: {last_backup_date = last_backup_date.addDays(1); break;}
					case BACKUP_WEEKLY: {last_backup_date = last_backup_date.addDays(7); break;}
					case BACKUP_FORTNIGHTLY: {last_backup_date = last_backup_date.addDays(14); break;}
					default: {last_backup_date = last_backup_date.addMonths(1); break;}
				}
				save_backup = (last_backup_date <= QDate::currentDate());
			}
			if(save_backup) {
				if(urlinfo.suffix().isEmpty()) QFile::copy(url.toLocalFile(), backupdir.absolutePath() + QString("/") + urlinfo.baseName() + QString("_") + QDate::currentDate().toString("yyyy-MM-dd"));
				else QFile::copy(url.toLocalFile(), backupdir.absolutePath() + QString("/") + urlinfo.baseName() + QString("_") + QDate::currentDate().toString("yyyy-MM-dd") + QString(".") + urlinfo.suffix());
			}
		}
	}
	
	if(do_cloud_sync && budget->autosyncEnabled()) {
		sync(false);
	}

	QString error = budget->saveFile(url.toLocalFile(), QFile::ReadUser | QFile::WriteUser);
	if(!error.isNull()) {
		QMessageBox::critical(parent, tr("Couldn't save file"), tr("Error saving %1: %2.").arg(url.toString()).arg(error));
		return false;
	}
	setWindowTitle(url.fileName() + "[*]");
	current_url = url;
	ActionFileReload->setEnabled(true);
	QSettings settings;
	settings.beginGroup("GeneralOptions");
	settings.setValue("lastURL", current_url.url());
	settings.endGroup();
	if(!cr_tmp_file.isEmpty()) {
		QFile autosaveFile(cr_tmp_file);
		autosaveFile.remove();
		cr_tmp_file = "";
	}
	settings.sync();
	updateRecentFiles(url.toLocalFile());
	setModified(false);

	return true;
}

void Eqonomize::importCSV() {
	ImportCSVDialog *dialog = new ImportCSVDialog(b_extra, budget, this);
	if(dialog->exec() == QDialog::Accepted) {
		reloadBudget();
		emit accountsModified();
		emit transactionsModified();
		setModified(true);
	}
	dialog->deleteLater();
}

void Eqonomize::importQIF() {
	if(importQIFFile(budget, this, b_extra)) {
		reloadBudget();
		emit accountsModified();
		emit transactionsModified();
		setModified(true);
	}
}

void Eqonomize::importEQZ() {
	QMimeDatabase db;
	QMimeType mime = db.mimeTypeForName("application/x-eqonomize");
	QString filter_string;
	if(mime.isValid()) filter_string = mime.filterString();
	if(filter_string.isEmpty()) filter_string = tr("Eqonomize! Accounting File") + "(*.eqz)";
	QString url = QFileDialog::getOpenFileName(this, QString(), current_url.isValid() ? current_url.adjusted(QUrl::RemoveFilename).toLocalFile() : QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QString("/"), filter_string);
	if(!url.isEmpty()) {
		if(openURL(QUrl::fromLocalFile(url), true)) setModified(true);
	}
}

void Eqonomize::exportQIF() {
	exportQIFFile(budget, this, b_extra);
}

void Eqonomize::checkAvailableVersion_readdata() {
	QString ssystem;
	#if defined (Q_OS_WIN32)
		ssystem = "Windows";
	#elif defined(Q_OS_ANDROID)
		return;
	#elif defined(Q_OS_LINUX)
		ssystem = "Linux";
	#else
		return;
	#endif
	if(checkVersionReply->error() != QNetworkReply::NoError) {
		checkVersionReply->abort();
		return;
	}
	QByteArray sbuffer = checkVersionReply->readAll();
	int i = sbuffer.indexOf(ssystem);
	if(i < 0) return;
	i = sbuffer.indexOf(':', i);
	if(i < 0) return;
	int i2 = sbuffer.indexOf('\n', i);
	if(i2 > 0) sbuffer = sbuffer.left(i2);
	sbuffer = sbuffer.right(sbuffer.length() - (i + 1)).trimmed();
	QSettings settings;
	settings.beginGroup("GeneralOptions");
	QByteArray old_version = settings.value("lastVersionFound").toByteArray();
	if(old_version.isEmpty()) old_version = VERSION;
	bool b = false;
	while(sbuffer != old_version) {
		QList<QByteArray> version_parts_new = sbuffer.split('.');
		if(version_parts_new.isEmpty()) break;
		QList<QByteArray> version_parts_old = old_version.split('.');
		if(version_parts_old.isEmpty()) break;
		if(version_parts_new.last().size() > 0) {
			char c = version_parts_new.last().at(version_parts_new.last().size() - 1);
			if(c < '0' || c > '9') {
				version_parts_new.last().chop(1);
				version_parts_new.append(QByteArray(1, c));
			}
		}
		if(version_parts_old.last().size() > 0) {
			char c = version_parts_old.last().at(version_parts_old.last().size() - 1);
			if(c < '0' || c > '9') {
				version_parts_old.last().chop(1);
				version_parts_old.append(QByteArray(1, c));
			}
		}
		for(i = 0; i < version_parts_new.size(); i++) {
			if(i == version_parts_old.size() || version_parts_new[i].toInt() > version_parts_old[i].toInt()) {b = true; break;}
			else if(version_parts_new[i].toInt() < version_parts_old[i].toInt()) break;
		}
		if(b && old_version != VERSION) {
			b = false;
			settings.setValue("lastVersionFound", sbuffer);
			old_version = VERSION;
		} else {
			break;
		}
	}
	if(b) {
		QMessageBox::information(this, tr("New version available"), tr("A new version of %1 is available.<br><br>You can get version %2 at %3.").arg("Eqonomize!").arg(QString(sbuffer)).arg("<a href=\"http://eqonomize.github.io/downloads.html\">eqonomize.github.io</a>"));
		settings.setValue("lastVersionFound", sbuffer);
	}
}

void Eqonomize::checkAvailableVersion() {
#if defined (Q_OS_WIN32)
#elif defined(Q_OS_ANDROID)
	return;
#elif defined(Q_OS_LINUX)
#else
	return;
#endif
	QSettings settings;
	settings.beginGroup("GeneralOptions");
	settings.setValue("lastVersionCheck", QDate::currentDate().toString(Qt::ISODate));
	checkVersionReply = budget->nam.get(QNetworkRequest(QUrl("https://eqonomize.github.io/CURRENT_VERSIONS")));
	connect(checkVersionReply, SIGNAL(finished()), this, SLOT(checkAvailableVersion_readdata()));
}

void Eqonomize::checkExchangeRatesTimeOut() {
	Currency *cur = budget->defaultCurrency();
	if(cur != budget->currency_euro && cur->exchangeRateSource() == EXCHANGE_RATE_SOURCE_NONE) cur = NULL;
	for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
		AssetsAccount *acc = *it;
		if(acc->currency() != NULL && acc->currency() != cur && (acc->currency() == budget->currency_euro || acc->currency()->exchangeRateSource() != EXCHANGE_RATE_SOURCE_NONE)) {
			if(cur) {
				if(timeToUpdateExchangeRates()) updateExchangeRates(true);
				break;
			}
			cur = acc->currency();
		}
	}
}
bool Eqonomize::timeToUpdateExchangeRates() {
	QSettings settings;
	settings.beginGroup("GeneralOptions");
	QDate last_update = QDate::fromString(settings.value("lastExchangeRatesUpdate").toString(), Qt::ISODate);
	QDate last_update_try = QDate::fromString(settings.value("lastExchangeRatesUpdateTry").toString(), Qt::ISODate);
	settings.endGroup();
	QDate curdate = QDate::currentDate();
	if(!last_update_try.isValid() || curdate > last_update_try) {
		if(!last_update.isValid() || curdate >= last_update.addDays(7)) {
			return true;
		}
	}
	return false;
}
void Eqonomize::updateExchangeRates(bool do_currencies_modified) {
	QProgressDialog *updateExchangeRatesProgressDialog = new QProgressDialog(tr("Updating exchange rates…"), tr("Abort"), 0, 1, this);
	updateExchangeRatesProgressDialog->setWindowModality(Qt::WindowModal);
	updateExchangeRatesProgressDialog->setMinimumDuration(200);
	updateExchangeRatesProgressDialog->setMaximum(2);
	connect(updateExchangeRatesProgressDialog, SIGNAL(canceled()), this, SLOT(cancelUpdateExchangeRates()));
	updateExchangeRatesProgressDialog->setValue(0);
	QEventLoop loop;
	updateExchangeRatesReply = budget->nam.get(QNetworkRequest(QUrl("https://www.ecb.europa.eu/stats/eurofxref/eurofxref-daily.xml")));
	connect(updateExchangeRatesReply, SIGNAL(finished()), &loop, SLOT(quit()));
	loop.exec();
	QSettings settings;
	settings.beginGroup("GeneralOptions");
	if(updateExchangeRatesReply->error() == QNetworkReply::OperationCanceledError) {
		//canceled by user
		updateExchangeRatesProgressDialog->reset();
	} else if(updateExchangeRatesReply->error() != QNetworkReply::NoError) {
		updateExchangeRatesProgressDialog->reset();
		QMessageBox::critical(this, tr("Error"), tr("Failed to download exchange rates from %1: %2.").arg("https://www.ecb.europa.eu/stats/eurofxref/eurofxref-daily.xml").arg(updateExchangeRatesReply->errorString()));
		updateExchangeRatesReply->abort();
	} else {
		QString errors = budget->loadECBData(updateExchangeRatesReply->readAll());
		if(!errors.isEmpty()) {
			QMessageBox::critical(this, tr("Error"), tr("Error reading data from %1: %2.").arg("https://www.ecb.europa.eu/stats/eurofxref/eurofxref-daily.xml").arg(errors));
		} else {
			settings.setValue("lastExchangeRatesUpdate", QDate::currentDate().toString(Qt::ISODate));
			updateExchangeRatesProgressDialog->setValue(1);
			QEventLoop loop2;
			//updateExchangeRatesReply = budget->nam.get(QNetworkRequest(QUrl("http://www.mycurrency.net/service/rates")));
			updateExchangeRatesReply = budget->nam.get(QNetworkRequest(QUrl("https://www.mycurrency.net/=EU")));
			connect(updateExchangeRatesReply, SIGNAL(finished()), &loop2, SLOT(quit()));
			loop2.exec();
			if(updateExchangeRatesReply->error() == QNetworkReply::OperationCanceledError) {
				//canceled by user
				updateExchangeRatesProgressDialog->reset();
			} else if(updateExchangeRatesReply->error() != QNetworkReply::NoError) {
				updateExchangeRatesProgressDialog->reset();
				//QMessageBox::critical(this, tr("Error"), tr("Failed to download exchange rates from %1: %2.").arg("http://www.mycurrency.net/service/rates").arg(updateExchangeRatesReply->errorString()));
				QMessageBox::critical(this, tr("Error"), tr("Failed to download exchange rates from %1: %2.").arg("https://www.mycurrency.net").arg(updateExchangeRatesReply->errorString()));
				updateExchangeRatesReply->abort();
			} else {
			
				//QString errors = budget->loadMyCurrencyNetData(updateExchangeRatesReply->readAll());
				QString errors = budget->loadMyCurrencyNetHtml(updateExchangeRatesReply->readAll());
				if(!errors.isEmpty()) {
					QMessageBox::critical(this, tr("Error"), tr("Error reading data from %1: %2.").arg("http://www.mycurrency.net/service/rates").arg(errors));
				}
			}
			QString error = budget->saveCurrencies();
			if(!error.isNull()) QMessageBox::critical(this, tr("Error"), tr("Error saving currencies: %1.").arg(error));
			if(do_currencies_modified) {
				budget->resetDefaultCurrencyChanged();
				currenciesModified();
			} else {
				if(currencyConversionWindow && currencyConversionWindow->isVisible()) {
					currencyConversionWindow->convertFrom();
				}
			}
			
			updateExchangeRatesProgressDialog->setValue(2);
			
		}
		
	}
	settings.setValue("lastExchangeRatesUpdateTry", QDate::currentDate().toString(Qt::ISODate));
	settings.endGroup();
	updateExchangeRatesProgressDialog->deleteLater();
	updateExchangeRatesReply->deleteLater();
}
void Eqonomize::cancelUpdateExchangeRates() {
	updateExchangeRatesReply->abort();
}
void Eqonomize::currenciesModified() {
	expensesWidget->updateFromAccounts();
	incomesWidget->updateToAccounts();
	transfersWidget->updateAccounts();
	expensesWidget->filterTransactions();
	incomesWidget->filterTransactions();
	transfersWidget->filterTransactions();
	filterAccounts();
	updateScheduledTransactions();
	updateSecurities();
	if(budget->defaultCurrencyChanged()) {
		setModified(true);
		budget->resetDefaultCurrencyChanged();
	}
	emit transactionsModified();
	updateUsesMultipleCurrencies();
	if(currencyConversionWindow) {
		currencyConversionWindow->updateCurrencies();
		if(currencyConversionWindow->isVisible()) currencyConversionWindow->convertFrom();
	}
}

void Eqonomize::warnAndAskForExchangeRate() {
	QDialog *dialog = new QDialog(this, 0);
	Currency *cur = budget->defaultCurrency();
	dialog->setWindowTitle(tr("Unrecognized Currency"));
	dialog->setModal(true);
	QVBoxLayout *box1 = new QVBoxLayout(dialog);
	QGridLayout *grid = new QGridLayout();
	box1->addLayout(grid);
	QLabel *label = new QLabel(tr("No exchange rate is available for the default currency (%1). If you wish to use multiple currencies you should set the exchange rate manually.").arg(cur->code()), dialog);
	label->setWordWrap(true);
	grid->addWidget(label, 0, 0, 1, 2);
	label = new QLabel(budget->currency_euro->formatValue(1.0) + " =", dialog);
	label->setAlignment(Qt::AlignRight);
	grid->addWidget(label, 1, 0);
	EqonomizeValueEdit *rateEdit = new EqonomizeValueEdit(0.00001, 1.0, 1.0, 5, true, this, budget);
	grid->addWidget(rateEdit, 1, 1);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(accept()));
	box1->addWidget(buttonBox);
	dialog->exec();
	if(rateEdit->value() != 1.0) {
		cur->setExchangeRate(rateEdit->value());
		budget->currencyModified(cur);
	}
	QString error = budget->saveCurrencies();
	if(!error.isNull()) QMessageBox::critical(this, tr("Error"), tr("Error saving currencies: %1.").arg(error));
	dialog->deleteLater();
}

void Eqonomize::setMainCurrency() {
	QDialog *dialog = new QDialog(this, 0);
	dialog->setWindowTitle(tr("Set Main Currency"));
	dialog->setModal(true);
	QVBoxLayout *box1 = new QVBoxLayout(dialog);
	QGridLayout *grid = new QGridLayout();
	box1->addLayout(grid);
	grid->addWidget(new QLabel(tr("Currency:"), dialog), 0, 0);
	setMainCurrencyCombo = new QComboBox(this);
	setMainCurrencyCombo->setEditable(false);
	grid->addWidget(setMainCurrencyCombo, 0, 1);
	setMainCurrencyCombo->addItem(tr("New currency…"));
	int i = 1;
	for(CurrencyList<Currency*>::const_iterator it = budget->currencies.constBegin(); it != budget->currencies.constEnd(); ++it) {
		Currency *currency = *it;
		if(!currency->name(false).isEmpty()) {
			setMainCurrencyCombo->addItem(QIcon(":/data/flags/" + currency->code() + ".png"), QString("%2 (%1)").arg(qApp->translate("currencies.xml", qPrintable(currency->name()))).arg(currency->code()));
		} else {
			setMainCurrencyCombo->addItem(currency->code());
		}
		setMainCurrencyCombo->setItemData(i, qVariantFromValue((void*) currency));
		if(currency == budget->defaultCurrency()) {
			setMainCurrencyCombo->setCurrentIndex(i);
			prev_set_main_currency_index = i;
		}
		i++;
	}
	QCheckBox *replaceButton = new QCheckBox(tr("Replace all occurrences of the former main currency"), dialog);
	replaceButton->setChecked(false);
	grid->addWidget(replaceButton, 1, 0, 1, 2);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), dialog, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(accept()));
	box1->addWidget(buttonBox);
	connect(setMainCurrencyCombo, SIGNAL(activated(int)), this, SLOT(setMainCurrencyIndexChanged(int)));
	if(dialog->exec() == QDialog::Accepted) {
		Currency *cur = NULL;
		int index = setMainCurrencyCombo->currentIndex();
		if(index > 0) cur = (Currency*) setMainCurrencyCombo->itemData(index).value<void*>();
		if(cur && cur != budget->defaultCurrency()) {
			if(replaceButton->isChecked()) {
				for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
					AssetsAccount *acc = *it;
					if(acc->currency() == budget->defaultCurrency()) {
						acc->setCurrency(cur);
					}
				}
			}
			budget->setDefaultCurrency(cur);
			budgetEdit->setCurrency(cur);
			currenciesModified();
			setModified(true);
		}
	}
	dialog->deleteLater();
}
void Eqonomize::setMainCurrencyIndexChanged(int index) {
	if(index == 0) {
		EditCurrencyDialog *dialog = new EditCurrencyDialog(budget, NULL, false, this);
		if(dialog->exec() == QDialog::Accepted) {
			Currency *cur = dialog->createCurrency();
			if(currencyConversionWindow) currencyConversionWindow->updateCurrencies();
			setMainCurrencyCombo->clear();
			setMainCurrencyCombo->addItem(tr("New currency…"));
			int i = 1;
			for(CurrencyList<Currency*>::const_iterator it = budget->currencies.constBegin(); it != budget->currencies.constEnd(); ++it) {
				Currency *currency = *it;
				setMainCurrencyCombo->addItem(QIcon(":/data/flags/" + currency->code() + ".png"), currency->code());
				setMainCurrencyCombo->setItemData(i, qVariantFromValue((void*) currency));
				if(currency == cur) {
					setMainCurrencyCombo->setCurrentIndex(i);
					prev_set_main_currency_index = i;
				}
				i++;
			}
		} else {
			setMainCurrencyCombo->setCurrentIndex(prev_set_main_currency_index);
		}
		dialog->deleteLater();
	} else {
		prev_set_main_currency_index = index;
	}
}
void Eqonomize::updateUsesMultipleCurrencies() {
	bool b = budget->usesMultipleCurrencies();
	transfersWidget->useMultipleCurrencies(b);
	incomesWidget->useMultipleCurrencies(b);
	expensesWidget->useMultipleCurrencies(b);
}
void Eqonomize::openCurrencyConversion() {
	if(timeToUpdateExchangeRates()) {
		updateExchangeRates(true);
	}
	if(!currencyConversionWindow) {
		currencyConversionWindow = new CurrencyConversionDialog(budget, this);
	}
	bool b = currencyConversionWindow->isVisible();
	currencyConversionWindow->show();
	if(!b) currencyConversionWindow->convertFrom();
	currencyConversionWindow->raise();
	currencyConversionWindow->activateWindow();
}

void Eqonomize::showOverTimeReport() {
	if(!otrDialog) {
		otrDialog = new OverTimeReportDialog(budget, NULL);
		QSettings settings;
		QSize dialog_size = settings.value("OverTimeReport/size", QSize()).toSize();
		if(!dialog_size.isValid()) {
			QDesktopWidget desktop;
			dialog_size = QSize(750, 650).boundedTo(desktop.availableGeometry(this).size());
		}
		otrDialog->resize(dialog_size);
		connect(this, SIGNAL(accountsModified()), ((OverTimeReportDialog*) otrDialog)->report, SLOT(updateAccounts()));
		connect(this, SIGNAL(transactionsModified()), ((OverTimeReportDialog*) otrDialog)->report, SLOT(updateTransactions()));
		connect(this, SIGNAL(timeToSaveConfig()), ((OverTimeReportDialog*) otrDialog)->report, SLOT(saveConfig()));
	} else if(!otrDialog->isVisible()) {
		((OverTimeReportDialog*) otrDialog)->report->resetOptions();
	}
	bool b = otrDialog->isVisible();
	otrDialog->show();
	if(!b) ((OverTimeReportDialog*) otrDialog)->report->updateDisplay();
	otrDialog->raise();
	otrDialog->activateWindow();
}
void Eqonomize::showCategoriesComparisonReport() {
	if(!ccrDialog) {
		ccrDialog = new CategoriesComparisonReportDialog(b_extra, budget, NULL);
		QSettings settings;
		QSize dialog_size = settings.value("CategoriesComparisonReport/size", QSize()).toSize();
		if(!dialog_size.isValid()) {
			QDesktopWidget desktop;
			dialog_size = QSize(750, 670).boundedTo(desktop.availableGeometry(this).size());
		}
		ccrDialog->resize(dialog_size);
		connect(this, SIGNAL(accountsModified()), ((CategoriesComparisonReportDialog*) ccrDialog)->report, SLOT(updateAccounts()));
		connect(this, SIGNAL(transactionsModified()), ((CategoriesComparisonReportDialog*) ccrDialog)->report, SLOT(updateTransactions()));
		connect(this, SIGNAL(timeToSaveConfig()), ((CategoriesComparisonReportDialog*) ccrDialog)->report, SLOT(saveConfig()));
	} else if(!ccrDialog->isVisible()) {
		((CategoriesComparisonReportDialog*) ccrDialog)->report->resetOptions();
	}
	bool b = ccrDialog->isVisible();
	ccrDialog->show();
	if(!b) ((CategoriesComparisonReportDialog*) ccrDialog)->report->updateDisplay();
	ccrDialog->raise();
	ccrDialog->activateWindow();
}
void Eqonomize::showOverTimeChart() {
	if(!otcDialog) {
		otcDialog = new OverTimeChartDialog(b_extra, budget, NULL);
		QSettings settings;
		QSize dialog_size = settings.value("OverTimeChart/size", QSize()).toSize();
		if(!dialog_size.isValid()) {
			QDesktopWidget desktop;
			dialog_size = QSize(850, b_extra ? 750 : 730).boundedTo(desktop.availableGeometry(this).size());
		}
		otcDialog->resize(dialog_size);		
		connect(this, SIGNAL(accountsModified()), ((OverTimeChartDialog*) otcDialog)->chart, SLOT(updateAccounts()));
		connect(this, SIGNAL(transactionsModified()), ((OverTimeChartDialog*) otcDialog)->chart, SLOT(updateTransactions()));
		connect(this, SIGNAL(timeToSaveConfig()), ((OverTimeChartDialog*) otcDialog)->chart, SLOT(saveConfig()));
	} else if(!otcDialog->isVisible()) {
		((OverTimeChartDialog*) otcDialog)->chart->resetOptions();
	}
	bool b = otcDialog->isVisible();
	otcDialog->show();
	if(!b) ((OverTimeChartDialog*) otcDialog)->chart->updateDisplay();
	otcDialog->raise();
	otcDialog->activateWindow();
}

void Eqonomize::showCategoriesComparisonChart() {
	if(!cccDialog) {
		cccDialog = new CategoriesComparisonChartDialog(budget, NULL);
		QSettings settings;
		QSize dialog_size = settings.value("CategoriesComparisonChart/size", QSize()).toSize();
		if(!dialog_size.isValid()) {
			QDesktopWidget desktop;
			dialog_size = QSize(750, 700).boundedTo(desktop.availableGeometry(this).size());
		}
		cccDialog->resize(dialog_size);
		connect(this, SIGNAL(accountsModified()), ((CategoriesComparisonChartDialog*) cccDialog)->chart, SLOT(updateAccounts()));
		connect(this, SIGNAL(transactionsModified()), ((CategoriesComparisonChartDialog*) cccDialog)->chart, SLOT(updateTransactions()));
		connect(this, SIGNAL(timeToSaveConfig()), ((CategoriesComparisonChartDialog*) cccDialog)->chart, SLOT(saveConfig()));
	} else if(!cccDialog->isVisible()) {
		((CategoriesComparisonChartDialog*) cccDialog)->chart->resetOptions();
	}
	bool b = cccDialog->isVisible();
	cccDialog->show();
	if(!b) ((CategoriesComparisonChartDialog*) cccDialog)->chart->updateDisplay();
	cccDialog->raise();
	cccDialog->activateWindow();
}

QString htmlize_string(QString str) {
	str.replace('<', "&lt;");
	str.replace('>', "&gt;");
	str.replace('&', "&amp;");
	str.replace('\"', "&quot;");
	return str;
}

bool Eqonomize::exportScheduleList(QTextStream &outf, int fileformat) {

	switch(fileformat) {
		case 'h': {
			outf.setCodec("UTF-8");
			outf << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">" << '\n';
			outf << "<html>" << '\n';
			outf << "\t<head>" << '\n';
			outf << "\t\t<title>"; outf << htmlize_string(tr("Transaction Schedule")); outf << "</title>" << '\n';
			outf << "\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" << '\n';
			outf << "\t\t<meta name=\"GENERATOR\" content=\"" << qApp->applicationDisplayName() << "\">" << '\n';
			outf << "\t</head>" << '\n';
			outf << "\t<body>" << '\n';
			outf << "\t\t<table border=\"1\" cellspacing=\"0\" cellpadding=\"5\">" << '\n';
			outf << "\t\t\t<caption>"; outf << htmlize_string(tr("Transaction Schedule")); outf << "</caption>" << '\n';
			outf << "\t\t\t<thead>" << '\n';
			outf << "\t\t\t\t<tr>" << '\n';
			QTreeWidgetItem *header = scheduleView->headerItem();
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
			QTreeWidgetItemIterator it(scheduleView);
			ScheduleListViewItem *i = (ScheduleListViewItem*) *it;
			while(i) {
				outf << "\t\t\t\t<tr>" << '\n';
				outf << "\t\t\t\t\t<td nowrap align=\"right\">" << htmlize_string(QLocale().toString(i->date(), QLocale::ShortFormat)) << "</td>";
				outf << "<td>" << htmlize_string(i->text(1)) << "</td>";
				outf << "<td>" << htmlize_string(i->text(2)) << "</td>";
				outf << "<td nowrap align=\"right\">" << htmlize_string(i->text(3)) << "</td>";
				outf << "<td align=\"center\">" << htmlize_string(i->text(4)) << "</td>";
				outf << "<td align=\"center\">" << htmlize_string(i->text(5)) << "</td>";
				outf << "<td>" << htmlize_string(i->text(6)) << "</td>" << "\n";
				outf << "\t\t\t\t</tr>" << '\n';
				++it;
				i = (ScheduleListViewItem*) *it;
			}
			outf << "\t\t\t</tbody>" << '\n';
			outf << "\t\t</table>" << '\n';
			outf << "\t</body>" << '\n';
			outf << "</html>" << '\n';
			break;
		}
		case 'c': {
			//outf.setEncoding(Q3TextStream::Locale);
			QTreeWidgetItem *header = scheduleView->headerItem();
			outf << "\"" << header->text(0) << "\",\"" << header->text(1) << "\",\"" << header->text(2) << "\",\"" << header->text(3) << "\",\"" << header->text(4) << "\",\""<< header->text(5) << "\",\"" << header->text(6) << "\"\n";
			QTreeWidgetItemIterator it(scheduleView);
			ScheduleListViewItem *i = (ScheduleListViewItem*) *it;
			while(i) {
				outf << "\"" << QLocale().toString(i->date(), QLocale::ShortFormat) << "\",\"" << i->text(1) << "\",\"" << i->text(2) << "\",\"" << i->text(3) << "\",\"" << i->text(4) << "\",\"" << i->text(5) << "\",\"" << i->text(6) << "\"\n";
				++it;
				i = (ScheduleListViewItem*) *it;
			}
			break;
		}
	}

	return true;

}

bool Eqonomize::exportSecuritiesList(QTextStream &outf, int fileformat) {

	switch(fileformat) {
		case 'h': {
			outf.setCodec("UTF-8");
			outf << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">" << '\n';
			outf << "<html>" << '\n';
			outf << "\t<head>" << '\n';
			outf << "\t\t<title>"; outf << htmlize_string(tr("Securities", "Financial security (e.g. stock, mutual fund)")); outf << "</title>" << '\n';
			outf << "\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" << '\n';
			outf << "\t\t<meta name=\"GENERATOR\" content=\"" << qApp->applicationDisplayName() << "\">" << '\n';
			outf << "\t</head>" << '\n';
			outf << "\t<body>" << '\n';
			outf << "\t\t<table border=\"1\" cellspacing=\"0\" cellpadding=\"5\">" << '\n';
			outf << "\t\t\t<caption>"; outf << htmlize_string(tr("Securities", "Financial security (e.g. stock, mutual fund)")); outf << "</caption>" << '\n';
			outf << "\t\t\t<thead>" << '\n';
			outf << "\t\t\t\t<tr>" << '\n';
			QTreeWidgetItem *header = securitiesView->headerItem();
			outf << "\t\t\t\t\t<th>" << htmlize_string(header->text(0)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(1)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(2)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(3)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(4)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(5)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(6)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(7)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(8)) << "</th>" << "\n";
			outf << "\t\t\t\t</tr>" << '\n';
			outf << "\t\t\t</thead>" << '\n';
			outf << "\t\t\t<tbody>" << '\n';
			QTreeWidgetItemIterator it(securitiesView);
			SecurityListViewItem *i = (SecurityListViewItem*) *it;
			while(i) {
				outf << "\t\t\t\t<tr>" << '\n';
				outf << "\t\t\t\t\t<td>" << htmlize_string(i->text(0)) << "</td>";
				outf << "<td nowrap align=\"right\">" << htmlize_string(i->text(1)) << "</td>";
				outf << "<td nowrap align=\"right\">" << htmlize_string(i->text(2)) << "</td>";
				outf << "<td nowrap align=\"right\">" << htmlize_string(i->text(3)) << "</td>";
				outf << "<td nowrap align=\"right\">" << htmlize_string(i->text(4)) << "</td>";
				outf << "<td nowrap align=\"right\">" << htmlize_string(i->text(5)) << "</td>";
				outf << "<td nowrap align=\"right\">" << htmlize_string(i->text(6)) << "</td>";
				outf << "<td align=\"center\">" << htmlize_string(i->text(7)) << "</td>";
				outf << "<td align=\"center\">" << htmlize_string(i->text(8)) << "</td>" << "\n";
				outf << "\t\t\t\t</tr>" << '\n';
				++it;
				i = (SecurityListViewItem*) *it;
			}
			outf << "\t\t\t\t<tr>" << '\n';
			if(securitiesView->topLevelItemCount() > 1) {
				outf << "\t\t\t\t\t<td style=\"border-top: thin solid\"><b>" << htmlize_string(tr("Total")) << "</b></td>";
				outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(budget->formatMoney(total_value)) << "</b></td>";
				outf << "<td align=\"right\" style=\"border-top: thin solid\"><b>-</b></td>";
				outf << "<td align=\"right\" style=\"border-top: thin solid\"><b>-</b></td>";
				outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(budget->formatMoney(total_cost)) << "</b></td>";
				outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(budget->formatMoney(total_profit)) << "</b></td>";
				outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toString(total_rate * 100) + "%") << "</b></td>";
				outf << "<td align=\"center\" style=\"border-top: thin solid\"><b>-</b></td>";
				outf << "<td align=\"center\" style=\"border-top: thin solid\"><b>-</b></td>" << "\n";
				outf << "\t\t\t\t</tr>" << '\n';
			}
			outf << "\t\t\t</tbody>" << '\n';
			outf << "\t\t</table>" << '\n';
			outf << "\t</body>" << '\n';
			outf << "</html>" << '\n';
			break;
		}
		case 'c': {
			//outf.setEncoding(Q3TextStream::Locale);
			QTreeWidgetItem *header = scheduleView->headerItem();
			outf << "\"" << header->text(0) << "\",\"" << header->text(1) << "\",\"" << header->text(2) << "\",\"" << header->text(3) << "\",\"" << header->text(4) << "\",\"" << header->text(5) << "\",\"" << header->text(6) << "\",\"" << header->text(7) << "\",\"" << header->text(8) << "\"\n";
			QTreeWidgetItemIterator it(securitiesView);
			SecurityListViewItem *i = (SecurityListViewItem*) *it;
			while(i) {
				outf << "\"" << i->text(0) << "\",\"" << i->text(1) << "\",\"" << i->text(2) << "\",\"" << i->text(3) << "\",\"" << i->text(4) << "\",\"" << i->text(5) << "\",\"" << i->text(6) << "\",\"" << i->text(7) << "\",\"" << i->text(8) << '\n';
				++it;
				i = (SecurityListViewItem*) *it;
			}
			break;
		}
	}

	return true;

}

bool Eqonomize::exportAccountsList(QTextStream &outf, int fileformat) {

	switch(fileformat) {
		case 'h': {
			outf.setCodec("UTF-8");
			outf << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">" << '\n';
			outf << "<html>" << '\n';
			outf << "\t<head>" << '\n';
			outf << "\t\t<title>"; outf << tr("Accounts &amp; Categories", "html format"); outf << "</title>" << '\n';
			outf << "\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" << '\n';
			outf << "\t\t<meta name=\"GENERATOR\" content=\"" << qApp->applicationDisplayName() << "\">" << '\n';
			outf << "\t</head>" << '\n';
			outf << "\t<body>" << '\n';
			if(accountsPeriodFromButton->isChecked()) outf << "\t\t<h1>" << tr("Accounts &amp; Categories (%1&ndash;%2)", "html format").arg(htmlize_string(QLocale().toString(from_date, QLocale::ShortFormat))).arg(htmlize_string(QLocale().toString(to_date, QLocale::ShortFormat))) << "</h1>" << '\n';
			else outf << "\t\t<h1>" << tr("Accounts &amp; Categories (to %1)", "html format").arg(htmlize_string(QLocale().toString(to_date, QLocale::ShortFormat))) << "</h1>" << '\n';
			outf << "\t\t<table border=\"0\" cellspacing=\"0\" cellpadding=\"5\">" << '\n';
			outf << "\t\t\t<caption>"; outf << htmlize_string(tr("Assets")); outf << "</caption>" << '\n';
			outf << "\t\t\t<thead>" << '\n';
			outf << "\t\t\t\t<tr>" << '\n';
			outf << "\t\t\t\t\t<th style=\"border-bottom: thin solid\">" << htmlize_string(tr("Name")) << "</th>";
			outf << "<th style=\"border-bottom: thin solid\">" << htmlize_string(tr("Type")) << "</th>";
			//: Noun, how much the account balance has changed
			outf << "<th style=\"border-bottom: thin solid\">" << htmlize_string(tr("Change"));
			bool includes_budget = (to_date > QDate::currentDate() && (expenses_budget >= 0.0 || incomes_budget >= 0.0));
			outf << "</th>";
			outf << "<th style=\"border-bottom: thin solid\">" << htmlize_string(tr("Balance", "Noun. Balance of an account"));
			outf << "</th>" << '\n';
			outf << "\t\t\t\t</tr>" << '\n';
			outf << "\t\t\t</thead>" << '\n';			
			outf << "\t\t\t<tbody>" << '\n';
			for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
				Account *account = *it;
				if(!IS_DEBT(((AssetsAccount*) account)) && account != budget->balancingAccount) {
					outf << "\t\t\t\t<tr>" << '\n';
					outf << "\t\t\t\t\t<td>" << htmlize_string(account->name());
					if(includes_budget && ((AssetsAccount*) account)->isBudgetAccount()) outf << "*";
					outf << "</td>";
					outf << "<td>";
					switch(((AssetsAccount*) account)->accountType()) {
						case ASSETS_TYPE_CASH: {outf << htmlize_string(tr("Cash")); break;}
						case ASSETS_TYPE_CURRENT: {outf << htmlize_string(tr("Current Account")); break;}
						case ASSETS_TYPE_SAVINGS: {outf << htmlize_string(tr("Savings Account")); break;}
						case ASSETS_TYPE_CREDIT_CARD: {outf << htmlize_string(tr("Credit Card")); break;}
						case ASSETS_TYPE_LIABILITIES: {outf << htmlize_string(tr("Liabilities")); break;}
						case ASSETS_TYPE_SECURITIES: {outf << htmlize_string(tr("Securities", "Financial security (e.g. stock, mutual fund)"));  break;}
						default: {outf << htmlize_string(tr("Other")); break;}
					}
					outf << "</td>";
					outf << "<td nowrap align=\"right\">" << htmlize_string(((AssetsAccount*) account)->currency()->formatValue(account_change[account])) << "</td>";
					outf << "<td nowrap align=\"right\">" << htmlize_string(((AssetsAccount*) account)->currency()->formatValue(account_value[account])) << "</td>" << "\n";
					outf << "\t\t\t\t</tr>" << '\n';
				}
			}
			outf << "\t\t\t\t<tr>" << '\n';
			outf << "\t\t\t\t\t<td style=\"border-top: thin solid\"><b>" << htmlize_string(tr("Total"));
			if(includes_budget) outf << "*";
			outf << "</b></td>";
			outf << "<td style=\"border-top: thin solid\">";
			outf << "</td>";
			outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(budget->formatMoney(assets_accounts_change)) << "</b></td>";
			outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(budget->formatMoney(assets_accounts_value)) << "</b></td>" << "\n";
			outf << "\t\t\t\t</tr>" << '\n';
			outf << "\t\t\t</tbody>" << '\n';
			outf << "\t\t</table>" << '\n';
			outf << "\t\t<br>" << '\n';
			outf << "\t\t<br>" << '\n';
			outf << "\t\t<table border=\"0\" cellspacing=\"0\" cellpadding=\"5\">" << '\n';
			outf << "\t\t\t<caption>"; outf << htmlize_string(tr("Liabilities")); outf << "</caption>" << '\n';
			outf << "\t\t\t<thead>" << '\n';
			outf << "\t\t\t\t<tr>" << '\n';
			outf << "\t\t\t\t\t<th style=\"border-bottom: thin solid\">" << htmlize_string(tr("Name")) << "</th>";
			outf << "<th style=\"border-bottom: thin solid\">" << htmlize_string(tr("Type")) << "</th>";
			//: Noun, how much the account balance has changed
			outf << "<th style=\"border-bottom: thin solid\">" << htmlize_string(tr("Change"));
			outf << "</th>";
			outf << "<th style=\"border-bottom: thin solid\">" << htmlize_string(tr("Balance", "Noun. Balance of an account"));
			outf << "</th>" << '\n';
			outf << "\t\t\t\t</tr>" << '\n';
			outf << "\t\t\t</thead>" << '\n';
			outf << "\t\t\t<tbody>" << '\n';
			for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
				Account *account = *it;
				if(IS_DEBT(((AssetsAccount*) account))) {
					outf << "\t\t\t\t<tr>" << '\n';
					outf << "\t\t\t\t\t<td>" << htmlize_string(account->name());
					outf << "</td>";
					outf << "<td>";
					switch(((AssetsAccount*) account)->accountType()) {
						case ASSETS_TYPE_CASH: {outf << htmlize_string(tr("Cash")); break;}
						case ASSETS_TYPE_CURRENT: {outf << htmlize_string(tr("Current Account")); break;}
						case ASSETS_TYPE_SAVINGS: {outf << htmlize_string(tr("Savings Account")); break;}
						case ASSETS_TYPE_CREDIT_CARD: {outf << htmlize_string(tr("Credit Card")); break;}
						case ASSETS_TYPE_LIABILITIES: {outf << htmlize_string(tr("Liabilities")); break;}
						case ASSETS_TYPE_SECURITIES: {outf << htmlize_string(tr("Securities", "Financial security (e.g. stock, mutual fund)"));  break;}
						default: {outf << htmlize_string(tr("Other")); break;}
					}
					outf << "</td>";
					outf << "<td nowrap align=\"right\">" << htmlize_string(QLocale().toCurrencyString(-account_change[account])) << "</td>";
					outf << "<td nowrap align=\"right\">" << htmlize_string(QLocale().toCurrencyString(-account_value[account])) << "</td>" << "\n";
					outf << "\t\t\t\t</tr>" << '\n';
				}
			}
			outf << "\t\t\t\t<tr>" << '\n';
			outf << "\t\t\t\t\t<td style=\"border-top: thin solid\"><b>" << htmlize_string(tr("Total"));
			outf << "</b></td>";
			outf << "<td style=\"border-top: thin solid\">";
			outf << "</td>";
			outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(-liabilities_accounts_change)) << "</b></td>";
			outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(-liabilities_accounts_value)) << "</b></td>" << "\n";
			outf << "\t\t\t\t</tr>" << '\n';
			outf << "\t\t\t</tbody>" << '\n';
			outf << "\t\t</table>" << '\n';
			outf << "\t\t<br>" << '\n';
			outf << "\t\t<br>" << '\n';
			outf << "\t\t<table border=\"0\" cellspacing=\"0\" cellpadding=\"5\">" << '\n';
			outf << "\t\t\t<caption>"; outf << htmlize_string(tr("Incomes")); outf << "</caption>" << '\n';
			outf << "\t\t\t<thead>" << '\n';
			outf << "\t\t\t\t<tr>" << '\n';
			outf << "\t\t\t\t\t<th style=\"border-bottom: thin solid\">" << htmlize_string(tr("Category")) << "</th>";
			outf << "<th style=\"border-bottom: thin solid\">" << htmlize_string(tr("Budget")) << "</th>";
			outf << "<th style=\"border-bottom: thin solid\">" << htmlize_string(tr("Remaining Budget")) << "</th>";
			//: Noun, how much the account balance has changed
			outf << "<th style=\"border-bottom: thin solid\">" << htmlize_string(tr("Change")) << "</th>" << '\n';
			outf << "<th style=\"border-bottom: thin solid\">" << htmlize_string(tr("Total Incomes")) << "</th>" << '\n';
			outf << "\t\t\t\t</tr>" << '\n';
			outf << "\t\t\t</thead>" << '\n';
			outf << "\t\t\t<tbody>" << '\n';
			for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
				Account *account = *it;
				outf << "\t\t\t\t<tr>" << '\n';
				outf << "\t\t\t\t\t<td>" << htmlize_string(account->name()) << "</td>";
				if(account_budget[account] < 0.0) {
					outf << "<td align=\"right\">-</td>";
					outf << "<td align=\"right\">-</td>";
				} else {
					outf << "<td nowrap align=\"right\">";
					outf << htmlize_string(QLocale().toCurrencyString(account_budget[account]));
					outf << "</td>";
					outf << "<td nowrap align=\"right\">";
					outf << htmlize_string(QLocale().toCurrencyString(account_budget_diff[account]));
					outf << "</td>";
				}
				outf << "<td nowrap align=\"right\">" << htmlize_string(QLocale().toCurrencyString(account_change[account])) << "</td>";
				outf << "<td nowrap align=\"right\">" << htmlize_string(QLocale().toCurrencyString(account_value[account])) << "</td>" << "\n";
				outf << "\t\t\t\t</tr>" << '\n';
			}
			outf << "\t\t\t\t<tr>" << '\n';
			outf << "\t\t\t\t\t<td style=\"border-top: thin solid\"><b>" << htmlize_string(tr("Total")) << "</b></td>";
			if(incomes_budget >= 0.0) {
				outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(incomes_budget)) << "</b></td>";
				outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(incomes_budget_diff)) << "</b></td>";
			} else {
				outf << "<td align=\"right\" style=\"border-top: thin solid\"><b>" << "-" << "</b></td>";
				outf << "<td align=\"right\" style=\"border-top: thin solid\"><b>" << "-" << "</b></td>";
			}
			outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(incomes_accounts_change)) << "</b></td>";
			outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(incomes_accounts_value)) << "</b></td>" << "\n";
			outf << "\t\t\t\t</tr>" << '\n';
			outf << "\t\t\t</tbody>" << '\n';
			outf << "\t\t</table>" << '\n';
			outf << "\t\t<br>" << '\n';
			outf << "\t\t<br>" << '\n';
			outf << "\t\t<table border=\"0\" cellspacing=\"0\" cellpadding=\"5\">" << '\n';
			outf << "\t\t\t<caption>"; outf << htmlize_string(tr("Expenses")); outf << "</caption>" << '\n';
			outf << "\t\t\t<thead>" << '\n';
			outf << "\t\t\t\t<tr>" << '\n';
			outf << "\t\t\t\t\t<th style=\"border-bottom: thin solid\">" << htmlize_string(tr("Category")) << "</th>";
			outf << "<th style=\"border-bottom: thin solid\">" << htmlize_string(tr("Budget")) << "</th>";
			outf << "<th style=\"border-bottom: thin solid\">" << htmlize_string(tr("Remaining Budget")) << "</th>";
			//: Noun, how much the account balance has changed
			outf << "<th style=\"border-bottom: thin solid\">" << htmlize_string(tr("Change")) << "</th>" << '\n';
			outf << "<th style=\"border-bottom: thin solid\">" << htmlize_string(tr("Total Expenses")) << "</th>" << '\n';
			outf << "\t\t\t\t</tr>" << '\n';
			outf << "\t\t\t</thead>" << '\n';
			outf << "\t\t\t<tbody>" << '\n';
			for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
				Account *account = *it;
				outf << "\t\t\t\t<tr>" << '\n';
				outf << "\t\t\t\t\t<td>" << htmlize_string(account->name()) << "</td>";
				if(account_budget[account] < 0.0) {
					outf << "<td align=\"right\">-</td>";
					outf << "<td align=\"right\">-</td>";
				} else {
					outf << "<td nowrap align=\"right\">";
					outf << htmlize_string(QLocale().toCurrencyString(account_budget[account]));
					outf << "</td>";
					outf << "<td nowrap align=\"right\">";
					outf << htmlize_string(QLocale().toCurrencyString(account_budget_diff[account]));
					outf << "</td>";
				}
				outf << "<td nowrap align=\"right\">" << htmlize_string(QLocale().toCurrencyString(account_change[account])) << "</td>";
				outf << "<td nowrap align=\"right\">" << htmlize_string(QLocale().toCurrencyString(account_value[account])) << "</td>" << "\n";
				outf << "\t\t\t\t</tr>" << '\n';
			}
			outf << "\t\t\t\t<tr>" << '\n';
			outf << "\t\t\t\t\t<td style=\"border-top: thin solid\"><b>" << htmlize_string(tr("Total")) << "</b></td>";
			if(expenses_budget >= 0.0) {
				outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(expenses_budget)) << "</b></td>";
				outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(expenses_budget_diff)) << "</b></td>";
			} else {
				outf << "<td align=\"right\" style=\"border-top: thin solid\"><b>" << "-" << "</b></td>";
				outf << "<td align=\"right\" style=\"border-top: thin solid\"><b>" << "-" << "</b></td>";
			}
			outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(expenses_accounts_change)) << "</b></td>";
			outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(expenses_accounts_value)) << "</b></td>" << "\n";
			outf << "\t\t\t\t</tr>" << '\n';
			outf << "\t\t\t</tbody>" << '\n';
			outf << "\t\t</table>" << '\n';
			if(includes_budget) {
				outf << "\t\t</br>" << '\n';
				outf << "\t\t<div align=\"left\" style=\"font-weight: normal\">" << "<small>";
				outf << "*" << htmlize_string(tr("Includes budgeted transactions"));
				outf << "</small></div>" << '\n';
			}
			outf << "\t</body>" << '\n';
			outf << "</html>" << '\n';
			break;
		}
		case 'c': {
			//outf.setEncoding(Q3TextStream::Locale);
			//: Noun, how much the account balance has changed
			outf << "\"" << tr("Account/Category") << "\",\"" << tr("Change") << "\",\"" << tr("Value") << "\"\n";
			for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
				Account *account = *it;
				outf << "\"" << account->name() << "\",\"" << QLocale().toCurrencyString(account_change[account]) << "\",\"" << QLocale().toCurrencyString(account_value[account]) << "\"\n";
			}
			for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
				Account *account = *it;
				outf << "\"" << account->name() << "\",\"" << QLocale().toCurrencyString(account_change[account]) << "\",\"" << QLocale().toCurrencyString(account_value[account]) << "\"\n";
			}
			for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
				Account *account = *it;
				outf << "\"" << account->name() << "\",\"" << QLocale().toCurrencyString(account_change[account]) << "\",\"" << QLocale().toCurrencyString(account_value[account]) << "\"\n";
			}
			break;
		}
	}

	return true;
}
void Eqonomize::printPreviewPaint(QPrinter *printer) {
	QString str;
	QTextStream stream(&str, QIODevice::WriteOnly);
	saveView(stream, 'h');
	QTextDocument htmldoc;
	htmldoc.setHtml(str);
	htmldoc.print(printer);
}
void Eqonomize::showPrintPreview() {
	if(tabs->currentIndex() == EXPENSES_PAGE_INDEX) {
		if(expensesWidget->isEmpty()) {
			QMessageBox::critical(this, tr("Error"), tr("Empty expenses list."));
			return;
		}
	} else if(tabs->currentIndex() == INCOMES_PAGE_INDEX) {
		if(incomesWidget->isEmpty()) {
			QMessageBox::critical(this, tr("Error"), tr("Empty incomes list."));
			return;
		}
	} else if(tabs->currentIndex() == TRANSFERS_PAGE_INDEX) {
		if(transfersWidget->isEmpty()) {
			QMessageBox::critical(this, tr("Error"), tr("Empty transfers list."));
			return;
		}
	} else if(tabs->currentIndex() == SECURITIES_PAGE_INDEX) {
		if(securitiesView->topLevelItemCount() == 0) {
			QMessageBox::critical(this, tr("Error"), tr("Empty securities list.", "Financial security (e.g. stock, mutual fund)"));
			return;
		}
	} else if(tabs->currentIndex() == SCHEDULE_PAGE_INDEX) {
		if(scheduleView->topLevelItemCount() == 0) {
			QMessageBox::critical(this, tr("Error"), tr("Empty schedule list."));
			return;
		}
	}
	
	QPrintPreviewDialog preview_dialog(this, Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
	connect(&preview_dialog, SIGNAL(paintRequested(QPrinter*)), this, SLOT(printPreviewPaint(QPrinter*)));
	preview_dialog.exec();
}
void Eqonomize::printView() {
	if(tabs->currentIndex() == EXPENSES_PAGE_INDEX) {
		if(expensesWidget->isEmpty()) {
			QMessageBox::critical(this, tr("Error"), tr("Empty expenses list."));
			return;
		}
	} else if(tabs->currentIndex() == INCOMES_PAGE_INDEX) {
		if(incomesWidget->isEmpty()) {
			QMessageBox::critical(this, tr("Error"), tr("Empty incomes list."));
			return;
		}
	} else if(tabs->currentIndex() == TRANSFERS_PAGE_INDEX) {
		if(transfersWidget->isEmpty()) {
			QMessageBox::critical(this, tr("Error"), tr("Empty transfers list."));
			return;
		}
	} else if(tabs->currentIndex() == SECURITIES_PAGE_INDEX) {
		if(securitiesView->topLevelItemCount() == 0) {
			QMessageBox::critical(this, tr("Error"), tr("Empty securities list.", "Financial security (e.g. stock, mutual fund)"));
			return;
		}
	} else if(tabs->currentIndex() == SCHEDULE_PAGE_INDEX) {
		if(scheduleView->topLevelItemCount() == 0) {
			QMessageBox::critical(this, tr("Error"), tr("Empty schedule list."));
			return;
		}
	}
	
	QPrinter printer;
	QPrintDialog print_dialog(&printer, this);
	if(print_dialog.exec() == QDialog::Accepted) {
		QString str;
		QTextStream stream(&str, QIODevice::WriteOnly);
		saveView(stream, 'h');
		QTextDocument htmldoc;
		htmldoc.setHtml(str);
		htmldoc.print(&printer);
	}
}
bool Eqonomize::saveView(QTextStream &file, int fileformat) {
	if(tabs->currentIndex() == ACCOUNTS_PAGE_INDEX) {return exportAccountsList(file, fileformat);}
	else if(tabs->currentIndex() == EXPENSES_PAGE_INDEX) {return expensesWidget->exportList(file, fileformat);}
	else if(tabs->currentIndex() == INCOMES_PAGE_INDEX) {return incomesWidget->exportList(file, fileformat);}
	else if(tabs->currentIndex() == TRANSFERS_PAGE_INDEX) {return transfersWidget->exportList(file, fileformat);}
	else if(tabs->currentIndex() == SECURITIES_PAGE_INDEX) {return exportSecuritiesList(file, fileformat);}
	else if(tabs->currentIndex() == SCHEDULE_PAGE_INDEX) {return exportScheduleList(file, fileformat);}
	return false;
}
void Eqonomize::onFilterSelected(QString filter) {
	QMimeDatabase db;
	QFileDialog *fileDialog = qobject_cast<QFileDialog*>(sender());
	if(filter == db.mimeTypeForName("text/csv").filterString()) {
		fileDialog->setDefaultSuffix(db.mimeTypeForName("text/csv").preferredSuffix());
	} else {
		fileDialog->setDefaultSuffix(db.mimeTypeForName("text/html").preferredSuffix());
	}
}
void Eqonomize::saveView() {
	if(tabs->currentIndex() == EXPENSES_PAGE_INDEX) {
		if(expensesWidget->isEmpty()) {
			QMessageBox::critical(this, tr("Error"), tr("Empty expenses list."));
			return;
		}
	} else if(tabs->currentIndex() == INCOMES_PAGE_INDEX) {
		if(incomesWidget->isEmpty()) {
			QMessageBox::critical(this, tr("Error"), tr("Empty incomes list."));
			return;
		}
	} else if(tabs->currentIndex() == TRANSFERS_PAGE_INDEX) {
		if(transfersWidget->isEmpty()) {
			QMessageBox::critical(this, tr("Error"), tr("Empty transfers list."));
			return;
		}
	} else if(tabs->currentIndex() == SECURITIES_PAGE_INDEX) {
		if(securitiesView->topLevelItemCount() == 0) {
			QMessageBox::critical(this, tr("Error"), tr("Empty securities list.", "Financial security (e.g. stock, mutual fund)"));
			return;
		}
	} else if(tabs->currentIndex() == SCHEDULE_PAGE_INDEX) {
		if(scheduleView->topLevelItemCount() == 0) {
			QMessageBox::critical(this, tr("Error"), tr("Empty schedule list."));
			return;
		}
	}
	char filetype = 'h';
	QMimeDatabase db;
	QString html_filter = db.mimeTypeForName("text/html").filterString();
	QString csv_filter = db.mimeTypeForName("text/csv").filterString();
	QFileDialog fileDialog(this);
	fileDialog.setNameFilters(QStringList(html_filter) << csv_filter);
	fileDialog.selectNameFilter(html_filter);
	fileDialog.setDefaultSuffix(db.mimeTypeForName("text/html").preferredSuffix());
	fileDialog.setAcceptMode(QFileDialog::AcceptSave);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
	fileDialog.setSupportedSchemes(QStringList("file"));
#endif
	fileDialog.setDirectory(last_document_directory);
	connect(&fileDialog, SIGNAL(filterSelected(QString)), this, SLOT(onFilterSelected(QString)));
	QString url;
	if(!fileDialog.exec()) return;
	QStringList urls = fileDialog.selectedFiles();
	if(urls.isEmpty()) return;
	url = urls[0];
	if((fileDialog.selectedNameFilter() == csv_filter || db.mimeTypeForFile(url, QMimeDatabase::MatchExtension) == db.mimeTypeForName("text/csv")) && db.mimeTypeForFile(url, QMimeDatabase::MatchExtension) != db.mimeTypeForName("text/html")) filetype = 'c';
	QSaveFile ofile(url);
	ofile.open(QIODevice::WriteOnly);
	if(!ofile.isOpen()) {
		ofile.cancelWriting();
		QMessageBox::critical(this, tr("Error"), tr("Couldn't open file for writing."));
		return;
	}
	last_document_directory = fileDialog.directory().absolutePath();
	QTextStream stream(&ofile);
	saveView(stream, filetype);
	if(!ofile.commit()) {
		QMessageBox::critical(this, tr("Error"), tr("Error while writing file; file was not saved."));
		return;
	}

}

#define NEW_ACTION(action, text, icon, shortcut, receiver, slot, name, menu) action = new QAction(this); action->setObjectName(name); action->setText(text); action->setIcon(LOAD_ICON(icon)); action->setShortcut(shortcut); menu->addAction(action); connect(action, SIGNAL(triggered()), receiver, slot);

#define NEW_ACTION_ALT(action, text, icon, icon_alt, shortcut, receiver, slot, name, menu) action = new QAction(this); action->setObjectName(name); action->setText(text); action->setIcon(LOAD_ICON2(icon, icon_alt)); action->setShortcut(shortcut); menu->addAction(action); connect(action, SIGNAL(triggered()), receiver, slot);

#define NEW_ACTION_3(action, text, icon, shortcuts, receiver, slot, name, menu) action = new QAction(this); action->setObjectName(name); action->setText(text); action->setIcon(LOAD_ICON(icon)); action->setShortcuts(shortcuts); menu->addAction(action); connect(action, SIGNAL(triggered()), receiver, slot);

#define NEW_ACTION_NOMENU(action, text, icon, shortcut, receiver, slot, name) action = new QAction(this); action->setObjectName(name); action->setText(text); action->setIcon(LOAD_ICON(icon)); action->setShortcut(shortcut); connect(action, SIGNAL(triggered()), receiver, slot);

#define NEW_ACTION_NOMENU_ALT(action, text, icon, icon_alt, shortcut, receiver, slot, name) action = new QAction(this); action->setObjectName(name); action->setText(text); action->setIcon(LOAD_ICON2(icon, icon_alt)); action->setShortcut(shortcut); connect(action, SIGNAL(triggered()), receiver, slot);

#define NEW_ACTION_2(action, text, shortcut, receiver, slot, name, menu) action = new QAction(this); action->setObjectName(name); action->setText(text); action->setShortcut(shortcut); menu->addAction(action); connect(action, SIGNAL(triggered()), receiver, slot);

#define NEW_ACTION_2_NOMENU(action, text, shortcut, receiver, slot, name) action = new QAction(this); action->setObjectName(name); action->setText(text); action->setShortcut(shortcut); connect(action, SIGNAL(triggered()), receiver, slot);

#define NEW_TOGGLE_ACTION(action, text, shortcut, receiver, slot, name, menu) action = new QAction(this); action->setObjectName(name); action->setText(text); action->setShortcut(shortcut); action->setCheckable(true); menu->addAction(action); connect(action, SIGNAL(triggered(bool)), receiver, slot);

#define NEW_RADIO_ACTION(action, text, group) action = new QAction(group); action->setCheckable(true); action->setText(text);

void Eqonomize::setupActions() {
	
	QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
	QMenu *accountsMenu = menuBar()->addMenu(tr("&Accounts"));
	QMenu *transactionsMenu = menuBar()->addMenu(tr("&Transactions"));
	QMenu *loansMenu = menuBar()->addMenu(tr("&Loans"));
	QMenu *securitiesMenu = menuBar()->addMenu(tr("&Securities", "Financial security (e.g. stock, mutual fund)"));
	QMenu *statisticsMenu = menuBar()->addMenu(tr("Stat&istics"));
	QMenu *settingsMenu = menuBar()->addMenu(tr("S&ettings"));
	QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
	
	fileToolbar = addToolBar(tr("File"));
	fileToolbar->setObjectName("file_toolbar");
	fileToolbar->setFloatable(false);
	fileToolbar->setMovable(false);
	accountsToolbar = addToolBar(tr("Accounts"));
	accountsToolbar->setObjectName("account_toolbar");
	accountsToolbar->setFloatable(false);
	accountsToolbar->setMovable(false);
	transactionsToolbar = addToolBar(tr("Transactions"));
	transactionsToolbar->setObjectName("transaction_toolbar");
	transactionsToolbar->setFloatable(false);
	transactionsToolbar->setMovable(false);
	statisticsToolbar = addToolBar(tr("Statistics"));
	statisticsToolbar->setObjectName("statistics_toolbar");
	statisticsToolbar->setFloatable(false);
	statisticsToolbar->setMovable(false);
	
	new QShortcut(QKeySequence::Find, this, SLOT(showFilter()));
	
	NEW_ACTION_3(ActionFileNew, tr("&New"), "document-new", QKeySequence::New, this, SLOT(fileNew()), "file_new", fileMenu);
	fileToolbar->addAction(ActionFileNew);
	NEW_ACTION_3(ActionFileOpen, tr("&Open…"), "document-open", QKeySequence::Open, this, SLOT(fileOpen()), "file_open", fileMenu);
	fileToolbar->addAction(ActionFileOpen);
	recentFilesMenu = fileMenu->addMenu(LOAD_ICON("document-open-recent"), tr("Open Recent"));
	QAction* recentFileAction = 0;
	for(int i = 0; i < MAX_RECENT_FILES; i++){
		recentFileAction = new QAction(this);
		recentFileAction->setVisible(false);
		QObject::connect(recentFileAction, SIGNAL(triggered()), this, SLOT(openRecent()));
		recentFilesMenu->addAction(recentFileAction);
		recentFileActionList.append(recentFileAction);
	}
	recentFilesMenu->addSeparator();
	NEW_ACTION_2(ActionClearRecentFiles, tr("Clear List"), 0, this, SLOT(clearRecentFiles()), "clear_recent_files", recentFilesMenu);
	fileMenu->addSeparator();
	NEW_ACTION_3(ActionFileSave, tr("&Save"), "document-save", QKeySequence::Save, this, SLOT(fileSave()), "file_save", fileMenu);
	fileToolbar->addAction(ActionFileSave);
	NEW_ACTION_3(ActionFileSaveAs, tr("Save As…"), "document-save-as", QKeySequence::SaveAs, this, SLOT(fileSaveAs()), "file_save_as", fileMenu);
	NEW_ACTION(ActionFileReload, tr("&Revert"), "document-revert", 0, this, SLOT(fileReload()), "file_revert", fileMenu);
	NEW_ACTION(ActionFileSync, tr("S&ynchronize"), "view-refresh", 0, this, SLOT(fileSynchronize()), "file_synchronize", fileMenu);
	fileMenu->addSeparator();
	NEW_ACTION_3(ActionPrintView, tr("&Print…"), "document-print", QKeySequence::Print, this, SLOT(printView()), "print_view", fileMenu);
	NEW_ACTION(ActionPrintPreview, tr("Print Preview…"), "document-print-preview", 0, this, SLOT(showPrintPreview()), "print_preview", fileMenu);
	fileToolbar->addAction(ActionPrintPreview);
	fileMenu->addSeparator();
	QMenu *importMenu = fileMenu->addMenu(tr("Import"));
	NEW_ACTION(ActionImportEQZ, tr("Import %1 File…").arg(qApp->applicationDisplayName()), "document-import", 0, this, SLOT(importEQZ()), "import_eqz", importMenu);
	NEW_ACTION(ActionImportCSV, tr("Import CSV File…"), "document-import", 0, this, SLOT(importCSV()), "import_csv", importMenu);
	NEW_ACTION(ActionImportQIF, tr("Import QIF File…"), "document-import", 0, this, SLOT(importQIF()), "import_qif", importMenu);
	NEW_ACTION_ALT(ActionSaveView, tr("Export View…"), "document-export", "eqz-export", 0, this, SLOT(saveView()), "save_view", fileMenu);
	fileToolbar->addAction(ActionSaveView);
	NEW_ACTION(ActionExportQIF, tr("Export As QIF File…"), "document-export", 0, this, SLOT(exportQIF()), "export_qif", fileMenu);
	fileMenu->addSeparator();
	NEW_ACTION(ActionUpdateExchangeRates, tr("Update Exchange Rates"), "view-refresh", 0, this, SLOT(updateExchangeRates()), "update_exchange_rates", fileMenu);
	NEW_ACTION_2(ActionConvertCurrencies, tr("Currency Converter"), 0, this, SLOT(openCurrencyConversion()), "convert_currencies", fileMenu);
	fileMenu->addSeparator();
	QList<QKeySequence> keySequences;	
	keySequences << QKeySequence(Qt::CTRL+Qt::Key_Q);
	keySequences << QKeySequence(QKeySequence::Quit);
	NEW_ACTION_3(ActionQuit, tr("&Quit"), "application-exit", keySequences, this, SLOT(close()), "application_quit", fileMenu);
	
	NEW_ACTION_NOMENU(ActionAddAccount, tr("Add Account…"), "document-new", 0, this, SLOT(addAccount()), "add_account");
	NEW_ACTION(ActionNewAssetsAccount, tr("New Account…"), "eqz-account", 0, this, SLOT(newAssetsAccount()), "new_assets_account", accountsMenu);
	NEW_ACTION(ActionNewLoan, tr("New Loan…"), "eqz-liabilities", 0, this, SLOT(newLoan()), "new_loan", loansMenu);
	NEW_ACTION(ActionNewIncomesAccount, tr("New Income Category…"), "eqz-income", 0, this, SLOT(newIncomesAccount()), "new_incomes_account", accountsMenu);
	NEW_ACTION(ActionNewExpensesAccount, tr("New Expense Category…"), "eqz-expense", 0, this, SLOT(newExpensesAccount()), "new_expenses_account", accountsMenu);
	NEW_ACTION_NOMENU(ActionAddAccountMenu, tr("Add Account…"), "eqz-account", 0, this, SLOT(addAccount()), "add_account");
	QMenu *newAccountMenu = new QMenu(tr("Add Account"), this);
	newAccountMenu->setIcon(LOAD_ICON("eqz-account"));
	newAccountMenu->addAction(ActionNewAssetsAccount);
	newAccountMenu->addAction(ActionNewLoan);
	newAccountMenu->addAction(ActionNewIncomesAccount);
	newAccountMenu->addAction(ActionNewExpensesAccount);
	ActionAddAccountMenu->setMenu(newAccountMenu);
	accountsToolbar->addAction(ActionAddAccountMenu);
	accountsMenu->addSeparator();
	NEW_ACTION_ALT(ActionEditAccount, tr("Edit…"), "document-edit", "eqz-edit", 0, this, SLOT(editAccount()), "edit_account", accountsMenu);
	NEW_ACTION(ActionReconcileAccount, tr("Reconcile Account…"), "eqz-ledger", 0, this, SLOT(reconcileAccount()), "reconcile_account", accountsMenu);
	NEW_ACTION(ActionBalanceAccount, tr("Adjust balance…", "Referring to account balance"), "eqz-balance", 0, this, SLOT(balanceAccount()), "balance_account", accountsMenu);
	accountsMenu->addSeparator();
	NEW_ACTION(ActionCloseAccount, tr("Close Account", "Mark account as closed"), "edit-delete", 0, this, SLOT(closeAccount()), "close_account", accountsMenu);
	NEW_ACTION(ActionDeleteAccount, tr("Remove"), "edit-delete", 0, this, SLOT(deleteAccount()), "delete_account", accountsMenu);
	accountsMenu->addSeparator();
	NEW_ACTION(ActionShowAccountTransactions, tr("Show Transactions"), "eqz-transactions", 0, this, SLOT(showAccountTransactions()), "show_account_transactions", accountsMenu);
	NEW_ACTION(ActionShowLedger, tr("Show Ledger"), "eqz-ledger", 0, this, SLOT(showLedger()), "show_ledger", accountsMenu);
	accountsToolbar->addAction(ActionShowLedger);

	NEW_ACTION(ActionNewExpense, tr("New Expense…"), "eqz-expense", Qt::CTRL+Qt::Key_E, this, SLOT(newScheduledExpense()), "new_expense", transactionsMenu);
	transactionsToolbar->addAction(ActionNewExpense);
	NEW_ACTION(ActionNewIncome, tr("New Income…"), "eqz-income", Qt::CTRL+Qt::Key_I, this, SLOT(newScheduledIncome()), "new_income", transactionsMenu);
	transactionsToolbar->addAction(ActionNewIncome);
	NEW_ACTION(ActionNewTransfer, tr("New Transfer…"), "eqz-transfer", Qt::CTRL+Qt::Key_T, this, SLOT(newScheduledTransfer()), "new_transfer", transactionsMenu);
	transactionsToolbar->addAction(ActionNewTransfer);
	NEW_ACTION(ActionNewMultiItemTransaction, tr("New Split Transaction…"), "eqz-split-transaction", Qt::CTRL+Qt::Key_W, this, SLOT(newMultiItemTransaction()), "new_multi_item_transaction", transactionsMenu);
	transactionsToolbar->addAction(ActionNewMultiItemTransaction);
	NEW_ACTION(ActionNewMultiAccountExpense, tr("New Expense with Multiple Payments…"), "eqz-expense", 0, this, SLOT(newMultiAccountExpense()), "new_multi_account_expense", transactionsMenu);
	NEW_ACTION_NOMENU(ActionNewRefund, tr("Refund…"), "eqz-income", 0, this, SLOT(newRefund()), "new_refund");
	NEW_ACTION_NOMENU(ActionNewRepayment, tr("Repayment…"), "eqz-expense", 0, this, SLOT(newRepayment()), "new_repayment");
	NEW_ACTION(ActionNewRefundRepayment, tr("New Refund/Repayment…"), "eqz-refund-repayment", 0, this, SLOT(newRefundRepayment()), "new_refund_repayment", transactionsMenu);
	transactionsMenu->addSeparator();
	NEW_ACTION_ALT(ActionEditTransaction, tr("Edit Transaction(s) (Occurrence)…"), "document-edit", "eqz-edit", 0, this, SLOT(editSelectedTransaction()), "edit_transaction", transactionsMenu);
	NEW_ACTION_NOMENU_ALT(ActionEditOccurrence, tr("Edit Occurrence…"), "document-edit", "eqz-edit", 0, this, SLOT(editOccurrence()), "edit_occurrence");
	NEW_ACTION_ALT(ActionEditScheduledTransaction, tr("Edit Schedule (Recurrence)…"), "document-edit", "eqz-edit", 0, this, SLOT(editSelectedScheduledTransaction()), "edit_scheduled_transaction", transactionsMenu);
	NEW_ACTION_NOMENU_ALT(ActionEditSchedule, tr("Edit Schedule…"), "document-edit", "eqz-edit", 0, this, SLOT(editScheduledTransaction()), "edit_schedule");
	NEW_ACTION_ALT(ActionEditSplitTransaction, tr("Edit Split Transaction…"), "document-edit", "eqz-edit", 0, this, SLOT(editSelectedSplitTransaction()), "edit_split_transaction", transactionsMenu);
	NEW_ACTION(ActionJoinTransactions, tr("Join Transactions…"), "eqz-join-transactions", 0, this, SLOT(joinSelectedTransactions()), "join_transactions", transactionsMenu);
	NEW_ACTION(ActionSplitUpTransaction, tr("Split Up Transaction"), "eqz-split-transaction", 0, this, SLOT(splitUpSelectedTransaction()), "split_up_transaction", transactionsMenu);
	transactionsMenu->addSeparator();
	NEW_ACTION(ActionSelectAssociatedFile, tr("Select Associated File"), "document-open", 0, this, SLOT(selectAssociatedFile()), "select_attachment", transactionsMenu);
	NEW_ACTION(ActionOpenAssociatedFile, tr("Open Associated File"), "system-run", 0, this, SLOT(openAssociatedFile()), "open_attachment", transactionsMenu);
	transactionsMenu->addSeparator();
	NEW_ACTION(ActionDeleteTransaction, tr("Remove Transaction(s) (Occurrence)"), "edit-delete", 0, this, SLOT(deleteSelectedTransaction()), "delete_transaction", transactionsMenu);
	NEW_ACTION_NOMENU(ActionDeleteOccurrence, tr("Remove Occurrence"), "edit-delete", 0, this, SLOT(removeOccurrence()), "delete_occurrence");
	NEW_ACTION(ActionDeleteScheduledTransaction, tr("Delete Schedule (Recurrence)"), "edit-delete", 0, this, SLOT(deleteSelectedScheduledTransaction()), "delete_scheduled_transaction", transactionsMenu);
	NEW_ACTION_NOMENU(ActionDeleteSchedule, tr("Delete Schedule"), "edit-delete", 0, this, SLOT(removeScheduledTransaction()), "delete_schedule");	
	NEW_ACTION(ActionDeleteSplitTransaction, tr("Remove Split Transaction"), "edit-delete", 0, this, SLOT(deleteSelectedSplitTransaction()), "delete_split_transaction", transactionsMenu);
	
	loansMenu->addSeparator();
	NEW_ACTION(ActionNewDebtPayment, tr("New Debt Payment…"), "eqz-debt-payment", 0, this, SLOT(newDebtPayment()), "new_loan_transaction", loansMenu);
	NEW_ACTION(ActionNewDebtInterest, tr("New Unpaid Interest…"), "eqz-debt-interest", 0, this, SLOT(newDebtInterest()), "new_debt_interest", loansMenu);
	NEW_ACTION(ActionNewExpenseWithLoan, tr("New Expense Paid with Loan…"), "eqz-expense", 0, this, SLOT(newExpenseWithLoan()), "new_expense_with_loan", loansMenu);
	transactionsToolbar->addAction(ActionNewDebtPayment);

	NEW_ACTION(ActionNewSecurity, tr("New Security…", "Financial security (e.g. stock, mutual fund)"), "document-new", 0, this, SLOT(newSecurity()), "new_security", securitiesMenu);
	NEW_ACTION_ALT(ActionEditSecurity, tr("Edit Security…", "Financial security (e.g. stock, mutual fund)"), "document-edit", "eqz-edit", 0, this, SLOT(editSecurity()), "edit_security", securitiesMenu);
	NEW_ACTION(ActionDeleteSecurity, tr("Remove Security", "Financial security (e.g. stock, mutual fund)"), "edit-delete", 0, this, SLOT(deleteSecurity()), "delete_security", securitiesMenu);
	securitiesMenu->addSeparator();
	NEW_ACTION(ActionBuyShares, tr("Shares Bought…", "Financial shares"), "eqz-income", 0, this, SLOT(buySecurities()), "buy_shares", securitiesMenu);
	NEW_ACTION(ActionSellShares, tr("Shares Sold…", "Financial shares"), "eqz-expense", 0, this, SLOT(sellSecurities()), "sell_shares", securitiesMenu);
	NEW_ACTION(ActionNewSecurityTrade, tr("Shares Exchanged…", "Shares of one security directly exchanged for shares of another; Financial shares"), "eqz-transfer", 0, this, SLOT(newSecurityTrade()), "new_security_trade", securitiesMenu);
	ActionNewSecurityTrade->setToolTip(tr("Shares of one security directly exchanged for shares of another", "Financial shares"));
	NEW_ACTION(ActionNewDividend, tr("Dividend…"), "eqz-income", 0, this, SLOT(newDividend()), "new_dividend", securitiesMenu);
	NEW_ACTION(ActionNewReinvestedDividend, tr("Reinvested Dividend…"), "eqz-income", 0, this, SLOT(newReinvestedDividend()), "new_reinvested_dividend", securitiesMenu);
	NEW_ACTION(ActionEditSecurityTransactions, tr("Transactions…"), "eqz-transactions", 0, this, SLOT(editSecurityTransactions()), "edit_security_transactions", securitiesMenu);
	securitiesMenu->addSeparator();
	NEW_ACTION(ActionSetQuotation, tr("Set Quote…", "Financial quote"), "view-calendar-day", 0, this, SLOT(setQuotation()), "set_quotation", securitiesMenu);
	NEW_ACTION(ActionEditQuotations, tr("Edit Quotes…", "Financial quote"), "view-calendar-list", 0, this, SLOT(editQuotations()), "edit_quotations", securitiesMenu);

	NEW_ACTION(ActionOverTimeReport, tr("Development Over Time Report…"), "eqz-overtime-report", 0, this, SLOT(showOverTimeReport()), "over_time_report", statisticsMenu);
	statisticsToolbar->addAction(ActionOverTimeReport);
	NEW_ACTION(ActionCategoriesComparisonReport, tr("Categories Comparison Report…"), "eqz-categories-report", 0, this, SLOT(showCategoriesComparisonReport()), "categories_comparison_report", statisticsMenu);
	statisticsToolbar->addAction(ActionCategoriesComparisonReport);
	NEW_ACTION(ActionOverTimeChart, tr("Development Over Time Chart…"), "eqz-overtime-chart", 0, this, SLOT(showOverTimeChart()), "over_time_chart", statisticsMenu);
	statisticsToolbar->addAction(ActionOverTimeChart);
	NEW_ACTION(ActionCategoriesComparisonChart, tr("Categories Comparison Chart…"), "eqz-categories-chart", 0, this, SLOT(showCategoriesComparisonChart()), "categories_comparison_chart", statisticsMenu);
	statisticsToolbar->addAction(ActionCategoriesComparisonChart);

	NEW_TOGGLE_ACTION(ActionExtraProperties, tr("Show payee and quantity"), 0, this, SLOT(useExtraProperties(bool)), "extra_properties", settingsMenu);
	ActionExtraProperties->setChecked(b_extra);
	ActionExtraProperties->setToolTip(tr("Show quantity and payer/payee properties for incomes and expenses."));
	
	NEW_ACTION_2(ActionSetMainCurrency, tr("Set Main Currency…"), 0, this, SLOT(setMainCurrency()), "set_main_currency", settingsMenu);
	
	NEW_TOGGLE_ACTION(ActionUseExchangeRateForTransactionDate, tr("Use Exchange Rate for Transaction Date"), 0, this, SLOT(useExchangeRateForTransactionDate(bool)), "use_exchange_rate_for_transaction_date", settingsMenu);
	ActionUseExchangeRateForTransactionDate->setChecked(budget->defaultTransactionConversionRateDate() == TRANSACTION_CONVERSION_RATE_AT_DATE);
	ActionUseExchangeRateForTransactionDate->setToolTip(tr("Use the exchange rate nearest the transaction date, instead of the latest available rate, when converting the value of transactions."));
	
	NEW_ACTION(ActionSetBudgetPeriod, tr("Set Budget Period…"), "view-calendar-day", 0, this, SLOT(setBudgetPeriod()), "set_budget_period", settingsMenu);
	
	QMenu *periodMenu = settingsMenu->addMenu(tr("Initial Period"));
	ActionSelectInitialPeriod = new QActionGroup(this);
	ActionSelectInitialPeriod->setObjectName("select_initial_period");
	NEW_RADIO_ACTION(AIPCurrentMonth, tr("Current Month"), ActionSelectInitialPeriod);
	NEW_RADIO_ACTION(AIPCurrentYear, tr("Current Year"), ActionSelectInitialPeriod);
	NEW_RADIO_ACTION(AIPCurrentWholeMonth, tr("Current Whole Month"), ActionSelectInitialPeriod);
	NEW_RADIO_ACTION(AIPCurrentWholeYear, tr("Current Whole Year"), ActionSelectInitialPeriod);
	NEW_RADIO_ACTION(AIPRememberLastDates, tr("Remember Last Dates"), ActionSelectInitialPeriod);
	periodMenu->addActions(ActionSelectInitialPeriod->actions());
	
	QMenu *backupMenu = settingsMenu->addMenu(tr("Backup Frequency"));
	ActionSelectBackupFrequency = new QActionGroup(this);
	ActionSelectBackupFrequency->setObjectName("select_backup_frequency");
	NEW_RADIO_ACTION(ABFDaily, tr("Daily"), ActionSelectBackupFrequency);
	NEW_RADIO_ACTION(ABFWeekly, tr("Weekly"), ActionSelectBackupFrequency);
	NEW_RADIO_ACTION(ABFFortnightly, tr("Fortnightly"), ActionSelectBackupFrequency);
	NEW_RADIO_ACTION(ABFMonthly, tr("Monthly"), ActionSelectBackupFrequency);
	NEW_RADIO_ACTION(ABFNever, tr("Never"), ActionSelectBackupFrequency);
	backupMenu->addActions(ActionSelectBackupFrequency->actions());
	
	NEW_ACTION_2(ActionSyncSettings, tr("Cloud Synchronization (experimental)…"), 0, this, SLOT(openSynchronizationSettings()), "open_synchronization_settings", settingsMenu);
	
	NEW_ACTION_3(ActionHelp, tr("Help"), "help-contents", QKeySequence::HelpContents, this, SLOT(showHelp()), "help", helpMenu);
	//ActionWhatsThis = QWhatsThis::createAction(this); helpMenu->addAction(ActionWhatsThis);
	helpMenu->addSeparator();
	NEW_ACTION_2(ActionReportBug, tr("Report Bug"), 0, this, SLOT(reportBug()), "report-bug", helpMenu);
	helpMenu->addSeparator();
	NEW_ACTION(ActionAbout, tr("About %1").arg(qApp->applicationDisplayName()), "", 0, this, SLOT(showAbout()), "about", helpMenu);
	ActionAbout->setIcon(LOAD_APP_ICON("eqonomize"));
	NEW_ACTION(ActionAboutQt, tr("About Qt"), "help-about", 0, this, SLOT(showAboutQt()), "about-qt", helpMenu);

	//ActionFileSave->setEnabled(false);
	ActionFileReload->setEnabled(false);
	ActionBalanceAccount->setEnabled(false);
	ActionEditAccount->setEnabled(false);
	ActionDeleteAccount->setEnabled(false);
	ActionCloseAccount->setEnabled(false);
	ActionShowAccountTransactions->setEnabled(false);
	ActionReconcileAccount->setEnabled(false);
	ActionEditTransaction->setEnabled(false);
	ActionDeleteTransaction->setEnabled(false);
	ActionEditSplitTransaction->setEnabled(false);
	ActionDeleteSplitTransaction->setEnabled(false);
	ActionSelectAssociatedFile->setEnabled(false);
	ActionOpenAssociatedFile->setEnabled(false);
	ActionJoinTransactions->setEnabled(false);
	ActionSplitUpTransaction->setEnabled(false);
	ActionEditScheduledTransaction->setEnabled(false);
	ActionDeleteScheduledTransaction->setEnabled(false);
	ActionNewRefund->setEnabled(false);
	ActionNewRepayment->setEnabled(false);
	ActionNewRefundRepayment->setEnabled(false);
	ActionEditSecurity->setEnabled(false);
	ActionDeleteSecurity->setEnabled(false);
	ActionSellShares->setEnabled(true);
	ActionBuyShares->setEnabled(true);
	ActionNewDividend->setEnabled(true);
	ActionNewReinvestedDividend->setEnabled(true);
	ActionNewSecurityTrade->setEnabled(true);
	ActionSetQuotation->setEnabled(false);
	ActionEditQuotations->setEnabled(false);
	ActionEditSecurityTransactions->setEnabled(false);

}

void Eqonomize::openRecent(){
	QAction *action = qobject_cast<QAction*>(sender());
	if(action) fileOpenRecent(QUrl::fromLocalFile(action->data().toString()));
}

void Eqonomize::clearRecentFiles(){
	QSettings settings;
	settings.beginGroup("GeneralOptions");
	QStringList recentFilePaths;
	settings.setValue("recentFiles", recentFilePaths);
	for (int i = 0; i < MAX_RECENT_FILES; i++) {
		recentFileActionList.at(i)->setVisible(false);
	}
	recentFilesMenu->setEnabled(false);
}

void Eqonomize::updateRecentFiles(QString filePath){
	QSettings settings;
	settings.beginGroup("GeneralOptions");
	QStringList recentFilePaths = settings.value("recentFiles").toStringList();
	if(!filePath.isEmpty()) {
		recentFilePaths.removeAll(filePath);
		recentFilePaths.prepend(filePath);
		while(recentFilePaths.size() > MAX_RECENT_FILES) recentFilePaths.removeLast();
		settings.setValue("recentFiles", recentFilePaths);
	}
	for (int i = 0; i < recentFilePaths.size() && i < MAX_RECENT_FILES; i++) {
		recentFileActionList.at(i)->setText(QFileInfo(recentFilePaths.at(i)).fileName());
		recentFileActionList.at(i)->setData(recentFilePaths.at(i));
		recentFileActionList.at(i)->setVisible(true);
	}
	for (int i = recentFilePaths.size(); i < MAX_RECENT_FILES; i++) {
		recentFileActionList.at(i)->setVisible(false);
	}
	recentFilesMenu->setEnabled(recentFilePaths.size() > 0);
}

void Eqonomize::showFilter() {
	TransactionListWidget *w = NULL;
	if(tabs->currentIndex() == EXPENSES_PAGE_INDEX) w = expensesWidget;
	else if(tabs->currentIndex() == INCOMES_PAGE_INDEX) w = incomesWidget;
	else if(tabs->currentIndex() == TRANSFERS_PAGE_INDEX) w = transfersWidget;
	if(!w) return;
	w->showFilter(true);
}

void Eqonomize::showHelp() {
	QLocale locale;
	QStringList langs = locale.uiLanguages();
	QString docdir = DOCUMENTATION_DIR "/C";
	for(int i = 0; i < langs.size(); ++i) {
		QString lang = langs.at(i);
		lang.replace('-', '_');
		QFileInfo fileinfo(QString(DOCUMENTATION_DIR "/") + lang + "/index.html");
		if(fileinfo.exists()) {
			docdir = QString(DOCUMENTATION_DIR "/") + lang;
			break;;
		}
		lang = lang.section('_', 0, 0);
		fileinfo.setFile(QString(DOCUMENTATION_DIR "/") + lang + "/index.html");
		if(fileinfo.exists()) {
			docdir = QString(DOCUMENTATION_DIR "/") + lang;
			break;
		}
	}
	if(!helpDialog) {
		helpDialog = new QDialog(0, Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
		helpDialog->setModal(false);
		helpDialog->setWindowTitle(tr("Help"));
		QVBoxLayout *box1 = new QVBoxLayout(helpDialog);
		QTextBrowser *helpBrowser = new QTextBrowser(helpDialog);
		if(docdir[0] == ':') {
			helpBrowser->setSource(QUrl(QString("qrc") + docdir + "/index.html"));
			helpBrowser->setSearchPaths(QStringList(QString("qrc") + docdir));
		} else {
			helpBrowser->setSearchPaths(QStringList(docdir));
			helpBrowser->setSource(QUrl(docdir + "/index.html"));
		}
		box1->addWidget(helpBrowser);
		QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
		connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), helpDialog, SLOT(reject()));
		box1->addWidget(buttonBox);
		QSize helpSize = QDesktopWidget().availableGeometry(this).size();
		helpSize.setHeight(helpSize.height() * 0.85);
		helpSize.setWidth(helpSize.height() * 1.2);
		helpDialog->resize(helpSize);
	}
	helpDialog->show();
	helpDialog->raise();
	helpDialog->activateWindow();
}
void Eqonomize::reportBug() {
	QDesktopServices::openUrl(QUrl("https://github.com/Eqonomize/Eqonomize/issues/new"));
}
void Eqonomize::showAbout() {
	QMessageBox::about(this, tr("About %1").arg(qApp->applicationDisplayName()), QString("<font size=+2><b>%1 v1.3.2</b></font><br><font size=+1>%2</font><br><<font size=+1><i><a href=\"http://eqonomize.github.io/\">http://eqonomize.github.io/</a></i></font><br><br>Copyright © 2006-2008, 2014, 2016-2018 Hanna Knutsson<br>%3").arg(qApp->applicationDisplayName()).arg(tr("A personal accounting program")).arg(tr("License: GNU General Public License Version 3")));
}
void Eqonomize::showAboutQt() {
	QMessageBox::aboutQt(this);
}

bool Eqonomize::crashRecovery(QUrl url) {
	QString autosaveFileName;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
		autosaveFileName = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/autosave/";
#else
		autosaveFileName = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/Eqonomize/eqonomize/autosave/";
#endif
	if(url.isEmpty()) autosaveFileName += "UNSAVED EQZ";
	else autosaveFileName += url.fileName();
	QFileInfo fileinfo(autosaveFileName);
	if(fileinfo.exists() && fileinfo.isWritable()) {
		if(QMessageBox::question(this, tr("Crash Recovery"), tr("%1 exited unexpectedly before the file was saved and data was lost.\nDo you want to load the last auto-saved version of the file?").arg(qApp->applicationDisplayName()), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
			QString errors;
			bool new_currency = false;
			QString error = budget->loadFile(autosaveFileName, errors, &new_currency);
			if(!error.isNull()) {
				QMessageBox::critical(this, tr("Couldn't open file"), tr("Error loading %1: %2.").arg(autosaveFileName).arg(error));
				QFile autosaveFile(autosaveFileName);
				autosaveFile.remove();
				return false;
			}
			if(!errors.isEmpty()) {
				QMessageBox::critical(this, tr("Error"), errors);
			}
			current_url = url;
			ActionFileReload->setEnabled(true);
			setWindowTitle(current_url.fileName() + "[*]");
			QFile autosaveFile(autosaveFileName);
			autosaveFile.remove();
			if(!cr_tmp_file.isEmpty()) {
				QFile autosaveFile2(cr_tmp_file);
				autosaveFile2.remove();
				cr_tmp_file = "";
			}
			
			if(new_currency) warnAndAskForExchangeRate();

			reloadBudget();

			emit accountsModified();
			emit transactionsModified();
			emit budgetUpdated();

			setModified(true);
			checkSchedule(true, this);

			return true;

		}
		QFile autosaveFile(autosaveFileName);
		autosaveFile.remove();
	}
	return false;

}
void Eqonomize::onAutoSaveTimeout() {
	auto_save_timeout = true;
	if(modified_auto_save) {
		autoSave();
	}
}
void Eqonomize::autoSave() {
	if(auto_save_timeout) {
		saveCrashRecovery();
		modified_auto_save = false;
		auto_save_timeout = false;
	}
}
void Eqonomize::saveCrashRecovery() {
	if(cr_tmp_file.isEmpty()) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
		cr_tmp_file = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/autosave/";
#else
		cr_tmp_file = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "Eqonomize/eqonomize/autosave/";
#endif
		QDir autosaveDir(cr_tmp_file);
		if(!autosaveDir.mkpath(cr_tmp_file)) {
			cr_tmp_file = "";
			return;
		}
		if(current_url.isEmpty()) cr_tmp_file += "UNSAVED EQZ";
		else cr_tmp_file += current_url.fileName();
	}	
	if(budget->saveFile(cr_tmp_file, QFile::ReadUser | QFile::WriteUser, true).isNull()) {
		QSettings settings;
		settings.beginGroup("GeneralOptions");
		settings.setValue("lastURL", current_url.url());
		settings.endGroup();
		settings.sync();
	}
}

void Eqonomize::setCommandLineParser(QCommandLineParser *p) {
	parser = p;
}
void Eqonomize::onActivateRequested(const QStringList &arguments, const QString &workingDirectory) {
	if(!arguments.isEmpty()) {
		parser->process(arguments);
		if(parser->isSet("expenses")) {
			showExpenses();
		} else if(parser->isSet("incomes")) {
			showIncomes();
		} else if(parser->isSet("transfers")) {
			showTransfers();
		}
		QStringList args = parser->positionalArguments();
		if(args.count() > 0) {
			QString curpath = QDir::currentPath();
			QDir::setCurrent(workingDirectory);
			openURL(QUrl::fromUserInput(args.at(0)));
			QDir::setCurrent(curpath);
		}
		args.clear();
	}
	setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
	show();
	activateWindow();
}

void Eqonomize::saveOptions() {	
	QSettings settings;
	settings.beginGroup("GeneralOptions");
	settings.setValue("lastURL", current_url.url());
	if(ActionSelectBackupFrequency->checkedAction() == ABFNever) {settings.setValue("backupFrequency", BACKUP_NEVER);}
	else if(ActionSelectBackupFrequency->checkedAction() == ABFDaily) {settings.setValue("backupFrequency", BACKUP_DAILY);}
	else if(ActionSelectBackupFrequency->checkedAction() == ABFWeekly) {settings.setValue("backupFrequency", BACKUP_WEEKLY);}
	else if(ActionSelectBackupFrequency->checkedAction() == ABFFortnightly) {settings.setValue("backupFrequency", BACKUP_FORTNIGHTLY);}
	else if(ActionSelectBackupFrequency->checkedAction() == ABFMonthly) {settings.setValue("backupFrequency", BACKUP_MONTHLY);}
	settings.setValue("useExtraProperties", b_extra);
	settings.setValue("useExchangeRateForTransactionDate", budget->defaultTransactionConversionRateDate() == TRANSACTION_CONVERSION_RATE_AT_DATE);
	settings.setValue("firstRun", false);
	settings.setValue("size", size());
	settings.setValue("windowState", saveState());
	settings.setValue("expensesListState", expensesWidget->saveState());
	settings.setValue("incomesListState", incomesWidget->saveState());
	settings.setValue("transfersListState", transfersWidget->saveState());
	settings.setValue("securitiesListState", securitiesView->header()->saveState());
	settings.setValue("scheduleListState", scheduleView->header()->saveState());

	if(ActionSelectInitialPeriod->checkedAction() == AIPCurrentMonth) {settings.setValue("initialPeriod", 0);}
	else if(ActionSelectInitialPeriod->checkedAction() == AIPCurrentYear) {settings.setValue("initialPeriod", 1);}
	else if(ActionSelectInitialPeriod->checkedAction() == AIPCurrentWholeMonth) {settings.setValue("initialPeriod", 2);}
	else if(ActionSelectInitialPeriod->checkedAction() == AIPCurrentWholeYear) {settings.setValue("initialPeriod", 3);}
	else {
		settings.setValue("initialPeriod", 4);
		settings.setValue("initialFromDate", from_date.toString(Qt::ISODate));
		settings.setValue("initialFromDateEnabled", accountsPeriodFromButton->isChecked());
		settings.setValue("initialToDate", to_date.toString(Qt::ISODate));
	}
	settings.endGroup();
	
	emit timeToSaveConfig();
}


void Eqonomize::readFileDependentOptions() {}
void Eqonomize::readOptions() {
	QSettings settings;
	settings.beginGroup("GeneralOptions");
	first_run = settings.value("firstRun", true).toBool();
	settings.setValue("firstRun", false);
	QSize window_size = settings.value("size", QSize()).toSize();
	if(window_size.isValid()) resize(window_size);
	restoreState(settings.value("windowState").toByteArray());
	expensesWidget->restoreState(settings.value("expensesListState").toByteArray());
	incomesWidget->restoreState(settings.value("incomesListState").toByteArray());
	transfersWidget->restoreState(settings.value("transfersListState").toByteArray());
	securitiesView->header()->restoreState(settings.value("securitiesListState").toByteArray());
	scheduleView->header()->restoreState(settings.value("scheduleListState").toByteArray());
	scheduleView->sortByColumn(0, Qt::AscendingOrder);
	settings.endGroup();
	updateRecentFiles();
}

void Eqonomize::closeEvent(QCloseEvent *event) {
	if(askSave(true)) {
		saveOptions();
		if(server) delete server;
		QMainWindow::closeEvent(event);
		qApp->closeAllWindows();
	} else {
		event->ignore();
	}
}

void Eqonomize::dragEnterEvent(QDragEnterEvent *event) {
	event->setAccepted(event->mimeData()->hasUrls());
}

void Eqonomize::dropEvent(QDropEvent *event) {
	QList<QUrl> urls =event->mimeData()->urls();
	if(!urls.isEmpty()) {
		const QUrl &url = urls.first();
		if(!askSave()) return;
		openURL(url);
	}
}

void Eqonomize::fileNew() {

	if(!askSave()) return;

	budget->clear();

	bool new_currency = false;
	if(current_url.isEmpty()) new_currency = budget->resetDefaultCurrency();

	current_url = "";
	setWindowTitle(tr("Untitled") + "[*]");
	ActionFileReload->setEnabled(false);
	QSettings settings;
	settings.beginGroup("GeneralOptions");
	settings.setValue("lastURL", current_url.url());
	if(!cr_tmp_file.isEmpty()) {
		QFile autosaveFile(cr_tmp_file);
		autosaveFile.remove();
		cr_tmp_file = "";
	}
	settings.endGroup();
	settings.sync();

	if(new_currency) warnAndAskForExchangeRate();

	reloadBudget();
	setModified(false);
	ActionFileSave->setEnabled(true);
	emit accountsModified();
	emit transactionsModified();
	emit budgetUpdated();

}

void Eqonomize::fileOpen() {
	if(!askSave()) return;
	QMimeDatabase db;
	QMimeType mime = db.mimeTypeForName("application/x-eqonomize");
	QString filter_string;
	if(mime.isValid()) filter_string = mime.filterString();
	if(filter_string.isEmpty()) filter_string = tr("Eqonomize! Accounting File") + "(*.eqz)";
	QString url = QFileDialog::getOpenFileName(this, QString(), current_url.isValid() ? current_url.adjusted(QUrl::RemoveFilename).toLocalFile() : QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QString("/"), filter_string);
	if(!url.isEmpty()) openURL(QUrl::fromLocalFile(url));
}
void Eqonomize::fileOpenRecent(const QUrl &url) {
	if(!askSave()) return;
	openURL(url);
}

void Eqonomize::fileReload() {
	openURL(current_url);
}
bool Eqonomize::fileSave() {
	if(!current_url.isValid()) {
		return fileSaveAs();
	} else {
		return saveURL(current_url);
	}
	return false;
}

bool Eqonomize::fileSaveAs() {
	return saveAs(true, true);
}
bool Eqonomize::saveAs(bool do_local_sync, bool do_cloud_sync, QWidget *parent) {
	if(!parent) parent = this;
	QMimeDatabase db;
	QMimeType mime = db.mimeTypeForName("application/x-eqonomize");
	QString filter_string;
	QString suffix;
	if(mime.isValid()) {
		filter_string = mime.filterString();
		suffix = mime.preferredSuffix();
	}
	if(filter_string.isEmpty()) filter_string = "Eqonomize! Accounting File (*.eqz)";
	if(suffix.isEmpty()) suffix = "eqz";
	QFileDialog fileDialog(parent);
	fileDialog.setNameFilter(filter_string);
	fileDialog.setDefaultSuffix(suffix);
	fileDialog.setAcceptMode(QFileDialog::AcceptSave);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
	fileDialog.setSupportedSchemes(QStringList("file"));
#endif
	if(current_url.isValid()) {
		fileDialog.setDirectory(current_url.adjusted(QUrl::RemoveFilename).toLocalFile());
	} else {
		fileDialog.setDirectory(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
		fileDialog.selectFile(QString("budget.") + suffix);
	}
	if(fileDialog.exec()) {
		QList<QUrl> urls = fileDialog.selectedUrls();
		if(urls.isEmpty()) return false;
		return saveURL(urls[0], do_local_sync, do_cloud_sync, parent);
	}
	return false;
}

bool Eqonomize::askSave(bool) {
	if(!modified) return true;
	int b_save = QMessageBox::warning(this, tr("Save file?"), tr("The current file has been modified. Do you want to save it?"), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
	if(b_save == QMessageBox::Yes) {
		return fileSave();
	}
	if(b_save == QMessageBox::No) {
		if(!cr_tmp_file.isEmpty()) {
			QFile autosaveFile(cr_tmp_file);
			autosaveFile.remove();
			cr_tmp_file = "";
		}
		return true;
	}
	return false;
}

void Eqonomize::optionsPreferences() {
}

void Eqonomize::checkSchedule() {
	checkSchedule(true, this);
}
bool Eqonomize::checkSchedule(bool update_display, QWidget *parent) {
	bool b = false;
	for(ScheduledTransactionList<ScheduledTransaction*>::const_iterator it = budget->scheduledTransactions.constBegin(); it != budget->scheduledTransactions.constEnd(); ++it) {
		ScheduledTransaction *strans = *it;
		if(strans->firstOccurrence() < QDate::currentDate() || (QTime::currentTime().hour() >= 18 && strans->firstOccurrence() == QDate::currentDate())) {
			b = true;
			break;
		}
	}
	if(b) {
		budget->setRecordNewAccounts(true);
		budget->setRecordNewSecurities(true);
		budget->resetDefaultCurrencyChanged();
		budget->resetCurrenciesModified();
		qApp->processEvents();
		ConfirmScheduleDialog *dialog = new ConfirmScheduleDialog(b_extra, budget, parent ? parent : this, tr("Confirm Schedule"));
		dialog->exec();
		foreach(Account* acc, budget->newAccounts) accountAdded(acc);
		budget->newAccounts.clear();
		foreach(Security *sec, budget->newSecurities) securityAdded(sec);
		budget->newSecurities.clear();
		if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
		budget->setRecordNewAccounts(false);
		budget->setRecordNewSecurities(false);
		Transactions *trans = dialog->firstTransaction();
		while(trans) {
			budget->addTransactions(trans);
			trans = dialog->nextTransaction();
		}
		dialog->deleteLater();
	}
	if(b && update_display) {
		expensesWidget->transactionsReset();
		incomesWidget->transactionsReset();
		transfersWidget->transactionsReset();
		filterAccounts();
		updateScheduledTransactions();
		updateSecurities();
		emit transactionsModified();
		setModified(true);
	}
	return b;
}


void Eqonomize::startBatchEdit() {
	in_batch_edit = true;
}
void Eqonomize::endBatchEdit() {
	if(in_batch_edit) {
		in_batch_edit = false;
		emit budgetUpdated();
		emit transactionsModified();
	}
}

void Eqonomize::updateScheduledTransactions() {
	scheduleView->clear();
	for(ScheduledTransactionList<ScheduledTransaction*>::const_iterator it = budget->scheduledTransactions.constBegin(); it != budget->scheduledTransactions.constEnd(); ++it) {
		ScheduledTransaction *strans = *it;
		new ScheduleListViewItem(scheduleView, strans, strans->firstOccurrence());
	}
	scheduleView->setSortingEnabled(true);
}
void Eqonomize::appendScheduledTransaction(ScheduledTransaction *strans) {
	new ScheduleListViewItem(scheduleView, strans, strans->firstOccurrence());
	scheduleView->setSortingEnabled(true);
}

void Eqonomize::addAccount() {
	QTreeWidgetItem *i = selectedItem(accountsView);
	if(i == NULL) {
		newAssetsAccount();
	} else if(i == liabilitiesItem || (account_items.contains(i) && (account_items[i]->type() == ACCOUNT_TYPE_ASSETS) && (((AssetsAccount*) account_items[i])->accountType() == ASSETS_TYPE_LIABILITIES))) {
		newLoan();
	} else if(i == assetsItem || (account_items.contains(i) && (account_items[i]->type() == ACCOUNT_TYPE_ASSETS))) {
		newAssetsAccount();
	} else if(i == incomesItem || (account_items.contains(i) && (account_items[i]->type() == ACCOUNT_TYPE_INCOMES))) {
		newIncomesAccount(account_items.contains(i) ? (IncomesAccount*) account_items[i] : NULL);
	} else {
		newExpensesAccount(account_items.contains(i) ? (ExpensesAccount*) account_items[i] : NULL);
	}
}
void Eqonomize::accountAdded(Account *acc) {
	switch(acc->type()) {
		case ACCOUNT_TYPE_ASSETS: {
			AssetsAccount *account = (AssetsAccount*) acc;
			appendAssetsAccount(account);
			filterAccounts();
			if(sender() != expensesWidget) expensesWidget->updateFromAccounts();
			if(sender() != incomesWidget) incomesWidget->updateToAccounts();
			if(sender() != transfersWidget) transfersWidget->updateAccounts();
			updateUsesMultipleCurrencies();
			break;
		}
		case ACCOUNT_TYPE_INCOMES: {
			IncomesAccount *account = (IncomesAccount*) acc;
			if(account->parentCategory()) {
				appendIncomesAccount(account, item_accounts[account->parentCategory()]);
			} else {
				appendIncomesAccount(account, incomesItem);
			}
			filterAccounts();
			if(sender() != incomesWidget) incomesWidget->updateFromAccounts();
			break;
		}
		case ACCOUNT_TYPE_EXPENSES: {
			ExpensesAccount *account = (ExpensesAccount*) acc;
			if(account->parentCategory()) {
				appendExpensesAccount(account, item_accounts[account->parentCategory()]);
			} else {
				appendExpensesAccount(account, expensesItem);
			}
			filterAccounts();
			if(sender() != expensesWidget) expensesWidget->updateToAccounts();
			break;
		}
	}
	emit accountsModified();
	setModified(true);
}
void Eqonomize::newAssetsAccount() {
	EditAssetsAccountDialog *dialog = new EditAssetsAccountDialog(budget, this, tr("New Account"));
	budget->resetDefaultCurrencyChanged();
	budget->resetCurrenciesModified();
	if(dialog->exec() == QDialog::Accepted) {
		AssetsAccount *account = dialog->newAccount();
		budget->addAccount(account);
		appendAssetsAccount(account);
		if(budget->currenciesModified() || budget->defaultCurrencyChanged()) {
			currenciesModified();
		} else {
			filterAccounts();
			expensesWidget->updateFromAccounts();
			incomesWidget->updateToAccounts();
			transfersWidget->updateAccounts();
			updateUsesMultipleCurrencies();
		}
		emit accountsModified();
		setModified(true);
	} else if(budget->currenciesModified() || budget->defaultCurrencyChanged()) {
		currenciesModified();
		if(budget->defaultCurrencyChanged()) setModified(true);
	}
	dialog->deleteLater();
}
void Eqonomize::newLoan() {
	EditAssetsAccountDialog *dialog = new EditAssetsAccountDialog(budget, this, tr("New Loan"), true);
	budget->resetDefaultCurrencyChanged();
	budget->resetCurrenciesModified();
	if(dialog->exec() == QDialog::Accepted) {
		Transaction *trans = NULL;
		AssetsAccount *account = dialog->newAccount(&trans);
		budget->addAccount(account);
		appendAssetsAccount(account);
		if(budget->currenciesModified() || budget->defaultCurrencyChanged()) {
			currenciesModified();
		} else {
			filterAccounts();
			expensesWidget->updateFromAccounts();
			incomesWidget->updateToAccounts();
			transfersWidget->updateAccounts();
			updateUsesMultipleCurrencies();
		}
		emit accountsModified();
		if(trans) {
			if(trans->date() > QDate::currentDate()) {
				ScheduledTransaction *strans = new ScheduledTransaction(budget, trans, NULL);
				budget->addScheduledTransaction(strans);
				transactionAdded(strans);
			} else {
				budget->addTransaction(trans);
				transactionAdded(trans);
			}
		}
		setModified(true);
	} else if(budget->currenciesModified() || budget->defaultCurrencyChanged()) {
		currenciesModified();
		if(budget->defaultCurrencyChanged()) setModified(true);
	}
	dialog->deleteLater();
}
void Eqonomize::newIncomesAccount(IncomesAccount *default_parent) {
	EditIncomesAccountDialog *dialog = new EditIncomesAccountDialog(budget, default_parent, this, tr("New Income Category"));
	if(dialog->exec() == QDialog::Accepted) {
		IncomesAccount *account = dialog->newAccount();
		budget->addAccount(account);
		if(account->parentCategory()) {
			appendIncomesAccount(account, item_accounts[account->parentCategory()]);
		} else {
			appendIncomesAccount(account, incomesItem);
		}
		filterAccounts();
		incomesWidget->updateFromAccounts();
		emit accountsModified();
		setModified(true);
	}
	dialog->deleteLater();
}
void Eqonomize::newExpensesAccount(ExpensesAccount *default_parent) {
	EditExpensesAccountDialog *dialog = new EditExpensesAccountDialog(budget, default_parent, this, tr("New Expense Category"));
	if(dialog->exec() == QDialog::Accepted) {
		ExpensesAccount *account = dialog->newAccount();
		budget->addAccount(account);
		if(account->parentCategory()) {
			appendExpensesAccount(account, item_accounts[account->parentCategory()]);
		} else {
			appendExpensesAccount(account, expensesItem);
		}
		filterAccounts();
		expensesWidget->updateToAccounts();
		emit accountsModified();
		setModified(true);
	}
	dialog->deleteLater();
}
void Eqonomize::accountExecuted(QTreeWidgetItem *i) {
	if(i->childCount() > 0) {
		i->setExpanded(!i->isExpanded());
	} else {
		showAccountTransactions(true);
	}
}
void Eqonomize::accountExecuted(QTreeWidgetItem *i, int c) {
	if(i == NULL) return;
	switch(c) {
		case 0: {
			if(i->childCount() > 0) {
				i->setExpanded(!i->isExpanded());
			} else if(account_items.contains(i)) {
				editAccount(account_items[i]);
			}
			break;
		}
		case 1: {
			if(account_items.contains(i)) {
				if(account_items[i]->type() == ACCOUNT_TYPE_ASSETS) {
					showAccountTransactions();
				} else {
					accountsTabs->setCurrentIndex(1);
					if(budgetEdit->isEnabled()) {
						budgetEdit->setFocus();
						budgetEdit->selectNumber();
					} else {
						budgetButton->setFocus();
					}
				}
			}
			break;
		}
		case 2: {
			showAccountTransactions();
			break;
		}
		case 3: {
			showAccountTransactions(true);
			break;
		}
	}
}
void Eqonomize::balanceAccount() {
	QTreeWidgetItem *i = selectedItem(accountsView);
	if(!account_items.contains(i)) return;
	Account *i_account = account_items[i];
	balanceAccount(i_account);
}
void Eqonomize::balanceAccount(Account *i_account) {
	if(!i_account) return;
	if(i_account->type() != ACCOUNT_TYPE_ASSETS || ((AssetsAccount*) i_account)->accountType() == ASSETS_TYPE_SECURITIES) return;
	AssetsAccount *account = (AssetsAccount*) i_account;
	double book_value = account->initialBalance();
	double current_balancing = 0.0;
	for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constBegin(); it != budget->transactions.constEnd(); ++it) {
		Transaction *trans = *it;
		if(trans->fromAccount() == account) {
			book_value -= trans->value();
			if(trans->toAccount() == budget->balancingAccount) current_balancing -= trans->value();
		}
		if(trans->toAccount() == account) {
			book_value += trans->value();
			if(trans->fromAccount() == budget->balancingAccount) current_balancing += trans->value();
		}
	}
	QDialog *dialog = new QDialog(this, 0);
	dialog->setWindowTitle(tr("Adjust Account Balance"));
	dialog->setModal(true);
	QVBoxLayout *box1 = new QVBoxLayout(dialog);
	box1->setSpacing(18);
	QGridLayout *grid = new QGridLayout();
	box1->addLayout(grid);
	grid->setSpacing(6);
	grid->addWidget(new QLabel(tr("Book value:"), dialog), 0, 0);
	QLabel *label = new QLabel(account->currency()->formatValue(book_value), dialog);
	label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	grid->addWidget(label, 0, 1);
	label = new QLabel(tr("of which %1 is balance adjustment", "Referring to account balance").arg(account->currency()->formatValue(current_balancing)), dialog);
	label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	grid->addWidget(label, 1, 1);
	grid->addWidget(new QLabel(tr("Real value:"), dialog), 2, 0);
	EqonomizeValueEdit *realEdit = new EqonomizeValueEdit(book_value, true, true, dialog, budget);
	realEdit->setCurrency(account->currency());
	grid->addWidget(realEdit, 2, 1);
	QFrame *frame = new QFrame(this);
	QHBoxLayout *wbox = new QHBoxLayout(frame);
	frame->setFrameShape(QFrame::Box);
	label = new QLabel(tr("Only use this when unable to find the cause of the incorrect recorded account balance."), dialog);
	label->setContentsMargins(6, 6, 6, 6);
	label->setWordWrap(true);
	wbox->addWidget(label);
	box1->addWidget(frame);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), dialog, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(accept()));
	box1->addWidget(buttonBox);
	if(dialog->exec() == QDialog::Accepted && realEdit->value() != book_value) {
		Transaction *trans = new Balancing(budget, realEdit->value() - book_value, QDate::currentDate(), account);
		budget->addTransaction(trans);
		transactionAdded(trans);
	}
	dialog->deleteLater();
}
void Eqonomize::editAccount() {
	QTreeWidgetItem *i = selectedItem(accountsView);
	if(!account_items.contains(i)) return;
	Account *i_account = account_items[i];
	editAccount(i_account);
}
bool Eqonomize::editAccount(Account *i_account) {
	return editAccount(i_account, this);
}
bool Eqonomize::editAccount(Account *i_account, QWidget *parent) {
	QTreeWidgetItem *i = item_accounts[i_account];
	switch(i_account->type()) {
		case ACCOUNT_TYPE_ASSETS: {
			EditAssetsAccountDialog *dialog = new EditAssetsAccountDialog(budget, parent, tr("Edit Account"));
			AssetsAccount *account = (AssetsAccount*) i_account;
			dialog->setAccount(account);
			double prev_ib = account->initialBalance();
			bool prev_debt = (i->parent() == liabilitiesItem);
			Account *previous_budget_account = budget->budgetAccount;
			Currency *cur = account->currency();
			budget->resetDefaultCurrencyChanged();
			budget->resetCurrenciesModified();
			if(dialog->exec() == QDialog::Accepted) {
				dialog->modifyAccount(account);
				bool currency_changed = account->currency() != cur;
				budget->accountModified(account);
				if(previous_budget_account != budget->budgetAccount) {
					if(account->isBudgetAccount() && previous_budget_account) {
						item_accounts[previous_budget_account]->setText(0, previous_budget_account->name());
					} else {
						i->setText(0, account->name());
					}
				}
				account_value[account] += (account->initialBalance() - prev_ib);
				bool is_debt = IS_DEBT(account);
				if(account->isBudgetAccount() && to_date > QDate::currentDate()) i->setText(0, account->name() + "*");
				else i->setText(0, account->name());
				if(is_debt != prev_debt) {
					if(is_debt) {
						assetsItem->removeChild(i);
						liabilitiesItem->addChild(i);
					} else {
						liabilitiesItem->removeChild(i);
						assetsItem->addChild(i);
					}
					if(budget->currenciesModified() || budget->defaultCurrencyChanged()) currenciesModified();
					else filterAccounts();
				} else if(budget->currenciesModified() || budget->defaultCurrencyChanged()) {
					currenciesModified();
				} else if(previous_budget_account != budget->budgetAccount || currency_changed) {
					filterAccounts();
				} else if(is_debt) {
					liabilities_accounts_value += account->currency()->convertTo(account->initialBalance() - prev_ib, budget->defaultCurrency(), to_date);
					i->setText(VALUE_COLUMN, account->currency()->formatValue(-account_value[account]) + " ");
					liabilitiesItem->setText(VALUE_COLUMN, budget->formatMoney(-liabilities_accounts_value) + " ");
				} else {
					assets_accounts_value += account->currency()->convertTo(account->initialBalance() - prev_ib, budget->defaultCurrency(), to_date);
					i->setText(VALUE_COLUMN, account->currency()->formatValue(account_value[account]) + " ");
					assetsItem->setText(VALUE_COLUMN, budget->formatMoney(assets_accounts_value) + " ");
				}
				i->setHidden(account->isClosed() && is_zero(account_value[account]) && is_zero(account_change[account]));
				emit accountsModified();
				setModified(true);
				if(!budget->currenciesModified() && !budget->defaultCurrencyChanged()) {
					expensesWidget->updateFromAccounts();
					incomesWidget->updateToAccounts();
					transfersWidget->updateAccounts();
					expensesWidget->filterTransactions();
					incomesWidget->filterTransactions();
					transfersWidget->filterTransactions();
					updateScheduledTransactions();
					updateSecurities();
				}
				assetsItem->sortChildren(0, Qt::AscendingOrder);
				liabilitiesItem->sortChildren(0, Qt::AscendingOrder);
				dialog->deleteLater();
				updateUsesMultipleCurrencies();
				return true;
			} else if(budget->currenciesModified() || budget->defaultCurrencyChanged()) {
				currenciesModified();
			}
			dialog->deleteLater();
			break;
		}
		case ACCOUNT_TYPE_INCOMES: {
			EditIncomesAccountDialog *dialog = new EditIncomesAccountDialog(budget, NULL, parent, tr("Edit Income Category"));
			IncomesAccount *account = (IncomesAccount*) i_account;
			dialog->setAccount(account);
			CategoryAccount *prev_parent = account->parentCategory();
			if(dialog->exec() == QDialog::Accepted) {
				dialog->modifyAccount(account);
				budget->accountModified(account);
				i->setText(0, account->name());
				emit accountsModified();
				setModified(true);
				incomesWidget->updateFromAccounts();
				if(prev_parent != account->parentCategory()) {
					QTreeWidgetItem *prev_parent_item = i->parent();
					item_accounts.remove(account);
					account_items.remove(i);
					delete i;
					QTreeWidgetItem *parent_item = incomesItem;
					if(account->parentCategory()) parent_item = item_accounts[account->parentCategory()];
					NEW_ACCOUNT_TREE_WIDGET_ITEM(i, parent_item, account->name(), "-", budget->formatMoney(account_change[account]), budget->formatMoney(account_value[account]) + " ");
					if(account->parentCategory()) {
						i->setFlags(i->flags() & ~Qt::ItemIsDropEnabled);
						parent_item->setFlags(parent_item->flags() & ~Qt::ItemIsDragEnabled);
					}
					if(prev_parent_item->childCount() == 0) prev_parent_item->setFlags(prev_parent_item->flags() | Qt::ItemIsDragEnabled);
					account_items[i] = account;
					item_accounts[account] = i;
					parent_item->sortChildren(0, Qt::AscendingOrder);
					filterAccounts();
				}
				incomesItem->sortChildren(0, Qt::AscendingOrder);
				incomesWidget->filterTransactions();
				updateScheduledTransactions();
				dialog->deleteLater();
				return true;
			}
			dialog->deleteLater();
			break;
		}
		case ACCOUNT_TYPE_EXPENSES: {
			EditExpensesAccountDialog *dialog = new EditExpensesAccountDialog(budget, NULL, parent, tr("Edit Expense Category"));
			ExpensesAccount *account = (ExpensesAccount*) i_account;
			dialog->setAccount(account);
			CategoryAccount *prev_parent = account->parentCategory();
			if(dialog->exec() == QDialog::Accepted) {
				dialog->modifyAccount(account);
				budget->accountModified(account);
				i->setText(0, account->name());
				emit accountsModified();
				setModified(true);
				expensesWidget->updateToAccounts();
				if(prev_parent != account->parentCategory()) {
					QTreeWidgetItem *prev_parent_item = i->parent();
					item_accounts.remove(account);
					account_items.remove(i);
					delete i;
					QTreeWidgetItem *parent_item = expensesItem;
					if(account->parentCategory()) parent_item = item_accounts[account->parentCategory()];
					NEW_ACCOUNT_TREE_WIDGET_ITEM(i, parent_item, account->name(), "-", budget->formatMoney(account_change[account]), budget->formatMoney(account_value[account]) + " ");
					if(account->parentCategory()) {
						i->setFlags(i->flags() & ~Qt::ItemIsDropEnabled);
						parent_item->setFlags(parent_item->flags() & ~Qt::ItemIsDragEnabled);
					}
					if(prev_parent_item->childCount() == 0) prev_parent_item->setFlags(prev_parent_item->flags() | Qt::ItemIsDragEnabled);
					account_items[i] = account;
					item_accounts[account] = i;
					parent_item->sortChildren(0, Qt::AscendingOrder);
					filterAccounts();
				}
				expensesItem->sortChildren(0, Qt::AscendingOrder);
				expensesWidget->filterTransactions();
				updateScheduledTransactions();
				dialog->deleteLater();
				return true;
			}
			dialog->deleteLater();
			break;
		}
	}
	return false;
}
void Eqonomize::accountMoved(QTreeWidgetItem *i, QTreeWidgetItem *target) {
	QMap<QTreeWidgetItem*, Account*>::const_iterator it_i = account_items.find(i);
	if(it_i == account_items.end()) return;
	Account *account = *it_i, *target_account = NULL;
	if(target != incomesItem && target != expensesItem) {
		QMap<QTreeWidgetItem*, Account*>::const_iterator it_target = account_items.find(target);
		if(it_target == account_items.end()) return;
		target_account = *it_target;
		if(target_account->type() != account->type()) return;
	}
	if(account->type() == ACCOUNT_TYPE_EXPENSES || account->type() == ACCOUNT_TYPE_INCOMES) {
		if(target == incomesItem && account->type() == ACCOUNT_TYPE_EXPENSES) return;
		if(target == expensesItem && account->type() == ACCOUNT_TYPE_INCOMES) return;
		CategoryAccount *ca = (CategoryAccount*) account;
		CategoryAccount *target_ca = (CategoryAccount*) target_account;
		if(target_ca && target_ca->parentCategory()) target_ca = target_ca->parentCategory();
		if(!ca->subCategories.isEmpty()) return;
		if(ca->parentCategory() == target_ca) return;
		QTreeWidgetItem *prev_parent_item = i->parent();
		ca->setParentCategory(target_ca);
		budget->accountModified(account);
		emit accountsModified();
		setModified(true);
		if(account->type() == ACCOUNT_TYPE_EXPENSES) expensesWidget->updateToAccounts();
		else incomesWidget->updateToAccounts();
		item_accounts.remove(ca);
		account_items.remove(i);
		delete i;
		QTreeWidgetItem *parent_item = NULL;
		if(ca->parentCategory()) parent_item = item_accounts[ca->parentCategory()];
		else if(account->type() == ACCOUNT_TYPE_EXPENSES) parent_item = expensesItem;
		else parent_item = incomesItem;
		NEW_ACCOUNT_TREE_WIDGET_ITEM(i, parent_item, ca->name(), "-", budget->formatMoney(account_change[ca]), budget->formatMoney(account_value[ca]) + " ");
		if(ca->parentCategory()) {
			i->setFlags(i->flags() & ~Qt::ItemIsDropEnabled);
			parent_item->setFlags(parent_item->flags() & ~Qt::ItemIsDragEnabled);
		} else {
			i->setFlags(i->flags() | Qt::ItemIsDropEnabled);
		}
		if(prev_parent_item->childCount() == 0) prev_parent_item->setFlags(prev_parent_item->flags() | Qt::ItemIsDragEnabled);
		account_items[i] = ca;
		item_accounts[ca] = i;
		parent_item->sortChildren(0, Qt::AscendingOrder);
		filterAccounts();
	}
}
void Eqonomize::deleteAccount() {
	QTreeWidgetItem *i = selectedItem(accountsView);
	if(!account_items.contains(i)) return;
	Account *account = account_items[i];
	if((account->type() == ACCOUNT_TYPE_INCOMES || account->type() == ACCOUNT_TYPE_EXPENSES) && !((CategoryAccount*) account)->subCategories.isEmpty()) {
		if(QMessageBox::question(this, tr("Remove subcategories?"), tr("Do you wish to remove the category including all subcategories?"), QMessageBox::Yes | QMessageBox::Cancel) != QMessageBox::Yes) return;
	}
	if(!budget->accountHasTransactions(account)) {
		if((account->type() == ACCOUNT_TYPE_INCOMES || account->type() == ACCOUNT_TYPE_EXPENSES) && !((CategoryAccount*) account)->subCategories.isEmpty()) {
			for(AccountList<CategoryAccount*>::const_iterator it = ((CategoryAccount*) account)->subCategories.constBegin(); it != ((CategoryAccount*) account)->subCategories.constEnd(); ++it) {
				CategoryAccount *ca = *it;
				QTreeWidgetItem *ca_i = item_accounts[ca];
				item_accounts.remove(ca);
				account_items.remove(ca_i);
			}
		}
		item_accounts.remove(account);
		account_items.remove(i);
		delete i;
		account_change.remove(account);
		account_value.remove(account);
		budget->removeAccount(account);
		expensesWidget->updateAccounts();
		transfersWidget->updateAccounts();
		incomesWidget->updateAccounts();
		filterAccounts();
		updateUsesMultipleCurrencies();
		emit accountsModified();
		setModified(true);
	} else {
		QRadioButton *deleteButton = NULL, *moveToButton = NULL;
		QComboBox *moveToCombo = NULL;
		QButtonGroup *group = NULL;
		QDialog *dialog = NULL;
		bool accounts_left = false;
		QVector<Account*> moveto_accounts;
		switch(account->type()) {
			case ACCOUNT_TYPE_EXPENSES: {accounts_left = budget->expensesAccounts.count() > 1; break;}
			case ACCOUNT_TYPE_INCOMES: {accounts_left = budget->incomesAccounts.count() > 1; break;}
			case ACCOUNT_TYPE_ASSETS: {
				for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
					AssetsAccount *aaccount = *it;
					if(aaccount != budget->balancingAccount && aaccount != account && ((((AssetsAccount*) account)->accountType() == ASSETS_TYPE_SECURITIES) == (aaccount->accountType() == ASSETS_TYPE_SECURITIES))) {
						accounts_left = true;
						break;
					}
				}
				break;
			}
			default: {break;}
		}
		if(accounts_left) {
			dialog = new QDialog(this, 0);
			dialog->setWindowTitle(tr("Move transactions?"));
			dialog->setModal(true);
			QVBoxLayout *box1 = new QVBoxLayout(dialog);
			QGridLayout *grid = new QGridLayout();
			grid->setSpacing(12);
			grid->setMargin(6);
			box1->addLayout(grid);
			group = new QButtonGroup(dialog);
			QLabel *label = NULL;
			moveToButton = new QRadioButton(tr("Move to:"), dialog);
			group->addButton(moveToButton);
			deleteButton = new QRadioButton(tr("Remove irreversibly from all accounts\n(do not do this if account has been closed!)"), dialog);
			group->addButton(deleteButton);
			moveToButton->setChecked(true);
			moveToCombo = new QComboBox(dialog);
			moveToCombo->setEditable(false);
			switch(account->type()) {
				case ACCOUNT_TYPE_EXPENSES: {
					label = new QLabel(tr("The category contains some expenses.\nWhat do you want to do with them?"), dialog);
					for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
						ExpensesAccount *eaccount = *it;
						if(eaccount != account) {
							moveToCombo->addItem(eaccount->name());
							moveto_accounts.push_back(eaccount);
						}
					}
					break;
				}
				case ACCOUNT_TYPE_INCOMES: {
					label = new QLabel(tr("The category contains some incomes.\nWhat do you want to do with them?"), dialog);
					for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
						IncomesAccount *iaccount = *it;
						if(iaccount != account) {
							moveToCombo->addItem(iaccount->name());
							moveto_accounts.push_back(iaccount);
						}
					}
					break;
				}
				case ACCOUNT_TYPE_ASSETS: {
					label = new QLabel(tr("The account contains some transactions.\nWhat do you want to do with them?"), dialog);
					for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
						AssetsAccount *aaccount = *it;
						if(aaccount != budget->balancingAccount && aaccount != account && ((((AssetsAccount*) account)->accountType() == ASSETS_TYPE_SECURITIES) == (aaccount->accountType() == ASSETS_TYPE_SECURITIES))) {
							moveToCombo->addItem(aaccount->name());
							moveto_accounts.push_back(aaccount);
						}
					}
					break;
				}
				default: {break;}
			}
			grid->addWidget(label, 0, 0, 1, 2);
			grid->addWidget(moveToButton, 1, 0);
			grid->addWidget(moveToCombo, 1, 1);
			grid->addWidget(deleteButton, 2, 0, 1, 2);
			QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
			buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
			buttonBox->button(QDialogButtonBox::Cancel)->setDefault(true);
			connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), dialog, SLOT(reject()));
			connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(accept()));
			box1->addWidget(buttonBox);
		}
		bool do_delete = false;
		if(accounts_left) {
			do_delete = (dialog->exec() == QDialog::Accepted);
		} else {
			switch(account->type()) {
				case ACCOUNT_TYPE_EXPENSES: {do_delete = (QMessageBox::warning(this, tr("Remove Category?"), tr("The category contains some expenses that will be removed. Do you still want to remove the category?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes); break;}
				case ACCOUNT_TYPE_INCOMES: {do_delete = (QMessageBox::warning(this, tr("Remove Category?"), tr("The category contains some incomes that will be removed. Do you still want to remove the category?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes); break;}
				case ACCOUNT_TYPE_ASSETS: {do_delete = (QMessageBox::warning(this, tr("Remove Account?"), tr("The account contains some transactions that will be removed. Do you still want to remove the account?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes); break;}
			}
		}
		if(do_delete) {
			if(accounts_left && moveToButton->isChecked()) {
				budget->moveTransactions(account, moveto_accounts[moveToCombo->currentIndex()]);
			}
			if((account->type() == ACCOUNT_TYPE_INCOMES || account->type() == ACCOUNT_TYPE_EXPENSES) && !((CategoryAccount*) account)->subCategories.isEmpty()) {
				for(AccountList<CategoryAccount*>::const_iterator it = ((CategoryAccount*) account)->subCategories.constBegin(); it != ((CategoryAccount*) account)->subCategories.constEnd(); ++it) {
					CategoryAccount *ca = *it;
					QTreeWidgetItem *ca_i = item_accounts[ca];
					item_accounts.remove(ca);
					account_items.remove(ca_i);
				}
			}
			if(account->topAccount() != account && i->parent()->childCount() == 0) i->parent()->setFlags(i->parent()->flags() | Qt::ItemIsDragEnabled);
			item_accounts.remove(account);
			account_items.remove(i);
			delete i;
			account_change.remove(account);
			account_value.remove(account);
			AccountType type = account->type();
			budget->removeAccount(account);
			filterAccounts();
			updateSecurities();
			updateScheduledTransactions();
			switch(type) {
				case ACCOUNT_TYPE_EXPENSES: {expensesWidget->filterTransactions(); break;}
				case ACCOUNT_TYPE_INCOMES: {incomesWidget->filterTransactions(); break;}
				case ACCOUNT_TYPE_ASSETS: {incomesWidget->filterTransactions(); expensesWidget->filterTransactions(); transfersWidget->filterTransactions(); break;}
			}
			expensesWidget->updateAccounts();
			transfersWidget->updateAccounts();
			incomesWidget->updateAccounts();
			emit accountsModified();
			emit transactionsModified();
			setModified(true);
			updateUsesMultipleCurrencies();
		}
		if(dialog) dialog->deleteLater();
	}
}

void Eqonomize::closeAccount() {
	QTreeWidgetItem *i = selectedItem(accountsView);
	if(!account_items.contains(i)) return;
	Account *acc = account_items[i];
	if(acc->type() == ACCOUNT_TYPE_ASSETS) {
		AssetsAccount *account = (AssetsAccount*) acc;
		bool b = !account->isClosed();
		account->setClosed(b);
		i->setHidden(account->isClosed() && is_zero(account_value[account]) && is_zero(account_change[account]));
		emit accountsModified();
		expensesWidget->updateFromAccounts();
		incomesWidget->updateToAccounts();
		transfersWidget->updateAccounts();
		setModified(true);
		if(b) {
			ActionCloseAccount->setText(tr("Reopen Account", "Mark account as not closed"));
			ActionCloseAccount->setIcon(LOAD_ICON("edit-undo"));
		} else {
			ActionCloseAccount->setText(tr("Close Account", "Mark account as closed"));
			ActionCloseAccount->setIcon(LOAD_ICON("edit-delete"));
		}
	}
}

void Eqonomize::transactionAdded(Transactions *transs) {
	setModified(true);
	switch(transs->generaltype()) {
		case GENERAL_TRANSACTION_TYPE_SINGLE: {
			Transaction *trans = (Transaction*) transs;
			addTransactionValue(trans, trans->date(), true);
			if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY || trans->type() == TRANSACTION_TYPE_SECURITY_SELL) {
				updateSecurity(((SecurityTransaction*) trans)->security());
			} else if(trans->type() == TRANSACTION_TYPE_INCOME && ((Income*) trans)->security()) {
				updateSecurity(((Income*) trans)->security());
			}
			break;
		}
		case GENERAL_TRANSACTION_TYPE_SPLIT: {
			SplitTransaction *split = (SplitTransaction*) transs;
			int c = split->count();
			for(int i = 0; i < c; i++) {
				Transaction *trans = split->at(i);
				addTransactionValue(trans, trans->date(), true);
				expensesWidget->onTransactionAdded(trans);
				incomesWidget->onTransactionAdded(trans);
				transfersWidget->onTransactionAdded(trans);
				if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY || trans->type() == TRANSACTION_TYPE_SECURITY_SELL) {
					updateSecurity(((SecurityTransaction*) trans)->security());
				} else if(trans->type() == TRANSACTION_TYPE_INCOME && ((Income*) trans)->security()) {
					updateSecurity(((Income*) trans)->security());
				}
			}
			break;
		}
		case GENERAL_TRANSACTION_TYPE_SCHEDULE: {
			ScheduledTransaction *strans = (ScheduledTransaction*) transs;
			appendScheduledTransaction(strans);
			addScheduledTransactionValue(strans, true);
			if(strans->transactiontype() == TRANSACTION_TYPE_SECURITY_BUY || strans->transactiontype() == TRANSACTION_TYPE_SECURITY_SELL) {
				updateSecurity(((SecurityTransaction*) strans->transaction())->security());
			} else if(strans->transactiontype() == TRANSACTION_TYPE_INCOME && ((Income*) strans->transaction())->security()) {
				updateSecurity(((Income*) strans->transaction())->security());
			}
			if(strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT) {
				SplitTransaction *split = (SplitTransaction*) strans->transaction();
				int c = split->count();
				for(int i = 0; i < c; i++) {
					Transaction *trans = split->at(i);
					if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY || trans->type() == TRANSACTION_TYPE_SECURITY_SELL) {
						updateSecurity(((SecurityTransaction*) trans)->security());
					} else if(trans->type() == TRANSACTION_TYPE_INCOME && ((Income*) trans)->security()) {
						updateSecurity(((Income*) trans)->security());
					}
				}
			}
			break;
		}
	}
	if(!in_batch_edit) emit transactionsModified();
	expensesWidget->onTransactionAdded(transs);
	incomesWidget->onTransactionAdded(transs);
	transfersWidget->onTransactionAdded(transs);
}
void Eqonomize::transactionModified(Transactions *transs, Transactions *oldtranss) {
	setModified(true);
	switch(transs->generaltype()) {
		case GENERAL_TRANSACTION_TYPE_SINGLE: {
			Transaction *trans = (Transaction*) transs;
			Transaction *oldtrans = (Transaction*) oldtranss;
			subtractTransactionValue(oldtrans, true);
			addTransactionValue(trans, trans->date(), true);
			if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY || trans->type() == TRANSACTION_TYPE_SECURITY_SELL) {
				updateSecurity(((SecurityTransaction*) trans)->security());
			} else if(trans->type() == TRANSACTION_TYPE_INCOME && ((Income*) trans)->security()) {
				updateSecurity(((Income*) trans)->security());
			}
			if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY || trans->type() == TRANSACTION_TYPE_SECURITY_SELL) {
				updateSecurity(((SecurityTransaction*) trans)->security());
				if(((SecurityTransaction*) trans)->security() != ((SecurityTransaction*) oldtrans)->security()) {
					updateSecurity(((SecurityTransaction*) oldtrans)->security());
				}
			} else if(trans->type() == TRANSACTION_TYPE_INCOME && ((Income*) trans)->security()) {
				updateSecurity(((Income*) trans)->security());
				if(((Income*) trans)->security() != ((Income*) oldtrans)->security()) {
					updateSecurity(((Income*) oldtrans)->security());
				}
			}
			break;
		}
		case GENERAL_TRANSACTION_TYPE_SPLIT: {
			return;
		}
		case GENERAL_TRANSACTION_TYPE_SCHEDULE: {
			ScheduledTransaction *strans = (ScheduledTransaction*) transs;
			ScheduledTransaction *oldstrans = (ScheduledTransaction*) oldtranss;
			if(strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT) {
				return;
			}
			QTreeWidgetItemIterator it(scheduleView);
			ScheduleListViewItem *i = (ScheduleListViewItem*) *it;
			while(i) {
				if(i->scheduledTransaction() == strans) {
					i->setScheduledTransaction(strans);
					i->setDate(strans->firstOccurrence());
					break;
				}
				++it;
				i = (ScheduleListViewItem*) *it;
			}
			subtractScheduledTransactionValue(oldstrans, true);
			addScheduledTransactionValue(strans, true);
			if(strans->transactiontype() == TRANSACTION_TYPE_SECURITY_BUY || strans->transactiontype() == TRANSACTION_TYPE_SECURITY_SELL) {
				updateSecurity(((SecurityTransaction*) strans->transaction())->security());
				if(((SecurityTransaction*) strans->transaction())->security() != ((SecurityTransaction*) oldstrans->transaction())->security()) {
					updateSecurity(((SecurityTransaction*) oldstrans->transaction())->security());
				}
			} else if(strans->transactiontype() == TRANSACTION_TYPE_INCOME && ((Income*) strans->transaction())->security()) {
				updateSecurity(((Income*) strans->transaction())->security());
				if(((Income*) strans->transaction())->security() != ((Income*) oldstrans->transaction())->security()) {
					updateSecurity(((Income*) oldstrans->transaction())->security());
				}
			}
			break;
		}
	}
	if(!in_batch_edit) emit transactionsModified();
	expensesWidget->onTransactionModified(transs, oldtranss);
	incomesWidget->onTransactionModified(transs, oldtranss);
	transfersWidget->onTransactionModified(transs, oldtranss);
}
void Eqonomize::transactionRemoved(Transactions *transs) {
	setModified(true);
	switch(transs->generaltype()) {
		case GENERAL_TRANSACTION_TYPE_SINGLE: {
			Transaction *trans = (Transaction*) transs;
			subtractTransactionValue(trans, true);
			if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY || trans->type() == TRANSACTION_TYPE_SECURITY_SELL) {
				updateSecurity(((SecurityTransaction*) trans)->security());
			} else if(trans->type() == TRANSACTION_TYPE_INCOME && ((Income*) trans)->security()) {
				updateSecurity(((Income*) trans)->security());
			}
			break;
		}
		case GENERAL_TRANSACTION_TYPE_SPLIT: {
			SplitTransaction *split = (SplitTransaction*) transs;
			int c = split->count();
			for(int i = 0; i < c; i++) {
				Transaction *trans = split->at(i);
				subtractTransactionValue(trans, true);
				expensesWidget->onTransactionRemoved(trans);
				incomesWidget->onTransactionRemoved(trans);
				transfersWidget->onTransactionRemoved(trans);
				if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY || trans->type() == TRANSACTION_TYPE_SECURITY_SELL) {
					updateSecurity(((SecurityTransaction*) trans)->security());
				} else if(trans->type() == TRANSACTION_TYPE_INCOME && ((Income*) trans)->security()) {
					updateSecurity(((Income*) trans)->security());
				}
			}
			break;
		}
		case GENERAL_TRANSACTION_TYPE_SCHEDULE: {
			ScheduledTransaction *strans = (ScheduledTransaction*) transs;
			QTreeWidgetItemIterator it(scheduleView);
			ScheduleListViewItem *i = (ScheduleListViewItem*) *it;
			while(i) {
				if(i->scheduledTransaction() == strans) {
					delete i;
					break;
				}
				++it;
				i = (ScheduleListViewItem*) *it;
			}
			subtractScheduledTransactionValue(strans, true);
			if(strans->transactiontype() == TRANSACTION_TYPE_SECURITY_BUY || strans->transactiontype() == TRANSACTION_TYPE_SECURITY_SELL) {
				updateSecurity(((SecurityTransaction*) strans->transaction())->security());
			} else if(strans->transactiontype() == TRANSACTION_TYPE_INCOME && ((Income*) strans->transaction())->security()) {
				updateSecurity(((Income*) strans->transaction())->security());
			}
			if(strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT) {
				SplitTransaction *split = (SplitTransaction*) strans->transaction();
				int c = split->count();
				for(int i = 0; i < c; i++) {
					Transaction *trans = split->at(i);
					if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY || trans->type() == TRANSACTION_TYPE_SECURITY_SELL) {
						updateSecurity(((SecurityTransaction*) trans)->security());
					} else if(trans->type() == TRANSACTION_TYPE_INCOME && ((Income*) trans)->security()) {
						updateSecurity(((Income*) trans)->security());
					}
				}
			}
			break;
		}
	}
	if(!in_batch_edit) emit transactionsModified();
	expensesWidget->onTransactionRemoved(transs);
	incomesWidget->onTransactionRemoved(transs);
	transfersWidget->onTransactionRemoved(transs);
}

void Eqonomize::appendExpensesAccount(ExpensesAccount *account, QTreeWidgetItem *parent_item) {
	NEW_ACCOUNT_TREE_WIDGET_ITEM(i, parent_item, account->name(), "-", budget->formatMoney(0.0), budget->formatMoney(0.0) + " ");
	if(!account->subCategories.isEmpty()) i->setFlags(i->flags() & ~Qt::ItemIsDragEnabled);
	if(account->parentCategory()) {
		i->setFlags(i->flags() & ~Qt::ItemIsDropEnabled);
		parent_item->setFlags(parent_item->flags() & ~Qt::ItemIsDragEnabled);
	}
	account_items[i] = account;
	item_accounts[account] = i;
	account_value[account] = 0.0;
	account_change[account] = 0.0;
	parent_item->sortChildren(0, Qt::AscendingOrder);
	for(AccountList<CategoryAccount*>::const_iterator it = account->subCategories.constBegin(); it != account->subCategories.constEnd(); ++it) {
		ExpensesAccount *ea = (ExpensesAccount*) *it;
		appendExpensesAccount(ea, i);
	}
}
void Eqonomize::appendIncomesAccount(IncomesAccount *account, QTreeWidgetItem *parent_item) {
	NEW_ACCOUNT_TREE_WIDGET_ITEM(i, parent_item, account->name(), "-", budget->formatMoney(0.0), budget->formatMoney(0.0) + " ");
	if(!account->subCategories.isEmpty()) i->setFlags(i->flags() & ~Qt::ItemIsDragEnabled);
	if(account->parentCategory()) {
		i->setFlags(i->flags() & ~Qt::ItemIsDropEnabled);
		parent_item->setFlags(parent_item->flags() & ~Qt::ItemIsDragEnabled);
	}
	account_items[i] = account;
	item_accounts[account] = i;
	account_value[account] = 0.0;
	account_change[account] = 0.0;
	parent_item->sortChildren(0, Qt::AscendingOrder);
	for(AccountList<CategoryAccount*>::const_iterator it = account->subCategories.constBegin(); it != account->subCategories.constEnd(); ++it) {
		IncomesAccount *ia = (IncomesAccount*) *it;
		appendIncomesAccount(ia, i);
	}
}
void Eqonomize::appendAssetsAccount(AssetsAccount *account) {
	bool is_debt = IS_DEBT(account);
	double initial_balance = account->initialBalance();	
	NEW_ACCOUNT_TREE_WIDGET_ITEM(i, is_debt ? liabilitiesItem : assetsItem, account->name(), QString::null, account->currency()->formatValue(0.0), account->currency()->formatValue((is_debt ? -initial_balance : initial_balance)) + " ");
	if(account->isBudgetAccount() && to_date > QDate::currentDate()) i->setText(0, account->name() + "*");
	i->setFlags(i->flags() & ~Qt::ItemIsDragEnabled);
	i->setFlags(i->flags() & ~Qt::ItemIsDropEnabled);
	account_items[i] = account;
	item_accounts[account] = i;
	account_value[account] = initial_balance;
	if(is_debt) {
		liabilities_accounts_value += account->currency()->convertTo(initial_balance, budget->defaultCurrency(), to_date);
		liabilitiesItem->setText(VALUE_COLUMN, budget->formatMoney(assets_accounts_value) + " ");
		liabilitiesItem->sortChildren(0, Qt::AscendingOrder);
	} else {
		assets_accounts_value += account->currency()->convertTo(initial_balance, budget->defaultCurrency(), to_date);
		assetsItem->setText(VALUE_COLUMN, budget->formatMoney(assets_accounts_value) + " ");
		assetsItem->sortChildren(0, Qt::AscendingOrder);
	}
	account_change[account] = 0.0;
	if(account->isClosed() && is_zero(initial_balance)) i->setHidden(true);
}
void Eqonomize::appendLoanAccount(LoanAccount*) {}

bool Eqonomize::filterTransaction(Transaction *trans) {
	if(accountsPeriodFromButton->isChecked() && trans->date() < from_date) return true;
	if(trans->date() > to_date) return true;
	return false;
}
void Eqonomize::subtractScheduledTransactionValue(ScheduledTransaction *strans, bool update_value_display) {
	addScheduledTransactionValue(strans, update_value_display, true);
}
void Eqonomize::addScheduledTransactionValue(ScheduledTransaction *strans, bool update_value_display, bool subtract) {
	if(!strans->recurrence()) {
		if(strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT) {
			SplitTransaction *split = (SplitTransaction*) strans->transaction();
			int c = split->count();
			for(int i = 0; i < c; i++) {
				addTransactionValue(split->at(i), strans->date(), update_value_display, subtract, -1, -1, NULL);
			}
			return;
		}
		return addTransactionValue((Transaction*) strans->transaction(), strans->transaction()->date(), update_value_display, subtract, -1, -1, NULL);
	}
	Recurrence *rec = strans->recurrence();
	QDate curdate = rec->firstOccurrence();
	int b_future = 1;
	if(to_date <= QDate::currentDate()) b_future = 0;
	else if(strans->transaction()->date() <= QDate::currentDate()) b_future = -1;
	while(!curdate.isNull() && curdate <= to_date) {
		if(strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT) {
			SplitTransaction *split = (SplitTransaction*) strans->transaction();
			int c = split->count();
			for(int i = 0; i < c; i++) {
				addTransactionValue(split->at(i), curdate, update_value_display, subtract, 1, b_future, NULL);
			}
		} else {
			addTransactionValue((Transaction*) strans->transaction(), curdate, update_value_display, subtract, 1, b_future, NULL);
		}
		curdate = rec->nextOccurrence(curdate);
	}
}
void Eqonomize::subtractTransactionValue(Transaction *trans, bool update_value_display) {
	addTransactionValue(trans, trans->date(), update_value_display, true);
}
void Eqonomize::addTransactionValue(Transaction *trans, const QDate &transdate, bool update_value_display, bool subtract, int n, int b_future, const QDate *monthdate) {
	if(n == 0) return;
	bool b_filter_to = n < 0 && transdate > to_date;
	bool b_from = accountsPeriodFromButton->isChecked();
	bool b_lastmonth = false;
	QDate date;
	if(b_filter_to) {
		if(!monthdate) {
			if(!budget->isSameBudgetMonth(transdate, to_date)) return;
		} else {
			if(transdate > *monthdate) return;
		}
		b_lastmonth = true;
	}
	bool b_filter =  !b_lastmonth && b_from && transdate < from_date;
	bool b_curmonth = false;
	bool b_firstmonth = false;	
	if(!b_lastmonth && b_future < 0) {
		if(to_date <= QDate::currentDate()) {
			b_future = 0;
		} else {
			b_future = (transdate > QDate::currentDate());
			if(!b_future) {
				QDate curdate = QDate::currentDate();
				if(budget->isSameBudgetMonth(curdate, transdate)) {
					b_curmonth = true;
					b_future = true;
				}
			}
		}
	} else if(!b_lastmonth && b_future) {
		b_curmonth = (transdate <= QDate::currentDate());
	}
	if(b_from && !b_lastmonth && b_filter && !frommonth_begin.isNull()) {
		if(transdate >= frommonth_begin) b_firstmonth = true;
	}
	bool balfrom = false, balto = false;
	bool to_is_debt = false, from_is_debt = false;
	double value = subtract ? -trans->fromValue(false) : trans->fromValue(false);
	double cvalue = trans->fromCurrency()->convertTo(trans->fromValue(false), budget->defaultCurrency(), to_date);
	double cvalue_then = trans->fromValue(true);
	if(subtract) cvalue = -cvalue;
	if(subtract) cvalue_then = -cvalue_then;
	if(!monthdate) {
		date = budget->lastBudgetDay(transdate);
		monthdate = &date;
	}
	if(n > 1) value *= n;
	bool from_sub = (trans->fromAccount() != trans->fromAccount()->topAccount());
	bool to_sub = (trans->toAccount() != trans->toAccount()->topAccount());
	switch(trans->fromAccount()->type()) {
		case ACCOUNT_TYPE_EXPENSES: {
			if(b_lastmonth) {
				account_month_endlast[trans->fromAccount()] -= cvalue_then;
				if(from_sub) account_month_endlast[trans->fromAccount()->topAccount()] -= cvalue_then;
				account_month[trans->fromAccount()][*monthdate] -= cvalue_then;
				if(from_sub) account_month[trans->fromAccount()->topAccount()][*monthdate] -= cvalue_then;
				if(update_value_display) {
					updateMonthlyBudget(trans->fromAccount());
					if(from_sub) updateMonthlyBudget(trans->fromAccount()->topAccount());
					updateTotalMonthlyExpensesBudget();
				}
				break;
			}
			bool update_month_display = false;
			if(b_firstmonth) {
				account_month_beginfirst[trans->fromAccount()] -= cvalue_then;
				if(from_sub) account_month_beginfirst[trans->fromAccount()->topAccount()] -= cvalue_then;
				update_month_display = true;
			}
			if(b_curmonth) {
				account_month_begincur[trans->fromAccount()] -= cvalue_then;
				if(from_sub) account_month_begincur[trans->fromAccount()->topAccount()] -= cvalue_then;
				update_month_display = true;
			}
			if(b_future || (!frommonth_begin.isNull() && transdate >= frommonth_begin) || (!prevmonth_begin.isNull() && transdate >= prevmonth_begin)) {
				account_month[trans->fromAccount()][*monthdate] -= cvalue_then;
				if(from_sub) account_month[trans->fromAccount()->topAccount()][*monthdate] -= cvalue_then;
				update_month_display = true;
			}
			if(update_value_display && update_month_display) {
				updateMonthlyBudget(trans->fromAccount());
				if(from_sub) updateMonthlyBudget(trans->fromAccount()->topAccount());
				updateTotalMonthlyExpensesBudget();
			}
			account_value[trans->fromAccount()] -= cvalue_then;
			if(from_sub) account_value[trans->fromAccount()->topAccount()] -= cvalue_then;
			expenses_accounts_value -= cvalue_then;
			if(!b_filter) {
				account_change[trans->fromAccount()] -= cvalue_then;
				if(from_sub) account_value[trans->fromAccount()->topAccount()] -= cvalue_then;
				expenses_accounts_change -= cvalue_then;
				if(update_value_display) {
					expensesItem->setText(CHANGE_COLUMN, budget->formatMoney(expenses_accounts_change));
					setAccountChangeColor(expensesItem, expenses_accounts_change, true);
				}
			}
			if(update_value_display) {
				expensesItem->setText(VALUE_COLUMN, budget->formatMoney(expenses_accounts_value) + " ");
			}
			break;
		}
		case ACCOUNT_TYPE_INCOMES: {
			if(b_lastmonth) {
				account_month_endlast[trans->fromAccount()] += cvalue_then;
				if(from_sub) account_month_endlast[trans->fromAccount()->topAccount()] += cvalue_then;
				account_month[trans->fromAccount()][*monthdate] += cvalue_then;
				if(from_sub) account_month[trans->fromAccount()->topAccount()][*monthdate] += cvalue_then;
				if(update_value_display) {
					updateMonthlyBudget(trans->fromAccount());
					if(from_sub) updateMonthlyBudget(trans->fromAccount()->topAccount());
					updateTotalMonthlyIncomesBudget();
				}
				break;
			}
			bool update_month_display = false;
			if(b_firstmonth) {
				account_month_beginfirst[trans->fromAccount()] += cvalue_then;
				if(from_sub) account_month_beginfirst[trans->fromAccount()->topAccount()] += cvalue_then;
				update_month_display = true;
			}
			if(b_curmonth) {
				account_month_begincur[trans->fromAccount()] += cvalue_then;
				if(from_sub) account_month_begincur[trans->fromAccount()->topAccount()] += cvalue_then;
				update_month_display = true;
			}
			if(b_future || (!frommonth_begin.isNull() && transdate >= frommonth_begin) || (!prevmonth_begin.isNull() && transdate >= prevmonth_begin)) {
				account_month[trans->fromAccount()][*monthdate] += cvalue_then;
				if(from_sub) account_month[trans->fromAccount()->topAccount()][*monthdate] += cvalue_then;
				update_month_display = true;
			}
			if(update_value_display && update_month_display) {
				updateMonthlyBudget(trans->fromAccount());
				if(from_sub) updateMonthlyBudget(trans->fromAccount()->topAccount());
				updateTotalMonthlyIncomesBudget();
			}
			account_value[trans->fromAccount()] += cvalue_then;
			if(from_sub) account_value[trans->fromAccount()->topAccount()] += cvalue_then;
			incomes_accounts_value += cvalue_then;
			if(!b_filter) {
				account_change[trans->fromAccount()] += cvalue_then;
				if(from_sub) account_change[trans->fromAccount()->topAccount()] += cvalue_then;
				incomes_accounts_change += cvalue_then;
				if(update_value_display) {
					incomesItem->setText(CHANGE_COLUMN, budget->formatMoney(incomes_accounts_change));
					setAccountChangeColor(incomesItem, incomes_accounts_change, false);
				}
			}
			if(update_value_display) {
				incomesItem->setText(VALUE_COLUMN, budget->formatMoney(incomes_accounts_value) + " ");
			}
			break;
		}
		case ACCOUNT_TYPE_ASSETS: {
			if(b_lastmonth) break;
			if(((AssetsAccount*) trans->fromAccount())->accountType() == ASSETS_TYPE_SECURITIES) {
				if(update_value_display) {
					updateSecurityAccount((AssetsAccount*) trans->fromAccount(), false);
					assetsItem->setText(VALUE_COLUMN, budget->formatMoney(assets_accounts_value) + " ");
					assetsItem->setText(CHANGE_COLUMN, budget->formatMoney(assets_accounts_change));
					setAccountChangeColor(assetsItem, assets_accounts_change, false);
					item_accounts[trans->fromAccount()]->setText(CHANGE_COLUMN, trans->fromAccount()->currency()->formatValue(account_change[trans->fromAccount()]));
					setAccountChangeColor(item_accounts[trans->fromAccount()], account_change[trans->fromAccount()], false);
				}
				break;
			}
			balfrom = (trans->fromAccount() == budget->balancingAccount);
			if(!balfrom) {
				from_is_debt = IS_DEBT(((AssetsAccount*) trans->fromAccount()));
				account_value[trans->fromAccount()] -= value;
				if(from_is_debt) liabilities_accounts_value -= cvalue;
				else assets_accounts_value -= cvalue;
				if(!b_filter) {
					account_change[trans->fromAccount()] -= value;
					if(from_is_debt) liabilities_accounts_change -= cvalue;
					else assets_accounts_change -= cvalue;
					if(update_value_display) {
						if(from_is_debt) {
							liabilitiesItem->setText(CHANGE_COLUMN, budget->formatMoney(-liabilities_accounts_change));
							setAccountChangeColor(liabilitiesItem, -liabilities_accounts_change, true);
							item_accounts[trans->fromAccount()]->setText(CHANGE_COLUMN, trans->fromAccount()->currency()->formatValue(-account_change[trans->fromAccount()]));
							setAccountChangeColor(item_accounts[trans->fromAccount()], -account_change[trans->fromAccount()], true);
						} else {
							assetsItem->setText(CHANGE_COLUMN, budget->formatMoney(assets_accounts_change));
							setAccountChangeColor(assetsItem, assets_accounts_change, false);
							item_accounts[trans->fromAccount()]->setText(CHANGE_COLUMN, trans->fromAccount()->currency()->formatValue(account_change[trans->fromAccount()]));
							setAccountChangeColor(item_accounts[trans->fromAccount()], account_change[trans->fromAccount()], false);
						}						
						item_accounts[trans->fromAccount()]->setHidden(trans->fromAccount()->isClosed() && is_zero(account_change[trans->fromAccount()]) && is_zero(account_value[trans->fromAccount()]));
					}
				}
				if(update_value_display) {
					if(from_is_debt) liabilitiesItem->setText(VALUE_COLUMN, budget->formatMoney(-liabilities_accounts_value) + " ");
					else assetsItem->setText(VALUE_COLUMN, budget->formatMoney(assets_accounts_value) + " ");
				}
			}
			break;
		}
	}
	value = subtract ? -trans->toValue(false) : trans->toValue(false);
	cvalue = trans->toCurrency()->convertTo(trans->toValue(false), budget->defaultCurrency(), to_date);
	cvalue_then = trans->toValue(true);
	if(subtract) cvalue = -cvalue;
	if(subtract) cvalue_then = -cvalue_then;
	switch(trans->toAccount()->type()) {
		case ACCOUNT_TYPE_EXPENSES: {
			if(b_lastmonth) {
				account_month_endlast[trans->toAccount()] += cvalue_then;
				if(to_sub) account_month_endlast[trans->toAccount()->topAccount()] += cvalue_then;
				account_month[trans->toAccount()][*monthdate] += cvalue_then;
				if(to_sub) account_month[trans->toAccount()->topAccount()][*monthdate] += cvalue_then;
				if(update_value_display) {
					updateMonthlyBudget(trans->toAccount());
					if(to_sub) updateMonthlyBudget(trans->toAccount()->topAccount());
					updateTotalMonthlyExpensesBudget();
				}
				break;
			}
			bool update_month_display = false;
			if(b_firstmonth) {
				account_month_beginfirst[trans->toAccount()] += cvalue_then;
				if(to_sub) account_month_beginfirst[trans->toAccount()->topAccount()] += cvalue_then;
				update_month_display = true;
			}
			if(b_curmonth) {
				account_month_begincur[trans->toAccount()] += cvalue_then;
				if(to_sub) account_month_begincur[trans->toAccount()->topAccount()] += cvalue_then;
				update_month_display = true;
			}
			if(b_future || (!frommonth_begin.isNull() && transdate >= frommonth_begin) || (!prevmonth_begin.isNull() && transdate >= prevmonth_begin)) {
				account_month[trans->toAccount()][*monthdate] += cvalue_then;
				if(to_sub) account_month[trans->toAccount()->topAccount()][*monthdate] += cvalue_then;
				update_month_display = true;
			}
			if(update_value_display && update_month_display) {
				updateMonthlyBudget(trans->toAccount());
				if(to_sub) updateMonthlyBudget(trans->toAccount()->topAccount());
				updateTotalMonthlyExpensesBudget();
			}
			account_value[trans->toAccount()] += cvalue_then;
			if(to_sub) account_value[trans->toAccount()->topAccount()] += cvalue_then;
			expenses_accounts_value += cvalue_then;
			if(!b_filter) {
				account_change[trans->toAccount()] += cvalue_then;
				if(to_sub) account_change[trans->toAccount()->topAccount()] += cvalue_then;
				expenses_accounts_change += cvalue_then;
				if(update_value_display) {
					expensesItem->setText(CHANGE_COLUMN, budget->formatMoney(expenses_accounts_change));
					setAccountChangeColor(expensesItem, expenses_accounts_change, true);
				}
			}
			if(update_value_display) {
				expensesItem->setText(VALUE_COLUMN, budget->formatMoney(expenses_accounts_value) + " ");
			}
			break;
		}
		case ACCOUNT_TYPE_INCOMES: {
			if(b_lastmonth) {
				account_month_endlast[trans->toAccount()] -= cvalue_then;
				if(to_sub) account_month_endlast[trans->toAccount()->topAccount()] -= cvalue_then;
				account_month[trans->toAccount()][*monthdate] -= cvalue_then;
				if(to_sub) account_month[trans->toAccount()->topAccount()][*monthdate] -= cvalue_then;
				if(update_value_display) {
					updateMonthlyBudget(trans->toAccount());
					if(to_sub) updateMonthlyBudget(trans->toAccount()->topAccount());
					updateTotalMonthlyIncomesBudget();
				}
				break;
			}
			bool update_month_display = false;
			if(b_firstmonth) {
				account_month_beginfirst[trans->toAccount()] -= cvalue_then;
				if(to_sub) account_month_beginfirst[trans->toAccount()->topAccount()] -= cvalue_then;
				update_month_display = true;
			}
			if(b_curmonth) {
				account_month_begincur[trans->toAccount()] -= cvalue_then;
				if(to_sub) account_month_begincur[trans->toAccount()->topAccount()] -= cvalue_then;
				update_month_display = true;
			}
			if(b_future || (!frommonth_begin.isNull() && transdate >= frommonth_begin) || (!prevmonth_begin.isNull() && transdate >= prevmonth_begin)) {
				account_month[trans->toAccount()][*monthdate] -= cvalue_then;
				if(to_sub) account_month[trans->toAccount()->topAccount()][*monthdate] -= cvalue_then;
				update_month_display = true;
			}
			if(update_value_display && update_month_display) {
				updateMonthlyBudget(trans->toAccount());
				if(to_sub) updateMonthlyBudget(trans->toAccount()->topAccount());
				updateTotalMonthlyIncomesBudget();
			}
			account_value[trans->toAccount()] -= cvalue_then;
			incomes_accounts_value -= cvalue_then;
			if(!b_filter) {
				account_change[trans->toAccount()] -= cvalue_then;
				if(to_sub) account_change[trans->toAccount()->topAccount()] -= cvalue_then;
				incomes_accounts_change -= value;
				if(update_value_display) {
					incomesItem->setText(CHANGE_COLUMN, budget->formatMoney(incomes_accounts_change));
					setAccountChangeColor(incomesItem, incomes_accounts_change, false);
				}
			}
			if(update_value_display) {
				incomesItem->setText(VALUE_COLUMN, budget->formatMoney(incomes_accounts_value) + " ");
			}
			break;
		}
		case ACCOUNT_TYPE_ASSETS: {
			if(b_lastmonth) break;
			if(((AssetsAccount*) trans->toAccount())->accountType() == ASSETS_TYPE_SECURITIES) {
				if(update_value_display) {
					updateSecurityAccount((AssetsAccount*) trans->toAccount(), false);
					assetsItem->setText(VALUE_COLUMN, budget->formatMoney(assets_accounts_value) + " ");
					assetsItem->setText(CHANGE_COLUMN, budget->formatMoney(assets_accounts_change));
					setAccountChangeColor(assetsItem, assets_accounts_change, false);
					item_accounts[trans->toAccount()]->setText(CHANGE_COLUMN, trans->toAccount()->currency()->formatValue(account_change[trans->toAccount()]));
					setAccountChangeColor(item_accounts[trans->toAccount()], account_change[trans->toAccount()], false);
				}
				break;
			}
			balto = (trans->toAccount() == budget->balancingAccount);
			if(!balto) {
				to_is_debt = IS_DEBT(((AssetsAccount*) trans->toAccount()));
				account_value[trans->toAccount()] += value;
				if(to_is_debt) liabilities_accounts_value += cvalue;
				else assets_accounts_value += cvalue;
				if(!b_filter) {
					account_change[trans->toAccount()] += value;
					if(to_is_debt) liabilities_accounts_change += cvalue;
					else assets_accounts_change += cvalue;
					if(update_value_display) {
						if(to_is_debt) {
							liabilitiesItem->setText(CHANGE_COLUMN, budget->formatMoney(-liabilities_accounts_change));
							setAccountChangeColor(liabilitiesItem, -liabilities_accounts_change, true);
							item_accounts[trans->toAccount()]->setText(CHANGE_COLUMN, trans->toAccount()->currency()->formatValue(-account_change[trans->toAccount()]));
							setAccountChangeColor(item_accounts[trans->toAccount()], -account_change[trans->toAccount()], true);
						} else {
							assetsItem->setText(CHANGE_COLUMN, budget->formatMoney(assets_accounts_change));
							setAccountChangeColor(assetsItem, assets_accounts_change, false);
							item_accounts[trans->toAccount()]->setText(CHANGE_COLUMN, trans->toAccount()->currency()->formatValue(account_change[trans->toAccount()]));
							setAccountChangeColor(item_accounts[trans->toAccount()], account_change[trans->toAccount()], false);
						}
						item_accounts[trans->toAccount()]->setHidden(trans->toAccount()->isClosed() && is_zero(account_change[trans->toAccount()]) && is_zero(account_value[trans->toAccount()]));
					}
				}
				if(update_value_display) {
					if(to_is_debt) liabilitiesItem->setText(VALUE_COLUMN, budget->formatMoney(-liabilities_accounts_value) + " ");
					else assetsItem->setText(VALUE_COLUMN, budget->formatMoney(assets_accounts_value) + " ");
				}
			}
			break;
		}
	}
	if(update_value_display && !b_lastmonth) {
		if(!balfrom) {
			item_accounts[trans->fromAccount()]->setText(VALUE_COLUMN, trans->fromAccount()->currency()->formatValue(from_is_debt ? -account_value[trans->fromAccount()] : account_value[trans->fromAccount()]) + " ");
			item_accounts[trans->fromAccount()]->setText(CHANGE_COLUMN, trans->fromAccount()->currency()->formatValue(from_is_debt ? -account_change[trans->fromAccount()] : account_change[trans->fromAccount()]));
			setAccountChangeColor(item_accounts[trans->fromAccount()], from_is_debt ? -account_change[trans->fromAccount()] : account_change[trans->fromAccount()], from_is_debt || trans->fromAccount()->type() == ACCOUNT_TYPE_EXPENSES);
			item_accounts[trans->fromAccount()]->setHidden(trans->fromAccount()->isClosed() && is_zero(account_change[trans->fromAccount()]) && is_zero(account_value[trans->fromAccount()]));
			if(from_sub) {
				item_accounts[trans->fromAccount()->topAccount()]->setText(VALUE_COLUMN, trans->fromAccount()->topAccount()->currency()->formatValue(account_value[trans->fromAccount()->topAccount()]) + " ");
				item_accounts[trans->fromAccount()->topAccount()]->setText(CHANGE_COLUMN, trans->fromAccount()->topAccount()->currency()->formatValue(account_change[trans->fromAccount()->topAccount()]));
				setAccountChangeColor(item_accounts[trans->fromAccount()->topAccount()], account_change[trans->fromAccount()->topAccount()], trans->fromAccount()->topAccount()->type() == ACCOUNT_TYPE_EXPENSES);
			}
		}
		if(!balto) {
			item_accounts[trans->toAccount()]->setText(VALUE_COLUMN, trans->toAccount()->currency()->formatValue(to_is_debt ? -account_value[trans->toAccount()] : account_value[trans->toAccount()]) + " ");
			item_accounts[trans->toAccount()]->setText(CHANGE_COLUMN, trans->toAccount()->currency()->formatValue(to_is_debt ? -account_change[trans->toAccount()] : account_change[trans->toAccount()]));
			setAccountChangeColor(item_accounts[trans->toAccount()], to_is_debt ? -account_change[trans->toAccount()] : account_change[trans->toAccount()], to_is_debt || trans->toAccount()->type() == ACCOUNT_TYPE_EXPENSES);
			item_accounts[trans->toAccount()]->setHidden(trans->toAccount()->isClosed() && is_zero(account_change[trans->toAccount()]) && is_zero(account_value[trans->toAccount()]));
			if(to_sub) {
				item_accounts[trans->toAccount()->topAccount()]->setText(VALUE_COLUMN, trans->toAccount()->topAccount()->currency()->formatValue(account_value[trans->toAccount()->topAccount()]) + " ");
				item_accounts[trans->toAccount()->topAccount()]->setText(CHANGE_COLUMN, trans->toAccount()->topAccount()->currency()->formatValue(account_change[trans->toAccount()->topAccount()]));
				setAccountChangeColor(item_accounts[trans->toAccount()->topAccount()], account_change[trans->toAccount()->topAccount()], trans->toAccount()->topAccount()->type() == ACCOUNT_TYPE_EXPENSES);
			}
		}
	}
}
void Eqonomize::updateTotalMonthlyExpensesBudget() {
	if(budget->expensesAccounts.count() > 0) {
		expenses_budget = 0.0;
		expenses_budget_diff = 0.0;
		bool b_budget = false;
		for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
			ExpensesAccount *eaccount = *it;
			if(!eaccount->parentCategory()) {
				double d = account_budget[eaccount];
				if(d >= 0.0) {
					expenses_budget += d;
					expenses_budget_diff += account_budget_diff[eaccount];
					b_budget = true;
				}
			}
		}
		if(b_budget) {
			expensesItem->setText(BUDGET_COLUMN, tr("%2 of %1", "%1: budget; %2: remaining budget").arg(budget->formatMoney(expenses_budget)).arg(budget->formatMoney(expenses_budget_diff)));
			setAccountBudgetColor(expensesItem, expenses_budget_diff, true);
			return;
		} else {
			expenses_budget = -1.0;
		}
	}
	expensesItem->setText(BUDGET_COLUMN, "-");
	setAccountBudgetColor(expensesItem, 0.0, true);
}
void Eqonomize::updateTotalMonthlyIncomesBudget() {
	if(budget->incomesAccounts.count() > 0) {
		incomes_budget = 0.0;
		incomes_budget_diff = 0.0;
		bool b_budget = false;
		for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
			IncomesAccount *iaccount = *it;
			double d = account_budget[iaccount];
			if(d >= 0.0) {
				incomes_budget += d;
				incomes_budget_diff += account_budget_diff[iaccount];
				b_budget = true;
			}
		}
		if(b_budget) {
			incomesItem->setText(BUDGET_COLUMN, tr("%2 of %1", "%1: budget; %2: remaining budget").arg(budget->formatMoney(incomes_budget)).arg(budget->formatMoney(incomes_budget_diff)));
			setAccountBudgetColor(incomesItem, incomes_budget_diff, false);
			return;
		} else {
			incomes_budget = -1.0;
		}
	}
	incomesItem->setText(BUDGET_COLUMN, "-");
	setAccountBudgetColor(incomesItem, 0.0, false);
}
struct budget_info {
	QMap<QDate, double>::const_iterator it;
	QMap<QDate, double>::const_iterator it_n;
	QMap<QDate, double>::const_iterator it_e;
	CategoryAccount *ca;
	bool after_from;
};
void Eqonomize::updateMonthlyBudget(Account *account) {

	if(account->type() == ACCOUNT_TYPE_ASSETS) return;
	CategoryAccount *ca = (CategoryAccount*) account;
	double mbudget = 0.0;
	QTreeWidgetItem *i = item_accounts[account];
	QMap<QDate, double>::iterator it = ca->mbudgets.begin();
	QMap<QDate, double>::iterator it_e = ca->mbudgets.end();
	QVector<budget_info> subs;
	while(it != it_e && it.value() < 0.0) ++it;

	QMap<QDate, double>::const_iterator tmp_it;
	QMap<QDate, double>::const_iterator tmp_it_e;
	for(AccountList<CategoryAccount*>::const_iterator it = ca->subCategories.constBegin(); it != ca->subCategories.constEnd(); ++it) {
		CategoryAccount *sub = *it;
		tmp_it = sub->mbudgets.begin();
		tmp_it_e = sub->mbudgets.end();
		while(tmp_it != tmp_it_e && tmp_it.value() < 0.0) ++tmp_it;
		if(tmp_it != tmp_it_e && tmp_it.key() <= to_date) {
			struct budget_info bi;
			bi.it = tmp_it;
			bi.it_e = tmp_it_e;
			bi.ca = sub;
			subs << bi;
		}
	}

	if(subs.isEmpty() && (it == it_e || budget->monthToBudgetMonth(it.key()) > to_date)) {

		i->setText(BUDGET_COLUMN, "-");
		setAccountBudgetColor(i, 0.0, account->type() == ACCOUNT_TYPE_EXPENSES);
		account_budget[account] = -1.0;

	} else {

		bool after_from = true;
		QDate monthdate, monthend, curdate = QDate::currentDate(), curmonth, frommonth;

		frommonth = frommonth_begin;
		if(it != it_e && accountsPeriodFromButton->isChecked() && frommonth > budget->monthToBudgetMonth(it.key())) {
			QMap<QDate, double>::iterator it_b = ca->mbudgets.begin();
			it = it_e;
			--it;
			while(it != it_b) {
				if(frommonth >= budget->monthToBudgetMonth(it.key())) break;
				--it;
			}
			after_from = false;
		}
		for(QVector<budget_info>::iterator sit = subs.begin(); sit != subs.end(); ++sit) {
			if(accountsPeriodFromButton->isChecked() && frommonth > budget->monthToBudgetMonth(sit->it.key())) {
				QMap<QDate, double>::iterator it_b = sit->ca->mbudgets.begin();
				sit->it = sit->it_e;
				--(sit->it);
				while(sit->it != it_b) {
					if(frommonth >= budget->monthToBudgetMonth(sit->it.key())) break;
					--(sit->it);
				}
				after_from = false;
			}
		}
		double diff = 0.0, future_diff = 0.0, future_change_diff = 0.0, m = 0.0, v = 0.0;
		QDate monthlast = budget->firstBudgetDay(to_date);

		QMap<QDate, double>::iterator it_n = it;
		if(it_n != it_e) ++it_n;
		for(QVector<budget_info>::iterator sit = subs.begin(); sit != subs.end(); ++sit) {
			sit->it_n = sit->it;
			if(sit->it_n != sit->it_e) ++(sit->it_n);
		}
		bool has_budget = false, has_subs_budget = false;
		bool b_firstmonth = !after_from && (monthdate != frommonth);
		monthdate = frommonth;

		do {
			if(it != it_e && budget->monthToBudgetMonth(it.key()) <= monthdate) m = it.value();
			else m = -1.0;
			if(m >= 0.0) {
				monthend = budget->lastBudgetDay(monthdate);
				has_budget = true;
				bool b_lastmonth = (monthlast == monthdate && to_date != monthend);
				v = account_month[account][monthend];
				if(partial_budget && (b_firstmonth || b_lastmonth)) {
					int days;
					if(b_firstmonth) days = from_date.daysTo(b_lastmonth ? to_date : monthend);
					else days = monthdate.daysTo(b_lastmonth ? to_date : monthend);
					int dim = monthdate.daysTo(monthend) + 1;
					m = (m * (days + 1)) / dim;
					if(b_firstmonth) v -= account_month_beginfirst[account];
					if(b_lastmonth) v -= account_month_endlast[account];
				}
				mbudget += m;
				diff += m - v;
			} else if(!subs.isEmpty()) {
				monthend = budget->lastBudgetDay(monthdate);
				bool b_lastmonth = (monthlast == monthdate && to_date != monthend);
				for(QVector<budget_info>::iterator sit = subs.begin(); sit != subs.end(); ++sit) {
					if(budget->monthToBudgetMonth(sit->it.key()) <= monthdate) m = sit->it.value();
					else m = -1.0;
					if(m >= 0.0) {
						has_subs_budget = true;
						v = account_month[sit->ca][monthend];
						if(partial_budget && (b_firstmonth || b_lastmonth)) {
							int days;
							if(b_firstmonth) days = from_date.daysTo(b_lastmonth ? to_date : monthend);
							else days = monthdate.daysTo(b_lastmonth ? to_date : monthend);
							int dim = monthdate.daysTo(monthend) + 1;
							m = (m * (days + 1)) / dim;
							if(b_firstmonth) v -= account_month_beginfirst[sit->ca];
							if(b_lastmonth) v -= account_month_endlast[sit->ca];
						}
						mbudget += m;
						diff += m - v;
					}
				}
			}
			b_firstmonth = false;
			budget->addBudgetMonthsSetFirst(monthdate, 1);
			if(it_n != it_e && monthdate == budget->monthToBudgetMonth(it_n.key())) {
				it = it_n;
				++it_n;
			}
			for(QVector<budget_info>::iterator sit = subs.begin(); sit != subs.end(); ++sit) {
				if(sit->it_n != sit->it_e && monthdate == budget->monthToBudgetMonth(sit->it_n.key())) {
					sit->it = sit->it_n;
					++(sit->it_n);
				}
			}
		} while(monthdate <= monthlast);

		bool b_future = (curdate < to_date);
		curdate = curdate.addDays(1);
		if(b_future && !ca->parentCategory()) {
			bool after_cur = true;
			
			it = ca->mbudgets.begin();
			while(it != it_e && it.value() < 0.0) ++it;
			if(it != it_e) {
				if(curdate < budget->monthToBudgetMonth(it.key())) {
					curmonth = budget->monthToBudgetMonth(it.key());
					after_cur = true;
				} else {
					curmonth = budget->monthToBudgetMonth(curdate);
					QMap<QDate, double>::const_iterator it_b = ca->mbudgets.begin();
					it = it_e;
					--it;
					while(it != it_b) {
						if(curmonth >= budget->monthToBudgetMonth(it.key())) break;
						--it;
					}
					after_cur = false;
				}
			}
			it_n = it;
			if(it_n != it_e) ++it_n;
			
			for(QVector<budget_info>::iterator sit = subs.begin(); sit != subs.end(); ++sit) {
				sit->it = sit->ca->mbudgets.begin();
				while(sit->it != sit->it_e && sit->it.value() < 0.0) ++(sit->it);
				if(curdate < budget->monthToBudgetMonth(sit->it.key())) {
					if(after_cur && curmonth > budget->monthToBudgetMonth(sit->it.key())) curmonth = budget->monthToBudgetMonth(sit->it.key());
				} else {
					if(after_cur) curmonth = budget->monthToBudgetMonth(curdate);
					QMap<QDate, double>::const_iterator it_b = sit->ca->mbudgets.begin();
					sit->it = sit->it_e;
					--(sit->it);
					while(sit->it != it_b) {
						if(curmonth >= budget->monthToBudgetMonth(sit->it.key())) break;
						--(sit->it);
					}
					after_cur = false;
				}
				sit->it_n = sit->it;
				if(sit->it_n != sit->it_e) ++(sit->it_n);
			}
			bool had_from = after_from || from_date <= curdate;
			bool b_curmonth = !after_cur && (curmonth != curdate);
			do {
				if(it != it_e && budget->monthToBudgetMonth(it.key()) <= curmonth) m = it.value();
				else m = -1.0;
				if(m >= 0.0) {
					monthend = budget->lastBudgetDay(curmonth);
					v = account_month[account][monthend];
					bool b_lastmonth = (monthlast == curmonth && to_date != monthend);
					bool b_frommonth = !had_from && frommonth == curmonth;
					int dim = curmonth.daysTo(monthend) + 1;
					if(partial_budget && (b_curmonth || b_lastmonth || b_frommonth)) {
						int days;
						if(b_curmonth) {
							v -= account_month_begincur[account];
							days = curdate.daysTo(b_lastmonth ? to_date : monthend);
						} else {
							days = curmonth.daysTo(b_lastmonth ? to_date : monthend);
						}
						if(b_lastmonth) v -= account_month_endlast[account];
						m = (m * (days + 1)) / dim;
						if(b_frommonth) {
							int days2;
							if(b_curmonth) days2 = curdate.daysTo(from_date);
							else days2 = curmonth.daysTo(from_date);
							double v3 = b_curmonth ? (account_month_beginfirst[account] - account_month_begincur[account]) : account_month_beginfirst[account];
							double m3 = ((m - v) * (days2 + 1)) / (days + 1);
							if(v3 > m3) m3 = v3;
							double m2 = m - m3;
							double v2 = v - v3;
							if(m2 > v2) future_change_diff += m2 - v2;
						}
					} else if(b_frommonth) {
						int days, days2;
						if(b_lastmonth) {
							v -= account_month_endlast[account];
							m = (m * curmonth.daysTo(to_date)) / dim;
						}
						if(b_curmonth) {
							days = curdate.daysTo(b_lastmonth ? to_date : monthend);
							days2 = curdate.daysTo(from_date);
						} else {
							days = curmonth.daysTo(b_lastmonth ? to_date : monthend);
							days2 = curmonth.daysTo(from_date);
						}
						double v3 = b_curmonth ? (account_month_beginfirst[account] - account_month_begincur[account]) : account_month_beginfirst[account];
						double m3 = ((m - v) * (days2 + 1)) / (days + 1);
						if(v3 > m3) m3 = v3;
						double m2 = m - v - m3;
						double v2 = v - account_month_beginfirst[account];
						if(m2 > v2) future_change_diff += m2 - v2;
					} else if(b_lastmonth) {
						v -= account_month_endlast[account];
						m = (m * curmonth.daysTo(to_date)) / dim;
					}
					if(m > v) {
						future_diff += m - v;
						if(had_from) future_change_diff += m - v;
					}
					if(b_frommonth) had_from = true;					
				} else if(!subs.isEmpty()) {
					monthend = budget->lastBudgetDay(curmonth);					
					bool b_lastmonth = (monthlast == monthdate && to_date != monthend);
					bool b_frommonth = !had_from && frommonth == curmonth;
					for(QVector<budget_info>::iterator sit = subs.begin(); sit != subs.end(); ++sit) {
						if(budget->monthToBudgetMonth(sit->it.key()) <= curmonth) m = sit->it.value();
						else m = -1.0;
						if(m >= 0.0) {
							v = account_month[sit->ca][monthend];
							int dim = curmonth.daysTo(monthend) + 1;
							if(partial_budget && (b_curmonth || b_lastmonth || b_frommonth)) {
								int days;
								if(b_curmonth) {
									v -= account_month_begincur[sit->ca];
									days = curdate.daysTo(b_lastmonth ? to_date : monthend);
								} else {
									days = curmonth.daysTo(b_lastmonth ? to_date : monthend);
								}
								if(b_lastmonth) v -= account_month_endlast[sit->ca];
								m = (m * (days + 1)) / dim;
								if(b_frommonth) {
									int days2;
									if(b_curmonth) days2 = curdate.daysTo(from_date);
									else days2 = curmonth.daysTo(from_date);
									double v3 = b_curmonth ? (account_month_beginfirst[sit->ca] - account_month_begincur[sit->ca]) : account_month_beginfirst[sit->ca];
									double m3 = ((m - v) * (days2 + 1)) / (days + 1);
									if(v3 > m3) m3 = v3;
									double m2 = m - m3;
									double v2 = v - v3;
									if(m2 > v2) future_change_diff += m2 - v2;
								}
							} else if(b_frommonth) {
								int days, days2;
								if(b_lastmonth) {
									v -= account_month_endlast[sit->ca];
									m = (m * curmonth.daysTo(to_date)) / dim;
								}
								if(b_curmonth) {
									days = curdate.daysTo(b_lastmonth ? to_date : monthend);
									days2 = curdate.daysTo(from_date);
								} else {
									days = curmonth.daysTo(b_lastmonth ? to_date : monthend);
									days2 = curmonth.daysTo(from_date);
								}
								double v3 = b_curmonth ? (account_month_beginfirst[sit->ca] - account_month_begincur[sit->ca]) : account_month_beginfirst[sit->ca];
								double m3 = ((m - v) * (days2 + 1)) / (days + 1);
								if(v3 > m3) m3 = v3;
								double m2 = m - v - m3;
								double v2 = v - account_month_beginfirst[sit->ca];
								if(m2 > v2) future_change_diff += m2 - v2;
							} else if(b_lastmonth) {
								v -= account_month_endlast[sit->ca];
								m = (m * curmonth.daysTo(to_date)) / dim;
							}							
							if(m > v) {
								future_diff += m - v;
								if(had_from) future_change_diff += m - v;
							}
							if(b_frommonth) had_from = true;
						}
					}
				}
				b_curmonth = false;
				budget->addBudgetMonthsSetFirst(curmonth, 1);
				if(it_n != it_e && curmonth == budget->monthToBudgetMonth(it_n.key())) {
					it = it_n;
					++it_n;
				}
				for(QVector<budget_info>::iterator sit = subs.begin(); sit != subs.end(); ++sit) {
					if(sit->it_n != sit->it_e && curmonth == budget->monthToBudgetMonth(sit->it_n.key())) {
						sit->it = sit->it_n;
						++(sit->it_n);
					}
				}
			} while(curmonth <= monthlast);
		}

		if(has_budget || has_subs_budget) {
			if(has_budget) i->setText(BUDGET_COLUMN, tr("%2 of %1", "%1: budget; %2: remaining budget").arg(budget->formatMoney(mbudget)).arg(budget->formatMoney(diff)));
			else i->setText(BUDGET_COLUMN, QString("(") + tr("%2 of %1", "%1: budget; %2: remaining budget").arg(budget->formatMoney(mbudget)).arg(budget->formatMoney(diff)) + QString(")"));
			setAccountBudgetColor(i, diff, account->type() == ACCOUNT_TYPE_EXPENSES);
			account_budget[account] = mbudget;
			account_budget_diff[account] = diff;
		} else {
			i->setText(BUDGET_COLUMN, "-");
			account_budget[account] = -1.0;
		}
		double future_diff_bak = future_diff, future_diff_change_bak = future_change_diff;
		future_diff -= account_future_diff[account];
		future_change_diff -= account_future_diff_change[account];
		account_future_diff[account] = future_diff_bak;
		account_future_diff_change[account] = future_diff_change_bak;
		if(is_zero(future_diff) && is_zero(future_change_diff)) return;
		if(budget->budgetAccount) {
			if(account->type() == ACCOUNT_TYPE_EXPENSES) {
				account_value[budget->budgetAccount] -= budget->budgetAccount->currency()->convertFrom(future_diff, budget->defaultCurrency());
				account_change[budget->budgetAccount] -= budget->budgetAccount->currency()->convertFrom(future_change_diff, budget->defaultCurrency());
			} else {
				account_value[budget->budgetAccount] += budget->budgetAccount->currency()->convertFrom(future_diff, budget->defaultCurrency());
				account_change[budget->budgetAccount] += budget->budgetAccount->currency()->convertFrom(future_change_diff, budget->defaultCurrency());
			}
			item_accounts[budget->budgetAccount]->setText(CHANGE_COLUMN, budget->budgetAccount->currency()->formatValue(account_change[budget->budgetAccount]));
			item_accounts[budget->budgetAccount]->setText(VALUE_COLUMN, budget->budgetAccount->currency()->formatValue(account_value[budget->budgetAccount]) + " ");
			setAccountChangeColor(item_accounts[budget->budgetAccount], account_value[budget->budgetAccount], false);
		}
		if(account->type() == ACCOUNT_TYPE_EXPENSES) {
			assets_accounts_value -= future_diff;
			assets_accounts_change -= future_change_diff;
		} else {
			assets_accounts_value += future_diff;
			assets_accounts_change += future_change_diff;
		}
		assetsItem->setText(VALUE_COLUMN, budget->formatMoney(assets_accounts_value) + " ");
		assetsItem->setText(CHANGE_COLUMN, budget->formatMoney(assets_accounts_change));
		setAccountChangeColor(assetsItem, assets_accounts_change, false);
	}

	if(item_accounts[account] == selectedItem(accountsView)) updateBudgetEdit();

}
void Eqonomize::updateBudgetEdit() {
	QTreeWidgetItem *i = selectedItem(accountsView);
	budgetEdit->blockSignals(true);
	budgetButton->blockSignals(true);
	if(i == NULL || i == liabilitiesItem || i == assetsItem || i == incomesItem || i == expensesItem || account_items[i]->type() == ACCOUNT_TYPE_ASSETS) {		
		budgetEdit->setEnabled(false);
		budgetButton->setEnabled(false);
		if(i == incomesItem || i == expensesItem) {
			QDate tomonth, prevmonth, prevmonth_end;
			tomonth = budget->budgetDateToMonth(to_date);
			prevmonth = budget->budgetDateToMonth(prevmonth_begin);
			prevmonth_end = prevmonth_begin.addDays(budget->daysInBudgetMonth(prevmonth_begin) - 1);
			double d_to = 0.0, d_prev = 0.0, v_prev = 0.0;
			CategoryAccount *ca = NULL;
			bool b_budget = false, b_budget_prev = false;
			AccountList<ExpensesAccount*>::const_iterator it_e = budget->expensesAccounts.constBegin();
			AccountList<IncomesAccount*>::const_iterator it_i = budget->incomesAccounts.constBegin();
			while((i == incomesItem && it_i != budget->incomesAccounts.constEnd()) || (i != incomesItem && it_e != budget->expensesAccounts.constEnd())) {
				if(i == incomesItem) {
					ca = *it_i;
					++it_i;
				} else {
					ca = *it_e;
					++it_e;
				}
				double d = ca->monthlyBudget(tomonth);
				if(d >= 0.0) {
					d_to += d;
					b_budget = true;
				}
				d = ca->monthlyBudget(prevmonth);
				if(d >= 0.0) {
					d_prev += d;
					b_budget_prev = true;
					v_prev += account_month[ca][prevmonth_end];
				}
			}
			if(!b_budget_prev) {
				it_e = budget->expensesAccounts.constBegin();
				it_i = budget->incomesAccounts.constBegin();
				while((i == incomesItem && it_i != budget->incomesAccounts.constEnd()) || (i != incomesItem && it_e != budget->expensesAccounts.constEnd())) {
					if(i == incomesItem) {
						ca = *it_i;
						++it_i;
					} else {
						ca = *it_e;
						++it_e;
					}
					v_prev += account_month[ca][prevmonth_end];
				}
			}
			if(!b_budget_prev) prevMonthBudgetLabel->setText(tr("%1 (with no budget)").arg(budget->formatMoney(v_prev)));
			else prevMonthBudgetLabel->setText(tr("%1 (with budget %2)").arg(budget->formatMoney(v_prev)).arg(budget->formatMoney(d_prev)));
			budgetButton->setChecked(b_budget);
			if(budgetEdit->value() != d_to) budgetEdit->setValue(d_to);
		} else {
			if(budgetEdit->value() != 0.0) budgetEdit->setValue(0.0);
			budgetButton->setChecked(false);
			prevMonthBudgetLabel->setText("-");
		}
	} else {
		CategoryAccount *ca = (CategoryAccount*) account_items[i];
		QDate tomonth = budget->budgetDateToMonth(to_date);
		double d = ca->monthlyBudget(tomonth);
		if(d < 0.0) {
			if(budgetEdit->value() != 0.0) budgetEdit->setValue(0.0);
			budgetButton->setChecked(false);
			budgetEdit->setEnabled(false);
		} else {
			if(budgetEdit->value() != d) budgetEdit->setValue(d);
			budgetButton->setChecked(true);
			budgetEdit->setEnabled(true);
		}
		budgetButton->setEnabled(true);
		d = ca->monthlyBudget(budget->budgetDateToMonth(prevmonth_begin));
		if(d < 0.0) prevMonthBudgetLabel->setText(tr("%1 (with no budget)").arg(budget->formatMoney(account_month[ca][prevmonth_begin.addDays(budget->daysInBudgetMonth(prevmonth_begin) - 1)])));
		else prevMonthBudgetLabel->setText(tr("%1 (with budget %2)").arg(budget->formatMoney(account_month[ca][prevmonth_begin.addDays(budget->daysInBudgetMonth(prevmonth_begin) - 1)])).arg(budget->formatMoney(d)));
 	}
	budgetEdit->blockSignals(false);
	budgetButton->blockSignals(false);
}
void Eqonomize::accountsSelectionChanged() {
	QTreeWidgetItem *i = selectedItem(accountsView);
	if(i == NULL || i == liabilitiesItem || i == assetsItem || i == incomesItem || i == expensesItem) {
		ActionDeleteAccount->setEnabled(false);
		ActionEditAccount->setEnabled(false);
		ActionBalanceAccount->setEnabled(false);
		budgetEdit->setEnabled(false);
		ActionReconcileAccount->setEnabled(false);
	} else {
		ActionDeleteAccount->setEnabled(true);
		ActionEditAccount->setEnabled(true);
		ActionBalanceAccount->setEnabled(account_items[i]->type() == ACCOUNT_TYPE_ASSETS && ((AssetsAccount*) account_items[i])->accountType() != ASSETS_TYPE_SECURITIES);
		ActionReconcileAccount->setEnabled(account_items[i]->type() == ACCOUNT_TYPE_ASSETS && ((AssetsAccount*) account_items[i])->accountType() != ASSETS_TYPE_SECURITIES);
		if(account_items[i]->type() == ACCOUNT_TYPE_ASSETS && ((AssetsAccount*) account_items[i])->isClosed()) {
			ActionCloseAccount->setEnabled(account_items[i]->type() == ACCOUNT_TYPE_ASSETS);
			ActionCloseAccount->setText(tr("Reopen Account", "Mark account as not closed"));
			ActionCloseAccount->setIcon(LOAD_ICON("edit-undo"));
		} else {
			ActionCloseAccount->setEnabled(account_items[i]->type() == ACCOUNT_TYPE_ASSETS && budget->accountHasTransactions(account_items[i]));
			ActionCloseAccount->setText(tr("Close Account", "Mark account as closed"));
			ActionCloseAccount->setIcon(LOAD_ICON("edit-delete"));
		}
	}
	ActionShowAccountTransactions->setEnabled(i != NULL && i != assetsItem && i != liabilitiesItem);
	updateBudgetEdit();
}
void Eqonomize::updateSecurityAccount(AssetsAccount *account, bool update_display) {
	double value = 0.0, value_from = 0.0;
	bool b_from = accountsPeriodFromButton->isChecked();
	for(SecurityList<Security*>::const_iterator it = budget->securities.constBegin(); it != budget->securities.constEnd(); ++it) {
		Security *security = *it;
		if(security->account() == account) {
			value += security->value(to_date, true);
			if(b_from) value_from += security->value(from_date, true);
		}
	}
	if(!b_from) {
		value_from = account->initialBalance();
	}
	assets_accounts_value -= account->currency()->convertTo(account_value[account], budget->defaultCurrency(), to_date);
	assets_accounts_value += account->currency()->convertTo(value, budget->defaultCurrency(), to_date);
	assets_accounts_change -= account->currency()->convertTo(account_change[account], budget->defaultCurrency(), to_date);
	assets_accounts_change += account->currency()->convertTo((value - value_from), budget->defaultCurrency(), to_date);
	account_value[account] = value;
	account_change[account] = value - value_from;
	if(update_display) {
		assetsItem->setText(VALUE_COLUMN, budget->formatMoney(assets_accounts_value) + " ");
		assetsItem->setText(CHANGE_COLUMN, budget->formatMoney(assets_accounts_change));
		setAccountChangeColor(assetsItem, assets_accounts_change, false);
		item_accounts[account]->setText(CHANGE_COLUMN, account->currency()->formatValue(value - value_from));
		item_accounts[account]->setText(VALUE_COLUMN, account->currency()->formatValue(value) + " ");
		setAccountChangeColor(item_accounts[account], value - value_from, false);
		item_accounts[account]->setHidden(account->isClosed() && is_zero(value) && is_zero(value_from));
	}
}
void Eqonomize::filterAccounts() {
	expenses_accounts_value = 0.0;
	expenses_accounts_change = 0.0;
	incomes_accounts_value = 0.0;
	incomes_accounts_change = 0.0;
	assets_accounts_value = 0.0;
	assets_accounts_change = 0.0;
	liabilities_accounts_value = 0.0;
	liabilities_accounts_change = 0.0;
	incomes_budget = 0.0;
	incomes_budget_diff = 0.0;
	expenses_budget = 0.0;
	expenses_budget_diff = 0.0;
	for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
		AssetsAccount *aaccount = *it;
		if(aaccount->accountType() == ASSETS_TYPE_SECURITIES) {
			account_value[aaccount] = 0.0;
			account_change[aaccount] = 0.0;
			updateSecurityAccount(aaccount, false);
		} else {
			account_value[aaccount] = aaccount->initialBalance();
			account_change[aaccount] = 0.0;
			if(aaccount->currency()) {
				if(IS_DEBT(aaccount)) liabilities_accounts_value += aaccount->currency()->convertTo(account_value[aaccount], budget->defaultCurrency(), to_date);
				else assets_accounts_value += aaccount->currency()->convertTo(account_value[aaccount], budget->defaultCurrency(), to_date);
			}
		}
	}
	QDate monthdate, monthdate_begin;
	bool b_from = accountsPeriodFromButton->isChecked();
	QDate lastmonth = budget->lastBudgetDay(to_date);
	QDate curdate = QDate::currentDate(), curmonth, curmonth_begin;
	curmonth_begin = budget->firstBudgetDay(curdate);
	prevmonth_begin = budget->firstBudgetDay(to_date);
	budget->addBudgetMonthsSetFirst(prevmonth_begin, -1);
	curmonth = budget->lastBudgetDay(curdate);
	frommonth_begin = QDate();
	for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
		IncomesAccount *iaccount = *it;
		account_value[iaccount] = 0.0;
		account_change[iaccount] = 0.0;
		account_month[iaccount] = QMap<QDate, double>();
		account_month[iaccount][monthdate] = 0.0;
		account_budget[iaccount] = -1.0;
		account_budget_diff[iaccount] = 0.0;
		account_month_beginfirst[iaccount] = 0.0;
		account_month_begincur[iaccount] = 0.0;
		account_month_endlast[iaccount] = 0.0;
		account_future_diff[iaccount] = 0.0;
		account_future_diff_change[iaccount] = 0.0;
		if(!iaccount->mbudgets.isEmpty() && iaccount->mbudgets.begin().value() >= 0.0 && (frommonth_begin.isNull() || budget->monthToBudgetMonth(iaccount->mbudgets.begin().key()) < frommonth_begin)) {
			frommonth_begin = budget->monthToBudgetMonth(iaccount->mbudgets.begin().key());
		}
	}
	for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
		ExpensesAccount *eaccount = *it;
		account_value[eaccount] = 0.0;
		account_change[eaccount] = 0.0;
		account_month[eaccount] = QMap<QDate, double>();
		account_month[eaccount][monthdate] = 0.0;
		account_budget[eaccount] = -1.0;
		account_budget_diff[eaccount] = 0.0;
		account_month_beginfirst[eaccount] = 0.0;
		account_month_begincur[eaccount] = 0.0;
		account_month_endlast[eaccount] = 0.0;
		account_future_diff[eaccount] = 0.0;
		account_future_diff_change[eaccount] = 0.0;
		if(!eaccount->mbudgets.isEmpty() && eaccount->mbudgets.begin().value() >= 0.0 && (frommonth_begin.isNull() || budget->monthToBudgetMonth(eaccount->mbudgets.begin().key()) < frommonth_begin)) {
			frommonth_begin = budget->monthToBudgetMonth(eaccount->mbudgets.begin().key());
		}
	}
	if(frommonth_begin.isNull() || (b_from && frommonth_begin < from_date)) {
		if(b_from) frommonth_begin = budget->firstBudgetDay(from_date);
		else frommonth_begin = curmonth_begin;
	}
	if(frommonth_begin.isNull() || frommonth_begin > prevmonth_begin) {
		monthdate_begin = prevmonth_begin;
	} else {
		monthdate_begin = frommonth_begin;
	}
	if(monthdate_begin > curmonth_begin) monthdate_begin = curmonth_begin;
	monthdate = budget->lastBudgetDay(monthdate_begin);
	bool b_future = false;
	bool b_past = (curdate >= to_date);
	if(b_past) {
		footer1->hide();
		assetsItem->setText(0, tr("Assets"));
		if(budget->budgetAccount) {
			item_accounts[budget->budgetAccount]->setText(0, budget->budgetAccount->name());
		}
	} else {
		footer1->show();
		assetsItem->setText(0, tr("Assets") + "*");
		if(budget->budgetAccount) {
			item_accounts[budget->budgetAccount]->setText(0, budget->budgetAccount->name() + "*");
		}
	}
	for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constBegin(); it != budget->transactions.constEnd(); ++it) {
		Transaction *trans = *it;
		if(trans->date() > lastmonth) break;
		if(!b_past && !b_future && trans->date() >= curmonth_begin) b_future = true;
		if(!b_from || b_future || trans->date() >= frommonth_begin || trans->date() >= prevmonth_begin) {
			while(trans->date() > monthdate) {
				budget->addBudgetMonthsSetFirst(monthdate_begin, 1);
				monthdate = budget->lastBudgetDay(monthdate_begin);
				for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
					IncomesAccount *iaccount = *it;
					account_month[iaccount][monthdate] = 0.0;
				}
				for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
					ExpensesAccount *eaccount = *it;
					account_month[eaccount][monthdate] = 0.0;
				}
			}
			addTransactionValue(trans, trans->date(), false, false, -1, b_future, &monthdate);
		} else {
			addTransactionValue(trans, trans->date(), false, false, -1, b_future, NULL);
		}
	}
	while(lastmonth >= monthdate) {
		budget->addBudgetMonthsSetFirst(monthdate_begin, 1);
		monthdate = budget->lastBudgetDay(monthdate_begin);
		for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
			IncomesAccount *iaccount = *it;
			account_month[iaccount][monthdate] = 0.0;
		}
		for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
			ExpensesAccount *eaccount = *it;
			account_month[eaccount][monthdate] = 0.0;
		}
	}
	for(ScheduledTransactionList<ScheduledTransaction*>::const_iterator it = budget->scheduledTransactions.constBegin(); it != budget->scheduledTransactions.constEnd(); ++it) {
		ScheduledTransaction *strans = *it;
		if(strans->transaction()->date() > lastmonth) break;
		addScheduledTransactionValue(strans, false, false);
	}
	for(QMap<QTreeWidgetItem*, Account*>::iterator it = account_items.begin(); it != account_items.end(); ++it) {
		switch(it.value()->type()) {
			case ACCOUNT_TYPE_INCOMES: {}
			case ACCOUNT_TYPE_EXPENSES: {
				updateMonthlyBudget(it.value());
				break;
			}
			default: {}
		}
	}
	updateTotalMonthlyIncomesBudget();
	updateTotalMonthlyExpensesBudget();
	for(QMap<QTreeWidgetItem*, Account*>::iterator it = account_items.begin(); it != account_items.end(); ++it) {
		bool is_debt = (it.key()->parent() == liabilitiesItem);
		it.key()->setText(CHANGE_COLUMN, it.value()->currency()->formatValue(is_debt ? -account_change[it.value()] : account_change[it.value()]));
		it.key()->setText(VALUE_COLUMN, it.value()->currency()->formatValue(is_debt ? -account_value[it.value()] : account_value[it.value()]) + " ");
		setAccountChangeColor(it.key(), is_debt ? -account_change[it.value()] : account_change[it.value()], is_debt || it.value()->type() == ACCOUNT_TYPE_EXPENSES);
		it.key()->setHidden(it.value()->isClosed() && is_zero(account_change[it.value()]) && is_zero(account_value[it.value()]));
	}
	incomesItem->setText(VALUE_COLUMN, budget->formatMoney(incomes_accounts_value) + " ");
	incomesItem->setText(CHANGE_COLUMN, budget->formatMoney(incomes_accounts_change));
	setAccountChangeColor(incomesItem, incomes_accounts_change, false);
	expensesItem->setText(VALUE_COLUMN, budget->formatMoney(expenses_accounts_value) + " ");
	expensesItem->setText(CHANGE_COLUMN, budget->formatMoney(expenses_accounts_change));
	setAccountChangeColor(expensesItem, expenses_accounts_change, true);
	assetsItem->setText(VALUE_COLUMN, budget->formatMoney(assets_accounts_value) + " ");
	assetsItem->setText(CHANGE_COLUMN, budget->formatMoney(assets_accounts_change));
	setAccountChangeColor(assetsItem, assets_accounts_change, false);
	liabilitiesItem->setText(VALUE_COLUMN, budget->formatMoney(-liabilities_accounts_value) + " ");
	liabilitiesItem->setText(CHANGE_COLUMN, budget->formatMoney(-liabilities_accounts_change));
	setAccountChangeColor(liabilitiesItem, -liabilities_accounts_change, true);
	budgetMonthEdit->blockSignals(true);
	budgetMonthEdit->setDate(budget->budgetDateToMonth(to_date));
	budgetMonthEdit->blockSignals(false);
	updateBudgetEdit();
}

EqonomizeTreeWidget::EqonomizeTreeWidget(QWidget *parent) : QTreeWidget(parent) {
	setAlternatingRowColors(true);
	setExpandsOnDoubleClick(false);
}
EqonomizeTreeWidget::EqonomizeTreeWidget() : QTreeWidget() {}
void EqonomizeTreeWidget::keyPressEvent(QKeyEvent *e) {
	if((e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) && currentItem()) {
		emit returnPressed(currentItem());
		QTreeWidget::keyPressEvent(e);
		e->accept();
		return;
	} else if(e->key() == Qt::Key_Space && currentItem()) {
		emit spacePressed(currentItem());
		QTreeWidget::keyPressEvent(e);
		e->accept();
		return;
	}
	QTreeWidget::keyPressEvent(e);
}
void EqonomizeTreeWidget::dropEvent(QDropEvent *event) {
	QTreeWidgetItem *target = itemAt(event->pos());
	if(!target) return;
	DropIndicatorPosition dropPos = dropIndicatorPosition();
	if(dropPos != OnItem) target = target->parent();
	QList<QTreeWidgetItem*> dragItems = selectedItems();
	if(!dragItems.isEmpty() && target) {
		emit itemMoved(dragItems[0], target);
	}
	event->setDropAction(Qt::IgnoreAction);
}

QIFWizardPage::QIFWizardPage() : QWizardPage() {
	is_complete = false;
}
bool QIFWizardPage::isComplete() const {return is_complete;}
void QIFWizardPage::setComplete(bool b) {is_complete = b; emit completeChanged();}

