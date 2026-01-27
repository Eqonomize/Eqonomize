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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <QButtonGroup>
#include <QCheckBox>
#include <QDateTime>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QMap>
#include <QRadioButton>
#include <QSpinBox>
#include <QTextStream>
#include <QVBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDateEdit>
#include <QMessageBox>
#include <QFileDialog>
#include <QPushButton>
#include <QMimeDatabase>
#include <QCompleter>
#include <QFileSystemModel>
#include <QSettings>

#include "budget.h"
#include "eqonomizevalueedit.h"
#include "eqonomize.h"
#include "accountcombobox.h"
#include "importcsvdialog.h"

#include <cmath>
#include <ctime>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
#	define DATE_TO_MSECS(d) QDateTime(d.startOfDay()).toMSecsSinceEpoch()
#else
#	define DATE_TO_MSECS(d) QDateTime(d).toMSecsSinceEpoch()
#endif

#define ALL_TYPES_ID		5

extern QString last_document_directory;

ImportCSVDialog::ImportCSVDialog(bool extra_parameters, Budget *budg, QWidget *parent) : QWizard(parent), b_extra(extra_parameters), budget(budg) {

	QSettings settings;

#ifdef _WIN32
	setWizardStyle(QWizard::ClassicStyle);
#endif

	setWindowTitle(tr("Import CSV file"));
	setModal(true);

	QIFWizardPage *page1 = new QIFWizardPage();
	page1->setTitle(tr("Transaction Type Selection"));
	setPage(0, page1);
	QVBoxLayout *layout1 = new QVBoxLayout(page1);
	typeGroup = new QButtonGroup(this);
	QVBoxLayout *typeGroup_layout = new QVBoxLayout();
	layout1->addLayout(typeGroup_layout);
	QRadioButton *rb = new QRadioButton(tr("Expenses"));
	rb->setChecked(true);
	typeGroup_layout->addWidget(rb);
	typeGroup->addButton(rb, 0);
	rb->setFocus();
	rb = new QRadioButton(tr("Incomes"));
	typeGroup_layout->addWidget(rb);
	typeGroup->addButton(rb, 1);
	rb = new QRadioButton(tr("Transfers"));
	typeGroup_layout->addWidget(rb);
	typeGroup->addButton(rb, 2);
	rb = new QRadioButton(tr("Expenses and incomes (negative cost)"));
	typeGroup_layout->addWidget(rb);
	typeGroup->addButton(rb, 3);
	rb = new QRadioButton(tr("Expenses and incomes (separate columns)"));
	typeGroup_layout->addWidget(rb);
	typeGroup->addButton(rb, 4);
	rb = new QRadioButton(tr("All types"));
	typeGroup_layout->addWidget(rb);
	typeGroup->addButton(rb, 5);
	typeDescriptionLabel = new QLabel(page1);
	typeDescriptionLabel->setWordWrap(true);
	layout1->addWidget(typeDescriptionLabel);
	layout1->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));

	QHBoxLayout *layoutPreset = new QHBoxLayout();
	presetLabel = new QLabel(tr("Presets:"));
	layoutPreset->addWidget(presetLabel);
	presetCombo = new QComboBox();
	presetCombo->setMinimumContentsLength(20);
	presetCombo->setEditable(false);
	presets = settings.value("GeneralOptions/CSVPresets").toMap();
	for(QMap<QString, QVariant>::const_iterator it = presets.constBegin(); it != presets.constEnd(); ++it) {
		presetCombo->addItem(it.key());
	}
	if(presetCombo->count() == 0) presetCombo->setEnabled(false);
	else presetCombo->setCurrentIndex(-1);
	layoutPreset->addWidget(presetCombo);
	layoutPreset->addStretch(1);
	layout1->addLayout(layoutPreset);

	QIFWizardPage *page2 = new QIFWizardPage();
	page2->setTitle(tr("File Selection"));
	setPage(1, page2);
	QGridLayout *layout2 = new QGridLayout(page2);

	layout2->addWidget(new QLabel(tr("File:"), page2), 0, 0);
	QHBoxLayout *layout2h = new QHBoxLayout();
	fileEdit = new QLineEdit(page2);
	QCompleter *completer = new QCompleter(this);
	QFileSystemModel *fsModel = new QFileSystemModel(completer);
	fsModel->setRootPath(QString());
	completer->setModel(fsModel);
	fileEdit->setCompleter(completer);
	layout2h->addWidget(fileEdit);
	fileButton = new QPushButton(LOAD_ICON("document-open"), QString(), page2);
	layout2h->addWidget(fileButton);
	layout2->addLayout(layout2h, 0, 1);
	layout2->addWidget(new QLabel(tr("First data row:"), page2), 1, 0);
	rowEdit = new QSpinBox(page2);
	rowEdit->setRange(0, 1000);
	rowEdit->setSpecialValueText(tr("Auto"));
	rowEdit->setValue(0);
	layout2->addWidget(rowEdit, 1, 1);
	layout2->addWidget(new QLabel(tr("Column delimiter:"), page2), 2, 0);
	delimiterCombo = new QComboBox(page2);
	delimiterCombo->setEditable(false);
	delimiterCombo->addItem(tr("Comma"));
	delimiterCombo->addItem(tr("Tabulator"));
	delimiterCombo->addItem(tr("Semicolon"));
	delimiterCombo->addItem(tr("Space"));
	delimiterCombo->addItem(tr("Other"));
	layout2->addWidget(delimiterCombo, 2, 1);
	delimiterEdit = new QLineEdit(page2);
	delimiterEdit->setEnabled(false);
	layout2->addWidget(delimiterEdit, 3, 1);
	layout2->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding), 4, 0, 1, 2);

	QIFWizardPage *page3 = new QIFWizardPage();
	page3->setTitle(tr("Columns Specification"));
	setPage(2, page3);
	QGridLayout *layout3 = new QGridLayout(page3);

	int row = 0;

	layout3->addWidget(new QLabel(tr("Date:"), page3), row, 0);
	dateGroup = new QButtonGroup(this);
	columnDateButton = new QRadioButton(tr("Column"), page3);
	dateGroup->addButton(columnDateButton);
	layout3->addWidget(columnDateButton, row, 1);
	columnDateEdit = new QSpinBox(page3);
	columnDateEdit->setRange(1, 100);
	columnDateEdit->setValue(1);
	columnDateButton->setChecked(true);
	layout3->addWidget(columnDateEdit, row, 2);
	valueDateButton = new QRadioButton(tr("Value"), page3);
	dateGroup->addButton(valueDateButton);
	layout3->addWidget(valueDateButton, row, 3);
	valueDateEdit = new EqonomizeDateEdit(QDate::currentDate(), page3);
	valueDateEdit->setCalendarPopup(true);
	valueDateEdit->setEnabled(false);
	layout3->addWidget(valueDateEdit, row, 4);
	row++;

	layout3->addWidget(new QLabel(tr("Description:", "Transaction description property (transaction title/generic article name)"), page3), row, 0);
	descriptionGroup = new QButtonGroup(this);
	columnDescriptionButton = new QRadioButton(tr("Column"), page3);
	descriptionGroup->addButton(columnDescriptionButton);
	layout3->addWidget(columnDescriptionButton, row, 1);
	columnDescriptionEdit = new QSpinBox(page3);
	columnDescriptionEdit->setRange(1, 100);
	columnDescriptionEdit->setValue(2);
	columnDescriptionButton->setChecked(true);
	layout3->addWidget(columnDescriptionEdit, row, 2);
	valueDescriptionButton = new QRadioButton(tr("Value"), page3);
	descriptionGroup->addButton(valueDescriptionButton);
	layout3->addWidget(valueDescriptionButton, row, 3);
	valueDescriptionEdit = new QLineEdit(page3);
	valueDescriptionEdit->setEnabled(false);
	layout3->addWidget(valueDescriptionEdit, row, 4);
	row++;

	costLabel = new QLabel(tr("Cost:"), page3);
	layout3->addWidget(costLabel, row, 0);
	costGroup = new QButtonGroup(this);
	columnCostButton = new QRadioButton(tr("Column"), page3);
	costGroup->addButton(columnCostButton);
	layout3->addWidget(columnCostButton, row, 1);
	columnCostEdit = new QSpinBox(page3);
	columnCostEdit->setRange(1, 100);
	columnCostEdit->setValue(3);
	columnCostButton->setChecked(true);
	layout3->addWidget(columnCostEdit, row, 2);
	valueCostButton = new QRadioButton(tr("Value"), page3);
	costGroup->addButton(valueCostButton);
	layout3->addWidget(valueCostButton, row, 3);
	valueCostEdit = new EqonomizeValueEdit(false, page3, budget);
	valueCostEdit->setEnabled(false);
	layout3->addWidget(valueCostEdit, row, 4);
	row++;

	valueLabel = new QLabel(tr("Cost:"), page3);
	layout3->addWidget(valueLabel, row, 0);
	valueGroup = new QButtonGroup(this);
	columnValueButton = new QRadioButton(tr("Column"), page3);
	valueGroup->addButton(columnValueButton);
	layout3->addWidget(columnValueButton, row, 1);
	columnValueEdit = new QSpinBox(page3);
	columnValueEdit->setRange(1, 100);
	columnValueEdit->setValue(3);
	columnValueButton->setChecked(true);
	layout3->addWidget(columnValueEdit, row, 2);
	valueValueButton = new QRadioButton(tr("Value"), page3);
	valueGroup->addButton(valueValueButton);
	layout3->addWidget(valueValueButton, row, 3);
	valueValueEdit = new EqonomizeValueEdit(false, page3, budget);
	valueValueEdit->setEnabled(false);
	layout3->addWidget(valueValueEdit, row, 4);
	row++;

	AC1Label = new QLabel(tr("Category:"), page3);
	layout3->addWidget(AC1Label, row, 0);
	AC1Group = new QButtonGroup(this);
	columnAC1Button = new QRadioButton(tr("Column"), page3);
	AC1Group->addButton(columnAC1Button);
	layout3->addWidget(columnAC1Button, row, 1);
	columnAC1Edit = new QSpinBox(page3);
	columnAC1Edit->setRange(1, 100);
	columnAC1Edit->setValue(4);
	columnAC1Edit->setEnabled(false);
	layout3->addWidget(columnAC1Edit, row, 2);
	valueAC1Button = new QRadioButton(tr("Value"), page3);
	AC1Group->addButton(valueAC1Button);
	valueAC1Button->setChecked(true);
	layout3->addWidget(valueAC1Button, row, 3);
	valueAC1Edit = new AccountComboBox(ACCOUNT_TYPE_EXPENSES, budget, true, false, false, true, true, page3, false);
	layout3->addWidget(valueAC1Edit, row, 4);
	row++;
	valueAC1IncomeEdit = new AccountComboBox(ACCOUNT_TYPE_INCOMES, budget, true, false, false, true, true, page3, false);
	layout3->addWidget(valueAC1IncomeEdit, row, 4);
	row++;

	AC2Label = new QLabel(tr("From account:"), page3);
	layout3->addWidget(AC2Label, row, 0);
	AC2Group = new QButtonGroup(this);
	columnAC2Button = new QRadioButton(tr("Column"), page3);
	AC2Group->addButton(columnAC2Button);
	layout3->addWidget(columnAC2Button, row, 1);
	columnAC2Edit = new QSpinBox(page3);
	columnAC2Edit->setRange(1, 100);
	columnAC2Edit->setValue(5);
	columnAC2Edit->setEnabled(false);
	layout3->addWidget(columnAC2Edit, row, 2);
	valueAC2Button = new QRadioButton(tr("Value"), page3);
	AC2Group->addButton(valueAC2Button);
	valueAC2Button->setChecked(true);
	layout3->addWidget(valueAC2Button, row, 3);
	valueAC2Edit = new AccountComboBox(ACCOUNT_TYPE_ASSETS, budget, true, false, false, true, true, page3, false);
	valueAC2Edit->setEditable(false);
	layout3->addWidget(valueAC2Edit, row, 4);
	row++;

	if(b_extra) {

		quantityLabel = new QLabel(tr("Quantity:"), page3);
		layout3->addWidget(quantityLabel, row, 0);
		quantityGroup = new QButtonGroup(this);
		columnQuantityButton = new QRadioButton(tr("Column"), page3);
		quantityGroup->addButton(columnQuantityButton);
		layout3->addWidget(columnQuantityButton, row, 1);
		columnQuantityEdit = new QSpinBox(page3);
		columnQuantityEdit->setRange(1, 100);
		columnQuantityEdit->setValue(6);
		columnQuantityEdit->setEnabled(false);
		layout3->addWidget(columnQuantityEdit, row, 2);
		valueQuantityButton = new QRadioButton(tr("Value"), page3);
		quantityGroup->addButton(valueQuantityButton);
		valueQuantityButton->setChecked(true);
		layout3->addWidget(valueQuantityButton, row, 3);
		valueQuantityEdit = new EqonomizeValueEdit(1.0, 2, false, false, page3, budget);
		layout3->addWidget(valueQuantityEdit, row, 4);
		row++;

		payeeLabel = new QLabel(tr("Payee:"), page3);
		layout3->addWidget(payeeLabel, row, 0);
		payeeGroup = new QButtonGroup(this);
		columnPayeeButton = new QRadioButton(tr("Column"), page3);
		payeeGroup->addButton(columnPayeeButton);
		layout3->addWidget(columnPayeeButton, row, 1);
		columnPayeeEdit = new QSpinBox(page3);
		columnPayeeEdit->setRange(1, 100);
		columnPayeeEdit->setValue(7);
		columnPayeeEdit->setEnabled(false);
		layout3->addWidget(columnPayeeEdit, row, 2);
		valuePayeeButton = new QRadioButton(tr("Value"), page3);
		payeeGroup->addButton(valuePayeeButton);
		valuePayeeButton->setChecked(true);
		layout3->addWidget(valuePayeeButton, row, 3);
		valuePayeeEdit = new QLineEdit(page3);
		layout3->addWidget(valuePayeeEdit, row, 4);
		row++;

	}

	tagsLabel = new QLabel(tr("Tags:"), page3);
	layout3->addWidget(tagsLabel, row, 0);
	tagsGroup = new QButtonGroup(this);
	columnTagsButton = new QRadioButton(tr("Column"), page3);
	tagsGroup->addButton(columnTagsButton);
	layout3->addWidget(columnTagsButton, row, 1);
	columnTagsEdit = new QSpinBox(page3);
	columnTagsEdit->setRange(1, 100);
	columnTagsEdit->setValue(b_extra ? 8 : 6);
	columnTagsEdit->setEnabled(false);
	layout3->addWidget(columnTagsEdit, row, 2);
	valueTagsButton = new QRadioButton(tr("Value"), page3);
	tagsGroup->addButton(valueTagsButton);
	valueTagsButton->setChecked(true);
	layout3->addWidget(valueTagsButton, row, 3);
	valueTagsEdit = new QLineEdit(page3);
	layout3->addWidget(valueTagsEdit, row, 4);
	row++;

	layout3->addWidget(new QLabel(tr("Comments:"), page3), row, 0);
	commentsGroup = new QButtonGroup(this);
	columnCommentsButton = new QRadioButton(tr("Column"), page3);
	commentsGroup->addButton(columnCommentsButton);
	layout3->addWidget(columnCommentsButton, row, 1);
	columnCommentsEdit = new QSpinBox(page3);
	columnCommentsEdit->setRange(1, 100);
	columnCommentsEdit->setValue(b_extra ? 8 : 6);
	columnCommentsEdit->setEnabled(false);
	layout3->addWidget(columnCommentsEdit, row, 2);
	valueCommentsButton = new QRadioButton(tr("Value"), page3);
	commentsGroup->addButton(valueCommentsButton);
	valueCommentsButton->setChecked(true);
	layout3->addWidget(valueCommentsButton, row, 3);
	valueCommentsEdit = new QLineEdit(page3);
	layout3->addWidget(valueCommentsEdit, row, 4);
	row++;

	QHBoxLayout *layout3_cm = new QHBoxLayout();
	layout3_cm->addStretch(1);
	createMissingButton = new QCheckBox(tr("Create missing categories and accounts"), page3);
	createMissingButton->setChecked(true);
	layout3_cm->addWidget(createMissingButton);
	layout3->addLayout(layout3_cm, row, 0, 1, 5);
	row++;

	QHBoxLayout *layout3_id = new QHBoxLayout();
	layout3_id->addStretch(1);
	ignoreDuplicateTransactionsButton = new QCheckBox(tr("Ignore duplicate transactions"), page3);
	ignoreDuplicateTransactionsButton->setChecked(false);
	layout3_id->addWidget(ignoreDuplicateTransactionsButton);
	layout3->addLayout(layout3_id, row, 0, 1, 5);
	row++;

	QHBoxLayout *layoutPreset2 = new QHBoxLayout();
	layoutPreset2->addStretch(1);
	savePresetButton = new QPushButton(tr("Save as preset…"), page3);
	layoutPreset2->addWidget(savePresetButton);
	layout3->addLayout(layoutPreset2, row, 0, 1, 5);
	row++;

	layout3->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding), row, 0, 1, 5);

	setOption(QWizard::HaveHelpButton, false);

	page3->setFinalPage(true);

	page1->setComplete(true);
	page2->setComplete(false);
	page3->setComplete(true);

	disconnect(button(NextButton), SIGNAL(clicked()), this, 0);
	connect(button(NextButton), SIGNAL(clicked()), this, SLOT(nextClicked()));

	typeChanged(0);

	connect(fileEdit, SIGNAL(textChanged(const QString&)), this, SLOT(onFileChanged(const QString&)));
	connect(fileButton, SIGNAL(clicked()), this, SLOT(selectFile()));
	connect(delimiterCombo, SIGNAL(activated(int)), this, SLOT(delimiterChanged(int)));
	connect(columnDescriptionButton, SIGNAL(toggled(bool)), columnDescriptionEdit, SLOT(setEnabled(bool)));
	connect(valueDescriptionButton, SIGNAL(toggled(bool)), valueDescriptionEdit, SLOT(setEnabled(bool)));
	connect(columnValueButton, SIGNAL(toggled(bool)), columnValueEdit, SLOT(setEnabled(bool)));
	connect(valueValueButton, SIGNAL(toggled(bool)), valueValueEdit, SLOT(setEnabled(bool)));
	connect(columnDateButton, SIGNAL(toggled(bool)), columnDateEdit, SLOT(setEnabled(bool)));
	connect(valueDateButton, SIGNAL(toggled(bool)), valueDateEdit, SLOT(setEnabled(bool)));
	connect(columnAC1Button, SIGNAL(toggled(bool)), columnAC1Edit, SLOT(setEnabled(bool)));
	connect(valueAC1Button, SIGNAL(toggled(bool)), valueAC1Edit, SLOT(setEnabled(bool)));
	connect(valueAC1Button, SIGNAL(toggled(bool)), valueAC1IncomeEdit, SLOT(setEnabled(bool)));
	connect(valueAC1Edit, SIGNAL(newAccountRequested()), valueAC1Edit, SLOT(createAccountSlot()));
	connect(valueAC1IncomeEdit, SIGNAL(newAccountRequested()), valueAC1IncomeEdit, SLOT(createAccountSlot()));
	connect(valueAC2Edit, SIGNAL(newAccountRequested()), valueAC2Edit, SLOT(createAccountSlot()));
	connect(columnAC2Button, SIGNAL(toggled(bool)), columnAC2Edit, SLOT(setEnabled(bool)));
	connect(valueAC2Button, SIGNAL(toggled(bool)), valueAC2Edit, SLOT(setEnabled(bool)));
	connect(columnTagsButton, SIGNAL(toggled(bool)), columnTagsEdit, SLOT(setEnabled(bool)));
	connect(valueTagsButton, SIGNAL(toggled(bool)), valueTagsEdit, SLOT(setEnabled(bool)));
	connect(columnCommentsButton, SIGNAL(toggled(bool)), columnCommentsEdit, SLOT(setEnabled(bool)));
	connect(valueCommentsButton, SIGNAL(toggled(bool)), valueCommentsEdit, SLOT(setEnabled(bool)));
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	connect(typeGroup, SIGNAL(idClicked(int)), this, SLOT(typeChanged(int)));
#else
	connect(typeGroup, SIGNAL(buttonClicked(int)), this, SLOT(typeChanged(int)));
