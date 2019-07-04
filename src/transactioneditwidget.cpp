/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016-2019 by Hanna Knutsson            *
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

#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QObject>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QUrl>
#include <QFileDialog>
#include <QAction>
#include <QDateEdit>
#include <QCompleter>
#include <QStandardItemModel>
#include <QStringList>
#include <QKeyEvent>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDirModel>
#include <QDebug>
#include <QSettings>
#include <QMenu>
#include <QInputDialog>

#include "budget.h"
#include "accountcombobox.h"
#include "editaccountdialogs.h"
#include "eqonomizevalueedit.h"
#include "recurrence.h"
#include "eqonomize.h"
#include "transactioneditwidget.h"

#include <cmath>

#define CURROW(row, col)	(b_autoedit ? row % rows : row)
#define CURCOL(row, col)	(b_autoedit ? ((row / rows) * 2) + col : col)
#define TEROWCOL(row, col)	CURROW(row, col), CURCOL(row, col)

TagButton::TagButton(bool small_button, bool allow_new_tag, Budget *budg, QWidget *parent) : QPushButton(parent), b_small(small_button) {
	tagMenu = new TagMenu(budg, this, allow_new_tag);
	setMenu(tagMenu);
	icon_shown = true;
	if(b_small) {
#if defined (Q_OS_WIN32)
		setIcon(LOAD_ICON2("tag", "eqz-tag"));
#else
		setText("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
		int w1 = sizeHint().width();
		setIcon(LOAD_ICON2("tag", "eqz-tag"));
		int w2 = sizeHint().width();
		if(w1 == w2) {
			setIcon(QIcon());
			icon_shown = false;
		}
#endif
		setText("(0)");
		setToolTip(tr("no tags"));
	} else {
		setText(tr("no tags"));
	}
	connect(tagMenu, SIGNAL(newTagRequested()), this, SIGNAL(newTagRequested()));
	if(!b_small) connect(tagMenu, SIGNAL(aboutToShow()), this, SLOT(resizeTagMenu()));
	connect(tagMenu, SIGNAL(selectedTagsChanged()), this, SLOT(updateText()));
}
void TagButton::resizeTagMenu() {
	tagMenu->setMinimumWidth(width());
}
void TagButton::updateText() {
	QString str = tagMenu->selectedTagsText();
	if(str.isEmpty()) str = tr("no tags");
	if(b_small) {
		setText(QString("(") + QString::number(tagMenu->selectedTagsCount()) + ")");
		setToolTip(str);
	} else {		
		setText(str);
	}
}
void TagButton::keyPressEvent(QKeyEvent *e) {
	if(e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
		emit returnPressed();
		e->accept();
		return;
	}
	QPushButton::keyPressEvent(e);
}
void TagButton::updateTags() {
	tagMenu->updateTags();
	updateText();
}
void TagButton::setTagSelected(QString tag, bool b, bool inconsistent) {
	tagMenu->setTagSelected(tag, b, inconsistent);
	updateText();
}
void TagButton::setTransaction(Transactions *trans) {
	tagMenu->setTransaction(trans);
	updateText();
}
void TagButton::setTransactions(QList<Transactions*> list) {
	tagMenu->setTransactions(list);
	updateText();
}
void TagButton::modifyTransaction(Transactions *trans, bool append) {
	tagMenu->modifyTransaction(trans, append);
	updateText();
}
QString TagButton::createTag() {
	QString new_tag = tagMenu->createTag();
	updateText();
	click();
	return new_tag;
}


TagMenu::TagMenu(Budget *budg, QWidget *parent, bool allow_new_tag) : QMenu(parent), budget(budg), allow_new(allow_new_tag) {}
void TagMenu::updateTags() {
	QHash<QString, bool> tagb;
	QHash<QString, bool> tagi;
	for(QHash<QString, QAction*>::const_iterator it = tag_actions.constBegin(); it != tag_actions.constEnd(); ++it) {
		tagb[it.key()] = it.value()->isChecked();
		tagi[it.key()] = it.value()->data().toBool();
	}
	clear();
	tag_actions.clear();
	if(allow_new) {
		addAction(tr("New tag…"), this, SIGNAL(newTagRequested()));
		if(!budget->tags.isEmpty()) addSeparator();
	}
	for(int i = 0; i < budget->tags.count(); i++) {
		QAction *action = addAction(budget->tags[i], this, SLOT(tagToggled()));
		action->setData(false);
		action->setCheckable(true);
		if(tagb.contains(budget->tags[i])) {
			action->setChecked(tagb[budget->tags[i]]);
			if(tagi[budget->tags[i]]) {
				action->setData(true);
				action->setText(budget->tags[i] + "*");
			}
		}
		tag_actions[budget->tags[i]] = action;
	}
}
void TagMenu::setTagSelected(QString tag, bool b, bool inconsistent) {
	QHash<QString, QAction*>::iterator it = tag_actions.find(tag);
	if(it != tag_actions.end()) {
		if(it.value()->data().toBool() != inconsistent) {
			if(inconsistent) it.value()->setText(it.key() + "*");
			else it.value()->setText(it.key());
		}
		it.value()->setChecked(b);
		it.value()->setData(inconsistent);
	}
}
void TagMenu::setTransaction(Transactions *trans) {
	for(QHash<QString, QAction*>::const_iterator it = tag_actions.constBegin(); it != tag_actions.constEnd(); ++it) {
		if(it.value()->data().toBool()) it.value()->setText(it.key());
	}
	for(QHash<QString, QAction*>::const_iterator it = tag_actions.constBegin(); it != tag_actions.constEnd(); ++it) {
		it.value()->setData(false);
		if(trans && trans->hasTag(it.key(), true)) it.value()->setChecked(true);
		else it.value()->setChecked(false);
	}
}
void TagMenu::setTransactions(QList<Transactions*> list) {
	if(list.count() == 0) {
		setTransaction(NULL);
	} else if(list.count() == 1) {
		setTransaction(list[0]);
	} else {
		QHash<QString, bool> tagb;
		QHash<QString, bool> tagi;
		Transactions *trans = list[0];
		for(QHash<QString, QAction*>::const_iterator it = tag_actions.constBegin(); it != tag_actions.constEnd(); ++it) {
			tagb[it.key()] = trans->hasTag(it.key(), true);
			tagi[it.key()] = false;
		}
		for(int i = 1; i < list.count(); i++) {
			trans = list[i];
			for(QHash<QString, QAction*>::const_iterator it = tag_actions.constBegin(); it != tag_actions.constEnd(); ++it) {
				if(tagb[it.key()] != trans->hasTag(it.key(), true)) {
					tagb[it.key()] = false;
					tagi[it.key()] = true;
				}
			}
		}
		for(QHash<QString, QAction*>::const_iterator it = tag_actions.constBegin(); it != tag_actions.constEnd(); ++it) {
			if(it.value()->data() != tagi[it.key()]) {
				if(tagi[it.key()]) it.value()->setText(it.key() + "*");
				else it.value()->setText(it.key());
				it.value()->setData(tagi[it.key()]);
			}
			it.value()->setChecked(tagb[it.key()]);
		}
	}
}
void TagMenu::modifyTransaction(Transactions *trans, bool append) {
	QStringList tags;
	for(QHash<QString, QAction*>::const_iterator it = tag_actions.constBegin(); it != tag_actions.constEnd(); ++it) {
		if(it.value()->data().toBool()) {
			if(trans->hasTag(it.key(), true)) tags << it.key();
		} else if(it.value()->isChecked()) {
			tags << it.key();
		}
	}
	if(trans->generaltype() == GENERAL_TRANSACTION_TYPE_SINGLE && ((Transaction*) trans)->parentSplit() && ((Transaction*) trans)->parentSplit()->count() > 1) ((Transaction*) trans)->parentSplit()->splitTags();
	if(!append) trans->clearTags();
	for(int i = 0; i < tags.count(); i++) {
		trans->addTag(tags[i]);
	}
	if(trans->generaltype() == GENERAL_TRANSACTION_TYPE_SINGLE && ((Transaction*) trans)->parentSplit() && ((Transaction*) trans)->parentSplit()->count() > 1) ((Transaction*) trans)->parentSplit()->joinTags();
}
int TagMenu::selectedTagsCount() {
	int n = 0;
	for(QHash<QString, QAction*>::const_iterator it = tag_actions.constBegin(); it != tag_actions.constEnd(); ++it) {
		if(it.value()->isChecked()) n++;
	}
	return n;
}
QString TagMenu::selectedTagsText() {
	QString str;
	for(QHash<QString, QAction*>::const_iterator it = tag_actions.constBegin(); it != tag_actions.constEnd(); ++it) {
		if(it.value()->isChecked()) {
			if(!str.isEmpty()) str += ", ";
			str += it.key();
		}
	}
	return str;
}
void TagMenu::tagToggled() {
	QAction *action = qobject_cast<QAction*>(sender());
	if(action->data().toBool()) {
		action->setData(false);
		for(QHash<QString, QAction*>::const_iterator it = tag_actions.constBegin(); it != tag_actions.constEnd(); ++it) {
			if(it.value() == action) {
				action->setText(it.key());
				break;
			}
		}
	}
	emit selectedTagsChanged();
}
void TagMenu::keyPressEvent(QKeyEvent *e) {
	if(e->key() == Qt::Key_Space) {
		QAction *action = activeAction();
		if(action) action->trigger();
		e->setAccepted(true);
	} else {
		QMenu::keyPressEvent(e);
	}
}
void TagMenu::mouseReleaseEvent(QMouseEvent *e) {
	if(e->button() == Qt::LeftButton) {
		QAction *action = actionAt(e->pos());
		if(action) action->trigger();
		e->setAccepted(true);
	} else {
		QMenu::mouseReleaseEvent(e);
	}
}
QString TagMenu::createTag() {
	QString new_tag = QInputDialog::getText(this, tr("New Tag"), tr("Tag:")).trimmed(); 
	if(!new_tag.isEmpty()) {
		if((new_tag.contains(",") && new_tag.contains("\"") && new_tag.contains("\'")) || (new_tag[0] == '\'' && new_tag.contains("\"")) || (new_tag[0] == '\"' && new_tag.contains("\'"))) {
			if(new_tag[0] == '\'') new_tag.remove("\'");
			else new_tag.remove("\"");
		}
		QString str = budget->findTag(new_tag);
		if(str.isEmpty()) {
			budget->tagAdded(new_tag);
			updateTags();
			setTagSelected(new_tag, true);
			return new_tag;
		} else {
			setTagSelected(str, true);
		}
	}
	return QString();
}

extern QString last_associated_file_directory;

TransactionEditWidget::TransactionEditWidget(bool auto_edit, bool extra_parameters, int transaction_type, Currency *split_currency, bool transfer_to, Security *sec, SecurityValueDefineType security_value_type, bool select_security, Budget *budg, QWidget *parent, bool allow_account_creation, bool multiaccount, bool withloan) : QWidget(parent), transtype(transaction_type), budget(budg), security(sec), b_autoedit(auto_edit), b_extra(extra_parameters), b_create_accounts(allow_account_creation), b_select_security(select_security && !security) {
	bool split = (split_currency != NULL);
	splitcurrency = split_currency;
	value_set = false; shares_set = false; sharevalue_set = false;
	b_sec = (transtype == TRANSACTION_TYPE_SECURITY_BUY || transtype == TRANSACTION_TYPE_SECURITY_SELL || transtype == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND);
	QVBoxLayout *editVLayout = new QVBoxLayout(this);
	int cols = 1;
	if(auto_edit) cols = 2;
	int rows = 6;
	if(b_sec && security_value_type == SECURITY_ALL_VALUES) rows += 1;
	else if(b_extra && !security && !select_security && transtype == TRANSACTION_TYPE_EXPENSE) rows += (multiaccount ? 1 : 2);
	else if(b_extra && !security && !select_security && transtype == TRANSACTION_TYPE_INCOME && !multiaccount) rows ++;
	else if(transtype == TRANSACTION_TYPE_TRANSFER) rows++;
	if(b_sec && security_value_type != SECURITY_VALUE_AND_SHARES) rows++;
	if(withloan) rows += 2;
	if(multiaccount) rows -= 4;
	if(split && !b_sec) rows -= 2;
	if(rows % cols > 0) rows = rows / cols + 1;
	else rows = rows / cols;
	editLayout = new QGridLayout();
	editVLayout->addLayout(editLayout);
	editVLayout->addStretch(1);
	valueEdit = NULL;
	depositEdit = NULL;
	downPaymentEdit = NULL;
	dateEdit = NULL;
	sharesEdit = NULL;
	quotationEdit = NULL;
	setQuoteButton = NULL;
	descriptionEdit = NULL;
	payeeEdit = NULL;
	lenderEdit = NULL;
	quantityEdit = NULL;
	toCombo = NULL;
	fromCombo = NULL;
	securityCombo = NULL;
	currencyCombo = NULL;
	commentsEdit = NULL;
	dateLabel = NULL;
	depositLabel = NULL;
	withdrawalLabel = NULL;
	fileEdit = NULL;
	tagButton = NULL;
	int i = 0;
	if(b_sec) {
		int decimals = budget->defaultShareDecimals();
		editLayout->addWidget(new QLabel(tr("Security:", "Financial security (e.g. stock, mutual fund)"), this), TEROWCOL(i, 0));
		if(select_security) {
			securityCombo = new EqonomizeComboBox(this);
			securityCombo->setEditable(false);
			if(b_create_accounts) securityCombo->addItem(tr("New Security…", "Financial security (e.g. stock, mutual fund)"), qVariantFromValue((void*) NULL));
			int i2 = (b_create_accounts ? 1 : 0);
			for(SecurityList<Security*>::const_iterator it = budget->securities.constBegin(); it != budget->securities.constEnd(); ++it) {
				Security *c_sec = *it;
				securityCombo->addItem(c_sec->name(), qVariantFromValue((void*) c_sec));
				if(c_sec == security || it == budget->securities.constBegin()) securityCombo->setCurrentIndex(i2);
				i2++;
			}
			if(b_create_accounts && i2 == 1) securityCombo->setCurrentIndex(-1);
			else if(b_create_accounts) securityCombo->insertSeparator(1);
			editLayout->addWidget(securityCombo, TEROWCOL(i, 1));
			if(!security) security = selectedSecurity();
		} else {
			editLayout->addWidget(new QLabel(security->name(), this), TEROWCOL(i, 1));
		}
		if(security) decimals = security->decimals();
		i++;
		if(security_value_type != SECURITY_SHARES_AND_QUOTATION && transtype != TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND) {
			if(transtype == TRANSACTION_TYPE_SECURITY_BUY) editLayout->addWidget(new QLabel(tr("Cost:"), this), TEROWCOL(i, 0));
			else editLayout->addWidget(new QLabel(tr("Income:"), this), TEROWCOL(i, 0));
			valueEdit = new EqonomizeValueEdit(false, this, budget);
			editLayout->addWidget(valueEdit, TEROWCOL(i, 1));
			i++;
		}
		if(security_value_type != SECURITY_VALUE_AND_QUOTATION) {
			if(transtype == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND) {
				editLayout->addWidget(new QLabel(tr("Shares added:", "Financial shares"), this), TEROWCOL(i, 0));
				sharesEdit = new EqonomizeValueEdit(0.0, decimals, false, false, this, budget);
				editLayout->addWidget(sharesEdit, TEROWCOL(i, 1));
				i++;
			} else if(transtype == TRANSACTION_TYPE_SECURITY_BUY) {
				editLayout->addWidget(new QLabel(tr("Shares bought:", "Financial shares"), this), TEROWCOL(i, 0));
				sharesEdit = new EqonomizeValueEdit(0.0, decimals, false, false, this, budget);
				editLayout->addWidget(sharesEdit, TEROWCOL(i, 1));
				i++;
			} else {
				editLayout->addWidget(new QLabel(tr("Shares sold:", "Financial shares"), this), TEROWCOL(i, 0));
				QHBoxLayout *sharesLayout = new QHBoxLayout();
				sharesEdit = new EqonomizeValueEdit(0.0, decimals, false, false, this, budget);
				sharesEdit->setSizePolicy(QSizePolicy::Expanding, sharesEdit->sizePolicy().verticalPolicy());
				sharesLayout->addWidget(sharesEdit);
				maxSharesButton = new QPushButton(tr("All"), this);
				maxSharesButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
				sharesLayout->addWidget(maxSharesButton);
				editLayout->addLayout(sharesLayout, TEROWCOL(i, 1), Qt::AlignRight);
				i++;
			}
		}
		if(security_value_type != SECURITY_VALUE_AND_SHARES) {
			editLayout->addWidget(new QLabel(tr("Price per share:", "Financial shares"), this), TEROWCOL(i, 0));
			quotationEdit = new EqonomizeValueEdit(0.0, security ? security->quotationDecimals() : budget->defaultQuotationDecimals(), false, true, this, budget);
			editLayout->addWidget(quotationEdit, TEROWCOL(i, 1));
			i++;
			setQuoteButton = new QCheckBox(tr("Set security share value"), this);
			QSettings settings;
			settings.beginGroup("GeneralOptions");
			setQuoteButton->setChecked(settings.value("setShareValueFromPrice", true).toBool());
			settings.endGroup();
			editLayout->addWidget(setQuoteButton, TEROWCOL(i, 1), Qt::AlignRight);
			i++;
		}
		if(security_value_type != SECURITY_SHARES_AND_QUOTATION && transtype == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND) {
			editLayout->addWidget(new QLabel(tr("Total value:"), this), TEROWCOL(i, 0));
			valueEdit = new EqonomizeValueEdit(false, this, budget);
			editLayout->addWidget(valueEdit, TEROWCOL(i, 1));
			i++;
		}
		if(!split) {
			dateLabel = new QLabel(tr("Date:"), this);
			editLayout->addWidget(dateLabel, TEROWCOL(i, 0));
			dateRow = CURROW(i, 0);
			dateLabelCol = CURCOL(i, 0);
			dateEdit = new EqonomizeDateEdit(this);
			dateEdit->setCalendarPopup(true);
			editLayout->addWidget(dateEdit, TEROWCOL(i, 1));
			dateEditCol = CURCOL(i, 1);
		}
		i++;
	} else {
		if(transtype == TRANSACTION_TYPE_INCOME && (security || select_security)) {
			editLayout->addWidget(new QLabel(tr("Security:", "Financial security (e.g. stock, mutual fund)"), this), TEROWCOL(i, 0));
			if(select_security) {
				securityCombo = new EqonomizeComboBox(this);
				securityCombo->setEditable(false);
				if(b_create_accounts) securityCombo->addItem(tr("New Security…", "Financial security (e.g. stock, mutual fund)"), qVariantFromValue((void*) NULL));
				int i2 = (b_create_accounts ? 1 : 0);
				for(SecurityList<Security*>::const_iterator it = budget->securities.constBegin(); it != budget->securities.constEnd(); ++it) {
					Security *c_sec = *it;
					securityCombo->addItem(c_sec->name(), qVariantFromValue((void*) c_sec));
					if(c_sec == security || it == budget->securities.constBegin()) securityCombo->setCurrentIndex(i2);
					i2++;
				}
				if(b_create_accounts && i2 == 1) securityCombo->setCurrentIndex(-1);
				else if(b_create_accounts) securityCombo->insertSeparator(1);
				editLayout->addWidget(securityCombo, TEROWCOL(i, 1));
				if(!security) security = selectedSecurity();
			} else {
				editLayout->addWidget(new QLabel(security->name(), this), TEROWCOL(i, 1));
			}
			i++;
		} else if(!multiaccount) {
			editLayout->addWidget(new QLabel(tr("Description:", "Transaction description property (transaction title/generic article name)"), this), TEROWCOL(i, 0));			
			descriptionEdit = new QLineEdit(this);
			descriptionEdit->setCompleter(new QCompleter(this));
			descriptionEdit->completer()->setModel(new QStandardItemModel(this));
			descriptionEdit->completer()->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
			descriptionEdit->setToolTip(tr("Transaction title/generic article name"));
			editLayout->addWidget(descriptionEdit, TEROWCOL(i, 1));
			i++;
		}		
		if(transtype == TRANSACTION_TYPE_TRANSFER) {
			withdrawalLabel = new QLabel(tr("Withdrawal:", "Money taken out from account"), this);
			editLayout->addWidget(withdrawalLabel, TEROWCOL(i, 0));
		} else if(transtype == TRANSACTION_TYPE_INCOME) {
			editLayout->addWidget(new QLabel(tr("Income:"), this), TEROWCOL(i, 0));
		} else {
			editLayout->addWidget(new QLabel(tr("Cost:"), this), TEROWCOL(i, 0));
		}
		valueEdit = new EqonomizeValueEdit(0.0, budget->defaultCurrency()->fractionalDigits(true), !withloan, !withloan, this, budget);
		if(withloan) {
			valueEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
			QHBoxLayout *valueLayout = new QHBoxLayout();
			valueLayout->addWidget(valueEdit);
			currencyCombo = new QComboBox(this);
			currencyCombo->setEditable(false);
			int i2 = 0;
			for(CurrencyList<Currency*>::const_iterator it = budget->currencies.constBegin(); it != budget->currencies.constEnd(); ++it) {
				Currency *currency = *it;
				currencyCombo->addItem(QIcon(":/data/flags/" + currency->code() + ".png"), currency->code());
				currencyCombo->setItemData(i2, qVariantFromValue((void*) currency));
				if(currency == budget->defaultCurrency()) currencyCombo->setCurrentIndex(i2);
				i2++;
			}
			valueLayout->addWidget(currencyCombo);
			editLayout->addLayout(valueLayout, TEROWCOL(i, 1));
		} else {
			editLayout->addWidget(valueEdit, TEROWCOL(i, 1));
		}
		i++;
		if(transtype == TRANSACTION_TYPE_TRANSFER) {
			depositLabel = new QLabel(tr("Deposit:", "Money put into account"), this);
			editLayout->addWidget(depositLabel, TEROWCOL(i, 0));
			depositRow = CURROW(i, 0);
			depositLabelCol = CURCOL(i, 0);
			depositEdit = new EqonomizeValueEdit(true, this, budget);
			editLayout->addWidget(depositEdit, TEROWCOL(i, 1));
			depositEditCol = CURCOL(i, 1);
			i++;
		}
		if(withloan) {
			editLayout->addWidget(new QLabel(tr("Downpayment:"), this), TEROWCOL(i, 0));
			downPaymentEdit = new EqonomizeValueEdit(false, this, budget);
			editLayout->addWidget(downPaymentEdit, TEROWCOL(i, 1));
			i++;
		}
		if(b_extra && !multiaccount && !select_security && !security && transtype == TRANSACTION_TYPE_EXPENSE) {
			editLayout->addWidget(new QLabel(tr("Quantity:"), this), TEROWCOL(i, 0));
			quantityEdit = new EqonomizeValueEdit(1.0, QUANTITY_DECIMAL_PLACES, true, false, this, budget);
			quantityEdit->setToolTip(tr("Number of items included in the transaction. Entered cost is total cost for all items."));
			editLayout->addWidget(quantityEdit, TEROWCOL(i, 1));
			i++;
		}
		if(!split) {
			dateLabel = new QLabel(tr("Date:"), this);
			editLayout->addWidget(dateLabel, TEROWCOL(i, 0));
			dateRow = CURROW(i, 0);
			dateLabelCol = CURCOL(i, 0);
			dateEdit = new EqonomizeDateEdit(this);
			dateEdit->setCalendarPopup(true);
			editLayout->addWidget(dateEdit, TEROWCOL(i, 1));
			dateEditCol = CURCOL(i, 1);
			i++;
			if(b_extra && cols == 2 && !multiaccount && !select_security && !security && transtype == TRANSACTION_TYPE_INCOME) {
				i++;
			}
		}
		
	}
	switch(transtype) {
		case TRANSACTION_TYPE_TRANSFER: {
			if(!split || transfer_to) {
				editLayout->addWidget(new QLabel(tr("From:"), this), TEROWCOL(i, 0));
				fromCombo = new AccountComboBox(ACCOUNT_TYPE_ASSETS, budget, b_create_accounts, false, false, !b_autoedit, true, this);
				editLayout->addWidget(fromCombo, TEROWCOL(i, 1));
				i++;
			}
			if(!split || !transfer_to) {
				editLayout->addWidget(new QLabel(tr("To:"), this), TEROWCOL(i, 0));
				toCombo = new AccountComboBox(ACCOUNT_TYPE_ASSETS, budget, b_create_accounts, false, false, !b_autoedit, false, this);
				editLayout->addWidget(toCombo, TEROWCOL(i, 1));
				i++;
			}
			break;
		}
		case TRANSACTION_TYPE_INCOME: {
			if(!multiaccount) {
				editLayout->addWidget(new QLabel(tr("Category:"), this), TEROWCOL(i, 0));
				fromCombo = new AccountComboBox(ACCOUNT_TYPE_INCOMES, budget, b_create_accounts, false, false, false, false, this);
				editLayout->addWidget(fromCombo, TEROWCOL(i, 1));
				i++;
			}
			if(!split) {
				editLayout->addWidget(new QLabel(tr("To account:"), this), TEROWCOL(i, 0));
				toCombo = new AccountComboBox(ACCOUNT_TYPE_ASSETS, budget, b_create_accounts, b_create_accounts && b_autoedit, b_autoedit, !b_autoedit, true, this);
				editLayout->addWidget(toCombo, TEROWCOL(i, 1));
				i++;
			}
			if(b_extra && !select_security && !security) {
				editLayout->addWidget(new QLabel(tr("Payer:"), this), TEROWCOL(i, 0));
				payeeEdit = new QLineEdit(this);
				if(split) payeeEdit->setPlaceholderText(tr("Payer of parent split transaction"));
				editLayout->addWidget(payeeEdit, TEROWCOL(i, 1));
				i++;
			}
			break;
		}
		case TRANSACTION_TYPE_SECURITY_BUY: {
			if(!split) {
				editLayout->addWidget(new QLabel(tr("From account:"), this), TEROWCOL(i, 0));
				fromCombo = new AccountComboBox(-1, budget, b_create_accounts, false, false, true, true, this);
				editLayout->addWidget(fromCombo, TEROWCOL(i, 1));
				i++;
			}
			break;
		}
		case TRANSACTION_TYPE_SECURITY_SELL: {
			if(!split) {
				editLayout->addWidget(new QLabel(tr("To account:"), this), TEROWCOL(i, 0));
				toCombo = new AccountComboBox(-2, budget, b_create_accounts, false, false, true, true, this);
				editLayout->addWidget(toCombo, TEROWCOL(i, 1));
				i++;
			}
			break;
		}
		case TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND: {
			editLayout->addWidget(new QLabel(tr("Category:"), this), TEROWCOL(i, 0));
			fromCombo = new AccountComboBox(ACCOUNT_TYPE_INCOMES, budget, b_create_accounts, false, false, false, false, this);
			editLayout->addWidget(fromCombo, TEROWCOL(i, 1));
			i++;
			break;
		}
		default: {
			if(!multiaccount) {
				editLayout->addWidget(new QLabel(tr("Category:"), this), TEROWCOL(i, 0));
				toCombo = new AccountComboBox(ACCOUNT_TYPE_EXPENSES, budget, b_create_accounts, false, false, false, false, this);
				editLayout->addWidget(toCombo, TEROWCOL(i, 1));
				i++;
			}
			if(!split) {
				if(withloan) {
					editLayout->addWidget(new QLabel(tr("Downpayment account:"), this), TEROWCOL(i, 0));
					fromCombo = new AccountComboBox(ACCOUNT_TYPE_ASSETS, budget, b_create_accounts, false, false, true, true, this);
				} else {
					editLayout->addWidget(new QLabel(tr("From account:"), this), TEROWCOL(i, 0));
					fromCombo = new AccountComboBox(ACCOUNT_TYPE_ASSETS, budget, b_create_accounts, b_create_accounts && b_autoedit, b_autoedit, true, true, this);
				}
				editLayout->addWidget(fromCombo, TEROWCOL(i, 1));
				i++;
			}
			if(b_extra) {
				editLayout->addWidget(new QLabel(tr("Payee:"), this), TEROWCOL(i, 0));
				payeeEdit = new QLineEdit(this);
				if(split) payeeEdit->setPlaceholderText(tr("Payee of parent split transaction"));
				editLayout->addWidget(payeeEdit, TEROWCOL(i, 1));
				i++;
			}
		}
	}
	if(withloan) {
		editLayout->addWidget(new QLabel(tr("Lender:"), this), TEROWCOL(i, 0));
		lenderEdit = new QLineEdit(this);
		editLayout->addWidget(lenderEdit, TEROWCOL(i, 1));
		i++;
	}
	if(!b_autoedit && (transtype == TRANSACTION_TYPE_INCOME || transtype == TRANSACTION_TYPE_EXPENSE) && !multiaccount && !sec) {
		editLayout->addWidget(new QLabel(tr("Tags:"), this), TEROWCOL(i, 0));
		tagButton = new TagButton(false, allow_account_creation, budget, this);
		editLayout->addWidget(tagButton, TEROWCOL(i, 1));
		tagsModified();
		i++;
		connect(tagButton, SIGNAL(newTagRequested()), this, SLOT(newTag()));
	}
	if(!b_autoedit && !split && !multiaccount) {
		editLayout->addWidget(new QLabel(tr("Associated file:"), this), TEROWCOL(i, 0));
		QHBoxLayout *fileLayout = new QHBoxLayout();
		fileEdit = new QLineEdit(this);
		QCompleter *completer = new QCompleter(this);
		completer->setModel(new QDirModel(completer));
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
		editLayout->addLayout(fileLayout, TEROWCOL(i, 1));
		i++;
		connect(selectFileButton, SIGNAL(clicked()), this, SLOT(selectFile()));
		connect(openFileButton, SIGNAL(clicked()), this, SLOT(openFile()));
	}
	if(!multiaccount) {
		editLayout->addWidget(new QLabel(tr("Comments:"), this), TEROWCOL(i, 0));
		commentsEdit = new QLineEdit(this);
		if(b_autoedit && (transtype == TRANSACTION_TYPE_INCOME || transtype == TRANSACTION_TYPE_EXPENSE) && !sec) {
			QHBoxLayout *box = new QHBoxLayout();
			editLayout->addLayout(box, TEROWCOL(i, 1));
			box->addWidget(commentsEdit, 1);
			tagButton = new TagButton(true, allow_account_creation, budget, this);
			if(!tagButton->icon_shown) {
				QLabel *tagIcon = new QLabel(this);
				tagIcon->setPixmap(LOAD_ICON2("tag", "eqz-tag").pixmap(tagButton->style()->pixelMetric(QStyle::PM_ButtonIconSize)));
				box->addWidget(tagIcon, 0);
			}
			box->addWidget(tagButton, 0);
			tagsModified();
			connect(tagButton, SIGNAL(newTagRequested()), this, SLOT(newTag()));
		} else {
			editLayout->addWidget(commentsEdit, TEROWCOL(i, 1));
		}
		i++;
	}
	bottom_layout = new QHBoxLayout();
	editVLayout->addLayout(bottom_layout);
	editVLayout->addStretch(1);

	description_changed = false;
	payee_changed = false;
	if(descriptionEdit) {
		descriptionEdit->completer()->setCaseSensitivity(Qt::CaseInsensitive);
	}
	if(payeeEdit) {
		payeeEdit->setCompleter(new QCompleter(this));
		payeeEdit->completer()->setModel(new QStandardItemModel(this));
		payeeEdit->completer()->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
		payeeEdit->completer()->setCaseSensitivity(Qt::CaseInsensitive);
	}
	if(splitcurrency) {
		if(valueEdit && !transfer_to) valueEdit->setCurrency(splitcurrency);
		if(quotationEdit && !transfer_to) quotationEdit->setCurrency(splitcurrency);
		if(depositEdit && transfer_to) depositEdit->setCurrency(splitcurrency);
	}
	if(dateEdit) connect(dateEdit, SIGNAL(dateChanged(const QDate&)), this, SIGNAL(dateChanged(const QDate&)));
	if(valueEdit) {
		connect(valueEdit, SIGNAL(valueChanged(double)), this, SLOT(valueChanged(double)));
		if(depositEdit) {
			connect(valueEdit, SIGNAL(editingFinished()), this, SLOT(valueEditingFinished()));
		}
	}
	if(b_sec) {
		switch(security_value_type) {
			case SECURITY_ALL_VALUES: {
				connect(sharesEdit, SIGNAL(returnPressed()), quotationEdit, SLOT(enterFocus()));
				if(transtype == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND) {
					connect(quotationEdit, SIGNAL(returnPressed()), valueEdit, SLOT(enterFocus()));
					if(dateEdit) connect(valueEdit, SIGNAL(returnPressed()), this, SLOT(focusDate()));
				} else if(dateEdit) {
					connect(quotationEdit, SIGNAL(returnPressed()), this, SLOT(focusDate()));
				}
				if(transtype == TRANSACTION_TYPE_SECURITY_SELL) connect(maxSharesButton, SIGNAL(clicked()), this, SLOT(maxShares()));
				break;
			}
			case SECURITY_SHARES_AND_QUOTATION: {
				connect(sharesEdit, SIGNAL(returnPressed()), quotationEdit, SLOT(enterFocus()));
				if(dateEdit) {
					connect(quotationEdit, SIGNAL(returnPressed()), this, SLOT(focusDate()));
				}
				if(transtype == TRANSACTION_TYPE_SECURITY_SELL) connect(maxSharesButton, SIGNAL(clicked()), this, SLOT(maxShares()));
				break;
			}
			case SECURITY_VALUE_AND_SHARES: {
				if(transtype == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND) {
					connect(sharesEdit, SIGNAL(returnPressed()), valueEdit, SLOT(enterFocus()));
					if(dateEdit) connect(valueEdit, SIGNAL(returnPressed()), this, SLOT(focusDate()));
				} else if(dateEdit) {
					connect(sharesEdit, SIGNAL(returnPressed()), this, SLOT(focusDate()));
				}
				if(transtype == TRANSACTION_TYPE_SECURITY_SELL) connect(maxSharesButton, SIGNAL(clicked()), this, SLOT(maxShares()));
				break;
			}
			case SECURITY_VALUE_AND_QUOTATION: {
				if(dateEdit) {
					connect(quotationEdit, SIGNAL(returnPressed()), this, SLOT(focusDate()));
				}
				break;
			}
		}		
		if(sharesEdit) connect(sharesEdit, SIGNAL(valueChanged(double)), this, SLOT(sharesChanged(double)));
		if(quotationEdit) connect(quotationEdit, SIGNAL(valueChanged(double)), this, SLOT(quotationChanged(double)));
	} else {
		if(downPaymentEdit) {
			if(quantityEdit) {
				connect(downPaymentEdit, SIGNAL(returnPressed()), quantityEdit, SLOT(enterFocus()));
				if(dateEdit) {
					connect(quantityEdit, SIGNAL(returnPressed()), this, SLOT(focusDate()));
				}
			} else {
				if(dateEdit) {
					connect(downPaymentEdit, SIGNAL(returnPressed()), this, SLOT(focusDate()));
				}
			}
		} else if(quantityEdit && dateEdit) {
			connect(quantityEdit, SIGNAL(returnPressed()), this, SLOT(focusDate()));
		} else if(depositEdit && dateEdit) {
			connect(depositEdit, SIGNAL(returnPressed()), this, SLOT(focusDate()));
		} else if(quantityEdit && fromCombo) {
			connect(quantityEdit, SIGNAL(returnPressed()), fromCombo, SLOT(focusAndSelectAll()));
		} else if(quantityEdit && toCombo) {
			connect(quantityEdit, SIGNAL(returnPressed()), toCombo, SLOT(focusAndSelectAll()));
		}
		if(descriptionEdit) {
			connect(descriptionEdit, SIGNAL(returnPressed()), valueEdit, SLOT(enterFocus()));
			connect(descriptionEdit, SIGNAL(editingFinished()), this, SLOT(setDefaultValue()));
			connect(descriptionEdit, SIGNAL(textChanged(const QString&)), this, SLOT(descriptionChanged(const QString&)));
		}
	}
	if(valueEdit && transtype != TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND) connect(valueEdit, SIGNAL(returnPressed()), this, SLOT(valueNextField()));
	if(payeeEdit) {
		connect(payeeEdit, SIGNAL(editingFinished()), this, SLOT(setDefaultValueFromPayee()));
		connect(payeeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(payeeChanged(const QString&)));
	}
	if(dateEdit) {
		if(b_autoedit) connect(dateEdit, SIGNAL(returnPressed()), this, SIGNAL(addmodify()));
		else if(fromCombo && transtype != TRANSACTION_TYPE_EXPENSE) connect(dateEdit, SIGNAL(returnPressed()), fromCombo, SLOT(focusAndSelectAll()));
		else if(toCombo) connect(dateEdit, SIGNAL(returnPressed()), toCombo, SLOT(focusAndSelectAll()));
	}
	if(payeeEdit && lenderEdit) connect(payeeEdit, SIGNAL(returnPressed()), lenderEdit, SLOT(setFocus()));
	else if(payeeEdit && tagButton && (!b_autoedit || !commentsEdit)) connect(payeeEdit, SIGNAL(returnPressed()), tagButton, SLOT(setFocus()));
	else if(payeeEdit && fileEdit) connect(payeeEdit, SIGNAL(returnPressed()), fileEdit, SLOT(setFocus()));
	else if(payeeEdit && commentsEdit) connect(payeeEdit, SIGNAL(returnPressed()), commentsEdit, SLOT(setFocus()));
	if(lenderEdit && tagButton) connect(lenderEdit, SIGNAL(returnPressed()), tagButton, SLOT(setFocus()));
	else if(lenderEdit && fileEdit) connect(lenderEdit, SIGNAL(returnPressed()), fileEdit, SLOT(setFocus()));
	else if(lenderEdit && commentsEdit) connect(lenderEdit, SIGNAL(returnPressed()), commentsEdit, SLOT(setFocus()));
	if(fileEdit && commentsEdit) connect(fileEdit, SIGNAL(returnPressed()), commentsEdit, SLOT(setFocus()));
	if(commentsEdit) {
		if(b_autoedit && tagButton) connect(commentsEdit, SIGNAL(returnPressed()), tagButton, SLOT(setFocus()));
		else connect(commentsEdit, SIGNAL(returnPressed()), this, SIGNAL(addmodify()));
	}
	if(tagButton) {
		if(b_autoedit) connect(tagButton, SIGNAL(returnPressed()), this, SIGNAL(addmodify()));
		else if(fileEdit) connect(tagButton, SIGNAL(returnPressed()), fileEdit, SLOT(setFocus()));
		else if(commentsEdit) connect(tagButton, SIGNAL(returnPressed()), commentsEdit, SLOT(setFocus()));
	}
	if(securityCombo) connect(securityCombo, SIGNAL(activated(int)), this, SLOT(securityChanged(int)));
	if(transtype == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND && securityCombo && sharesEdit) {
		connect(securityCombo, SIGNAL(returnPressed()), sharesEdit, SLOT(enterFocus()));
		connect(securityCombo, SIGNAL(itemSelected(int)), sharesEdit, SLOT(enterFocus()));
	} else if(securityCombo && valueEdit) {
		connect(securityCombo, SIGNAL(returnPressed()), valueEdit, SLOT(enterFocus()));
		connect(securityCombo, SIGNAL(itemSelected(int)), valueEdit, SLOT(enterFocus()));
	}
	if(currencyCombo) connect(currencyCombo, SIGNAL(activated(int)), this, SLOT(currencyChanged(int)));
	if(setQuoteButton) connect(setQuoteButton, SIGNAL(toggled(bool)), this, SLOT(setQuoteToggled(bool)));
	if(fromCombo) {
		connect(fromCombo, SIGNAL(newAccountRequested()), this, SLOT(newFromAccount()));
		connect(fromCombo, SIGNAL(newLoanRequested()), this, SIGNAL(newLoanRequested()));
		connect(fromCombo, SIGNAL(multipleAccountsRequested()), this, SIGNAL(multipleAccountsRequested()));
		connect(fromCombo, SIGNAL(accountSelected(Account*)), this, SLOT(fromActivated()));
		connect(fromCombo, SIGNAL(currentAccountChanged(Account*)), this, SLOT(fromChanged(Account*)));
		if(toCombo && transtype != TRANSACTION_TYPE_EXPENSE) connect(fromCombo, SIGNAL(returnPressed()), toCombo, SLOT(focusAndSelectAll()));
		else if(payeeEdit) connect(fromCombo, SIGNAL(returnPressed()), payeeEdit, SLOT(setFocus()));
		else if(tagButton) connect(fromCombo, SIGNAL(returnPressed()), tagButton, SLOT(setFocus()));
		else if(fileEdit) connect(fromCombo, SIGNAL(returnPressed()), fileEdit, SLOT(setFocus()));
		else if(commentsEdit) connect(fromCombo, SIGNAL(returnPressed()), commentsEdit, SLOT(setFocus()));
	}
	if(toCombo) {
		connect(toCombo, SIGNAL(newAccountRequested()), this, SLOT(newToAccount()));
		connect(toCombo, SIGNAL(newLoanRequested()), this, SIGNAL(newLoanRequested()));
		connect(toCombo, SIGNAL(multipleAccountsRequested()), this, SIGNAL(multipleAccountsRequested()));
		connect(toCombo, SIGNAL(accountSelected(Account*)), this, SLOT(toActivated()));
		connect(toCombo, SIGNAL(currentAccountChanged(Account*)), this, SLOT(toChanged(Account*)));
		if(fromCombo && transtype == TRANSACTION_TYPE_EXPENSE) connect(toCombo, SIGNAL(returnPressed()), fromCombo, SLOT(focusAndSelectAll()));
		else if(payeeEdit) connect(toCombo, SIGNAL(returnPressed()), payeeEdit, SLOT(setFocus()));
		else if(tagButton) connect(toCombo, SIGNAL(returnPressed()), tagButton, SLOT(setFocus()));
		else if(fileEdit) connect(toCombo, SIGNAL(returnPressed()), fileEdit, SLOT(setFocus()));
		else if(commentsEdit) connect(toCombo, SIGNAL(returnPressed()), commentsEdit, SLOT(setFocus()));
	}
	if(auto_edit) {
		if(valueEdit) connect(valueEdit, SIGNAL(valueChanged(double)), this, SIGNAL(propertyChanged()));
		if(depositEdit) connect(depositEdit, SIGNAL(valueChanged(double)), this, SIGNAL(propertyChanged()));
		if(quantityEdit) connect(quantityEdit, SIGNAL(valueChanged(double)), this, SIGNAL(propertyChanged()));
		if(dateEdit) connect(dateEdit, SIGNAL(dateChanged(const QDate&)), this, SIGNAL(propertyChanged()));
		if(commentsEdit) connect(commentsEdit, SIGNAL(textChanged(const QString&)), this, SIGNAL(propertyChanged()));
		if(descriptionEdit) connect(descriptionEdit, SIGNAL(textChanged(const QString&)), this, SIGNAL(propertyChanged()));
		if(payeeEdit) connect(payeeEdit, SIGNAL(textChanged(const QString&)), this, SIGNAL(propertyChanged()));
	}
	b_multiple_currencies = true;
	useMultipleCurrencies(budget->usesMultipleCurrencies());
	if(security) securityChanged();
}
void TransactionEditWidget::setQuoteToggled(bool b) {
	QSettings settings;
	settings.beginGroup("GeneralOptions");
	settings.setValue("setShareValueFromPrice", b);
	settings.endGroup();
}
void TransactionEditWidget::selectFile() {
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
void TransactionEditWidget::openFile() {
	open_file_list(fileEdit->text().trimmed());
}
void TransactionEditWidget::useMultipleCurrencies(bool b) {
	if(b == b_multiple_currencies) return;
	b_multiple_currencies = b;
	if(!depositEdit) return;
	if(b_autoedit && dateEdit) {
		editLayout->removeWidget(dateLabel);
		editLayout->removeWidget(depositLabel);
		editLayout->removeWidget(dateEdit);
		editLayout->removeWidget(depositEdit);
		if(b) {
			editLayout->addWidget(depositEdit, depositRow, depositEditCol);
			editLayout->addWidget(depositLabel, depositRow, depositLabelCol);
			editLayout->addWidget(dateEdit, dateRow, dateEditCol);
			editLayout->addWidget(dateLabel, dateRow, dateLabelCol);
		} else {
			editLayout->addWidget(dateEdit, depositRow, depositEditCol);
			editLayout->addWidget(dateLabel, depositRow, depositLabelCol);
			editLayout->addWidget(depositEdit, dateRow, dateEditCol);
			editLayout->addWidget(depositLabel, dateRow, dateLabelCol);
		}
	}
	depositEdit->setVisible(b);
	depositLabel->setVisible(b);
	if(b) {
		if(withdrawalLabel) withdrawalLabel->setText(tr("Withdrawal:", "Money taken out from account"));
	} else {
		if(withdrawalLabel) withdrawalLabel->setText(tr("Amount:"));
	}
}
void TransactionEditWidget::valueNextField() {
	if(depositEdit && depositEdit->isEnabled()) {
		depositEdit->enterFocus();
	} else if(sharesEdit) {
		sharesEdit->enterFocus();
	} else if(downPaymentEdit) {
		downPaymentEdit->enterFocus();
	} else if(quantityEdit) {
		quantityEdit->enterFocus();
	} else if(dateEdit) {
		focusDate();
	}
}
void TransactionEditWidget::newFromAccount() {
	budget->resetDefaultCurrencyChanged();
	budget->resetCurrenciesModified();
	Account *account = fromCombo->createAccount();
	if(account) {		
		emit accountAdded(account);
		if(toCombo) toCombo->updateAccounts();
	}
	if(budget->currenciesModified() || budget->defaultCurrencyChanged()) emit currenciesModified();
}
void TransactionEditWidget::newToAccount() {
	budget->resetDefaultCurrencyChanged();
	budget->resetCurrenciesModified();
	Account *account = toCombo->createAccount();
	if(account) {		
		emit accountAdded(account);
		if(fromCombo) fromCombo->updateAccounts();
	}
	if(budget->currenciesModified() || budget->defaultCurrencyChanged()) emit currenciesModified();
}
void TransactionEditWidget::fromActivated() {
	if(toCombo && transtype != TRANSACTION_TYPE_EXPENSE) toCombo->focusAndSelectAll();
	else if(payeeEdit) payeeEdit->setFocus();
	else if(commentsEdit) commentsEdit->setFocus();
	if(transtype == TRANSACTION_TYPE_INCOME) {
		setDefaultValueFromCategory();
	}
}
void TransactionEditWidget::toActivated() {
	if(fromCombo && transtype == TRANSACTION_TYPE_EXPENSE) fromCombo->focusAndSelectAll();
	else if(payeeEdit) payeeEdit->setFocus();
	else if(commentsEdit) commentsEdit->setFocus();
	if(transtype == TRANSACTION_TYPE_EXPENSE) {
		setDefaultValueFromCategory();
	}
}
void TransactionEditWidget::fromChanged(Account *acc) {
	if(!acc) return;
	if(downPaymentEdit) {
		downPaymentEdit->setCurrency(acc->currency());
	} else if(valueEdit && acc->type() == ACCOUNT_TYPE_ASSETS) {
		valueEdit->setCurrency(acc->currency());
		if(quotationEdit && selectedSecurity()) {
			quotationEdit->setCurrency(acc->currency());
			security = selectedSecurity();
			bool b = (security->currency() == quotationEdit->currency());
			if(setQuoteButton->isEnabled() != b) {
				setQuoteButton->setEnabled(b);
				setQuoteButton->blockSignals(true);
				if(b) {
					setQuoteButton->setChecked(b_prev_update_quote);
				} else {
					b_prev_update_quote = setQuoteButton->isChecked();
					setQuoteButton->setChecked(false);
				}
				setQuoteButton->blockSignals(false);
			}
		}
	}
	if(transtype == TRANSACTION_TYPE_TRANSFER) {
		Currency *cur2 = splitcurrency;
		if(toCombo && toCombo->currentAccount()) {
			cur2 = toCombo->currentAccount()->currency();
		}
		if(depositEdit) {
			if(cur2 && acc->currency() && acc->currency() != cur2) {
				depositEdit->setEnabled(true);
			} else {
				depositEdit->setEnabled(false);
				if(is_zero(valueEdit->value())) valueEdit->setValue(depositEdit->value());
				else depositEdit->setValue(valueEdit->value());
			}
		}
	}
}
void TransactionEditWidget::toChanged(Account *acc) {
	if(!acc) return;
	if(transtype == TRANSACTION_TYPE_TRANSFER) {
		if(depositEdit) {
			depositEdit->setCurrency(acc->currency());
			Currency *cur2 = splitcurrency;
			if(fromCombo && fromCombo->currentAccount()) {
				cur2 = fromCombo->currentAccount()->currency();
			}
			if(cur2 && acc->currency() && acc->currency() != cur2) {
				depositEdit->setEnabled(true);
			} else {
				depositEdit->setEnabled(false);
				if(is_zero(valueEdit->value())) valueEdit->setValue(depositEdit->value());
				else depositEdit->setValue(valueEdit->value());
			}
		}
	} else {
		if(valueEdit && acc->type() == ACCOUNT_TYPE_ASSETS) {
			valueEdit->setCurrency(acc->currency());
			if(quotationEdit && selectedSecurity()) {
				quotationEdit->setCurrency(acc->currency());
				security = selectedSecurity();
				bool b = (security->currency() == quotationEdit->currency());
				if(setQuoteButton->isEnabled() != b) {
					setQuoteButton->setEnabled(b);
					setQuoteButton->blockSignals(true);
					if(b) {
						setQuoteButton->setChecked(b_prev_update_quote);
					} else {
						b_prev_update_quote = setQuoteButton->isChecked();
						setQuoteButton->setChecked(false);
					}
					setQuoteButton->blockSignals(false);
				}
			}
		}
	}
}
void TransactionEditWidget::focusDate() {
	if(!dateEdit) return;
	dateEdit->setFocus();
	dateEdit->setCurrentSection(QDateTimeEdit::DaySection);
}
void TransactionEditWidget::currentDateChanged(const QDate &olddate, const QDate &newdate) {
	if(dateEdit && olddate == dateEdit->date() && valueEdit && valueEdit->value() == 0.0 && descriptionEdit && descriptionEdit->text().isEmpty()) {
		dateEdit->setDate(newdate);
	}
}
void TransactionEditWidget::securityChanged(int index) {
	if(b_create_accounts && index == 0) {
		// New security
		EditSecurityDialog *dialog = new EditSecurityDialog(budget, this, tr("New Security", "Financial security (e.g. stock, mutual fund)"), b_create_accounts);
		if((b_create_accounts || dialog->checkAccount()) && dialog->exec() == QDialog::Accepted) {
			Security *sec = dialog->newSecurity();
			if(sec) {
				budget->addSecurity(sec);
				securityCombo->blockSignals(true);
				securityCombo->clear();
				securityCombo->addItem(tr("New Security…", "Financial security (e.g. stock, mutual fund)"), qVariantFromValue((void*) NULL));
				securityCombo->insertSeparator(1);
				int i2 = 2;
				for(SecurityList<Security*>::const_iterator it = budget->securities.constBegin(); it != budget->securities.constEnd(); ++it) {
					Security *c_sec = *it;
					securityCombo->addItem(c_sec->name(), qVariantFromValue((void*) c_sec));
					if(c_sec == sec) securityCombo->setCurrentIndex(i2);
					i2++;
				}
				security = sec;
				securityCombo->blockSignals(false);
				dialog->deleteLater();
				securityChanged();
				return;
			}
		}
		if(security) {
			securityCombo->setCurrentIndex(securityCombo->findData(qVariantFromValue((void*) security)));
		} else {
			securityCombo->setCurrentIndex(-1);
		}
		dialog->deleteLater();
		return;
	}
	security = selectedSecurity();
	if(security) {
		if(sharesEdit) sharesEdit->setPrecision(security->decimals());
		if(quotationEdit) {
			if(quotationEdit->value() == 0.0 && date().isValid() && security->currency() == quotationEdit->currency() && security->hasQuotation(date())) {
				quotationEdit->blockSignals(true);
				quotationEdit->setValue(security->getQuotation(date()));
				quotationEdit->blockSignals(false);
			}
			quotationEdit->setPrecision(security->quotationDecimals());
			bool b = (security->currency() == quotationEdit->currency());
			if(setQuoteButton->isEnabled() != b) {
				setQuoteButton->setEnabled(b);
				setQuoteButton->blockSignals(true);
				if(b) {
					setQuoteButton->setChecked(b_prev_update_quote);
				} else {
					b_prev_update_quote = setQuoteButton->isChecked();
					setQuoteButton->setChecked(false);
				}
				setQuoteButton->blockSignals(false);
			}
		}
		if(sharesEdit && security && shares_date.isValid()) sharesEdit->setMaximum(security->shares(shares_date));
	}
}
void TransactionEditWidget::currencyChanged(int index) {
	Currency *cur = (Currency*) currencyCombo->itemData(index).value<void*>();
	valueEdit->setPrecision(cur->fractionalDigits());
}
void TransactionEditWidget::valueEditingFinished() {
	if(valueEdit && depositEdit && depositEdit->isEnabled() && depositEdit->value() == 0.0 && valueEdit->currency() && depositEdit->currency()) {
		depositEdit->setValue(valueEdit->currency()->convertTo(valueEdit->value(), depositEdit->currency()));
	}
}
void TransactionEditWidget::valueChanged(double value) {
	if(valueEdit && depositEdit) {
		if(!depositEdit->isEnabled()) depositEdit->setValue(value);
	}
	if(valueEdit && commentsEdit && calculatedText_object == valueEdit && !calculatedText.isEmpty()) {
		if(commentsEdit->text().isEmpty()) commentsEdit->setText(calculatedText);
		calculatedText = "";
	}
	if(!quotationEdit || !sharesEdit || !valueEdit) return;
	value_set = value != 0.0;
	if(!shares_set && quotationEdit->value() != 0.0) {
		sharesEdit->blockSignals(true);
		sharesEdit->setValue(value / quotationEdit->value());
		sharesEdit->blockSignals(false);
	} else if(sharesEdit->value() != 0.0) {
		quotationEdit->blockSignals(true);
		quotationEdit->setValue(value / sharesEdit->value());
		quotationEdit->blockSignals(false);
	}
}
void TransactionEditWidget::sharesChanged(double value) {
	if(!quotationEdit || !sharesEdit || !valueEdit) return;
	shares_set = value != 0.0;
	if((!value_set && quotationEdit->value() != 0.0) || (transtype == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND && sharevalue_set)) {
		valueEdit->blockSignals(true);
		valueEdit->setValue(value * quotationEdit->value());
		valueEdit->blockSignals(false);
	} else if(value != 0.0) {
		quotationEdit->blockSignals(true);
		quotationEdit->setValue(valueEdit->value() / value);
		quotationEdit->blockSignals(false);
	}
}
void TransactionEditWidget::quotationChanged(double value) {
	if(!quotationEdit || !sharesEdit || !valueEdit) return;
	sharevalue_set = value != 0.0;
	if(!value_set || (transtype == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND && shares_set && sharesEdit->value() != 0.0)) {
		valueEdit->blockSignals(true);
		valueEdit->setValue(value * sharesEdit->value());
		valueEdit->blockSignals(false);
	} else if(value != 0.0) {
		sharesEdit->blockSignals(true);
		sharesEdit->setValue(valueEdit->value() / value);
		sharesEdit->blockSignals(false);
	}
}
void TransactionEditWidget::setMaxShares(double max) {
	if(sharesEdit) sharesEdit->setMaximum(max);
}
void TransactionEditWidget::setMaxSharesDate(QDate quotation_date) {
	shares_date = quotation_date;
	if(sharesEdit && security && shares_date.isValid()) sharesEdit->setMaximum(security->shares(shares_date));
}
void TransactionEditWidget::maxShares() {
	if(selectedSecurity() && sharesEdit) {
		sharesEdit->setValue(selectedSecurity()->shares(dateEdit->date()));
	}
}
QHBoxLayout *TransactionEditWidget::bottomLayout() {
	return bottom_layout;
}
void TransactionEditWidget::focusFirst() {
	if(!descriptionEdit) {
		if(b_select_security && securityCombo) securityCombo->setFocus();
		else if(valueEdit && transtype != TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND) valueEdit->setFocus();
		else if(sharesEdit) sharesEdit->setFocus();
	} else {
		descriptionEdit->setFocus();
	}
}
bool TransactionEditWidget::firstHasFocus() const {
	if(!descriptionEdit) {
		if(b_select_security && securityCombo) return securityCombo->hasFocus();
		else if(valueEdit && transtype != TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND) return valueEdit->hasFocus();
		else if(sharesEdit) return sharesEdit->hasFocus();
	}
	return descriptionEdit->hasFocus();
}
void TransactionEditWidget::setValues(QString description_value, double value_value, double quantity_value, QDate date_value, Account *from_account_value, Account *to_account_value, QString payee_value, QString comment_value) {
	if(descriptionEdit) descriptionEdit->setText(description_value);
	if(valueEdit) valueEdit->setValue(value_value);
	if(quantityEdit) quantityEdit->setValue(quantity_value);
	if(dateEdit && date_value.isValid()) dateEdit->setDate(date_value);
	if(fromCombo && from_account_value) fromCombo->setCurrentAccount(from_account_value);
	if(toCombo && to_account_value) toCombo->setCurrentAccount(to_account_value);
	if(payeeEdit) payeeEdit->setText(payee_value);
	if(commentsEdit) commentsEdit->setText(comment_value);
}
void TransactionEditWidget::setPayee(QString payee) {
	if(payeeEdit) payeeEdit->setText(payee);
}
QString TransactionEditWidget::description() const {
	if(!descriptionEdit) return QString();
	return descriptionEdit->text();
}
QString TransactionEditWidget::payee() const {
	if(!payeeEdit) return QString();
	return payeeEdit->text();
}
QString TransactionEditWidget::comments() const {
	if(!commentsEdit) return QString();
	return commentsEdit->text();
}
double TransactionEditWidget::value() const {
	if(!valueEdit) return 0.0;
	return valueEdit->value();
}
double TransactionEditWidget::quantity() const {
	if(!quantityEdit) return 1.0;
	return quantityEdit->value();
}
Account *TransactionEditWidget::fromAccount() const {
	if(!fromCombo) return NULL;
	return fromCombo->currentAccount();
}
Account *TransactionEditWidget::toAccount() const {
	if(!toCombo) return NULL;
	return toCombo->currentAccount();
}
QDate TransactionEditWidget::date() {
	if(!dateEdit) return QDate();
	return dateEdit->date();
}
void TransactionEditWidget::descriptionChanged(const QString&) {
	description_changed = true;
}
void TransactionEditWidget::payeeChanged(const QString&) {
	payee_changed = true;
}
void TransactionEditWidget::setDefaultValue() {
	if(descriptionEdit && description_changed && !descriptionEdit->text().isEmpty() && valueEdit && valueEdit->value() == 0.0) {
		Transaction *trans = NULL;
		if(default_values.contains(descriptionEdit->text().toLower())) trans = default_values[descriptionEdit->text().toLower()];
		if(trans) {
			if(trans->parentSplit() && trans->parentSplit()->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ACCOUNTS) valueEdit->setValue(trans->parentSplit()->value());
			else valueEdit->setValue(trans->value());
			if(toCombo) toCombo->setCurrentAccount(trans->toAccount());
			if(fromCombo) fromCombo->setCurrentAccount(trans->fromAccount());
			if(quantityEdit) {
				if(trans->quantity() <= 0.0) quantityEdit->setValue(1.0);
				else quantityEdit->setValue(trans->quantity());
			}
			if(payeeEdit && trans->type() == TRANSACTION_TYPE_EXPENSE) payeeEdit->setText(((Expense*) trans)->payee());
			if(payeeEdit && trans->type() == TRANSACTION_TYPE_INCOME) payeeEdit->setText(((Income*) trans)->payer());
		}
	}
}
void TransactionEditWidget::setDefaultValueFromPayee() {
	if(payeeEdit && payee_changed && !payeeEdit->text().isEmpty() && valueEdit && valueEdit->value() == 0.0 && descriptionEdit && descriptionEdit->text().isEmpty()) {
		Transaction *trans = NULL;
		if(default_payee_values.contains(payeeEdit->text().toLower())) trans = default_payee_values[payeeEdit->text().toLower()];
		if(trans) {
			if(trans->parentSplit() && trans->parentSplit()->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ACCOUNTS) valueEdit->setValue(trans->parentSplit()->value());
			else valueEdit->setValue(trans->value());
			if(toCombo) toCombo->setCurrentAccount(trans->toAccount());
			if(fromCombo) fromCombo->setCurrentAccount(trans->fromAccount());
			if(quantityEdit) {
				if(trans->quantity() <= 0.0) quantityEdit->setValue(1.0);
				else quantityEdit->setValue(trans->quantity());
			}
			if(descriptionEdit) descriptionEdit->setText(trans->description());
			if(valueEdit) {
				valueEdit->enterFocus();
			}
		}
	}
}
void TransactionEditWidget::setDefaultValueFromCategory() {
	if(((transtype == TRANSACTION_TYPE_INCOME && fromCombo) || (transtype == TRANSACTION_TYPE_EXPENSE && toCombo)) && valueEdit && valueEdit->value() == 0.0 && descriptionEdit && descriptionEdit->text().isEmpty()) {
		Transaction *trans = NULL;
		if(transtype == TRANSACTION_TYPE_INCOME && default_category_values.contains(fromAccount())) trans = default_category_values[fromAccount()];
		else if(transtype == TRANSACTION_TYPE_EXPENSE && default_category_values.contains(toAccount())) trans = default_category_values[toAccount()];
		if(trans) {
			if(trans->parentSplit() && trans->parentSplit()->type() == SPLIT_TRANSACTION_TYPE_MULTIPLE_ACCOUNTS) valueEdit->setValue(trans->parentSplit()->value());
			else valueEdit->setValue(trans->value());
			if(toCombo && transtype == TRANSACTION_TYPE_INCOME) toCombo->setCurrentAccount(trans->toAccount());
			if(fromCombo && transtype == TRANSACTION_TYPE_EXPENSE) fromCombo->setCurrentAccount(trans->fromAccount());
			if(quantityEdit) {
				if(trans->quantity() <= 0.0) quantityEdit->setValue(1.0);
				else quantityEdit->setValue(trans->quantity());
			}
			if(descriptionEdit) descriptionEdit->setText(trans->description());
			if(payeeEdit && trans->type() == TRANSACTION_TYPE_EXPENSE) payeeEdit->setText(((Expense*) trans)->payee());
			if(payeeEdit && trans->type() == TRANSACTION_TYPE_INCOME) payeeEdit->setText(((Income*) trans)->payer());
			if(valueEdit) {
				valueEdit->enterFocus();
			}
		}
	}
}
void TransactionEditWidget::updateFromAccounts(Account *exclude_account, Currency *force_currency, bool set_default) {
	if(!fromCombo) return;
	fromCombo->updateAccounts(exclude_account, force_currency);
	if(set_default) setDefaultFromAccount();
}
void TransactionEditWidget::updateToAccounts(Account *exclude_account, Currency *force_currency, bool set_default) {
	if(!toCombo) return;
	toCombo->updateAccounts(exclude_account, force_currency);
	if(set_default) setDefaultToAccount();
}
void TransactionEditWidget::updateAccounts(Account *exclude_account, Currency *force_currency, bool set_default) {
	Account *afrom = NULL;
	if(fromCombo) {
		afrom = fromCombo->currentAccount();
		fromCombo->clear();
	}
	updateToAccounts(exclude_account, force_currency, set_default);
	updateFromAccounts(exclude_account, force_currency, set_default);
	if(fromCombo) fromCombo->setCurrentAccount(afrom);
}
void TransactionEditWidget::transactionAdded(Transaction *trans) {
	if(descriptionEdit && trans->type() == transtype && (transtype != TRANSACTION_TYPE_INCOME || !((Income*) trans)->security()) && trans->subtype() != TRANSACTION_SUBTYPE_DEBT_FEE && trans->subtype() != TRANSACTION_SUBTYPE_DEBT_INTEREST) {
		if(!trans->description().isEmpty()) {
			if(!default_values.contains(trans->description().toLower())) {
				QList<QStandardItem*> row;
				row << new QStandardItem(trans->description());
				row << new QStandardItem(trans->description().toLower());
				((QStandardItemModel*) descriptionEdit->completer()->model())->appendRow(row);
				((QStandardItemModel*) descriptionEdit->completer()->model())->sort(1);
			}
			default_values[trans->description().toLower()] = trans;
		}
		if(payeeEdit && transtype == TRANSACTION_TYPE_EXPENSE && !((Expense*) trans)->payee().isEmpty()) {
			if(!default_payee_values.contains(((Expense*) trans)->payee().toLower())) {
				QList<QStandardItem*> row;
				row << new QStandardItem(((Expense*) trans)->payee());
				row << new QStandardItem(((Expense*) trans)->payee().toLower());
				((QStandardItemModel*) payeeEdit->completer()->model())->appendRow(row);
				((QStandardItemModel*) payeeEdit->completer()->model())->sort(1);
			}
			default_payee_values[((Expense*) trans)->payee().toLower()] = trans;
		} else if(payeeEdit && transtype == TRANSACTION_TYPE_INCOME && !((Income*) trans)->security() && !((Income*) trans)->payer().isEmpty()) {
			if(!default_payee_values.contains(((Income*) trans)->payer().toLower())) {
				QList<QStandardItem*> row;
				row << new QStandardItem(((Income*) trans)->payer());
				row << new QStandardItem(((Income*) trans)->payer().toLower());
				((QStandardItemModel*) payeeEdit->completer()->model())->appendRow(row);
				((QStandardItemModel*) payeeEdit->completer()->model())->sort(1);
			}
			default_payee_values[((Income*) trans)->payer().toLower()] = trans;
		}
		if(transtype == TRANSACTION_TYPE_INCOME && fromCombo) {
			default_category_values[trans->fromAccount()] = trans;
		} else if(transtype == TRANSACTION_TYPE_EXPENSE && toCombo) {
			default_category_values[trans->toAccount()] = trans;
		}
	}
}
void TransactionEditWidget::tagsModified() {
	if(tagButton) tagButton->updateTags();
}
void TransactionEditWidget::transactionModified(Transaction *trans) {
	transactionAdded(trans);
}
bool TransactionEditWidget::checkAccounts() {
	switch(transtype) {
		case TRANSACTION_TYPE_TRANSFER: {
			if(fromCombo && !fromCombo->hasAccount()) {
				QMessageBox::critical(this, tr("Error"), tr("No suitable account available."));
				return false;
			}
			if(toCombo && !toCombo->hasAccount()) {
				QMessageBox::critical(this, tr("Error"), tr("No suitable account available."));
				return false;
			}
			break;
		}
		case TRANSACTION_TYPE_INCOME: {
			if(fromCombo && !fromCombo->hasAccount()) {
				QMessageBox::critical(this, tr("Error"), tr("No income category available."));
				return false;
			}
			if(toCombo && !toCombo->hasAccount()) {
				QMessageBox::critical(this, tr("Error"), tr("No suitable account available."));
				return false;
			}
			break;
		}
		case TRANSACTION_TYPE_SECURITY_BUY: {
			if(fromCombo && !fromCombo->hasAccount()) {
				QMessageBox::critical(this, tr("Error"), tr("No suitable account or income category available."));
				return false;
			}
			break;
		}
		case TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND: {
			if(fromCombo && !fromCombo->hasAccount()) {
				QMessageBox::critical(this, tr("Error"), tr("No income category available."));
				return false;
			}
			break;
		}
		case TRANSACTION_TYPE_SECURITY_SELL: {
			if(toCombo && !toCombo->hasAccount()) {
				QMessageBox::critical(this, tr("Error"), tr("No suitable account available."));
				return false;
			}
			break;
		}
		default: {
			if(toCombo && !toCombo->hasAccount()) {
				QMessageBox::critical(this, tr("Error"), tr("No expense category available."));
				return false;
			}
			if(fromCombo && !fromCombo->hasAccount() && (!downPaymentEdit || !is_zero(downPaymentEdit->value()))) {
				QMessageBox::critical(this, tr("Error"), tr("No suitable account available."));
				return false;
			}
			break;
		}
	}
	if(securityCombo && securityCombo->count() <= (b_create_accounts ? 2 : 0)) {
		QMessageBox::critical(this, tr("Error"), tr("No security available.", "Financial security (e.g. stock, mutual fund)"));
		return false;
	}
	return true;
}
bool TransactionEditWidget::isCleared() {
	return (!valueEdit || valueEdit->value() == 0.0) && (!dateEdit || dateEdit->date() == QDate::currentDate()) && (!quantityEdit || quantityEdit->value() == 1.0) && (!depositEdit || depositEdit->value() == 0.0) && (!commentsEdit || commentsEdit->text().isEmpty()) && (!descriptionEdit || descriptionEdit->text().isEmpty()) && (!payeeEdit || payeeEdit->text().isEmpty());
}
bool TransactionEditWidget::validValues(bool) {

	if(dateEdit && !dateEdit->date().isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		dateEdit->setFocus();
		dateEdit->selectAll();
		return false;
	}

	if(!checkAccounts()) return false;

	if((toCombo && !toCombo->currentAccount()) || (fromCombo && !downPaymentEdit && !fromCombo->currentAccount())) return false;
	if(toCombo && fromCombo && toCombo->currentAccount() == fromCombo->currentAccount()) {
		QMessageBox::critical(this, tr("Error"), tr("Cannot transfer money to and from the same account."));
		return false;
	}
	if(downPaymentEdit && downPaymentEdit->value() >= valueEdit->value()) {
		QMessageBox::critical(this, tr("Error"), tr("Downpayment must be less than total cost."));
		downPaymentEdit->setFocus();
		return false;
	}
	switch(transtype) {
		case TRANSACTION_TYPE_TRANSFER: {
			if((toCombo && toCombo->currentAccount()->type() == ACCOUNT_TYPE_ASSETS && ((AssetsAccount*) toCombo->currentAccount())->accountType() == ASSETS_TYPE_SECURITIES) || (fromCombo && fromCombo->currentAccount()->type() == ACCOUNT_TYPE_ASSETS && ((AssetsAccount*) fromCombo->currentAccount())->accountType() == ASSETS_TYPE_SECURITIES)) {
				QMessageBox::critical(this, tr("Error"), tr("Cannot create a regular transfer to/from a securities account."));
				return false;
			}
			break;
		}
		case TRANSACTION_TYPE_INCOME: {
			if(toCombo && toCombo->currentAccount()->type() == ACCOUNT_TYPE_ASSETS && ((AssetsAccount*) toCombo->currentAccount())->accountType() == ASSETS_TYPE_SECURITIES) {
				QMessageBox::critical(this, tr("Error"), tr("Cannot create a regular income to a securities account."));
				return false;
			}
			break;
		}
		case TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND: {
			if(sharesEdit && sharesEdit->value() == 0.0) {
				QMessageBox::critical(this, tr("Error"), tr("Zero shares not allowed."));
				return false;
			}
			break;
		}
		case TRANSACTION_TYPE_SECURITY_BUY: {}
		case TRANSACTION_TYPE_SECURITY_SELL: {
			if(sharesEdit && sharesEdit->value() == 0.0) {
				QMessageBox::critical(this, tr("Error"), tr("Zero shares not allowed."));
				return false;
			}
			if(valueEdit && valueEdit->value() == 0.0) {
				QMessageBox::critical(this, tr("Error"), tr("Zero value not allowed."));
				return false;
			}
			if(quotationEdit && quotationEdit->value() == 0.0) {
				QMessageBox::critical(this, tr("Error"), tr("Zero price per share not allowed."));
				return false;
			}
			/*if(ask_questions && sharesEdit && selectedSecurity() && sharesEdit->value() > selectedSecurity()->shares(dateEdit->date())) {
				if(QMessageBox::warning(this, tr("Warning"), tr("Number of sold shares are greater than available shares at selected date. Do you want to create the transaction nevertheless?"), QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel) {
					return false;
				}
			}*/
			break;
		}
		default: {
			if(fromCombo && fromCombo->currentAccount() && fromCombo->currentAccount()->type() == ACCOUNT_TYPE_ASSETS && ((AssetsAccount*) fromCombo->currentAccount())->accountType() == ASSETS_TYPE_SECURITIES) {
				QMessageBox::critical(this, tr("Error"), tr("Cannot create a regular expense from a securities account."));
				return false;
			}
			break;
		}
	}

	return true;
}
bool TransactionEditWidget::modifyTransaction(Transaction *trans) {
	if(!validValues()) return false;
	bool b_transsec = (trans->type() == TRANSACTION_TYPE_SECURITY_BUY || trans->type() == TRANSACTION_TYPE_SECURITY_SELL || trans->subtype() == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND);
	if(b_sec) {
		if((transtype != TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND || transtype != trans->subtype()) && trans->type() != transtype) return false;
		if(trans->subtype() == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND) {
			if(securityCombo) ((ReinvestedDividend*) trans)->setSecurity(selectedSecurity());
			if(fromCombo) ((ReinvestedDividend*) trans)->setFromAccount(fromCombo->currentAccount());
		} else {
			if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY) {
				if(fromCombo) ((SecurityTransaction*) trans)->setAccount(fromCombo->currentAccount());
			} else {
				if(toCombo) ((SecurityTransaction*) trans)->setAccount(toCombo->currentAccount());
			}
			if(securityCombo) ((SecurityTransaction*) trans)->setSecurity(selectedSecurity());
		}
		if(dateEdit) trans->setDate(dateEdit->date());
		double shares = 0.0, value = 0.0, share_value = 0.0;
		if(valueEdit) value = valueEdit->value();
		if(sharesEdit) shares = sharesEdit->value();
		if(quotationEdit) share_value = quotationEdit->value();
		if(!quotationEdit) share_value = value / shares;
		else if(!sharesEdit) shares = value / share_value;
		else if(!valueEdit) value = shares * share_value;
		if(trans->subtype() == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND) {
			((ReinvestedDividend*) trans)->setValue(value);
			((ReinvestedDividend*) trans)->setShares(shares);
			if(setQuoteButton && setQuoteButton->isChecked()) ((ReinvestedDividend*) trans)->security()->setQuotation(trans->date(), share_value);
		} else {
			((SecurityTransaction*) trans)->setValue(value);
			((SecurityTransaction*) trans)->setShares(shares);
			if(setQuoteButton && setQuoteButton->isChecked()) ((SecurityTransaction*) trans)->security()->setQuotation(trans->date(), share_value);
		}
		if(commentsEdit) trans->setComment(commentsEdit->text());
		if(fileEdit) trans->setAssociatedFile(fileEdit->text());
		trans->setModified();
		return true;
	} else if(b_transsec) {
		return false;
	}
	if(dateEdit) trans->setDate(dateEdit->date());
	if(fromCombo) trans->setFromAccount(fromCombo->currentAccount());
	if(toCombo) {
		if(toCombo->currentAccount() != budget->balancingAccount && trans->toAccount() != budget->balancingAccount) {
			trans->setToAccount(toCombo->currentAccount());
		}
	}
	if(depositEdit && trans->type() == TRANSACTION_TYPE_TRANSFER) {
		((Transfer*) trans)->setAmount(valueEdit->value(), depositEdit->value());
	} else {
		trans->setValue(valueEdit->value());
	}
	if(descriptionEdit && (trans->type() != TRANSACTION_TYPE_INCOME || !((Income*) trans)->security())) trans->setDescription(descriptionEdit->text());
	if(commentsEdit) trans->setComment(commentsEdit->text());
	if(fileEdit) trans->setAssociatedFile(fileEdit->text());
	if(quantityEdit) trans->setQuantity(quantityEdit->value());
	if(payeeEdit && trans->type() == TRANSACTION_TYPE_EXPENSE) ((Expense*) trans)->setPayee(payeeEdit->text());
	if(payeeEdit && trans->type() == TRANSACTION_TYPE_INCOME) ((Income*) trans)->setPayer(payeeEdit->text());
	if(tagButton) tagButton->modifyTransaction(trans);
	trans->setModified();
	return true;
}
Security *TransactionEditWidget::selectedSecurity() {
	if(securityCombo && securityCombo->currentData().isValid()) {
		return (Security*) securityCombo->currentData().value<void*>();
	}
	return security;
}
Transactions *TransactionEditWidget::createTransactionWithLoan() {

	if(!validValues()) return NULL;
	
	AssetsAccount *loan = new AssetsAccount(budget, ASSETS_TYPE_LIABILITIES, tr("Loan for %1").arg(descriptionEdit->text().isEmpty() ? toCombo->currentAccount()->name() : descriptionEdit->text()));
	loan->setCurrency((Currency*) currencyCombo->currentData().value<void*>());
	if(payeeEdit && lenderEdit->text().isEmpty()) loan->setMaintainer(payeeEdit->text());
	else loan->setMaintainer(lenderEdit->text());
	budget->addAccount(loan);
	
	if(is_zero(downPaymentEdit->value())) {
		Expense *expense = new Expense(budget, valueEdit->value(), dateEdit->date(), (ExpensesAccount*) toCombo->currentAccount(), loan, descriptionEdit->text(), commentsEdit->text());
		if(quantityEdit) expense->setQuantity(quantityEdit->value());
		if(payeeEdit) expense->setPayee(payeeEdit->text());
		if(tagButton) tagButton->modifyTransaction(expense);
		return expense;
	}
	
	MultiAccountTransaction *split = new MultiAccountTransaction(budget, (CategoryAccount*) toCombo->currentAccount(), descriptionEdit->text());
	split->setComment(commentsEdit->text());
	if(fileEdit) split->setAssociatedFile(fileEdit->text());
	if(quantityEdit) split->setQuantity(quantityEdit->value());
	
	Expense *expense = new Expense(budget, valueEdit->value() - downPaymentEdit->value(), dateEdit->date(), (ExpensesAccount*) toCombo->currentAccount(), loan);
	split->addTransaction(expense);
	
	Expense *down_payment = new Expense(budget, downPaymentEdit->value(), dateEdit->date(), (ExpensesAccount*) toCombo->currentAccount(), (AssetsAccount*) fromCombo->currentAccount());
	down_payment->setPayee(loan->maintainer());
	split->addTransaction(down_payment);
	if(tagButton) tagButton->modifyTransaction(split);
	return split;
}
Transaction *TransactionEditWidget::createTransaction() {
	if(!validValues()) return NULL;
	Transaction *trans;
	if(transtype == TRANSACTION_TYPE_TRANSFER) {
		if(toCombo && toCombo->currentAccount() == budget->balancingAccount) {
			trans = new Balancing(budget, -valueEdit->value(), dateEdit ? dateEdit->date() : QDate(), fromCombo ? (AssetsAccount*) fromCombo->currentAccount() : NULL);
		} else if(fromCombo && fromCombo->currentAccount() == budget->balancingAccount) {
			trans = new Balancing(budget, valueEdit->value(), dateEdit ? dateEdit->date() : QDate(), toCombo ? (AssetsAccount*) toCombo->currentAccount() : NULL);
		} else {
			Transfer *transfer = new Transfer(budget, valueEdit->value(), depositEdit->value(), dateEdit ? dateEdit->date() : QDate(), fromCombo ? (AssetsAccount*) fromCombo->currentAccount() : NULL, toCombo ? (AssetsAccount*) toCombo->currentAccount() : NULL, descriptionEdit ? descriptionEdit->text() : QString(), commentsEdit ? commentsEdit->text() : NULL);
			trans = transfer;
		}
	} else if(transtype == TRANSACTION_TYPE_INCOME) {
		Income *income = new Income(budget, valueEdit->value(), dateEdit ? dateEdit->date() : QDate(), fromCombo ? (IncomesAccount*) fromCombo->currentAccount() : NULL, toCombo ? (AssetsAccount*) toCombo->currentAccount() : NULL, descriptionEdit ? descriptionEdit->text() : QString(), commentsEdit ? commentsEdit->text() : NULL);
		if(selectedSecurity()) income->setSecurity(selectedSecurity());
		if(quantityEdit) income->setQuantity(quantityEdit->value());
		if(payeeEdit) income->setPayer(payeeEdit->text());
		trans = income;
	} else if(transtype == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND) {
		if(!selectedSecurity()) return NULL;
		double shares = 0.0, value = 0.0, share_value = 0.0;
		if(valueEdit) value = valueEdit->value();
		if(sharesEdit) shares = sharesEdit->value();
		if(quotationEdit) share_value = quotationEdit->value();
		if(!quotationEdit) share_value = value / shares;
		else if(!sharesEdit) shares = value / share_value;
		else if(!valueEdit) value = shares * share_value;
		ReinvestedDividend *rediv = new ReinvestedDividend(budget, value, shares, dateEdit ? dateEdit->date() : QDate(), selectedSecurity(), fromCombo ? (IncomesAccount*) fromCombo->currentAccount() : NULL, commentsEdit ? commentsEdit->text() : NULL);
		if(setQuoteButton && setQuoteButton->isChecked()) selectedSecurity()->setQuotation(rediv->date(), share_value);
		trans = rediv;
	} else if(transtype == TRANSACTION_TYPE_SECURITY_BUY) {
		if(!selectedSecurity()) return NULL;
		double shares = 0.0, value = 0.0, share_value = 0.0;
		if(valueEdit) value = valueEdit->value();
		if(sharesEdit) shares = sharesEdit->value();
		if(quotationEdit) share_value = quotationEdit->value();
		if(!quotationEdit) share_value = value / shares;
		else if(!sharesEdit) shares = value / share_value;
		else if(!valueEdit) value = shares * share_value;
		SecurityBuy *secbuy = new SecurityBuy(selectedSecurity(), value, shares, dateEdit ? dateEdit->date() : QDate(), fromCombo ? fromCombo->currentAccount() : NULL, commentsEdit ? commentsEdit->text() : NULL);
		if(setQuoteButton && setQuoteButton->isChecked()) selectedSecurity()->setQuotation(secbuy->date(), share_value);
		trans = secbuy;
	} else if(transtype == TRANSACTION_TYPE_SECURITY_SELL) {
		if(!selectedSecurity()) return NULL;
		double shares = 0.0, value = 0.0, share_value = 0.0;
		if(valueEdit) value = valueEdit->value();
		if(sharesEdit) shares = sharesEdit->value();
		if(quotationEdit) share_value = quotationEdit->value();
		if(!quotationEdit) share_value = value / shares;
		else if(!sharesEdit) shares = value / share_value;
		else if(!valueEdit) value = shares * share_value;
		SecuritySell *secsell = new SecuritySell(selectedSecurity(), value, shares, dateEdit ? dateEdit->date() : QDate(), toCombo ? toCombo->currentAccount() : NULL, commentsEdit ? commentsEdit->text() : NULL);
		if(setQuoteButton && setQuoteButton->isChecked()) selectedSecurity()->setQuotation(secsell->date(), share_value);
		trans = secsell;
	} else {
		Expense *expense = new Expense(budget, valueEdit->value(), dateEdit ? dateEdit->date() : QDate(), toCombo ? (ExpensesAccount*) toCombo->currentAccount() : NULL, fromCombo ? (AssetsAccount*) fromCombo->currentAccount() : NULL, descriptionEdit ? descriptionEdit->text() : QString(), commentsEdit ? commentsEdit->text() : NULL);
		if(quantityEdit) expense->setQuantity(quantityEdit->value());
		if(payeeEdit) expense->setPayee(payeeEdit->text());
		trans = expense;
	}
	if(fileEdit) trans->setAssociatedFile(fileEdit->text());
	if(tagButton) tagButton->modifyTransaction(trans);
	return trans;
}
void TransactionEditWidget::transactionRemoved(Transaction *trans) {
	if(descriptionEdit && trans->type() == transtype && !trans->description().isEmpty() && default_values.contains(trans->description().toLower()) && default_values[trans->description().toLower()] == trans) {
		QString lower_description = trans->description().toLower();
		default_values[trans->description().toLower()] = NULL;
		switch(transtype) {
			case TRANSACTION_TYPE_EXPENSE: {
				for(TransactionList<Expense*>::const_iterator it = budget->expenses.constEnd(); it != budget->expenses.constBegin();) {
					--it;
					Expense *expense = *it;
					if(expense != trans && expense->description().toLower() == lower_description && expense->subtype() != TRANSACTION_SUBTYPE_DEBT_FEE && expense->subtype() != TRANSACTION_SUBTYPE_DEBT_INTEREST) {
						default_values[lower_description] = expense;
						break;
					}
				}
				break;
			}
			case TRANSACTION_TYPE_INCOME: {
				for(TransactionList<Income*>::const_iterator it = budget->incomes.constEnd(); it != budget->incomes.constBegin();) {
					--it;
					Income *income = *it;
					if(income != trans && !income->security() && income->description().toLower() == lower_description) {
						default_values[lower_description] = income;
						break;
					}
				}
				break;
			}
			case TRANSACTION_TYPE_TRANSFER: {
				for(TransactionList<Transfer*>::const_iterator it = budget->transfers.constEnd(); it != budget->transfers.constBegin();) {
					--it;
					Transfer *transfer = *it;
					if(transfer != trans && transfer->description().toLower() == lower_description) {
						default_values[lower_description] = transfer;
						break;
					}
				}
				break;
			}
			default: {}
		}
	}
	if(payeeEdit && transtype == TRANSACTION_TYPE_INCOME && trans->type() == transtype && !((Income*) trans)->payer().isEmpty() && default_payee_values.contains(((Income*) trans)->payer().toLower()) && default_payee_values[((Income*) trans)->payer().toLower()] == trans) {
		default_payee_values[((Income*) trans)->payer().toLower()] = NULL;
		QString lower_payee = ((Income*) trans)->payer().toLower();
		for(TransactionList<Income*>::const_iterator it = budget->incomes.constEnd(); it != budget->incomes.constBegin();) {
			--it;
			Income *income = *it;
			if(income != trans && !income->security()  && income->payer().toLower() == lower_payee) {
				default_payee_values[lower_payee] = income;
				break;
			}
		}
	}
	if(payeeEdit && transtype == TRANSACTION_TYPE_EXPENSE && trans->type() == transtype && !((Expense*) trans)->payee().isEmpty() && default_payee_values.contains(((Expense*) trans)->payee().toLower()) && default_payee_values[((Expense*) trans)->payee().toLower()] == trans) {
		default_payee_values[((Expense*) trans)->payee().toLower()] = NULL;
		QString lower_payee = ((Expense*) trans)->payee().toLower();
		for(TransactionList<Expense*>::const_iterator it = budget->expenses.constEnd(); it != budget->expenses.constBegin();) {
			--it;
			Expense *expense = *it;
			if(expense != trans && expense->payee().toLower() == lower_payee && expense->subtype() != TRANSACTION_SUBTYPE_DEBT_FEE && expense->subtype() != TRANSACTION_SUBTYPE_DEBT_INTEREST) {
				default_payee_values[lower_payee] = expense;
				break;
			}
		}
	}
	if(transtype == TRANSACTION_TYPE_INCOME && fromCombo && trans->type() == transtype && default_category_values.contains(trans->fromAccount()) && default_category_values[trans->fromAccount()] == trans) {
		bool category_found = false;
		Account *cat = trans->fromAccount();
		for(TransactionList<Income*>::const_iterator it = budget->incomes.constEnd(); it != budget->incomes.constBegin();) {
			--it;
			Income *income = *it;
			if(income != trans && !income->security()  && income->fromAccount() == cat) {
				default_category_values[cat] = income;
				category_found = true;
				break;
			}
		}
		if(!category_found) default_category_values.remove(cat);
	} else if(transtype == TRANSACTION_TYPE_EXPENSE && toCombo && trans->type() == transtype && default_category_values.contains(trans->toAccount()) && default_category_values[trans->toAccount()] == trans) {
		bool category_found = false;
		Account *cat = trans->toAccount();
		for(TransactionList<Expense*>::const_iterator it = budget->expenses.constEnd(); it != budget->expenses.constBegin();) {
			--it;
			Expense *expense = *it;
			if(expense != trans && expense->toAccount() == cat && expense->subtype() != TRANSACTION_SUBTYPE_DEBT_FEE && expense->subtype() != TRANSACTION_SUBTYPE_DEBT_INTEREST) {
				default_category_values[cat] = expense;
				category_found = true;
				break;
			}
		}
		if(!category_found) default_category_values.remove(cat);
	}
}
void TransactionEditWidget::transactionsReset() {
	if(descriptionEdit) ((QStandardItemModel*) descriptionEdit->completer()->model())->clear();
	if(payeeEdit) ((QStandardItemModel*) payeeEdit->completer()->model())->clear();
	default_values.clear();
	default_payee_values.clear();
	default_category_values.clear();
	switch(transtype) {
		case TRANSACTION_TYPE_EXPENSE: {
			for(TransactionList<Expense*>::const_iterator it = budget->expenses.constEnd(); it != budget->expenses.constBegin();) {
				--it;
				Expense *expense = *it;
				if(expense->subtype() != TRANSACTION_SUBTYPE_DEBT_FEE && expense->subtype() != TRANSACTION_SUBTYPE_DEBT_INTEREST) {
					if(descriptionEdit && !expense->description().isEmpty() && !default_values.contains(expense->description().toLower())) {
						QList<QStandardItem*> row;
						row << new QStandardItem(expense->description());
						row << new QStandardItem(expense->description().toLower());
						((QStandardItemModel*) descriptionEdit->completer()->model())->appendRow(row);
						default_values[expense->description().toLower()] = expense;
					}
					if(payeeEdit && !expense->payee().isEmpty() && !default_payee_values.contains(expense->payee().toLower())) {
						QList<QStandardItem*> row;
						row << new QStandardItem(expense->payee());
						row << new QStandardItem(expense->payee().toLower());
						((QStandardItemModel*) payeeEdit->completer()->model())->appendRow(row);
						default_payee_values[expense->payee().toLower()] = expense;
					}
					if(toCombo && !default_category_values.contains(expense->category())) {
						default_category_values[expense->category()] = expense;
					}
				}
			}
			break;
		}
		case TRANSACTION_TYPE_INCOME: {
			for(TransactionList<Income*>::const_iterator it = budget->incomes.constEnd(); it != budget->incomes.constBegin();) {
				--it;
				Income *income = *it;
				if(!income->security()) {
					if(descriptionEdit && !income->description().isEmpty() && !default_values.contains(income->description().toLower())) {
						QList<QStandardItem*> row;
						row << new QStandardItem(income->description());
						row << new QStandardItem(income->description().toLower());
						((QStandardItemModel*) descriptionEdit->completer()->model())->appendRow(row);
						default_values[income->description().toLower()] = income;
					}
					if(payeeEdit && !income->payer().isEmpty() && !default_payee_values.contains(income->payer().toLower())) {
						QList<QStandardItem*> row;
						row << new QStandardItem(income->payer());
						row << new QStandardItem(income->payer().toLower());
						((QStandardItemModel*) payeeEdit->completer()->model())->appendRow(row);
						default_payee_values[income->payer().toLower()] = income;
					}
					if(fromCombo && !default_category_values.contains(income->category())) {
						default_category_values[income->category()] = income;
					}
				}
			}
			break;
		}
		case TRANSACTION_TYPE_TRANSFER: {
			for(TransactionList<Transfer*>::const_iterator it = budget->transfers.constEnd(); it != budget->transfers.constBegin();) {
				--it;
				Transfer *transfer = *it;
				if(descriptionEdit && !transfer->description().isEmpty() && !default_values.contains(transfer->description().toLower())) {
					QList<QStandardItem*> row;
					row << new QStandardItem(transfer->description());
					row << new QStandardItem(transfer->description().toLower());
					((QStandardItemModel*) descriptionEdit->completer()->model())->appendRow(row);
					default_values[transfer->description().toLower()] = transfer;
				}
			}
			break;
		}
		default: {}
	}
	if(descriptionEdit) ((QStandardItemModel*) descriptionEdit->completer()->model())->sort(1);
	if(payeeEdit) ((QStandardItemModel*) payeeEdit->completer()->model())->sort(1);
}
void TransactionEditWidget::newTag() {
	QString new_tag = tagButton->createTag();
	if(!new_tag.isEmpty()) emit tagAdded(new_tag);
}
void TransactionEditWidget::setDefaultFromAccount() {
	if(!fromCombo) return;
	if((transtype == TRANSACTION_TYPE_INCOME && security) || transtype == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND) {
		for(TransactionList<Income*>::const_iterator it = budget->incomes.constEnd(); it != budget->incomes.constBegin();) {
			--it;
			Income *income = *it;
			if(income->security()) {
				fromCombo->setCurrentAccount(income->category());
				break;
			}
		}
	} else if(transtype == TRANSACTION_TYPE_INCOME) {
		if(budget->incomes.isEmpty()) return;
		Income *trans = budget->incomes.last();
		if(trans) fromCombo->setCurrentAccount(trans->category());
	} else if(transtype == TRANSACTION_TYPE_EXPENSE) {
		if(budget->expenses.isEmpty()) return;
		Expense *trans = budget->expenses.last();
		if(trans) fromCombo->setCurrentAccount(trans->from());
	} else if(transtype == TRANSACTION_TYPE_TRANSFER) {
		if(budget->transfers.isEmpty()) return;
		Transaction *trans = budget->transfers.last();
		if(trans) fromCombo->setCurrentAccount(trans->fromAccount());
	} else if(transtype == TRANSACTION_TYPE_SECURITY_BUY) {
		if(budget->securityTransactions.isEmpty()) return;
		SecurityTransaction *trans = budget->securityTransactions.last();
		if(trans) fromCombo->setCurrentAccount(trans->account());
	}
}

void TransactionEditWidget::setDefaultToAccount() {
	if(!toCombo) return;
	if(transtype == TRANSACTION_TYPE_INCOME) {
		if(budget->incomes.isEmpty()) return;
		Income *trans = budget->incomes.last();
		if(trans) toCombo->setCurrentAccount(trans->to());
	} else if(transtype == TRANSACTION_TYPE_EXPENSE) {
		if(budget->expenses.isEmpty()) return;
		Expense *trans = budget->expenses.last();
		if(trans) toCombo->setCurrentAccount(trans->category());
	} else if(transtype == TRANSACTION_TYPE_TRANSFER) {
		if(budget->transfers.isEmpty()) return;
		Transaction *trans = budget->transfers.last();
		if(trans) toCombo->setCurrentAccount(trans->toAccount());
	} else if(transtype == TRANSACTION_TYPE_SECURITY_SELL) {
		if(budget->securityTransactions.isEmpty()) return;
		SecurityTransaction *trans = budget->securityTransactions.last();
		if(trans) toCombo->setCurrentAccount(trans->account());
	}
}
void TransactionEditWidget::setDefaultAccounts() {
	setDefaultToAccount();
	setDefaultFromAccount();
}
void TransactionEditWidget::setAccount(Account *account) {
	if(fromCombo && (transtype == TRANSACTION_TYPE_EXPENSE || transtype == TRANSACTION_TYPE_TRANSFER || transtype == TRANSACTION_TYPE_SECURITY_BUY)) {
		fromCombo->setCurrentAccount(account);
	} else if(toCombo) {
		toCombo->setCurrentAccount(account);
	}
}
void TransactionEditWidget::setFromAccount(Account *account) {
	if(fromCombo) fromCombo->setCurrentAccount(account);
}
void TransactionEditWidget::setToAccount(Account *account) {
	if(toCombo) toCombo->setCurrentAccount(account);
}
void TransactionEditWidget::setTransaction(Transaction *trans) {
	if(valueEdit) valueEdit->blockSignals(true);
	if(sharesEdit) sharesEdit->blockSignals(true);
	if(quotationEdit) quotationEdit->blockSignals(true);
	blockSignals(true);
	b_select_security = false;
	if(trans == NULL) {
		value_set = false; shares_set = false; sharevalue_set = false;
		description_changed = false;
		payee_changed = false;
		if(dateEdit) dateEdit->setDate(QDate::currentDate());
		if(b_sec) {
			if(sharesEdit) sharesEdit->setValue(0.0);
			if(quotationEdit) quotationEdit->setValue(0.0);
			if(valueEdit) valueEdit->setValue(0.0);
			if(valueEdit) valueEdit->setFocus();
			else sharesEdit->setFocus();
		} else {
			if(descriptionEdit) {
				descriptionEdit->clear();
				if(isVisible()) descriptionEdit->setFocus();
			} else {
				if(isVisible()) valueEdit->setFocus();
			}
			valueEdit->setValue(0.0);
		}
		if(commentsEdit) commentsEdit->clear();
		if(fileEdit) fileEdit->clear();
		if(quantityEdit) quantityEdit->setValue(1.0);
		if(depositEdit) depositEdit->setValue(0.0);
		if(payeeEdit) payeeEdit->clear();
		if(dateEdit) emit dateChanged(dateEdit->date());
	} else {
		value_set = true; shares_set = true; sharevalue_set = true;
		if(dateEdit) dateEdit->setDate(trans->date());
		if(commentsEdit) commentsEdit->setText(trans->comment());
		if(fileEdit) fileEdit->setText(trans->associatedFile());
		if(toCombo && (!b_sec || transtype == TRANSACTION_TYPE_SECURITY_SELL)) {
			toCombo->setCurrentAccount(trans->toAccount());
		}
		if(fromCombo && (!b_sec || transtype == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND || transtype == TRANSACTION_TYPE_SECURITY_BUY)) {
			fromCombo->setCurrentAccount(trans->fromAccount());
		}
		if(depositEdit) {
			if(valueEdit) valueEdit->setValue(trans->fromValue());
			depositEdit->setValue(trans->toValue());
		} else if(valueEdit) {
			valueEdit->setValue(trans->value());
		}
		if(b_sec) {
			if((transtype != TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND || transtype != trans->subtype()) && trans->type() != transtype) return;
			//if(transtype == TRANSACTION_TYPE_SECURITY_SELL) setMaxShares(((SecurityTransaction*) trans)->security()->shares(QDate::currentDate()) + ((SecurityTransaction*) trans)->shares());
			if(sharesEdit) sharesEdit->setValue(trans->subtype() == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND ? ((ReinvestedDividend*) trans)->shares() : ((SecurityTransaction*) trans)->shares());
			if(quotationEdit) {
				quotationEdit->setValue(trans->value() / (trans->subtype() == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND ? ((ReinvestedDividend*) trans)->shares() : ((SecurityTransaction*) trans)->shares()));
				if(setQuoteButton) {
					Security *sec = (trans->subtype() == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND ? ((ReinvestedDividend*) trans)->security() : ((SecurityTransaction*) trans)->security());
					setQuoteButton->blockSignals(true);
					if(sec->currency() == quotationEdit->currency()) {
						setQuoteButton->setChecked(sec->hasQuotation(trans->date()) && sec->getQuotation(trans->date()) == quotationEdit->value());
						setQuoteButton->setEnabled(true);
					} else {
						b_prev_update_quote = setQuoteButton->isChecked();
						setQuoteButton->setChecked(false);
						setQuoteButton->setEnabled(false);
					}
					setQuoteButton->blockSignals(false);
				}
			}
			if(isVisible()) {
				if(valueEdit) valueEdit->setFocus();
				else sharesEdit->setFocus();
			}
		} else {
			description_changed = false;
			payee_changed = false;
			if(descriptionEdit) {
				descriptionEdit->setText(trans->description());
				if(isVisible()) {
					descriptionEdit->setFocus();
					descriptionEdit->selectAll();
				}
			} else {
				if(isVisible()) valueEdit->setFocus();
			}
			if(quantityEdit) quantityEdit->setValue(trans->quantity());
			if(payeeEdit && trans->type() == TRANSACTION_TYPE_EXPENSE) payeeEdit->setText(((Expense*) trans)->payee());
			if(payeeEdit && trans->type() == TRANSACTION_TYPE_INCOME) payeeEdit->setText(((Income*) trans)->payer());
		}
		if(dateEdit) emit dateChanged(trans->date());
	}
	if(tagButton) tagButton->setTransaction(trans);
	blockSignals(false);
	if(valueEdit) valueEdit->blockSignals(false);
	if(sharesEdit) sharesEdit->blockSignals(false);
	if(quotationEdit) quotationEdit->blockSignals(false);
	emit propertyChanged();
}
void TransactionEditWidget::setTransaction(Transaction *trans, const QDate &date) {
	if(dateEdit) dateEdit->blockSignals(true);
	setTransaction(trans);
	if(dateEdit) {
		dateEdit->setDate(date);
		dateEdit->blockSignals(false);
		emit dateChanged(date);
	}
}
void TransactionEditWidget::setMultiAccountTransaction(MultiAccountTransaction *split) {
	if(!split) {
		setTransaction(NULL);
		return;
	}
	blockSignals(true);
	b_select_security = false;
	if(valueEdit) valueEdit->blockSignals(true);
	if(dateEdit) dateEdit->setDate(split->date());
	if(dateEdit) emit dateChanged(split->date());
	if(commentsEdit) commentsEdit->setText(split->comment());
	if(fileEdit) fileEdit->setText(split->associatedFile());
	if(split->transactiontype() == TRANSACTION_TYPE_EXPENSE) {
		if(toCombo) {
			toCombo->setCurrentAccount(split->category());
		}
		if(fromCombo) {
			Account *acc = split->at(0)->fromAccount();
			if(acc) {
				fromCombo->setCurrentAccount(acc);
			}
		}
	} else {
		if(fromCombo) {
			fromCombo->setCurrentAccount(split->category());
		}
		if(toCombo) {
			Account *acc = split->at(0)->toAccount();
			if(acc) {
				toCombo->setCurrentAccount(acc);
			}
		}
	}
	description_changed = false;
	payee_changed = false;
	if(descriptionEdit) {
		descriptionEdit->setText(split->description());
		if(isVisible()) {
			descriptionEdit->setFocus();
			descriptionEdit->selectAll();
		}
	} else {
		if(isVisible()) valueEdit->enterFocus();
	}
	valueEdit->setValue(split->value());
	if(quantityEdit) quantityEdit->setValue(split->quantity());
	if(payeeEdit) payeeEdit->setText(split->payeeText());
	if(tagButton) tagButton->setTransaction(split);
	blockSignals(false);
	emit propertyChanged();
}

TransactionEditDialog::TransactionEditDialog(bool extra_parameters, int transaction_type, Currency *split_currency, bool transfer_to, Security *security, SecurityValueDefineType security_value_type, bool select_security, Budget *budg, QWidget *parent, bool allow_account_creation, bool multiaccount, bool withloan) : QDialog(parent, 0){
	setModal(true);
	QVBoxLayout *box1 = new QVBoxLayout(this);
	switch(transaction_type) {
		case TRANSACTION_TYPE_EXPENSE: {setWindowTitle(tr("Edit Expense")); break;}
		case TRANSACTION_TYPE_INCOME: {
			if(security || select_security) setWindowTitle(tr("Edit Dividend"));
			else setWindowTitle(tr("Edit Income"));
			break;
		}
		case TRANSACTION_TYPE_TRANSFER: {setWindowTitle(tr("Edit Transfer")); break;}
		case TRANSACTION_TYPE_SECURITY_BUY: {setWindowTitle(tr("Edit Securities Purchase", "Financial security (e.g. stock, mutual fund)")); break;}
		case TRANSACTION_TYPE_SECURITY_SELL: {setWindowTitle(tr("Edit Securities Sale", "Financial security (e.g. stock, mutual fund)")); break;}
		case TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND: {setWindowTitle(tr("Edit Reinvested Dividend")); break;}
	}
	editWidget = new TransactionEditWidget(false, extra_parameters, transaction_type, split_currency, transfer_to, security, security_value_type, select_security, budg, this, allow_account_creation, multiaccount, withloan);
	box1->addWidget(editWidget);
	editWidget->transactionsReset();
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);
}
void TransactionEditDialog::accept() {
	if(editWidget->validValues(true)) {
		QDialog::accept();
	}
}
void TransactionEditDialog::keyPressEvent(QKeyEvent *e) {
	if(e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) return;
	QDialog::keyPressEvent(e);
}

