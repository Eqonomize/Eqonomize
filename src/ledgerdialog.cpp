/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016-2017 by Hanna Knutsson            *
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
#include <QLineEdit>
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
#include <QHeaderView>
#include <QMenu>
#include <QAction>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
#	include <QScreen>
#else
#	include <QDesktopWidget>
#endif
#include <QShortcut>
#include <QDateTimeEdit>

#include "budget.h"
#include "eqonomize.h"
#include "eqonomizevalueedit.h"
#include "transactioneditwidget.h"
#include "eqonomizevalueedit.h"
#include "editaccountdialogs.h"

extern QString htmlize_string(QString str);
extern QColor createExpenseColor(QTreeWidgetItem *i, int = 0);
extern QColor createIncomeColor(QTreeWidgetItem *i, int = 0);
extern QColor createTransferColor(QTreeWidgetItem *i, int = 0);
extern QColor createExpenseColor(QWidget *w);
extern QColor createIncomeColor(QWidget *w);
extern QColor createTransferColor(QWidget *w);
extern void setColumnTextWidth(QTreeWidget *w, int i, QString str);
extern void setColumnDateWidth(QTreeWidget *w, int i);
extern void setColumnMoneyWidth(QTreeWidget *w, int i, Budget *budget, double v = 9999999.99, int d = -1);
extern void setColumnStrlenWidth(QTreeWidget *w, int i, int l);

QColor incomeColor, expenseColor;
QColor labelIncomeColor, labelExpenseColor, labelTransferColor;

class LedgerListViewItem : public QTreeWidgetItem, public QObject {
	protected:
		Transaction *o_trans;
		SplitTransaction *o_split;
	public:
		LedgerListViewItem(Transaction *trans, SplitTransaction *split, QTreeWidget *parent, QString, QString = QString(), QString = QString(), QString = QString(), QString = QString(), QString = QString(), QString = QString(), QString = QString(), QString = QString(), QString = QString(), QString = QString());
		void setReconciled(int, int);
		void updateMark(int);
		void setColors();
		Transaction *transaction() const;
		SplitTransaction *splitTransaction() const;
		bool matches(const QString&) const;
		double d_balance;
		bool b_other_account;
		int b_reconciled;
};

LedgerListViewItem::LedgerListViewItem(Transaction *trans, SplitTransaction *split, QTreeWidget *parent, QString s1, QString s2, QString s3, QString s4, QString s5, QString s6, QString s7, QString s8, QString s9, QString s10, QString s11) : QTreeWidgetItem(parent), o_trans(trans), o_split(split), b_other_account(false), b_reconciled(-1) {
	setText(1, s1);
	setText(2, s2);
	setText(3, s3);
	setText(4, s4);
	setText(5, s5);
	setText(6, s6);
	setText(7, s7);
	setText(9, s8);
	setText(10, s9);
	setText(11, s10);
	setText(8, s11);
	setTextAlignment(8, Qt::AlignRight | Qt::AlignVCenter);
	setTextAlignment(9, Qt::AlignRight | Qt::AlignVCenter);
	setTextAlignment(10, Qt::AlignRight | Qt::AlignVCenter);
	setTextAlignment(11, Qt::AlignRight | Qt::AlignVCenter);
	setTextAlignment(0, Qt::AlignCenter | Qt::AlignVCenter);
	if(parent) {
		if(!expenseColor.isValid()) expenseColor = createExpenseColor(this, 8);
		setForeground(8, expenseColor);
		if(!incomeColor.isValid()) incomeColor = createIncomeColor(this, 9);
		setForeground(9, incomeColor);
		setForeground(10, expenseColor);
	}
}
void LedgerListViewItem::setReconciled(int b, int mark) {
	b_reconciled = b;
	if(b < 0) {
		setCheckState(0, Qt::Unchecked);
	} else if(b) {
		setCheckState(0, Qt::Checked);
	} else {
		setCheckState(0, Qt::Unchecked);
	}
	updateMark(mark);
}
void LedgerListViewItem::updateMark(int mark) {
	if(mark < -1) return;
	QBrush br;
	if(mark == 0 || (mark > 0 && b_reconciled != 0)) {
		br = treeWidget()->viewport()->palette().alternateBase();
	} else if(mark > 0) {
		br = treeWidget()->viewport()->palette().base();
	}
	if(background(1) != br) {
		for(int c = 0; c <= 11; c++) {
			setBackground(c, br);
		}
	}
	if(isDisabled() != (mark >= 0 && b_reconciled < 0)) setDisabled(mark >= 0 && b_reconciled < 0);
}
void LedgerListViewItem::setColors() {
	if(!expenseColor.isValid()) expenseColor = createExpenseColor(this, 8);
	setForeground(8, expenseColor);
	if(!incomeColor.isValid()) incomeColor = createIncomeColor(this, 9);
	setForeground(9, incomeColor);
	setForeground(10, expenseColor);
}
Transaction *LedgerListViewItem::transaction() const {
	return o_trans;
}
SplitTransaction *LedgerListViewItem::splitTransaction() const {
	return o_split;
}
bool LedgerListViewItem::matches(const QString &str) const {
	for(int i = 3; i <= 7; i++) {
		if(!treeWidget()->isColumnHidden(i) && text(i).contains(str, Qt::CaseInsensitive)) return true;
	}
	return false;
}

