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

#include "eqonomizevalueedit.h"

#include "budget.h"

#include <cmath>
#include <cfloat>

#include <QDebug>

#define MAX_VALUE 1000000000000.0

EqonomizeValueEdit::EqonomizeValueEdit(bool allow_negative, QWidget *parent, Budget *budg) : QDoubleSpinBox(parent), budget(budg) {
	init(allow_negative ? -MAX_VALUE : 0.0, MAX_VALUE, 1.0, 0.0, -1, true);
}
EqonomizeValueEdit::EqonomizeValueEdit(double value, bool allow_negative, bool show_currency, QWidget *parent, Budget *budg) : QDoubleSpinBox(parent), budget(budg) {
	init(allow_negative ? -MAX_VALUE : 0.0, MAX_VALUE, 1.0, value, -1, show_currency);
}
EqonomizeValueEdit::EqonomizeValueEdit(double value, int precision, bool allow_negative, bool show_currency, QWidget *parent, Budget *budg) : QDoubleSpinBox(parent), budget(budg) {
	init(allow_negative ? -MAX_VALUE : 0.0, MAX_VALUE, 1.0, value, precision, show_currency);
}
EqonomizeValueEdit::EqonomizeValueEdit(double lower, double step, double value, int precision, bool show_currency, QWidget *parent, Budget *budg) : QDoubleSpinBox(parent), budget(budg) {
	init(lower, MAX_VALUE, step, value, precision, show_currency);
}
EqonomizeValueEdit::EqonomizeValueEdit(double lower, double upper, double step, double value, int precision, bool show_currency, QWidget *parent, Budget *budg) : QDoubleSpinBox(parent), budget(budg) {
	init(lower, upper, step, value, precision, show_currency);
}
EqonomizeValueEdit::~EqonomizeValueEdit() {}

