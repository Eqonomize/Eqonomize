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

#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QObject>
#include <QPushButton>
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

#include "budget.h"
#include "editaccountdialogs.h"
#include "eqonomizevalueedit.h"
#include "recurrence.h"
#include "transactioneditwidget.h"

#include <cmath>

#define TEROWCOL(row, col)	row % rows, ((row / rows) * 2) + col

EqonomizeDateEdit::EqonomizeDateEdit(QWidget *parent) : QDateEdit(QDate::currentDate(), parent) {}
void EqonomizeDateEdit::keyPressEvent(QKeyEvent *event) {
	QDateEdit::keyPressEvent(event);
	if(event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
		emit returnPressed();
	}
}


TransactionEditWidget::TransactionEditWidget(bool auto_edit, bool extra_parameters, int transaction_type, bool split, bool transfer_to, Security *sec, SecurityValueDefineType security_value_type, bool select_security, Budget *budg, QWidget *parent, bool allow_account_creation) : QWidget(parent), transtype(transaction_type), budget(budg), security(sec), b_autoedit(auto_edit), b_extra(extra_parameters), b_create_accounts(allow_account_creation) {
	value_set = false; shares_set = false; sharevalue_set = false;
	b_sec = (transtype == TRANSACTION_TYPE_SECURITY_BUY || transtype == TRANSACTION_TYPE_SECURITY_SELL);
	QVBoxLayout *editVLayout = new QVBoxLayout(this);
	int cols = 1;
	if(auto_edit) cols = 2;
	int rows = 6;
	if(b_sec && security_value_type == SECURITY_ALL_VALUES) rows = 7;
	else if(b_extra && !security && !select_security && (transtype == TRANSACTION_TYPE_EXPENSE || transtype == TRANSACTION_TYPE_INCOME)) rows = 8;
	if(rows % cols > 0) rows += rows / cols + 1;
	else rows = rows / cols;
	QGridLayout *editLayout = new QGridLayout();
	editVLayout->addLayout(editLayout);
	dateEdit = NULL;
	sharesEdit = NULL;
	quotationEdit = NULL;
	valueEdit = NULL;
	descriptionEdit = NULL;
	payeeEdit = NULL;
	quantityEdit = NULL;
	toCombo = NULL;
	fromCombo = NULL;
	securityCombo = NULL;
	added_from_account = NULL;
	added_to_account = NULL;
	int i = 0;
	if(b_sec) {
		int decimals = budget->defaultShareDecimals();
		editLayout->addWidget(new QLabel(tr("Security:"), this), TEROWCOL(i, 0));
		if(select_security) {
			securityCombo = new QComboBox(this);
			securityCombo->setEditable(false);
			Security *c_sec = budget->securities.first();
			int i2 = 0;
			while(c_sec) {
				securityCombo->addItem(c_sec->name());
				if(c_sec == security) securityCombo->setCurrentIndex(i2);
				c_sec = budget->securities.next();
				i2++;
			}
			editLayout->addWidget(securityCombo, TEROWCOL(i, 1));
			if(!security) security = selectedSecurity();
		} else {
			editLayout->addWidget(new QLabel(security->name(), this), TEROWCOL(i, 1));
		}
		if(security) decimals = security->decimals();
		i++;
		if(security_value_type != SECURITY_SHARES_AND_QUOTATION) {
			if(transtype == TRANSACTION_TYPE_SECURITY_BUY) editLayout->addWidget(new QLabel(tr("Cost:"), this), TEROWCOL(i, 0));
			else editLayout->addWidget(new QLabel(tr("Income:"), this), TEROWCOL(i, 0));
			valueEdit = new EqonomizeValueEdit(false, this);
			editLayout->addWidget(valueEdit, TEROWCOL(i, 1));
			i++;
		}
		if(security_value_type != SECURITY_VALUE_AND_QUOTATION) {
			if(transtype == TRANSACTION_TYPE_SECURITY_BUY) {
				editLayout->addWidget(new QLabel(tr("Shares bought:"), this), TEROWCOL(i, 0));
				sharesEdit = new EqonomizeValueEdit(0.0, decimals, false, false, this);
				editLayout->addWidget(sharesEdit, TEROWCOL(i, 1));
				i++;
			} else {
				editLayout->addWidget(new QLabel(tr("Shares sold:"), this), TEROWCOL(i, 0));
				QHBoxLayout *sharesLayout = new QHBoxLayout();
				sharesEdit = new EqonomizeValueEdit(0.0, decimals, false, false, this);
				sharesEdit->setSizePolicy(QSizePolicy::Expanding, sharesEdit->sizePolicy().verticalPolicy());
				sharesLayout->addWidget(sharesEdit);
				maxSharesButton = new QPushButton(tr("All"), this);
				maxSharesButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
				sharesLayout->addWidget(maxSharesButton);
				editLayout->addLayout(sharesLayout, TEROWCOL(i, 1));
				i++;
			}
		}
		if(security_value_type != SECURITY_VALUE_AND_SHARES) {
			editLayout->addWidget(new QLabel(tr("Price per share:"), this), TEROWCOL(i, 0));
			quotationEdit = new EqonomizeValueEdit(0.0, security ? security->quotationDecimals() : budget->defaultQuotationDecimals(), false, true, this);
			editLayout->addWidget(quotationEdit, TEROWCOL(i, 1));
			i++;
		}
		if(!split) {
			editLayout->addWidget(new QLabel(tr("Date:"), this), TEROWCOL(i, 0));
			dateEdit = new EqonomizeDateEdit(this);
			dateEdit->setCalendarPopup(true);
			editLayout->addWidget(dateEdit, TEROWCOL(i, 1));
		}
		i++;
	} else {
		if(transtype == TRANSACTION_TYPE_INCOME && (security || select_security)) {
			editLayout->addWidget(new QLabel(tr("Security:"), this), TEROWCOL(i, 0));
			if(select_security) {
				securityCombo = new QComboBox(this);
				securityCombo->setEditable(false);
				Security *c_sec = budget->securities.first();
				int i2 = 0;
				while(c_sec) {
					securityCombo->addItem(c_sec->name());
					if(c_sec == security) securityCombo->setCurrentIndex(i2);
					c_sec = budget->securities.next();
					i2++;
				}
				editLayout->addWidget(securityCombo, TEROWCOL(i, 1));
				if(!security) security = selectedSecurity();
			} else {
				editLayout->addWidget(new QLabel(security->name(), this), TEROWCOL(i, 1));
			}
		} else {
			editLayout->addWidget(new QLabel(tr("Generic Description:"), this), TEROWCOL(i, 0));
			descriptionEdit = new QLineEdit(this);
			descriptionEdit->setCompleter(new QCompleter(this));
			descriptionEdit->completer()->setModel(new QStandardItemModel(this));
			descriptionEdit->completer()->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
			editLayout->addWidget(descriptionEdit, TEROWCOL(i, 1));
		}
		i++;
		if(transtype == TRANSACTION_TYPE_TRANSFER) {
			editLayout->addWidget(new QLabel(tr("Amount:"), this), TEROWCOL(i, 0));
		} else if(transtype == TRANSACTION_TYPE_INCOME) {
			editLayout->addWidget(new QLabel(tr("Income:"), this), TEROWCOL(i, 0));
		} else {
			editLayout->addWidget(new QLabel(tr("Cost:"), this), TEROWCOL(i, 0));

		}
		valueEdit = new EqonomizeValueEdit(true, this);
		editLayout->addWidget(valueEdit, TEROWCOL(i, 1));
		i++;
		if(b_extra && !select_security && !security && (transtype == TRANSACTION_TYPE_EXPENSE || transtype == TRANSACTION_TYPE_INCOME)) {
			editLayout->addWidget(new QLabel(tr("Quantity:"), this), TEROWCOL(i, 0));
			quantityEdit = new EqonomizeValueEdit(1.0, 2, true, false, this);
			editLayout->addWidget(quantityEdit, TEROWCOL(i, 1));
			i++;
		}
		if(!split) {
			editLayout->addWidget(new QLabel(tr("Date:"), this), TEROWCOL(i, 0));
			dateEdit = new EqonomizeDateEdit(this);
			dateEdit->setCalendarPopup(true);
			editLayout->addWidget(dateEdit, TEROWCOL(i, 1));
			i++;
		}
	}
	switch(transtype) {
		case TRANSACTION_TYPE_TRANSFER: {
			if(!split || transfer_to) {
				editLayout->addWidget(new QLabel(tr("From:"), this), TEROWCOL(i, 0));
				fromCombo = new QComboBox(this);
				fromCombo->setEditable(false);
				editLayout->addWidget(fromCombo, TEROWCOL(i, 1));
				i++;
			}
			if(!split || !transfer_to) {
				editLayout->addWidget(new QLabel(tr("To:"), this), TEROWCOL(i, 0));
				toCombo = new QComboBox(this);
				toCombo->setEditable(false);
				editLayout->addWidget(toCombo, TEROWCOL(i, 1));
				i++;
			}
			break;
		}
		case TRANSACTION_TYPE_INCOME: {
			editLayout->addWidget(new QLabel(tr("Category:"), this), TEROWCOL(i, 0));
			fromCombo = new QComboBox(this);
			fromCombo->setEditable(false);
			editLayout->addWidget(fromCombo, TEROWCOL(i, 1));
			i++;
			if(!split) {
				editLayout->addWidget(new QLabel(tr("To account:"), this), TEROWCOL(i, 0));
				toCombo = new QComboBox(this);
				toCombo->setEditable(false);
				editLayout->addWidget(toCombo, TEROWCOL(i, 1));
				i++;
			}
			if(b_extra && !select_security && !security) {
				editLayout->addWidget(new QLabel(tr("Payer:"), this), TEROWCOL(i, 0));
				payeeEdit = new QLineEdit(this);
				editLayout->addWidget(payeeEdit, TEROWCOL(i, 1));
				i++;
			}
			break;
		}
		case TRANSACTION_TYPE_SECURITY_BUY: {
			if(!split) {
				editLayout->addWidget(new QLabel(tr("From account:"), this), TEROWCOL(i, 0));
				fromCombo = new QComboBox(this);
				fromCombo->setEditable(false);
				editLayout->addWidget(fromCombo, TEROWCOL(i, 1));
				i++;
			}
			break;
		}
		case TRANSACTION_TYPE_SECURITY_SELL: {
			if(!split) {
				editLayout->addWidget(new QLabel(tr("To account:"), this), TEROWCOL(i, 0));
				toCombo = new QComboBox(this);
				toCombo->setEditable(false);
				editLayout->addWidget(toCombo, TEROWCOL(i, 1));
				i++;
			}
			break;
		}
		default: {
			editLayout->addWidget(new QLabel(tr("Category:"), this), TEROWCOL(i, 0));
			toCombo = new QComboBox(this);
			toCombo->setEditable(false);
			editLayout->addWidget(toCombo, TEROWCOL(i, 1));
			i++;
			if(!split) {
				editLayout->addWidget(new QLabel(tr("From account:"), this), TEROWCOL(i, 0));
				fromCombo = new QComboBox(this);
				fromCombo->setEditable(false);
				editLayout->addWidget(fromCombo, TEROWCOL(i, 1));
				i++;
			}
			if(b_extra) {
				editLayout->addWidget(new QLabel(tr("Payee:"), this), TEROWCOL(i, 0));
				payeeEdit = new QLineEdit(this);
				editLayout->addWidget(payeeEdit, TEROWCOL(i, 1));
				i++;
			}
		}
	}
	editLayout->addWidget(new QLabel(tr("Comments:"), this), TEROWCOL(i, 0));
	commentsEdit = new QLineEdit(this);
	editLayout->addWidget(commentsEdit, TEROWCOL(i, 1));
	i++;
	bottom_layout = new QHBoxLayout();
	editVLayout->addLayout(bottom_layout);
	editVLayout->addStretch(1);

	description_changed = false;
	if(descriptionEdit) {
		descriptionEdit->completer()->setCaseSensitivity(Qt::CaseInsensitive);
	}
	if(payeeEdit) {
		payeeEdit->setCompleter(new QCompleter(this));
		payeeEdit->completer()->setModel(new QStandardItemModel(this));
		payeeEdit->completer()->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
		payeeEdit->completer()->setCaseSensitivity(Qt::CaseInsensitive);
	}

	if(dateEdit) connect(dateEdit, SIGNAL(dateChanged(const QDate&)), this, SIGNAL(dateChanged(const QDate&)));
	if(b_sec) {
		switch(security_value_type) {
			case SECURITY_ALL_VALUES: {
				connect(valueEdit, SIGNAL(returnPressed()), sharesEdit, SLOT(setFocus()));
				connect(valueEdit, SIGNAL(returnPressed()), sharesEdit, SLOT(selectAll()));
				connect(sharesEdit, SIGNAL(returnPressed()), quotationEdit, SLOT(setFocus()));
				connect(sharesEdit, SIGNAL(returnPressed()), quotationEdit, SLOT(selectAll()));
				if(dateEdit) {
					connect(quotationEdit, SIGNAL(returnPressed()), this, SLOT(focusDate()));
				}
				if(transtype == TRANSACTION_TYPE_SECURITY_SELL) connect(maxSharesButton, SIGNAL(clicked()), this, SLOT(maxShares()));
				break;
			}
			case SECURITY_SHARES_AND_QUOTATION: {
				connect(sharesEdit, SIGNAL(returnPressed()), quotationEdit, SLOT(setFocus()));
				connect(sharesEdit, SIGNAL(returnPressed()), quotationEdit, SLOT(selectAll()));
				if(dateEdit) {
					connect(quotationEdit, SIGNAL(returnPressed()), this, SLOT(focusDate()));
				}
				if(transtype == TRANSACTION_TYPE_SECURITY_SELL) connect(maxSharesButton, SIGNAL(clicked()), this, SLOT(maxShares()));
				break;
			}
			case SECURITY_VALUE_AND_SHARES: {
				connect(valueEdit, SIGNAL(returnPressed()), sharesEdit, SLOT(setFocus()));
				connect(valueEdit, SIGNAL(returnPressed()), sharesEdit, SLOT(selectAll()));
				if(dateEdit) {
					connect(sharesEdit, SIGNAL(returnPressed()), this, SLOT(focusDate()));
				}
				if(transtype == TRANSACTION_TYPE_SECURITY_SELL) connect(maxSharesButton, SIGNAL(clicked()), this, SLOT(maxShares()));
				break;
			}
			case SECURITY_VALUE_AND_QUOTATION: {
				connect(valueEdit, SIGNAL(returnPressed()), quotationEdit, SLOT(setFocus()));
				connect(valueEdit, SIGNAL(returnPressed()), quotationEdit, SLOT(selectAll()));
				if(dateEdit) {
					connect(quotationEdit, SIGNAL(returnPressed()), this, SLOT(focusDate()));
				}
				break;
			}
		}
		if(valueEdit) connect(valueEdit, SIGNAL(valueChanged(double)), this, SLOT(valueChanged(double)));
		if(sharesEdit) connect(sharesEdit, SIGNAL(valueChanged(double)), this, SLOT(sharesChanged(double)));
		if(quotationEdit) connect(quotationEdit, SIGNAL(valueChanged(double)), this, SLOT(quotationChanged(double)));
	} else {
		if(quantityEdit) {
			connect(valueEdit, SIGNAL(returnPressed()), quantityEdit, SLOT(setFocus()));
			connect(valueEdit, SIGNAL(returnPressed()), quantityEdit, SLOT(selectAll()));
			if(dateEdit) {
				connect(quantityEdit, SIGNAL(returnPressed()), this, SLOT(focusDate()));
			}
		} else  {
			if(dateEdit) {
				connect(valueEdit, SIGNAL(returnPressed()), this, SLOT(focusDate()));
			}
		}
		if(descriptionEdit) {
			connect(descriptionEdit, SIGNAL(returnPressed()), valueEdit, SLOT(setFocus()));
			connect(descriptionEdit, SIGNAL(returnPressed()), valueEdit, SLOT(selectAll()));
			connect(descriptionEdit, SIGNAL(editingFinished()), this, SLOT(setDefaultValue()));
			connect(descriptionEdit, SIGNAL(textChanged(const QString&)), this, SLOT(descriptionChanged(const QString&)));
		}
	}
	if(b_autoedit && dateEdit) connect(dateEdit, SIGNAL(returnPressed()), this, SIGNAL(addmodify()));
	if(payeeEdit) connect(payeeEdit, SIGNAL(returnPressed()), commentsEdit, SLOT(setFocus()));
	if(b_autoedit) connect(commentsEdit, SIGNAL(returnPressed()), this, SIGNAL(addmodify()));
	if(securityCombo) connect(securityCombo, SIGNAL(activated(int)), this, SLOT(securityChanged()));
	if(fromCombo && b_create_accounts) connect(fromCombo, SIGNAL(activated(int)), this, SLOT(fromActivated(int)));
	if(toCombo && b_create_accounts) connect(toCombo, SIGNAL(activated(int)), this, SLOT(toActivated(int)));
}
void TransactionEditWidget::fromActivated(int index) {
	bool move_focus = true;;
	if(index == fromCombo->count() - 1) {
		fromCombo->setCurrentIndex(froms.count() > 0 ? 0 : -1);
		move_focus = false;
		switch(transtype) {
			case TRANSACTION_TYPE_SECURITY_BUY: {}
			case TRANSACTION_TYPE_TRANSFER: {}
			case TRANSACTION_TYPE_EXPENSE: {
				EditAssetsAccountDialog *dialog = new EditAssetsAccountDialog(budget, this, tr("New Account"));
				if(dialog->exec() == QDialog::Accepted) {
					AssetsAccount *account = dialog->newAccount();
					budget->addAccount(account);
					added_from_account = account;
					updateAccounts();
					emit accountAdded(account);
					move_focus = true;
				}
				dialog->deleteLater();
				break;
			}
			case TRANSACTION_TYPE_INCOME: {
				EditIncomesAccountDialog *dialog = new EditIncomesAccountDialog(budget, NULL, this, tr("New Income Category"));
				if(dialog->exec() == QDialog::Accepted) {
					IncomesAccount *account = dialog->newAccount();
					budget->addAccount(account);
					added_from_account = account;
					updateAccounts();
					emit accountAdded(account);
					move_focus = true;
				}
				dialog->deleteLater();
				break;
			}
			case TRANSACTION_TYPE_SECURITY_SELL: {break;}
		}
	}
	if(move_focus) {
		if(toCombo && transtype != TRANSACTION_TYPE_INCOME) toCombo->setFocus();
		else if(payeeEdit) payeeEdit->setFocus();
		else if(commentsEdit) commentsEdit->setFocus();
	}
}

