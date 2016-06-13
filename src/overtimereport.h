/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016 by Hanna Knutsson                                        *
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

#ifndef OVER_TIME_REPORT_H
#define OVER_TIME_REPORT_H

#include <QWidget>

class QCheckBox;
class QComboBox;
class QPushButton;

class KHTMLPart;

class Account;
class Budget;

class OverTimeReport : public QWidget {

	Q_OBJECT

	public:

		OverTimeReport(Budget *budg, QWidget *parent);

	protected:

		Budget *budget;
		QString source;

		Account *current_account;
		QString current_description;
		int current_source;
		bool has_empty_description;
		
		KHTMLPart *htmlpart;
		QComboBox *sourceCombo, *categoryCombo, *descriptionCombo;
		QPushButton *saveButton, *printButton;
		QCheckBox *valueButton, *dailyButton, *monthlyButton, *yearlyButton, *countButton, *perButton;

	public slots:

		void sourceChanged(int);
		void categoryChanged(int);
		void descriptionChanged(int);
		void updateTransactions();
		void updateAccounts();
		void updateDisplay();
		void save();
		void print();
		void saveConfig();

};

#endif
