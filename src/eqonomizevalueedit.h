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

#ifndef EQONOMIZE_VALUE_EDIT_H
#define EQONOMIZE_VALUE_EDIT_H

#include <QDoubleSpinBox>

class EqonomizeValueEdit : public QDoubleSpinBox {

	Q_OBJECT

	public:

		EqonomizeValueEdit(bool allow_negative = true, QWidget *parent = 0);
		EqonomizeValueEdit(double value, bool allow_negative, bool show_currency, QWidget *parent);
		EqonomizeValueEdit(double value, int precision, bool allow_negative, bool show_currency, QWidget *parent);
		EqonomizeValueEdit(double lower, double step, double value, int precision, bool show_currency, QWidget *parent);
		EqonomizeValueEdit(double lower, double upper, double step, double value, int precision, bool show_currency, QWidget *parent);
		~EqonomizeValueEdit();
		void init(double lower, double upper, double step, double value, int precision, bool show_currency);
		
		void setRange(double lower, double step, int precision);
		void setRange(double lower, double upper, double step, int precision);
		void setPrecision(int precision);

	protected:
		
		int i_precision;
	
	protected slots:
	
		void onEditingFinished();

	signals:

		void returnPressed();

};


#endif