#endif
	if(b_extra) {
		connect(columnQuantityButton, SIGNAL(toggled(bool)), columnQuantityEdit, SLOT(setEnabled(bool)));
		connect(valueQuantityButton, SIGNAL(toggled(bool)), valueQuantityEdit, SLOT(setEnabled(bool)));
		connect(columnPayeeButton, SIGNAL(toggled(bool)), columnPayeeEdit, SLOT(setEnabled(bool)));
		connect(valuePayeeButton, SIGNAL(toggled(bool)), valuePayeeEdit, SLOT(setEnabled(bool)));
	}
	connect(presetCombo, SIGNAL(activated(int)), this, SLOT(loadPreset(int)));
	connect(savePresetButton, SIGNAL(clicked()), this, SLOT(savePreset()));

	page3->adjustSize();
	page2->setMinimumWidth(page3->minimumSizeHint().width() + 100);
	page2->setMinimumHeight(page3->minimumSizeHint().height() + 100);
	page1->setMinimumSize(page2->minimumSize());

	costLabel->hide();
	valueCostEdit->hide();
	valueCostButton->hide();
	columnCostEdit->hide();
	columnCostButton->hide();

}

ImportCSVDialog::~ImportCSVDialog() {
}

void ImportCSVDialog::loadPreset(int index) {
	s_preset = presetCombo->itemText(index);
	if(!presets.contains(s_preset)) return;
	QList<QVariant> preset = presets[s_preset].toList();
	if(preset.count() < 4) return;
	typeGroup->button(preset.at(0).toInt())->setChecked(true);
	typeChanged(preset.at(0).toInt());
	fileEdit->setText(preset.at(1).toString());
	rowEdit->setValue(preset.at(2).toInt());
	QString delimiter = preset.at(3).toString();
	if(delimiter == ",") delimiterCombo->setCurrentIndex(0);
	else if(delimiter == "\t") delimiterCombo->setCurrentIndex(1);
	else if(delimiter == ";") delimiterCombo->setCurrentIndex(2);
	else if(delimiter == " ") delimiterCombo->setCurrentIndex(3);
	else {
		delimiterCombo->setCurrentIndex(4);
		delimiterEdit->setText(delimiter);
	}
	int i = 4;
	if(i >= preset.count()) return;
	if(preset.at(i).toBool()) {
		valueDateButton->setChecked(true);
		valueDateEdit->setDate(preset.at(i + 1).toDate());
	} else {
		columnDateButton->setChecked(true);
		columnDateEdit->setValue(preset.at(i + 1).toInt());
	}
	i += 2;
	if(i >= preset.count()) return;
	if(preset.at(i).toBool()) {
		valueDescriptionButton->setChecked(true);
		valueDescriptionEdit->setText(preset.at(i + 1).toString());
	} else {
		columnDescriptionButton->setChecked(true);
		columnDescriptionEdit->setValue(preset.at(i + 1).toInt());
	}
	i += 2;
	if(i >= preset.count()) return;
	if(preset.at(i).toBool()) {
		valueCostButton->setChecked(true);
		valueCostEdit->setValue(preset.at(i + 1).toDouble());
	} else {
		columnCostButton->setChecked(true);
		columnCostEdit->setValue(preset.at(i + 1).toInt());
	}
	i += 2;
	if(i >= preset.count()) return;
	if(preset.at(i).toBool()) {
		valueValueButton->setChecked(true);
		valueValueEdit->setValue(preset.at(i + 1).toDouble());
	} else {
		columnValueButton->setChecked(true);
		columnValueEdit->setValue(preset.at(i + 1).toInt());
	}
	i += 2;
	if(i >= preset.count()) return;
	if(preset.at(i).toBool()) {
		valueAC1Button->setChecked(true);
		if(!valueAC1Edit->setCurrentAccountId(preset.at(i + 1).toLongLong())) valueAC1Edit->setCurrentIndex(1);
		if(!valueAC1IncomeEdit->setCurrentAccountId(preset.at(i + 2).toLongLong())) valueAC1IncomeEdit->setCurrentIndex(1);
	} else {
		columnAC1Button->setChecked(true);
		columnAC1Edit->setValue(preset.at(i + 1).toInt());
	}
	i += 3;
	if(i >= preset.count()) return;
	if(preset.at(i).toBool()) {
		valueAC2Button->setChecked(true);
		if(!valueAC2Edit->setCurrentAccountId(preset.at(i + 1).toLongLong())) valueAC2Edit->setCurrentIndex(1);
	} else {
		columnAC2Button->setChecked(true);
		columnAC2Edit->setValue(preset.at(i + 1).toInt());
	}
	i += 2;
	if(i >= preset.count()) return;
	if(b_extra) {
		if(preset.at(i).toBool()) {
			valueQuantityButton->setChecked(true);
			valueQuantityEdit->setValue(preset.at(i + 1).toDouble());
		} else {
			columnQuantityButton->setChecked(true);
			columnQuantityEdit->setValue(preset.at(i + 1).toInt());
		}
		i += 2;
		if(preset.at(i).toBool()) {
			valuePayeeButton->setChecked(true);
			valuePayeeEdit->setText(preset.at(i + 1).toString());
		} else {
			columnPayeeButton->setChecked(true);
			columnPayeeEdit->setValue(preset.at(i + 1).toInt());
		}
		i += 2;
	} else {
		i += 4;
	}
	if(i >= preset.count()) return;
	if(preset.at(i).toBool()) {
		valueTagsButton->setChecked(true);
		valueTagsEdit->setText(preset.at(i + 1).toString());
	} else {
		columnTagsButton->setChecked(true);
		columnTagsEdit->setValue(preset.at(i + 1).toInt());
	}
	i += 2;
	if(i >= preset.count()) return;
	if(preset.at(i).toBool()) {
		valueCommentsButton->setChecked(true);
		valueCommentsEdit->setText(preset.at(i + 1).toString());
	} else {
		columnCommentsButton->setChecked(true);
		columnCommentsEdit->setValue(preset.at(i + 1).toInt());
	}
	i += 2;
	createMissingButton->setChecked(preset.at(i).toBool());
	i++;
	ignoreDuplicateTransactionsButton->setChecked(i < preset.count() && preset.at(i).toBool());
}
void ImportCSVDialog::savePreset() {
	QDialog *dialog = new QDialog(this);
	dialog->setWindowTitle(tr("Save Preset"));
	QVBoxLayout *box1 = new QVBoxLayout(dialog);
	QComboBox *presetEdit = new QComboBox(dialog);
	presetEdit->setMinimumContentsLength(20);
	presetEdit->setEditable(true);
	for(QMap<QString, QVariant>::const_iterator it = presets.constBegin(); it != presets.constEnd(); ++it) {
		presetEdit->addItem(it.key());
	}
	presetEdit->setCurrentText(s_preset);
	box1->addWidget(presetEdit);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, Qt::Horizontal, dialog);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	buttonBox->button(QDialogButtonBox::Cancel)->setAutoDefault(false);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), dialog, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(accept()));
	box1->addWidget(buttonBox);
	if(dialog->exec() == QDialog::Accepted) {
		QList<QVariant> preset;
		s_preset = presetEdit->currentText();
		preset << typeGroup->checkedId();
		preset << fileEdit->text();
		preset << rowEdit->value();
		switch(delimiterCombo->currentIndex()) {
			case 0: {preset << ","; break;}
			case 1: {preset << "\t"; break;}
			case 2: {preset << ";"; break;}
			case 3: {preset << " "; break;}
			default: {preset << delimiterEdit->text();}
		}
		if(valueDateButton->isChecked()) {
			preset << true;
			preset << valueDateEdit->date();
		} else {
			preset << false;
			preset << columnDateEdit->value();
		}
		if(valueDescriptionButton->isChecked()) {
			preset << true;
			preset << valueDescriptionEdit->text();
		} else {
			preset << false;
			preset << columnDescriptionEdit->value();
		}
		if(valueCostButton->isChecked()) {
			preset << true;
			preset << valueCostEdit->value();
		} else {
			preset << false;
			preset << columnCostEdit->value();
		}
		if(valueValueButton->isChecked()) {
			preset << true;
			preset << valueValueEdit->value();
		} else {
			preset << false;
			preset << columnValueEdit->value();
		}
		if(valueAC1Button->isChecked()) {
			preset << true;
			if(valueAC1Edit->currentAccount()) {
				preset << valueAC1Edit->currentAccount()->id();
			} else {
				preset << (qlonglong) 0;
			}
			if(valueAC1IncomeEdit->currentAccount()) {
				preset << valueAC1IncomeEdit->currentAccount()->id();
			} else {
				preset << (qlonglong) 0;
			}
		} else {
			preset << false;
			preset << columnAC1Edit->value();
			preset << 0;
		}
		if(valueAC2Button->isChecked()) {
			preset << true;
			if(valueAC2Edit->currentAccount()) {
				preset << valueAC2Edit->currentAccount()->id();
			} else {
				preset << (qlonglong) 0;
			}
		} else {
			preset << false;
			preset << columnAC2Edit->value();
		}
		if(b_extra) {
			if(valueQuantityButton->isChecked()) {
				preset << true;
				preset << valueQuantityEdit->value();
			} else {
				preset << false;
				preset << columnQuantityEdit->value();
			}
			if(valuePayeeButton->isChecked()) {
				preset << true;
				preset << valuePayeeEdit->text();
			} else {
				preset << false;
				preset << columnPayeeEdit->value();
			}
		} else {
			preset << true;
			preset << 1.0;
			preset << true;
			preset << QString();
		}
		if(valueTagsButton->isChecked()) {
			preset << true;
			preset << valueTagsEdit->text();
		} else {
			preset << false;
			preset << columnTagsEdit->value();
		}
		if(valueCommentsButton->isChecked()) {
			preset << true;
			preset << valueCommentsEdit->text();
		} else {
			preset << false;
			preset << columnCommentsEdit->value();
		}
		preset << createMissingButton->isChecked();
		preset << ignoreDuplicateTransactionsButton->isChecked();
		presets[s_preset] = preset;
		if(presetEdit->currentIndex() >= 0) {
			presetCombo->setCurrentIndex(presetEdit->currentIndex());
		} else {
			presetCombo->addItem(s_preset);
			presetCombo->setCurrentIndex(presetCombo->count() - 1);
			presetCombo->setEnabled(true);
		}
		QSettings settings;
		settings.beginGroup("GeneralOptions");
		settings.setValue("CSVPresets", presets);
	}
	dialog->deleteLater();
}
void ImportCSVDialog::onFileChanged(const QString &str) {
	((QIFWizardPage*) page(1))->setComplete(!str.isEmpty());
}
void ImportCSVDialog::selectFile() {
	QMimeDatabase db;
	QString url = QFileDialog::getOpenFileName(this, QString(), fileEdit->text().isEmpty() ? last_document_directory + "/" : fileEdit->text().trimmed(), db.mimeTypeForName("text/csv").filterString());
	if(!url.isEmpty()) fileEdit->setText(url);
}
void ImportCSVDialog::delimiterChanged(int index) {
	delimiterEdit->setEnabled(index == 4);
}
void ImportCSVDialog::typeChanged(int id) {

	valueAC1Button->setChecked(true);
	columnAC1Edit->setEnabled(false);
	valueAC1Edit->setEnabled(true);
	valueAC1Button->setEnabled(true);
	valueAC2Button->setChecked(true);
	columnAC2Edit->setEnabled(false);
	valueAC2Edit->setEnabled(true);
	valueAC2Button->setEnabled(true);
	createMissingButton->setEnabled(id != ALL_TYPES_ID);
	createMissingButton->setChecked(id != ALL_TYPES_ID);

	valueAC1Edit->clear();
	valueAC1IncomeEdit->clear();
	valueAC2Edit->clear();
	if(id < 5) valueAC2Edit->updateAccounts();
	if(b_extra) {
		quantityLabel->setVisible(id != 2);
		valueQuantityEdit->setVisible(id != 2);
		columnQuantityEdit->setVisible(id != 2);
		columnQuantityButton->setVisible(id != 2);
		valueQuantityButton->setVisible(id != 2);
		payeeLabel->setVisible(id != 2);
		valuePayeeEdit->setVisible(id != 2);
		columnPayeeEdit->setVisible(id != 2);
		columnPayeeButton->setVisible(id != 2);
		valuePayeeButton->setVisible(id != 2);
		valueQuantityButton->setChecked(id == 2);
		valuePayeeButton->setChecked(id == 2);
	}
	valueTagsButton->setChecked(id == 2);
	tagsLabel->setVisible(id != 2);
	valueTagsEdit->setVisible(id != 2);
	columnTagsEdit->setVisible(id != 2);
	columnTagsButton->setVisible(id != 2);
	valueTagsButton->setVisible(id != 2);
	valueAC1IncomeEdit->setVisible(id == 3 || id == 4);
	if(id == 4) {
		costLabel->show();
		valueCostEdit->show();
		columnCostEdit->show();
		columnCostButton->show();
		valueCostButton->show();
		columnCostButton->setChecked(true);
		valueCostButton->setEnabled(false);
		valueCostEdit->setEnabled(false);
		columnValueEdit->setValue(4);
		columnAC1Edit->setValue(5);
		columnAC2Edit->setValue(6);
		if(b_extra) {
			columnQuantityEdit->setValue(7);
			columnPayeeEdit->setValue(8);
			columnTagsEdit->setValue(9);
			columnCommentsEdit->setValue(10);
		} else {
			columnTagsEdit->setValue(7);
			columnCommentsEdit->setValue(8);
		}
	} else {
		costLabel->hide();
		valueCostEdit->hide();
		columnCostEdit->hide();
		columnCostButton->hide();
		valueCostButton->hide();
		columnValueEdit->setValue(3);
		columnAC1Edit->setValue(4);
		columnAC2Edit->setValue(5);
		if(id == 2) {
			columnCommentsEdit->setValue(6);
		} else if(b_extra) {
			columnQuantityEdit->setValue(6);
			columnPayeeEdit->setValue(7);
			columnTagsEdit->setValue(8);
			columnCommentsEdit->setValue(9);
		} else {
			columnTagsEdit->setValue(6);
			columnCommentsEdit->setValue(7);
		}
	}

	switch(id) {
		case 0: {
			typeDescriptionLabel->setText(tr("Imports data as expenses. Costs have positive value. Value is the only required column."));
			valueLabel->setText(tr("Cost:"));
			AC1Label->setText(tr("Category:"));
			AC2Label->setText(tr("From account:"));
			if(b_extra) payeeLabel->setText(tr("Payee:"));
			valueAC1Edit->setAccountType(ACCOUNT_TYPE_EXPENSES);
			valueAC1Edit->updateAccounts();
			break;
		}
		case 1: {
			typeDescriptionLabel->setText(tr("Imports data as incomes. Value is the only required column."));
			valueLabel->setText(tr("Income:"));
			AC1Label->setText(tr("Category:"));
			AC2Label->setText(tr("To account:"));
			if(b_extra) payeeLabel->setText(tr("Payer:"));
			valueAC1Edit->setAccountType(ACCOUNT_TYPE_INCOMES);
			valueAC1Edit->updateAccounts();
			break;
		}
		case 2: {
			typeDescriptionLabel->setText(tr("Imports data as transfers. Value is the only required column."));
			valueLabel->setText(tr("Amount:"));
			AC1Label->setText(tr("From account:"));
			AC2Label->setText(tr("To account:"));
			valueAC1Edit->setAccountType(ACCOUNT_TYPE_ASSETS);
			valueAC1Edit->updateAccounts();
			break;
		}
		case 3: {
			typeDescriptionLabel->setText(tr("Imports data as expenses and incomes. Costs have negative value. Value is the only required column."));
			valueLabel->setText(tr("Value:"));
			AC1Label->setText(tr("Category:"));
			AC2Label->setText(tr("Account:"));
			valueAC1Edit->setAccountType(ACCOUNT_TYPE_EXPENSES);
			valueAC1Edit->updateAccounts();
			valueAC1IncomeEdit->updateAccounts();
			if(b_extra) payeeLabel->setText(tr("Payee/payer:"));
			break;
		}
		case 4: {
			typeDescriptionLabel->setText(tr("Imports data as expenses and incomes. Costs and incomes have separate columns. Income and cost both all required columns."));
			valueLabel->setText(tr("Income:"));
			AC1Label->setText(tr("Category:"));
			AC2Label->setText(tr("Account:"));
			valueAC1Edit->setAccountType(ACCOUNT_TYPE_EXPENSES);
			valueAC1Edit->updateAccounts();
			valueAC1IncomeEdit->updateAccounts();
			if(b_extra) payeeLabel->setText(tr("Payee/payer:"));
			break;
		}
		case ALL_TYPES_ID: {
			typeDescriptionLabel->setText(tr("Imports data as expenses, incomes, and transfers. Costs have negative or positive value. Value, to, and from are all required columns. Accounts and categories must be existing."));
			columnAC1Button->setChecked(true);
			columnAC1Edit->setEnabled(true);
			valueAC1Edit->setEnabled(false);
			valueAC1Button->setEnabled(false);
			columnAC2Button->setChecked(true);
			columnAC2Edit->setEnabled(true);
			valueAC2Edit->setEnabled(false);
			valueAC2Button->setEnabled(false);
			valueLabel->setText(tr("Value:"));
			AC1Label->setText(tr("From:"));
			AC2Label->setText(tr("To:"));
			if(b_extra) payeeLabel->setText(tr("Payee/payer:"));
			break;
		}
	}
	columnValueButton->setChecked(true);
	valueValueButton->setEnabled(false);
	if(valueAC1Edit->count() == 0) {
		columnAC1Button->setChecked(true);
		columnAC1Edit->setEnabled(true);
		valueAC1Edit->setEnabled(false);
		valueAC1Button->setEnabled(false);
	}
	if((id == 3 || id == 4) && valueAC1IncomeEdit->count() == 0) {
		columnAC1Button->setChecked(true);
		columnAC1Edit->setEnabled(true);
		valueAC1Edit->setEnabled(false);
		valueAC1Button->setEnabled(false);
	}
	if(valueAC2Edit->count() == 0) {
		columnAC2Button->setChecked(true);
		columnAC2Edit->setEnabled(true);
		valueAC2Edit->setEnabled(false);
		valueAC2Button->setEnabled(false);
	}


}
void ImportCSVDialog::nextClicked() {
	if(currentPage() == page(0)) {
		fileEdit->setFocus();
	} else if(currentPage() == page(1)) {
		QString url = fileEdit->text().trimmed();
		if(url.isEmpty()) {
			QMessageBox::critical(this, tr("Error"), tr("A file must be selected."));
			fileEdit->setFocus();
			return;
		} else {
			QFileInfo info(url);
			if(info.isDir()) {
				QMessageBox::critical(this, tr("Error"), tr("Selected file is a directory."));
				fileEdit->setFocus();
				return;
			} else if(!info.exists()) {
				QMessageBox::critical(this, tr("Error"), tr("Selected file does not exist."));
				fileEdit->setFocus();
				return;
			}
		}
		if(delimiterCombo->currentIndex() == 4 && delimiterEdit->text().isEmpty()) {
			QMessageBox::critical(this, tr("Error"), tr("Empty delimiter."));
			delimiterEdit->setFocus();
			return;
		}
		columnDescriptionEdit->setFocus();
	}
	QWizard::next();
}

