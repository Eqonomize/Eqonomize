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

#ifndef EQONOMIZE_VALUE_EDIT_H
#define EQONOMIZE_VALUE_EDIT_H

#include <QDoubleSpinBox>
#include <QStringList>
#include <QDateEdit>

class Currency;
class Budget;

class EqonomizeValueEdit : public QDoubleSpinBox {

	Q_OBJECT

	public:

		EqonomizeValueEdit(bool allow_negative, QWidget *parent, Budget *budg);
		EqonomizeValueEdit(double value, bool allow_negative, bool show_currency, QWidget *parent, Budget *budg);
		EqonomizeValueEdit(double value, int precision, bool allow_negative, bool show_currency, QWidget *parent, Budget *budg);
		EqonomizeValueEdit(double lower, double step, double value, int precision, bool show_currency, QWidget *parent, Budget *budg);
		EqonomizeValueEdit(double lower, double upper, double step, double value, int precision, bool show_currency, QWidget *parent, Budget *budg);
		~EqonomizeValueEdit();
		void init(double lower, double upper, double step, double value, int precision, bool show_currency);
		
		void setRange(double lower, double step, int precision);
		void setRange(double lower, double upper, double step, int precision);
		void setPrecision(int precision);
		
		void setCurrency(Currency *currency, bool keep_precision = false, int as_default = -1, bool is_temporary = false);
		Currency *currency();
		
		virtual void fixup(QString &input) const;
		virtual QValidator::State validate(QString &input, int &pos) const;
		virtual QString textFromValue(double value) const;
		virtual double valueFromText(const QString &text) const;		

	protected:
		
		int i_precision;
		Budget *budget;
		Currency *o_currency;
		QString s_prefix, s_suffix;
		
		double fixup_sub(QString &input, QStringList &errors, bool &calculated) const;
		virtual void focusInEvent(QFocusEvent*);
		
		bool eventFilter(QObject*, QEvent*);
		
	public slots:
	
		void enterFocus();
		void selectNumber();
	
	protected slots:
	
		void onEditingFinished();
		void onTextChanged(const QString&);

	signals:

		void returnPressed();

};

class EqonomizeDateEdit : public QDateEdit {

	Q_OBJECT

	public:
	
		EqonomizeDateEdit(QWidget *parent = NULL);
		EqonomizeDateEdit(const QDate &date, QWidget *parent = NULL);
	
	protected slots:
	
		void keyPressEvent(QKeyEvent *event);
		
	signals:
	
		void returnPressed();

};

extern QString calculatedText;
extern const EqonomizeValueEdit *calculatedText_object;

#endif