void EqonomizeValueEdit::init(double lower, double upper, double step, double value, int precision, bool show_currency) {
	o_currency = NULL;
	i_precision = precision;
	QDoubleSpinBox::setRange(lower, upper);
	setSingleStep(step);
	setAlignment(Qt::AlignRight);
	if(show_currency) {
		if(budget) {
			if(i_precision < 0) i_precision = budget->defaultCurrency()->fractionalDigits(true);
			setCurrency(budget->defaultCurrency(), true);
		} else {
			if(CURRENCY_IS_PREFIX) {
				setPrefix(QLocale().currencySymbol());
			} else {
				setSuffix(QString(" ") + QLocale().currencySymbol());
			}
		}
	}
	if(i_precision < 0) i_precision = MONETARY_DECIMAL_PLACES;
	setDecimals(i_precision);
	setValue(value);
	connect(this, SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));
}
void EqonomizeValueEdit::onEditingFinished() {
	if(hasFocus()) emit returnPressed();
}
void EqonomizeValueEdit::setRange(double lower, double step, int precision) {
	setRange(lower, MAX_VALUE, step, precision);
}
void EqonomizeValueEdit::setRange(double lower, double upper, double step, int precision) {
	i_precision = precision;
	setDecimals(precision);
	QDoubleSpinBox::setRange(lower, upper);
	setSingleStep(step);
}
void EqonomizeValueEdit::setPrecision(int precision) {
	if(precision == i_precision) return;
	i_precision = precision;
	setDecimals(precision);
}
void EqonomizeValueEdit::setCurrency(Currency *currency, bool keep_precision, int as_default, bool is_temporary) {

	if(is_temporary) o_currency = NULL;
	else o_currency = currency;
	
	if(!currency) {
		setSuffix(QString());
		setPrefix(QString());
		return;
	}
	
	if((as_default == 0 || (as_default < 0 && currency != budget->defaultCurrency())) || currency->symbol().isEmpty()) {
		setSuffix(QString(" ") + currency->code());
		setPrefix(QString());
	} else {
		if(currency && currency->symbolPrecedes()) setPrefix(currency->symbol());
		else setPrefix(QString());
		if(currency && !currency->symbolPrecedes()) setSuffix(" " + currency->symbol());
		else setSuffix(QString());
	}
	if(!keep_precision) {
		setDecimals(currency->fractionalDigits(true));
	}

}
QValidator::State EqonomizeValueEdit::validate(QString &input, int &pos) const {
	QValidator::State s = QDoubleSpinBox::validate(input, pos);
	if(s == QValidator::Invalid && (pos == 0 || (input[pos - 1] != '[' && input[pos - 1] != ']' && input[pos - 1] != '(' && input[pos - 1] != ')' && input[pos - 1] != ' '))) return QValidator::Intermediate;
	return s;
}
void EqonomizeValueEdit::fixup(QString &input) const {
	QString str = input.mid(prefix().length(), input.length() - prefix().length() - suffix().length()).trimmed();
	if(budget && o_currency) {
		int i = str.indexOf(QRegExp("[1234567890.,-+]"));
		if(i >= 1) {
			QString scur = str.left(i).trimmed();
			Currency *cur = budget->findCurrency(scur);
			if(!cur && budget->defaultCurrency()->symbol(false) == scur) cur = budget->defaultCurrency();
			if(!cur) cur = budget->findCurrencySymbol(scur, true);
			if(cur) {
				QString value = str.right(str.length() - i);
				fixup(value);
				input = QLocale().toString(cur->convertTo(QLocale().toDouble(value), o_currency), 'f', decimals());
				QDoubleSpinBox::fixup(input);
				return;
			}
		}
		i = str.lastIndexOf(QRegExp("[1234567890.,-+]"));
		if(i >= 0 && i < str.length() - 1) {
			QString scur = str.right(str.length() - (i + 1)).trimmed();
			Currency *cur = budget->findCurrency(scur);
			if(!cur && budget->defaultCurrency()->symbol(false) == scur) cur = budget->defaultCurrency();
			if(!cur) cur = budget->findCurrencySymbol(scur, true);
			if(cur) {
				QString value = str.left(i + 1);
				fixup(value);
				input = QLocale().toString(cur->convertTo(QLocale().toDouble(value), o_currency), 'f', decimals());
				QDoubleSpinBox::fixup(input);
				return;
			}
		}
	}	
	int i = str.indexOf(QRegExp("[-+]"), 1);
	if(i >= 1) {
		QStringList terms = str.split(QRegExp("[-+]"));
		QChar c = '+';
		i = 0;
		double v = 0.0;
		for(int terms_i = 0; terms_i < terms.size(); terms_i++) {
			if(terms[terms_i].isEmpty()) {
				if(i < str.length() && str[i] == '-') {
					if(c == '-') c = '+';
					else c = '-';
				}
			} else {
				i += terms[terms_i].length();
				fixup(terms[terms_i]);
				if(c == '-') v -= QLocale().toDouble(terms[terms_i]);
				else v += QLocale().toDouble(terms[terms_i]);
				if(i < str.length()) c = str[i];
			}
		}
		input = QLocale().toString(v, 'f', decimals());
		QDoubleSpinBox::fixup(input);
		return;
	}
	if(str.indexOf("**") >= 0) str = str.replace("**", "^");
	i = str.indexOf('*', 1);
	if(i >= 1 && i != str.length() - 1) {
		QString factor1 = str.left(i);
		QString factor2 = str.right(str.length() - (i + 1));
		qDebug() << factor1 << factor2;
		fixup(factor1);
		fixup(factor2);
		double v = QLocale().toDouble(factor1) * QLocale().toDouble(factor2);
		input = QLocale().toString(v, 'f', decimals());
		QDoubleSpinBox::fixup(input);
		return;
	}
	i = str.indexOf('/', 1);
	if(i >= 1 && i != str.length() - 1) {
		QString num = str.left(i);
		QString den = str.right(str.length() - (i + 1));
		fixup(num);
		fixup(den);
		double v = QLocale().toDouble(num) / QLocale().toDouble(den);
		input = QLocale().toString(v, 'f', decimals());
		QDoubleSpinBox::fixup(input);
		return;
	}
	i = str.indexOf('^', 1);
	if(i >= 1 && i != str.length() - 1) {
		QString base = str.left(i);
		QString exp = str.right(str.length() - (i + 1));
		fixup(base);
		fixup(exp);
		double v = pow(QLocale().toDouble(base), QLocale().toDouble(exp));
		input = QLocale().toString(v, 'f', decimals());
		QDoubleSpinBox::fixup(input);
		return;
	}
	QDoubleSpinBox::fixup(input);
}

