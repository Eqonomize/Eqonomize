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
#include <QDebug>

#include "budget.h"
#include "accountcombobox.h"
#include "editaccountdialogs.h"
#include "eqonomizevalueedit.h"
#include "recurrence.h"
#include "transactioneditwidget.h"

#include <cmath>

#define TEROWCOL(row, col)	(b_autoedit ? row % rows : row), (b_autoedit ? ((row / rows) * 2) + col : col)

EqonomizeDateEdit::EqonomizeDateEdit(QWidget *parent) : QDateEdit(QDate::currentDate(), parent) {}
void EqonomizeDateEdit::keyPressEvent(QKeyEvent *event) {
	QDateEdit::keyPressEvent(event);
	if(event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
		emit returnPressed();
	}
}


TransactionEditWidget::TransactionEditWidget(bool auto_edit, bool extra_parameters, int transaction_type, Currency *split_currency, bool transfer_to, Security *sec, SecurityValueDefineType security_value_type, bool select_security, Budget *budg, QWidget *parent, bool allow_account_creation, bool multiaccount, bool withloan) : QWidget(parent), transtype(transaction_type), budget(budg), security(sec), b_autoedit(auto_edit), b_extra(extra_parameters), b_create_accounts(allow_account_creation) {
	bool split = (split_currency != NULL);
	splitcurrency = split_currency;
	value_set = false; shares_set = false; sharevalue_set = false;
	b_sec = (transtype == TRANSACTION_TYPE_SECURITY_BUY || transtype == TRANSACTION_TYPE_SECURITY_SELL);
	QVBoxLayout *editVLayout = new QVBoxLayout(this);
	int cols = 1;
	if(auto_edit) cols = 2;
	int rows = 6;
	if(b_sec && security_value_type == SECURITY_ALL_VALUES) rows += 1;
	else if(b_extra && !security && !select_security && transtype == TRANSACTION_TYPE_EXPENSE) rows += (multiaccount ? 1 : 2);
	else if(b_extra && !security && !select_security && transtype == TRANSACTION_TYPE_INCOME && !multiaccount) rows ++;
	else if(transtype == TRANSACTION_TYPE_TRANSFER) rows++;
	if(withloan) rows += 2;
	if(multiaccount) rows -= 4;
	if(split && !b_sec) rows -= 2;
	if(rows % cols > 0) rows = rows / cols + 1;
	else rows = rows / cols;
	QGridLayout *editLayout = new QGridLayout();
	editVLayout->addLayout(editLayout);
	editVLayout->addStretch(1);
	valueEdit = NULL;
	depositEdit = NULL;
	downPaymentEdit = NULL;
	dateEdit = NULL;
	sharesEdit = NULL;
	quotationEdit = NULL;
	descriptionEdit = NULL;
	payeeEdit = NULL;
	lenderEdit = NULL;
	quantityEdit = NULL;
	toCombo = NULL;
	fromCombo = NULL;
	securityCombo = NULL;
	currencyCombo = NULL;
	commentsEdit = NULL;
	int i = 0;
	if(b_sec) {
		int decimals = budget->defaultShareDecimals();
		editLayout->addWidget(new QLabel(tr("Security:", "Financial security (e.g. stock, mutual fund)"), this), TEROWCOL(i, 0));
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
			valueEdit = new EqonomizeValueEdit(false, this, budget);
			editLayout->addWidget(valueEdit, TEROWCOL(i, 1));
			i++;
		}
		if(security_value_type != SECURITY_VALUE_AND_QUOTATION) {
			if(transtype == TRANSACTION_TYPE_SECURITY_BUY) {
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
				editLayout->addLayout(sharesLayout, TEROWCOL(i, 1));
				i++;
			}
		}
		if(security_value_type != SECURITY_VALUE_AND_SHARES) {
			editLayout->addWidget(new QLabel(tr("Price per share:", "Financial shares"), this), TEROWCOL(i, 0));
			quotationEdit = new EqonomizeValueEdit(0.0, security ? security->quotationDecimals() : budget->defaultQuotationDecimals(), false, true, this, budget);
			if(security) quotationEdit->setCurrency(security->currency(), true);
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
			editLayout->addWidget(new QLabel(tr("Security:", "Financial security (e.g. stock, mutual fund)"), this), TEROWCOL(i, 0));
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
			editLayout->addWidget(new QLabel(tr("Withdrawal:"), this), TEROWCOL(i, 0));
		} else if(transtype == TRANSACTION_TYPE_INCOME) {
			editLayout->addWidget(new QLabel(tr("Income:"), this), TEROWCOL(i, 0));
		} else {
			editLayout->addWidget(new QLabel(tr("Cost:"), this), TEROWCOL(i, 0));
		}
		valueEdit = new EqonomizeValueEdit(0.0, !withloan, !withloan, this, budget);
		if(withloan) {
			valueEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
			QHBoxLayout *valueLayout = new QHBoxLayout();
			valueLayout->addWidget(valueEdit);
			currencyCombo = new QComboBox(this);
			currencyCombo->setEditable(false);
			Currency *currency = budget->currencies.first();
			int i2 = 0;
			while(currency) {
				currencyCombo->addItem(currency->code());
				currencyCombo->setItemData(i2, qVariantFromValue((void*) currency));
				if(currency == budget->defaultCurrency()) currencyCombo->setCurrentIndex(i2);
				i2++;
				currency = budget->currencies.next();
			}
			valueLayout->addWidget(currencyCombo);
			editLayout->addLayout(valueLayout, TEROWCOL(i, 1));
		} else {
			editLayout->addWidget(valueEdit, TEROWCOL(i, 1));
		}
		i++;
		if(transtype == TRANSACTION_TYPE_TRANSFER) {
			editLayout->addWidget(new QLabel(tr("Deposit:"), this), TEROWCOL(i, 0));
			depositEdit = new EqonomizeValueEdit(true, this, budget);
			editLayout->addWidget(depositEdit, TEROWCOL(i, 1));
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
			quantityEdit = new EqonomizeValueEdit(1.0, QUANTITY_DECIMAL_PLACES, true, false, this);
			quantityEdit->setToolTip(tr("Number of items included in the transaction. Entered cost is total cost for all items."));
			editLayout->addWidget(quantityEdit, TEROWCOL(i, 1));
			i++;
		}
		if(!split) {
			editLayout->addWidget(new QLabel(tr("Date:"), this), TEROWCOL(i, 0));
			dateEdit = new EqonomizeDateEdit(this);
			dateEdit->setCalendarPopup(true);
			editLayout->addWidget(dateEdit, TEROWCOL(i, 1));
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
				fromCombo = new AccountComboBox(ACCOUNT_TYPE_ASSETS, budget, b_create_accounts, false, false, !b_autoedit, false, this);
				editLayout->addWidget(fromCombo, TEROWCOL(i, 1));
				i++;
			}
			if(!split || !transfer_to) {
				editLayout->addWidget(new QLabel(tr("To:"), this), TEROWCOL(i, 0));
				toCombo = new AccountComboBox(ACCOUNT_TYPE_ASSETS, budget, b_create_accounts, false, false, !b_autoedit, false, this);
				toCombo->setEditable(false);
				editLayout->addWidget(toCombo, TEROWCOL(i, 1));
				i++;
			}
			break;
		}
		case TRANSACTION_TYPE_INCOME: {
			if(!multiaccount) {
				editLayout->addWidget(new QLabel(tr("Category:"), this), TEROWCOL(i, 0));
				fromCombo = new AccountComboBox(ACCOUNT_TYPE_INCOMES, budget, b_create_accounts, false, false, false, false, this);
				fromCombo->setEditable(false);
				editLayout->addWidget(fromCombo, TEROWCOL(i, 1));
				i++;
			}
			if(!split) {
				editLayout->addWidget(new QLabel(tr("To account:"), this), TEROWCOL(i, 0));
				toCombo = new AccountComboBox(ACCOUNT_TYPE_ASSETS, budget, b_create_accounts, b_create_accounts && b_autoedit, b_autoedit, !b_autoedit, true, this);
				toCombo->setEditable(false);
				editLayout->addWidget(toCombo, TEROWCOL(i, 1));
				i++;
			}
			if(b_extra && !split && !select_security && !security) {
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
				fromCombo = new AccountComboBox(ACCOUNT_TYPE_ASSETS, budget, b_create_accounts, false, false, true, true, this);
				fromCombo->setEditable(false);
				editLayout->addWidget(fromCombo, TEROWCOL(i, 1));
				i++;
			}
			break;
		}
		case TRANSACTION_TYPE_SECURITY_SELL: {
			if(!split) {
				editLayout->addWidget(new QLabel(tr("To account:"), this), TEROWCOL(i, 0));
				toCombo = new AccountComboBox(ACCOUNT_TYPE_ASSETS, budget, b_create_accounts, false, false, true, true, this);
				toCombo->setEditable(false);
				editLayout->addWidget(toCombo, TEROWCOL(i, 1));
				i++;
			}
			break;
		}
		default: {
			if(!multiaccount) {
				editLayout->addWidget(new QLabel(tr("Category:"), this), TEROWCOL(i, 0));
				toCombo = new AccountComboBox(ACCOUNT_TYPE_EXPENSES, budget, b_create_accounts, false, false, false, false, this);
				toCombo->setEditable(false);
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
				fromCombo->setEditable(false);
				editLayout->addWidget(fromCombo, TEROWCOL(i, 1));
				i++;
			}
			if(b_extra && !split) {
				editLayout->addWidget(new QLabel(tr("Payee:"), this), TEROWCOL(i, 0));
				payeeEdit = new QLineEdit(this);
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
	if(!multiaccount) {
		editLayout->addWidget(new QLabel(tr("Comments:"), this), TEROWCOL(i, 0));
		commentsEdit = new QLineEdit(this);
		editLayout->addWidget(commentsEdit, TEROWCOL(i, 1));
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
		if(depositEdit && transfer_to) depositEdit->setCurrency(splitcurrency);
	}
	if(dateEdit) connect(dateEdit, SIGNAL(dateChanged(const QDate&)), this, SIGNAL(dateChanged(const QDate&)));
	if(valueEdit) connect(valueEdit, SIGNAL(valueChanged(double)), this, SLOT(valueChanged(double)));
	if(b_sec) {
		switch(security_value_type) {
			case SECURITY_ALL_VALUES: {
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
				if(dateEdit) {
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
				connect(downPaymentEdit, SIGNAL(returnPressed()), quantityEdit, SLOT(setFocus()));
				connect(downPaymentEdit, SIGNAL(returnPressed()), quantityEdit, SLOT(selectAll()));
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
		}
		if(descriptionEdit) {
			connect(descriptionEdit, SIGNAL(returnPressed()), valueEdit, SLOT(setFocus()));
			connect(descriptionEdit, SIGNAL(returnPressed()), valueEdit, SLOT(selectAll()));
			connect(descriptionEdit, SIGNAL(editingFinished()), this, SLOT(setDefaultValue()));
			connect(descriptionEdit, SIGNAL(textChanged(const QString&)), this, SLOT(descriptionChanged(const QString&)));
		}
	}
	if(valueEdit) connect(valueEdit, SIGNAL(returnPressed()), this, SLOT(valueNextField()));
	if(payeeEdit) {
		connect(payeeEdit, SIGNAL(editingFinished()), this, SLOT(setDefaultValueFromPayee()));
		connect(payeeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(payeeChanged(const QString&)));
	}
	if(b_autoedit && dateEdit) connect(dateEdit, SIGNAL(returnPressed()), this, SIGNAL(addmodify()));
	if(payeeEdit && lenderEdit) connect(payeeEdit, SIGNAL(returnPressed()), lenderEdit, SLOT(setFocus()));
	else if(payeeEdit && commentsEdit) connect(payeeEdit, SIGNAL(returnPressed()), commentsEdit, SLOT(setFocus()));
	if(lenderEdit && commentsEdit) connect(lenderEdit, SIGNAL(returnPressed()), commentsEdit, SLOT(setFocus()));
	if(commentsEdit && b_autoedit) connect(commentsEdit, SIGNAL(returnPressed()), this, SIGNAL(addmodify()));
	if(securityCombo) connect(securityCombo, SIGNAL(activated(int)), this, SLOT(securityChanged()));
	if(currencyCombo) connect(currencyCombo, SIGNAL(activated(int)), this, SLOT(currencyChanged(int)));
	if(fromCombo) {
		connect(fromCombo, SIGNAL(newAccountRequested()), this, SLOT(newFromAccount()));
		connect(fromCombo, SIGNAL(newLoanRequested()), this, SIGNAL(newLoanRequested()));
		connect(fromCombo, SIGNAL(multipleAccountsRequested()), this, SIGNAL(multipleAccountsRequested()));
		connect(fromCombo, SIGNAL(accountSelected()), this, SLOT(fromActivated()));
		connect(fromCombo, SIGNAL(currentAccountChanged()), this, SLOT(fromChanged()));
	}
	if(toCombo) {
		connect(toCombo, SIGNAL(newAccountRequested()), this, SLOT(newToAccount()));
		connect(toCombo, SIGNAL(newLoanRequested()), this, SIGNAL(newLoanRequested()));
		connect(toCombo, SIGNAL(multipleAccountsRequested()), this, SIGNAL(multipleAccountsRequested()));
		connect(toCombo, SIGNAL(accountSelected()), this, SLOT(toActivated()));
		connect(toCombo, SIGNAL(currentAccountChanged()), this, SLOT(toChanged()));
	}
}
void TransactionEditWidget::valueNextField() {
	if(depositEdit && depositEdit->isEnabled()) {
		depositEdit->setFocus();
		depositEdit->selectAll();
	} else if(sharesEdit) {
		sharesEdit->setFocus();
		sharesEdit->selectAll();
	} else if(downPaymentEdit) {
		downPaymentEdit->setFocus();
		downPaymentEdit->selectAll();
	} else if(quantityEdit) {
		quantityEdit->setFocus();
		quantityEdit->selectAll();
	} else if(dateEdit) {
		focusDate();
	}
}
void TransactionEditWidget::newFromAccount() {
	Account *account = fromCombo->createAccount();
	if(account) {
		emit accountAdded(account);
	}
}
void TransactionEditWidget::newToAccount() {
	Account *account = toCombo->createAccount();
	if(account) {
		emit accountAdded(account);
	}
}
void TransactionEditWidget::fromActivated() {
	if(toCombo && transtype != TRANSACTION_TYPE_EXPENSE) toCombo->setFocus();
	else if(payeeEdit) payeeEdit->setFocus();
	else if(commentsEdit) commentsEdit->setFocus();
	if(transtype == TRANSACTION_TYPE_INCOME) {
		setDefaultValueFromCategory();
	}
}
void TransactionEditWidget::toActivated() {
	if(fromCombo && transtype == TRANSACTION_TYPE_EXPENSE) fromCombo->setFocus();
	else if(payeeEdit) payeeEdit->setFocus();
	else if(commentsEdit) commentsEdit->setFocus();
	if(transtype == TRANSACTION_TYPE_EXPENSE) {
		setDefaultValueFromCategory();
	}
}
void TransactionEditWidget::fromChanged() {
	Account *acc = fromCombo->currentAccount();
	if(!acc) return;
	if(downPaymentEdit) {
		downPaymentEdit->setCurrency(acc->currency());
	} else if(valueEdit && acc->type() == ACCOUNT_TYPE_ASSETS) {
		valueEdit->setCurrency(acc->currency());
	}
	if(transtype == TRANSACTION_TYPE_TRANSFER) {
		Currency *cur2 = splitcurrency;
		if(toCombo && toCombo->currentAccount()) {
			cur2 = toCombo->currentAccount()->currency();
		}
		if(cur2 && depositEdit) {
			if(acc->currency() != cur2) {
				depositEdit->setEnabled(true);
			} else {
				depositEdit->setEnabled(false);
				if(is_zero(valueEdit->value())) valueEdit->setValue(depositEdit->value());
				else depositEdit->setValue(valueEdit->value());
			}
		}
	}
}
void TransactionEditWidget::toChanged() {
	Account *acc = toCombo->currentAccount();
	if(!acc) return;
	if(transtype == TRANSACTION_TYPE_TRANSFER) {
		if(depositEdit) {
			depositEdit->setCurrency(acc->currency());
			Currency *cur2 = splitcurrency;
			if(fromCombo && fromCombo->currentAccount()) {
				cur2 = fromCombo->currentAccount()->currency();
			}
			if(cur2) {
				if(acc->currency() != cur2) {
					depositEdit->setEnabled(true);
				} else {
					depositEdit->setEnabled(false);
					if(is_zero(valueEdit->value())) valueEdit->setValue(depositEdit->value());
					else depositEdit->setValue(valueEdit->value());
				}
			}
		}
	} else {
		if(valueEdit && acc->type() == ACCOUNT_TYPE_ASSETS) {
			valueEdit->setCurrency(acc->currency());
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
void TransactionEditWidget::securityChanged() {
	security = selectedSecurity();
	if(security) {
		if(sharesEdit) sharesEdit->setPrecision(security->decimals());
		if(quotationEdit) {
			quotationEdit->setPrecision(security->quotationDecimals());
			quotationEdit->setCurrency(security->currency(), false);
		}
		if(sharesEdit && security && shares_date.isValid()) sharesEdit->setMaximum(security->shares(shares_date));
	}
}
void TransactionEditWidget::currencyChanged(int) {}
void TransactionEditWidget::valueChanged(double value) {
	if(valueEdit && depositEdit && !depositEdit->isEnabled()) depositEdit->setValue(value);
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
				valueEdit->setFocus();
				valueEdit->selectAll();
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
				valueEdit->setFocus();
				valueEdit->selectAll();
			}
		}
	}
}
void TransactionEditWidget::updateFromAccounts(Account *exclude_account, Currency *force_currency) {
	if(!fromCombo) return;
	fromCombo->updateAccounts(exclude_account, force_currency);
}
void TransactionEditWidget::updateToAccounts(Account *exclude_account, Currency *force_currency) {
	if(!toCombo) return;
	toCombo->updateAccounts(exclude_account, force_currency);
}
void TransactionEditWidget::updateAccounts(Account *exclude_account, Currency *force_currency) {
	updateToAccounts(exclude_account, force_currency);
	updateFromAccounts(exclude_account, force_currency);
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
	if(securityCombo && securityCombo->count() == 0) {
		QMessageBox::critical(this, tr("Error"), tr("No security available.", "Financial security (e.g. stock, mutual fund)"));
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
	bool b_transsec = (trans->type() == TRANSACTION_TYPE_SECURITY_BUY || trans->type() == TRANSACTION_TYPE_SECURITY_SELL);
	if(b_sec) {
		if(trans->type() != transtype) return false;
		if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY) {
			if(fromCombo) ((SecurityTransaction*) trans)->setAccount(fromCombo->currentAccount());
		} else {
			if(toCombo) ((SecurityTransaction*) trans)->setAccount(toCombo->currentAccount());
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
		if(commentsEdit) trans->setComment(commentsEdit->text());
		return true;
	} else if(b_transsec) {
		return false;
	}
	if(dateEdit) trans->setDate(dateEdit->date());
	if(fromCombo) trans->setFromAccount(fromCombo->currentAccount());
	if(toCombo) trans->setToAccount(toCombo->currentAccount());
	trans->setValue(valueEdit->value());
	if(descriptionEdit && (trans->type() != TRANSACTION_TYPE_INCOME || !((Income*) trans)->security())) trans->setDescription(descriptionEdit->text());
	if(commentsEdit) trans->setComment(commentsEdit->text());
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
		return expense;
	}
	
	MultiAccountTransaction *split = new MultiAccountTransaction(budget, (CategoryAccount*) toCombo->currentAccount(), descriptionEdit->text());
	split->setComment(commentsEdit->text());
	if(quantityEdit) split->setQuantity(quantityEdit->value());
	
	Expense *expense = new Expense(budget, valueEdit->value() - downPaymentEdit->value(), dateEdit->date(), (ExpensesAccount*) toCombo->currentAccount(), loan);
	split->addTransaction(expense);
	
	Expense *down_payment = new Expense(budget, downPaymentEdit->value(), dateEdit->date(), (ExpensesAccount*) toCombo->currentAccount(), (AssetsAccount*) fromCombo->currentAccount());
	down_payment->setPayee(loan->maintainer());
	split->addTransaction(down_payment);
	
	return split;
}
Transaction *TransactionEditWidget::createTransaction() {
	if(!validValues()) return NULL;
	Transaction *trans;
	if(transtype == TRANSACTION_TYPE_TRANSFER) {
		Transfer *transfer = new Transfer(budget, valueEdit->value(), depositEdit->value(), dateEdit ? dateEdit->date() : QDate(), fromCombo ? (AssetsAccount*) fromCombo->currentAccount() : NULL, toCombo ? (AssetsAccount*) toCombo->currentAccount() : NULL, descriptionEdit ? descriptionEdit->text() : QString::null, commentsEdit ? commentsEdit->text() : NULL);
		trans = transfer;
	} else if(transtype == TRANSACTION_TYPE_INCOME) {
		Income *income = new Income(budget, valueEdit->value(), dateEdit ? dateEdit->date() : QDate(), fromCombo ? (IncomesAccount*) fromCombo->currentAccount() : NULL, toCombo ? (AssetsAccount*) toCombo->currentAccount() : NULL, descriptionEdit ? descriptionEdit->text() : QString::null, commentsEdit ? commentsEdit->text() : NULL);
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
		SecurityBuy *secbuy = new SecurityBuy(selectedSecurity(), value, shares, share_value, dateEdit ? dateEdit->date() : QDate(), fromCombo ? fromCombo->currentAccount() : NULL, commentsEdit ? commentsEdit->text() : NULL);
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
		SecuritySell *secsell = new SecuritySell(selectedSecurity(), value, shares, share_value, dateEdit ? dateEdit->date() : QDate(), toCombo ? toCombo->currentAccount() : NULL, commentsEdit ? commentsEdit->text() : NULL);
		trans = secsell;
	} else {
		Expense *expense = new Expense(budget, valueEdit->value(), dateEdit ? dateEdit->date() : QDate(), toCombo ? (ExpensesAccount*) toCombo->currentAccount() : NULL, fromCombo ? (AssetsAccount*) fromCombo->currentAccount() : NULL, descriptionEdit ? descriptionEdit->text() : QString::null, commentsEdit ? commentsEdit->text() : NULL);
		if(quantityEdit) expense->setQuantity(quantityEdit->value());
		if(payeeEdit) expense->setPayee(payeeEdit->text());
		trans = expense;
	}
	return trans;
}
void TransactionEditWidget::transactionRemoved(Transaction *trans) {
	if(descriptionEdit && trans->type() == transtype && !trans->description().isEmpty() && default_values.contains(trans->description().toLower()) && default_values[trans->description().toLower()] == trans) {
		QString lower_description = trans->description().toLower();
		default_values[trans->description().toLower()] = NULL;
		switch(transtype) {
			case TRANSACTION_TYPE_EXPENSE: {
				Expense *expense = budget->expenses.last();
				while(expense) {
					if(expense != trans && expense->description().toLower() == lower_description && expense->subtype() != TRANSACTION_SUBTYPE_DEBT_FEE && expense->subtype() != TRANSACTION_SUBTYPE_DEBT_INTEREST) {
						default_values[lower_description] = expense;
						break;
					}
					expense = budget->expenses.previous();
				}
				break;
			}
			case TRANSACTION_TYPE_INCOME: {
				Income *income = budget->incomes.last();
				while(income) {
					if(income != trans && !income->security() && income->description().toLower() == lower_description) {
						default_values[lower_description] = income;
						break;
					}
					income = budget->incomes.previous();
				}
				break;
			}
			case TRANSACTION_TYPE_TRANSFER: {
				Transfer *transfer= budget->transfers.last();
				while(transfer) {
					if(transfer != trans && transfer->description().toLower() == lower_description) {
						default_values[lower_description] = transfer;
						break;
					}
					transfer = budget->transfers.previous();
				}
				break;
			}
			default: {}
		}
	}
	if(payeeEdit && transtype == TRANSACTION_TYPE_INCOME && trans->type() == transtype && !((Income*) trans)->payer().isEmpty() && default_payee_values.contains(((Income*) trans)->payer().toLower()) && default_payee_values[((Income*) trans)->payer().toLower()] == trans) {
		default_payee_values[((Income*) trans)->payer().toLower()] = NULL;
		QString lower_payee = ((Income*) trans)->payer().toLower();
		Income *income = budget->incomes.last();
		while(income) {
			if(income != trans && !income->security()  && income->payer().toLower() == lower_payee) {
				default_payee_values[lower_payee] = income;
				break;
			}
			income = budget->incomes.previous();
		}
	}
	if(payeeEdit && transtype == TRANSACTION_TYPE_EXPENSE && trans->type() == transtype && !((Expense*) trans)->payee().isEmpty() && default_payee_values.contains(((Expense*) trans)->payee().toLower()) && default_payee_values[((Expense*) trans)->payee().toLower()] == trans) {
		default_payee_values[((Expense*) trans)->payee().toLower()] = NULL;
		QString lower_payee = ((Expense*) trans)->payee().toLower();
		Expense *expense = budget->expenses.last();
		while(expense) {
			if(expense != trans && expense->payee().toLower() == lower_payee && expense->subtype() != TRANSACTION_SUBTYPE_DEBT_FEE && expense->subtype() != TRANSACTION_SUBTYPE_DEBT_INTEREST) {
				default_payee_values[lower_payee] = expense;
				break;
			}
			expense = budget->expenses.previous();
		}
	}
	if(transtype == TRANSACTION_TYPE_INCOME && fromCombo && trans->type() == transtype && default_category_values.contains(trans->fromAccount()) && default_category_values[trans->fromAccount()] == trans) {
		bool category_found = false;
		Account *cat = trans->fromAccount();
		Income *income = budget->incomes.last();
		while(income) {
			if(income != trans && !income->security()  && income->fromAccount() == cat) {
				default_category_values[cat] = income;
				category_found = true;
				break;
			}
			income = budget->incomes.previous();
		}
		if(!category_found) default_category_values.remove(cat);
	} else if(transtype == TRANSACTION_TYPE_EXPENSE && toCombo && trans->type() == transtype && default_category_values.contains(trans->toAccount()) && default_category_values[trans->toAccount()] == trans) {
		bool category_found = false;
		Account *cat = trans->toAccount();
		Expense *expense = budget->expenses.last();
		while(expense) {
			if(expense != trans && expense->toAccount() == cat && expense->subtype() != TRANSACTION_SUBTYPE_DEBT_FEE && expense->subtype() != TRANSACTION_SUBTYPE_DEBT_INTEREST) {
				default_category_values[cat] = expense;
				category_found = true;
				break;
			}
			expense = budget->expenses.previous();
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
			Expense *expense = budget->expenses.last();
			while(expense) {
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
				expense = budget->expenses.previous();
			}
			break;
		}
		case TRANSACTION_TYPE_INCOME: {
			Income *income = budget->incomes.last();
			while(income) {
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
				income = budget->incomes.previous();
			}
			break;
		}
		case TRANSACTION_TYPE_TRANSFER: {
			Transfer *transfer= budget->transfers.last();
			while(transfer) {
				if(descriptionEdit && !transfer->description().isEmpty() && !default_values.contains(transfer->description().toLower())) {
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
	if(descriptionEdit) ((QStandardItemModel*) descriptionEdit->completer()->model())->sort(1);
	if(payeeEdit) ((QStandardItemModel*) payeeEdit->completer()->model())->sort(1);
}
void TransactionEditWidget::setCurrentToItem(int index) {
	if(toCombo) toCombo->setCurrentAccountIndex(index);
}
void TransactionEditWidget::setCurrentFromItem(int index) {
	if(fromCombo) fromCombo->setCurrentAccountIndex(index);
}
void TransactionEditWidget::setAccount(Account *account) {
	if(fromCombo && (transtype == TRANSACTION_TYPE_EXPENSE || transtype == TRANSACTION_TYPE_TRANSFER || transtype == TRANSACTION_TYPE_SECURITY_BUY)) {
		fromCombo->setCurrentAccount(account);
	} else if(toCombo) {
		toCombo->setCurrentAccount(account);
	}
}
int TransactionEditWidget::currentToItem() {
	if(!toCombo) return -1;
	return toCombo->currentAccountIndex();
}
int TransactionEditWidget::currentFromItem() {
	if(!fromCombo) return -1;
	return fromCombo->currentAccountIndex();
}

void TransactionEditWidget::setTransaction(Transaction *trans) {
	if(valueEdit) valueEdit->blockSignals(true);
	if(sharesEdit) sharesEdit->blockSignals(true);
	if(quotationEdit) quotationEdit->blockSignals(true);
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
		if(quantityEdit) quantityEdit->setValue(1.0);
		if(payeeEdit) payeeEdit->clear();
		if(dateEdit) emit dateChanged(dateEdit->date());
	} else {
		value_set = true; shares_set = true; sharevalue_set = true;
		if(dateEdit) dateEdit->setDate(trans->date());
		if(commentsEdit) commentsEdit->setText(trans->comment());
		if(toCombo && (!b_sec || transtype == TRANSACTION_TYPE_SECURITY_SELL)) {
			toCombo->setCurrentAccount(trans->toAccount());
		}
		if(fromCombo && (!b_sec || transtype == TRANSACTION_TYPE_SECURITY_BUY)) {
			fromCombo->setCurrentAccount(trans->fromAccount());
		}
		if(depositEdit) {
			if(valueEdit) valueEdit->setValue(trans->fromValue());
			depositEdit->setValue(trans->toValue());
		} else if(valueEdit) {
			valueEdit->setValue(trans->value());
		}
		if(b_sec) {
			if(transtype != trans->type()) return;
			//if(transtype == TRANSACTION_TYPE_SECURITY_SELL) setMaxShares(((SecurityTransaction*) trans)->security()->shares(QDate::currentDate()) + ((SecurityTransaction*) trans)->shares());
			if(sharesEdit) sharesEdit->setValue(((SecurityTransaction*) trans)->shares());
			if(quotationEdit) quotationEdit->setValue(((SecurityTransaction*) trans)->shareValue());
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
	if(valueEdit) valueEdit->blockSignals(false);
	if(sharesEdit) sharesEdit->blockSignals(false);
	if(quotationEdit) quotationEdit->blockSignals(false);
}
void TransactionEditWidget::setTransaction(Transaction *trans, const QDate &date) {
	setTransaction(trans);
	if(dateEdit) dateEdit->setDate(date);
}
void TransactionEditWidget::setMultiAccountTransaction(MultiAccountTransaction *split) {
	if(!split) {
		setTransaction(NULL);
		return;
	}
	if(valueEdit) valueEdit->blockSignals(true);
	if(dateEdit) dateEdit->setDate(split->date());
	if(commentsEdit) commentsEdit->setText(split->comment());
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
		if(isVisible()) valueEdit->setFocus();
	}
	valueEdit->setValue(split->value());
	if(quantityEdit) quantityEdit->setValue(split->quantity());
	if(payeeEdit) payeeEdit->setText(split->payees());
	
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
	}
	editWidget = new TransactionEditWidget(false, extra_parameters, transaction_type, split_currency, transfer_to, security, security_value_type, select_security, budg, this, allow_account_creation, multiaccount, withloan);
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
		categoryCombo = new AccountComboBox(transtype == TRANSACTION_TYPE_EXPENSE ? ACCOUNT_TYPE_EXPENSES : ACCOUNT_TYPE_INCOMES, budget, b_create_accounts, false, false, true, true, this);
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
		if(valueEdit) valueEdit->setValue(trans->value());
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
		if(valueEdit) valueEdit->setValue(0.0);
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
				b_payee = false;
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
	return true;
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

