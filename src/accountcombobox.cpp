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

#include <QDebug>
#include <QKeyEvent>
#include <QCompleter>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QPainter>

#include "accountcombobox.h"
#include "account.h"
#include "budget.h"
#include "editaccountdialogs.h"

QComboBoxListViewEq::QComboBoxListViewEq(AccountComboBox *cmb) : combo(cmb) {}
void QComboBoxListViewEq::keyPressEvent(QKeyEvent *e) {
	QString str = e->text().trimmed();
	if((!str.isEmpty() && str[0].isPrint()) || (!filter_str.isEmpty() && (e->key() == Qt::Key_Backspace || e->key() == Qt::Key_Delete))) {
		QSortFilterProxyModel *filterModel = (QSortFilterProxyModel*) model();
		if(e->key() == Qt::Key_Backspace || e->key() == Qt::Key_Delete) {
			filter_str.chop(1);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
			QRegularExpression reg(filter_str.isEmpty() ? "" : QString("\\b")+filter_str, QRegularExpression::CaseInsensitiveOption);
			filterModel->setFilterRegularExpression(reg);
#else
			QRegExp reg(filter_str.isEmpty() ? "" : QString("\\b")+filter_str, Qt::CaseInsensitive);
			filterModel->setFilterRegExp(reg);
#endif
			return;
		}
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		QRegularExpression reg(QString("\\b")+filter_str+str, QRegularExpression::CaseInsensitiveOption);
		filterModel->setFilterRegularExpression(reg);
#else
		QRegExp reg(QString("\\b")+filter_str+str, Qt::CaseInsensitive);
		filterModel->setFilterRegExp(reg);
#endif
		if(filterModel->rowCount() == 0) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
			filterModel->setFilterRegularExpression(filter_str.isEmpty() ? "" : QString("\\b")+filter_str);
#else
			filterModel->setFilterRegExp(filter_str.isEmpty() ? "" : QString("\\b")+filter_str);
#endif
		} else if(filterModel->rowCount() == 1 && filterModel->data(filterModel->index(0, 0), Qt::UserRole).value<void*>() != NULL) {
			combo->hidePopup();
		} else {
			filter_str += str;
		}
		return;
	}
	QListView::keyPressEvent(e);
}
void QComboBoxListViewEq::resizeEvent(QResizeEvent *event) {
	resizeContents(viewport()->width(), contentsSize().height());
	QListView::resizeEvent(event);
}
QStyleOptionViewItem QComboBoxListViewEq::viewOptions() const {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	QStyleOptionViewItem option;
	initViewItemOption(&option);
#else
	QStyleOptionViewItem option = QListView::viewOptions();
#endif
	option.showDecorationSelected = true;
	if(combo) option.font = combo->font();
	return option;
}
void QComboBoxListViewEq::paintEvent(QPaintEvent *e) {
	if (combo) {
		QStyleOptionComboBox opt;
		opt.initFrom(combo);
		opt.editable = combo->isEditable();
		if(combo->style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, combo)) {
			//we paint the empty menu area to avoid having blank space that can happen when scrolling
			QStyleOptionMenuItem menuOpt;
			menuOpt.initFrom(this);
			menuOpt.palette = palette();
			menuOpt.state = QStyle::State_None;
			menuOpt.checkType = QStyleOptionMenuItem::NotCheckable;
			menuOpt.menuRect = e->rect();
			menuOpt.maxIconWidth = 0;
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
			menuOpt.tabWidth = 0;
#endif
			QPainter p(viewport());
			combo->style()->drawControl(QStyle::CE_MenuEmptyArea, &menuOpt, &p, this);
		}
	}
	QListView::paintEvent(e);
}
QComboBoxDelegateEq::QComboBoxDelegateEq(QObject *parent, QComboBox *cmb) : QStyledItemDelegate(parent), mCombo(cmb) {}
bool QComboBoxDelegateEq::isSeparator(const QModelIndex &index) {
	return index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("separator");
}
void QComboBoxDelegateEq::setSeparator(QAbstractItemModel *model, const QModelIndex &index) {
	model->setData(index, QString::fromLatin1("separator"), Qt::AccessibleDescriptionRole);
	if (QStandardItemModel *m = qobject_cast<QStandardItemModel*>(model))
		if (QStandardItem *item = m->itemFromIndex(index))
			item->setFlags(item->flags() & ~(Qt::ItemIsSelectable|Qt::ItemIsEnabled));
}
void QComboBoxDelegateEq::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
	if (isSeparator(index)) {
		QRect rect = option.rect;
		if (const QAbstractItemView *view = qobject_cast<const QAbstractItemView*>(option.widget))
			rect.setWidth(view->viewport()->width());
		QStyleOption opt;
		opt.rect = rect;
		mCombo->style()->drawPrimitive(QStyle::PE_IndicatorToolBarSeparator, &opt, painter, mCombo);
	} else {
		QStyledItemDelegate::paint(painter, option, index);
	}
}
QSize QComboBoxDelegateEq::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
	if (isSeparator(index)) {
		int pm = mCombo->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, NULL, mCombo);
		return QSize(pm, pm);
	}
	return QStyledItemDelegate::sizeHint(option, index);
}

