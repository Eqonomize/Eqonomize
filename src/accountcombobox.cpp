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
			QRegExp reg(filter_str.isEmpty() ? "" : QString("\\b")+filter_str, Qt::CaseInsensitive);
			filterModel->setFilterRegExp(reg);
			return;
		}
		QRegExp reg(QString("\\b")+filter_str+str, Qt::CaseInsensitive);
		filterModel->setFilterRegExp(reg);
		if(filterModel->rowCount() == 0) {
			filterModel->setFilterRegExp(filter_str.isEmpty() ? "" : QString("\\b")+filter_str);
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
	QStyleOptionViewItem option = QListView::viewOptions();
	option.showDecorationSelected = true;
	if (combo)
		option.font = combo->font();
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
			menuOpt.tabWidth = 0;
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

AccountComboBox::AccountComboBox(int account_type, Budget *budg, bool add_new_account_action, bool add_new_loan_action, bool add_multiple_accounts_action, bool exclude_securities_accounts, bool exclude_balancing_account, QWidget *parent) : QComboBox(parent), i_type(account_type), budget(budg), new_account_action(add_new_account_action), new_loan_action(add_new_loan_action && i_type == ACCOUNT_TYPE_ASSETS), multiple_accounts_action(add_multiple_accounts_action && i_type == ACCOUNT_TYPE_ASSETS), b_exclude_securities(exclude_securities_accounts), b_exclude_balancing(exclude_balancing_account) {
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
	filterModel->setFilterRegExp("");
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
				row->setData(QVariant::fromValue(QVariant::fromValue(NULL)), Qt::UserRole);\
				sourceModel->appendRow(row);}
#define APPEND_ROW 		{QStandardItem *row = new QStandardItem(account->nameWithParent());\
				row->setData(QVariant::fromValue(QVariant::fromValue((void*) account)), Qt::UserRole);\
				sourceModel->appendRow(row);}

#define APPEND_SEPARATOR	{QStandardItem *row = new QStandardItem();\
				row->setData(QVariant::fromValue(QVariant::fromValue(NULL)), Qt::UserRole);\
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
			bool add_secondary_list = false;
			for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
				AssetsAccount *account = *it;
				if(account != exclude_account && (!force_currency || account->currency() == force_currency)) {
					bool b_add = false;
					if(i_type >= 100) {
						if(account->accountType() == i_type) {
							if(account->isClosed()) add_secondary_list = true;
							else b_add = true;
						}
					} else if(i_type == -3) {
						if(account->accountType() == ASSETS_TYPE_CREDIT_CARD || account->accountType() == ASSETS_TYPE_LIABILITIES) {
							if(account->isClosed()) add_secondary_list = true;
							else b_add = true;
						}
					} else if((account->accountType() == ASSETS_TYPE_SECURITIES && !b_exclude_securities) || account->accountType() == ASSETS_TYPE_LIABILITIES || (account == budget->balancingAccount && !b_exclude_balancing) || account->isClosed()) {
						add_secondary_list = true;
					} else if(account->accountType() != ASSETS_TYPE_SECURITIES && account != budget->balancingAccount) {
						b_add = true;
					}
					if(b_add) {
						if(cactions > 0 && count() == cactions) APPEND_SEPARATOR
						APPEND_ROW
					}
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
			if(add_secondary_list) {
				if(count() > 0 && count() >= cactions) APPEND_SEPARATOR
				for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
					bool b_add = false;
					AssetsAccount *account = *it;
					if(i_type >= 100) {
						if(account->accountType() == i_type && account->isClosed()) {
							b_add = true;
						}
					} else if(i_type == -3) {
						if((account->accountType() == ASSETS_TYPE_CREDIT_CARD || account->accountType() == ASSETS_TYPE_LIABILITIES) && account->isClosed()) {
							b_add = true;
						}
					} else if((account->accountType() == ASSETS_TYPE_SECURITIES && !b_exclude_securities) || account->accountType() == ASSETS_TYPE_LIABILITIES || (account == budget->balancingAccount && !b_exclude_balancing) || account->isClosed()) {
						b_add = true;
					}
					if(b_add) {
						APPEND_ROW
					}
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
bool AccountComboBox::hasAccount() const {
	return count() > firstAccountIndex();
}
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
		QRegExp reg(QString("\\b")+str, Qt::CaseInsensitive);
		Account *account = currentAccount();
		filterModel->setFilterRegExp(reg);
		if(filterModel->rowCount() == 0) {
			filterModel->setFilterRegExp("");
			setCurrentAccount(account);
		} else if(filterModel->rowCount() == 1 && filterModel->data(filterModel->index(0, 0), Qt::UserRole).value<void*>() != NULL) {
			account = (Account*) filterModel->data(filterModel->index(0, 0), Qt::UserRole).value<void*>();
			filterModel->setFilterRegExp("");
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

