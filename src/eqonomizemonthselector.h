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

#ifndef EQONOMIZE_MONTH_SELECTOR_H
#define EQONOMIZE_MONTH_SELECTOR_H

#include <QDateTime>
#include <QWidget>

class QComboBox;
class QSpinBox;

class EqonomizeMonthSelector : public QWidget {

	Q_OBJECT

	public:

		EqonomizeMonthSelector(QWidget *parent);
		~EqonomizeMonthSelector();
		
		QDate date() const;

	protected:

		QComboBox *monthCombo;
		QSpinBox *yearEdit;

		QDate d_date;

		void updateMonths();

	public slots:

		void setDate(const QDate&);
		void focusMonth();
		void focusYear();
		void setMonthEnabled(bool);

	protected slots:
		
		void onYearChanged(int);
		void onMonthChanged(int);

	signals:

		void dateChanged(const QDate&);
		void yearChanged(const QDate&);
		void monthChanged(const QDate&);

};


#endif