AccountComboBox::AccountComboBox(int account_type, Budget *budg, bool add_new_account_action, bool add_new_loan_action, bool add_multiple_accounts_action, bool exclude_securities_accounts, bool exclude_balancing_account, QWidget *parent, bool sort_account_types) : QComboBox(parent), i_type(account_type), budget(budg), new_account_action(add_new_account_action), new_loan_action(add_new_loan_action && i_type == ACCOUNT_TYPE_ASSETS), multiple_accounts_action(add_multiple_accounts_action && i_type == ACCOUNT_TYPE_ASSETS), b_exclude_securities(exclude_securities_accounts), b_exclude_balancing(exclude_balancing_account), b_sort(sort_account_types) {
	setEditable(false);
	added_account = NULL;
	block_account_selected = false;
	sourceModel = new QStandardItemModel(this);
	filterModel = new QSortFilterProxyModel(this);
	filterModel->setSourceModel(sourceModel);
	QListView *listView = new QComboBoxListViewEq(this);
	listView->setTextElideMode(Qt::ElideMiddle);
	setView(listView);
	setItemDelegate(new QComboBoxDelegateEq(listView, this));
	setModel(filterModel);
	connect(this, SIGNAL(activated(int)), this, SLOT(accountActivated(int)));
	connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentIndexChanged(int)));
}
AccountComboBox::~AccountComboBox() {}

void AccountComboBox::hidePopup() {
	QComboBox::hidePopup();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	filterModel->setFilterRegularExpression("");
#else
	filterModel->setFilterRegExp("");
#endif
	((QComboBoxListViewEq*) view())->filter_str = "";
	if(!currentAccount()) setCurrentIndex(firstAccountIndex() < count() ? firstAccountIndex() : -1);
}
int AccountComboBox::firstAccountIndex() const {
	int index = 0;
	if(new_account_action) index++;
	if(new_loan_action) index++;
	if(multiple_accounts_action) index++;
	if(index > 0) index++;
	return index;
}
Account *AccountComboBox::currentAccount() const {
	if(!currentData().isValid()) return NULL;
	return (Account*) currentData().value<void*>();
}
void AccountComboBox::setCurrentAccount(Account *account) {
	if(account) {
		int index = findData(QVariant::fromValue((void*) account));
		if(index >= 0) setCurrentIndex(index);
	}
}
bool AccountComboBox::setCurrentAccountId(qlonglong id) {
	for(int i = 0; i < count(); i++) {
		if(itemData(i).isValid() && itemData(i).value<void*>() && ((Account*) itemData(i).value<void*>())->id() == id) {
			setCurrentIndex(i);
			return true;
		}
	}
	return false;
}
int AccountComboBox::currentAccountIndex() const {
	int index = currentIndex() - firstAccountIndex();
	if(index < 0) return -1;
	return index;
}
void AccountComboBox::setCurrentAccountIndex(int index) {
	index += firstAccountIndex();
	if(index < count()) setCurrentIndex(index);
}

#define APPEND_ACTION_ROW(s)	{QStandardItem *row = new QStandardItem(s);\
				row->setData(QVariant::fromValue(NULL), Qt::UserRole);\
				sourceModel->appendRow(row);}
#define APPEND_ROW 		{QStandardItem *row = new QStandardItem(account->nameWithParent());\
				row->setData(QVariant::fromValue((void*) account), Qt::UserRole);\
				sourceModel->appendRow(row);}

#define APPEND_SEPARATOR	{QStandardItem *row = new QStandardItem();\
				row->setData(QVariant::fromValue(NULL), Qt::UserRole);\
				row->setData(QString::fromLatin1("separator"), Qt::AccessibleDescriptionRole);\
				row->setFlags(Qt::NoItemFlags);\
				sourceModel->appendRow(row);}

