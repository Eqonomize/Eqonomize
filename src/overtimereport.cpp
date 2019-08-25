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

#include "overtimereport.h"


#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QObject>
#include <QString>
#include <QPushButton>
#include <QVBoxLayout>
#include <qvector.h>
#include <QTextStream>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QUrl>
#include <QFileDialog>
#include <QSaveFile>
#include <QApplication>
#include <QTemporaryFile>
#include <QMimeDatabase>
#include <QMimeType>
#include <QMessageBox>
#include <QPrinter>
#include <QTextEdit>
#include <QPrintDialog>
#include <QSettings>
#include <QStandardPaths>
#include <QRadioButton>
#include <QButtonGroup>
#include <QAction>
#include <QKeyEvent>

#include "account.h"
#include "budget.h"
#include "recurrence.h"
#include "transaction.h"

#include <cmath>

extern QString htmlize_string(QString str);
extern QString last_document_directory;

struct month_info {
	double value;
	double expense;
	double count;
	QDate date;
	QMap<QString, double> tags;
	QMap<Account*, double> cats;
};

DescriptionsCombo::DescriptionsCombo(int type, Budget *budg, QWidget *parent, bool show_all) : QPushButton(parent) {
	itemsMenu = new DescriptionsMenu(type, budg, this, show_all);
	setMenu(itemsMenu);
	setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	clear();
	setMinimumWidth(sizeHint().width());
	connect(itemsMenu, SIGNAL(aboutToShow()), this, SLOT(resizeDescriptionsMenu()));
	connect(itemsMenu, SIGNAL(selectedItemsChanged()), this, SLOT(updateText()));
	connect(itemsMenu, SIGNAL(selectedItemsChanged()), this, SIGNAL(selectedItemsChanged()));
}
void DescriptionsCombo::resizeDescriptionsMenu() {
	itemsMenu->setMinimumWidth(width());
}
void DescriptionsCombo::updateText() {
	setText(itemsMenu->selectedItemsText(0).replace("&", "&&"));
	setToolTip(itemsMenu->selectedItemsText(1));
}
void DescriptionsCombo::addItem(QString str) {
	itemsMenu->addItem(str);
}
void DescriptionsCombo::updateItems(const QStringList &list) {
	itemsMenu->updateItems(list);
}
void DescriptionsCombo::setItemType(int type) {
	itemsMenu->setItemType(type);
}
int DescriptionsCombo::itemType() {
	return itemsMenu->itemType();
}
void DescriptionsCombo::setItemSelected(const QString &str, bool b) {
	itemsMenu->setItemSelected(str, b);
	updateText();
}
bool DescriptionsCombo::itemSelected(const QString &str) {
	return itemsMenu->itemSelected(str);
}
bool DescriptionsCombo::allItemsSelected() {
	return itemsMenu->allItemsSelected();
}
QStringList &DescriptionsCombo::selectedItems() {
	return itemsMenu->selected_items;
}
QString DescriptionsCombo::selectedItemsText(int type) {
	return itemsMenu->selectedItemsText(type);
}
bool DescriptionsCombo::testTransaction(Transactions *trans) {
	return itemsMenu->testTransaction(trans);
}
void DescriptionsCombo::selectAll() {
	itemsMenu->selectAll();
	updateText();
}
void DescriptionsCombo::deselectAll() {
	itemsMenu->deselectAll();
	updateText();
}
void DescriptionsCombo::clear() {
	itemsMenu->clearItems();
	updateText();
}

DescriptionsMenu::DescriptionsMenu(int type, Budget *budg, QWidget *parent, bool show_all) : QMenu(parent), budget(budg), b_all(show_all), i_type(type) {
	if(b_all) {
		if(i_type == 0) addAction(tr("All Descriptions Combined", "Referring to the transaction description property (transaction title/generic article name)"), this, SLOT(toggleAll()));
		else if(i_type == 1) addAction(tr("All Tags Combined"), this, SLOT(toggleAll()));
		else if(i_type == 2) addAction(tr("All Payees Combined"), this, SLOT(toggleAll()));
		else if(i_type == 3) addAction(tr("All Payers Combined"), this, SLOT(toggleAll()));
		else if(i_type == 4) addAction(tr("All Payees/Payers Combined"), this, SLOT(toggleAll()));
	}
}
void DescriptionsMenu::addItem(QString str) {
	if(b_all && item_actions.isEmpty()) addSeparator();
	QAction *action = addAction(str.replace("&", "&&"), this, SLOT(itemToggled()));
	action->setCheckable(true);
	item_actions[str] = action;
}
void DescriptionsMenu::updateItems(const QStringList &list) {
	QHash<QString, bool> itemb;
	for(QHash<QString, QAction*>::const_iterator it = item_actions.constBegin(); it != item_actions.constEnd(); ++it) {
		itemb[it.key()] = it.value()->isChecked();
	}
	clearItems();
	if(!list.isEmpty()) addSeparator();
	bool has_empty = false;
	for(int i = 0; i < list.count(); i++) {
		QString str = list.at(i);
		if(str.isEmpty()) {
			has_empty = true;
		} else {
			QAction *action = addAction(str.replace("&", "&&"), this, SLOT(itemToggled()));
			action->setCheckable(true);
			if(itemb.contains(list.at(i))) {
				action->setChecked(itemb[list.at(i)]);
				if(itemb[list.at(i)]) selected_items << list.at(i);
			}
			item_actions[list.at(i)] = action;
		}
	}
	if(has_empty) {
		QString str;
		if(i_type == 0) str = tr("No description", "Referring to the transaction description property (transaction title/generic article name)");
		else if(i_type == 2) str = tr("No payee");
		else if(i_type == 3) str = tr("No payer");
		else if(i_type == 4) str = tr("No payee/payer");
		QAction *action = addAction(str, this, SLOT(itemToggled()));
		action->setCheckable(true);
		if(itemb.contains(QString())) {
			action->setChecked(itemb[QString()]);
			if(itemb[QString()]) selected_items << QString();
		}
		item_actions[QString()] = action;
	}
}
void DescriptionsMenu::setItemType(int type) {
	i_type = type;
}
int DescriptionsMenu::itemType() {
	return i_type;
}
void DescriptionsMenu::clearItems() {
	clear();
	item_actions.clear();
	selected_items.clear();
	if(b_all) {
		if(i_type == 0) addAction(tr("All Descriptions Combined", "Referring to the transaction description property (transaction title/generic article name)"), this, SLOT(toggleAll()));
		else if(i_type == 1) addAction(tr("All Tags Combined"), this, SLOT(toggleAll()));
		else if(i_type == 2) addAction(tr("All Payees Combined"), this, SLOT(toggleAll()));
		else if(i_type == 3) addAction(tr("All Payers Combined"), this, SLOT(toggleAll()));
		else if(i_type == 4) addAction(tr("All Payees/Payers Combined"), this, SLOT(toggleAll()));
	}
}
bool DescriptionsMenu::testTransaction(Transactions *trans) {
	if(allItemsSelected()) return true;
	for(QStringList::const_iterator it = selected_items.constBegin(); it != selected_items.constEnd(); ++it) {
		if(i_type == 0 && trans->description().compare(*it, Qt::CaseInsensitive) == 0) return true;
		if(i_type == 1 && trans->hasTag(*it, true)) return true;
		if(i_type >= 2 && i_type <= 4 && trans->payeeText().compare(*it, Qt::CaseInsensitive) == 0) return true;
	}
	return false;
}
void DescriptionsMenu::setItemSelected(const QString &str, bool b) {
	QHash<QString, QAction*>::iterator it = item_actions.find(str);
	if(it != item_actions.end() && b != it.value()->isChecked()) {
		it.value()->setChecked(b);
		if(b) selected_items << it.key();
		else selected_items.removeAll(it.key());
	}
}
bool DescriptionsMenu::itemSelected(const QString &str) {
	if(selected_items.count() == 1) return selected_items.at(0) == str;
	QHash<QString, QAction*>::iterator it = item_actions.find(str);
	if(it != item_actions.end()) {
		return it.value()->isChecked();
	}
	return false;
}
bool DescriptionsMenu::allItemsSelected() {
	int n = selectedItemsCount();
	return (b_all && n == 0) || n == item_actions.count();
}
int DescriptionsMenu::selectedItemsCount() {
	return selected_items.count();
}
QString DescriptionsMenu::selectedItemsText(int type) {
	QString str;
	int n = 0;
	for(int i = 0; i < selected_items.count(); i++) {
		if(i == 0 || type > 0) {
			if(type == 1 && i > 0) str += ", ";
			else if(type == 2 && i > 0) str += " + ";
			if(selected_items.at(i).isEmpty() && i_type != 1) {
				if(i_type == 0) str += tr("No description", "Referring to the transaction description property (transaction title/generic article name)");
				else if(i_type == 2) str += tr("No payee");
				else if(i_type == 3) str += tr("No payer");
				else if(i_type == 4) str += tr("No payee/payer");
			} else {
				str += selected_items.at(i);
			}
		}
		n++;
	}
	if(n == 0 || (type == 0 && n > 1)) {
		if(n == 0 || n == item_actions.count()) {
			if(i_type == 0) str = tr("All Descriptions Combined", "Referring to the transaction description property (transaction title/generic article name)");
			else if(i_type == 1) str = tr("All Tags Combined");
			else if(i_type == 2) str = tr("All Payees Combined");
			else if(i_type == 3) str = tr("All Payers Combined");
			else if(i_type == 4) str = tr("All Payees/Payers Combined");
		} else {
			if(i_type == 0) str = tr("%n descriptions", "Referring to the transaction description property (transaction title/generic article name)", n);
			else if(i_type == 1) str = tr("%n tags", "", n);
			else if(i_type == 2) str = tr("%n payees", "", n);
			else if(i_type == 3) str = tr("%n payers", "", n);
			else if(i_type == 4) str = tr("%n payees/payers", "", n);
		}
	}
	return str;
}
void DescriptionsMenu::itemToggled() {
	QAction *action = qobject_cast<QAction*>(sender());
	for(QHash<QString, QAction*>::const_iterator it = item_actions.constBegin(); it != item_actions.constEnd(); ++it) {
		if(it.value() == action) {
			selected_items.removeAll(it.key());
			if(action->isChecked()) selected_items << it.key();
			break;
		}
	}
	emit selectedItemsChanged();
}
void DescriptionsMenu::keyPressEvent(QKeyEvent *e) {
	if(e->key() == Qt::Key_Space) {
		QAction *action = activeAction();
		if(action) action->trigger();
		e->setAccepted(true);
		return;
	} else if(e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) {
		deselectAll();
		QAction *action = activeAction();
		if(action) action->trigger();
		e->setAccepted(true);
		hide();
		return;
	}
	QMenu::keyPressEvent(e);
}
void DescriptionsMenu::mouseReleaseEvent(QMouseEvent *e) {
	QAction *action = actionAt(e->pos());
	if(action) {
		if(e->button() != Qt::LeftButton || e->pos().x() <= actionGeometry(action).height()) {
			action->trigger();
			e->setAccepted(true);
		} else {
			deselectAll();
			action->trigger();
			e->setAccepted(true);
			hide();
		}
		return;
	}
	QMenu::mouseReleaseEvent(e);
}
void DescriptionsMenu::selectAll() {
	selected_items.clear();
	for(QHash<QString, QAction*>::const_iterator it = item_actions.constBegin(); it != item_actions.constEnd(); ++it) {
		it.value()->setChecked(true);
		selected_items << it.key();
	}
}
void DescriptionsMenu::deselectAll() {
	selected_items.clear();
	for(QHash<QString, QAction*>::const_iterator it = item_actions.constBegin(); it != item_actions.constEnd(); ++it) {
		it.value()->setChecked(false);
	}
}
void DescriptionsMenu::toggleAll() {
	if(selectedItemsCount() == item_actions.count()) deselectAll();
	else selectAll();
	emit selectedItemsChanged();
}

