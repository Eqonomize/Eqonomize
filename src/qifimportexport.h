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

#ifndef QIF_IMPORT_EXPORT_H
#define QIF_IMPORT_EXPORT_H

#include <QMap>
#include <QString>
#include <QTextStream>
#include <QWizard>
#include <QDialog>

#include "budget.h"

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QTreeWidget;
class QTreeWidgetItem;

class Budget;
class AssetsAccount;

struct qif_info {
	AssetsAccount *current_account;
	QMap<QString, int> unknown_defs;
	QMap<QString, QString> unknown_defs_pre;
	bool unhandled;
	bool unknown;
	bool had_type, had_type_def, had_account_def;
	bool account_defined;
	bool had_transaction;
	int value_format, date_format, shares_format, price_format, percentage_format;
	char separator;
	bool p1, p2, p3, p4, ly;
	int lz;
	QString opening_balance_str;
	int accounts, categories, transactions, securities, security_transactions, duplicates, failed_transactions;
};

class ImportQIFDialog : public QWizard {

	Q_OBJECT
	
	protected:

		Budget *budget;
		qif_info qi;
		bool b_extra;
		int next_id;

		QLineEdit *fileEdit;
		QPushButton *fileButton;
		QTreeWidget *defsView;
		QComboBox *defsCombo, *dateFormatCombo, *accountCombo;
		QLineEdit *openingBalanceEdit;
		QCheckBox *ignoreDuplicateTransactionsButton;
		
	public:
		
		ImportQIFDialog(Budget *budg, QWidget *parent, bool extra_parameters);
		~ImportQIFDialog();
		
		void showPage(int index);
		int nextId() const;

	protected slots:

		void nextClicked();
		void accept();
		void onFileChanged(const QString&);
		void selectFile();
		void defSelectionChanged();
		void defSelected(int);
		
};

class ExportQIFDialog : public QDialog {

	Q_OBJECT

	protected:

		Budget *budget;
		qif_info qi;
		bool b_extra;

		QRadioButton *descriptionAsSubcategoryButton, *descriptionAsPayeeButton, *descriptionAsMemoButton, *descriptionIgnoreButton;
		QComboBox *accountCombo, *dateFormatCombo, *valueFormatCombo;
		QDialogButtonBox *buttonBox;
		QLineEdit *fileEdit;
		QPushButton *fileButton;

	public:

		ExportQIFDialog(Budget *budg, QWidget *parent, bool extra_parameters = false);
		~ExportQIFDialog();

	protected slots:
		
		void accept();
		void onFileChanged(const QString&);
		void selectFile();

};


void importQIF(QTextStream &fstream, bool test, qif_info &qi, Budget *budget, bool ignore_duplicates);
bool importQIFFile(Budget *budget, QWidget *parent, bool extra_parameters = false);

void exportQIF(QTextStream &fstream, qif_info &qi, Budget *budget, bool export_cats = true);
bool exportQIFFile(Budget *budget, QWidget *parent, bool extra_parameters = false);

#endif
