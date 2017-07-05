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


#ifndef CURRENCY_CONVERSION_DIALOG_H
#define CURRENCY_CONVERSION_DIALOG_H

#include <QDialog>

class QComboBox;

class Budget;
class EqonomizeValueEdit;

class CurrencyConversionDialog : public QDialog {

	Q_OBJECT
	
	protected:

		EqonomizeValueEdit *fromEdit, *toEdit;
		QComboBox *fromCombo, *toCombo;
		Budget *budget;
		
	public:
		
		CurrencyConversionDialog(Budget *budg, QWidget *parent);
		
		void updateCurrencies();

	public slots:

		void convertFrom();
		void convertTo();

};

#endif