QDate readCSVDate(const QString &str, const QString &date_format, const QString &alt_date_format) {
	QDate date = QDate::fromString(str, date_format);
	if(!date.isValid() && !alt_date_format.isEmpty()) {
		date = QDate::fromString(str, alt_date_format);
		if(date.year() < 1970 && alt_date_format.count('y') < 4) {
			date = date.addYears(100);
		}
	} else if(date.year() < 1970 && date_format.count('y') < 4) {
		date = date.addYears(100);
	}
	return date;
}
double readCSVValue(const QString &str, int value_format, bool *ok) {
	QString str2 = str;
	int l = (int) str2.length();
	str2.replace(QLocale().negativeSign(), "-");
	str2.replace(QLocale().positiveSign(), "+");
	str2.replace(QChar(0x2212), "-");
	if(value_format == 2) {
		str2.replace(".", "");
		str2.replace(",", ".");
	} else if(value_format == 1) {
		str2.replace(",", "");
	}
	for(int i = 0; i < l; i++) {
		if(str2[i].isDigit() || str2[i] == '+' || str2[i] == '-' || str2[i] == '.') {
			if(i > 0) {
				str2.remove(0, i);
				l -= i;
			}
			break;
		}
	}
	l--;
	for(int i = l; i >= 0; i--) {
		if(str2[i].isDigit()) {
			if(i < l) {
				str2.truncate(i + 1);
			}
			break;
		}
	}
	return str2.toDouble(ok);
}