AccountsCombo::AccountsCombo(Budget *budg, QWidget *parent, bool shows_assets) : QPushButton(parent) {
	accountsMenu = new AccountsMenu(budg, this, shows_assets);
	setMenu(accountsMenu);
	setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	clear();
	setMinimumWidth(sizeHint().width());
	connect(accountsMenu, SIGNAL(aboutToShow()), this, SLOT(resizeAccountsMenu()));
	connect(accountsMenu, SIGNAL(selectedAccountsChanged()), this, SLOT(updateText()));
	connect(accountsMenu, SIGNAL(selectedAccountsChanged()), this, SIGNAL(selectedAccountsChanged()));
}
void AccountsCombo::resizeAccountsMenu() {
	accountsMenu->setMinimumWidth(width());
}
void AccountsCombo::updateText() {
	setText(accountsMenu->selectedAccountsText(0).replace("&", "&&"));
	setToolTip(accountsMenu->selectedAccountsText(1));
}
void AccountsCombo::updateAccounts(int type) {
	accountsMenu->updateAccounts(type);
	updateText();
}
void AccountsCombo::setAccountSelected(Account *account, bool b) {
	accountsMenu->setAccountSelected(account, b);
	updateText();
}
bool AccountsCombo::accountSelected(Account *account) {
	return accountsMenu->accountSelected(account);
}
bool AccountsCombo::allAccountsSelected() {
	return accountsMenu->allAccountsSelected();
}
bool AccountsCombo::testTransactionRelation(Transactions *trans, bool exclude_securities) {
	if(accountsMenu->allAccountsSelected()) return true;
	for(QList<Account*>::const_iterator it = accountsMenu->selected_accounts.constBegin(); it != accountsMenu->selected_accounts.constEnd(); ++it) {
		if((!exclude_securities || (*it)->type() != ACCOUNT_TYPE_ASSETS || ((AssetsAccount*) *it)->accountType() != ASSETS_TYPE_SECURITIES) && trans->relatesToAccount(*it)) {
			return true;
		}
	}
	return false;
}
double AccountsCombo::transactionChange(Transactions *trans, bool exclude_securities) {
	double d = 0.0;
	for(QList<Account*>::const_iterator it = accountsMenu->selected_accounts.constBegin(); it != accountsMenu->selected_accounts.constEnd(); ++it) {
		if(!exclude_securities || (*it)->type() != ACCOUNT_TYPE_ASSETS || ((AssetsAccount*) *it)->accountType() != ASSETS_TYPE_SECURITIES) d += trans->accountChange(*it);
	}
	return d;
}
QList<Account*> &AccountsCombo::selectedAccounts() {
	return accountsMenu->selected_accounts;
}
QString AccountsCombo::selectedAccountsText(int type) {
	return accountsMenu->selectedAccountsText(type);
}
void AccountsCombo::selectAll() {
	accountsMenu->selectAll();
	updateText();
}
void AccountsCombo::deselectAll() {
	accountsMenu->deselectAll();
	updateText();
}
void AccountsCombo::clear() {
	updateAccounts(-1);
	updateText();
}

AccountsMenu::AccountsMenu(Budget *budg, QWidget *parent, bool shows_assets) : QMenu(parent), budget(budg), b_assets(shows_assets) {}
void AccountsMenu::updateAccounts(int type) {
	QHash<Account*, bool> catb;
	if(type >= 0) {
		for(QHash<Account*, QAction*>::const_iterator it = account_actions.constBegin(); it != account_actions.constEnd(); ++it) {
			catb[it.key()] = it.value()->isChecked();
		}
	}
	clear();
	account_actions.clear();
	selected_accounts.clear();
	if(type >= 0) {
		if(b_assets) addAction(tr("All Accounts"), this, SLOT(toggleAll()));
		else addAction(tr("All Categories Combined"), this, SLOT(toggleAll()));
	}
	if(type == ACCOUNT_TYPE_ASSETS) {
		if(!budget->assetsAccounts.isEmpty()) addSeparator();
		for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
			if(*it != budget->balancingAccount) {
				QAction *action = addAction((*it)->nameWithParent().replace("&", "&&"), this, SLOT(accountToggled()));
				action->setCheckable(true);
				if(catb.contains(*it)) {
					action->setChecked(catb[*it]);
					if(catb[*it]) selected_accounts << *it;
				}
				account_actions[*it] = action;
			}
		}
	} else if(type == ACCOUNT_TYPE_INCOMES) {
		if(!budget->incomesAccounts.isEmpty()) addSeparator();
		for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
			QAction *action = addAction((*it)->nameWithParent().replace("&", "&&"), this, SLOT(accountToggled()));
			action->setCheckable(true);
			if(catb.contains(*it)) {
				action->setChecked(catb[*it]);
				if(catb[*it]) selected_accounts << *it;
			}
			account_actions[*it] = action;
		}
	} else {
		if(!budget->expensesAccounts.isEmpty()) addSeparator();
		for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
			QAction *action = addAction((*it)->nameWithParent().replace("&", "&&"), this, SLOT(accountToggled()));
			action->setCheckable(true);
			if(catb.contains(*it)) {
				action->setChecked(catb[*it]);
				if(catb[*it]) selected_accounts << *it;
			}
			account_actions[*it] = action;
		}
	}
}
void AccountsMenu::setAccountSelected(Account *account, bool b) {
	QHash<Account*, QAction*>::iterator it = account_actions.find(account);
	if(it != account_actions.end() && b != it.value()->isChecked()) {
		it.value()->setChecked(b);
		if(b) selected_accounts << it.key();
		else selected_accounts.removeAll(it.key());
	}
}
bool AccountsMenu::accountSelected(Account *account) {
	if(selected_accounts.count() == 1) return selected_accounts.at(0) == account;
	QHash<Account*, QAction*>::iterator it = account_actions.find(account);
	if(it != account_actions.end()) {
		return it.value()->isChecked();
	}
	return false;
}
bool AccountsMenu::allAccountsSelected() {
	int n = selectedAccountsCount();
	return n == 0 || n == account_actions.count();
}
int AccountsMenu::selectedAccountsCount() {
	return selected_accounts.count();
}
QString AccountsMenu::selectedAccountsText(int type) {
	QString str;
	int n = 0;
	for(int i = 0; i < selected_accounts.count(); i++) {
		if(i == 0 || type > 0) {
			if(type == 1 && i > 0) str += ", ";
			else if(type == 2 && i > 0) str += " + ";
			str += selected_accounts.at(i)->nameWithParent();
		}
		n++;
	}
	if(n == 0 || (type == 0 && n > 1)) {
		if(n == 0 || n == account_actions.count()) {
			if(b_assets) str = tr("All Accounts");
			else str = tr("All Categories Combined");
		} else {
			if(b_assets) str = tr("%n accounts", "", n);
			else str = tr("%n categories", "", n);
		}
	}
	return str;
}
void AccountsMenu::accountToggled() {
	QAction *action = qobject_cast<QAction*>(sender());
	for(QHash<Account*, QAction*>::const_iterator it = account_actions.constBegin(); it != account_actions.constEnd(); ++it) {
		if(it.value() == action) {
			selected_accounts.removeAll(it.key());
			if(action->isChecked()) selected_accounts << it.key();
			break;
		}
	}
	emit selectedAccountsChanged();
}
void AccountsMenu::keyPressEvent(QKeyEvent *e) {
	if(e->key() == Qt::Key_Space) {
		QAction *action = activeAction();
		if(action) action->trigger();
		e->setAccepted(true);
		return;
	} else if(e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) {
		deselectAll();
		QAction *action = activeAction();
		if(action) action->trigger();
		e->setAccepted(true);
		hide();
		return;
	}
	QMenu::keyPressEvent(e);
}
void AccountsMenu::mouseReleaseEvent(QMouseEvent *e) {
	QAction *action = actionAt(e->pos());
	if(action) {
		if(e->button() != Qt::LeftButton || e->pos().x() <= actionGeometry(action).height()) {
			action->trigger();
			e->setAccepted(true);
		} else {
			deselectAll();
			action->trigger();
			e->setAccepted(true);
			hide();
		}
		return;
	}
	QMenu::mouseReleaseEvent(e);
}
void AccountsMenu::selectAll() {
	selected_accounts.clear();
	for(QHash<Account*, QAction*>::const_iterator it = account_actions.constBegin(); it != account_actions.constEnd(); ++it) {
		it.value()->setChecked(true);
		selected_accounts << it.key();
	}
}
void AccountsMenu::deselectAll() {
	selected_accounts.clear();
	for(QHash<Account*, QAction*>::const_iterator it = account_actions.constBegin(); it != account_actions.constEnd(); ++it) {
		it.value()->setChecked(false);
	}
}
void AccountsMenu::toggleAll() {
	if(selectedAccountsCount() == account_actions.count()) deselectAll();
	else selectAll();
	emit selectedAccountsChanged();
}