void AccountComboBox::updateAccounts(Account *exclude_account, Currency *force_currency) {
	Account *current_account = currentAccount();
	sourceModel->clear();
	switch(i_type) {
		case ACCOUNT_TYPE_INCOMES: {
			if(new_account_action) {
				APPEND_ACTION_ROW(tr("New income category…"));
			}
			for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
				IncomesAccount *account = *it;
				if(account != exclude_account) {
					if(new_account_action && count() == 1) APPEND_SEPARATOR
					APPEND_ROW
				}
			}
			if(current_account) setCurrentAccount(current_account);
			if(currentIndex() < firstAccountIndex()) {
				if(firstAccountIndex() < count()) setCurrentIndex(firstAccountIndex());
				else setCurrentIndex(-1);
			}
			break;
		}
		case ACCOUNT_TYPE_EXPENSES: {
			if(new_account_action) {
				APPEND_ACTION_ROW(tr("New expense category…"))
			}
			for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
				ExpensesAccount *account = *it;
				if(account != exclude_account) {
					if(new_account_action && count() == 1) APPEND_SEPARATOR
					APPEND_ROW
				}
			}
			if(current_account) setCurrentAccount(current_account);
			if(currentIndex() < firstAccountIndex()) {
				if(firstAccountIndex() < count()) setCurrentIndex(firstAccountIndex());
				else setCurrentIndex(-1);
			}
			break;
		}
		default: {
			if(new_account_action) APPEND_ACTION_ROW(tr("New account…"));
			if(new_loan_action) APPEND_ACTION_ROW(tr("Paid with loan…"));
			if(multiple_accounts_action) APPEND_ACTION_ROW(tr("Multiple accounts/payments…"));
			int cactions = count();
			QList<AssetsAccount*> accounts[3];
			for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
				AssetsAccount *account = *it;
				if(account != exclude_account && (!force_currency || account->currency() == force_currency)) {
					int level = 0;
					if(i_type >= 100) {
						if(account->accountType() == i_type) {
							if(account->isClosed()) level = 3;
							else level = 1;
						}
					} else if(i_type == -3) {
						if(account->accountType() == ASSETS_TYPE_CREDIT_CARD || account->accountType() == ASSETS_TYPE_LIABILITIES) {
							if(account->isClosed()) level = 3;
							else level = 1;
						}
					} else if((account->accountType() == ASSETS_TYPE_SECURITIES && !b_exclude_securities) || account->accountType() == ASSETS_TYPE_LIABILITIES || (account == budget->balancingAccount && !b_exclude_balancing) || account->isClosed()) {
						level = 3;
					} else if(account->accountType() == ASSETS_TYPE_CASH || account->accountType() == ASSETS_TYPE_CURRENT || account->accountType() == ASSETS_TYPE_CURRENT) {
						level = 1;
					} else if(account->accountType() != ASSETS_TYPE_SECURITIES && account != budget->balancingAccount) {
						if(b_sort) level = 2;
						else level = 1;
					}
					if(level > 0) {
						accounts[level - 1] << account;
					}
				}
			}
			if(!accounts[0].isEmpty()) {
				if(count() > 0 && count() == cactions) APPEND_SEPARATOR
				for(QList<AssetsAccount*>::const_iterator it = accounts[0].constBegin(); it != accounts[0].constEnd(); ++it) {
					AssetsAccount *account = *it;
					APPEND_ROW
				}
			}
			if(!accounts[1].isEmpty()) {
				if((count() > 0 && count() == cactions) || (accounts[0].count() + accounts[1].count() >= 5 && accounts[0].count() >= 2 && accounts[1].count() >= 2)) APPEND_SEPARATOR
				for(QList<AssetsAccount*>::const_iterator it = accounts[1].constBegin(); it != accounts[1].constEnd(); ++it) {
					AssetsAccount *account = *it;
					APPEND_ROW
				}
			}
			if(i_type == -1) {
				if(count() > 0 && count() >= cactions) APPEND_SEPARATOR
				for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
					IncomesAccount *account = *it;
					if(account != exclude_account) {
						APPEND_ROW
					}
				}
			}
			if(i_type == -2) {
				if(count() > 0 && count() >= cactions) APPEND_SEPARATOR
				for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
					ExpensesAccount *account = *it;
					if(account != exclude_account) {
						APPEND_ROW
					}
				}
			}
			if(!accounts[2].isEmpty()) {
				if(count() > 0 && count() >= cactions) APPEND_SEPARATOR
				for(QList<AssetsAccount*>::const_iterator it = accounts[2].constBegin(); it != accounts[2].constEnd(); ++it) {
					AssetsAccount *account = *it;
					APPEND_ROW
				}
			}
			if(current_account) setCurrentAccount(current_account);
			if(currentIndex() < firstAccountIndex()) {
				if(firstAccountIndex() < count()) setCurrentIndex(firstAccountIndex());
				else setCurrentIndex(-1);
			}
			break;
		}
	}
}
void AccountComboBox::setAccountType(int account_type) {
	i_type = account_type;
}
bool AccountComboBox::hasAccount() const {
	return count() > firstAccountIndex();
}
void AccountComboBox::createAccountSlot() {createAccount();}
Account *AccountComboBox::createAccount() {
	Account *account = NULL;
	switch(i_type) {
		case ACCOUNT_TYPE_INCOMES: {
			EditIncomesAccountDialog *dialog = new EditIncomesAccountDialog(budget, NULL, this, tr("New Income Category"));
			if(dialog->exec() == QDialog::Accepted) {
				account = dialog->newAccount();
			}
			dialog->deleteLater();
			break;
		}
		case ACCOUNT_TYPE_EXPENSES: {
			EditExpensesAccountDialog *dialog = new EditExpensesAccountDialog(budget, NULL, this, tr("New Expense Category"));
			if(dialog->exec() == QDialog::Accepted) {
				account = dialog->newAccount();
			}
			dialog->deleteLater();
			break;
		}
		default: {
			EditAssetsAccountDialog *dialog = new EditAssetsAccountDialog(budget, this, tr("New Account"), false, i_type == -3 ? ASSETS_TYPE_LIABILITIES : (i_type >= 100 ? i_type : -1), i_type >= 100);
			if(dialog->exec() == QDialog::Accepted) {
				account = dialog->newAccount();
			}
			dialog->deleteLater();
			break;
		}
	}
	if(account) {
		budget->addAccount(account);
		updateAccounts();
		setCurrentAccount(account);
		emit accountSelected(account);
	}
	return account;
}