MultipleTransactionsEditDialog::MultipleTransactionsEditDialog(bool extra_parameters, int transaction_type, Budget *budg, QWidget *parent, bool allow_account_creation)  : QDialog(parent), transtype(transaction_type), budget(budg), b_extra(extra_parameters), b_create_accounts(allow_account_creation) {

	setWindowTitle(tr("Modify Transactions"));
	setModal(true);

	added_account = NULL;
	
	/*int rows = 4;
	if(b_extra && (transtype == TRANSACTION_TYPE_EXPENSE || transtype == TRANSACTION_TYPE_INCOME)) rows= 5;
	else if(transtype == TRANSACTION_TYPE_TRANSFER) rows = 3;
	else rows = 2;*/
	QVBoxLayout *box1 = new QVBoxLayout(this);
	QGridLayout *editLayout = new QGridLayout();
	box1->addLayout(editLayout);

	descriptionButton = new QCheckBox(tr("Description:", "Transaction description property (transaction title/generic article name)"), this);
	descriptionButton->setChecked(false);
	editLayout->addWidget(descriptionButton, 0, 0);
	descriptionEdit = new QLineEdit(this);
	descriptionEdit->setEnabled(false);
	editLayout->addWidget(descriptionEdit, 0, 1);

	valueButton = NULL;
	valueEdit = NULL;
	if(transtype == TRANSACTION_TYPE_EXPENSE || transtype == TRANSACTION_TYPE_INCOME || transtype == TRANSACTION_TYPE_TRANSFER) {
		if(transtype == TRANSACTION_TYPE_TRANSFER) valueButton = new QCheckBox(tr("Amount:"), this);
		else if(transtype == TRANSACTION_TYPE_INCOME) valueButton = new QCheckBox(tr("Income:"), this);
		else valueButton = new QCheckBox(tr("Cost:"), this);
		valueButton->setChecked(false);
		editLayout->addWidget(valueButton, 1, 0);
		valueEdit = new EqonomizeValueEdit(false, this, budget);
		valueEdit->setEnabled(false);
		editLayout->addWidget(valueEdit, 1, 1);
	}

	dateButton = new QCheckBox(tr("Date:"), this);
	dateButton->setChecked(false);
	editLayout->addWidget(dateButton, 2, 0);
	dateEdit = new EqonomizeDateEdit(QDate::currentDate(), this);
	dateEdit->setCalendarPopup(true);
	dateEdit->setEnabled(false);
	editLayout->addWidget(dateEdit, 2, 1);

	categoryButton = NULL;
	categoryCombo = NULL;
	if(transtype == TRANSACTION_TYPE_EXPENSE || transtype == TRANSACTION_TYPE_INCOME) {
		categoryButton = new QCheckBox(tr("Category:"), this);
		categoryButton->setChecked(false);
		editLayout->addWidget(categoryButton, 3, 0);
		categoryCombo = new AccountComboBox(transtype == TRANSACTION_TYPE_EXPENSE ? ACCOUNT_TYPE_EXPENSES : ACCOUNT_TYPE_INCOMES, budget, b_create_accounts, false, false, true, true, this);
		categoryCombo->setEnabled(false);
		updateAccounts();
		editLayout->addWidget(categoryCombo, 3, 1);
		connect(categoryButton, SIGNAL(toggled(bool)), categoryCombo, SLOT(setEnabled(bool)));
	}

	payeeButton = NULL;
	payeeEdit = NULL;
	if(b_extra && (transtype == TRANSACTION_TYPE_EXPENSE || transtype == TRANSACTION_TYPE_INCOME)) {
		if(transtype == TRANSACTION_TYPE_INCOME) payeeButton = new QCheckBox(tr("Payer:"), this);
		else payeeButton = new QCheckBox(tr("Payee:"), this);
		payeeButton->setChecked(false);
		editLayout->addWidget(payeeButton, 4, 0);
		payeeEdit = new QLineEdit(this);
		payeeEdit->setEnabled(false);
		editLayout->addWidget(payeeEdit, 4, 1);
		connect(payeeButton, SIGNAL(toggled(bool)), payeeEdit, SLOT(setEnabled(bool)));
	}
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);

	if(categoryCombo) connect(categoryCombo, SIGNAL(newAccountRequested()), this, SLOT(newCategory()));
	connect(descriptionButton, SIGNAL(toggled(bool)), descriptionEdit, SLOT(setEnabled(bool)));
	if(valueButton) connect(valueButton, SIGNAL(toggled(bool)), valueEdit, SLOT(setEnabled(bool)));
	connect(dateButton, SIGNAL(toggled(bool)), dateEdit, SLOT(setEnabled(bool)));

}
void MultipleTransactionsEditDialog::newCategory() {
	categoryCombo->createAccount();
}
void MultipleTransactionsEditDialog::setTransaction(Transaction *trans) {
	if(trans) {
		descriptionEdit->setText(trans->description());
		dateEdit->setDate(trans->date());
		if(valueEdit) {
			valueEdit->setValue(trans->value());
			valueEdit->setCurrency(trans->currency());
		}
		if(transtype == TRANSACTION_TYPE_EXPENSE) {
			categoryCombo->setCurrentAccount(((Expense*) trans)->category());
			if(payeeEdit) payeeEdit->setText(((Expense*) trans)->payee());
		} else if(transtype == TRANSACTION_TYPE_INCOME) {
			categoryCombo->setCurrentAccount(((Income*) trans)->category());
			if(payeeEdit) payeeEdit->setText(((Income*) trans)->payer());
		}
	} else {
		descriptionEdit->clear();
		dateEdit->setDate(QDate::currentDate());
		if(valueEdit) {
			valueEdit->setValue(0.0);
			valueEdit->setCurrency(budget->defaultCurrency());
		}
		if(payeeEdit) payeeEdit->clear();
	}
}
void MultipleTransactionsEditDialog::setTransaction(Transaction *trans, const QDate &date) {
	setTransaction(trans);
	dateEdit->setDate(date);
}
void MultipleTransactionsEditDialog::updateAccounts() {
	if(!categoryCombo) return;
	categoryCombo->updateAccounts();
}
bool MultipleTransactionsEditDialog::modifyTransaction(Transaction *trans, bool change_parent) {
	if(!validValues()) return false;
	bool b_descr = true, b_value = true, b_payee = true, b_category = true, b_date = true;
	if(trans->parentSplit()) {
		switch(trans->parentSplit()->type()) {
			case SPLIT_TRANSACTION_TYPE_MULTIPLE_ITEMS: {
				b_date = false;
				if(change_parent) {
					MultiItemTransaction *split = (MultiItemTransaction*) trans->parentSplit();
					if(dateButton->isChecked()) split->setDate(dateEdit->date());
					if(payeeEdit && payeeButton->isChecked()) split->setPayee(payeeEdit->text());
				}
				break;
			}
			case SPLIT_TRANSACTION_TYPE_MULTIPLE_ACCOUNTS: {
				b_descr = false;
				b_category = false;
				if(change_parent) {
					MultiAccountTransaction *split = (MultiAccountTransaction*) trans->parentSplit();
					if(dateButton->isChecked()) split->setDate(dateEdit->date());
					if(descriptionButton->isChecked()) split->setDescription(descriptionEdit->text());
					if(categoryButton && categoryButton->isChecked()) split->setCategory((CategoryAccount*) categoryCombo->currentAccount());
				}
				break;
			}
			case SPLIT_TRANSACTION_TYPE_LOAN: {
				b_payee = false;
				b_category = false;
				b_date = false;
				b_descr = false;
				if(change_parent) {
					DebtPayment *split = (DebtPayment*) trans->parentSplit();
					if(dateButton->isChecked()) split->setDate(dateEdit->date());
					if(categoryButton && categoryButton->isChecked()) split->setExpenseCategory((ExpensesAccount*) categoryCombo->currentAccount());
				}
				break;
			}
		}
	}
	switch(trans->type()) {
		case TRANSACTION_TYPE_EXPENSE: {
			if(b_category && categoryButton && transtype == trans->type() && categoryButton->isChecked()) ((Expense*) trans)->setCategory((ExpensesAccount*) categoryCombo->currentAccount());
			if(b_payee && payeeEdit && payeeButton->isChecked()) ((Expense*) trans)->setPayee(payeeEdit->text());
			break;
		}
		case TRANSACTION_TYPE_INCOME: {
			if(((Income*) trans)->security()) b_descr = false;
			else if(b_payee && payeeEdit && payeeButton->isChecked()) ((Income*) trans)->setPayer(payeeEdit->text());
			if(b_category && categoryButton && transtype == trans->type() && categoryButton->isChecked()) ((Income*) trans)->setCategory((IncomesAccount*) categoryCombo->currentAccount());
			break;
		}
		case TRANSACTION_TYPE_TRANSFER: {
			break;
		}
		case TRANSACTION_TYPE_SECURITY_BUY: {
			if(b_category && categoryButton && transtype == TRANSACTION_TYPE_INCOME && categoryButton->isChecked()) ((SecurityTransaction*) trans)->setAccount(categoryCombo->currentAccount());
			b_descr = false;
			b_value = false;
			break;
		}
		case TRANSACTION_TYPE_SECURITY_SELL: {
			if(b_category && categoryButton && transtype == TRANSACTION_TYPE_INCOME && categoryButton->isChecked()) ((SecurityTransaction*) trans)->setAccount(categoryCombo->currentAccount());
			b_descr = false;
			b_value = false;
			break;
		}
	}
	if(b_descr && descriptionButton->isChecked()) trans->setDescription(descriptionEdit->text());
	if(valueEdit && b_value && valueButton->isChecked()) trans->setValue(valueEdit->value());
	if(b_date && dateButton->isChecked()) trans->setDate(dateEdit->date());
	trans->setModified();
	return true;
}
bool MultipleTransactionsEditDialog::modifySplitTransaction(SplitTransaction *trans) {
	if(!validValues()) return false;
	if(!dateButton->isChecked() && (!payeeEdit || !payeeButton->isChecked())) return false;
	switch(trans->type()) {
		case SPLIT_TRANSACTION_TYPE_MULTIPLE_ITEMS: {
			MultiItemTransaction *split = (MultiItemTransaction*) trans;
			if(dateButton->isChecked()) split->setDate(dateEdit->date());
			if(payeeEdit && payeeButton->isChecked()) split->setPayee(payeeEdit->text());
			trans->setModified();
			return true;
		}
		case SPLIT_TRANSACTION_TYPE_MULTIPLE_ACCOUNTS: {
			break;
		}
		case SPLIT_TRANSACTION_TYPE_LOAN: {
			DebtPayment *split = (DebtPayment*) trans;
			if(dateButton->isChecked()) {
				split->setDate(dateEdit->date());
				trans->setModified();
				return true;
			}
			break;
		}
	}
	return false;
}