LedgerDialog::LedgerDialog(AssetsAccount *acc, Budget *budg, Eqonomize *parent, QString title, bool extra_parameters, bool do_reconciliation) : QDialog(NULL, Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint), account(acc), mainWin(parent), budget(budg), b_extra(extra_parameters) {

	setWindowTitle(title);
	setModal(false);

	headerMenu = NULL;
	listMenu = NULL;

	key_event = NULL;

	re1 = 0;
	re2 = 0;

	setAttribute(Qt::WA_DeleteOnClose, true);

	if(!budget) budget = account->budget();

	QVBoxLayout *box1 = new QVBoxLayout(this);

	bool focus_list = (account != NULL);

	if(!account) {
		for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
			AssetsAccount *aaccount = *it;
			if(aaccount != budget->balancingAccount && aaccount->accountType() != ASSETS_TYPE_SECURITIES && (!account || !aaccount->isClosed())) {
				account = aaccount;
				if(!account->isClosed()) break;
			}
		}
	}

	QHBoxLayout *topbox = new QHBoxLayout();
	box1->addLayout(topbox);
	topbox->addWidget(new QLabel(tr("Account:"), this));
	accountCombo = new QComboBox(this);
	accountCombo->setEditable(false);

	bool show_balancing = false;
	for(TransactionList<Transfer*>::const_iterator it = budget->transfers.constBegin(); it != budget->transfers.constEnd(); ++it) {
		if((*it)->to() == budget->balancingAccount || (*it)->from() == budget->balancingAccount) {
			show_balancing = true;
			break;
		}
	}

	int i = 0;
	for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
		AssetsAccount *aaccount = *it;
		if((show_balancing || aaccount != budget->balancingAccount) && aaccount->accountType() != ASSETS_TYPE_SECURITIES) {
			accountCombo->addItem(aaccount->name(), QVariant::fromValue((void*) aaccount));
			if(aaccount == account) accountCombo->setCurrentIndex(i);
			i++;
		}
	}
	topbox->addWidget(accountCombo);
	QDialogButtonBox *topbuttons = new QDialogButtonBox(this);
	editAccountButton = topbuttons->addButton(tr("Edit Account…"), QDialogButtonBox::ActionRole);
	editAccountButton->setAutoDefault(false);
	editAccountButton->setEnabled(accountCombo->count() > 0);
	exportButton = topbuttons->addButton(tr("Export…"), QDialogButtonBox::ActionRole);
	exportButton->setAutoDefault(false);
	printButton = topbuttons->addButton(tr("Print…"), QDialogButtonBox::ActionRole);
	printButton->setAutoDefault(false);
	reconcileButton = topbuttons->addButton(tr("Reconcile", "Accounting context"), QDialogButtonBox::ActionRole);
	reconcileButton->setCheckable(true);
	reconcileButton->setChecked(do_reconciliation);
	reconcileButton->setAutoDefault(false);
	markReconciledButton = topbuttons->addButton(tr("Mark all as reconciled", "Accounting context"), QDialogButtonBox::ActionRole);
	markReconciledButton->setAutoDefault(false);
	topbox->addWidget(topbuttons);
	QHBoxLayout *findbox = new QHBoxLayout();
	findbox->setSpacing(0);
	searchEdit = new QLineEdit(this);
	ActionSearch = searchEdit->addAction(LOAD_ICON("edit-find"), QLineEdit::LeadingPosition);
	findbox->addWidget(searchEdit);
	searchNextButton = new QPushButton(LOAD_ICON("go-down"), "", this);
	searchNextButton->setEnabled(false);
	findbox->addWidget(searchNextButton);
	searchPreviousButton = new QPushButton(LOAD_ICON("go-up"), "", this);
	searchPreviousButton->setEnabled(false);
	findbox->addWidget(searchPreviousButton);
	topbox->addLayout(findbox);
	topbox->addStretch(1);

	reconcileWidget = new QFrame(this);
	QGridLayout *reconcileLayout = new QGridLayout;
	reconcileLayout->addWidget(new QLabel(tr("Opening balance:", "Accounting context")), 0, 0);
	reconcileStartEdit = new EqonomizeDateEdit(QDate::currentDate(), this);
	reconcileStartEdit->setCalendarPopup(true);
	reconcileLayout->addWidget(reconcileStartEdit, 0, 1);
	reconcileOpeningEdit = new EqonomizeValueEdit(0.0, true, true, this, budget);
	reconcileLayout->addWidget(reconcileOpeningEdit, 0, 2);
	reconcileBOpeningLabel = new QLabel(this);
	reconcileLayout->addWidget(reconcileBOpeningLabel, 0, 3);
	reconcileROpeningLabel = new QLabel(this);
	reconcileLayout->addWidget(reconcileROpeningLabel, 0, 4);
	reconcileLayout->addWidget(new QLabel(tr("Change:", "Accounting context")), 1, 0);
	reconcileChangeEdit = new EqonomizeValueEdit(0.0, true, true, this, budget);
	reconcileLayout->addWidget(reconcileChangeEdit, 1, 2);
	reconcileBChangeLabel = new QLabel(this);
	reconcileLayout->addWidget(reconcileBChangeLabel, 1, 3);
	reconcileRChangeLabel = new QLabel(this);
	reconcileLayout->addWidget(reconcileRChangeLabel, 1, 4);
	reconcileLayout->addWidget(new QLabel(tr("Closing balance:", "Accounting context")), 2, 0);
	reconcileEndEdit = new EqonomizeDateEdit(QDate::currentDate(), this);
	reconcileEndEdit->setCalendarPopup(true);
	reconcileLayout->addWidget(reconcileEndEdit, 2, 1);
	reconcileClosingEdit = new EqonomizeValueEdit(0.0, true, true, this, budget);
	reconcileLayout->addWidget(reconcileClosingEdit, 2, 2);
	reconcileBClosingLabel = new QLabel(this);
	reconcileLayout->addWidget(reconcileBClosingLabel, 2, 3);
	reconcileRClosingLabel = new QLabel(this);
	reconcileLayout->addWidget(reconcileRClosingLabel, 2, 4);
	reconcileLayout->setColumnStretch(0, 0);
	reconcileLayout->setColumnStretch(1, 1);
	reconcileLayout->setColumnStretch(2, 1);
	reconcileLayout->setColumnStretch(3, 0);
	reconcileLayout->setColumnStretch(4, 3);
	reconcileLayout->setHorizontalSpacing(12);
	reconcileWidget->setLayout(reconcileLayout);
	box1->addWidget(reconcileWidget);

	QHBoxLayout *box2 = new QHBoxLayout();
	box1->addLayout(box2);
	QVBoxLayout *box3 = new QVBoxLayout();
	box2->addLayout(box3);
	transactionsView = new EqonomizeTreeWidget(this);
	transactionsView->setSortingEnabled(false);
	transactionsView->setAllColumnsShowFocus(true);
	transactionsView->setColumnCount(12);
	QStringList headers;
	headers << tr("R", "Header for account reconciled checkbox column");
	headers << tr("Date");
	headers << tr("Type");
	headers << tr("Description", "Transaction description property (transaction title/generic article name)");
	headers << tr("Account/Category");
	headers << tr("Payee/Payer");
	headers << tr("Tags");
	headers << tr("Comments");
	headers << tr("Expense");
	headers << tr("Deposit", "Money put into account");
	headers << tr("Withdrawal", "Money taken out from account");
	headers << tr("Balance", "Noun. Balance of an account");
	transactionsView->setHeaderLabels(headers);
	transactionsView->setRootIsDecorated(false);
	transactionsView->resizeColumnToContents(0);
	updateColumnWidths();
	transactionsView->setColumnHidden(5, true);
	transactionsView->setColumnHidden(6, true);
	transactionsView->setColumnHidden(7, true);
	transactionsView->setSelectionMode(QTreeWidget::ExtendedSelection);
	QSizePolicy sp = transactionsView->sizePolicy();
	sp.setHorizontalPolicy(QSizePolicy::MinimumExpanding);
	transactionsView->setSizePolicy(sp);
	box3->addWidget(transactionsView);
	QDialogButtonBox *buttons = new QDialogButtonBox(Qt::Vertical, this);
	QPushButton *newButton = buttons->addButton(tr("New"), QDialogButtonBox::ActionRole);
	QMenu *newMenu = new QMenu(this);
	newButton->setMenu(newMenu);
	newButton->setAutoDefault(false);
	ActionNewDebtPayment = newMenu->addAction(mainWin->ActionNewDebtPayment->icon(), mainWin->ActionNewDebtPayment->text());
	connect(ActionNewDebtPayment, SIGNAL(triggered()), this, SLOT(newDebtPayment()));
	ActionNewDebtInterest = newMenu->addAction(mainWin->ActionNewDebtInterest->icon(), mainWin->ActionNewDebtInterest->text());
	connect(ActionNewDebtInterest, SIGNAL(triggered()), this, SLOT(newDebtInterest()));
	connect(newMenu->addAction(mainWin->ActionNewExpense->icon(), mainWin->ActionNewExpense->text()), SIGNAL(triggered()), this, SLOT(newExpense()));
	connect(newMenu->addAction(mainWin->ActionNewIncome->icon(), mainWin->ActionNewIncome->text()), SIGNAL(triggered()), this, SLOT(newIncome()));
	connect(newMenu->addAction(mainWin->ActionNewTransfer->icon(), mainWin->ActionNewTransfer->text()), SIGNAL(triggered()), this, SLOT(newTransfer()));
	connect(newMenu->addAction(mainWin->ActionNewMultiItemTransaction->icon(), mainWin->ActionNewMultiItemTransaction->text()), SIGNAL(triggered()), this, SLOT(newSplit()));
	editButton = buttons->addButton(tr("Edit…"), QDialogButtonBox::ActionRole);
	editButton->setEnabled(false);
	editButton->setAutoDefault(false);
	removeButton = buttons->addButton(tr("Delete"), QDialogButtonBox::ActionRole);
	removeButton->setEnabled(false);
	removeButton->setAutoDefault(false);
	//: join transactions together
	joinButton = buttons->addButton(tr("Join…"), QDialogButtonBox::ActionRole);
	joinButton->setEnabled(false);
	joinButton->setAutoDefault(false);
	//: split up joined transactions
	splitUpButton = buttons->addButton(tr("Split Up"), QDialogButtonBox::ActionRole);
	splitUpButton->setEnabled(false);
	splitUpButton->setAutoDefault(false);
	box2->addWidget(buttons);

	statLabel = new QLabel("", this);
	statLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	box3->addWidget(statLabel);

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
	buttonBox->button(QDialogButtonBox::Close)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Close)->setAutoDefault(false);
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(reject()));
	box1->addWidget(buttonBox);

#define NEW_ACTION(action, text, icon, receiver, slot) action = new QAction(this); action->setText(text); action->setIcon(LOAD_ICON(icon)); connect(action, SIGNAL(triggered()), receiver, slot);
#define NEW_ACTION_ALT(action, text, icon, icon_alt, receiver, slot) action = new QAction(this); action->setText(text); action->setIcon(LOAD_ICON2(icon, icon_alt)); connect(action, SIGNAL(triggered()), receiver, slot);

	NEW_ACTION_ALT(ActionEdit, tr("Edit Transaction(s)…"), "document-edit", "eqz-edit", this, SLOT(edit()));
	NEW_ACTION(ActionClone, mainWin->ActionCloneTransaction->text(), "edit-copy", this, SLOT(cloneTransaction()));
	NEW_ACTION(ActionJoin, mainWin->ActionJoinTransactions->text(), "eqz-join-transactions", this, SLOT(joinTransactions()));
	NEW_ACTION(ActionSplit, mainWin->ActionSplitUpTransaction->text(), "eqz-split-transaction", this, SLOT(splitUpTransaction()));
	NEW_ACTION(ActionOpenFile, mainWin->ActionOpenAssociatedFile->text(), "system-run", this, SLOT(openAssociatedFile()));
	NEW_ACTION(ActionDelete, tr("Remove Transaction(s)"), "edit-delete", this, SLOT(remove()));
	NEW_ACTION(ActionMarkReconciled, tr("Mark as reconciled"), "edit-delete", this, SLOT(reconcileTransactions()));
	NEW_ACTION(ActionEditTimestamp, mainWin->ActionEditTimestamp->text(), "eqz-schedule", this, SLOT(editTimestamp()));

	new QShortcut(QKeySequence::Find, searchEdit, SLOT(setFocus()));

	connect(transactionsView, SIGNAL(itemSelectionChanged()), this, SLOT(transactionSelectionChanged()));
	connect(transactionsView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(edit(QTreeWidgetItem*, int)));
	connect(transactionsView, SIGNAL(returnPressed(QTreeWidgetItem*)), this, SLOT(onTransactionReturnPressed(QTreeWidgetItem*)));
	connect(transactionsView, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT(transactionActivated(QTreeWidgetItem*, int)));
	connect(transactionsView, SIGNAL(spacePressed(QTreeWidgetItem*)), this, SLOT(onTransactionSpacePressed(QTreeWidgetItem*)));
	connect(removeButton, SIGNAL(clicked()), this, SLOT(remove()));
	connect(editButton, SIGNAL(clicked()), this, SLOT(edit()));
	connect(joinButton, SIGNAL(clicked()), this, SLOT(joinTransactions()));
	connect(splitUpButton, SIGNAL(clicked()), this, SLOT(splitUpTransaction()));
	connect(exportButton, SIGNAL(clicked()), this, SLOT(saveView()));
	connect(printButton, SIGNAL(clicked()), this, SLOT(printView()));
	connect(editAccountButton, SIGNAL(clicked()), this, SLOT(editAccount()));
	connect(accountCombo, SIGNAL(activated(int)), this, SLOT(accountActivated(int)));
	connect(mainWin, SIGNAL(transactionsModified()), this, SLOT(updateTransactions()));
	connect(mainWin, SIGNAL(accountsModified()), this, SLOT(updateAccounts()));
	connect(reconcileButton, SIGNAL(toggled(bool)), this, SLOT(toggleReconciliation(bool)));
	connect(markReconciledButton, SIGNAL(clicked()), this, SLOT(markAsReconciled()));
	connect(reconcileStartEdit, SIGNAL(dateChanged(const QDate&)), this, SLOT(reconcileStartDateChanged(const QDate&)));
	connect(reconcileEndEdit, SIGNAL(dateChanged(const QDate&)), this, SLOT(reconcileEndDateChanged(const QDate&)));
	connect(reconcileOpeningEdit, SIGNAL(valueChanged(double)), this, SLOT(reconcileOpeningChanged(double)));
	connect(reconcileClosingEdit, SIGNAL(valueChanged(double)), this, SLOT(reconcileClosingChanged(double)));
	connect(reconcileChangeEdit, SIGNAL(valueChanged(double)), this, SLOT(reconcileChangeChanged(double)));
	transactionsView->header()->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(transactionsView->header(), SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(popupHeaderMenu(const QPoint&)));
	transactionsView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(transactionsView, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(popupListMenu(const QPoint&)));
	connect(ActionSearch, SIGNAL(triggered(bool)), this, SLOT(search()));
	connect(searchEdit, SIGNAL(textChanged(const QString&)), this, SLOT(searchChanged(const QString&)));
	connect(searchNextButton, SIGNAL(clicked()), this, SLOT(search()));
	connect(searchPreviousButton, SIGNAL(clicked()), this, SLOT(searchPrevious()));

	QSettings settings;
	QSize dialog_size = settings.value("Ledger/size", QSize()).toSize();
	if(settings.value("GeneralOptions/version", 0).toInt() >= 140) {
		transactionsView->header()->restoreState(settings.value("Ledger/listState", QByteArray()).toByteArray());
	}
	transactionsView->header()->setSectionsMovable(true);
	b_ascending = settings.value("Ledger/ascending", false).toBool();
	if(dialog_size.isValid()) {
		resize(dialog_size);
	}

	accountChanged();

	if(focus_list) transactionsView->setFocus();

	toggleReconciliation(reconcileButton->isChecked());

	transactionsView->setMinimumHeight(500);

}
LedgerDialog::~LedgerDialog() {}

