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

#include "eqonomizevalueedit.h"

#include "budget.h"

#include <cmath>
#include <cfloat>

#include <QLineEdit>
#include <QMessageBox>
#include <QDebug>
#include <QFocusEvent>
#include <QMenu>

#define MAX_VALUE 1000000000000.0

QString calculatedText;
QString last_error;
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
		if(i_precision < 0) i_precision = budget->defaultCurrency()->fractionalDigits(true);
		setCurrency(budget->defaultCurrency(), true);
	}
	if(i_precision < 0) i_precision = MONETARY_DECIMAL_PLACES;
	setDecimals(i_precision);
	setValue(value);
	connect(this, SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));
	QLineEdit *w = findChild<QLineEdit*>();
	if(w) connect(w, SIGNAL(textChanged(const QString&)), this, SLOT(onTextChanged(const QString&)));
	w->installEventFilter(this);
}
bool EqonomizeValueEdit::eventFilter(QObject *, QEvent *e) {
	if(e->type() == QMouseEvent::MouseButtonDblClick) {
		QLineEdit *w = findChild<QLineEdit*>();
		QString text = w->text();
		int pos = w->cursorPosition();
		if(pos >= text.length()) pos = text.length() - 1;
		if(!text[pos].isNumber() && !text[pos].isSpace() && !text[pos].isPunct()) return false;
		selectNumber();
		return true;
	}
	return false;
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
			if(text.length() > start && text[start] == ' ') start++;
		}
		if(!s_suffix.isEmpty() && text.endsWith(s_suffix)) {
			end -= s_suffix.length();
			if(end > 0 && text[end - 1] == ' ') end--;
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
		if(currency && currency->codePrecedes()) {
			s_prefix = currency->code();
			s_suffix = QString();
		} else {
			s_suffix = currency->code();
			s_prefix = QString();
		}
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
	QString input2 = input;
	if(o_currency) {
		input2.remove(budget->monetary_group_separator);
		input2.replace(budget->monetary_decimal_separator, QLocale().decimalPoint());
	} else {
		input2.remove(budget->group_separator);
		input2.replace(budget->decimal_separator, QLocale().decimalPoint());
	}
	int pos2 = pos;
	QValidator::State s = QDoubleSpinBox::validate(input2, pos2);
	if(s == QValidator::Invalid) {
		QString str = input2.trimmed();
		if(!s_suffix.isEmpty() && str.endsWith(s_suffix)) {
			str = str.left(str.length() - s_suffix.length());
		}
		if(!s_suffix.isEmpty()) {
			str.remove(s_suffix);
		}
		str = str.simplified();
		str.remove(' ');
		if(str != input2) {
			if(pos2 > str.length()) pos2 = str.length();
			if(QDoubleSpinBox::validate(str, pos2) == QValidator::Acceptable) return QValidator::Acceptable;
		}
		return QValidator::Intermediate;
	}
	return s;
}

QString EqonomizeValueEdit::textFromValue(double value) const {
	if(o_currency) return o_currency->formatValue(value, decimals(), true, false, true);
	if(!s_suffix.isEmpty()) return s_prefix + budget->formatValue(value, decimals()) + QString(" ") + s_suffix;
	return s_prefix + budget->formatValue(value, decimals());
}
double EqonomizeValueEdit::valueFromText(const QString &t) const {
	QString str = t.simplified();
	if(!s_suffix.isEmpty()) {
		str.remove(s_suffix);
	}
	if(!s_prefix.isEmpty()) {
		str.remove(s_prefix);
	}
	str.remove(' ');
	if(o_currency) {
		str.remove(budget->monetary_group_separator);
		str.replace(budget->monetary_decimal_separator, QLocale().decimalPoint());
	} else {
		str.remove(budget->group_separator);
		str.replace(budget->decimal_separator, QLocale().decimalPoint());
	}
	return QDoubleSpinBox::valueFromText(str);
}

