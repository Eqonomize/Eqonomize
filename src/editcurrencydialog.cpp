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
#include <QHBoxLayout>
#include <QLabel>
#include <QGridLayout>
#include <QDateEdit>
#include <QRadioButton>
#include <QButtonGroup>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QMessageBox>
#include <QPushButton>
#include <QLocale>
#include <QRegExpValidator>
#include <QDebug>

#include "budget.h"
#include "eqonomizevalueedit.h"
#include "editcurrencydialog.h"


EditCurrencyDialog::EditCurrencyDialog(Budget *budg, Currency *cur, bool enable_set_as_default, QWidget *parent) : QDialog(parent), budget(budg), currency(cur) {

	if(currency) setWindowTitle(tr("Edit Currency"));
	else setWindowTitle(tr("New Currency"));
	setModal(true);

	int row = 0;

	QVBoxLayout *box1 = new QVBoxLayout(this);

	QGridLayout *grid = new QGridLayout();
	box1->addLayout(grid);

	grid->addWidget(new QLabel(tr("Code:"), this), row, 0);
	codeEdit = new QLineEdit(this);
	if(currency) {
		codeEdit->setText(currency->code());
		codeEdit->setEnabled(false);
	} else {
		codeEdit->setValidator(new QRegExpValidator(QRegExp("^[^\\d\\s\\.\\,\\+\\-\\*\\^]+$")));
	}
	grid->addWidget(codeEdit, row, 1); row++;
	grid->addWidget(new QLabel(tr("Symbol:"), this), row, 0);
	symbolEdit = new QLineEdit(this);
	if(currency) symbolEdit->setText(currency->symbol(false));
	grid->addWidget(symbolEdit, row, 1); row++;
	QHBoxLayout *box2 = new QHBoxLayout();
	QButtonGroup *precedesGroup = new QButtonGroup(this);
	prefixButton = new QRadioButton(tr("Prefix"), this);
	suffixButton = new QRadioButton(tr("Suffix"), this);
	psDefaultButton = new QRadioButton(tr("Default"), this);
	precedesGroup->addButton(prefixButton);
	precedesGroup->addButton(suffixButton);
	precedesGroup->addButton(psDefaultButton);
	if(currency && currency->symbolPrecedes(false) > 0) prefixButton->setChecked(true);
	else if(currency && currency->symbolPrecedes(false) == 0) suffixButton->setChecked(true);
	else psDefaultButton->setChecked(true);
	box2->addWidget(prefixButton);
	box2->addWidget(suffixButton);
	box2->addWidget(psDefaultButton);
	grid->addLayout(box2, row, 0, 1, 2, Qt::AlignRight); row++;
	grid->addWidget(new QLabel(tr("Name:"), this), row, 0);
	nameEdit = new QLineEdit(this);
	if(currency) nameEdit->setText(currency->name(false));
	grid->addWidget(nameEdit, row, 1); row++;
	grid->addWidget(new QLabel(tr("Decimals:"), this), row, 0);
	decimalsEdit = new QSpinBox(this);
	decimalsEdit->setRange(-1, 4);
	if(currency) decimalsEdit->setValue(currency->fractionalDigits(false));
	else decimalsEdit->setValue(-1);
	decimalsEdit->setSpecialValueText(tr("Default"));
	grid->addWidget(decimalsEdit, row, 1); row++;
	QLabel *label = new QLabel(budget->currency_euro->formatValue(1.0) + " =", this);
	label->setAlignment(Qt::AlignRight);
	grid->addWidget(label, row, 0);
	rateEdit = new EqonomizeValueEdit(0.00001, 1.0, currency ? currency->exchangeRate() : 1.0, 5, false, this, budget);
	if(currency) rateEdit->setCurrency(currency, true);
	if(currency == budget->currency_euro) rateEdit->setEnabled(false);
	grid->addWidget(rateEdit, row, 1); row++;
	grid->addWidget(new QLabel(tr("Date:"), this), row, 0);
	dateEdit = new EqonomizeDateEdit(this);
	dateEdit->setMaximumDate(QDate::currentDate());
	dateEdit->setCalendarPopup(true);
	if(currency == budget->currency_euro) dateEdit->setEnabled(false);
	QDate date;
	if(currency) date = currency->lastExchangeRateDate();
	if(date.isValid()) dateEdit->setDate(date);
	else dateEdit->setDate(QDate::currentDate());
	grid->addWidget(dateEdit, row, 1); row++;
	if(enable_set_as_default) {
		defaultButton = new QCheckBox(tr("Main currency"), this);
		defaultButton->setChecked(currency == budget->defaultCurrency());
		defaultButton->setEnabled(currency != budget->defaultCurrency());
		grid->addWidget(defaultButton, row, 0, 1, 2); row++;
	} else {
		defaultButton = NULL;
	}

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);

	if(currency == budget->defaultCurrency()) {
		connect(symbolEdit, SIGNAL(textChanged(const QString&)), this, SLOT(currencyChanged()));
	}
	if(!currency) {
		connect(codeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(currencyChanged()));
	}
	connect(prefixButton, SIGNAL(toggled(bool)), this, SLOT(currencyChanged()));
	connect(suffixButton, SIGNAL(toggled(bool)), this, SLOT(currencyChanged()));
	connect(psDefaultButton, SIGNAL(toggled(bool)), this, SLOT(currencyChanged()));
	connect(decimalsEdit, SIGNAL(valueChanged(int)), this, SLOT(currencyChanged()));
	if(defaultButton) connect(defaultButton, SIGNAL(toggled(bool)), this, SLOT(currencyChanged()));

}