void LedgerDialog::keyPressEvent(QKeyEvent *e) {
	if(e == key_event) return;
	QDialog::keyPressEvent(e);
	if(!e->isAccepted() && !transactionsView->hasFocus()) {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
		key_event = new QKeyEvent(e->type(), e->key(), e->modifiers(), e->nativeScanCode(), e->nativeVirtualKey(), e->nativeModifiers(), e->text(), e->isAutoRepeat(), e->count());
#else
		key_event = new QKeyEvent(*e);
#endif
		QApplication::sendEvent(transactionsView, key_event);
		delete key_event;
	}
}
void LedgerDialog::updateReconciliationStats(bool b_toggled, bool scroll_to, bool update_markers) {
	if(!account) return;
	QDate d_start = reconcileStartEdit->date();
	QDate d_end = reconcileEndEdit->date();
	if(!d_start.isValid() || !d_end.isValid()) return;
	double d_rec_ch = 0.0;
	d_book_op = account->initialBalance();
	d_book_cl = account->initialBalance();
	d_rec_op = account->initialBalance();
	bool b_started = false, b_finished = false;;
	LedgerListViewItem *i_first = NULL, *i_last = NULL;
	transactionsView->model()->blockSignals(true);
	QTreeWidgetItemIterator it(transactionsView, QTreeWidgetItemIterator::All);
	while(*it) {
		LedgerListViewItem *i = (LedgerListViewItem*) *it;
		Transactions *trans = i->transaction();
		if(i->splitTransaction()) trans = i->splitTransaction();
		if(!trans) {
			i->updateMark(0);
			break;
		}
		if(!b_finished && trans->date() < d_start) {
			d_book_op = i->d_balance;
			b_finished = true;
		}
		if(!b_finished && trans->date() <= d_end) {
			if(!b_started) {
				b_started = true;
				d_book_cl = i->d_balance;
				i_first = i;
			}
			i_last = i;
			if(update_markers) i->updateMark(1);
		}
		if(b_started && trans->isReconciled(account)) {
			if(b_finished) d_rec_op += trans->accountChange(account);
			else d_rec_ch += trans->accountChange(account);

		}
		if(update_markers && (b_finished || !b_started)) i->updateMark(b_finished ? 2 : 0);
		++it;
	}
	transactionsView->model()->blockSignals(false);
	transactionsView->dataChanged(QModelIndex(), QModelIndex());
	if(scroll_to && i_first && i_last) {
		transactionsView->scrollToItem(i_first);
		transactionsView->scrollToItem(i_last);
	}
	if(b_toggled || re1 == 0) {
		reconcileOpeningEdit->blockSignals(true);
		reconcileClosingEdit->blockSignals(true);
		reconcileChangeEdit->blockSignals(true);
		if(re1 == 0 || reconcileOpeningEdit->value() == 0.0) reconcileOpeningEdit->setValue(d_book_op);
		if(re1 == 0 || reconcileClosingEdit->value() == 0.0) reconcileClosingEdit->setValue(d_book_cl);
		reconcileChangeEdit->setValue(d_book_cl - d_book_op);
		reconcileOpeningEdit->blockSignals(false);
		reconcileClosingEdit->blockSignals(false);
		reconcileChangeEdit->blockSignals(false);
	}
	d_rec_cl = d_rec_op + d_rec_ch;
	updateReconciliationStatLabels();
}
QString format_diff_value(double d_value, Currency *currency) {
	QString str = "<font color=";
	if(is_zero(d_value)) str += labelTransferColor.name();
	else if(d_value < 0) str += labelExpenseColor.name();
	else str += labelIncomeColor.name();
	str += ">";
	str += currency->formatValue(d_value, -1, true, true);
	str += "</font>";
	return str;
}
QString format_value(double d_value, Currency *currency) {
	return QString("<b>") + currency->formatValue(d_value) + "</b>";
}
void LedgerDialog::updateReconciliationStatLabels() {
	if(!labelExpenseColor.isValid()) labelExpenseColor = createExpenseColor(reconcileRChangeLabel);
	if(!labelIncomeColor.isValid()) labelIncomeColor = createIncomeColor(reconcileRChangeLabel);
	if(!labelTransferColor.isValid()) labelTransferColor = createTransferColor(reconcileRChangeLabel);
	reconcileRChangeLabel->setText(tr("Reconciled: %1 (%2)", "Accounting context").arg(format_value(d_rec_cl - d_rec_op, account->currency())).arg(format_diff_value(d_rec_cl - d_rec_op - reconcileChangeEdit->value(), account->currency())));
	reconcileROpeningLabel->setText(tr("Reconciled: %1 (%2)", "Accounting context").arg(format_value(d_rec_op, account->currency())).arg(format_diff_value(d_rec_op - reconcileOpeningEdit->value(), account->currency())));
	reconcileRClosingLabel->setText(tr("Reconciled: %1 (%2)", "Accounting context").arg(format_value(d_rec_cl, account->currency())).arg(format_diff_value(d_rec_cl - reconcileClosingEdit->value(), account->currency())));
	reconcileBChangeLabel->setText(tr("Book value: %1 (%2)", "Accounting context").arg(format_value(d_book_cl - d_book_op, account->currency())).arg(format_diff_value(d_book_cl - d_book_op - reconcileChangeEdit->value(), account->currency())));
	reconcileBClosingLabel->setText(tr("Book value: %1 (%2)", "Accounting context").arg(format_value(d_book_cl, account->currency())).arg(format_diff_value(d_book_cl - reconcileClosingEdit->value(), account->currency())));
	reconcileBOpeningLabel->setText(tr("Book value: %1 (%2)", "Accounting context").arg(format_value(d_book_op, account->currency())).arg(format_diff_value(d_book_op - reconcileOpeningEdit->value(), account->currency())));
}
void LedgerDialog::popupListMenu(const QPoint &p) {
	QTreeWidgetItem *qwi = transactionsView->itemAt(p);
	if(!qwi || qwi->isDisabled()) return;
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.isEmpty()) return;
	bool b = false;
	if(reconcileButton->isChecked()) {
		for(int index = 0; index < selection.size(); index++) {
			LedgerListViewItem *i = (LedgerListViewItem*) selection[index];
			if(i->b_reconciled == 0) {
				b = true;
			}
		}
	}
	if(!listMenu) {
		listMenu = new QMenu(this);
		listMenu->addAction(ActionMarkReconciled);
		listMenu->addSeparator();
		listMenu->addAction(ActionEdit);
		listMenu->addAction(ActionClone);
		listMenu->addAction(ActionJoin);
		listMenu->addAction(ActionSplit);
		listMenu->addAction(ActionEditTimestamp);
		listMenu->addSeparator();
		listMenu->addAction(ActionOpenFile);
		listMenu->addSeparator();
		listMenu->addAction(ActionDelete);
	}
	ActionMarkReconciled->setVisible(reconcileButton->isChecked());
	ActionMarkReconciled->setEnabled(b);
	listMenu->popup(transactionsView->viewport()->mapToGlobal(p));
}
void LedgerDialog::toggleReconciliation(bool b) {
	transactionsView->setColumnHidden(0, !b);
	transactionsView->setAlternatingRowColors(!b);
	reconcileWidget->setVisible(b);
	markReconciledButton->setVisible(b);
	if(b) {
		updateReconciliationStats(true, true, true);
		reconcileStartEdit->setFocus();
	} else {
		transactionsView->model()->blockSignals(true);
		QTreeWidgetItemIterator it(transactionsView, QTreeWidgetItemIterator::All);
		while(*it) {
			LedgerListViewItem *i = (LedgerListViewItem*) *it;
			i->updateMark(-1);
			++it;
		}
		transactionsView->model()->blockSignals(false);
		transactionsView->dataChanged(QModelIndex(), QModelIndex());
	}
}
void LedgerDialog::reconcileStartDateChanged(const QDate &date) {
	if(!date.isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		return;
	}
	if(date > reconcileEndEdit->date()) {
		reconcileEndEdit->blockSignals(true);
		reconcileStartEdit->blockSignals(true);
		reconcileEndEdit->setDate(date);
		QMessageBox::critical(this, tr("Error"), tr("Opening date is after closing date."));
		reconcileStartEdit->blockSignals(false);
		reconcileEndEdit->blockSignals(false);
	}
	updateReconciliationStats(false, true, true);
}
void LedgerDialog::reconcileEndDateChanged(const QDate &date) {
	if(!date.isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		return;
	}
	if(date < reconcileStartEdit->date()) {
		reconcileEndEdit->blockSignals(true);
		reconcileStartEdit->blockSignals(true);
		reconcileStartEdit->setDate(date);
		QMessageBox::critical(this, tr("Error"), tr("Closing date is before opening date."));
		reconcileStartEdit->blockSignals(false);
		reconcileEndEdit->blockSignals(false);
	}
	updateReconciliationStats(false, true, true);
}
void LedgerDialog::reconcileOpeningChanged(double) {
	re2 = re1;
	re1 = 1;
	reconcileClosingEdit->blockSignals(true);
	reconcileChangeEdit->blockSignals(true);
	if(re2 == 3) reconcileClosingEdit->setValue(reconcileOpeningEdit->value() + reconcileChangeEdit->value());
	else reconcileChangeEdit->setValue(reconcileClosingEdit->value() - reconcileOpeningEdit->value());
	reconcileClosingEdit->blockSignals(false);
	reconcileChangeEdit->blockSignals(false);
	updateReconciliationStatLabels();
}
void LedgerDialog::reconcileClosingChanged(double) {
	re2 = re1;
	re1 = 2;
	reconcileOpeningEdit->blockSignals(true);
	reconcileChangeEdit->blockSignals(true);
	if(re2 == 3) reconcileOpeningEdit->setValue(reconcileClosingEdit->value() - reconcileChangeEdit->value());
	else reconcileChangeEdit->setValue(reconcileClosingEdit->value() - reconcileOpeningEdit->value());
	reconcileOpeningEdit->blockSignals(false);
	reconcileChangeEdit->blockSignals(false);
	updateReconciliationStatLabels();
}
void LedgerDialog::reconcileChangeChanged(double) {
	re2 = re1;
	re1 = 3;
	reconcileClosingEdit->blockSignals(true);
	reconcileOpeningEdit->blockSignals(true);
	if(re2 == 2) reconcileOpeningEdit->setValue(reconcileClosingEdit->value() - reconcileChangeEdit->value());
	else reconcileClosingEdit->setValue(reconcileOpeningEdit->value() + reconcileChangeEdit->value());
	reconcileClosingEdit->blockSignals(false);
	reconcileOpeningEdit->blockSignals(false);
	updateReconciliationStatLabels();
}
void LedgerDialog::onTransactionSpacePressed(QTreeWidgetItem *i) {
	transactionActivated(i, -1);
}
void LedgerDialog::onTransactionReturnPressed(QTreeWidgetItem *i) {
	if(transactionsView->isColumnHidden(0)) edit(i, -1);
	else transactionActivated(i, -1);
}
void LedgerDialog::transactionActivated(QTreeWidgetItem *i, int c) {
	if(c == 0 || (c < 0 && !transactionsView->isColumnHidden(0))) {
		LedgerListViewItem *li = (LedgerListViewItem*) i;
		if(li->b_reconciled >= 0) {
			Transactions *trans = li->splitTransaction();
			if(!trans) trans = li->transaction();
			if(!trans) return;
			bool b = !trans->isReconciled(account);
			trans->setReconciled(account, b);
			trans->setModified();
			if(trans->isReconciled(account) == b) {
				if(trans->date() <= reconcileEndEdit->date()) {
					if(b) {
						if(trans->date() < reconcileStartEdit->date()) d_rec_op += trans->accountChange(account);
						d_rec_cl += trans->accountChange(account);
					} else {
						if(trans->date() < reconcileStartEdit->date()) d_rec_op -= trans->accountChange(account);
						d_rec_cl -= trans->accountChange(account);
					}
				}
				li->setReconciled(b, trans->date() > reconcileEndEdit->date() ? 0 : (trans->date() >= reconcileStartEdit->date() ? 1 : 2));
				updateReconciliationStatLabels();
				mainWin->setModified(true);
			}
		}
	}
}
void LedgerDialog::reconcileTransactions() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	bool b = false;
	transactionsView->model()->blockSignals(true);
	for(int index = 0; index < selection.size(); index++) {
		LedgerListViewItem *i = (LedgerListViewItem*) selection[index];
		if(i->b_reconciled == 0) {
			Transactions *trans = i->splitTransaction();
			if(!trans) trans = i->transaction();
			if(trans) {
				trans->setReconciled(account, true);
				trans->setModified();
				if(trans->isReconciled(account)) {
					if(trans->date() <= reconcileEndEdit->date()) {
						if(trans->date() < reconcileStartEdit->date()) d_rec_op += trans->accountChange(account);
						d_rec_cl += trans->accountChange(account);
					}
					i->setReconciled(true, trans->date() > reconcileEndEdit->date() ? 0 : (trans->date() >= reconcileStartEdit->date() ? 1 : 2));
					b = true;
				}
			}
		}
	}
	transactionsView->model()->blockSignals(false);
	if(b) {
		transactionsView->dataChanged(QModelIndex(), QModelIndex());
		updateReconciliationStatLabels();
		mainWin->setModified(true);
	}
}
void LedgerDialog::markAsReconciled() {
	QTreeWidgetItemIterator it(transactionsView, QTreeWidgetItemIterator::All);
	QDate d_end = reconcileEndEdit->date();
	if(!d_end.isValid()) return;
	bool b_started = false;
	bool b = false;
	transactionsView->model()->blockSignals(true);
	while(*it) {
		LedgerListViewItem *i = (LedgerListViewItem*) *it;
		if(i->b_reconciled == 0) {
			Transactions *trans = i->transaction();
			if(i->splitTransaction()) trans = i->splitTransaction();
			if(trans && (b_started || trans->date() <= d_end)) {
				b_started = true;
				trans->setReconciled(account, true);
				trans->setModified();
				if(trans->isReconciled(account)) {
					if(trans->date() <= reconcileEndEdit->date()) {
						if(trans->date() < reconcileStartEdit->date()) d_rec_op += trans->accountChange(account);
						d_rec_cl += trans->accountChange(account);
					}
					i->setReconciled(true, trans->date() > reconcileEndEdit->date() ? 0 : (trans->date() >= reconcileStartEdit->date() ? 1 : 2));
					b = true;
				}
			}
		}
		++it;
	}
	transactionsView->model()->blockSignals(false);
	if(b) {
		transactionsView->dataChanged(QModelIndex(), QModelIndex());
		updateReconciliationStatLabels();
		mainWin->setModified(true);
	}
}