//p1 MDY
//p2 DMY
//p3 YMD
//p4 YDM
void testCSVDate(const QString &str, bool &p1, bool &p2, bool &p3, bool &p4, bool &ly, char &separator, int &lz) {
	if(separator < 1) {
		for(int i = 0; i < (int) str.length(); i++) {
			if(str[i] < '0' || str[i] > '9') {
				separator = str[i].toLatin1();
				break;
			}
		}
		if(separator < 1) separator = 1;
		p1 = (separator != 1 || str.length() == 6);
		p2 = (separator != 1 || str.length() == 6);
		p3 = true;
		p4 = (separator != 1 || str.length() == 6);
		ly = (separator == 1 && str.length() >= 8);
	}
	if(p1 + p2 + p3 + p4 <= 1) {
		lz = 1;
		return;
	}
	QStringList strlist;
	if(separator == 1) {
		strlist << str.left(2);
		strlist << str.mid(2, 2);
		strlist << str.right(2);
	} else {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
		strlist = str.split(separator, Qt::SkipEmptyParts);
#else
		strlist = str.split(separator, QString::SkipEmptyParts);
#endif
	}
	if(strlist.count() == 2 && (p1 || p2)) {
		int i = strlist[1].indexOf('\'');
		if(i >= 0) {
			strlist.append(strlist[1]);
			strlist[2].remove(0, i + 1);
			strlist[1].truncate(i);
			p3 = false;
			p4 = false;
			ly = false;
		}
	}
	if(strlist.count() < 3) return;
	if(p1 || p2) {
		int v1 = strlist[0].toInt();
		if(v1 > 12) p1 = false;
		if(v1 > 31 || v1 < 1) {
			p2 = false;
			if(v1 >= 100) ly = true;
			else ly = false;
		}
	}
	int v2 = strlist[1].toInt();
	if(v2 > 12) {p2 = false; p3 = false;}
	int v3 = strlist[2].toInt();
	if(v3 > 12) p4 = false;
	if(v3 > 31 || v3 < 1) {
		p3 = false;
		if(v3 >= 100) ly = true;
		else ly = false;
	}
	if(strlist[1].length() == 1) lz = 0;
	else if(strlist[1][0] == '0') lz = 1;
	else if(!p3 && !p4 && strlist[0].length() == 1) lz = 0;
	else if(!p3 && !p4 && strlist[0][0] == '0') lz = 1;
	else if(!p1 && !p2 && strlist[2].length() == 1) lz = 0;
	else if(!p1 && !p2 && strlist[2][0] == '0') lz = 1;
}

