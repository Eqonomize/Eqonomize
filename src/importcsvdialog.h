/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016 by Hanna Knutsson                 *
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

#ifndef IMPORT_CSV_DIALOG_H
#define IMPORT_CSV_DIALOG_H

#include <QWizard>

class QButtonGroup;
class QCheckBox;
class QLabel;
class QRadioButton;
class QSpinBox;
class QComboBox;
class QLineEdit;
class QDateEdit;
class QPushButton;

class Budget;
class EqonomizeValueEdit;

struct csv_info;

class ImportCSVDialog : public QWizard {

	Q_OBJECT
	
	protected:

		bool b_extra;
		Budget *budget;

		QLabel *typeDescriptionLabel;
		QButtonGroup *typeGroup, *dateGroup, *valueGroup, *costGroup, *descriptionGroup, *AC1Group, *AC2Group, *commentsGroup, *payeeGroup, *quantityGroup, *tagsGroup;

		QLineEdit *fileEdit;
		QPushButton *fileButton;
		QSpinBox *rowEdit;
		QComboBox *delimiterCombo;
		QLineEdit *delimiterEdit;

		QRadioButton *columnDescriptionButton, *valueDescriptionButton;
		QSpinBox *columnDescriptionEdit;
		QLineEdit *valueDescriptionEdit;

		QLabel *valueLabel;
		QRadioButton *columnValueButton, *valueValueButton;
		QSpinBox *columnValueEdit;
		EqonomizeValueEdit *valueValueEdit;

		QLabel *costLabel;
		QRadioButton *columnCostButton, *valueCostButton;
		QSpinBox *columnCostEdit;
		EqonomizeValueEdit *valueCostEdit;

		QRadioButton *columnDateButton, *valueDateButton;
		QSpinBox *columnDateEdit;
		QDateEdit *valueDateEdit;

		QLabel *AC1Label;
		QRadioButton *columnAC1Button, *valueAC1Button;
		QSpinBox *columnAC1Edit;
		QComboBox *valueAC1Edit;

		QLabel *AC2Label;
		QRadioButton *columnAC2Button, *valueAC2Button;
		QSpinBox *columnAC2Edit;
		QComboBox *valueAC2Edit;

		QRadioButton *columnCommentsButton, *valueCommentsButton;
		QSpinBox *columnCommentsEdit;
		QLineEdit *valueCommentsEdit;
		
		QLabel *tagsLabel;
		QRadioButton *columnTagsButton, *valueTagsButton;
		QSpinBox *columnTagsEdit;
		QLineEdit *valueTagsEdit;
		
		QLabel *payeeLabel;
		QRadioButton *columnPayeeButton, *valuePayeeButton;
		QSpinBox *columnPayeeEdit;
		QLineEdit *valuePayeeEdit;
		
		QLabel *quantityLabel;
		QRadioButton *columnQuantityButton, *valueQuantityButton;
		QSpinBox *columnQuantityEdit;
		EqonomizeValueEdit *valueQuantityEdit;

		QCheckBox *createMissingButton;

		bool import(bool test, csv_info *ci);
		
	public:
		
		ImportCSVDialog(bool extra_parameters, Budget *budg, QWidget *parent);
		~ImportCSVDialog();		

	protected slots:

		void onFileChanged(const QString&);
		void selectFile();
		void nextClicked();
		void typeChanged(int);
		void delimiterChanged(int);
		void accept();
		
};

#endif