void LedgerDialog::popupHeaderMenu(const QPoint &p) {
	if(!headerMenu) {
		headerMenu = new QMenu(this);
		QTreeWidgetItem *header = transactionsView->headerItem();
		QAction *a = headerMenu->addAction(header->text(2));
		a->setProperty("column_index", QVariant::fromValue(2));
		a->setCheckable(true);
		a->setChecked(!transactionsView->isColumnHidden(2));
		connect(a, SIGNAL(toggled(bool)), this, SLOT(hideColumn(bool)));
		a = headerMenu->addAction(header->text(3));
		a->setProperty("column_index", QVariant::fromValue(3));
		a->setCheckable(true);
		a->setChecked(!transactionsView->isColumnHidden(3));
		connect(a, SIGNAL(toggled(bool)), this, SLOT(hideColumn(bool)));
		a = headerMenu->addAction(header->text(4));
		a->setProperty("column_index", QVariant::fromValue(4));
		a->setCheckable(true);
		a->setChecked(!transactionsView->isColumnHidden(4));
		connect(a, SIGNAL(toggled(bool)), this, SLOT(hideColumn(bool)));
		if(b_extra) {
			a = headerMenu->addAction(header->text(5));
			a->setProperty("column_index", QVariant::fromValue(5));
			a->setCheckable(true);
			a->setChecked(!transactionsView->isColumnHidden(5));
			connect(a, SIGNAL(toggled(bool)), this, SLOT(hideColumn(bool)));
		}
		a = headerMenu->addAction(header->text(6));
		a->setCheckable(true);
		a->setProperty("column_index", QVariant::fromValue(6));
		a->setChecked(!transactionsView->isColumnHidden(6));
		connect(a, SIGNAL(toggled(bool)), this, SLOT(hideColumn(bool)));
		a = headerMenu->addAction(header->text(7));
		a->setCheckable(true);
		a->setProperty("column_index", QVariant::fromValue(7));
		a->setChecked(!transactionsView->isColumnHidden(7));
		connect(a, SIGNAL(toggled(bool)), this, SLOT(hideColumn(bool)));
		headerMenu->addSeparator();
		a = headerMenu->addAction(tr("Ascending order"));
		a->setCheckable(true);
		a->setChecked(b_ascending);
		connect(a, SIGNAL(toggled(bool)), this, SLOT(ascendingToggled(bool)));

	}
	headerMenu->popup(transactionsView->header()->viewport()->mapToGlobal(p));
}

void LedgerDialog::ascendingToggled(bool b) {
	b_ascending = b;
	transactionsView->verticalScrollBar()->setValue(0);
	updateTransactions();
}

void LedgerDialog::hideColumn(bool do_show) {
	transactionsView->setColumnHidden(sender()->property("column_index").toInt(), !do_show);
}