OverTimeReport::OverTimeReport(Budget *budg, QWidget *parent) : QWidget(parent), budget(budg) {

	block_display_update = false;

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	QDialogButtonBox *buttons = new QDialogButtonBox(this);
	saveButton = buttons->addButton(tr("Save As…"), QDialogButtonBox::ActionRole);
	saveButton->setAutoDefault(false);
	printButton = buttons->addButton(tr("Print…"), QDialogButtonBox::ActionRole);
	printButton->setAutoDefault(false);
	layout->addWidget(buttons);

	htmlview = new QTextEdit(this);
	htmlview->setReadOnly(true);
	layout->addWidget(htmlview);

	QSettings settings;
	settings.beginGroup("OverTimeReport");

	QWidget *settingsWidget = new QWidget(this);
	QGridLayout *settingsLayout = new QGridLayout(settingsWidget);

	settingsLayout->addWidget(new QLabel(tr("Source:"), settingsWidget), 0, 0);
	QHBoxLayout *choicesLayout = new QHBoxLayout();
	settingsLayout->addLayout(choicesLayout, 0, 1);
	sourceCombo = new QComboBox(settingsWidget);
	sourceCombo->setEditable(false);
	sourceCombo->addItem(tr("Profits"));
	sourceCombo->addItem(tr("Expenses"));
	sourceCombo->addItem(tr("Incomes"));
	sourceCombo->addItem(tr("Assets & Liabilities"));
	sourceCombo->addItem(tr("Tags"));
	choicesLayout->addWidget(sourceCombo);
	choicesLayout->setStretchFactor(sourceCombo, 0);
	categoryCombo = new AccountsCombo(budget, settingsWidget);
	categoryCombo->setEnabled(false);
	choicesLayout->addWidget(categoryCombo);
	choicesLayout->setStretchFactor(categoryCombo, 1);
	tagCombo = new DescriptionsCombo(1, budget, settingsWidget, false);
	tagCombo->hide();
	choicesLayout->addWidget(tagCombo);
	choicesLayout->setStretchFactor(tagCombo, 1);
	descriptionCombo = new DescriptionsCombo(0, budget, settingsWidget);
	descriptionCombo->setEnabled(false);
	choicesLayout->addWidget(descriptionCombo);
	choicesLayout->setStretchFactor(descriptionCombo, 1);
	accountCombo = new AccountsCombo(budget, settingsWidget, true);
	accountCombo->updateAccounts(ACCOUNT_TYPE_ASSETS);
	choicesLayout->addWidget(accountCombo);
	choicesLayout->setStretchFactor(accountCombo, 1);

	current_source = 0;

	columnsLabel = new QLabel(tr("Columns:"), settingsWidget);
	settingsLayout->addWidget(columnsLabel, 1, 0);
	QHBoxLayout *enabledLayout = new QHBoxLayout();
	QButtonGroup *group = new QButtonGroup(this);
	catsButton = new QRadioButton(tr("Categories"), settingsWidget);
	group->addButton(catsButton, 1);
	enabledLayout->addWidget(catsButton);
	tagsButton = new QRadioButton(tr("Tags"), settingsWidget);
	group->addButton(tagsButton, 2);
	enabledLayout->addWidget(tagsButton);
	totalButton = new QRadioButton(tr("Total:"), settingsWidget);
	group->addButton(totalButton, 0);
	enabledLayout->addWidget(totalButton);
	switch(settings.value("columns", 0).toInt()) {
		case 1: {catsButton->setChecked(true); break;}
		case 2: {tagsButton->setChecked(true); break;}
		default: {totalButton->setChecked(true); break;}
	}
	settingsLayout->addLayout(enabledLayout, 1, 1);
	valueButton = new QCheckBox(tr("Value"), settingsWidget);
	valueButton->setChecked(settings.value("valueEnabled", true).toBool());
	enabledLayout->addWidget(valueButton);
	dailyButton = new QCheckBox(tr("Daily"), settingsWidget);
	dailyButton->setChecked(settings.value("dailyAverageEnabled", true).toBool());
	enabledLayout->addWidget(dailyButton);
	monthlyButton = new QCheckBox(tr("Monthly"), settingsWidget);
	monthlyButton->setChecked(settings.value("monthlyAverageEnabled", true).toBool());
	enabledLayout->addWidget(monthlyButton);
	yearlyButton = new QCheckBox(tr("Yearly"), settingsWidget);
	yearlyButton->setChecked(settings.value("yearlyEnabled", false).toBool());
	enabledLayout->addWidget(yearlyButton);
	countButton = new QCheckBox(tr("Quantity"), settingsWidget);
	countButton->setChecked(settings.value("transactionCountEnabled", true).toBool());
	enabledLayout->addWidget(countButton);
	perButton = new QCheckBox(tr("Average value"), settingsWidget);
	perButton->setChecked(settings.value("valuePerTransactionEnabled", false).toBool());
	enabledLayout->addWidget(perButton);
	enabledLayout->addStretch(1);
	
	valueButton->setEnabled(current_source != 12 && current_source != 0 && totalButton->isChecked());
	dailyButton->setEnabled(current_source != 12 && current_source != 0 && totalButton->isChecked());
	monthlyButton->setEnabled(current_source != 12 && current_source != 0 && totalButton->isChecked());
	yearlyButton->setEnabled(current_source != 12 && current_source != 0 && totalButton->isChecked());
	countButton->setEnabled(current_source != 12 && current_source != 0 && totalButton->isChecked());
	perButton->setEnabled(current_source != 12 && current_source != 0 && totalButton->isChecked());
	columnsLabel->setEnabled(current_source != 12 && current_source != 0);
	tagsButton->setEnabled(current_source != 12 && current_source != 0);
	catsButton->setEnabled(current_source != 12 && current_source != 0);
	totalButton->setEnabled(current_source != 12 && current_source != 0);
	
	settings.endGroup();
	
	layout->addWidget(settingsWidget);
	
	updateTags();

	connect(group, SIGNAL(buttonToggled(int, bool)), this, SLOT(columnsToggled(int, bool)));
	connect(valueButton, SIGNAL(toggled(bool)), this, SLOT(updateDisplay()));
	connect(dailyButton, SIGNAL(toggled(bool)), this, SLOT(updateDisplay()));
	connect(monthlyButton, SIGNAL(toggled(bool)), this, SLOT(updateDisplay()));
	connect(yearlyButton, SIGNAL(toggled(bool)), this, SLOT(updateDisplay()));
	connect(countButton, SIGNAL(toggled(bool)), this, SLOT(updateDisplay()));
	connect(perButton, SIGNAL(toggled(bool)), this, SLOT(updateDisplay()));
	connect(sourceCombo, SIGNAL(activated(int)), this, SLOT(sourceChanged(int)));
	connect(categoryCombo, SIGNAL(selectedAccountsChanged()), this, SLOT(categoryChanged()));
	connect(tagCombo, SIGNAL(selectedItemsChanged()), this, SLOT(tagChanged()));
	connect(descriptionCombo, SIGNAL(selectedItemsChanged()), this, SLOT(descriptionChanged()));
	connect(accountCombo, SIGNAL(selectedAccountsChanged()), this, SLOT(updateDisplay()));
	connect(saveButton, SIGNAL(clicked()), this, SLOT(save()));
	connect(printButton, SIGNAL(clicked()), this, SLOT(print()));

}

void OverTimeReport::resetOptions() {
	block_display_update = true;
	accountCombo->blockSignals(true);
	accountCombo->selectAll();
	accountCombo->blockSignals(false);
	sourceCombo->blockSignals(true);
	sourceCombo->setCurrentIndex(0);
	sourceCombo->blockSignals(false);
	columnsToggled(tagsButton->isChecked() ? 2 : (catsButton->isChecked() ? 1 : 0), true);
	sourceChanged(0);
	block_display_update = false;
	updateDisplay();
}

void OverTimeReport::columnsToggled(int id, bool b) {
	if(!b) return;
	valueButton->setEnabled(id == 0);
	dailyButton->setEnabled(id == 0);
	monthlyButton->setEnabled(id == 0);
	yearlyButton->setEnabled(id == 0);
	countButton->setEnabled(id == 0);
	perButton->setEnabled(id == 0);
	updateDisplay();
}
void OverTimeReport::descriptionChanged() {
	if(descriptionCombo->allItemsSelected()) {
		if(sourceCombo->currentIndex() == 4) current_source = 13;
		else if(sourceCombo->currentIndex() == 2) current_source = 5;
		else current_source = 6;
	} else {
		if(sourceCombo->currentIndex() == 4) current_source = 14;
		else if(sourceCombo->currentIndex() == 2) current_source = 9;
		else current_source = 10;
	}
	updateDisplay();
}
void OverTimeReport::categoryChanged() {
	descriptionCombo->blockSignals(true);
	if(categoryCombo->allAccountsSelected()) {
		if(sourceCombo->currentIndex() == 2) {
			current_source = 1;
		} else {
			current_source = 2;
		}
		descriptionCombo->clear();
		descriptionCombo->setEnabled(false);
	} else {
		if(sourceCombo->currentIndex() == 1) {
			current_source = 6;
		} else {
			current_source = 5;
		}
		QMap<QString, QString> descriptions;
		for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constEnd(); it != budget->transactions.constBegin();) {
			--it;
			Transaction *trans = *it;
			if(categoryCombo->accountSelected(trans->fromAccount()) || categoryCombo->accountSelected(trans->toAccount())) {
				if(!descriptions.contains(trans->description().toLower())) descriptions[trans->description().toLower()] = trans->description();
			}
		}
		QStringList items;
		QMap<QString, QString>::iterator it_e = descriptions.end();
		for(QMap<QString, QString>::iterator it = descriptions.begin(); it != it_e; ++it) {
			items << it.value();
		}
		descriptionCombo->updateItems(items);
		descriptionCombo->setEnabled(true);
	}
	descriptionCombo->blockSignals(false);
	updateDisplay();
}
void OverTimeReport::tagChanged() {
	descriptionCombo->blockSignals(true);
	if(sourceCombo->currentIndex() == 4) {
		current_source = 13;
		QMap<QString, QString> descriptions;
		for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constEnd(); it != budget->transactions.constBegin();) {
			--it;
			Transaction *trans = *it;
			if((trans->type() == TRANSACTION_TYPE_EXPENSE || trans->type() == TRANSACTION_TYPE_INCOME) && tagCombo->testTransaction(trans)) {
				if(!descriptions.contains(trans->description().toLower())) descriptions[trans->description().toLower()] = trans->description();
			}
		}
		QStringList items;
		QMap<QString, QString>::iterator it_e = descriptions.end();
		for(QMap<QString, QString>::iterator it = descriptions.begin(); it != it_e; ++it) {
			items << it.value();
		}
		descriptionCombo->updateItems(items);
		descriptionCombo->setEnabled(true);
	} else {
		descriptionCombo->clear();
	}
	descriptionCombo->blockSignals(false);
	updateDisplay();
}
void OverTimeReport::sourceChanged(int index) {
	categoryCombo->blockSignals(true);
	descriptionCombo->blockSignals(true);
	categoryCombo->clear();
	tagCombo->blockSignals(true);
	descriptionCombo->clear();
	descriptionCombo->setEnabled(false);
	if(index == 2) {
		categoryCombo->updateAccounts(ACCOUNT_TYPE_INCOMES);
		categoryCombo->setEnabled(true);
		categoryCombo->show();
		tagCombo->hide();
		current_source = 1;
	} else if(index == 1) {
		categoryCombo->updateAccounts(ACCOUNT_TYPE_EXPENSES);
		categoryCombo->setEnabled(true);
		categoryCombo->show();
		tagCombo->hide();
		current_source = 2;
	} else if(index == 4) {
		categoryCombo->setEnabled(false);
		categoryCombo->hide();
		tagCombo->show();
		current_source = 13;
	} else {
		categoryCombo->setEnabled(false);
		categoryCombo->show();
		tagCombo->hide();
		if(index == 3) current_source = 12;
		else current_source = 0;
	}
	valueButton->setEnabled(current_source != 12 && current_source != 0 && totalButton->isChecked());
	dailyButton->setEnabled(current_source != 12 && current_source != 0 && totalButton->isChecked());
	monthlyButton->setEnabled(current_source != 12 && current_source != 0 && totalButton->isChecked());
	yearlyButton->setEnabled(current_source != 12 && current_source != 0 && totalButton->isChecked());
	countButton->setEnabled(current_source != 12 && current_source != 0 && totalButton->isChecked());
	perButton->setEnabled(current_source != 12 && current_source != 0 && totalButton->isChecked());
	columnsLabel->setEnabled(current_source != 12 && current_source != 0);
	tagsButton->setEnabled(current_source != 12 && current_source != 0);
	catsButton->setEnabled(current_source != 12 && current_source != 0);
	totalButton->setEnabled(current_source != 12 && current_source != 0);
	categoryCombo->blockSignals(false);
	tagCombo->blockSignals(false);
	descriptionCombo->blockSignals(false);
	if(index == 4) tagChanged();
	else updateDisplay();
}


void OverTimeReport::saveConfig() {
	QSettings settings;
	settings.beginGroup("OverTimeReport");
	settings.setValue("size", ((QDialog*) parent())->size());
	settings.setValue("columns", tagsButton->isChecked() ? 2 : (catsButton->isChecked() ? 1 : 0));
	settings.setValue("valueEnabled", valueButton->isChecked());
	settings.setValue("dailyAverageEnabled", dailyButton->isChecked());
	settings.setValue("monthlyAverageEnabled", monthlyButton->isChecked());
	settings.setValue("yearlyAverageEnabled", yearlyButton->isChecked());
	settings.setValue("transactionCountEnabled", countButton->isChecked());
	settings.setValue("valuePerTransactionEnabled", perButton->isChecked());
	settings.endGroup();
}
void OverTimeReport::save() {
	QMimeDatabase db;
	QFileDialog fileDialog(this);
	fileDialog.setNameFilter(db.mimeTypeForName("text/html").filterString());
	fileDialog.setDefaultSuffix(db.mimeTypeForName("text/html").preferredSuffix());
	fileDialog.setAcceptMode(QFileDialog::AcceptSave);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
	fileDialog.setSupportedSchemes(QStringList("file"));
#endif
	fileDialog.setDirectory(last_document_directory);
	QString url;
	if(!fileDialog.exec()) return;
	QStringList urls = fileDialog.selectedFiles();
	if(urls.isEmpty()) return;
	url = urls[0];
	QSaveFile ofile(url);
	ofile.open(QIODevice::WriteOnly);
	ofile.setPermissions((QFile::Permissions) 0x0660);
	if(!ofile.isOpen()) {
		ofile.cancelWriting();
		QMessageBox::critical(this, tr("Error"), tr("Couldn't open file for writing."));
		return;
	}
	last_document_directory = fileDialog.directory().absolutePath();
	QTextStream outf(&ofile);
	outf.setCodec("UTF-8");
	outf << source;
	if(!ofile.commit()) {
		QMessageBox::critical(this, tr("Error"), tr("Error while writing file; file was not saved."));
		return;
	}
}

void OverTimeReport::print() {
	QPrinter printer;
	QPrintDialog print_dialog(&printer, this);
	if(print_dialog.exec() == QDialog::Accepted) {
		htmlview->print(&printer);
	}
}