void EqonomizeValueEdit::fixup(QString &input) const {
	if(!s_prefix.isEmpty() && input.startsWith(s_prefix)) {
		input = input.right(input.length() - s_prefix.length());
		if(input.isEmpty()) input = QString::number(1);
	}
	if(!s_suffix.isEmpty() && input.endsWith(s_suffix)) {
		input = input.left(input.length() - s_suffix.length());
		if(input.isEmpty()) input = QString::number(1);
	}
	QString calculatedText_pre = input.trimmed();
	input.replace("√", QString('^') + (o_currency ? o_currency->formatValue(0.5, -1, false, false, true) : budget->formatValue(0.5, 1)));
	input.remove(QRegExp("\\s"));
	if(o_currency) {
		input.remove(budget->monetary_group_separator);
		if(!budget->monetary_negative_sign.isEmpty()) input.replace(budget->monetary_negative_sign, "-");
		if(!budget->monetary_positive_sign.isEmpty()) input.replace(budget->monetary_positive_sign, "+");
	} else {
		input.remove(budget->group_separator);
		if(!budget->negative_sign.isEmpty()) input.replace(budget->negative_sign, "-");
		if(!budget->positive_sign.isEmpty()) input.replace(budget->positive_sign, "+");
	}
	input.replace("⋅", "*");
	input.replace("×", "*");
	input.replace("−", "-");
	input.replace("∕", "/");
	input.replace("÷", "/");
	input.replace("²", "^2");
	input.replace("³", "^3");
	input.replace(']', ')');
	input.replace('[', '(');
	QStringList errors;
	bool calculated = false;
	double v = fixup_sub(input, errors, calculated);
	if(o_currency) {
		input = o_currency->formatValue(v, decimals(), false, false, true);
	} else {
		input = budget->formatValue(v, decimals());
	}
	if(calculated && errors.isEmpty()) {
		calculatedText = calculatedText_pre;
		calculatedText_object = this;
	} else if(calculatedText_object == this) {
		calculatedText_object = NULL;
		calculatedText.clear();
	}
	if(!errors.isEmpty()) {
		QString error;
		for(int i = 0; i < errors.size(); i++) {
			if(i != 0) error += '\n';
			error += errors[i];
		}
		if(last_error == error) {
			last_error = "";
		} else {
			last_error = error;
			QMessageBox::critical((QWidget*) parent(), tr("Error"), error);
		}
	}
}
double EqonomizeValueEdit::fixup_sub(QString &input, QStringList &errors, bool &calculated) const {
	input = input.trimmed();
	if(input.isEmpty()) {
		return 0.0;
	}
	if(o_currency) {
		input.remove(budget->monetary_group_separator);
		if(!budget->monetary_negative_sign.isEmpty()) input.replace(budget->monetary_negative_sign, "-");
		if(!budget->monetary_positive_sign.isEmpty()) input.replace(budget->monetary_positive_sign, "+");
	} else {
		input.remove(budget->group_separator);
		if(!budget->negative_sign.isEmpty()) input.replace(budget->negative_sign, "-");
		if(!budget->positive_sign.isEmpty()) input.replace(budget->positive_sign, "+");
	}
	input.replace(QLocale().negativeSign(), '-');
	input.replace(QLocale().positiveSign(), '+');
	int i = input.indexOf(')', 1);
	if(i < 1) {
		i = input.indexOf('(', 0);
		if(i == 0) {
			input.remove(0, 1);
			return fixup_sub(input, errors, calculated);
		} else if(i >= 0) {
			input += ')';
			i = input.length() - 1;
		} else {
			i = -1;
		}
	}
	if(i >= 1) {
		int i2 = input.lastIndexOf('(', i - 1);
		if(i2 < 0 || i2 > i) {
			if(i == input.length() - 1) {
				input.chop(1);
				return fixup_sub(input, errors, calculated);
			}
			input.prepend('(');
			i++;
			i2 = 0;
		}
		if(i2 == 0 && i == input.length() - 1) {
			input.remove(0, 1);
			input.chop(1);
			return fixup_sub(input, errors, calculated);
		}
		if(i < input.length() - 1 && (input[i + 1].isNumber() || input[i + 1] == '(')) input.insert(i + 1, '*');
		QString str = input.mid(i2 + 1, i - i2 - 1);
		double v = fixup_sub(str, errors, calculated);
		input.replace(i2, i - i2 + 1, o_currency ? o_currency->formatValue(0.5, decimals() + 2, false, false, true) : budget->formatValue(v, decimals() + 2));
		if(i2 > 0 && (input[i2 - 1].isNumber() || input[i2 - 1] == ')')) input.insert(i2, '*');
		calculated = true;
		return fixup_sub(input, errors, calculated);
	}
	i = input.indexOf(QRegExp("[-+]"), 1);
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
			if(terms[terms_i].endsWith('*') || terms[terms_i].endsWith('/') || terms[terms_i].endsWith('^')) {
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
					if(!signs[terms_i]) v -= fixup_sub(terms[terms_i], errors, calculated);
					else v += fixup_sub(terms[terms_i], errors, calculated);
				}
			}			
			calculated = true;
			return v;
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
					errors << tr("Empty denominator.");
				} else {
					errors << tr("Empty factor.");
				}
			} else {
				i += terms[terms_i].length();
				if(c == '/') {
					double den = fixup_sub(terms[terms_i], errors, calculated);
					if(den == 0.0) {
						errors << tr("Division by zero.");
					} else {
						v /= den;
					}
				} else {
					v *= fixup_sub(terms[terms_i], errors, calculated);
				}
				if(i < input.length()) c = input[i];
			}
			i++;
		}
		calculated = true;
		return v;
	}
	i = input.indexOf(QLocale().percent());
	if(i >= 0) {
		double v = 0.01;
		if(input.length() > 1) {
			if(i > 0 && i < input.length() - 1) {
				QString str = input.right(input.length() - 1 - i);
				input = input.left(i);
				i = 0;
				v = fixup_sub(str, errors, calculated) * v;
			} else if(i == input.length() - 1) {
				input = input.left(i);
			} else if(i == 0) {
				input = input.right(input.length() - 1);
			}
			v = fixup_sub(input, errors, calculated) * v;
		}
		calculated = true;
		return v;
	}	
	if(o_currency) {
		QString reg_exp_str = "[\\d\\+\\-\\^";
		reg_exp_str += '\\';
		reg_exp_str += budget->monetary_decimal_separator;
		reg_exp_str += '\\';
		reg_exp_str += budget->monetary_group_separator;
		if(budget->monetary_decimal_separator != "." && budget->monetary_group_separator != ".") {
			reg_exp_str += '\\';
			reg_exp_str += '.';
		}
		reg_exp_str += "]";
		int i = input.indexOf(QRegExp(reg_exp_str));
		if(i >= 1) {
			QString scur = input.left(i).trimmed();
			Currency *cur = budget->findCurrency(scur);
			if(!cur && budget->defaultCurrency()->symbol(false) == scur) cur = budget->defaultCurrency();
			if(!cur) cur = budget->findCurrencySymbol(scur, true);
			if(cur) {
				QString value = input.right(input.length() - i);				
				double v = fixup_sub(value, errors, calculated);
				if(cur != o_currency) {
					v = cur->convertTo(v, o_currency);
				}
				calculated = true;
				return v;
			}
			errors << tr("Unknown or ambiguous currency, or unrecognized characters, in expression: %1.").arg(scur);
		}
		if(i >= 0) {
			i = input.lastIndexOf(QRegExp(reg_exp_str));
			if(i >= 0 && i < input.length() - 1) {
				QString scur = input.right(input.length() - (i + 1)).trimmed();
				Currency *cur = budget->findCurrency(scur);
				if(!cur && budget->defaultCurrency()->symbol(false) == scur) cur = budget->defaultCurrency();
				if(!cur) cur = budget->findCurrencySymbol(scur, true);
				if(cur) {
					QString value = input.left(i + 1);
					double v = fixup_sub(value, errors, calculated);
					if(cur != o_currency) {
						v = cur->convertTo(v, o_currency);
					}
					calculated = true;
					return v;
				}
				errors << tr("Unknown or ambiguous currency, or unrecognized characters, in expression: %1.").arg(scur);
			}
		} else {
			Currency *cur = budget->findCurrency(input);
			if(!cur && budget->defaultCurrency()->symbol(false) == input) cur = budget->defaultCurrency();
			if(!cur) cur = budget->findCurrencySymbol(input, true);
			if(cur) {
				double v = 1.0;
				if(cur != o_currency) {
					v = cur->convertTo(v, o_currency);
				}
				calculated = true;
				return v;
			}
			errors << tr("Unknown or ambiguous currency, or unrecognized characters, in expression: %1.").arg(input);
		}
	}
	i = input.indexOf('^', 0);
	if(i >= 0 && i != input.length() - 1) {
		QString base = input.left(i);
		if(base.isEmpty()) {
			errors << tr("Empty base.");
		} else {
			QString exp = input.right(input.length() - (i + 1));
			double v;
			if(exp.isEmpty()) {
				errors << tr("Error"), tr("Empty exponent.");
				v = 1.0;
			} else {
				v = pow(fixup_sub(base, errors, calculated), fixup_sub(exp, errors, calculated));
			}
			calculated = true;
			return v;
		}
	}
	
	if(!o_currency) {
		QString reg_exp_str = "[^\\d\\+\\-";
		reg_exp_str += '\\';
		reg_exp_str += budget->decimal_separator;
		reg_exp_str += '\\';
		reg_exp_str += budget->group_separator;
		if(budget->decimal_separator != "." && budget->group_separator != ".") {
			reg_exp_str += '\\';
			reg_exp_str += '.';
		}
		reg_exp_str += "]";
		i = input.indexOf(QRegExp(reg_exp_str));
		if(i >= 0) {
			errors << tr("Unrecognized characters in expression.");
		}
		input.remove(budget->group_separator);
		input.replace(budget->decimal_separator, ".");
		return input.toDouble();
	}
	input.remove(budget->monetary_group_separator);
	input.replace(budget->monetary_decimal_separator, ".");
	return input.toDouble();
}