void LedgerDialog::saveConfig(bool only_state) {
	QSettings settings;
	if(!only_state) settings.setValue("Ledger/size", size());
	settings.setValue("Ledger/listState", transactionsView->header()->saveState());
	if(!only_state) settings.setValue("Ledger/ascending", b_ascending);
}
void LedgerDialog::updateColumnWidths() {
	setColumnDateWidth(transactionsView, 1);
	setColumnTextWidth(transactionsView, 2, tr("Debt Payment"));
	setColumnStrlenWidth(transactionsView, 3, 20);
	setColumnStrlenWidth(transactionsView, 4, 20);
	setColumnStrlenWidth(transactionsView, 5, 20);
	setColumnStrlenWidth(transactionsView, 6, 10);
	setColumnStrlenWidth(transactionsView, 7, 20);
	setColumnMoneyWidth(transactionsView, 8, budget);
	setColumnMoneyWidth(transactionsView, 9, budget);
	setColumnMoneyWidth(transactionsView, 10, budget);
	setColumnMoneyWidth(transactionsView, 11, budget, 999999999.99);
	min_width_1 = transactionsView->columnWidth(0) + transactionsView->columnWidth(1) + transactionsView->columnWidth(2) + transactionsView->columnWidth(3) + transactionsView->columnWidth(4) + transactionsView->columnWidth(9) + transactionsView->columnWidth(10) + transactionsView->columnWidth(11) + 30;
	min_width_2 = min_width_1 + transactionsView->columnWidth(8);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
#	if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
	QScreen *scr = screen();
#	else
	QScreen *scr = QGuiApplication::screenAt(pos());
#	endif
	if(!scr) scr = QGuiApplication::primaryScreen();
	QRect rect = scr->availableGeometry();
#else
	QRect rect = QApplication::desktop()->availableGeometry(this);
#endif
	if(rect.width() > min_width_1 * 1.2) transactionsView->setMinimumWidth(min_width_1);
}
void LedgerDialog::accountChanged() {
	editAccountButton->setEnabled(account != NULL);
	if(!account) return;
	bool b_loan = (account->accountType() == ASSETS_TYPE_LIABILITIES || account->accountType() == ASSETS_TYPE_CREDIT_CARD);
	transactionsView->setColumnHidden(8, !b_loan);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
#	if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
	QScreen *scr = screen();
#	else
	QScreen *scr = QGuiApplication::screenAt(pos());
#	endif
	if(!scr) scr = QGuiApplication::primaryScreen();
	QRect rect = scr->availableGeometry();
#else
	QRect rect = QApplication::desktop()->availableGeometry(this);
#endif
	if(rect.width() > min_width_2 * 1.2) transactionsView->setMinimumWidth(b_loan ? min_width_2 : min_width_1);
	ActionNewDebtInterest->setVisible(b_loan);
	ActionNewDebtPayment->setVisible(b_loan);
	transactionsView->verticalScrollBar()->setValue(0);
	updateTransactions(true);
	reconcileOpeningEdit->setCurrency(account->currency());
	reconcileChangeEdit->setCurrency(account->currency());
	reconcileClosingEdit->setCurrency(account->currency());
}
void LedgerDialog::accountActivated(int index) {
	if(accountCombo->itemData(index).isValid()) account = (AssetsAccount*) accountCombo->itemData(index).value<void*>();
	accountChanged();
}
void LedgerDialog::updateAccounts() {
	accountCombo->blockSignals(true);
	accountCombo->clear();
	bool show_balancing = false;
	for(TransactionList<Transfer*>::const_iterator it = budget->transfers.constBegin(); it != budget->transfers.constEnd(); ++it) {
		if((*it)->to() == budget->balancingAccount || (*it)->from() == budget->balancingAccount) {
			show_balancing = true;
			break;
		}
	}
	int i = 0;
	bool account_found = false;
	for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
		AssetsAccount *aaccount = *it;
		if((show_balancing || aaccount != budget->balancingAccount) && aaccount->accountType() != ASSETS_TYPE_SECURITIES) {
			accountCombo->addItem(aaccount->name(), QVariant::fromValue((void*) aaccount));
			if(aaccount == account) {
				accountCombo->setCurrentIndex(i);
				account_found = true;
			}
			i++;
		}
	}
	if(!account_found) {
		if(accountCombo->count() == 0) {
			close();
			return;
		}
		accountActivated(0);
	} else {
		updateTransactions();
	}
	accountCombo->blockSignals(false);
}
extern QString last_document_directory;
void LedgerDialog::saveView() {
	if(transactionsView->topLevelItemCount() == 0) {
		QMessageBox::critical(this, tr("Error"), tr("Empty transaction list."));
		return;
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
	ofile.setDirectWriteFallback(true);
	ofile.open(QIODevice::WriteOnly);
	if(!ofile.isOpen()) {
		ofile.cancelWriting();
		QMessageBox::critical(this, tr("Error"), tr("Couldn't open file for writing."));
		return;
	}
	last_document_directory = fileDialog.directory().absolutePath();
	QTextStream stream(&ofile);
	exportList(stream, filetype);
	if(!ofile.commit()) {
		QMessageBox::critical(this, tr("Error"), tr("Error while writing file; file was not saved."));
		return;
	}

}
void LedgerDialog::onFilterSelected(QString filter) {
	QMimeDatabase db;
	QFileDialog *fileDialog = qobject_cast<QFileDialog*>(sender());
	if(filter == db.mimeTypeForName("text/csv").filterString()) {
		fileDialog->setDefaultSuffix(db.mimeTypeForName("text/csv").preferredSuffix());
	} else {
		fileDialog->setDefaultSuffix(db.mimeTypeForName("text/html").preferredSuffix());
	}
}
bool LedgerDialog::exportList(QTextStream &outf, int fileformat, QDate first_date, QDate last_date) {
	switch(fileformat) {
		case 'h': {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
			outf.setCodec("UTF-8");
#endif
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
			outf << "\t\t\t\t\t";
			QTreeWidgetItem *header = transactionsView->headerItem();
			for(int index_v = 0; index_v <= 11; index_v++) {
				int index = transactionsView->header()->logicalIndex(index_v);
				if(index > 0 && !transactionsView->isColumnHidden(index)) outf << "<th>" << htmlize_string(header->text(index)) << "</th>";
			}
			outf << "\n";
			outf << "\t\t\t\t</tr>" << '\n';
			outf << "\t\t\t</thead>" << '\n';
			outf << "\t\t\t<tbody>" << '\n';
			QTreeWidgetItemIterator it(transactionsView);
			LedgerListViewItem *i = (LedgerListViewItem*) *it;
			while(i) {
				bool include = true;
				Transactions *transs = i->transaction();
				if(!transs) transs = i->splitTransaction();
				if(first_date.isValid() && (!transs || transs->date() < first_date)) include = false;
				if(include && last_date.isValid() && transs && transs->date() > last_date) include = false;
				if(include) {
					outf << "\t\t\t\t<tr>" << '\n';
					outf << "\t\t\t\t\t";
					for(int index_v = 0; index_v <= 11; index_v++) {
						int index = transactionsView->header()->logicalIndex(index_v);
						if(index > 0 && !transactionsView->isColumnHidden(index)) {
							if(index == 1) outf << "<td nowrap>" << htmlize_string(i->text(index)) << "</td>";
							else if(index >= 8) outf << "<td nowrap align=\"right\">" << htmlize_string(i->text(index)) << "</td>";
							else outf << "<td>" << htmlize_string(i->text(index)) << "</td>";
						}
					}
					outf << "\n";
					outf << "\t\t\t\t</tr>" << '\n';
				}
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
			for(int index = 1; index <= 11; index++) {
				if(!transactionsView->isColumnHidden(index)) {
					if(index > 1) outf << ",";
					outf << "\"" << header->text(index) << "\"";
				}
			}
			outf << "\n";
			QTreeWidgetItemIterator it(transactionsView);
			LedgerListViewItem *i = (LedgerListViewItem*) *it;
			while(i) {
				bool include = true;
				Transactions *transs = i->transaction();
				if(!transs) transs = i->splitTransaction();
				if(first_date.isValid() && (!transs || transs->date() < first_date)) include = false;
				if(include && last_date.isValid() && transs && transs->date() > last_date) include = false;
				if(include) {
					for(int index = 1; index <= 11; index++) {
						if(!transactionsView->isColumnHidden(index)) {
							if(index > 1) outf << ",";
							if(index >= 8) outf << "\"" << i->text(index).replace("−", "-").remove(" ") << "\"";
							else if(index == 6) outf << "\"" << i->text(index).replace("\"", "\'") << "\"";
							else outf << "\"" << i->text(index) << "\"";
						}
					}
					outf << "\n";
				}
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
	QDate first_date, last_date = QDate::currentDate();
	QTreeWidgetItemIterator it(transactionsView);
	LedgerListViewItem *i = (LedgerListViewItem*) *it;
	Transactions *transs = NULL, *prev_transs = NULL;
	while(i) {
		prev_transs = transs;
		transs = i->transaction();
		if(!transs) transs = i->splitTransaction();
		++it;
		i = (LedgerListViewItem*) *it;
	}
	if(!transs) transs = prev_transs;
	if(transs) first_date = transs->date();
	bool run_print = true;
	if(first_date.isValid()) {
		QDialog *dialog = new QDialog(this);
		dialog->setWindowTitle(tr("Select Time Period"));
		QVBoxLayout *box1 = new QVBoxLayout(dialog);
		QGridLayout *grid = new QGridLayout();
		box1->addLayout(grid);
		grid->addWidget(new QLabel(tr("From:"), dialog), 0, 0);
		QDateEdit *dateFromEdit = new EqonomizeDateEdit(first_date, dialog);
		dateFromEdit->setDateRange(first_date, QDate::currentDate());
		dateFromEdit->setCalendarPopup(true);
		grid->addWidget(dateFromEdit, 0, 1);
		grid->addWidget(new QLabel(tr("To:"), dialog), 1, 0);
		QDateEdit *dateToEdit = new EqonomizeDateEdit(QDate::currentDate(), dialog);
		dateToEdit->setDateRange(first_date, QDate::currentDate());
		dateToEdit->setCalendarPopup(true);
		grid->addWidget(dateToEdit, 1, 1);
		QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, Qt::Horizontal, dialog);
		buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
		buttonBox->button(QDialogButtonBox::Cancel)->setAutoDefault(false);
		buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
		connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), dialog, SLOT(reject()));
		connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(accept()));
		box1->addWidget(buttonBox);
		run_print = false;
		while(dialog->exec() == QDialog::Accepted) {
			if(dateFromEdit->date() <= first_date) first_date = QDate();
			else first_date = dateFromEdit->date();
			last_date = dateToEdit->date();
			if(last_date < dateFromEdit->date()) {
				QMessageBox::critical(this, tr("Error"), tr("To date is before from date."));
			} else {
				run_print = true;
				break;
			}
		}
		dialog->deleteLater();
	}
	if(run_print) {
		QPrinter printer;
		QPrintDialog print_dialog(&printer, this);
		if(print_dialog.exec() == QDialog::Accepted) {
			QString str;
			QTextStream stream(&str, QIODevice::WriteOnly);
			exportList(stream, 'h', first_date, last_date);
			QTextDocument htmldoc;
			htmldoc.setHtml(str);
			htmldoc.print(&printer);
		}
	}
}
void LedgerDialog::joinTransactions() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	MultiItemTransaction *split = NULL;
	QString payee;
	QString file;
	QList<Transaction*> sel_bak;
	bool use_payee = true;
	for(int index = 0; index < selection.size(); index++) {
		LedgerListViewItem *i = (LedgerListViewItem*) selection[index];
		if(!i->splitTransaction()) {
			Transaction *trans = i->transaction();
			if(trans && !trans->parentSplit()) {
				sel_bak << trans;
				if(!split) {
					split = new MultiItemTransaction(budget, i->transaction()->date(), account);
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
	if(use_payee && !payee.isEmpty()) split->setPayee(payee);
	if(mainWin->editSplitTransaction(split, this, true)) {
		delete split;
		for(int index = 0; index < sel_bak.size(); index++) {
			Transaction *trans = sel_bak.at(index);
			budget->removeTransaction(trans, true);
			mainWin->transactionRemoved(trans);
			delete trans;
		}
	} else {
		delete split;
	}
}
void LedgerDialog::splitUpTransaction() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	LedgerListViewItem *i = (LedgerListViewItem*) selection.first();
	if(i) {
		if(i->splitTransaction() && i->splitTransaction()->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ITEMS) mainWin->splitUpTransaction(i->splitTransaction());
		else if(i->transaction() && i->transaction()->parentSplit() && i->transaction()->parentSplit()->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ITEMS) mainWin->splitUpTransaction(i->transaction()->parentSplit());
	}
}
void LedgerDialog::transactionSelectionChanged() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	bool selected = !selection.isEmpty();
	bool b_join = selected;
	bool b_split = selected;
	bool b_edit = selected;
	bool b_remove = selected;
	bool b_file = selected;
	bool b_clone = selection.size() == 1;
	SplitTransaction *split = NULL;
	for(int index = 0; index < selection.size(); index++) {
		LedgerListViewItem *i = (LedgerListViewItem*) selection[index];
		Transaction *trans = i->transaction();
		if(i->splitTransaction()) {
			if(b_edit && (index > 0 || selection.size() > 1)) b_edit = false;
			b_join = false;
			if(b_split) b_split = (selection.count() == 1) && (i->splitTransaction()->type() != SPLIT_TRANSACTION_TYPE_LOAN);
			if(b_file) b_file = (selection.count() == 1) && !i->splitTransaction()->associatedFile().isEmpty();

		} else if(!trans) {
			if(selection.size() > 1) b_edit = false;
			b_remove = false;
			b_join = false;
			b_split = false;
			b_file = false;
			b_clone = false;
			break;
		} else {
			if((b_join || b_edit) && trans->subtype() == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND) {
				b_join = false;
				if(index > 0 || selection.size() > 1) b_edit = false;
			}
			if(b_edit && trans->parentSplit() && (index > 0 || selection.size() > 1)) b_edit = false;
			if(b_split) {
				if(!split) split = trans->parentSplit();
				if(!trans->parentSplit() || trans->parentSplit() != split || trans->parentSplit()->type() == SPLIT_TRANSACTION_TYPE_LOAN) b_split = false;
			}
			if(b_join && trans->parentSplit()) b_join = false;
			if(b_file) b_file = (selection.count() == 1) && (!trans->associatedFile().isEmpty() || (trans->parentSplit() && !trans->parentSplit()->associatedFile().isEmpty()));
		}
	}
	if(b_join && selection.size() == 1) b_join = false;
	joinButton->setEnabled(b_join);
	ActionJoin->setEnabled(b_join);
	splitUpButton->setEnabled(b_split);
	ActionSplit->setEnabled(b_split);
	removeButton->setEnabled(b_remove);
	ActionDelete->setEnabled(b_remove);
	editButton->setEnabled(b_edit);
	ActionEdit->setEnabled(b_edit);
	ActionClone->setEnabled(b_clone);
	ActionOpenFile->setEnabled(b_file);
	ActionEditTimestamp->setEnabled(b_edit);
	if(selection.size() > 1) {
		double v = 0.0, total_balance = 0.0, previous_balance = 0.0;
		int quantity = 0;
		QDate first_date, last_date;
		LedgerListViewItem *i_prev = NULL;
		bool b_continuous = true, b_initial = false;
		bool b_reverse = transactionsView->itemAbove(selection[0]) != selection[1];
		for(int index = (b_reverse ? selection.size() - 1 : 0); b_reverse ? index >= 0 : index < selection.size(); b_reverse ? index-- : index++) {
			LedgerListViewItem *i = (LedgerListViewItem*) selection[index];
			if(b_continuous && i_prev && transactionsView->itemAbove(i_prev) != i) b_continuous = false;
			i_prev = i;
			Transactions *trans = i->transaction();
			if(i->splitTransaction()) trans = i->splitTransaction();
			if(trans) {
				if(!i->b_other_account) {
					v += trans->accountChange(account);
					quantity++;
					if(b_continuous) {
						if(last_date.isValid() && trans->date() != last_date) total_balance += previous_balance * last_date.daysTo(trans->date());
						previous_balance = i->d_balance;
						last_date = trans->date();
						if(!first_date.isValid()) first_date = last_date;
					}
				}
			} else {
				v += i->d_balance;
				b_initial = true;
			}
		}
		if(b_continuous) {
			total_balance += previous_balance;
			if(first_date.isValid() && first_date != last_date) total_balance /= first_date.daysTo(last_date) + 1;
		}
		if(quantity + b_initial > 1) {
			if(b_continuous) statLabel->setText(QString("<div align=\"right\"><b>%1</b> %4 &nbsp; <b>%2</b> %5 &nbsp; <b>%3</b> %6</div>").arg(tr("Balance change:", "Account balance")).arg(tr("Average balance:", "Account balance")).arg(tr("Number of transactions:")).arg(account->currency()->formatValue(v)).arg(account->currency()->formatValue(total_balance)).arg(budget->formatValue(quantity, 0)));
			else statLabel->setText(QString("<div align=\"right\"><b>%1</b> %3 &nbsp; <b>%2</b> %4").arg(tr("Balance change:", "Account balance")).arg(tr("Number of transactions:")).arg(account->currency()->formatValue(v)).arg(budget->formatValue(quantity, 0)));
		} else {
			statLabel->setText(stat_total_text);
		}
	} else {
		statLabel->setText(stat_total_text);
	}
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
	mainWin->newMultiItemTransaction(this, account);
}
void LedgerDialog::newDebtPayment() {
	mainWin->newDebtPayment(this, account, false);
}
void LedgerDialog::newDebtInterest() {
	mainWin->newDebtPayment(this, account, true);
}
void LedgerDialog::remove() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() > 1) {
		if(QMessageBox::warning(this, tr("Delete transactions?"), tr("Are you sure you want to delete all (%1) selected transactions?").arg(selection.count()), QMessageBox::Ok | QMessageBox::Cancel) != QMessageBox::Ok) {
			return;
		}
		mainWin->startBatchEdit();
	}
	transactionsView->clearSelection();
	for(int index = 0; index < selection.size(); index++) {
		LedgerListViewItem *i = (LedgerListViewItem*) selection[index];
		if(i->splitTransaction()) {
			SplitTransaction *split = i->splitTransaction();
			budget->removeSplitTransaction(split, true);
			mainWin->transactionRemoved(split, NULL, true);
			delete split;
		} else if(i->transaction()) {
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
	if(selection.count() > 1) mainWin->endBatchEdit();
}
void LedgerDialog::openAssociatedFile() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() > 0) {
		LedgerListViewItem *i = (LedgerListViewItem*) selection.first();
		Transactions *transs = i->transaction();
		if(i->splitTransaction()) transs = i->splitTransaction();
		else if(i->transaction()->parentSplit() && transs->associatedFile().isEmpty()) transs = i->transaction()->parentSplit();
		if(transs) {
			open_file_list(transs->associatedFile());
		}
	}
}
void LedgerDialog::editTimestamp() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	QList<Transactions*> trans;
	for(int index = 0; index < selection.size(); index++) {
		LedgerListViewItem *i = (LedgerListViewItem*) selection[index];
		if(i->splitTransaction()) trans << i->splitTransaction();
		else if(i->transaction()) trans << i->transaction();
	}
	mainWin->editTimestamp(trans, this);
}
void LedgerDialog::cloneTransaction() {
	QList<QTreeWidgetItem*> selection = transactionsView->selectedItems();
	if(selection.count() == 1) {
		LedgerListViewItem *i = (LedgerListViewItem*) selection.first();
		if(i->splitTransaction()) mainWin->editSplitTransaction(i->splitTransaction(), this, true);
		else if(i->transaction()->parentSplit()) mainWin->editSplitTransaction(i->transaction()->parentSplit(), this, false, true);
		else if(i->transaction()) mainWin->editTransaction(i->transaction(), this, true);
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
		bool warned1 = false, warned2 = false, warned3 = false;
		bool equal_date = true, equal_description = true, equal_value = true, equal_category = true, equal_payee = b_extra;
		int transtype = -1;
		Transaction *comptrans = NULL;
		Account *compcat = NULL;
		QDate compdate;
		for(int index = 0; index < selection.size(); index++) {
			LedgerListViewItem *i = (LedgerListViewItem*) selection[index];
			if(i->transaction() && !i->transaction()->parentSplit()) {
				if(!comptrans) {
					comptrans = i->transaction();
					compdate = i->transaction()->date();
					transtype = i->transaction()->type();
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
					if(transtype >= 0 && i->transaction()->type() != transtype) {
						transtype = -1;
					}
					if(equal_date && (compdate != i->transaction()->date())) {
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
		}
		MultipleTransactionsEditDialog *dialog = new MultipleTransactionsEditDialog(b_extra, transtype, budget, this);
		LedgerListViewItem *i = (LedgerListViewItem*) transactionsView->currentItem();
		if(!i || !i->isSelected()) i = (LedgerListViewItem*) selection.first();
		if(i && i->transaction()) {
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
			mainWin->startBatchEdit();
			for(int index = 0; index < selection.size(); index++) {
				LedgerListViewItem *i = (LedgerListViewItem*) selection[index];
				if(i->transaction() && !i->transaction()->parentSplit()) {
					if(!warned1 && (i->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || i->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL)) {
						if(dialog->valueButton && dialog->valueButton->isChecked()) {
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
					Transaction *trans = i->transaction();
					Transaction *oldtrans = trans->copy();
					if(dialog->modifyTransaction(trans)) {
						if(future) {
							budget->removeTransaction(trans, true);
							mainWin->transactionRemoved(trans);
							ScheduledTransaction *strans = new ScheduledTransaction(budget, trans, NULL);
							budget->addScheduledTransaction(strans);
							mainWin->transactionAdded(strans);
						} else {
							mainWin->transactionModified(trans, oldtrans);
						}
					}
					delete oldtrans;
				}
			}
			mainWin->endBatchEdit();
			//transactionSelectionChanged();
		}
		dialog->deleteLater();
	}
}
void LedgerDialog::edit(QTreeWidgetItem *qwi, int c) {
	if(c != 0 && qwi) {
		LedgerListViewItem *i = (LedgerListViewItem*) qwi;
		if(i->splitTransaction()) mainWin->editSplitTransaction(i->splitTransaction(), this);
		else if(!i->transaction()) mainWin->editAccount(account, this);
		else if(i->transaction()->parentSplit()) mainWin->editSplitTransaction(i->transaction()->parentSplit(), this);
		else if(i->transaction()) mainWin->editTransaction(i->transaction(), this);
	}
}
void LedgerDialog::updateTransactions(bool update_reconciliation_date) {
	expenseColor = QColor();
	incomeColor = QColor();
	int scroll_h = transactionsView->horizontalScrollBar()->value();
	int scroll_v = transactionsView->verticalScrollBar()->value();
	if(b_ascending) scroll_v = transactionsView->verticalScrollBar()->maximum() - scroll_v;
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
	double total_balance = 0.0;
	double previous_balance = 0.0;
	QDate previous_date;
	double reductions = 0.0;
	double expenses = 0.0;
	int quantity = 0;
	QDate curdate = QDate::currentDate();
	bool b_reconciling = reconcileButton->isChecked();

	if(balance != 0.0) {
		LedgerListViewItem *i = new LedgerListViewItem(NULL, NULL, transactionsView, "-", "-", tr("Opening balance", "Account balance"), "-", "-", QString(), QString(), QString(), QString(), account->currency()->formatValue(balance));
		i->d_balance = balance;
		previous_balance = balance;
		i->setReconciled(-1, b_reconciling ? 2 : -2);
	}

	int trans_index = 0;
	int split_index = 0;
	Transaction *trans = NULL;
	if(trans_index < budget->transactions.size()) trans = budget->transactions.at(trans_index);
	SplitTransaction *split = NULL;
	if(split_index < budget->splitTransactions.size()) split = budget->splitTransactions.at(split_index);
	Transactions *transs = trans;
	if(!transs || (split && split->date() < trans->date())) transs = split;
	QVector<SplitTransaction*> splits;
	bool last_reconciled = true;
	QDate rec_date, first_date;
	LedgerListViewItem *selected_item = NULL;
	while(transs) {
		if(transs == split) {
			if(split->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ITEMS && ((MultiItemTransaction*) split)->account() == account) {
				double value = split->accountChange(account);
				balance += value;
				if(previous_date.isValid() && split->date() != previous_date) total_balance += previous_balance * previous_date.daysTo(split->date());
				previous_balance = balance;
				previous_date = split->date();
				if(!first_date.isValid()) first_date = previous_date;
				LedgerListViewItem *i = new LedgerListViewItem(NULL, split, NULL, QLocale().toString(split->date(), QLocale::ShortFormat), tr("Split Transaction"), split->description(), ((MultiItemTransaction*) split)->fromAccountsString(), split->payeeText(), split->tagsText(), split->comment(), (value >= 0.0) ? account->currency()->formatValue(value) : QString(), (value < 0.0) ? account->currency()->formatValue(-value) : QString(), account->currency()->formatValue(balance));
				if(b_ascending) transactionsView->addTopLevelItem(i);
				else transactionsView->insertTopLevelItem(0, i);
				i->setColors();
				i->d_balance = balance;
				i->setReconciled(split->isReconciled(account), b_reconciling ? 2 : -2);
				if(split->isReconciled(account)) {
					last_reconciled = true;
				} else if(last_reconciled) {
					rec_date = split->date();
					last_reconciled = false;
				}
				quantity++;
				if(split == selected_split) {
					selected_item = i;
				}
				if(!split->associatedFile().isEmpty()) i->setIcon(value >= 0 ? 9 : 10, LOAD_ICON_STATUS("mail-attachment"));
			} else if(split->type() == SPLIT_TRANSACTION_TYPE_LOAN) {
				if(((DebtPayment*) split)->loan() == account) {
					DebtPayment *lsplit = (DebtPayment*) split;
					Transaction *ltrans = lsplit->paymentTransaction();
					bool b_tb = false;
					if(ltrans) {
						b_tb = true;
						double value = ltrans->toValue();
						balance += value;
						reductions += value;
						LedgerListViewItem *i = new LedgerListViewItem(ltrans, NULL, NULL, QLocale().toString(split->date(), QLocale::ShortFormat), tr("Debt Payment"), tr("Reduction"), ltrans->fromAccount()->name(), lsplit->loan()->maintainer(), ltrans->tagsText(true), lsplit->comment(), (value >= 0.0) ? account->currency()->formatValue(value) : QString(), value >= 0.0 ? QString() : account->currency()->formatValue(-value), account->currency()->formatValue(balance));
						if(b_ascending) transactionsView->addTopLevelItem(i);
						else transactionsView->insertTopLevelItem(0, i);
						i->setColors();
						i->d_balance = balance;
						i->setReconciled(ltrans->isReconciled(account), b_reconciling ? 2 : -2);
						if(ltrans->isReconciled(account)) {
							last_reconciled = true;
						} else if(last_reconciled) {
							rec_date = ltrans->date();
							last_reconciled = false;
						}
						if(ltrans == selected_transaction) {
							selected_item = i;
						}
						quantity++;
						if(!split->associatedFile().isEmpty()) i->setIcon(value >= 0 ? 9 : 10, LOAD_ICON_STATUS("mail-attachment"));
					}
					ltrans = lsplit->feeTransaction();
					if(ltrans) {
						double value = ltrans->value();
						expenses += value;
						bool to_balance = (ltrans->fromAccount() == account);
						if(to_balance) {
							b_tb = true;
							balance -= value;
							reductions -= value;
							quantity++;
						}
						LedgerListViewItem *i = new LedgerListViewItem(ltrans, NULL, NULL, QLocale().toString(split->date(), QLocale::ShortFormat), tr("Debt Payment"), tr("Fee"), ltrans->fromAccount()->name(), ((DebtFee*) ltrans)->payee(), ltrans->tagsText(true), lsplit->comment(), (to_balance || value >= 0.0) ? QString() : account->currency()->formatValue(-value), (to_balance && value >= 0.0) ? account->currency()->formatValue(value) : QString(), account->currency()->formatValue(balance), to_balance ? QString() : account->currency()->formatValue(value));
						if(b_ascending) transactionsView->addTopLevelItem(i);
						else transactionsView->insertTopLevelItem(0, i);
						i->setColors();
						i->d_balance = balance;
						i->b_other_account = !to_balance;
						i->setReconciled(to_balance ? ltrans->isReconciled(account) : -1, b_reconciling ? 2 : -2);
						if(ltrans->isReconciled(account)) {
							last_reconciled = true;
						} else if(last_reconciled) {
							rec_date = ltrans->date();
							last_reconciled = false;
						}
						if(ltrans == selected_transaction) {
							selected_item = i;
						}
						if(!split->associatedFile().isEmpty()) i->setIcon(to_balance ? (value >= 0.0 ? 10 : 9) : 8, LOAD_ICON_STATUS("mail-attachment"));
					}
					ltrans = lsplit->interestTransaction();
					if(ltrans) {
						double value = ltrans->value();
						expenses += value;
						bool to_balance = (ltrans->fromAccount() == account);
						if(to_balance) {
							b_tb = true;
							balance -= value;
							reductions -= value;
							quantity++;
						}
						LedgerListViewItem *i = new LedgerListViewItem(ltrans, NULL, NULL, QLocale().toString(split->date(), QLocale::ShortFormat), tr("Debt Payment"), tr("Interest"), ltrans->fromAccount()->name(), ((DebtInterest*) ltrans)->payee(), ltrans->tagsText(true), lsplit->comment(), (to_balance || value >= 0.0) ? QString() : account->currency()->formatValue(-value), (to_balance && value >= 0.0) ? account->currency()->formatValue(value) : QString(), account->currency()->formatValue(balance), to_balance ? QString() : account->currency()->formatValue(value));
						if(b_ascending) transactionsView->addTopLevelItem(i);
						else transactionsView->insertTopLevelItem(0, i);
						i->setColors();
						i->d_balance = balance;
						i->b_other_account = !to_balance;
						i->setReconciled(to_balance ? ltrans->isReconciled(account) : -1, b_reconciling ? 2 : -2);
						if(ltrans->isReconciled(account)) {
							last_reconciled = true;
						} else if(last_reconciled) {
							rec_date = ltrans->date();
							last_reconciled = false;
						}
						if(ltrans == selected_transaction) {
							selected_item = i;
						}
						if(!split->associatedFile().isEmpty()) i->setIcon(to_balance ? (value >= 0.0 ? 10 : 9) : 8, LOAD_ICON_STATUS("mail-attachment"));
					}
					if(b_tb) {
						if(previous_date.isValid() && lsplit->date() != previous_date) total_balance += previous_balance * previous_date.daysTo(lsplit->date());
						previous_balance = balance;
						previous_date = lsplit->date();
						if(!first_date.isValid()) first_date = previous_date;
					}
				} else if(((DebtPayment*) split)->account() == account) {
					double value = split->accountChange(account);
					if(value != 0.0) {
						balance += value;
						LedgerListViewItem *i = new LedgerListViewItem(NULL, split, NULL, QLocale().toString(split->date(), QLocale::ShortFormat), tr("Debt Payment"), split->description(), ((DebtPayment*) split)->loan()->name(), ((DebtPayment*) split)->loan()->maintainer(), split->tagsText(), split->comment(), (value >= 0.0) ? account->currency()->formatValue(value) : QString(), (value < 0.0) ? account->currency()->formatValue(-value) : QString(), account->currency()->formatValue(balance));
						if(b_ascending) transactionsView->addTopLevelItem(i);
						else transactionsView->insertTopLevelItem(0, i);
						i->setColors();
						i->d_balance = balance;
						i->setReconciled(split->isReconciled(account), b_reconciling ? 2 : -2);
						if(split->isReconciled(account)) {
							last_reconciled = true;
						} else if(last_reconciled) {
							rec_date = split->date();
							last_reconciled = false;
						}
						quantity++;
						if(split == selected_split) {
							selected_item = i;
						}
						if(!split->associatedFile().isEmpty()) i->setIcon(value >= 0 ? 9 : 10, LOAD_ICON_STATUS("mail-attachment"));
					}
					if(previous_date.isValid() && split->date() != previous_date) total_balance += previous_balance * previous_date.daysTo(split->date());
					previous_balance = balance;
					previous_date = split->date();
					if(!first_date.isValid()) first_date = previous_date;
				}
			}
		} else if(transs->relatesToAccount(account) && (!trans->parentSplit() || trans->parentSplit()->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ACCOUNTS || (trans->parentSplit()->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ITEMS && ((MultiItemTransaction*) trans->parentSplit())->account() != account))) {
			double value = trans->accountChange(account);
			balance += value;
			LedgerListViewItem *i = new LedgerListViewItem(trans, NULL, NULL, QLocale().toString(trans->date(), QLocale::ShortFormat), QString(), trans->description(), account == trans->fromAccount() ? trans->toAccount()->name() : trans->fromAccount()->name(), trans->payeeText(), trans->tagsText(true), trans->comment(), (value >= 0.0) ? account->currency()->formatValue(value) : QString(), value >= 0.0 ? QString() : account->currency()->formatValue(-value), account->currency()->formatValue(balance));
			quantity++;
			if(b_ascending) transactionsView->addTopLevelItem(i);
			else transactionsView->insertTopLevelItem(0, i);
			i->setColors();
			i->setReconciled(trans->isReconciled(account), b_reconciling ? 2 : -2);
			if(trans->type() == TRANSACTION_TYPE_INCOME) {
				if(value >= 0.0) i->setText(2, tr("Income"));
				else i->setText(2, tr("Repayment"));
			} else if(trans->type() == TRANSACTION_TYPE_EXPENSE) {
				if(value <= 0.0) i->setText(2, tr("Expense"));
				else i->setText(2, tr("Refund"));
			} else if(trans->relatesToAccount(budget->balancingAccount)) {
				i->setText(2, tr("Account Balance Adjustment"));
				i->setReconciled(-1, b_reconciling ? 2 : -2);
			} else {
				i->setText(2, tr("Transfer"));
			}
			if(trans == selected_transaction) {
				selected_item = i;
			}
			i->d_balance = balance;
			if(trans->isReconciled(account)) {
				last_reconciled = true;
			} else if(last_reconciled) {
				rec_date = trans->date();
				last_reconciled = false;
			}
			if(previous_date.isValid() && trans->date() != previous_date) total_balance += previous_balance * previous_date.daysTo(trans->date());
			previous_balance = balance;
			previous_date = trans->date();
			if(!first_date.isValid()) first_date = previous_date;
			if(!trans->associatedFile().isEmpty() || (trans->parentSplit() && !trans->parentSplit()->associatedFile().isEmpty())) i->setIcon(value >= 0 ? 9 : 10, LOAD_ICON_STATUS("mail-attachment"));
		}
		if(transs == trans) {
			++trans_index;
			trans = NULL;
			if(trans_index < budget->transactions.size()) trans = budget->transactions.at(trans_index);
			if(trans && trans->date() > curdate) trans = NULL;
		} else {
			++split_index;
			split = NULL;
			if(split_index < budget->splitTransactions.size()) split = budget->splitTransactions.at(split_index);
			if(split && split->date() > curdate) split = NULL;
		}
		transs = trans;
		if(!transs || (split && (split->date() < trans->date() || (split->date() == trans->date() && split->timestamp() < trans->timestamp())))) transs = split;
	}

	if(!update_reconciliation_date) {
		if(b_ascending && scroll_v == 0) transactionsView->scrollToBottom();
		else transactionsView->verticalScrollBar()->setValue(b_ascending ? transactionsView->verticalScrollBar()->maximum() - scroll_v : scroll_v);
		transactionsView->horizontalScrollBar()->setValue(scroll_h);
	} else if(b_ascending) {
		transactionsView->scrollToBottom();
	}
	if(selected_item) {
		selected_item->setSelected(true);
		transactionsView->setCurrentItem(selected_item);
		transactionsView->scrollToItem(selected_item);
	}

	if(previous_date.isValid() && previous_date < QDate::currentDate()) previous_balance *= previous_date.daysTo(QDate::currentDate());
	total_balance += previous_balance;
	if(first_date.isValid() && first_date < QDate::currentDate()) total_balance /= (first_date.daysTo(QDate::currentDate()) + (previous_date == QDate::currentDate() ? 1 : 0));
	if(account->accountType() == ASSETS_TYPE_LIABILITIES || account->accountType() == ASSETS_TYPE_CREDIT_CARD) {
		stat_total_text = QString("<div align=\"right\"><b>%1</b> %4 &nbsp; <b>%2</b> %5 &nbsp; <b>%3</b> %6</div>").arg(tr("Current debt:")).arg(tr("Total debt reduction:")).arg(tr("Total interest and fees:")).arg(account->currency()->formatValue(-balance)).arg(account->currency()->formatValue(reductions)).arg(account->currency()->formatValue(expenses));
	} else {
		stat_total_text = QString("<div align=\"right\"><b>%1</b> %4 &nbsp; <b>%2</b> %5 &nbsp; <b>%3</b> %6</div>").arg(tr("Current balance:", "Account balance")).arg(tr("Average balance:", "Account balance")).arg(tr("Number of transactions:")).arg(account->currency()->formatValue(balance)).arg(account->currency()->formatValue(total_balance)).arg(budget->formatValue(quantity, 0));
	}
	statLabel->setText(stat_total_text);
	if(!rec_date.isValid()) {
		LedgerListViewItem *i = (LedgerListViewItem*) transactionsView->topLevelItem(0);
		if(i) {
			if(i->splitTransaction()) rec_date = i->splitTransaction()->date();
			else if(i->transaction()) rec_date = i->transaction()->date();
			if(rec_date < QDate::currentDate()) rec_date = rec_date.addDays(1);
		}
	}
	if(update_reconciliation_date && rec_date.isValid()) {
		reconcileStartEdit->blockSignals(true);
		reconcileStartEdit->setDate(rec_date);
		reconcileStartEdit->blockSignals(false);
	}
	labelExpenseColor = QColor();
	labelIncomeColor = QColor();
	labelTransferColor = QColor();
	if(b_reconciling) updateReconciliationStats(false, true, true);

}
void LedgerDialog::reject() {
	saveConfig();
	QDialog::reject();
}
void LedgerDialog::editAccount() {
	if(!account) return;
	mainWin->editAccount(account, this);
}
void LedgerDialog::search() {
	QString str = searchEdit->text().trimmed();
	if(str.isEmpty()) return;
	QTreeWidgetItem *i_first = transactionsView->currentItem();
	if(i_first && !i_first->isSelected()) i_first = NULL;
	QTreeWidgetItemIterator it(transactionsView, QTreeWidgetItemIterator::All);
	if(i_first) {
		it = QTreeWidgetItemIterator(i_first, QTreeWidgetItemIterator::All);
		++it;
		if(!(*it)) {
			i_first = NULL;
			it = QTreeWidgetItemIterator(transactionsView, QTreeWidgetItemIterator::All);
		}
	}
	transactionsView->clearSelection();
	transactionsView->setCurrentItem(NULL);
	bool wrapped_around = false;
	while(*it) {
		if(((LedgerListViewItem*) *it)->matches(str)) {
			(*it)->setSelected(true);
			transactionsView->setCurrentItem(*it, 1);
			transactionsView->scrollToItem(*it);
			break;
		}
		if(wrapped_around && *it == i_first) break;
		++it;
		if(!wrapped_around && i_first && !(*it)) {
			it = QTreeWidgetItemIterator(transactionsView, QTreeWidgetItemIterator::All);
			wrapped_around = true;
		}
	}
}
void LedgerDialog::searchPrevious() {
	QString str = searchEdit->text().trimmed();
	if(str.isEmpty()) return;
	QTreeWidgetItem *i_first = transactionsView->currentItem();
	if(i_first && !i_first->isSelected()) i_first = NULL;
	int i = transactionsView->topLevelItemCount();
	QTreeWidgetItemIterator it(transactionsView, QTreeWidgetItemIterator::All);
	if(i_first) {
		it = QTreeWidgetItemIterator(i_first, QTreeWidgetItemIterator::All);
		--it;
		if(!(*it)) {
			i_first = NULL;
			it = QTreeWidgetItemIterator(transactionsView, QTreeWidgetItemIterator::All);
			it += i - 1;
		}
	} else {
		it += i - 1;
	}
	transactionsView->clearSelection();
	transactionsView->setCurrentItem(NULL);
	bool wrapped_around = false;
	while(*it) {
		if(((LedgerListViewItem*) *it)->matches(str)) {
			(*it)->setSelected(true);
			transactionsView->setCurrentItem(*it, 1);
			transactionsView->scrollToItem(*it);
			break;
		}
		if(wrapped_around && *it == i_first) break;
		--it;
		if(!wrapped_around && i_first && !(*it)) {
			it = QTreeWidgetItemIterator(transactionsView, QTreeWidgetItemIterator::All);
			it += i - 1;
			wrapped_around = true;
		}
	}
}
void LedgerDialog::searchChanged(const QString &str) {
	searchNextButton->setEnabled(!str.trimmed().isEmpty());
	searchPreviousButton->setEnabled(!str.trimmed().isEmpty());
}

