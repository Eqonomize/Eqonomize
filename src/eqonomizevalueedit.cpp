/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016 by Hanna Knutsson                 *
 *   hanna_k@fmgirl.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "eqonomizevalueedit.h"

#include <QDoubleSpinBox>
#include <QLayout>
#include <QLineEdit>
#include <QObject>
#include <QSpinBox>
#include <QVBoxLayout>

#include "budget.h"

#include <cmath>

class Eqonomize_QSpinBox : public QSpinBox {
	public:
		Eqonomize_QSpinBox(int lower, int upper, int value, QWidget *parent) : QSpinBox(parent) {
			setRange(lower, upper);
			setValue(value);
		}
		QLineEdit *lineEdit() const {
			return QAbstractSpinBox::lineEdit();
		}
};
class Eqonomize_KDoubleSpinBox : public QDoubleSpinBox {
	public:
		Eqonomize_KDoubleSpinBox(double lower, double upper, double step, double value, int precision, QWidget *parent) : QDoubleSpinBox(parent) {
			setRange(lower, upper);
			setSingleStep(step);
			setValue(value);
			setDecimals(precision);
		}
		QLineEdit *lineEdit() const {
			return QAbstractSpinBox::lineEdit();
		}
};

EqonomizeValueEdit::EqonomizeValueEdit(bool allow_negative, QWidget *parent) : QWidget(parent) {
	layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	init(allow_negative ? INT_MIN / pow(10, MONETARY_DECIMAL_PLACES) + 1.0 : 0.0, INT_MAX / pow(10, MONETARY_DECIMAL_PLACES) - 1.0, 1.0, 0.0, MONETARY_DECIMAL_PLACES, true);
}
EqonomizeValueEdit::EqonomizeValueEdit(double value, bool allow_negative, bool show_currency, QWidget *parent) : QWidget(parent) {
	layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	init(allow_negative ? INT_MIN / pow(10, MONETARY_DECIMAL_PLACES) + 1.0 : 0.0, INT_MAX / pow(10, MONETARY_DECIMAL_PLACES) - 1.0, 1.0, value, MONETARY_DECIMAL_PLACES, show_currency);
}
EqonomizeValueEdit::EqonomizeValueEdit(double value, int precision, bool allow_negative, bool show_currency, QWidget *parent) : QWidget(parent) {
	layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	init(allow_negative ? INT_MIN / pow(10, precision) + 1.0 : 0.0, INT_MAX / pow(10, precision) - 1.0, 1.0, value, precision, show_currency);
}
EqonomizeValueEdit::EqonomizeValueEdit(double lower, double upper, double step, double value, int precision, bool show_currency, QWidget *parent) : QWidget(parent) {
	layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	init(lower, upper, step, value, precision, show_currency);
}
EqonomizeValueEdit::~EqonomizeValueEdit() {}