EqonomizeCalendarWidget::EqonomizeCalendarWidget(QWidget *parent) : QCalendarWidget(parent) {
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(popupContextMenu(const QPoint&)));
	popupMenu = NULL;
}
void EqonomizeCalendarWidget::keyPressEvent(QKeyEvent *event) {
	if(event->key() == Qt::Key_Home) {
		setSelectedDate(QDate::currentDate());
		event->accept();
		return;
	}
}
void EqonomizeCalendarWidget::popupContextMenu(const QPoint &pos) {
	if(!popupMenu) {
		popupMenu = new QMenu(this);
		popupMenu->addAction(tr("Today"), this, SLOT(selectToday()));
	}
	popupMenu->popup(mapToGlobal(pos));
}
void EqonomizeCalendarWidget::selectToday() {
	setSelectedDate(QDate::currentDate());
}

EqonomizeDateEdit::EqonomizeDateEdit(QWidget *parent) : QDateEdit(QDate::currentDate(), parent) {
	setCalendarPopup(true);
	setCalendarWidget(new EqonomizeCalendarWidget(this));
	popupMenu = NULL;
}
EqonomizeDateEdit::EqonomizeDateEdit(const QDate &date, QWidget *parent) : QDateEdit(date, parent) {
	setCalendarPopup(true);
	setCalendarWidget(new EqonomizeCalendarWidget(this));
	popupMenu = NULL;
}
void EqonomizeDateEdit::keyPressEvent(QKeyEvent *event) {
	if(event->key() == Qt::Key_Down || event->key() == Qt::Key_Up) {
		if(currentSection() == QDateTimeEdit::DaySection) {
			QDate d = date();
			if(event->key() == Qt::Key_Up && d.day() == d.daysInMonth()) {
				setDate(d.addDays(1));
				event->accept();
				return;
			} else if(event->key() == Qt::Key_Down && d.day() == 1) {
				setDate(d.addDays(-1));
				event->accept();
				return;
			}
		} else if(currentSection() == QDateTimeEdit::MonthSection) {
			QDate d = date();
			if(event->key() == Qt::Key_Up && d.month() == 12) {
				setDate(d.addMonths(1));
				event->accept();
				return;
			} else if(event->key() == Qt::Key_Down && d.month() == 1) {
				setDate(d.addMonths(-1));
				event->accept();
				return;
			}
		}
	} else if((event->key() == Qt::Key_PageUp || event->key() == Qt::Key_PageDown)) {
		if(currentSection() == QDateTimeEdit::DaySection) {
			setDate(date().addMonths(event->key() == Qt::Key_PageUp ? 1 : -1));
			event->accept();
			return;
		} else if(currentSection() == QDateTimeEdit::MonthSection) {
			setDate(date().addYears(event->key() == Qt::Key_PageUp ? 1 : -1));
			event->accept();
			return;
		}
	} else if(event->key() == Qt::Key_Home && event->modifiers() == Qt::ControlModifier) {
		setDate(QDate::currentDate());
		event->accept();
		return;
	}
	QDateEdit::keyPressEvent(event);
	if(event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
		event->accept();
		emit returnPressed();
	}
}
void EqonomizeDateEdit::setToday() {
	setDate(QDate::currentDate());
}
void EqonomizeDateEdit::contextMenuEvent(QContextMenuEvent *event) {
	if(!popupMenu) {
		popupMenu = lineEdit()->createStandardContextMenu();
		popupMenu->addSeparator();
		todayAction = popupMenu->addAction(tr("Today"), this, SLOT(setToday()), Qt::CTRL | Qt::Key_Home);
	}
	todayAction->setEnabled(date() != QDate::currentDate());
	popupMenu->exec(event->globalPos());
}

