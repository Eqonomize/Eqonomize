/***************************************************************************
 *   Copyright (C) 2017 by Hanna Knutsson                                  *
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


#ifndef EDIT_CURRENCY_DIALOG_H
#define EDIT_CURRENCY_DIALOG_H

#include <QDialog>

class QCheckBox;
class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QRadioButton;
class QSpinBox;

class Currency;
class Budget;
class EqonomizeValueEdit;

class EditCurrencyDialog : public QDialog {

	Q_OBJECT
	
	protected:

		QLineEdit *codeEdit, *symbolEdit, *nameEdit;
		QDateEdit *dateEdit;
		EqonomizeValueEdit *rateEdit;
		QCheckBox *defaultButton;
		QRadioButton *prefixButton, *suffixButton, *psDefaultButton;
		QSpinBox *decimalsEdit;
		Budget *budget;
		Currency *currency;
		
	public:
		
		EditCurrencyDialog(Budget *budg, Currency *cur, bool enable_set_as_default, QWidget *parent);

		void modifyCurrency(Currency*);
		Currency *createCurrency();

	protected slots:

		void accept();
		void currencyChanged();

};

#endif