void AccountComboBox::accountActivated(int index) {
	if(!itemData(index).isValid() || itemData(index).value<void*>() == NULL) {
		if(new_account_action && index == 0) {
			setCurrentIndex(firstAccountIndex());
			emit newAccountRequested();
			return;
		} else if(new_loan_action && ((index == 1 && new_account_action) || (index == 0 && !new_account_action))) {
			setCurrentIndex(firstAccountIndex());
			emit newLoanRequested();
			return;
		} else if(multiple_accounts_action && ((index == 2 && new_account_action && new_loan_action) || (index == 1 && !(new_account_action) != !(new_loan_action)) || (index == 0 && !new_account_action && !new_loan_action))) {
			setCurrentIndex(firstAccountIndex());
			emit multipleAccountsRequested();
			return;
		}
	}
	if(!block_account_selected) {
		Account *account = NULL;
		if(itemData(index).isValid()) account = (Account*) itemData(index).value<void*>();
		emit accountSelected(account);
	}
}
void AccountComboBox::onCurrentIndexChanged(int index) {
	Account *account = NULL;
	if(itemData(index).isValid()) account = (Account*) itemData(index).value<void*>();
	emit currentAccountChanged(account);
}
void AccountComboBox::keyPressEvent(QKeyEvent *e) {
	block_account_selected = true;
	QString str = e->text().trimmed();
	if(!str.isEmpty()) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		QRegularExpression reg(QString("\\b")+str, QRegularExpression::CaseInsensitiveOption);
#else
		QRegExp reg(QString("\\b")+str, Qt::CaseInsensitive);
#endif
		Account *account = currentAccount();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		filterModel->setFilterRegularExpression(reg);
#else
		filterModel->setFilterRegExp(reg);
#endif
		if(filterModel->rowCount() == 0) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
			filterModel->setFilterRegularExpression("");
#else
			filterModel->setFilterRegExp("");
#endif
			setCurrentAccount(account);
		} else if(filterModel->rowCount() == 1 && filterModel->data(filterModel->index(0, 0), Qt::UserRole).value<void*>() != NULL) {
			account = (Account*) filterModel->data(filterModel->index(0, 0), Qt::UserRole).value<void*>();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
			filterModel->setFilterRegularExpression("");
#else
			filterModel->setFilterRegExp("");
#endif
			setCurrentAccount(account);
		} else {
			((QComboBoxListViewEq*) view())->filter_str = str;
			showPopup();
		}
		return;
	}
	QComboBox::keyPressEvent(e);
	block_account_selected = false;
	if(!e->isAccepted() && (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return)) emit returnPressed();
}
void AccountComboBox::focusAndSelectAll() {
	setFocus();
}

