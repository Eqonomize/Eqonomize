/***************************************************************************
 *   Copyright (C) 2017 by Hanna Knutsson                                  *
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

#include <QVBoxLayout>
#include <QGridLayout>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QMessageBox>
#include <QPushButton>
#include <QLocale>
#include <QDebug>

#include "budget.h"
#include "eqonomizevalueedit.h"
#include "currencyconversiondialog.h"

CurrencyConversionDialog::CurrencyConversionDialog(Budget *budg, QWidget *parent) : QDialog(parent), budget(budg) {

	setWindowTitle(tr("Currency Converter"));
	setModal(false);
	
	QVBoxLayout *box1 = new QVBoxLayout(this);
	
	QGridLayout *grid = new QGridLayout();
	box1->addLayout(grid);

	
	fromEdit = new EqonomizeValueEdit(1.0, false, false, this, budget);
	grid->addWidget(fromEdit, 0, 0);
	fromCombo = new QComboBox(this);
	fromCombo->setEditable(false);
	grid->addWidget(fromCombo, 0, 1);
	
	toEdit = new EqonomizeValueEdit(1.0, false, false, this, budget);
	grid->addWidget(toEdit, 1, 0);
	toCombo = new QComboBox(this);
	toCombo->setEditable(false);
	grid->addWidget(toCombo, 1, 1);
		
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
	buttonBox->button(QDialogButtonBox::Close)->setAutoDefault(false);
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(reject()));
	box1->addWidget(buttonBox);
	
	updateCurrencies();
	
	convert_from = true;
	
	connect(fromEdit, SIGNAL(valueChanged(double)), this, SLOT(convertFrom()));
	connect(toEdit, SIGNAL(valueChanged(double)), this, SLOT(convertTo()));
	connect(fromCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(convert()));
	connect(toCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(convert()));
	
}

void CurrencyConversionDialog::updateCurrencies() {
	Currency *fromCur = NULL, *prev_fromCur = NULL;
	Currency *toCur = NULL, *prev_toCur = NULL;
	if(fromCombo->count() > 0) {
		fromCur  = (Currency*) fromCombo->currentData().value<void*>();
		if(!budget->currencies.contains(fromCur)) fromCur = NULL;
		toCur = (Currency*) toCombo->currentData().value<void*>();
		if(!budget->currencies.contains(toCur)) toCur = NULL;
		prev_fromCur = fromCur;
		prev_toCur = toCur;
	}
	if(!toCur) toCur = budget->defaultCurrency();
	
	if(!fromCur) {
		fromCur = budget->defaultCurrency();
		if(toCur == fromCur) {
			for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
				AssetsAccount *acc = *it;
				if(acc->currency() != NULL && acc->currency() != toCur) {
					fromCur = acc->currency();
					break;
				}
			}
		}
		if(toCur == fromCur) fromCur = budget->currency_euro;
		if(toCur == fromCur) {
			fromCur = budget->findCurrency("USD");
			if(!fromCur) fromCur = toCur;
		}
	}
	fromCombo->clear();
	toCombo->clear();
	int i = 0;
	for(CurrencyList<Currency*>::const_iterator it = budget->currencies.constBegin(); it != budget->currencies.constEnd(); ++it) {
		Currency *currency = *it;
		if(!currency->name(false).isEmpty()) {
			fromCombo->addItem(QIcon(":/data/flags/" + currency->code() + ".png"), QString("%2 (%1)").arg(qApp->translate("currencies.xml", qPrintable(currency->name()))).arg(currency->code()));
			toCombo->addItem(QIcon(":/data/flags/" + currency->code() + ".png"), QString("%2 (%1)").arg(qApp->translate("currencies.xml", qPrintable(currency->name()))).arg(currency->code()));
		} else {
			fromCombo->addItem(currency->code());
			toCombo->addItem(currency->code());
		}
		fromCombo->setItemData(i, QVariant::fromValue((void*) currency));
		toCombo->setItemData(i, QVariant::fromValue((void*) currency));
		if(currency == fromCur) fromCombo->setCurrentIndex(i);
		if(currency == toCur) toCombo->setCurrentIndex(i);
		i++;
	}
	if(prev_fromCur != fromCur || prev_toCur != toCur) convertFrom();
}

void CurrencyConversionDialog::convertFrom() {
	if(!fromCombo->currentData().isValid() || !toCombo->currentData().isValid()) return;
	Currency *fromCur  = (Currency*) fromCombo->currentData().value<void*>();
	Currency *toCur  = (Currency*) toCombo->currentData().value<void*>();
	if(!fromCur || !toCur) return;
	toEdit->blockSignals(true);
	toEdit->setValue(fromCur->convertTo(fromEdit->value(), toCur));
	toEdit->blockSignals(false);
	convert_from = true;
}
void CurrencyConversionDialog::convertTo() {
	if(!fromCombo->currentData().isValid() || !toCombo->currentData().isValid()) return;
	Currency *fromCur  = (Currency*) fromCombo->currentData().value<void*>();
	Currency *toCur  = (Currency*) toCombo->currentData().value<void*>();
	if(!fromCur || !toCur) return;
	fromEdit->blockSignals(true);
	fromEdit->setValue(toCur->convertTo(toEdit->value(), fromCur));
	fromEdit->blockSignals(false);
	convert_from = false;
}
void CurrencyConversionDialog::convert() {
	if(convert_from) convertFrom();
	else convertTo();
}

