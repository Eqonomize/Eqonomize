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

#include <QLineEdit>
#include <QMessageBox>
#include <QDebug>

#define MAX_VALUE 1000000000000.0

QString calculatedText;
const EqonomizeValueEdit *calculatedText_object = NULL;

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
				s_prefix = QLocale().currencySymbol();
			} else {
				s_suffix = QLocale().currencySymbol();
			}
		}
	}
	if(i_precision < 0) i_precision = MONETARY_DECIMAL_PLACES;
	setDecimals(i_precision);
	setValue(value);
	connect(this, SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));
	QLineEdit *w = findChild<QLineEdit*>();
	if(w) connect(w, SIGNAL(textChanged(const QString&)), this, SLOT(onTextChanged(const QString&)));
}
void EqonomizeValueEdit::onTextChanged(const QString &text) {
	if(text == s_suffix) {
		QLineEdit *w = findChild<QLineEdit*>();
		if(w) {
			w->setCursorPosition(0);
		}
	}
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
void EqonomizeValueEdit::selectNumber() {
	if(s_prefix.isEmpty() && s_suffix.isEmpty()) {
		selectAll();
		return;
	}
	QLineEdit *w = findChild<QLineEdit*>();
	if(w) {
		QString text = w->text();
		int start = 0;
		int end = text.length();
		if(!s_prefix.isEmpty() && text.startsWith(s_prefix)) {
			start = s_prefix.length();
		}
		if(!s_suffix.isEmpty() && text.endsWith(QString(" ") + s_suffix)) {
			end -= (s_suffix.length() + 1);
		}		
		w->setSelection(end, start - end);
	}
}
void EqonomizeValueEdit::focusInEvent(QFocusEvent *e) {
	if(e->reason() == Qt::TabFocusReason || e->reason() == Qt::BacktabFocusReason) {
		QDoubleSpinBox::focusInEvent(e);
		selectNumber();
	} else {
		QDoubleSpinBox::focusInEvent(e);
	}
	
}
void EqonomizeValueEdit::enterFocus() {
	setFocus(Qt::TabFocusReason);
}
void EqonomizeValueEdit::setCurrency(Currency *currency, bool keep_precision, int as_default, bool is_temporary) {

	if(is_temporary) o_currency = NULL;
	else o_currency = currency;
	
	if(!currency) {
		s_suffix = QString();
		s_prefix = QString();
		setValue(value());
		return;
	}
	
	if((as_default == 0 || (as_default < 0 && currency != budget->defaultCurrency())) || currency->symbol().isEmpty()) {
		s_suffix = currency->code();
		s_prefix = QString();
	} else {
		if(currency && currency->symbolPrecedes()) s_prefix = currency->symbol();
		else s_prefix = QString();
		if(currency && !currency->symbolPrecedes()) s_suffix = currency->symbol();
		else s_suffix = QString();
	}
	if(!keep_precision) {
		setDecimals(currency->fractionalDigits(true));
	} else {
		setValue(value());
	}

}
Currency *EqonomizeValueEdit::currency() {return o_currency;}

QValidator::State EqonomizeValueEdit::validate(QString &input, int &pos) const {
	QValidator::State s = QDoubleSpinBox::validate(input, pos);
	if(s == QValidator::Invalid && (pos == 0 || (input[pos - 1] != '[' && input[pos - 1] != ']' && input[pos - 1] != '(' && input[pos - 1] != ')' && input[pos - 1] != ' '))) return QValidator::Intermediate;
	return s;
}

QString EqonomizeValueEdit::textFromValue(double value) const {
	if(!s_suffix.isEmpty()) return s_prefix + QLocale().toString(value, 'f', decimals()) + QString(" ") + s_suffix;
	return s_prefix + QLocale().toString(value, 'f', decimals());
}
double EqonomizeValueEdit::valueFromText(const QString &text) const {
	QString str = text;
	if(!s_suffix.isEmpty()) {
		str.remove(QString(" ") + s_suffix);
	}
	if(!s_prefix.isEmpty()) {
		str.remove(s_prefix);
	}
	return QDoubleSpinBox::valueFromText(str);
}

void EqonomizeValueEdit::fixup(QString &input) const {
	if(!s_prefix.isEmpty() && input.startsWith(s_prefix)) {
		input = input.right(input.length() - s_prefix.length());
		if(input.isEmpty()) input = QLocale().toString(1);
	}
	if(!s_suffix.isEmpty() && input.endsWith(s_suffix)) {
		input = input.left(input.length() - s_suffix.length());
		if(input.isEmpty()) input = QLocale().toString(1);
	}
	QString calculatedText_pre = input.trimmed();
	input.remove(QRegExp("\\s"));
	if(fixup_sub(input)) {
		calculatedText = calculatedText_pre;
		calculatedText_object = this;
	} else if(calculatedText_object == this) {
		calculatedText_object = NULL;
		calculatedText.clear();
	}
}
bool EqonomizeValueEdit::fixup_sub(QString &input) const {
	input = input.trimmed();
	if(input.isEmpty()) {
		input = QLocale().toString(0);
		QDoubleSpinBox::fixup(input);
		return false;
	}
	input.replace(QLocale().negativeSign(), '-');
	input.replace(QLocale().positiveSign(), '+');
	int i = input.indexOf(QRegExp("[-+]"), 1);
	if(i >= 1) {
		QStringList terms = input.split(QRegExp("[-+]"));
		i = 0;
		double v = 0.0;
		QList<bool> signs;
		signs << true;
		for(int terms_i = 0; terms_i < terms.size() - 1; terms_i++) {
			i += terms[terms_i].length();
			if(input[i] == '-') signs << false;
			else signs << true;
			i++;
		}
		for(int terms_i = 0; terms_i < terms.size() - 1; terms_i++) {
			if(terms_i <terms[terms_i].endsWith('*') || terms[terms_i].endsWith('/') || terms[terms_i].endsWith('^')) {
				if(!signs[terms_i + 1]) terms[terms_i] += '-';
				else terms[terms_i] += '+';
				terms[terms_i] += terms[terms_i + 1];
				signs.removeAt(terms_i + 1);
				terms.removeAt(terms_i + 1);
				terms_i--;
			}
		}
		if(terms.size() > 1) {
			for(int terms_i = 0; terms_i < terms.size(); terms_i++) {
				if(terms[terms_i].isEmpty()) {
					if(!signs[terms_i] && terms_i + 1 < terms.size()) {
						signs[terms_i + 1] = !signs[terms_i + 1];
					}
				} else {
					fixup_sub(terms[terms_i]);
					if(!signs[terms_i]) v -= QLocale().toDouble(terms[terms_i]);
					else v += QLocale().toDouble(terms[terms_i]);
				}
			}
			input = QLocale().toString(v, 'f', decimals());
			QDoubleSpinBox::fixup(input);
			return true;
		}
	}
	if(input.indexOf("**") >= 0) input.replace("**", "^");
	i = input.indexOf(QRegExp("[*/]"), 0);
	if(i >= 0) {
		QStringList terms = input.split(QRegExp("[*/]"));
		QChar c = '*';
		i = 0;
		double v = 1.0;
		for(int terms_i = 0; terms_i < terms.size(); terms_i++) {
			if(terms[terms_i].isEmpty()) {
				if(c == '/') {
					QMessageBox::critical((QWidget*) parent(), tr("Error"), tr("Empty denominator."));
				} else {
					QMessageBox::critical((QWidget*) parent(), tr("Error"), tr("Empty factor."));
				}
			} else {
				i += terms[terms_i].length();
				fixup_sub(terms[terms_i]);
				if(c == '/') {
					double den = QLocale().toDouble(terms[terms_i]);
					if(den == 0.0) {
						QMessageBox::critical((QWidget*) parent(), tr("Error"), tr("Division by zero."));
					} else {
						v /= den;
					}
				} else {
					v *= QLocale().toDouble(terms[terms_i]);
				}
				if(i < input.length()) c = input[i];
			}
			i++;
		}
		input = QLocale().toString(v, 'f', decimals());
		QDoubleSpinBox::fixup(input);
		return true;
	}
	i = input.indexOf(QLocale().percent());
	if(i >= 0) {
		double v = 0.01;
		if(input.length() > 1) {
			if(i > 0 && i < input.length() - 1) {
				QString str = input.right(input.length() - 1 - i);
				input = input.left(i);
				i = 0;
				fixup_sub(str);
				v = QLocale().toDouble(str) * v;
			} else if(i == input.length() - 1) {
				input = input.left(i);
				fixup_sub(input);
			} else if(i == 0) {
				input = input.right(input.length() - 1);
				fixup_sub(input);
			}
			v = QLocale().toDouble(input) * v;
		}
		input = QLocale().toString(v, 'f', decimals());
		QDoubleSpinBox::fixup(input);
		return true;
	}	
	if(budget && o_currency) {
		int i = input.indexOf(QRegExp(QString("[\\d+-\\^") + QLocale().decimalPoint() + QLocale().groupSeparator() + QString("]")));
		if(i >= 1) {
			QString scur = input.left(i).trimmed();
			Currency *cur = budget->findCurrency(scur);
			if(!cur && budget->defaultCurrency()->symbol(false) == scur) cur = budget->defaultCurrency();
			if(!cur) cur = budget->findCurrencySymbol(scur, true);
			if(cur) {
				QString value = input.right(input.length() - i);
				if(cur == o_currency) {
					fixup_sub(value);
					input = value;
				} else {
					fixup_sub(value);
					input = QLocale().toString(cur->convertTo(QLocale().toDouble(value), o_currency), 'f', decimals());
				}
				QDoubleSpinBox::fixup(input);
				return true;
			}
			QMessageBox::critical((QWidget*) parent(), tr("Error"), tr("Unknown or ambiguous currency, or unrecognized characters, in expression: %1.").arg(scur));
		}
		if(i >= 0) {
			i = input.lastIndexOf(QRegExp(QString("[\\d+-\\^") + QLocale().decimalPoint() + QLocale().groupSeparator() + QString("]")));
			if(i >= 0 && i < input.length() - 1) {
				QString scur = input.right(input.length() - (i + 1)).trimmed();
				Currency *cur = budget->findCurrency(scur);
				if(!cur && budget->defaultCurrency()->symbol(false) == scur) cur = budget->defaultCurrency();
				if(!cur) cur = budget->findCurrencySymbol(scur, true);
				if(cur) {
					QString value = input.left(i + 1);
					if(cur == o_currency) {
						fixup_sub(value);
						input = value;
					} else {
						fixup_sub(value);
						input = QLocale().toString(cur->convertTo(QLocale().toDouble(value), o_currency), 'f', decimals());
					}
					QDoubleSpinBox::fixup(input);
					return true;
				}
				QMessageBox::critical((QWidget*) parent(), tr("Error"), tr("Unknown or ambiguous currency, or unrecognized characters, in expression: %1.").arg(scur));
			}
		} else {
			Currency *cur = budget->findCurrency(input);
			if(!cur && budget->defaultCurrency()->symbol(false) == input) cur = budget->defaultCurrency();
			if(!cur) cur = budget->findCurrencySymbol(input, true);
			if(cur) {
				if(cur == o_currency) {
					input = QLocale().toString(1);
				} else {
					input = QLocale().toString(cur->convertTo(1, o_currency), 'f', decimals());
				}
				QDoubleSpinBox::fixup(input);
				return true;
			}
			QMessageBox::critical((QWidget*) parent(), tr("Error"), tr("Unknown or ambiguous currency, or unrecognized characters, in expression: %1.").arg(input));
		}
	}
	i = input.indexOf('^', 0);
	if(i >= 0 && i != input.length() - 1) {
		QString base = input.left(i);
		if(base.isEmpty()) {
			QMessageBox::critical((QWidget*) parent(), tr("Error"), tr("Empty base."));
		} else {
			QString exp = input.right(input.length() - (i + 1));
			if(exp.isEmpty()) {
				QMessageBox::critical((QWidget*) parent(), tr("Error"), tr("Empty exponent."));
				input = "1";
			} else {
				fixup_sub(base);
				fixup_sub(exp);
				double v = pow(QLocale().toDouble(base), QLocale().toDouble(exp));
				input = QLocale().toString(v, 'f', decimals());
			}
			QDoubleSpinBox::fixup(input);
			return true;
		}
	}
	
	if(!budget || !o_currency) {
		i = input.indexOf(QRegExp(QString("[^\\d+-") + QLocale().decimalPoint() + QLocale().groupSeparator() + QString("]")));
		if(i >= 0) {
			QMessageBox::critical((QWidget*) parent(), tr("Error"), tr("Unrecognized characters in expression."));
		}
	}
	input.replace('-', QLocale().negativeSign());
	input.replace('+', QLocale().positiveSign());
	QDoubleSpinBox::fixup(input);
	return false;
}