void testCSVValue(const QString &str, int &value_format) {
	if(value_format <= 0) {
		int i = str.lastIndexOf('.');
		int i2 = str.lastIndexOf(',');
		if(i2 >= 0 && i >= 0) {
			if(i2 > i) value_format = 2;
			else value_format = 1;
			return;
		}
		if(i >= 0) {
			i2 = 0;
			int l = (int) str.length();
			for(int index = i + 1; index < l; index++) {
				if(str[index].isDigit()) {
					i2++;
				} else {
					break;
				}
			}
			if(i2 < 3) value_format = 1;
			else value_format = -1;
		} else if(i2 >= 0) {
			i = 0;
			int l = (int) str.length();
			for(int index = i2 + 1; index < l; index++) {
				if(str[index].isDigit()) {
					i++;
				} else {
					break;
				}
			}
			if(i < 3) value_format = 2;
			else value_format = -1;
		}
	}
}

struct csv_info {
	int value_format;
	char separator;
	bool p1, p2, p3, p4, ly;
	int lz;
};


bool ImportCSVDialog::import(bool test, csv_info *ci) {

	QString date_format, alt_date_format;
	if(test) {
		ci->p1 = true;
		ci->p2 = true;
		ci->p3 = true;
		ci->p4 = true;
		ci->ly = true;
		ci->lz = -1;
		ci->value_format = 0;
		ci->separator = 0;
	} else {
		if(ci->p1) {
			date_format += ci->lz == 0 ? "M" : "MM";
			if(ci->separator > 1) date_format += ci->separator;
			date_format += ci->lz == 0 ? "d" : "dd";
			if(ci->separator > 1) date_format += ci->separator;
			if(ci->ly) {
				date_format += "yyyy";
			} else {
				if(ci->separator > 1) {
					alt_date_format = date_format;
					alt_date_format += '\'';
					alt_date_format += "yy";
				}
				date_format += "yy";
			}
		} else if(ci->p2) {
			date_format += ci->lz == 0 ? "d" : "dd";
			if(ci->separator > 1) date_format += ci->separator;
			date_format += ci->lz == 0 ? "M" : "MM";
			if(ci->separator > 1) date_format += ci->separator;
			if(ci->ly) {
				date_format += "yyyy";
			} else {
				if(ci->separator > 1) {
					alt_date_format = date_format;
					alt_date_format += '\'';
					alt_date_format += "yy";
				}
				date_format += "yy";
			}
		} else if(ci->p3) {
			if(ci->ly) date_format += "yyyy";
			else date_format += "yy";
			if(ci->separator > 1) date_format += ci->separator;
			date_format += ci->lz == 0 ? "M" : "MM";
			if(ci->separator > 1) date_format += ci->separator;
			date_format += ci->lz == 0 ? "d" : "dd";
		} else if(ci->p4) {
			if(ci->ly) date_format += "yyyy";
			else date_format += "yy";
			if(ci->separator > 1) date_format += ci->separator;
			date_format += ci->lz == 0 ? "d" : "dd";
			if(ci->separator > 1) date_format += ci->separator;
			date_format += ci->lz == 0 ? "M" : "MM";
		}
	}
	int first_row = rowEdit->value();
	int type = typeGroup->checkedId();
	QString delimiter;
	switch(delimiterCombo->currentIndex()) {
		case 0: {delimiter = ","; break;}
		case 1: {delimiter = "\t"; break;}
		case 2: {delimiter = ";"; break;}
		case 3: {delimiter = " "; break;}
		case 4: {delimiter = delimiterEdit->text(); break;}
	}
	int description_c = columnDescriptionButton->isChecked() ? columnDescriptionEdit->value() : -1;
	int value_c = columnValueButton->isChecked() ? columnValueEdit->value() : -1;
	int cost_c = (type == 4 && columnCostButton->isChecked()) ? columnCostEdit->value() : -1;
	int date_c = columnDateButton->isChecked() ? columnDateEdit->value() : -1;
	if(test && date_c < 0) {
		ci->p1 = true;
		ci->p2 = false;
		ci->p3 = false;
		ci->p4 = false;
		ci->ly = false;
		ci->lz = 1;
	}
	int AC1_c = columnAC1Button->isChecked() ? columnAC1Edit->value() : -1;
	int AC2_c = columnAC2Button->isChecked() ? columnAC2Edit->value() : -1;
	int comments_c = columnCommentsButton->isChecked() ? columnCommentsEdit->value() : -1;
	int tags_c = columnTagsButton->isChecked() ? columnTagsEdit->value() : -1;
	int payee_c = -1;
	if(b_extra && columnPayeeButton->isChecked()) payee_c = columnPayeeEdit->value();
	int quantity_c = -1;
	if(b_extra && columnQuantityButton->isChecked()) quantity_c = columnQuantityEdit->value();
	int ncolumns = 0, min_columns = 0;
	if(value_c > ncolumns) ncolumns = value_c;
	if(cost_c > ncolumns) ncolumns = cost_c;
	if(date_c > ncolumns) ncolumns = date_c;
	if(AC1_c > ncolumns) ncolumns = AC1_c;
	if(AC2_c > ncolumns) ncolumns = AC2_c;
	min_columns = ncolumns;
	if(description_c > ncolumns) ncolumns = description_c;
	if(comments_c > ncolumns) ncolumns = comments_c;
	if(tags_c > ncolumns) ncolumns = tags_c;
	if(payee_c > ncolumns) ncolumns = payee_c;
	if(quantity_c > ncolumns) ncolumns = quantity_c;
	if((description_c > 0 && (description_c == value_c || description_c == cost_c || description_c == date_c || description_c == AC1_c || description_c == AC2_c || description_c == comments_c || description_c == payee_c || description_c == quantity_c || description_c == tags_c))
		   || (value_c > 0 && (value_c == date_c || value_c == cost_c || value_c == AC1_c || value_c == AC2_c || value_c == comments_c || value_c == payee_c || value_c == quantity_c || value_c == tags_c))
		   || (cost_c > 0 && (cost_c == date_c || cost_c == AC1_c || cost_c == AC2_c || cost_c == comments_c || cost_c == payee_c || cost_c == quantity_c || cost_c == tags_c))
		   || (date_c > 0 && (date_c == AC1_c || date_c == AC2_c || date_c == comments_c || date_c == payee_c || date_c == quantity_c || date_c == tags_c))
		   || (AC1_c > 0 && (AC1_c == AC2_c || AC1_c == comments_c || AC1_c == payee_c || AC1_c == quantity_c || AC1_c == tags_c))
		   || (AC2_c > 0 && (AC2_c == comments_c || AC2_c == payee_c || AC2_c == quantity_c || AC2_c == tags_c))
		   || (comments_c > 0 && (comments_c == payee_c || comments_c == quantity_c || comments_c == tags_c))
		   || (payee_c > 0 && (payee_c == quantity_c || payee_c == tags_c))
		   || (tags_c > 0 && (tags_c == quantity_c))
	  ) {
		if(QMessageBox::warning(this, tr("Warning"), tr("The same column number is selected multiple times. Do you wish to proceed anyway?"), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
			return false;
		}
	}
	bool create_missing = createMissingButton->isChecked() && type != ALL_TYPES_ID;
	bool ignore_duplicates = ignoreDuplicateTransactionsButton->isChecked();
	QString description, comments, payee, tags;
	double quantity = 1.0;
	if(!test && description_c < 0) description = valueDescriptionEdit->text();
	if(!test && comments_c < 0) comments = valueCommentsEdit->text();
	if(!test && b_extra && quantity_c < 0) quantity = valueQuantityEdit->value();
	if(!test && tags_c < 0) tags = valueTagsEdit->text();
	QMap<QString, Account*> eaccounts, iaccounts, aaccounts;
	Account *ac1 = NULL, *ac1i = NULL, *ac2 = NULL;
	if(!test && (AC1_c >= 0 || AC2_c >= 0)) {
		for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
			ExpensesAccount *ea = *it;
			eaccounts[ea->nameWithParent()] = ea;
			if(ea->parentCategory() && !eaccounts.contains(ea->name())) eaccounts[ea->name()] = ea;
		}
		for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
			IncomesAccount *ia = *it;
			iaccounts[ia->nameWithParent()] = ia;
			if(ia->parentCategory() && !iaccounts.contains(ia->name())) iaccounts[ia->name()] = ia;
		}
		for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
			AssetsAccount *aa = *it;
			aaccounts[aa->name()] = aa;
		}
	}
	if(AC1_c < 0) {
		ac1 = valueAC1Edit->currentAccount();
		ac1i = valueAC1IncomeEdit->currentAccount();
		if(!ac1 || ((type == 3 || type == 4) && !ac1i)) {
			QMessageBox::critical(this, tr("Error"), tr("No account/category selected."));
			return false;
		}
	}
	if(AC2_c < 0) {
		if(valueAC2Edit->currentAccount()) ac2 = valueAC2Edit->currentAccount();
		if(!ac2) {
			QMessageBox::critical(this, tr("Error"), tr("No account/category selected."));
			return false;
		}
		if(ac1 == ac2) {
			QMessageBox::critical(this, tr("Error"), tr("Selected from account is the same as the to account."));
			return false;
		}
	}
	QDate date;
	if(date_c < 0) {
		date = valueDateEdit->date();
		if(!date.isValid()) {
			QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
			return false;
		}
	}

	double value = 0.0;
	if(value_c < 0) {
		value = valueValueEdit->value();
	}
	double cost = 0.0;

	QString url = fileEdit->text().trimmed();

	QFile file(url);
	if(!file.open(QIODevice::ReadOnly) ) {
		QMessageBox::critical(this, tr("Error"), tr("Couldn't open %1 for reading.").arg(url));
		return false;
	} else if(!file.size()) {
		QMessageBox::critical(this, tr("Error"), tr("Error reading %1.").arg(url));
		return false;
	}

	QFileInfo fileInfo(url);
	last_document_directory = fileInfo.absoluteDir().absolutePath();

	QTextStream fstream(&file);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	fstream.setCodec("UTF-8");