bool MultipleTransactionsEditDialog::checkAccounts() {
	if(!categoryCombo) return true;
	switch(transtype) {
		case TRANSACTION_TYPE_INCOME: {			
			if(!categoryButton->isChecked()) return true;
			if(!categoryCombo->hasAccount()) {
				QMessageBox::critical(this, tr("Error"), tr("No income category available."));
				return false;
			}
			break;
		}
		case TRANSACTION_TYPE_EXPENSE: {
			if(!categoryButton->isChecked()) return true;
			if(!categoryCombo->hasAccount()) {
				QMessageBox::critical(this, tr("Error"), tr("No expense category available."));
				return false;
			}
			break;
		}
		default: {}
	}
	return true;
}
bool MultipleTransactionsEditDialog::validValues() {
	if(dateButton->isChecked() && !dateEdit->date().isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		dateEdit->setFocus();
		dateEdit->selectAll();
		return false;
	}
	if(!checkAccounts()) return false;
	if(categoryCombo && !categoryCombo->currentAccount()) return false;
	return true;
}
void MultipleTransactionsEditDialog::accept() {
	if(!descriptionButton->isChecked() && (!valueButton || !valueButton->isChecked()) && !dateButton->isChecked() && (!categoryButton || !categoryButton->isChecked()) && (!payeeButton || !payeeButton->isChecked())) {
		reject();
	} else if(validValues()) {
		QDialog::accept();
	}
}
QDate MultipleTransactionsEditDialog::date() {
	if(!dateButton->isChecked()) return QDate();
	return dateEdit->date();
}