void EqonomizeValueEdit::init(double lower, double upper, double step, double value, int precision, bool show_currency) {
	i_precision = precision;
	if(i_precision > 0) {
		valueEdit = new Eqonomize_KDoubleSpinBox(lower, upper, step, value, precision, this);
		lineEdit()->setAlignment(Qt::AlignRight);
	} else {
		valueEdit = new Eqonomize_QSpinBox((int) round(lower), (int) round(upper), (int) round(step), this);
		lineEdit()->setAlignment(Qt::AlignRight);
		setValue(value);
	}
	layout->addWidget(valueEdit);
	valueEdit->show();
	setFocusProxy(valueEdit);
	if(show_currency) {
		if(CURRENCY_IS_PREFIX) {
			if(i_precision > 0) ((QDoubleSpinBox*) valueEdit)->setPrefix(QLocale().currencySymbol() + " ");
			else ((QSpinBox*) valueEdit)->setPrefix(QLocale().currencySymbol() + " ");
		} else {
			if(i_precision > 0) ((QDoubleSpinBox*) valueEdit)->setSuffix(QString(" ") + QLocale().currencySymbol());
			else ((QSpinBox*) valueEdit)->setSuffix(QString(" ") + QLocale().currencySymbol());
		}
	}
	if(i_precision > 0) {
		connect((QDoubleSpinBox*) valueEdit, SIGNAL(valueChanged(double)), this, SIGNAL(valueChanged(double)));
	} else {
		connect((QSpinBox*) valueEdit, SIGNAL(valueChanged(int)), this, SLOT(onValueChanged(int)));
	}
	connect(valueEdit, SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));
}
void EqonomizeValueEdit::onEditingFinished() {
	if(hasFocus()) emit returnPressed();
}
QLineEdit *EqonomizeValueEdit::lineEdit() const {
	if(i_precision > 0) return ((Eqonomize_KDoubleSpinBox*) valueEdit)->lineEdit();
	return ((Eqonomize_QSpinBox*) valueEdit)->lineEdit();
}
void EqonomizeValueEdit::selectAll() {
	valueEdit->selectAll();
}
double EqonomizeValueEdit::value() const {
	if(i_precision > 0) return ((QDoubleSpinBox*) valueEdit)->value();
	else return (double) ((QSpinBox*) valueEdit)->value();
}
double EqonomizeValueEdit::maxValue() const {
	if(i_precision > 0) return ((QDoubleSpinBox*) valueEdit)->maximum();
	else return (double) ((QSpinBox*) valueEdit)->maximum();
}
void EqonomizeValueEdit::onValueChanged(int) {
	emit valueChanged((double) ((QSpinBox*) valueEdit)->value());
}
void EqonomizeValueEdit::setValue(double d_value) {
	if(i_precision > 0) ((QDoubleSpinBox*) valueEdit)->setValue(d_value);
	else ((QSpinBox*) valueEdit)->setValue((int) round(d_value));
}
void EqonomizeValueEdit::setMaxValue(double d_value) {
	if(i_precision > 0) ((QDoubleSpinBox*) valueEdit)->setMaximum(d_value);
	else ((QSpinBox*) valueEdit)->setMaximum((int) round(d_value));
}
void EqonomizeValueEdit::setRange(double lower, double upper, double step, int precision) {
	if(precision != i_precision && (precision == 0 || i_precision == 0)) {
		double d_value = value();
		bool show_currency = false;
		if(i_precision > 0) show_currency = !((QDoubleSpinBox*) valueEdit)->prefix().isEmpty() || !((QDoubleSpinBox*) valueEdit)->suffix().isEmpty();
		else show_currency = !((QSpinBox*) valueEdit)->prefix().isEmpty() || !((QSpinBox*) valueEdit)->suffix().isEmpty();
		delete valueEdit;
		init(lower, upper, step, d_value, precision, show_currency);
		return;
	}
	i_precision = precision;
	if(i_precision > 0) {
		((QDoubleSpinBox*) valueEdit)->setRange(lower, upper);
		((QDoubleSpinBox*) valueEdit)->setSingleStep(step);
		((QDoubleSpinBox*) valueEdit)->setDecimals(precision);
	} else {
		((QSpinBox*) valueEdit)->setRange((int) round(lower), (int) round(upper));
	}
}
void EqonomizeValueEdit::setPrecision(int precision) {
	if(precision == i_precision) return;
	if(precision == 0 || i_precision == 0) {
		double d_value = value();
		bool show_currency = false;
		if(i_precision > 0) show_currency = !((QDoubleSpinBox*) valueEdit)->prefix().isEmpty() || !((QDoubleSpinBox*) valueEdit)->suffix().isEmpty();
		else show_currency = !((QSpinBox*) valueEdit)->prefix().isEmpty() || !((QSpinBox*) valueEdit)->suffix().isEmpty();
		bool b_neg = false;
		double d_step;
		if(i_precision > 0) {
			b_neg = ((QDoubleSpinBox*) valueEdit)->minimum() < 0.0;
			d_step = ((QDoubleSpinBox*) valueEdit)->singleStep();
		} else {
			b_neg = ((QSpinBox*) valueEdit)->minimum() < 0;
			d_step = ((QSpinBox*) valueEdit)->singleStep();
		}		
		delete valueEdit;
		init(b_neg ? INT_MIN / pow(10, precision) + 1.0 : 0.0, INT_MAX / pow(10, precision) - 1.0, d_step, d_value, precision, show_currency);
	} else {
		bool b_neg = ((QDoubleSpinBox*) valueEdit)->minimum() < 0.0;
		double d_step = ((QDoubleSpinBox*) valueEdit)->singleStep();
		i_precision = precision;
		((QDoubleSpinBox*) valueEdit)->setRange(b_neg ? INT_MIN / pow(10, precision) + 1.0 : 0.0, INT_MAX / pow(10, precision) - 1.0);
		((QDoubleSpinBox*) valueEdit)->setSingleStep(d_step);
		((QDoubleSpinBox*) valueEdit)->setDecimals(precision);
	}
}