#endif

	//bool had_data = false;
	int successes = 0;
	int failed = 0;
	int duplicates = 0;
	bool missing_columns = false, value_error = false, date_error = false;
	bool AC1_empty = false, AC2_empty = false, AC1_missing = false, AC2_missing = false, AC_security = false, AC_balancing = false, AC_same = false;
	bool AC1_category = (type == 0 || type == 1 || type == 3 || type == 4);
	int AC1_c_bak = AC1_c;
	int AC2_c_bak = AC2_c;
	int row = 0;
	QString line = fstream.readLine();
	QString new_ac1 = "", new_ac2 = "";
	QDate curdate = QDate::currentDate();
	QMap<QDate, qint64> datestamps;
	while(!line.isNull()) {
		row++;
		if((first_row == 0 && !line.isEmpty() && line[0] != '#') || (first_row > 0 && row >= first_row && !line.isEmpty())) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
			QStringList columns = line.split(delimiter, Qt::KeepEmptyParts);
#else
			QStringList columns = line.split(delimiter, QString::KeepEmptyParts);
#endif
			for(QStringList::Iterator it = columns.begin(); it != columns.end(); ++it) {
				int i = 0;
				while(i < (int) (*it).length() && ((*it)[i] == ' ' || (*it)[i] == '\t')) {
					i++;
				}
				if(!(*it).isEmpty() && (*it)[i] == '\"') {
					(*it).remove(0, i + 1);
					i = (*it).length() - 1;
					while(i > 0 && ((*it)[i] == ' ' || (*it)[i] == '\t')) {
						i--;
					}
					if(i >= 0 && (*it)[i] == '\"') {
						(*it).truncate(i);
					} else {
						QStringList::Iterator it2 = it;
						++it2;
						while(it2 != columns.end()) {
							i = (*it2).length() - 1;
							while(i > 0 && ((*it2)[i] == ' ' || (*it2)[i] == '\t')) {
								i--;
							}
							if(i >= 0 && (*it2)[i] == '\"') {
								(*it2).truncate(i);
								*it += delimiter;
								*it += *it2;
								it2 = columns.erase(it2);
								it = it2;
								it--;
								break;
							}
							*it += delimiter;
							*it += *it2;
							it2 = columns.erase(it2);
							it = it2;
							it--;
						}
					}
				}
				*it = (*it).trimmed();
			}
			if((int) columns.count() < min_columns) {
				if(first_row != 0) {
					missing_columns = true;
					failed++;
				}
			} else {
				bool success = true;
				if(!test && success && description_c > 0) {
					description = columns[description_c - 1];
				}
				if(success && value_c > 0) {
					if(cost_c <= 0 || !columns[value_c - 1].isEmpty()) {
						bool ok = true;
						if(first_row == 0) {
							ok = false;
							QString &str = columns[value_c - 1];
							int l = (int) str.length();
							for(int i = 0; i < l; i++) {
								if(str[i].isDigit()) {
									ok = true;
									break;
								}
							}
						}
						if(!ok) {
							failed--;
							success = false;
						} else if(test) {
							if(ci->value_format <= 0) testCSVValue(columns[value_c - 1], ci->value_format);
						} else {
							value = readCSVValue(columns[value_c - 1], ci->value_format, &ok);
							if(!ok) {
								if(first_row == 0) failed--;
								else value_error = true;
								success = false;
							}
						}
					} else {
						value = 0.0;
					}
				}
				if(success && cost_c > 0) {
					if(value == 0.0 || !columns[cost_c - 1].isEmpty()) {
						bool ok = true;
						if(first_row == 0) {
							ok = false;
							QString &str = columns[cost_c - 1];
							int l = (int) str.length();
							for(int i = 0; i < l; i++) {
								if(str[i].isDigit()) {
									ok = true;
									break;
								}
							}
						}
						if(!ok) {
							failed--;
							success = false;
						} else if(test) {
							if(ci->value_format <= 0) testCSVValue(columns[cost_c - 1], ci->value_format);
						} else {
							cost = readCSVValue(columns[cost_c - 1], ci->value_format, &ok);
							if(!ok) {
								if(first_row == 0) failed--;
								else value_error = true;
								success = false;
							}
							value -= cost;
						}
					}
				}
				if(success && date_c > 0) {
					bool ok = true;
					if(first_row == 0) {
						ok = false;
						QString &str = columns[date_c - 1];
						for(int i = 0; i < (int) str.length(); i++) {
							if(str[i].isDigit()) {
								ok = true;
								break;
							}
						}
					}
					if(!ok) {
						failed--;
						success = false;
					} else if(test) {
						if(ci->p1 + ci->p2 + ci->p3 + ci->p4 > 1 || ci->lz < 0) testCSVDate(columns[date_c - 1], ci->p1, ci->p2, ci->p3, ci->p4, ci->ly, ci->separator, ci->lz);
					} else {
						date = readCSVDate(columns[date_c - 1], date_format, alt_date_format);
						if(!date.isValid()) {
							if(first_row == 0) failed--;
							else date_error = true;
							success = false;
						}
					}
				}
				if(success && first_row == 0) first_row = row;
				if(test && ci->p1 + ci->p2 + ci->p3 + ci->p4 < 2 && ci->lz >= 0 && ci->value_format > 0) break;
				if(test) success = false;
				if(success && type == ALL_TYPES_ID && value < 0.0) {
					AC1_c = AC2_c_bak;
					AC2_c = AC1_c_bak;
					value = -value;
				}
				if(success && AC1_c > 0) {
					if(AC1_category && columns[AC1_c - 1].isEmpty()) columns[AC1_c - 1] = tr("Uncategorized");
					QMap<QString, Account*>::iterator it_ac;
					bool found = false;
					if(type == 0 || ((type == 3 || type == 4) && value < 0.0)) {
						it_ac = eaccounts.find(columns[AC1_c - 1]);
						found = (it_ac != eaccounts.end());
					} else if(type == 1 || type == 3 || type == 4) {
						it_ac = iaccounts.find(columns[AC1_c - 1]);
						found = (it_ac != iaccounts.end());
					} else if(type == 2) {
						it_ac = aaccounts.find(columns[AC1_c - 1]);
						found = (it_ac != aaccounts.end());
					} else if(type == ALL_TYPES_ID) {
						it_ac = iaccounts.find(columns[AC1_c - 1]);
						found = (it_ac != iaccounts.end());
						if(!found) {
							it_ac = aaccounts.find(columns[AC1_c - 1]);
							found = (it_ac != aaccounts.end());
						}
					}
					if(found) {
						ac1 = it_ac.value();
						if(ac1->type() == ACCOUNT_TYPE_ASSETS && ((AssetsAccount*) ac1)->accountType() == ASSETS_TYPE_SECURITIES) {
							AC_security = true;
							success = false;
						} else if(type != 2 && type != ALL_TYPES_ID && ac1 == budget->balancingAccount) {
							AC_balancing = true;
							success = false;
						}
					} else if(columns[AC1_c - 1].isEmpty()) {
						AC1_empty = true;
						success = false;
					} else if(create_missing) {
						new_ac1 = columns[AC1_c - 1];
					} else {
						AC1_missing = true;
						success = false;
					}
				}
				if(success && AC2_c > 0) {
					QMap<QString, Account*>::iterator it_ac;
					bool found = false;
					if(type == ALL_TYPES_ID) {
						it_ac = eaccounts.find(columns[AC2_c - 1]);
						found = (it_ac != eaccounts.end());
						if(!found) {
							it_ac = aaccounts.find(columns[AC2_c - 1]);
							found = (it_ac != aaccounts.end());
						}
					} else {
						it_ac = aaccounts.find(columns[AC2_c - 1]);
						found = (it_ac != aaccounts.end());
					}
					if(found) {
						ac2 = it_ac.value();
						if(ac1 == ac2) {
							AC_same = true;
							success = false;
						} else if(ac2->type() == ACCOUNT_TYPE_ASSETS && ((AssetsAccount*) ac2)->accountType() == ASSETS_TYPE_SECURITIES) {
							AC_security = true;
							success = false;
						} else if(ac2 == budget->balancingAccount) {
							if(type != 2) {
								AC_balancing = true;
								success = false;
							} else {
								if(type == ALL_TYPES_ID && ac1->type() != ACCOUNT_TYPE_ASSETS) {
									it_ac = aaccounts.find(columns[AC1_c - 1]);
									found = it_ac != aaccounts.end();
									if(found) {
										ac1 = it_ac.value();
										if(ac1->type() == ACCOUNT_TYPE_ASSETS && ((AssetsAccount*) ac1)->accountType() == ASSETS_TYPE_SECURITIES) {
											AC_security = true;
											success = false;
										} else if(ac1 == budget->balancingAccount) {
											AC_same = true;
											success = false;
										}
									} else {
										AC_balancing = true;
										success = false;
									}
								}
								if(success) {
									value = -value;
									Account *ac1_bak = ac1;
									ac1 = ac2;
									ac2 = ac1_bak;
								}
							}
						} else if(type == ALL_TYPES_ID && ac1 == budget->balancingAccount && ac2->type() != ACCOUNT_TYPE_ASSETS) {
							it_ac = aaccounts.find(columns[AC2_c - 1]);
							found = it_ac != aaccounts.end();
							if(found) {
								ac2 = it_ac.value();
								if(ac2->type() == ACCOUNT_TYPE_ASSETS && ((AssetsAccount*) ac2)->accountType() == ASSETS_TYPE_SECURITIES) {
									AC_security = true;
									success = false;
								} else if(ac2 == budget->balancingAccount) {
									AC_same = true;
									success = false;
								}
							} else {
								AC_balancing = true;
								success = false;
							}
						}
					} else if(columns[AC2_c - 1].isEmpty()) {
						AC2_empty = true;
						success = false;
					} else if(create_missing) {
						new_ac2 = columns[AC2_c - 1];
						if(new_ac1 == new_ac2) {
							new_ac1 = "";
							new_ac2 = "";
							AC_same = true;
							success = false;
						}
					} else {
						AC2_missing = true;
						success = false;
					}
				}
				if(success && type == ALL_TYPES_ID) {
					AC1_c = AC1_c_bak;
					AC2_c = AC2_c_bak;
				}
				if(success && comments_c > 0) {
					comments = columns[comments_c - 1];
				}
				if(success && tags_c > 0) {
					tags = columns[tags_c - 1];
				}
				if(success && payee_c > 0) {
					payee = columns[payee_c - 1];
				}
				if(success && quantity_c > 0) {
					if(!columns[quantity_c - 1].isEmpty()) {
						bool ok = false;
						quantity = readCSVValue(columns[quantity_c - 1], ci->value_format, &ok);
						if(!ok) {
							quantity = 1.0;
						}
					} else {
						quantity = 1.0;
					}
				}
				if(success) {
					if(!new_ac1.isEmpty()) {
						if(type == 0 || ((type == 3 || type == 4) && value < 0.0)) {
							if(new_ac1.indexOf(':') > 0) {
								QString new_ac1a = new_ac1.section(':', 0, 0).trimmed();
								QString new_ac1b = new_ac1.section(':', 1).trimmed();
								Account *ac1a = NULL;
								if(!new_ac1a.isEmpty()) {
									QMap<QString, Account*>::iterator it_ac_a = eaccounts.find(new_ac1a);
									if(it_ac_a != eaccounts.end()) {
										ac1a = it_ac_a.value();
									} else {
										ac1a = new ExpensesAccount(budget, new_ac1a);
										budget->addAccount(ac1a);
										eaccounts[ac1a->name()] = ac1a;
									}
								}
								if(new_ac1b.isEmpty()) {
									ac1 = ac1a;
								} else {
									ac1 = new ExpensesAccount(budget, new_ac1b);
									((ExpensesAccount*) ac1)->setParentCategory((ExpensesAccount*) ac1a);
									budget->addAccount(ac1);
									eaccounts[ac1->nameWithParent()] = ac1;
									if(!eaccounts.contains(ac1->name())) eaccounts[ac1->name()] = ac1;
								}
							} else {
								ac1 = new ExpensesAccount(budget, new_ac1);
								budget->addAccount(ac1);
								eaccounts[ac1->name()] = ac1;
							}
						} else if(type == 1 || type == 3 || type == 4) {
							if(new_ac1.indexOf(':') > 0) {
								QString new_ac1a = new_ac1.section(':', 0, 0);
								QString new_ac1b = new_ac1.section(':', 1);
								Account *ac1a = NULL;
								QMap<QString, Account*>::iterator it_ac_a = iaccounts.find(new_ac1a);
								if(!new_ac1a.isEmpty()) {
									if(it_ac_a != iaccounts.end()) {
										ac1a = it_ac_a.value();
									} else {
										ac1a = new IncomesAccount(budget, new_ac1a);
										budget->addAccount(ac1a);
										iaccounts[ac1a->name()] = ac1a;
									}
								}
								if(new_ac1b.isEmpty()) {
									ac1 = ac1a;
								} else {
									ac1 = new IncomesAccount(budget, new_ac1b);
									((IncomesAccount*) ac1)->setParentCategory((IncomesAccount*) ac1a);
									budget->addAccount(ac1);
									iaccounts[ac1->nameWithParent()] = ac1;
									if(!iaccounts.contains(ac1->name())) iaccounts[ac1->name()] = ac1;
								}
							} else {
								ac1 = new IncomesAccount(budget, new_ac1);
								budget->addAccount(ac1);
								iaccounts[ac1->name()] = ac1;
							}
						} else if(type == 2) {
							ac1 = new AssetsAccount(budget, ASSETS_TYPE_CASH, new_ac1);
							budget->addAccount(ac1);
							aaccounts[ac1->name()] = ac1;
						}
						new_ac1 = "";
					}
					if(!new_ac2.isEmpty()) {
						ac2 = new AssetsAccount(budget, ASSETS_TYPE_CASH, new_ac2);
						budget->addAccount(ac2);
						aaccounts[ac2->name()] = ac2;
						new_ac2 = "";
					}
					Transaction *trans = NULL;
					switch(type) {
						case 0: {
							trans = new Expense(budget, value, date, (ExpensesAccount*) ac1, (AssetsAccount*) ac2, description, comments);
							((Expense*) trans)->setPayee(payee);
							successes++;
							break;
						}
						case 1: {
							trans = new Income(budget, value, date, (IncomesAccount*) ac1, (AssetsAccount*) ac2, description, comments);
							((Income*) trans)->setPayer(payee);
							successes++;
							break;
						}
						case 2: {
							if(ac1 == budget->balancingAccount) {
								trans = new Balancing(budget, value, date, (AssetsAccount*) ac2, description);
							} else if(value < 0.0) {
								trans = new Transfer(budget, -value, date, (AssetsAccount*) ac2, (AssetsAccount*) ac1, description, comments);
							} else {
								trans = new Transfer(budget, value, date, (AssetsAccount*) ac1, (AssetsAccount*) ac2, description, comments);
							}
							successes++;
							break;
						}
						case 3: {}
						case 4: {
							if(value < 0.0) {
								trans = new Expense(budget, -value, date, (ExpensesAccount*) ac1, (AssetsAccount*) ac2, description, comments);
								((Expense*) trans)->setPayee(payee);
							} else {
								trans = new Income(budget, value, date, AC1_c < 0 ? (IncomesAccount*) ac1i : (IncomesAccount*) ac1, (AssetsAccount*) ac2, description, comments);
								((Income*) trans)->setPayer(payee);
							}
							successes++;
							break;
						}
						case ALL_TYPES_ID: {
							if(ac1 == budget->balancingAccount) {
								trans = new Balancing(budget, value, date, (AssetsAccount*) ac2, description);
							} else if(ac1->type() == ACCOUNT_TYPE_INCOMES) {
								trans = new Income(budget, value, date, (IncomesAccount*) ac1, (AssetsAccount*) ac2, description, comments);
								((Income*) trans)->setPayer(payee);
							} else if(ac2->type() == ACCOUNT_TYPE_EXPENSES) {
								trans = new Expense(budget, value, date, (ExpensesAccount*) ac2, (AssetsAccount*) ac1, description, comments);
								((Expense*) trans)->setPayee(payee);
							} else {
								trans = new Transfer(budget, value, date, (AssetsAccount*) ac1, (AssetsAccount*) ac2, description, comments);
							}
							successes++;
							break;
						}
					}
					if(trans) {
						trans->readTags(tags);
						trans->setQuantity(quantity);
						if(ignore_duplicates && budget->findDuplicateTransaction(trans)) {
							duplicates++;
							successes--;
							delete trans;
						} else if(trans->date() > curdate) {
							trans->setTimestamp(datestamps.contains(QDate::currentDate()) ? datestamps[QDate::currentDate()] + 1 : DATE_TO_MSECS(QDate::currentDate()) / 1000);
							datestamps[QDate::currentDate()] = trans->timestamp();
							budget->addScheduledTransaction(new ScheduledTransaction(budget, trans, NULL));
						} else {
							trans->setTimestamp(datestamps.contains(trans->date()) ? datestamps[trans->date()] + 1 : DATE_TO_MSECS(trans->date()) / 1000);
							datestamps[trans->date()] = trans->timestamp();
							budget->addTransaction(trans);
						}
					}
				} else {
					failed++;
				}
			}
		}
		line = fstream.readLine();
	}

	file.close();

	if(test) {
		return true;
	}

	QString info = "", details = "";
	if(successes > 0) {
		info = tr("Successfully imported %n transaction(s).", "", successes);
	} else {
		info = tr("Unable to import any transactions.");
	}
	if(duplicates > 0) {
		info += "\n";
		info += tr("%n duplicate transaction(s) was ignored.", "", duplicates);
	}
	if(failed > 0) {
		info += '\n';
		info += tr("Failed to import %n data row(s).", "", failed);
		if(missing_columns) {details += "\n-"; details += tr("Required columns missing.");}
		if(value_error) {details += "\n-"; details += tr("Invalid value.");}
		if(date_error) {details += "\n-"; details += tr("Invalid date.");}
		if(AC1_empty) {details += "\n-"; if(AC1_category) {details += tr("Empty category name.");} else {details += tr("Empty account name.");}}
		if(AC2_empty) {details += "\n-"; details += tr("Empty account name.");}
		if(AC1_missing) {details += "\n-"; if(AC1_category) {details += tr("Unknown category found.");} else {details += tr("Unknown account found.");}}
		if(AC2_missing) {details += "\n-"; details += tr("Unknown account found.");}
		if(AC_security) {details += "\n-"; details += tr("Cannot import security transactions (to/from security accounts).");}
		if(AC_balancing) {details += "\n-"; details += tr("Balancing account wrongly used.", "Referring to the account used for adjustments of account balances.");}
		if(AC_same) {details += "\n-"; details += tr("Same to and from account/category.");}
	} else if(successes == 0 && duplicates == 0) {
		info = tr("No data found.");
	}
	if(failed > 0 || successes == 0) {
		QMessageBox::critical(this, tr("Error"), info + details);
	} else {
		QMessageBox::information(this, tr("Information"), info);
	}
	return successes > 0;
}
void ImportCSVDialog::accept() {
	csv_info ci;
	if(!import(true, &ci)) return;
	int ps = ci.p1 + ci.p2 + ci.p3 + ci.p4;
	if(ps == 0) {
		QMessageBox::critical(this, tr("Error"), tr("Unrecognized date format."));
		return;
	}
	if(ci.value_format < 0 || ps > 1) {
		QDialog *dialog = new QDialog(this);
		dialog->setWindowTitle(tr("Specify Format"));
		dialog->setModal(true);
		QVBoxLayout *box1 = new QVBoxLayout(dialog);
		QGridLayout *grid = new QGridLayout();
		box1->addLayout(grid);
		QLabel *label = new QLabel(tr("The format of dates and/or numbers in the CSV file is ambiguous. Please select the correct format."), dialog);
		label->setWordWrap(true);
		grid->addWidget(label, 0, 0, 1, 2);
		QComboBox *dateFormatCombo = NULL;
		if(ps > 1) {
			grid->addWidget(new QLabel(tr("Date format:"), dialog), 1, 0);
			dateFormatCombo = new QComboBox(dialog);
			dateFormatCombo->setEditable(false);
			if(ci.p1) {
				QString date_format = ci.lz == 0 ? "M" : "MM";;
				if(ci.separator > 1) date_format += ci.separator;
				date_format += ci.lz == 0 ? "D" : "DD";
				if(ci.separator > 1) date_format += ci.separator;
				date_format += "YY";
				if(ci.ly) date_format += "YY";
				dateFormatCombo->addItem(date_format);
			}
			if(ci.p2) {
				QString date_format = ci.lz == 0 ? "D" : "DD";
				if(ci.separator > 1) date_format += ci.separator;
				date_format += ci.lz == 0 ? "M" : "MM";
				if(ci.separator > 1) date_format += ci.separator;
				date_format += "YY";
				if(ci.ly) date_format += "YY";
				dateFormatCombo->addItem(date_format);
			}
			if(ci.p3) {
				QString date_format = "YY";
				if(ci.ly) date_format += "YY";
				if(ci.separator > 1) date_format += ci.separator;
				date_format += ci.lz == 0 ? "M" : "MM";
				if(ci.separator > 1) date_format += ci.separator;
				date_format += ci.lz == 0 ? "D" : "DD";
				dateFormatCombo->addItem(date_format);
			}
			if(ci.p4) {
				QString date_format = "YY";
				if(ci.ly) date_format += "YY";
				if(ci.separator > 1) date_format += ci.separator;
				date_format += ci.lz == 0 ? "D" : "DD";
				if(ci.separator > 1) date_format += ci.separator;
				date_format += ci.lz == 0 ? "M" : "MM";
				dateFormatCombo->addItem(date_format);
			}
			grid->addWidget(dateFormatCombo, 1, 1);
		}
		QComboBox *valueFormatCombo = NULL;
		if(ci.value_format < 0) {
			grid->addWidget(new QLabel(tr("Value format:"), dialog), ps > 1 ? 2 : 1, 0);
			valueFormatCombo = new QComboBox(dialog);
			valueFormatCombo->setEditable(false);
			valueFormatCombo->addItem("1,000,000.00");
			valueFormatCombo->addItem("1.000.000,00");
			grid->addWidget(valueFormatCombo, ps > 1 ? 2 : 1, 1);
		}
		QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, Qt::Horizontal, dialog);
		buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
		buttonBox->button(QDialogButtonBox::Cancel)->setAutoDefault(false);
		buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
		connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), dialog, SLOT(reject()));
		connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(accept()));
		box1->addWidget(buttonBox);
		if(dialog->exec() != QDialog::Accepted) {
			return;
		}
		if(ps > 1) {
			bool p1 = false, p2 = false, p3 = false, p4 = false;
			int p[4];
			int i = 0;
			if(ci.p1) {p[i] = 1; i++;}
			if(ci.p2) {p[i] = 2; i++;}
			if(ci.p3) {p[i] = 3; i++;}
			if(ci.p4) {p[i] = 4; i++;}
			switch(p[dateFormatCombo->currentIndex()]) {
				case 1: {p1 = true; break;}
				case 2: {p2 = true; break;}
				case 3: {p3 = true; break;}
				case 4: {p4 = true; break;}
			}
			ci.p1 = p1; ci.p2 = p2; ci.p3 = p3; ci.p4 = p4;
			if(ci.lz < 0) ci.lz = 1;
		}
		if(ci.value_format < 0) ci.value_format = valueFormatCombo->currentIndex() + 1;
		dialog->deleteLater();
	}
	if(import(false, &ci)) QWizard::accept();
}