void EditCurrencyDialog::currencyChanged() {
	Currency *cur = new Currency(budget, codeEdit->text().trimmed(), symbolEdit->text().trimmed(), nameEdit->text().trimmed(), 1.0, QDate::currentDate(), decimalsEdit->value(), prefixButton->isChecked() ? 1 : (suffixButton->isChecked() ? 0 : - 1));
	rateEdit->setCurrency(cur, true, defaultButton && defaultButton->isChecked(), true);
	delete cur;
}
Currency *EditCurrencyDialog::createCurrency() {
	Currency *cur = new Currency(budget, codeEdit->text().trimmed(), symbolEdit->text().trimmed(), nameEdit->text().trimmed(), rateEdit->value(), (rateEdit->value() == 1.0 && dateEdit->date() == QDate::currentDate()) ? QDate() : dateEdit->date(), decimalsEdit->value(), prefixButton->isChecked() ? 1 : (suffixButton->isChecked() ? 0 : - 1));
	budget->addCurrency(cur);
	QString error = budget->saveCurrencies();
	if(!error.isNull()) QMessageBox::critical(isVisible() ? this : parentWidget(), tr("Error"), tr("Error saving currencies: %1.").arg(error));
	if(defaultButton && defaultButton->isChecked()) budget->setDefaultCurrency(cur);
	return cur;
}
void EditCurrencyDialog::modifyCurrency(Currency *cur) {
	cur->setName(nameEdit->text().trimmed());
	cur->setSymbol(symbolEdit->text().trimmed());
	if(cur != budget->currency_euro && (!cur->rates.isEmpty() || rateEdit->value() != 1.0 || dateEdit->date() != QDate::currentDate())) {
		cur->setExchangeRate(rateEdit->value(), dateEdit->date());
	}
	cur->setFractionalDigits(decimalsEdit->value());
	if(prefixButton->isChecked()) {
		cur->setSymbolPrecedes(1);
	} else if(suffixButton->isChecked()) {
		cur->setSymbolPrecedes(0);
	} else {
		cur->setSymbolPrecedes(-1);
	}
	budget->currencyModified(cur);
	QString error = budget->saveCurrencies();
	if(!error.isNull()) QMessageBox::critical(isVisible() ? this : parentWidget(), tr("Error"), tr("Error saving currencies: %1.").arg(error));
	if(defaultButton && defaultButton->isChecked()) budget->setDefaultCurrency(cur);
}
void EditCurrencyDialog::accept() {
	if(!currency) {
		QString scode = codeEdit->text().trimmed();
		if(scode.isEmpty()) {
			codeEdit->setFocus();
			QMessageBox::critical(this, tr("Error"), tr("Empty code."));
			return;
		}
		if(budget->findCurrency(scode)) {
			codeEdit->setFocus();
			QMessageBox::critical(this, tr("Error"), tr("Code already exists."));
			return;
		}
	}
	QDialog::accept();
}