void OverTimeReport::updateDisplay() {

	if(!isVisible() || block_display_update) return;

	bool b_tags = tagsButton->isChecked(), b_cats = catsButton->isChecked();
	bool enabled[8];
	enabled[0] = !b_tags && !b_cats && valueButton->isChecked();
	enabled[1] = !b_tags && !b_cats && dailyButton->isChecked();
	enabled[2] = !b_tags && !b_cats && monthlyButton->isChecked();
	enabled[3] = !b_tags && !b_cats && yearlyButton->isChecked();
	enabled[4] = !b_tags && !b_cats && countButton->isChecked();
	enabled[5] = !b_tags && !b_cats && perButton->isChecked();
	enabled[6] = false;
	enabled[7] = false;
	
	bool assets_selected = !accountCombo->allAccountsSelected();
	bool single_assets = assets_selected && accountCombo->selectedAccounts().count() == 1;
	
	QList<Account*> selected_categories;
	QVector<month_info> monthly_values;
	month_info *mi = NULL;
	QDate first_date;
	AccountType at = ACCOUNT_TYPE_EXPENSES;
	CategoryAccount *cat = NULL;
	int type = 0;
	QString title, valuetitle, pertitle, expensetitle, sumtitle;
	switch(current_source) {
		case 0: {
			if(assets_selected) {
				type = 4;
				//: Noun, how much the account balance has changed
				title = tr("Change: %1").arg(accountCombo->selectedAccountsText(1));
				valuetitle = tr("Deposit", "Money put into account");
				expensetitle = tr("Withdrawal", "Money taken out from account");
				//: Noun, how much the account balance has changed
				sumtitle = tr("Change");
			} else {
				type = 0;
				title = tr("Profits");
				valuetitle = tr("Incomes");
				expensetitle = tr("Expenses");
				sumtitle = title;
			}
			enabled[0] = true;
			enabled[1] = false;
			enabled[2] = false;
			enabled[3] = false;
			enabled[4] = false;
			enabled[5] = false;
			enabled[6] = true;
			enabled[7] = true;
			b_tags = false; b_cats = false;
			break;
		}
		case 1: {
			if(assets_selected) title = tr("Incomes, %1").arg(accountCombo->selectedAccountsText(2));
			else title = tr("Incomes");
			pertitle = tr("Average Income");
			valuetitle = title;
			type = 1;
			at = ACCOUNT_TYPE_INCOMES;
			break;
		}
		case 2: {
			if(assets_selected) title = tr("Expenses, %1").arg(accountCombo->selectedAccountsText(2));
			else title = tr("Expenses");
			pertitle = tr("Average Cost");
			valuetitle = title;
			type = 1;
			at = ACCOUNT_TYPE_EXPENSES;
			break;
		}
		case 5: {
			selected_categories = categoryCombo->selectedAccounts();
			if(assets_selected) title = tr("Incomes, %2: %1").arg(categoryCombo->selectedAccountsText(1)).arg(accountCombo->selectedAccountsText(2));
			else title = tr("Incomes: %1").arg(categoryCombo->selectedAccountsText(1));
			pertitle = tr("Average Income");
			valuetitle = tr("Incomes");
			type = 2;
			at = ACCOUNT_TYPE_INCOMES;
			break;
		}
		case 6: {
			selected_categories = categoryCombo->selectedAccounts();
			if(assets_selected) title = tr("Expenses, %2: %1").arg(categoryCombo->selectedAccountsText(1)).arg(accountCombo->selectedAccountsText(2));
			else title = tr("Expenses: %1").arg(categoryCombo->selectedAccountsText(1));
			pertitle = tr("Average Cost");
			valuetitle = tr("Expenses");
			type = 2;
			at = ACCOUNT_TYPE_EXPENSES;
			break;
		}
		case 9: {
			selected_categories = categoryCombo->selectedAccounts();
			if(assets_selected) title = tr("Incomes, %3: %2, %1").arg(categoryCombo->selectedAccountsText(2)).arg(descriptionCombo->selectedItemsText(2)).arg(accountCombo->selectedAccountsText(2));
			else title = tr("Incomes: %2, %1").arg(categoryCombo->selectedAccountsText(2)).arg(descriptionCombo->selectedItemsText(2));
			pertitle = tr("Average Income");
			valuetitle = tr("Incomes");
			type = 3;
			at = ACCOUNT_TYPE_INCOMES;
			break;
		}
		case 10: {
			selected_categories = categoryCombo->selectedAccounts();
			if(assets_selected) title = tr("Expenses, %3: %2, %1").arg(categoryCombo->selectedAccountsText(2)).arg(descriptionCombo->selectedItemsText(2)).arg(accountCombo->selectedAccountsText(2));
			else title = tr("Expenses: %2, %1").arg(categoryCombo->selectedAccountsText(2)).arg(descriptionCombo->selectedItemsText(2));
			pertitle = tr("Average Cost");
			valuetitle = tr("Expenses");
			type = 3;
			at = ACCOUNT_TYPE_EXPENSES;
			break;
		}
		case 12: {
			if(assets_selected) {
				title = tr("Value: %1").arg(accountCombo->selectedAccountsText(1));
				type = 6;
				valuetitle = tr("Value");
			} else {
				title = tr("Assets & Liabilities");
				type = 5;
				valuetitle = tr("Assets");
				expensetitle = tr("Liabilities");
				sumtitle = tr("Total");
				enabled[6] = true;
				enabled[7] = true;
			}
			enabled[0] = true;
			enabled[1] = false;
			enabled[2] = false;
			enabled[3] = false;
			enabled[4] = false;
			enabled[5] = false;
			b_tags = false; b_cats = false;
			break;
		}
		case 13: {
			if(tagCombo->selectedItems().isEmpty()) {htmlview->setHtml(""); return;}
			if(assets_selected) title = tr("%2: %1").arg(tagCombo->selectedItemsText(1)).arg(accountCombo->selectedAccountsText(1));
			else title = tagCombo->selectedItemsText(1);
			pertitle = tr("Average Value");
			valuetitle = tr("Value");
			at = ACCOUNT_TYPE_ASSETS;
			type = 7;
			break;
		}
		case 14: {
			if(tagCombo->selectedItems().isEmpty()) {htmlview->setHtml(""); return;}
			if(assets_selected) title = tr("%3: %2, %1").arg(tagCombo->selectedItemsText(1)).arg(descriptionCombo->selectedItemsText(2)).arg(accountCombo->selectedAccountsText(2));
			else title = tr("%2, %1").arg(tagCombo->selectedItemsText(1)).arg(descriptionCombo->selectedItemsText(2));
			pertitle = tr("Average Value");
			valuetitle = tr("Value");
			at = ACCOUNT_TYPE_ASSETS;
			type = 8;
			break;
		}
		default: {
			htmlview->setHtml(""); return;
		}
	}

	if(selected_categories.count() == 1) {
		cat = (CategoryAccount*) selected_categories.at(0);
	}
	
	QDate start_date, first_trans_date;
	for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constBegin(); it != budget->transactions.constEnd(); ++it) {
		Transaction *trans = *it;
		if(!first_trans_date.isValid()) {
			first_trans_date = budget->firstBudgetDay(trans->date());
		}
		if(((selected_categories.count() == 0 || categoryCombo->testTransactionRelation(trans)) && ((current_source != 13 && current_source != 14) || (tagCombo->testTransaction(trans) && (trans->type() == TRANSACTION_TYPE_EXPENSE || trans->type() == TRANSACTION_TYPE_INCOME))) && (!b_tags || trans->tagsCount(true) > 0) && (current_source == 12 || trans->fromAccount()->type() != ACCOUNT_TYPE_ASSETS || trans->toAccount()->type() != ACCOUNT_TYPE_ASSETS) && (!assets_selected || accountCombo->testTransactionRelation(trans))) || (current_source == 0 && assets_selected && accountCombo->testTransactionRelation(trans))) {
			start_date = trans->date();
			if(!budget->isFirstBudgetDay(start_date)) {
				start_date = budget->firstBudgetDay(start_date);
				if(current_source == 12) budget->addBudgetMonthsSetFirst(start_date, -1);
				else if(start_date == first_trans_date) budget->addBudgetMonthsSetFirst(start_date, 1);
			}
			break;
		}
	}
	QDate curmonth = budget->firstBudgetDay(QDate::currentDate());
	if(start_date.isNull() || start_date > curmonth) start_date = curmonth;
	if(start_date != first_trans_date) {
		if(budget->budgetYear(start_date) == budget->budgetYear(first_trans_date)) start_date = first_trans_date;
		else start_date = budget->firstBudgetDayOfYear(start_date);
	}
	if(start_date == curmonth) {
		budget->addBudgetMonthsSetFirst(start_date, -1);
	}
	first_date = start_date;

	QDate curdate = QDate::currentDate().addDays(-1);
	if(!budget->isLastBudgetDay(curdate)) {
		curdate = budget->lastBudgetDay(curdate);
		budget->addBudgetMonthsSetLast(curdate, -1);
	}
	if(curdate < first_date || budget->isSameBudgetMonth(start_date, curdate)) {
		curdate = QDate::currentDate();
	}

	bool started = false;
	bool b_income = false, b_expense = false;
	bool includes_planned = false;
	QMap<QString, bool> tag_includes_planned;
	QMap<Account*, bool> cat_includes_planned;
	if(b_tags) {for(int i = 0; i < budget->tags.count(); i++) tag_includes_planned[budget->tags[i]] = false;}
	if(b_cats) {
		if(cat) {
			for(AccountList<CategoryAccount*>::const_iterator it = cat->subCategories.constBegin(); it != cat->subCategories.constEnd(); ++it) cat_includes_planned[*it] = false;
			cat_includes_planned[cat] = false;
		} else if(selected_categories.count() > 0) {
			for(QList<Account*>::const_iterator it = selected_categories.constBegin(); it != selected_categories.constEnd(); ++it) cat_includes_planned[*it] = false;
		} else {
			if(at != ACCOUNT_TYPE_EXPENSES) {for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) cat_includes_planned[*it] = false;}
			if(at != ACCOUNT_TYPE_INCOMES) {for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) cat_includes_planned[*it] = false;}
		}
	}

	for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constBegin(); it != budget->transactions.constEnd(); ++it) {
		Transaction *trans = *it;
		if(trans->date() > curdate) break;
		bool include = false;
		int sign = 1;
		if(!started && trans->date() >= first_date) started = true;
		if(started && (!assets_selected || accountCombo->testTransactionRelation(trans, type == 6)) && ((current_source != 13 && current_source != 14) || tagCombo->testTransaction(trans))) {
			if(type == 7 || (type == 8 && descriptionCombo->testTransaction(trans))) {
				include = true;
				if(trans->type() == TRANSACTION_TYPE_EXPENSE) {b_expense = true; sign = -1;}
				else if(trans->type() == TRANSACTION_TYPE_INCOME) {b_income = true; sign = 1;}
				else include = false;
			} else if(type >= 4 && type != 8) {
				include = true;
			} else if((type == 1 && trans->fromAccount()->type() == at) || (type == 2 && (categoryCombo->accountSelected(trans->fromAccount()) || categoryCombo->accountSelected(trans->fromAccount()->topAccount()))) || (type == 3 && (categoryCombo->accountSelected(trans->fromAccount()) || categoryCombo->accountSelected(trans->fromAccount()->topAccount())) && descriptionCombo->testTransaction(trans)) || (type == 0 && trans->fromAccount()->type() != ACCOUNT_TYPE_ASSETS)) {
				if(type == 0) sign = 1;
				else if(at == ACCOUNT_TYPE_INCOMES) sign = 1;
				else sign = -1;
				include = true;
			} else if((type == 1 && trans->toAccount()->type() == at) || (type == 2 && (categoryCombo->accountSelected(trans->toAccount()) || categoryCombo->accountSelected(trans->toAccount()->topAccount()))) || (type == 3 && (categoryCombo->accountSelected(trans->toAccount()) || categoryCombo->accountSelected(trans->toAccount()->topAccount())) && descriptionCombo->testTransaction(trans)) || (type == 0 && trans->toAccount()->type() != ACCOUNT_TYPE_ASSETS)) {
				if(type == 0) sign = -1;
				else if(at == ACCOUNT_TYPE_INCOMES) sign = -1;
				else sign = 1;
				include = true;
			}
		}
		if(include) {
			if(!mi || trans->date() > mi->date) {
				QDate newdate, olddate;
				newdate = budget->lastBudgetDay(trans->date());
				if(mi) {
					olddate = mi->date;
					budget->addBudgetMonthsSetLast(olddate, 1);
				} else {
					olddate = budget->lastBudgetDay(first_date);
				}
				while(olddate < newdate) {
					monthly_values.append(month_info());
					mi = &monthly_values.back();
					mi->value = 0.0;
					mi->count = 0.0;
					mi->date = olddate;
					if(b_tags) {for(int i = 0; i < budget->tags.count(); i++) mi->tags[budget->tags[i]] = 0.0;}
					if(b_cats) {
						if(cat) {
							for(AccountList<CategoryAccount*>::const_iterator it = cat->subCategories.constBegin(); it != cat->subCategories.constEnd(); ++it) mi->cats[*it] = 0.0;
							mi->cats[cat] = 0.0;
						} else if(selected_categories.count() > 0) {
							for(QList<Account*>::const_iterator it = selected_categories.constBegin(); it != selected_categories.constEnd(); ++it) mi->cats[*it] = 0.0;
						} else {
							if(at != ACCOUNT_TYPE_EXPENSES) {for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) mi->cats[*it] = 0.0;}
							if(at != ACCOUNT_TYPE_INCOMES) {for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) mi->cats[*it] = 0.0;}
						}
					}
					budget->addBudgetMonthsSetLast(olddate, 1);
				}
				monthly_values.append(month_info());
				mi = &monthly_values.back();
				if(b_tags) {for(int i = 0; i < budget->tags.count(); i++) mi->tags[budget->tags[i]] = 0.0;}
				if(b_cats) {
					if(cat) {
						for(AccountList<CategoryAccount*>::const_iterator it = cat->subCategories.constBegin(); it != cat->subCategories.constEnd(); ++it) mi->cats[*it] = 0.0;
						mi->cats[cat] = 0.0;
					} else if(selected_categories.count() > 0) {
						for(QList<Account*>::const_iterator it = selected_categories.constBegin(); it != selected_categories.constEnd(); ++it) mi->cats[*it] = 0.0;
					} else {
						if(at != ACCOUNT_TYPE_EXPENSES) {for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) mi->cats[*it] = 0.0;}
						if(at != ACCOUNT_TYPE_INCOMES) {for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) mi->cats[*it] = 0.0;}
					}
				}
				if(type == 0) {
					if(sign == 1) mi->value = trans->value(!single_assets);
					else mi->expense = trans->value(!single_assets);
					mi->count = trans->quantity();
				} else if(type == 4) {
					if(accountCombo->transactionChange(trans) >= 0.0) mi->value = accountCombo->transactionChange(trans);
					else mi->expense = -accountCombo->transactionChange(trans);
					mi->count = 1.0;
				} else if(type == 6) {
					mi->value = accountCombo->transactionChange(trans, true);
					mi->count = 1.0;
				} else if(type == 5) {
					mi->expense = 0.0;
					mi->value = 0.0;
					if(trans->fromAccount()->type() == ACCOUNT_TYPE_ASSETS) {
						if(((AssetsAccount*) trans->fromAccount())->accountType() == ASSETS_TYPE_LIABILITIES || ((AssetsAccount*) trans->fromAccount())->accountType() == ASSETS_TYPE_CREDIT_CARD) mi->expense -= trans->value(true);
						else if(((AssetsAccount*) trans->fromAccount())->accountType() != ASSETS_TYPE_SECURITIES && trans->fromAccount() != budget->balancingAccount) mi->value -= trans->value(true);
					}
					if(trans->toAccount()->type() == ACCOUNT_TYPE_ASSETS) {
						if(((AssetsAccount*) trans->toAccount())->accountType() == ASSETS_TYPE_LIABILITIES || ((AssetsAccount*) trans->toAccount())->accountType() == ASSETS_TYPE_CREDIT_CARD) mi->expense += trans->value(true);
						else if(((AssetsAccount*) trans->toAccount())->accountType() != ASSETS_TYPE_SECURITIES && trans->toAccount() != budget->balancingAccount) mi->value += trans->value(true);
					}
				} else {
					mi->value = trans->value(!single_assets) * sign;
					mi->count = trans->quantity();
				}
				if(b_tags) {for(int i = 0; i < trans->tagsCount(true); i++) mi->tags[trans->getTag(i, true)] = mi->value;}
				if(b_cats) {
					if((at == ACCOUNT_TYPE_ASSETS && (trans->fromAccount()->type() == ACCOUNT_TYPE_INCOMES || trans->fromAccount()->type() == ACCOUNT_TYPE_EXPENSES)) || (at != ACCOUNT_TYPE_ASSETS && trans->fromAccount()->type() == at)) mi->cats[cat ? trans->fromAccount() : trans->fromAccount()->topAccount()] = mi->value;
					else if((at == ACCOUNT_TYPE_ASSETS && (trans->toAccount()->type() == ACCOUNT_TYPE_INCOMES || trans->toAccount()->type() == ACCOUNT_TYPE_EXPENSES)) || (at != ACCOUNT_TYPE_ASSETS && trans->toAccount()->type() == at)) mi->cats[cat ? trans->toAccount() : trans->toAccount()->topAccount()] = mi->value;
				}
				mi->date = newdate;
			} else {
				if(type == 0) {
					if(sign == 1) mi->value += trans->value(!single_assets);
					else mi->expense += trans->value(!single_assets);
					mi->count += trans->quantity();
				} else if(type == 4) {
					if(accountCombo->transactionChange(trans) >= 0.0) mi->value += accountCombo->transactionChange(trans);
					else mi->expense -= accountCombo->transactionChange(trans);
					mi->count++;
				} else if(type == 6) {
					mi->value += accountCombo->transactionChange(trans, true);
					mi->count++;
				} else if(type == 5) {
					if(trans->fromAccount()->type() == ACCOUNT_TYPE_ASSETS) {
						if(((AssetsAccount*) trans->fromAccount())->accountType() == ASSETS_TYPE_LIABILITIES || ((AssetsAccount*) trans->fromAccount())->accountType() == ASSETS_TYPE_CREDIT_CARD) mi->expense -= trans->value(true);
						else if(((AssetsAccount*) trans->fromAccount())->accountType() != ASSETS_TYPE_SECURITIES && trans->fromAccount() != budget->balancingAccount) mi->value -= trans->value(true);
					}
					if(trans->toAccount()->type() == ACCOUNT_TYPE_ASSETS) {
						if(((AssetsAccount*) trans->toAccount())->accountType() == ASSETS_TYPE_LIABILITIES || ((AssetsAccount*) trans->toAccount())->accountType() == ASSETS_TYPE_CREDIT_CARD) mi->expense += trans->value(true);
						else if(((AssetsAccount*) trans->toAccount())->accountType() != ASSETS_TYPE_SECURITIES && trans->toAccount() != budget->balancingAccount) mi->value += trans->value(true);
					}
				} else {
					double v = trans->value(!single_assets) * sign;
					mi->value += v;
					mi->count += trans->quantity();
					if(b_tags) {for(int i = 0; i < trans->tagsCount(true); i++) mi->tags[trans->getTag(i, true)] += v;}
					if(b_cats) {
						if((at == ACCOUNT_TYPE_ASSETS && (trans->fromAccount()->type() == ACCOUNT_TYPE_INCOMES || trans->fromAccount()->type() == ACCOUNT_TYPE_EXPENSES)) || (at != ACCOUNT_TYPE_ASSETS && trans->fromAccount()->type() == at)) mi->cats[trans->fromAccount()] += v;
						else if((at == ACCOUNT_TYPE_ASSETS && (trans->toAccount()->type() == ACCOUNT_TYPE_INCOMES || trans->toAccount()->type() == ACCOUNT_TYPE_EXPENSES)) || (at != ACCOUNT_TYPE_ASSETS && trans->toAccount()->type() == at)) mi->cats[trans->toAccount()] += v;
					}
				}
			}
		}
	}
	if(mi) {
		while(mi->date < curdate) {
			QDate newdate = mi->date;
			budget->addBudgetMonthsSetLast(newdate, 1);
			monthly_values.append(month_info());
			mi = &monthly_values.back();
			mi->value = 0.0;
			mi->expense = 0.0;
			mi->count = 0.0;
			mi->date = newdate;
			if(b_tags) {for(int i = 0; i < budget->tags.count(); i++) mi->tags[budget->tags[i]] = 0.0;}
			if(b_cats) {
				if(cat) {
					for(AccountList<CategoryAccount*>::const_iterator it = cat->subCategories.constBegin(); it != cat->subCategories.constEnd(); ++it) mi->cats[*it] = 0.0;
					mi->cats[cat] = 0.0;
				} else if(selected_categories.count() > 0) {
					for(QList<Account*>::const_iterator it = selected_categories.constBegin(); it != selected_categories.constEnd(); ++it) mi->cats[*it] = 0.0;
				} else {
					if(at != ACCOUNT_TYPE_EXPENSES) {for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) mi->cats[*it] = 0.0;}
					if(at != ACCOUNT_TYPE_INCOMES) {for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) mi->cats[*it] = 0.0;}
				}
			}
		}
	}
	double scheduled_value = 0.0;
	double scheduled_expense = 0.0;
	double scheduled_count = 0.0;
	QMap<QString, double> tag_scheduled_value;
	QMap<Account*, double> cat_scheduled_value;
	if(b_tags) {for(int i = 0; i < budget->tags.count(); i++) tag_scheduled_value[budget->tags[i]] = 0.0;}
	if(b_cats) {
		if(cat) {
			for(AccountList<CategoryAccount*>::const_iterator it = cat->subCategories.constBegin(); it != cat->subCategories.constEnd(); ++it) cat_scheduled_value[*it] = 0.0;
			cat_scheduled_value[cat] = 0.0;
		} else if(selected_categories.count() > 0) {
			for(QList<Account*>::const_iterator it = selected_categories.constBegin(); it != selected_categories.constEnd(); ++it) cat_scheduled_value[*it] = 0.0;
		} else {
			if(at != ACCOUNT_TYPE_EXPENSES) {for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) cat_scheduled_value[*it] = 0.0;}
			if(at != ACCOUNT_TYPE_INCOMES) {for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) cat_scheduled_value[*it] = 0.0;}
		}
	}
	if(mi) {
		int split_i = 0;
		for(ScheduledTransactionList<ScheduledTransaction*>::const_iterator it = budget->scheduledTransactions.constBegin(); it != budget->scheduledTransactions.constEnd();) {
			ScheduledTransaction *strans = *it;
			if(strans->firstOccurrence() > mi->date) break;
			started = true;
			if(split_i == 0 && strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT && ((SplitTransaction*) strans->transaction())->count() == 0) {
				do {
					++it;
					if(it == budget->scheduledTransactions.constEnd()) {
						strans = NULL;
						break;
					}
					strans = *it;
					if(strans->firstOccurrence() > mi->date) {
						strans = NULL;
						break;
					}
				} while(split_i == 0 && strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT && ((SplitTransaction*) strans->transaction())->count() == 0);
				if(!strans) break;
			}
			Transaction *trans = NULL;
			if(strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT) {
				trans = ((SplitTransaction*) strans->transaction())->at(split_i);
				split_i++;
			} else {
				trans = (Transaction*) strans->transaction();
			}
			bool include = false;
			int sign = 1;
			if((!assets_selected || accountCombo->testTransactionRelation(trans, type == 6)) && ((current_source != 13 && current_source != 14) || tagCombo->testTransaction(trans))) {
				if(type == 7 || (type == 8 && descriptionCombo->testTransaction(trans))) {
					include = true;
					if(trans->type() == TRANSACTION_TYPE_EXPENSE) {b_expense = true; sign = -1;}
					else if(trans->type() == TRANSACTION_TYPE_INCOME) {b_income = true; sign = 1;}
					else include = false;
				} else if(type >= 4 && type != 8) {
					include = true;
				} else if((type == 1 && trans->fromAccount()->type() == at) || (type == 2 && (categoryCombo->accountSelected(trans->fromAccount()) || categoryCombo->accountSelected(trans->fromAccount()->topAccount()))) || (type == 3 && (categoryCombo->accountSelected(trans->fromAccount()) || categoryCombo->accountSelected(trans->fromAccount()->topAccount())) && descriptionCombo->testTransaction(trans)) || (type == 0 && trans->fromAccount()->type() != ACCOUNT_TYPE_ASSETS)) {
					if(type == 0) sign = 1;
					else if(at == ACCOUNT_TYPE_INCOMES) sign = 1;
					else sign = -1;
					include = true;
				} else if((type == 1 && trans->toAccount()->type() == at) || (type == 2 && (categoryCombo->accountSelected(trans->toAccount()) || categoryCombo->accountSelected(trans->toAccount()->topAccount()))) || (type == 3 && (categoryCombo->accountSelected(trans->toAccount()) || categoryCombo->accountSelected(trans->toAccount()->topAccount())) && descriptionCombo->testTransaction(trans)) || (type == 0 && trans->toAccount()->type() != ACCOUNT_TYPE_ASSETS)) {
					if(type == 0) sign = -1;
					else if(at == ACCOUNT_TYPE_INCOMES) sign = -1;
					else sign = 1;
					include = true;
				}
			}
			if(include) {
				int count = (strans->recurrence() ? strans->recurrence()->countOccurrences(mi->date) : 1);
				if(count != 0) {
					includes_planned = true;
					if(type == 0) {
						if(sign == 1) scheduled_value += (trans->value(!single_assets) * count);
						else scheduled_expense += (trans->value(!single_assets) * count);
						scheduled_count += count * trans->quantity();
					} else if(type == 4) {
						if(accountCombo->transactionChange(trans) >= 0.0) scheduled_value += accountCombo->transactionChange(trans) * count;
						else scheduled_expense -= accountCombo->transactionChange(trans) * count;
						scheduled_count += count;
					} else if(type == 6) {
						scheduled_value += accountCombo->transactionChange(trans, true) * count;
						scheduled_count += count;
					} else if(type == 5) {
						if(trans->fromAccount()->type() == ACCOUNT_TYPE_ASSETS) {
							if(((AssetsAccount*) trans->fromAccount())->accountType() == ASSETS_TYPE_LIABILITIES || ((AssetsAccount*) trans->fromAccount())->accountType() == ASSETS_TYPE_CREDIT_CARD) scheduled_expense -= trans->value(true) * count;
							else if(((AssetsAccount*) trans->fromAccount())->accountType() != ASSETS_TYPE_SECURITIES && trans->fromAccount() != budget->balancingAccount) scheduled_value -= trans->value(true) * count;
						}
						if(trans->toAccount()->type() == ACCOUNT_TYPE_ASSETS) {
							if(((AssetsAccount*) trans->toAccount())->accountType() == ASSETS_TYPE_LIABILITIES || ((AssetsAccount*) trans->toAccount())->accountType() == ASSETS_TYPE_CREDIT_CARD) scheduled_expense += trans->value(true) * count;
							else if(((AssetsAccount*) trans->toAccount())->accountType() != ASSETS_TYPE_SECURITIES && trans->toAccount() != budget->balancingAccount) scheduled_value += trans->value(true) * count;
						}
					} else {
						double v = (trans->value(!single_assets) * sign * count);
						scheduled_value += v;
						scheduled_count += count * trans->quantity();
						if(b_tags) {
							for(int i = 0; i < trans->tagsCount(true); i++) {tag_scheduled_value[trans->getTag(i, true)] += v; tag_includes_planned[trans->getTag(i, true)] = true;}
						}
						if(b_cats) {
							if((at == ACCOUNT_TYPE_ASSETS && (trans->fromAccount()->type() == ACCOUNT_TYPE_INCOMES || trans->fromAccount()->type() == ACCOUNT_TYPE_EXPENSES)) || (at != ACCOUNT_TYPE_ASSETS && trans->fromAccount()->type() == at)) {cat_scheduled_value[trans->fromAccount()] += v; cat_includes_planned[trans->fromAccount()] = true;}
							else if((at == ACCOUNT_TYPE_ASSETS && (trans->toAccount()->type() == ACCOUNT_TYPE_INCOMES || trans->toAccount()->type() == ACCOUNT_TYPE_EXPENSES)) || (at != ACCOUNT_TYPE_ASSETS && trans->toAccount()->type() == at)) {cat_scheduled_value[trans->toAccount()] += v; cat_includes_planned[trans->toAccount()] = true;}
						}
					}
				}
			}
			if(strans->transaction()->generaltype() != GENERAL_TRANSACTION_TYPE_SPLIT || split_i >= ((SplitTransaction*) strans->transaction())->count()) {
				++it;
				split_i = 0;
			}
		}
	}
	if(monthly_values.isEmpty()) {
		monthly_values.append(month_info());
		mi = &monthly_values.back();
		mi->value = 0.0;
		mi->expense = 0.0;
		mi->count = 0.0;
		mi->date = budget->lastBudgetDay(first_date);
		while(mi->date < curdate) {
			QDate newdate = mi->date;
			budget->addBudgetMonthsSetLast(newdate, 1);
			monthly_values.append(month_info());
			mi = &monthly_values.back();
			mi->value = 0.0;
			mi->expense = 0.0;
			mi->count = 0.0;
			mi->date = newdate;
			if(b_tags) {for(int i = 0; i < budget->tags.count(); i++) mi->tags[budget->tags[i]] = 0.0;}
			if(b_cats) {
				if(cat) {
					for(AccountList<CategoryAccount*>::const_iterator it = cat->subCategories.constBegin(); it != cat->subCategories.constEnd(); ++it) mi->cats[*it] = 0.0;
					mi->cats[cat] = 0.0;
				} else if(selected_categories.count() > 0) {
					for(QList<Account*>::const_iterator it = selected_categories.constBegin(); it != selected_categories.constEnd(); ++it) mi->cats[*it] = 0.0;
				} else {
					if(at != ACCOUNT_TYPE_EXPENSES) {for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) mi->cats[*it] = 0.0;}
					if(at != ACCOUNT_TYPE_INCOMES) {for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) mi->cats[*it] = 0.0;}
				}
			}
		}
	}
	Currency *currency = budget->defaultCurrency();
	if(single_assets) currency = ((AssetsAccount*) accountCombo->selectedAccounts()[0])->currency();

	if(current_source == 12) {
		if(type == 6) {
			QList<Account*> account_list = accountCombo->selectedAccounts();
			double total_value = 0.0;
			for(QList<Account*>::const_iterator it = account_list.constBegin(); it != account_list.constEnd(); ++it) {
				AssetsAccount *current_assets = (AssetsAccount*) *it;
				if(current_assets->accountType() != ASSETS_TYPE_SECURITIES) {
					total_value += current_assets->currency()->convertTo(current_assets->initialBalance(false), currency, start_date);
				}
			}
			QVector<month_info>::iterator it_b = monthly_values.begin();
			QVector<month_info>::iterator it_e = monthly_values.end();
			while(it_b != it_e) {
				total_value += it_b->value;
				it_b->value = total_value;
				for(QList<Account*>::const_iterator it = account_list.constBegin(); it != account_list.constEnd(); ++it) {
					AssetsAccount *current_assets = (AssetsAccount*) *it;
					if(current_assets->accountType() == ASSETS_TYPE_SECURITIES) {
						for(SecurityList<Security*>::const_iterator it_s = budget->securities.constBegin(); it_s != budget->securities.constEnd(); ++it_s) {
							if((*it_s)->account() == current_assets) {
								it_b->value += current_assets->currency()->convertTo((*it_s)->value(it_b->date, -1), currency, it_b->date);
							}
						}
					}
				}
				it_b++;
			}
		} else if(type == 5) {
			double total_value = 0.0, total_expense = 0.0;
			for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
				AssetsAccount *account = *it;
				if(account->accountType() == ASSETS_TYPE_LIABILITIES || account->accountType() == ASSETS_TYPE_CREDIT_CARD) total_expense += account->currency()->convertTo(account->initialBalance(false), budget->defaultCurrency(), start_date);
				else total_value += account->currency()->convertTo(account->initialBalance(false), budget->defaultCurrency(), start_date);
			}
			QVector<month_info>::iterator it_b = monthly_values.begin();
			QVector<month_info>::iterator it_e = monthly_values.end();
			while(it_b != it_e) {
				total_value += it_b->value;
				it_b->value = total_value;
				total_expense += it_b->expense;
				it_b->expense = total_expense;
				it_b++;
			}
			for(SecurityList<Security*>::const_iterator it = budget->securities.constBegin(); it != budget->securities.constEnd(); ++it) {
				Security *sec = *it;
				it_b = monthly_values.begin();
				it_e = monthly_values.end();
				while(it_b != it_e) {
					it_b->value += sec->currency()->convertTo(sec->value(it_b->date, -1), budget->defaultCurrency(), it_b->date);
					it_b++;
				}
			}
		}
	}
	QStringList tags;
	if(b_tags) {
		for(int i = 0; i < budget->tags.count(); i++) {
			if(tag_includes_planned[budget->tags[i]]) {
				tags << budget->tags[i];
			} else {
				for(QVector<month_info>::iterator mit = monthly_values.begin(); mit != monthly_values.end(); ++mit) {
					if(mit->tags[budget->tags[i]] != 0.0) {
						tags << budget->tags[i];
						break;
					}
				}
			}
		}
		if(tags.isEmpty()) tags = budget->tags;
	}
	QVector<Account*> cats;
	if(b_cats) {
		if(cat) {
			if(cat->subCategories.isEmpty()) {
				cats << cat;
			} else {
				for(AccountList<CategoryAccount*>::const_iterator it = cat->subCategories.constBegin(); it != cat->subCategories.constEnd(); ++it) {
					cats << *it;
				}
				if(cat_includes_planned[cat]) {
					cats << cat;
				} else {
					for(QVector<month_info>::iterator mit = monthly_values.begin(); mit != monthly_values.end(); ++mit) {
						if(mit->cats[cat] != 0.0) {
							cats << cat;
							break;
						}
					}
				}
			}
		} else if(selected_categories.count() > 0) {
			Account *acc = NULL;
			bool b_subs = true;
			for(QList<Account*>::const_iterator it = selected_categories.constBegin(); it != selected_categories.constEnd(); ++it) {
				bool b = false;
				if(cat_includes_planned[*it]) {
					b = true;
				} else {
					for(QVector<month_info>::iterator mit = monthly_values.begin(); mit != monthly_values.end(); ++mit) {
						if(mit->cats[*it] != 0.0) {
							b = true;
							break;
						}
					}
				}
				if(b) {
					if(!acc) acc = (*it)->topAccount();
					else if(acc != (*it)->topAccount()) b_subs = false;
					cats << *it;
				}
			}
			if(!b_subs) {
				for(int i = 0; i < cats.count();) {
					Account *acc = cats[i]->topAccount();
					if(acc != cats[i]) {
						if(!cat_includes_planned[acc]) cat_includes_planned[acc] = cat_includes_planned[cats[i]];
						cat_scheduled_value[acc] += cat_scheduled_value[cats[i]];
						for(QVector<month_info>::iterator mit = monthly_values.begin(); mit != monthly_values.end(); ++mit) {
							mit->cats[acc] += mit->cats[cats[i]];
						}
						cats.removeAt(i);
						if(!cats.contains(acc)) cats << acc;
					} else {
						i++;
					}
				}
			} else if(cats.isEmpty()) {
				b_subs = false;
				for(QList<Account*>::const_iterator it = selected_categories.constBegin(); it != selected_categories.constEnd(); ++it) {
					if(*it == (*it)->topAccount()) {
						cats << *it;
					}
				}
			}
		} else {
			Account *acc = NULL;
			bool b_subs = true;
			if(at != ACCOUNT_TYPE_EXPENSES) {
				for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
					bool b = false;
					if(cat_includes_planned[*it]) {
						b = true;
					} else {
						for(QVector<month_info>::iterator mit = monthly_values.begin(); mit != monthly_values.end(); ++mit) {
							if(mit->cats[*it] != 0.0) {
								b = true;
								break;
							}
						}
					}
					if(b) {
						if(!acc) acc = (*it)->topAccount();
						else if(acc != (*it)->topAccount()) b_subs = false;
						cats << *it;
					}
				}
			}
			if(at != ACCOUNT_TYPE_INCOMES) {
				for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
					bool b = false;
					if(cat_includes_planned[*it]) {
						b = true;
					} else {
						for(QVector<month_info>::iterator mit = monthly_values.begin(); mit != monthly_values.end(); ++mit) {
							if(mit->cats[*it] != 0.0) {
								b = true;
								break;
							}
						}
					}
					if(b) {
						if(!acc) acc = (*it)->topAccount();
						else if(acc != (*it)->topAccount()) b_subs = false;
						cats << *it;
					}
				}
			}
			if(!b_subs) {
				for(int i = 0; i < cats.count();) {
					Account *acc = cats[i]->topAccount();
					if(acc != cats[i]) {
						if(!cat_includes_planned[acc]) cat_includes_planned[acc] = cat_includes_planned[cats[i]];
						cat_scheduled_value[acc] += cat_scheduled_value[cats[i]];
						for(QVector<month_info>::iterator mit = monthly_values.begin(); mit != monthly_values.end(); ++mit) {
							mit->cats[acc] += mit->cats[cats[i]];
						}
						cats.removeAt(i);
						if(!cats.contains(acc)) cats << acc;
					} else {
						i++;
					}
				}
			} else if(cats.isEmpty()) {
				b_subs = false;
				if(at != ACCOUNT_TYPE_EXPENSES) {
					for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
						if(*it == (*it)->topAccount()) {
							cats << *it;
						}
					}
				}
				if(at != ACCOUNT_TYPE_INCOMES) {
					for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
						if(*it == (*it)->topAccount()) {
							cats << *it;
						}
					}
				}
			}
		}
	}

	if(current_source == 13 || current_source == 14) {
		if(b_expense && !b_income) {
			pertitle = tr("Average Cost");
			valuetitle = tr("Expenses");
			for(QVector<month_info>::iterator mit = monthly_values.begin(); mit != monthly_values.end(); ++mit) {
				mit->value = -mit->value;
				mit->expense = -mit->expense;
				if(b_tags) {
					for(int i = 0; i < tags.count(); i++) {
						mit->tags[tags[i]] = -mit->tags[tags[i]];
					}
				}
				if(b_cats) {
					for(int i = 0; i < cats.count(); i++) {
						mit->cats[cats[i]] = -mit->cats[cats[i]];
					}
				}
			}
			scheduled_value = -scheduled_value;
			scheduled_expense = -scheduled_expense;
			if(b_tags) {
				for(int i = 0; i < tags.count(); i++) {
					tag_scheduled_value[tags[i]] = -tag_scheduled_value[tags[i]];
				}
			}
			if(b_cats) {
				for(int i = 0; i < cats.count(); i++) {
					cat_scheduled_value[cats[i]] = -cat_scheduled_value[cats[i]];
				}
			}
			if(current_source == 13) {
				if(assets_selected) title = tr("Expenses, %2: %1").arg(tagCombo->selectedItemsText(1)).arg(accountCombo->selectedAccountsText(2));
				else title = tr("Expenses: %1").arg(tagCombo->selectedItemsText(1));
			} else {
				if(assets_selected) title = tr("Expenses, %3: %2, %1").arg(tagCombo->selectedItemsText(2)).arg(descriptionCombo->selectedItemsText(2)).arg(accountCombo->selectedAccountsText(2));
				else title = tr("Expenses: %2, %1").arg(tagCombo->selectedItemsText(2)).arg(descriptionCombo->selectedItemsText(2));
			}
		} else if(b_income && !b_expense) {
			pertitle = tr("Average Income");
			valuetitle = tr("Incomes");
			if(current_source == 13) {
				if(assets_selected) title = tr("Incomes, %2: %1").arg(tagCombo->selectedItemsText(1)).arg(accountCombo->selectedAccountsText(2));
				else title = tr("Incomes: %1").arg(tagCombo->selectedItemsText(1));
			} else {
				if(assets_selected) title = tr("Incomes, %3: %2, %1").arg(tagCombo->selectedItemsText(2)).arg(descriptionCombo->selectedItemsText(2)).arg(accountCombo->selectedAccountsText(2));
				else title = tr("Incomes: %2, %1").arg(tagCombo->selectedItemsText(2)).arg(descriptionCombo->selectedItemsText(2));
			}
		}
	}
	double average_month = budget->averageMonth(first_date, curdate, true);
	double average_year = budget->averageYear(first_date, curdate, true);
	source = "";
	QTextStream outf(&source, QIODevice::WriteOnly);
	outf << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">" << '\n';
	outf << "<html>" << '\n';
	outf << "\t<head>" << '\n';
	outf << "\t\t<title>";
	outf << htmlize_string(title);
	outf << "</title>" << '\n';
	outf << "\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" << '\n';
	outf << "\t\t<meta name=\"GENERATOR\" content=\"" << qApp->applicationDisplayName() << "\">" << '\n';
	outf << "\t</head>" << '\n';
	outf << "\t<body bgcolor=\"white\" style=\"color: black; margin-top: 10; margin-bottom: 10; margin-left: 10; margin-right: 10\">" << '\n';
	outf << "\t\t<h2 align=\"center\">" << htmlize_string(title) << "</h2>" << '\n' << "\t\t<br>";
	outf << "\t\t<table width=\"100%\" border=\"0.5\" style=\"border-style: solid; border-color: #cccccc\" cellspacing=\"0\" cellpadding=\"5\">" << '\n';
	outf << "\t\t\t<thead align=\"left\">" << '\n';
	outf << "\t\t\t\t<tr bgcolor=\"#f0f0f0\">" << '\n';
	outf << "\t\t\t\t\t<th align=\"left\">" << htmlize_string(tr("Year")) << "</th>";
	outf << "\t\t\t\t\t<th align=\"left\">" << htmlize_string(tr("Month")) << "</th>";
	bool use_footer1 = false;
	if(enabled[0]) {
		outf << "\t\t\t\t\t<th align=\"left\">" << htmlize_string(valuetitle);
		if(includes_planned) {outf << "*"; use_footer1 = true;}
		outf<< "</th>";
	}
	if(enabled[1]) outf << "\t\t\t\t\t<th align=\"left\">" << htmlize_string(tr("Daily Average")) << "</th>";
	if(enabled[2]) outf << "\t\t\t\t\t<th align=\"left\">" << htmlize_string(tr("Monthly Average")) << (use_footer1 ? "**" : "*") << "</th>";
	if(enabled[3]) outf << "\t\t\t\t\t<th align=\"left\">" << htmlize_string(tr("Yearly Average")) << (use_footer1 ? "**" : "*") << "</th>";
	if(enabled[4]) {
		outf << "\t\t\t\t\t<th align=\"left\">" << htmlize_string(tr("Quantity"));
		if(includes_planned) {outf << "*"; use_footer1 = true;}
		outf<< "</th>";
	}
	if(enabled[5]) {
		outf << "\t\t\t\t\t<th align=\"left\">" << htmlize_string(pertitle);
		if(includes_planned) {outf << "*"; use_footer1 = true;}
		outf<< "</th>";
	}
	if(enabled[6]) {
		outf << "\t\t\t\t\t<th align=\"left\">" << htmlize_string(expensetitle);
		if(includes_planned) {outf << "*"; use_footer1 = true;}
		outf<< "</th>";
	}
	if(enabled[7]) {
		outf << "\t\t\t\t\t<th align=\"left\">" << htmlize_string(sumtitle);
		if(includes_planned) {outf << "*"; use_footer1 = true;}
		outf<< "</th>";
	}
	if(b_tags) {
		for(int i = 0; i < tags.count(); i++) {
			outf << "\t\t\t\t\t<th align=\"left\">" << htmlize_string(tags[i]);
			if(tag_includes_planned[tags[i]]) {outf << "*"; use_footer1 = true;}
			outf<< "</th>";
		}
	}
	if(b_cats) {
		for(int i = 0; i < cats.count(); i++) {
			outf << "\t\t\t\t\t<th align=\"left\">" << htmlize_string(cats[i]->name());
			if(cat_includes_planned[cats[i]]) {outf << "*"; use_footer1 = true;}
			outf<< "</th>";
		}
		if(cats.count() > 1) {
			outf << "\t\t\t\t\t<th align=\"left\">" << htmlize_string(tr("Total"));
			if(includes_planned) {outf << "*"; use_footer1 = true;}
			outf<< "</th>";
		}
	}
	
	outf << "\t\t\t\t</tr>" << '\n';
	outf << "\t\t\t</thead>" << '\n';
	outf << "\t\t\t<tbody>" << '\n';
	QVector<month_info>::iterator it_b = monthly_values.begin();
	QVector<month_info>::iterator it_e = monthly_values.end();
	if(it_e != it_b) --it_e;
	QVector<month_info>::iterator it = monthly_values.end();
	int year = 0;
	double yearly_value = 0.0, total_value = 0.0;
	double yearly_expense = 0.0, total_expense = 0.0;
	double yearly_count = 0.0, total_count = 0.0;
	QMap<QString, double> tag_total_value, tag_yearly_value;
	QMap<Account*, double> cat_total_value, cat_yearly_value;
	if(b_tags) {for(int i = 0; i < tags.count(); i++) {tag_yearly_value[tags[i]] = 0.0; tag_total_value[tags[i]] = 0.0;}}
	if(b_cats) {for(int i = 0; i < cats.count(); i++) {cat_yearly_value[cats[i]] = 0.0; cat_total_value[cats[i]] = 0.0;}}
	QDate year_date;
	bool first_year = true, first_month = true;
	bool multiple_months = monthly_values.size() > 1;
	bool multiple_years = multiple_months && budget->budgetYear(first_date) != budget->budgetYear(curdate);
	int i_count_frac = 0;
	double intpart = 0.0;
	while(it != it_b) {
		--it;
		if(modf(it->count, &intpart) != 0.0) {
			i_count_frac = 2;
			break;
		}
	}
	it = monthly_values.end();
	while(it != it_b) {
		--it;
		if(first_month || year != budget->budgetYear(it->date)) {
			if(!first_month && multiple_years && current_source != 12) {
				outf << "\t\t\t\t<tr bgcolor=\"#f0f0f0\">" << '\n';
				outf << "\t\t\t\t\t<td></td>";
				outf << "\t\t\t\t\t<td align=\"left\"><b>" << htmlize_string(tr("Subtotal")) << "</b></td>";
				if(enabled[0]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(first_year ? (yearly_value + scheduled_value) : yearly_value)) << "</b></td>";
				int days = 1;
				if(first_year) {
					days = budget->dayOfBudgetYear(curdate);
				} else if(budget->budgetYear(first_date) == year) {
					days = budget->daysInBudgetYear(year_date);
					days -= (budget->dayOfBudgetYear(first_date) - 1);
				} else {
					days = budget->daysInBudgetYear(year_date);
				}
				if(enabled[1]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(yearly_value / days)) << "</b></td>";
				if(enabled[2]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(((yearly_value * average_month) / days))) << "</b></td>";
				if(enabled[3]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue((yearly_value * average_year) / days)) << "</b></td>";
				if(enabled[4]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(budget->formatValue(first_year ? (yearly_count + scheduled_count) : yearly_count, i_count_frac)) << "</b></td>";
				double pervalue = 0.0;
				if(first_year) {
					pervalue = (((yearly_count + scheduled_count) == 0.0) ? 0.0 : ((yearly_value + scheduled_value) / (yearly_count + scheduled_count)));
				} else {
					pervalue = (yearly_count == 0.0 ? 0.0 : (yearly_value / yearly_count));
				}
				if(enabled[5]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(pervalue)) << "</b></td>";
				if(enabled[6]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(first_year ? (yearly_expense + scheduled_expense) : yearly_expense)) << "</b></td>";
				if(enabled[7]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(first_year ? (yearly_value + scheduled_value - yearly_expense - scheduled_expense) : yearly_value - yearly_expense)) << "</b></td>";
				if(b_tags) {
					for(int i = 0; i < tags.count(); i++) {
						outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(first_year ? (tag_yearly_value[tags[i]] + tag_scheduled_value[tags[i]]) : tag_yearly_value[tags[i]])) << "</b></td>";
					}
				}
				if(b_cats) {
					for(int i = 0; i < cats.count(); i++) {
						outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(first_year ? (cat_yearly_value[cats[i]] + cat_scheduled_value[cats[i]]) : cat_yearly_value[cats[i]])) << "</b></td>";
					}
					if(cats.count() > 1) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(first_year ? (yearly_value + scheduled_value) : yearly_value)) << "</b></td>";
				}
				first_year = false;
				outf << "\n";
				outf << "\t\t\t\t</tr>" << '\n';
				outf << "\t\t\t\t<tr>" << '\n';
			} else if(!first_month && multiple_years && current_source == 12) {
				outf << "\t\t\t\t<tr bgcolor=\"#f0f0f0\">" << '\n';
			} else {
				outf << "\t\t\t\t<tr>" << '\n';
			}
			year = budget->budgetYear(it->date);
			yearly_value = it->value;
			yearly_expense = it->expense;
			yearly_count = it->count;
			year_date = it->date;
			if(b_tags) {for(int i = 0; i < tags.count(); i++) tag_yearly_value[tags[i]] = it->tags[tags[i]];}
			if(b_cats) {for(int i = 0; i < cats.count(); i++) cat_yearly_value[cats[i]] = it->cats[cats[i]];}
			outf << "\t\t\t\t\t<td align=\"left\">" << htmlize_string(budget->budgetYearString(it->date)) << "</td>";
		} else {
			outf << "\t\t\t\t<tr>" << '\n';
			yearly_value += it->value;
			yearly_expense += it->expense;
			yearly_count += it->count;
			if(b_tags) {for(int i = 0; i < tags.count(); i++) tag_yearly_value[tags[i]] += it->tags[tags[i]];}
			if(b_cats) {for(int i = 0; i < cats.count(); i++) cat_yearly_value[cats[i]] += it->cats[cats[i]];}
			outf << "\t\t\t\t\t<td></td>";
		}
		total_value += it->value;
		total_expense += it->expense;
		total_count += it->count;
		outf << "\t\t\t\t\t<td align=\"left\">" << htmlize_string(QLocale().monthName(budget->budgetMonth(it->date), QLocale::LongFormat)) << "</td>";
		if(enabled[0]) outf << "<td nowrap align=\"right\">" << htmlize_string(currency->formatValue(first_month ? (it->value + scheduled_value) : it->value)) << "</td>";
		int days = 0;
		if(first_month) {
			days = budget->dayOfBudgetMonth(curdate);
		} else if(it == it_b) {
			days = budget->daysInBudgetMonth(it->date);
			days -= (budget->dayOfBudgetMonth(first_date) - 1);
		} else {
			days = budget->dayOfBudgetMonth(it->date);
		}
		if(enabled[1]) outf << "<td nowrap align=\"right\">" << htmlize_string(currency->formatValue(it->value / days)) << "</td>";
		if(enabled[2]) outf << "<td nowrap align=\"right\">" << htmlize_string(currency->formatValue((it->value * average_month) / days)) << "</td>";
		if(enabled[3]) outf << "<td nowrap align=\"right\">" << htmlize_string(currency->formatValue((it->value * average_year) / days)) << "</td>";
		if(enabled[4]) outf << "<td nowrap align=\"right\">" << htmlize_string(budget->formatValue(first_month ? (it->count + scheduled_count) : it->count, i_count_frac)) << "</td>";
		if(enabled[5]) {
			double pervalue = 0.0;
			if(first_month) {
				pervalue = (((it->count + scheduled_count) == 0.0) ? 0.0 : ((it->value + scheduled_value) / (it->count + scheduled_count)));
			} else {
				pervalue = (it->count == 0.0 ? 0.0 : (it->value / it->count));
			}
			outf << "<td nowrap align=\"right\">" << htmlize_string(currency->formatValue(pervalue)) << "</td>";
		}
		if(enabled[6]) outf << "<td nowrap align=\"right\">" << htmlize_string(currency->formatValue(first_month ? (it->expense + scheduled_expense) : it->expense)) << "</td>";
		if(enabled[7]) outf << "<td nowrap align=\"right\">" << htmlize_string(currency->formatValue(current_source == 12 ? (first_month ? (it->value + it->expense + scheduled_value + scheduled_expense) : it->value + it->expense) : (first_month ? (it->value - it->expense + scheduled_value - scheduled_expense) : it->value - it->expense))) << "</td>";
		if(b_tags) {
			for(int i = 0; i < tags.count(); i++) {
				tag_total_value[tags[i]] += it->tags[tags[i]];
				outf << "<td nowrap align=\"right\">" << htmlize_string(currency->formatValue(first_month ? (it->tags[tags[i]] + tag_scheduled_value[tags[i]]) : it->tags[tags[i]])) << "</td>";
			}
		}
		if(b_cats) {
			for(int i = 0; i < cats.count(); i++) {
				cat_total_value[cats[i]] += it->cats[cats[i]];
				outf << "<td nowrap align=\"right\">" << htmlize_string(currency->formatValue(first_month ? (it->cats[cats[i]] + cat_scheduled_value[cats[i]]) : it->cats[cats[i]])) << "</td>";
			}
			if(cats.count() > 1) outf << "<td nowrap align=\"right\">" << htmlize_string(currency->formatValue(first_month ? (it->value + scheduled_value) : it->value)) << "</td>";
		}
		first_month = false;
		outf << "\n";
		outf << "\t\t\t\t</tr>" << '\n';
	}
	if(multiple_years && current_source != 12) {
		outf << "\t\t\t\t<tr bgcolor=\"#f0f0f0\">" << '\n';
		outf << "\t\t\t\t\t<td></td>";
		outf << "\t\t\t\t\t<td align=\"left\"><b>" << htmlize_string(tr("Subtotal")) << "</b></td>";
		if(enabled[0]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(yearly_value)) << "</b></td>";
		int days = budget->daysInBudgetYear(year_date);
		days -= (budget->dayOfBudgetYear(first_date) - 1);
		if(enabled[1]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(yearly_value / days)) << "</b></td>";
		if(enabled[2]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue((yearly_value * average_month) / days)) << "</b></td>";
		if(enabled[3]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue((yearly_value * average_year) / days)) << "</b></td>";
		if(enabled[4]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(budget->formatValue(yearly_count, i_count_frac)) << "</b></td>";
		if(enabled[5]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(yearly_count == 0.0 ? 0.0 : (yearly_value / yearly_count))) << "</b></td>";
		if(enabled[6]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(yearly_expense)) << "</b></td>";
		if(enabled[7]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(yearly_value - yearly_expense)) << "</b></td>";
		if(b_tags) {
			for(int i = 0; i < tags.count(); i++) {
				outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(tag_yearly_value[tags[i]])) << "</b></td>";
			}
		}
		if(b_cats) {
			for(int i = 0; i < cats.count(); i++) {
				outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(cat_yearly_value[cats[i]])) << "</b></td>";
			}
			if(cats.count() > 1) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(yearly_value)) << "</b></td>";
		}
		outf << "\n";
		outf << "\t\t\t\t</tr>" << '\n';
	}
	if(multiple_months && current_source != 12) {
		outf << "\t\t\t\t<tr bgcolor=\"#f0f0f0\">" << '\n';
		int days = first_date.daysTo(curdate) + 1;
		outf << "\t\t\t\t\t<td align=\"left\"><b>" << htmlize_string(tr("Total")) << "</b></td>";
		outf << "\t\t\t\t\t<td></td>";
		if(enabled[0]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(total_value + scheduled_value)) << "</b></td>";
		if(enabled[1]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(total_value / days)) << "</b></td>";
		if(enabled[2]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue((total_value * average_month) / days)) << "</b></td>";
		if(enabled[3]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue((total_value * average_year) / days)) << "</b></td>";
		if(enabled[4]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(budget->formatValue(total_count + scheduled_count, i_count_frac)) << "</b></td>";
		if(enabled[5]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue((total_count + scheduled_count) == 0.0 ? 0.0 : ((total_value + scheduled_value) / (total_count + scheduled_count)))) << "</b></td>";
		if(enabled[6]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(total_expense + scheduled_expense)) << "</b></td>";
		if(enabled[7]) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(total_value - total_expense + scheduled_value - scheduled_expense)) << "</b></td>";
		if(b_tags) {
			for(int i = 0; i < tags.count(); i++) {
				outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(tag_total_value[tags[i]] + tag_scheduled_value[tags[i]])) << "</b></td>";
			}
		}
		if(b_cats) {
			for(int i = 0; i < cats.count(); i++) {
				outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(cat_total_value[cats[i]] + cat_scheduled_value[cats[i]])) << "</b></td>";
			}
			if(cats.count() > 1) outf << "<td nowrap align=\"right\"><b>" << htmlize_string(currency->formatValue(total_value + scheduled_value)) << "</b></td>";
		}
		outf << "\n";
		outf << "\t\t\t\t</tr>" << '\n';
	}
	outf << "\t\t\t</tbody>" << '\n';
	outf << "\t\t</table>" << '\n';
	outf << "\t\t<div align=\"right\" style=\"font-weight: normal\">" << "<small>" << '\n';
	if(use_footer1) {
		outf << "\t\t\t<br>" << '\n';
		outf << "\t\t\t" << "*" << htmlize_string(tr("Includes scheduled transactions")) << '\n';
	}
	if(enabled[2] || enabled[3]) {
		outf << "\t\t\t" << "<br>" << '\n';
		outf << "\t\t\t" << (use_footer1 ? "**" : "*") << htmlize_string(tr("Adjusted for the average month / year (%1 / %2 days)").arg(budget->formatValue(average_month, 1)).arg(budget->formatValue(average_year, 1))) << '\n';
	}
	outf << "\t\t</small></div>" << '\n';
	outf << "\t</body>" << '\n';
	outf << "</html>" << '\n';
	htmlview->setLineWrapMode(QTextEdit::NoWrap);
	htmlview->setHtml(source);
	if(htmlview->document()->size().width() < htmlview->width()) htmlview->setLineWrapMode(QTextEdit::WidgetWidth);
}
void OverTimeReport::updateTransactions() {
	if(categoryCombo->isVisible()) categoryChanged();
	else tagChanged();
}
void OverTimeReport::updateTags() {
	block_display_update = true;
	tagCombo->blockSignals(true);
	descriptionCombo->blockSignals(true);
	tagCombo->clear();
	tagCombo->updateItems(budget->tags);
	tagCombo->blockSignals(false);
	descriptionCombo->blockSignals(false);
	if(tagCombo->isVisible()) {
		tagChanged();
		block_display_update = false;
		updateDisplay();
	} else {
		block_display_update = false;
	}
}
void OverTimeReport::updateAccounts() {
	block_display_update = true;
	if(categoryCombo->isEnabled() && categoryCombo->isVisible()) {
		categoryCombo->blockSignals(true);
		descriptionCombo->blockSignals(true);
		categoryCombo->clear();
		if(sourceCombo->currentIndex() == 1) {
			categoryCombo->updateAccounts(ACCOUNT_TYPE_EXPENSES);
		} else {
			categoryCombo->updateAccounts(ACCOUNT_TYPE_INCOMES);
		}
		if(categoryCombo->allAccountsSelected()) {
			descriptionCombo->clear();
			descriptionCombo->setEnabled(false);
			if(sourceCombo->currentIndex() == 2) {
				current_source = 1;
			} else {
				current_source = 2;
			}
			descriptionCombo->setEnabled(false);
		} else {
			categoryChanged();
		}
		categoryCombo->blockSignals(false);
		descriptionCombo->blockSignals(false);
	}
	accountCombo->blockSignals(true);
	accountCombo->updateAccounts(ACCOUNT_TYPE_ASSETS);
	accountCombo->blockSignals(false);
	block_display_update = false;
	updateDisplay();
}