void TransactionEditWidget::toActivated(int index) {
	bool move_focus = true;
	if(index == toCombo->count() - 1) {
		toCombo->setCurrentIndex(tos.count() > 0 ? 0 : -1);
		move_focus = false;
		switch(transtype) {
			case TRANSACTION_TYPE_SECURITY_SELL: {}
			case TRANSACTION_TYPE_TRANSFER: {}
			case TRANSACTION_TYPE_INCOME: {
				EditAssetsAccountDialog *dialog = new EditAssetsAccountDialog(budget, this, tr("New Account"));
				if(dialog->exec() == QDialog::Accepted) {
					AssetsAccount *account = dialog->newAccount();
					budget->addAccount(account);
					added_to_account = account;
					updateAccounts();
					emit accountAdded(account);
					move_focus = true;
				}
				dialog->deleteLater();
				break;
			}
			case TRANSACTION_TYPE_EXPENSE: {
				EditExpensesAccountDialog *dialog = new EditExpensesAccountDialog(budget, NULL, this, tr("New Expense Category"));
				if(dialog->exec() == QDialog::Accepted) {
					ExpensesAccount *account = dialog->newAccount();
					budget->addAccount(account);
					added_to_account = account;
					updateAccounts();
					emit accountAdded(account);
					move_focus = true;
				}
				dialog->deleteLater();
				break;
			}
			case TRANSACTION_TYPE_SECURITY_BUY: {break;}
		}
	}
	if(move_focus) {
		if(fromCombo && transtype && transtype == TRANSACTION_TYPE_INCOME) fromCombo->setFocus();
		else if(payeeEdit) payeeEdit->setFocus();
		else if(commentsEdit) commentsEdit->setFocus();
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
void TransactionEditWidget::securityChanged() {
	security = selectedSecurity();
	if(security) {
		if(sharesEdit) sharesEdit->setPrecision(security->decimals());
		if(quotationEdit) quotationEdit->setPrecision(security->quotationDecimals());
		if(sharesEdit && security && shares_date.isValid()) sharesEdit->setMaximum(security->shares(shares_date));
	}
}
void TransactionEditWidget::valueChanged(double value) {
	if(!quotationEdit || !sharesEdit || !valueEdit) return;
	value_set = true;
	if(shares_set && !sharevalue_set) {
		quotationEdit->blockSignals(true);
		quotationEdit->setValue(value / sharesEdit->value());
		quotationEdit->blockSignals(false);
	} else if(!shares_set && sharevalue_set) {
		sharesEdit->blockSignals(true);
		sharesEdit->setValue(value / quotationEdit->value());
		sharesEdit->blockSignals(false);
	}
}
void TransactionEditWidget::sharesChanged(double value) {
	if(!quotationEdit || !sharesEdit || !valueEdit) return;
	shares_set = true;
	if(value_set && !sharevalue_set) {
		quotationEdit->blockSignals(true);
		quotationEdit->setValue(valueEdit->value() / value);
		quotationEdit->blockSignals(false);
	} else if(!value_set && sharevalue_set) {
		valueEdit->blockSignals(true);
		valueEdit->setValue(value * quotationEdit->value());
		valueEdit->blockSignals(false);
	}
}
void TransactionEditWidget::quotationChanged(double value) {
	if(!quotationEdit || !sharesEdit || !valueEdit) return;
	sharevalue_set = true;
	if(value_set && !shares_set) {
		sharesEdit->blockSignals(true);
		sharesEdit->setValue(valueEdit->value() / value);
		sharesEdit->blockSignals(false);
	} else if(!value_set && shares_set) {
		valueEdit->blockSignals(true);
		valueEdit->setValue(value * sharesEdit->value());
		valueEdit->blockSignals(false);
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
void TransactionEditWidget::focusDescription() {
	if(!descriptionEdit) {
		if(valueEdit) valueEdit->setFocus();
		else sharesEdit->setFocus();
	} else {
		descriptionEdit->setFocus();
	}
}
QDate TransactionEditWidget::date() {
	if(!dateEdit) return QDate();
	return dateEdit->date();
}
void TransactionEditWidget::descriptionChanged(const QString&) {
	description_changed = true;
}
void TransactionEditWidget::setDefaultValue() {
	if(descriptionEdit && description_changed && !descriptionEdit->text().isEmpty() && valueEdit && valueEdit->value() == 0.0) {
		Transaction *trans = NULL;
		if(default_values.contains(descriptionEdit->text().toLower())) trans = default_values[descriptionEdit->text().toLower()];
		if(trans) {
			valueEdit->setValue(trans->value());
			if(toCombo) {
				for(QVector<Account*>::size_type i = 0; i < tos.size(); i++) {
					if(trans->toAccount() == tos[i]) {
						toCombo->setCurrentIndex(i);
						break;
					}
				}
			}
			if(fromCombo) {
				for(QVector<Account*>::size_type i = 0; i < froms.size(); i++) {
					if(trans->fromAccount() == froms[i]) {
						fromCombo->setCurrentIndex(i);
						break;
					}
				}
			}
			if(quantityEdit) {
				if(trans->quantity() <= 0.0) quantityEdit->setValue(1.0);
				else quantityEdit->setValue(trans->quantity());
			}
			if(payeeEdit && trans->type() == TRANSACTION_TYPE_EXPENSE) payeeEdit->setText(((Expense*) trans)->payee());
			if(payeeEdit && trans->type() == TRANSACTION_TYPE_INCOME) payeeEdit->setText(((Income*) trans)->payer());
		}
	}
}
void TransactionEditWidget::updateFromAccounts(Account *exclude_account) {
	if(!fromCombo) return;
	Account *current_account = NULL;
	if(added_from_account) {
		current_account = added_from_account;
		added_from_account = NULL;
	} else if(fromCombo->currentIndex() < froms.size()) {
		current_account = froms[fromCombo->currentIndex()];
	}
	fromCombo->clear();
	froms.clear();
	Account *account;
	int current_index = -1;
	switch(transtype) {
		case TRANSACTION_TYPE_TRANSFER: {
			AssetsAccount *account = budget->assetsAccounts.first();
			while(account) {
				if(account != exclude_account && (b_autoedit || ((AssetsAccount*) account)->accountType() != ASSETS_TYPE_SECURITIES)) {
					fromCombo->addItem(account->name());
					froms.push_back(account);
					if(account == current_account) current_index = froms.count() - 1;
				}
				account = budget->assetsAccounts.next();
			}
			if(b_create_accounts) {
				fromCombo->insertSeparator(fromCombo->count());
				fromCombo->addItem(tr("New Account…"));
			}
			if(current_index >= 0) {
				fromCombo->setCurrentIndex(current_index);
			} else {
				for(QVector<Account*>::size_type i = 0; i < froms.size(); i++) {
					if(froms[i] != budget->balancingAccount && ((AssetsAccount*) froms[i])->accountType() != ASSETS_TYPE_SECURITIES) {
						fromCombo->setCurrentIndex((int) i);
						break;
					}
				}
			}
			break;
		}
		case TRANSACTION_TYPE_EXPENSE: {
			account = budget->assetsAccounts.first();
			while(account) {
				if(account != exclude_account && account != budget->balancingAccount && ((AssetsAccount*) account)->accountType() != ASSETS_TYPE_SECURITIES) {
					fromCombo->addItem(account->name());
					froms.push_back(account);
					if(account == current_account) current_index = froms.count() - 1;
				}
				account = budget->assetsAccounts.next();
			}
			if(b_create_accounts) {
				fromCombo->insertSeparator(fromCombo->count());
				fromCombo->addItem(tr("New Account…"));
			}
			if(current_index >= 0) {
				fromCombo->setCurrentIndex(current_index);
			} else {
				for(QVector<Account*>::size_type i = 0; i < froms.size(); i++) {
					if(froms[i] != budget->balancingAccount && ((AssetsAccount*) froms[i])->accountType() != ASSETS_TYPE_SECURITIES) {
						fromCombo->setCurrentIndex((int) i);
						break;
					}
				}
			}
			break;
		}
		case TRANSACTION_TYPE_INCOME: {
			account = budget->incomesAccounts.first();
			while(account) {
				if(account != exclude_account) {
					fromCombo->addItem(account->nameWithParent());
					froms.push_back(account);
					if(account == current_account) current_index = froms.count() - 1;
				}
				account = budget->incomesAccounts.next();
			}
			if(b_create_accounts) {
				fromCombo->insertSeparator(fromCombo->count());
				fromCombo->addItem(tr("New Income Category…"));
			}
			if(!b_create_accounts || fromCombo->count() > 2) fromCombo->setCurrentIndex(current_index >= 0 ? current_index : 0);
			break;
		}
		case TRANSACTION_TYPE_SECURITY_BUY: {
			account = budget->assetsAccounts.first();
			while(account) {
				if(account != exclude_account && account != budget->balancingAccount && ((AssetsAccount*) account)->accountType() != ASSETS_TYPE_SECURITIES) {
					fromCombo->addItem(account->name());
					froms.push_back(account);
					if(account == current_account) current_index = froms.count() - 1;
				}
				account = budget->assetsAccounts.next();
			}
			account = budget->incomesAccounts.first();
			while(account) {
				if(account != exclude_account) {
					fromCombo->addItem(account->name());
					froms.push_back(account);
					account = budget->incomesAccounts.next();
				}
			}
			if(b_create_accounts) {
				fromCombo->insertSeparator(fromCombo->count());
				fromCombo->addItem(tr("New Account…"));
			}
			if(!b_create_accounts || fromCombo->count() > 2) fromCombo->setCurrentIndex(current_index >= 0 ? current_index : 0);
			break;
		}
		case TRANSACTION_TYPE_SECURITY_SELL: {
			break;
		}
	}
}
void TransactionEditWidget::updateToAccounts(Account *exclude_account) {
	if(!toCombo) return;
	Account *current_account = NULL;
	if(added_to_account) {
		current_account = added_to_account;
		added_to_account = NULL;
	} else if(toCombo->currentIndex() < tos.size()) {
		current_account = tos[toCombo->currentIndex()];
	}
	toCombo->clear();
	tos.clear();
	Account *account;
	int current_index = -1;
	switch(transtype) {
		case TRANSACTION_TYPE_TRANSFER: {
			account = budget->assetsAccounts.first();
			while(account) {
				if(account != exclude_account && (b_autoedit || ((AssetsAccount*) account)->accountType() != ASSETS_TYPE_SECURITIES)) {
					toCombo->addItem(account->name());
					tos.push_back(account);
					if(account == current_account) current_index = tos.count() - 1;
				}
				account = budget->assetsAccounts.next();
			}
			if(b_create_accounts) {
				toCombo->insertSeparator(toCombo->count());
				toCombo->addItem(tr("New Account…"));
			}
			if(current_index >= 0) {
				toCombo->setCurrentIndex(current_index);
			} else {
				for(QVector<Account*>::size_type i = 0; i < tos.size(); i++) {
					if(tos[i] != budget->balancingAccount && ((AssetsAccount*) tos[i])->accountType() != ASSETS_TYPE_SECURITIES) {
						toCombo->setCurrentIndex((int) i);
						break;
					}
				}
			}
			break;
		}
		case TRANSACTION_TYPE_INCOME: {
			account = budget->assetsAccounts.first();
			while(account) {
				if(account != exclude_account && account != budget->balancingAccount && (b_autoedit || ((AssetsAccount*) account)->accountType() != ASSETS_TYPE_SECURITIES)) {
					toCombo->addItem(account->name());
					tos.push_back(account);
					if(account == current_account) current_index = tos.count() - 1;
				}
				account = budget->assetsAccounts.next();
			}
			if(b_create_accounts) {
				toCombo->insertSeparator(toCombo->count());
				toCombo->addItem(tr("New Account…"));
			}
			if(current_index >= 0) {
				toCombo->setCurrentIndex(current_index);
			} else {
				for(QVector<Account*>::size_type i = 0; i < tos.size(); i++) {
					if(tos[i] != budget->balancingAccount && ((AssetsAccount*) tos[i])->accountType() != ASSETS_TYPE_SECURITIES) {
						toCombo->setCurrentIndex((int) i);
						break;
					}
				}
			}
			break;
		}
		case TRANSACTION_TYPE_EXPENSE: {
			account = budget->expensesAccounts.first();
			while(account) {
				if(account != exclude_account) {
					toCombo->addItem(account->nameWithParent());
					tos.push_back(account);
					if(account == current_account) current_index = tos.count() - 1;
				}
				account = budget->expensesAccounts.next();
			}
			if(b_create_accounts) {
				toCombo->insertSeparator(toCombo->count());
				toCombo->addItem(tr("New Expense Category…"));
			}
			if(!b_create_accounts || toCombo->count() > 2) toCombo->setCurrentIndex(current_index >= 0 ? current_index : 0);
			break;
		}
		case TRANSACTION_TYPE_SECURITY_BUY: {
			break;
		}
		case TRANSACTION_TYPE_SECURITY_SELL: {
			account = budget->assetsAccounts.first();
			while(account) {
				if(account != exclude_account && account != budget->balancingAccount && ((AssetsAccount*) account)->accountType() != ASSETS_TYPE_SECURITIES) {
					toCombo->addItem(account->name());
					tos.push_back(account);
					if(account == current_account) current_index = tos.count() - 1;
				}
				account = budget->assetsAccounts.next();
			}
			if(b_create_accounts) {
				toCombo->insertSeparator(toCombo->count());
				toCombo->addItem(tr("New Account…"));
			}
			if(!b_create_accounts || toCombo->count() > 2) toCombo->setCurrentIndex(current_index >= 0 ? current_index : 0);
			break;
		}
	}	
}
void TransactionEditWidget::updateAccounts(Account *exclude_account) {
	updateFromAccounts(exclude_account);
	updateToAccounts(exclude_account);
}
void TransactionEditWidget::transactionAdded(Transaction *trans) {
	if(descriptionEdit && trans->type() == transtype && (transtype != TRANSACTION_TYPE_INCOME || !((Income*) trans)->security())) {
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
			if(((QStandardItemModel*) payeeEdit->completer()->model())->findItems(((Expense*) trans)->payee().toLower(), Qt::MatchExactly, 1).isEmpty()) {
				QList<QStandardItem*> row;
				row << new QStandardItem(((Expense*) trans)->payee());
				row << new QStandardItem(((Expense*) trans)->payee().toLower());
				((QStandardItemModel*) payeeEdit->completer()->model())->appendRow(row);
				((QStandardItemModel*) payeeEdit->completer()->model())->sort(1);
			}
		} else if(payeeEdit && transtype == TRANSACTION_TYPE_INCOME && !((Income*) trans)->security() && !((Income*) trans)->payer().isEmpty()) {
			if(((QStandardItemModel*) payeeEdit->completer()->model())->findItems(((Income*) trans)->payer().toLower(), Qt::MatchExactly, 1).isEmpty()) {
				QList<QStandardItem*> row;
				row << new QStandardItem(((Income*) trans)->payer());
				row << new QStandardItem(((Income*) trans)->payer().toLower());
				((QStandardItemModel*) payeeEdit->completer()->model())->appendRow(row);
				((QStandardItemModel*) payeeEdit->completer()->model())->sort(1);
			}
		}
	}
}
void TransactionEditWidget::transactionModified(Transaction *trans) {
	transactionAdded(trans);
}
bool TransactionEditWidget::checkAccounts() {
	switch(transtype) {
		case TRANSACTION_TYPE_TRANSFER: {
			if(fromCombo && froms.count() == 0) {
				QMessageBox::critical(this, tr("Error"), tr("No suitable account available."));
				return false;
			}
			if(toCombo && tos.count() == 0) {
				QMessageBox::critical(this, tr("Error"), tr("No suitable account available."));
				return false;
			}
			break;
		}
		case TRANSACTION_TYPE_INCOME: {
			if(fromCombo && froms.count() == 0) {
				QMessageBox::critical(this, tr("Error"), tr("No income category available."));
				return false;
			}
			if(toCombo && tos.count() == 0) {
				QMessageBox::critical(this, tr("Error"), tr("No suitable account available."));
				return false;
			}
			break;
		}
		case TRANSACTION_TYPE_SECURITY_BUY: {
			if(fromCombo && froms.count() == 0) {
				QMessageBox::critical(this, tr("Error"), tr("No suitable account or income category available."));
				return false;
			}
			break;
		}
		case TRANSACTION_TYPE_SECURITY_SELL: {
			if(toCombo && tos.count() == 0) {
				QMessageBox::critical(this, tr("Error"), tr("No suitable account available."));
				return false;
			}
			break;
		}
		default: {
			if(toCombo && tos.count() == 0) {
				QMessageBox::critical(this, tr("Error"), tr("No expense category available."));
				return false;
			}
			if(fromCombo && froms.count() == 0) {
				QMessageBox::critical(this, tr("Error"), tr("No suitable account available."));
				return false;
			}
			break;
		}
	}
	if(securityCombo && securityCombo->count() == 0) {
		QMessageBox::critical(this, tr("Error"), tr("No security available."));
		return false;
	}
	return true;
}
bool TransactionEditWidget::validValues(bool) {
	if(dateEdit && !dateEdit->date().isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		dateEdit->setFocus();
		dateEdit->selectAll();
		return false;
	}
	if(!checkAccounts()) return false;
	if((toCombo && toCombo->currentIndex() >= tos.count()) || (fromCombo && fromCombo->currentIndex() >= froms.count())) return false;
	if(toCombo && fromCombo && tos[toCombo->currentIndex()] == froms[fromCombo->currentIndex()]) {
		QMessageBox::critical(this, tr("Error"), tr("Cannot transfer money to and from the same account."));
		return false;
	}
	switch(transtype) {
		case TRANSACTION_TYPE_TRANSFER: {
			if((toCombo && tos[toCombo->currentIndex()]->type() == ACCOUNT_TYPE_ASSETS && ((AssetsAccount*) tos[toCombo->currentIndex()])->accountType() == ASSETS_TYPE_SECURITIES) || (fromCombo && froms[fromCombo->currentIndex()]->type() == ACCOUNT_TYPE_ASSETS && ((AssetsAccount*) froms[fromCombo->currentIndex()])->accountType() == ASSETS_TYPE_SECURITIES)) {
				QMessageBox::critical(this, tr("Error"), tr("Cannot create a regular transfer to/from a securities account."));
				return false;
			}
			break;
		}
		case TRANSACTION_TYPE_INCOME: {
			if(toCombo && tos[toCombo->currentIndex()]->type() == ACCOUNT_TYPE_ASSETS && ((AssetsAccount*) tos[toCombo->currentIndex()])->accountType() == ASSETS_TYPE_SECURITIES) {
				QMessageBox::critical(this, tr("Error"), tr("Cannot create a regular income to a securities account."));
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
			if(fromCombo && froms[fromCombo->currentIndex()]->type() == ACCOUNT_TYPE_ASSETS && ((AssetsAccount*) froms[fromCombo->currentIndex()])->accountType() == ASSETS_TYPE_SECURITIES) {
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
	bool b_transsec = (trans->type() == TRANSACTION_TYPE_SECURITY_BUY || trans->type() == TRANSACTION_TYPE_SECURITY_SELL);
	if(b_sec) {
		if(trans->type() != transtype) return false;
		if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY) {
			if(fromCombo) ((SecurityTransaction*) trans)->setAccount(froms[fromCombo->currentIndex()]);
		} else {
			if(toCombo) ((SecurityTransaction*) trans)->setAccount(tos[toCombo->currentIndex()]);
		}
		if(dateEdit) trans->setDate(dateEdit->date());
		double shares = 0.0, value = 0.0, share_value = 0.0;
		if(valueEdit) value = valueEdit->value();
		if(sharesEdit) shares = sharesEdit->value();
		if(quotationEdit) share_value = quotationEdit->value();
		if(!quotationEdit) share_value = value / shares;
		else if(!sharesEdit) shares = value / share_value;
		else if(!valueEdit) value = shares * share_value;
		((SecurityTransaction*) trans)->setValue(value);
		((SecurityTransaction*) trans)->setShares(shares);
		((SecurityTransaction*) trans)->setShareValue(share_value);
		trans->setComment(commentsEdit->text());
		return true;
	} else if(b_transsec) {
		return false;
	}
	if(dateEdit) trans->setDate(dateEdit->date());
	if(fromCombo) trans->setFromAccount(froms[fromCombo->currentIndex()]);
	if(toCombo) trans->setToAccount(tos[toCombo->currentIndex()]);
	trans->setValue(valueEdit->value());
	if(descriptionEdit && (trans->type() != TRANSACTION_TYPE_INCOME || !((Income*) trans)->security())) trans->setDescription(descriptionEdit->text());
	trans->setComment(commentsEdit->text());
	if(quantityEdit) trans->setQuantity(quantityEdit->value());
	if(payeeEdit && trans->type() == TRANSACTION_TYPE_EXPENSE) ((Expense*) trans)->setPayee(payeeEdit->text());
	if(payeeEdit && trans->type() == TRANSACTION_TYPE_INCOME) ((Income*) trans)->setPayer(payeeEdit->text());
	return true;
}
Security *TransactionEditWidget::selectedSecurity() {
	if(securityCombo) {
		int i = securityCombo->currentIndex();
		if(i >= 0) {
			Security *c_sec = budget->securities.first();
			while(i > 0 && c_sec) {
				c_sec = budget->securities.next();
				i--;
			}
			return c_sec;
		}
	}
	return security;
}
Transaction *TransactionEditWidget::createTransaction() {
	if(!validValues()) return NULL;
	Transaction *trans;
	if(transtype == TRANSACTION_TYPE_TRANSFER) {
		Transfer *transfer = new Transfer(budget, valueEdit->value(), dateEdit ? dateEdit->date() : QDate(), fromCombo ? (AssetsAccount*) froms[fromCombo->currentIndex()] : NULL, toCombo ? (AssetsAccount*) tos[toCombo->currentIndex()] : NULL, descriptionEdit->text(), commentsEdit->text());
		trans = transfer;
	} else if(transtype == TRANSACTION_TYPE_INCOME) {
		Income *income = new Income(budget, valueEdit->value(), dateEdit ? dateEdit->date() : QDate(), fromCombo ? (IncomesAccount*) froms[fromCombo->currentIndex()] : NULL, toCombo ? (AssetsAccount*) tos[toCombo->currentIndex()] : NULL, descriptionEdit ? descriptionEdit->text() : QString::null, commentsEdit->text());
		if(selectedSecurity()) income->setSecurity(selectedSecurity());
		if(quantityEdit) income->setQuantity(quantityEdit->value());
		if(payeeEdit) income->setPayer(payeeEdit->text());
		trans = income;
	} else if(transtype == TRANSACTION_TYPE_SECURITY_BUY) {
		if(!selectedSecurity()) return NULL;
		double shares = 0.0, value = 0.0, share_value = 0.0;
		if(valueEdit) value = valueEdit->value();
		if(sharesEdit) shares = sharesEdit->value();
		if(quotationEdit) share_value = quotationEdit->value();
		if(!quotationEdit) share_value = value / shares;
		else if(!sharesEdit) shares = value / share_value;
		else if(!valueEdit) value = shares * share_value;
		SecurityBuy *secbuy = new SecurityBuy(selectedSecurity(), value, shares, share_value, dateEdit ? dateEdit->date() : QDate(), fromCombo ? froms[fromCombo->currentIndex()] : NULL, commentsEdit->text());
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
		SecuritySell *secsell = new SecuritySell(selectedSecurity(), value, shares, share_value, dateEdit ? dateEdit->date() : QDate(), toCombo ? tos[toCombo->currentIndex()] : NULL, commentsEdit->text());
		trans = secsell;
	} else {
		Expense *expense = new Expense(budget, valueEdit->value(), dateEdit ? dateEdit->date() : QDate(), toCombo ? (ExpensesAccount*) tos[toCombo->currentIndex()] : NULL, fromCombo ? (AssetsAccount*) froms[fromCombo->currentIndex()] : NULL, descriptionEdit->text(), commentsEdit->text());
		if(quantityEdit) expense->setQuantity(quantityEdit->value());
		if(payeeEdit) expense->setPayee(payeeEdit->text());
		trans = expense;
	}
	return trans;
}
void TransactionEditWidget::transactionRemoved(Transaction *trans) {
	if(descriptionEdit && trans->type() == transtype && !trans->description().isEmpty() && default_values.contains(trans->description().toLower()) && default_values[trans->description().toLower()] == trans) {
		QString lower_description = trans->description().toLower();
		bool descr_found = false;
		switch(transtype) {
			case TRANSACTION_TYPE_EXPENSE: {
				Expense *expense = budget->expenses.last();
				while(expense) {
					if(expense != trans && expense->description().toLower() == lower_description) {
						default_values[expense->description().toLower()] = expense;
						descr_found = true;
						break;
					}
					expense = budget->expenses.previous();
				}
				break;
			}
			case TRANSACTION_TYPE_INCOME: {
				Income *income = budget->incomes.last();
				while(income) {
					if(income != trans && income->description() == lower_description) {
						default_values[income->description().toLower()] = income;
						descr_found = true;
						break;
					}
					income = budget->incomes.previous();
				}
				break;
			}
			case TRANSACTION_TYPE_TRANSFER: {
				Transfer *transfer= budget->transfers.last();
				while(transfer) {
					if(transfer != trans && transfer->description() == lower_description) {
						default_values[transfer->description().toLower()] = transfer;
						descr_found = true;
						break;
					}
					transfer = budget->transfers.previous();
				}
				break;
			}
			default: {}
		}
		if(!descr_found) default_values.remove(trans->description().toLower());
	}
}
void TransactionEditWidget::transactionsReset() {
	if(!descriptionEdit) return;
	((QStandardItemModel*) descriptionEdit->completer()->model())->clear();
	if(payeeEdit) ((QStandardItemModel*) payeeEdit->completer()->model())->clear();
	default_values.clear();
	QStringList payee_list;
	switch(transtype) {
		case TRANSACTION_TYPE_EXPENSE: {
			Expense *expense = budget->expenses.last();
			while(expense) {
				if(!expense->description().isEmpty() && !default_values.contains(expense->description().toLower())) {
					QList<QStandardItem*> row;
					row << new QStandardItem(expense->description());
					row << new QStandardItem(expense->description().toLower());
					((QStandardItemModel*) descriptionEdit->completer()->model())->appendRow(row);
					default_values[expense->description().toLower()] = expense;
				}
				if(payeeEdit && !expense->payee().isEmpty() && !payee_list.contains(expense->payee(), Qt::CaseInsensitive)) {
					QList<QStandardItem*> row;
					row << new QStandardItem(expense->payee());
					row << new QStandardItem(expense->payee().toLower());
					((QStandardItemModel*) payeeEdit->completer()->model())->appendRow(row);
					payee_list << expense->payee().toLower();
				}
				expense = budget->expenses.previous();
			}
			break;
		}
		case TRANSACTION_TYPE_INCOME: {
			Income *income = budget->incomes.last();
			while(income) {
				if(!income->security() && !income->description().isEmpty() && !default_values.contains(income->description().toLower())) {
					QList<QStandardItem*> row;
					row << new QStandardItem(income->description());
					row << new QStandardItem(income->description().toLower());
					((QStandardItemModel*) descriptionEdit->completer()->model())->appendRow(row);
					default_values[income->description().toLower()] = income;
				}
				if(payeeEdit && !income->security() && !income->payer().isEmpty() && !payee_list.contains(income->payer(), Qt::CaseInsensitive)) {
					QList<QStandardItem*> row;
					row << new QStandardItem(income->payer());
					row << new QStandardItem(income->payer().toLower());
					((QStandardItemModel*) payeeEdit->completer()->model())->appendRow(row);
					payee_list << income->payer().toLower();
				}
				income = budget->incomes.previous();
			}
			break;
		}
		case TRANSACTION_TYPE_TRANSFER: {
			Transfer *transfer= budget->transfers.last();
			while(transfer) {
				if(!transfer->description().isEmpty() && !default_values.contains(transfer->description().toLower())) {
					QList<QStandardItem*> row;
					row << new QStandardItem(transfer->description());
					row << new QStandardItem(transfer->description().toLower());
					((QStandardItemModel*) descriptionEdit->completer()->model())->appendRow(row);
					default_values[transfer->description().toLower()] = transfer;
				}
				transfer = budget->transfers.previous();
			}
			break;
		}
		default: {}
	}
	((QStandardItemModel*) descriptionEdit->completer()->model())->sort(1);
	if(payeeEdit) ((QStandardItemModel*) payeeEdit->completer()->model())->sort(1);
}
void TransactionEditWidget::setCurrentToItem(int index) {
	if(toCombo && index < tos.count()) toCombo->setCurrentIndex(index);
}
void TransactionEditWidget::setCurrentFromItem(int index) {
	if(fromCombo && index < froms.count()) fromCombo->setCurrentIndex(index);
}
void TransactionEditWidget::setAccount(Account *account) {
	if(fromCombo && (transtype == TRANSACTION_TYPE_EXPENSE || transtype == TRANSACTION_TYPE_TRANSFER || transtype == TRANSACTION_TYPE_SECURITY_BUY)) {
		for(QVector<Account*>::size_type i = 0; i < froms.size(); i++) {
			if(account == froms[i]) {
				fromCombo->setCurrentIndex(i);
				break;
			}
		}
	} else if(toCombo) {
		for(QVector<Account*>::size_type i = 0; i < tos.size(); i++) {
			if(account == tos[i]) {
				toCombo->setCurrentIndex(i);
				break;
			}
		}
	}
}
int TransactionEditWidget::currentToItem() {
	if(!toCombo) return -1;
	return toCombo->currentIndex();
}
int TransactionEditWidget::currentFromItem() {
	if(!fromCombo) return -1;
	return fromCombo->currentIndex();
}
void TransactionEditWidget::setTransaction(Transaction *trans) {
	if(valueEdit) valueEdit->blockSignals(true);
	if(sharesEdit) sharesEdit->blockSignals(true);
	if(quotationEdit) quotationEdit->blockSignals(true);
	if(trans == NULL) {
		value_set = false; shares_set = false; sharevalue_set = false;
		description_changed = false;
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
				descriptionEdit->setFocus();
			} else {
				valueEdit->setFocus();
			}
			valueEdit->setValue(0.0);
		}
		commentsEdit->clear();
		if(quantityEdit) quantityEdit->setValue(1.0);
		if(payeeEdit) payeeEdit->clear();
		if(dateEdit) emit dateChanged(trans->date());
	} else {
		value_set = true; shares_set = true; sharevalue_set = true;
		if(dateEdit) dateEdit->setDate(trans->date());
		commentsEdit->setText(trans->comment());
		if(toCombo && (!b_sec || transtype == TRANSACTION_TYPE_SECURITY_SELL)) {
			for(QVector<Account*>::size_type i = 0; i < tos.size(); i++) {
				if(trans->toAccount() == tos[i]) {
					toCombo->setCurrentIndex(i);
					break;
				}
			}
		}
		if(fromCombo && (!b_sec || transtype == TRANSACTION_TYPE_SECURITY_BUY)) {
			for(QVector<Account*>::size_type i = 0; i < froms.size(); i++) {
				if(trans->fromAccount() == froms[i]) {
					fromCombo->setCurrentIndex(i);
					break;
				}
			}
		}
		if(b_sec) {
			if(transtype != trans->type()) return;
			//if(transtype == TRANSACTION_TYPE_SECURITY_SELL) setMaxShares(((SecurityTransaction*) trans)->security()->shares(QDate::currentDate()) + ((SecurityTransaction*) trans)->shares());
			if(sharesEdit) sharesEdit->setValue(((SecurityTransaction*) trans)->shares());
			if(quotationEdit) quotationEdit->setValue(((SecurityTransaction*) trans)->shareValue());
			if(valueEdit) valueEdit->setValue(trans->value());
			if(valueEdit) valueEdit->setFocus();
			else sharesEdit->setFocus();
		} else {
			if(descriptionEdit) {
				descriptionEdit->setText(trans->description());
				description_changed = false;
				descriptionEdit->setFocus();
				descriptionEdit->selectAll();
			} else {
				valueEdit->setFocus();
			}
			valueEdit->setValue(trans->value());
			if(quantityEdit) quantityEdit->setValue(trans->quantity());
			if(payeeEdit && trans->type() == TRANSACTION_TYPE_EXPENSE) payeeEdit->setText(((Expense*) trans)->payee());
			if(payeeEdit && trans->type() == TRANSACTION_TYPE_INCOME) payeeEdit->setText(((Income*) trans)->payer());
		}
		if(dateEdit) emit dateChanged(trans->date());
	}
	if(valueEdit) valueEdit->blockSignals(false);
	if(sharesEdit) sharesEdit->blockSignals(false);
	if(quotationEdit) quotationEdit->blockSignals(false);
}
void TransactionEditWidget::setScheduledTransaction(ScheduledTransaction *strans, const QDate &date) {
	if(strans == NULL) {
		setTransaction(NULL);
	} else {
		setTransaction(strans->transaction());
		if(dateEdit) dateEdit->setDate(date);
	}
}

TransactionEditDialog::TransactionEditDialog(bool extra_parameters, int transaction_type, bool split, bool transfer_to, Security *security, SecurityValueDefineType security_value_type, bool select_security, Budget *budg, QWidget *parent, bool allow_account_creation) : QDialog(parent, 0){
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
		case TRANSACTION_TYPE_SECURITY_BUY: {setWindowTitle(tr("Edit Securities Bought")); break;}
		case TRANSACTION_TYPE_SECURITY_SELL: {setWindowTitle(tr("Edit Securities Sold")); break;}
	}
	editWidget = new TransactionEditWidget(false, extra_parameters, transaction_type, split, transfer_to, security, security_value_type, select_security, budg, this, allow_account_creation);
	box1->addWidget(editWidget);
	editWidget->transactionsReset();
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);
}
void TransactionEditDialog::accept() {
	if(editWidget->validValues(true)) {
		QDialog::accept();
	}
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

	descriptionButton = new QCheckBox(tr("Generic Description:"), this);
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
		valueEdit = new EqonomizeValueEdit(false, this);
		valueEdit->setEnabled(false);
		editLayout->addWidget(valueEdit, 1, 1);
	}

	dateButton = new QCheckBox(tr("Date:"), this);
	dateButton->setChecked(false);
	editLayout->addWidget(dateButton, 2, 0);
	dateEdit = new QDateEdit(QDate::currentDate(), this);
	dateEdit->setCalendarPopup(true);
	dateEdit->setEnabled(false);
	editLayout->addWidget(dateEdit, 2, 1);

	categoryButton = NULL;
	categoryCombo = NULL;
	if(transtype == TRANSACTION_TYPE_EXPENSE || transtype == TRANSACTION_TYPE_INCOME) {
		categoryButton = new QCheckBox(tr("Category:"), this);
		categoryButton->setChecked(false);
		editLayout->addWidget(categoryButton, 3, 0);
		categoryCombo = new QComboBox(this);
		categoryCombo->setEditable(false);
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
	buttonBox->button(QDialogButtonBox::Cancel)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Cancel)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);

	connect(categoryCombo, SIGNAL(activated(int)), this, SLOT(categoryActivated(int)));
	connect(descriptionButton, SIGNAL(toggled(bool)), descriptionEdit, SLOT(setEnabled(bool)));
	if(valueButton) connect(valueButton, SIGNAL(toggled(bool)), valueEdit, SLOT(setEnabled(bool)));
	connect(dateButton, SIGNAL(toggled(bool)), dateEdit, SLOT(setEnabled(bool)));

}
void MultipleTransactionsEditDialog::categoryActivated(int index) {
	if(index != categoryCombo->count() - 1) return;
	categoryCombo->setCurrentIndex(categories.count() > 0 ? 0 : -1);
	switch(transtype) {
		case TRANSACTION_TYPE_INCOME: {
			EditIncomesAccountDialog *dialog = new EditIncomesAccountDialog(budget, NULL, this, tr("New Income Category"));
			if(dialog->exec() == QDialog::Accepted) {
				IncomesAccount *account = dialog->newAccount();
				budget->addAccount(account);
				added_account = account;
				updateAccounts();
			}
			dialog->deleteLater();
			break;
		}
		case TRANSACTION_TYPE_EXPENSE: {
			EditExpensesAccountDialog *dialog = new EditExpensesAccountDialog(budget, NULL, this, tr("New Expense Category"));
			if(dialog->exec() == QDialog::Accepted) {
				ExpensesAccount *account = dialog->newAccount();
				budget->addAccount(account);
				added_account = account;
				updateAccounts();
			}
			dialog->deleteLater();
			break;
		}
		default: {break;}
	}
}
void MultipleTransactionsEditDialog::setTransaction(Transaction *trans) {
	if(trans) {
		descriptionEdit->setText(trans->description());
		dateEdit->setDate(trans->date());
		if(valueEdit) valueEdit->setValue(trans->value());
		if(transtype == TRANSACTION_TYPE_EXPENSE) {
			for(QVector<Account*>::size_type i = 0; i < categories.size(); i++) {
				if(((Expense*) trans)->category() == categories[i]) {
					categoryCombo->setCurrentIndex(i);
					break;
				}
			}
			if(payeeEdit) payeeEdit->setText(((Expense*) trans)->payee());
		} else if(transtype == TRANSACTION_TYPE_INCOME) {
			for(QVector<Account*>::size_type i = 0; i < categories.size(); i++) {
				if(((Income*) trans)->category() == categories[i]) {
					categoryCombo->setCurrentIndex(i);
					break;
				}
			}
			if(payeeEdit) payeeEdit->setText(((Income*) trans)->payer());
		}
	} else {
		descriptionEdit->clear();
		dateEdit->setDate(QDate::currentDate());
		if(valueEdit) valueEdit->setValue(0.0);
		if(payeeEdit) payeeEdit->clear();
	}
}
void MultipleTransactionsEditDialog::setScheduledTransaction(ScheduledTransaction *strans, const QDate &date) {
	if(strans) setTransaction(strans->transaction());
	else setTransaction(NULL);
	dateEdit->setDate(date);
}
void MultipleTransactionsEditDialog::updateAccounts() {
	if(!categoryCombo) return;
	categoryCombo->clear();
	categories.clear();
	switch(transtype) {
		case TRANSACTION_TYPE_INCOME: {
			Account *account = budget->incomesAccounts.first();
			while(account) {
				categoryCombo->addItem(account->nameWithParent());
				categories.push_back(account);
				if(account == added_account) categoryCombo->setCurrentIndex(categoryCombo->count() - 1);
				account = budget->incomesAccounts.next();
			}
			if(b_create_accounts) {
				categoryCombo->insertSeparator(categoryCombo->count());
				categoryCombo->addItem(tr("New Income Category…"));
			}
			break;
		}
		case TRANSACTION_TYPE_EXPENSE: {
			Account *account = budget->expensesAccounts.first();
			while(account) {
				categoryCombo->addItem(account->nameWithParent());
				categories.push_back(account);
				if(account == added_account) categoryCombo->setCurrentIndex(categoryCombo->count() - 1);
				account = budget->expensesAccounts.next();
			}
			if(b_create_accounts) {
				categoryCombo->insertSeparator(categoryCombo->count());
				categoryCombo->addItem(tr("New Expense Category…"));
			}
			break;
		}
		default: {}
	}
	added_account = NULL;
}
bool MultipleTransactionsEditDialog::modifyTransaction(Transaction *trans) {
	if(!validValues()) return false;
	bool b_descr = true, b_value = true;
	switch(trans->type()) {
		case TRANSACTION_TYPE_EXPENSE: {
			if(transtype == trans->type() && categoryButton->isChecked()) ((Expense*) trans)->setCategory((ExpensesAccount*) categories[categoryCombo->currentIndex()]);
			if(payeeEdit && payeeButton->isChecked()) ((Expense*) trans)->setPayee(payeeEdit->text());
			break;
		}
		case TRANSACTION_TYPE_INCOME: {
			if(((Income*) trans)->security()) b_descr = false;
			else if(payeeEdit && payeeButton->isChecked()) ((Income*) trans)->setPayer(payeeEdit->text());
			if(transtype == trans->type() && categoryButton->isChecked()) ((Income*) trans)->setCategory((IncomesAccount*) categories[categoryCombo->currentIndex()]);
			break;
		}
		case TRANSACTION_TYPE_TRANSFER: {
			break;
		}
		case TRANSACTION_TYPE_SECURITY_BUY: {
			if(transtype == TRANSACTION_TYPE_INCOME && categoryButton->isChecked()) ((SecurityTransaction*) trans)->setAccount(categories[categoryCombo->currentIndex()]);
			b_descr = false;
			b_value = false;
			break;
		}
		case TRANSACTION_TYPE_SECURITY_SELL: {
			if(transtype == TRANSACTION_TYPE_INCOME && categoryButton->isChecked()) ((SecurityTransaction*) trans)->setAccount(categories[categoryCombo->currentIndex()]);
			b_descr = false;
			b_value = false;
			break;
		}
	}
	if(b_descr && descriptionButton->isChecked()) trans->setDescription(descriptionEdit->text());
	if(valueEdit && b_value && valueButton->isChecked()) trans->setValue(valueEdit->value());
	if(dateButton->isChecked() && !trans->parentSplit()) trans->setDate(dateEdit->date());
	return true;
}

bool MultipleTransactionsEditDialog::checkAccounts() {
	switch(transtype) {
		case TRANSACTION_TYPE_INCOME: {
			if(!categoryButton->isChecked()) return true;
			if(categoryCombo->count() <= b_create_accounts ? 2 : 0) {
				QMessageBox::critical(this, tr("Error"), tr("No income category available."));
				return false;
			}
			break;
		}
		case TRANSACTION_TYPE_EXPENSE: {
			if(!categoryButton->isChecked()) return true;
			if(categoryCombo->count() <= b_create_accounts ? 2 : 0) {
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
	if(categoryCombo->currentIndex() >= categories.count()) return false;
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

