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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "overtimechart.h"

#ifdef QT_CHARTS_LIB
#include <QtCharts/QLegendMarker>
#include <QtCharts/QBarLegendMarker>
#include <QGraphicsItem>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QHorizontalBarSeries>
#include <QtCharts/QStackedBarSeries>
#include <QtCharts/QCategoryAxis>
#include <QtCharts/QBarCategoryAxis>
#include <QFont>
#else
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsSimpleTextItem>
#endif
#include <QButtonGroup>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <qimage.h>
#include <QImageWriter>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMatrix>
#include <QObject>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QPrinter>
#include <QPrintDialog>
#include <QRadioButton>
#include <QResizeEvent>
#include <QString>
#include <QVBoxLayout>
#include <QVector>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QCheckBox>
#include <QDialog>
#include <QUrl>
#include <QFileDialog>
#include <QSaveFile>
#include <QApplication>
#include <QTemporaryFile>
#include <QMimeDatabase>
#include <QMimeType>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>

#include "account.h"
#include "budget.h"
#include "eqonomizemonthselector.h"
#include "recurrence.h"
#include "transaction.h"

#include <cmath>

struct chart_month_info {
	double value;
	double count;
	QDate date;
};
extern QString last_picture_directory;

void calculate_minmax_lines(double &maxvalue, double &minvalue, int &y_lines, int &y_minor, bool minmaxequal = false, bool use_deciminor = true) {
	if(-minvalue > maxvalue) maxvalue = -minvalue;
	int maxvalue_exp = floor(log10(maxvalue));
	int maxvalue_digit;
	double orig_maxvalue = maxvalue;
	if(maxvalue_exp < 1.0) {
		maxvalue_digit = ceil(maxvalue);
		maxvalue = maxvalue_digit;
		y_lines = maxvalue_digit;
		maxvalue_exp = 0.0;
	} else {			
		maxvalue /= pow(10, maxvalue_exp);
		if(maxvalue <= 1.0) {
			maxvalue = 10.0;
			maxvalue_exp -= 1.0;
		}
		maxvalue_digit = ceil(maxvalue);
		y_lines = maxvalue_digit;
		maxvalue = maxvalue_digit * pow(10, maxvalue_exp);
		if(!minmaxequal && y_lines <= 5 && orig_maxvalue <= (maxvalue / (y_lines * 2)) * ((y_lines * 2) - 1)) {
			maxvalue = (maxvalue / (y_lines * 2)) * ((y_lines * 2) - 1);
			y_lines = (y_lines * 2) - 1;
		}
		if(y_lines <= 2) y_lines = 4;
	}
	if(minvalue < 0.0) {
		if(minmaxequal) {
			if(y_lines < 6) {
				y_lines *= 2;				
			} else {
				maxvalue = ceil(maxvalue_digit / 2.0) * pow(10, maxvalue_exp) * 2;
				y_lines += y_lines % 2;
			}
			minvalue = -maxvalue;
		} else {
			int neg_y_lines = -floor(minvalue / (maxvalue / y_lines));
			minvalue = -neg_y_lines * (maxvalue / y_lines);
			y_lines += neg_y_lines;
		}
	}
	if(maxvalue == 0.0 && minvalue == 0.0) {
		maxvalue = 1.0;
		minvalue = -1.0;
		y_lines = 2;
	}
	double tickdiff = (maxvalue - minvalue) / y_lines;
	y_minor = round(tickdiff/pow(10, floor(log10(tickdiff)))) - 1;
	if(y_minor == 0 && (use_deciminor || maxvalue > 10.0)) {
		if(y_lines <= 3) y_minor = 9;
		else if(y_lines <= 6) y_minor = 4;
		else if(y_lines <= 12) y_minor = 1;
	}
}

OverTimeChart::OverTimeChart(Budget *budg, QWidget *parent, bool extra_parameters) : QWidget(parent), budget(budg), b_extra(extra_parameters) {

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

		QHBoxLayout *buttons = new QHBoxLayout();
#ifdef QT_CHARTS_LIB
	buttons->addWidget(new QLabel(tr("Chart type:"), this));
	typeCombo = new QComboBox(this);
	typeCombo->addItem(tr("Line Chart"));
	typeCombo->addItem(tr("Vertical Bar Chart"));
	typeCombo->addItem(tr("Horizontal Bar Chart"));
	typeCombo->addItem(tr("Stacked Bar Chart"));
	buttons->addWidget(typeCombo);
	buttons->addWidget(new QLabel(tr("Theme:"), this));
	themeCombo = new QComboBox(this);
	themeCombo->addItem("Light", QChart::ChartThemeLight);
	themeCombo->addItem("Blue Cerulean", QChart::ChartThemeBlueCerulean);
	themeCombo->addItem("Dark", QChart::ChartThemeDark);
	themeCombo->addItem("Brown Sand", QChart::ChartThemeBrownSand);
	themeCombo->addItem("Blue NCS", QChart::ChartThemeBlueNcs);
	themeCombo->addItem("High Contrast", QChart::ChartThemeHighContrast);
	themeCombo->addItem("Blue Icy", QChart::ChartThemeBlueIcy);
	themeCombo->addItem("Qt", QChart::ChartThemeQt);
	buttons->addWidget(themeCombo);
#endif
	buttons->addStretch();
	saveButton = new QPushButton(tr("Save As…"), this);
	saveButton->setAutoDefault(false);
	buttons->addWidget(saveButton);
	printButton = new QPushButton(tr("Print…"), this);
	printButton->setAutoDefault(false);
	buttons->addWidget(printButton);
	layout->addLayout(buttons);
#ifdef QT_CHARTS_LIB
	chart = new QChart();
	chart->setLocalizeNumbers(true);
	view = new QChartView(chart, this);
	view->setRubberBand(QChartView::RectangleRubberBand);
	axisX = NULL;
	axisY = NULL;
	point_label = NULL;
#else
	scene = NULL;
	view = new QGraphicsView(this);
#endif
	view->setRenderHint(QPainter::Antialiasing, true);
	layout->addWidget(view);

	QSettings settings;
	settings.beginGroup("OverTimeChart");

	QWidget *settingsWidget = new QWidget(this);
	QGridLayout *settingsLayout = new QGridLayout(settingsWidget);

	QLabel *sourceLabel = new QLabel(tr("Source:"), settingsWidget);
	settingsLayout->addWidget(sourceLabel, 2, 0);
	QHBoxLayout *choicesLayout = new QHBoxLayout();
	settingsLayout->addLayout(choicesLayout, 2, 1);
	QGridLayout *choicesLayout_extra = NULL;
	if(b_extra) {
		sourceLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
		choicesLayout_extra = new QGridLayout();
		choicesLayout->addLayout(choicesLayout_extra);
	}
	sourceCombo = new QComboBox(settingsWidget);
	sourceCombo->setEditable(false);
	sourceCombo->addItem(tr("Incomes and Expenses"));
	sourceCombo->addItem(tr("Profits"));
	sourceCombo->addItem(tr("Expenses"));
	sourceCombo->addItem(tr("Incomes"));
	if(b_extra) choicesLayout_extra->addWidget(sourceCombo, 0, 0);
	else choicesLayout->addWidget(sourceCombo);
	categoryCombo = new QComboBox(settingsWidget);
	categoryCombo->setEditable(false);
	categoryCombo->addItem(tr("All Categories Combined"));
	categoryCombo->setEnabled(false);
	if(b_extra) choicesLayout_extra->addWidget(categoryCombo, 0, 1);
	else choicesLayout->addWidget(categoryCombo);
	descriptionCombo = new QComboBox(settingsWidget);
	descriptionCombo->setEditable(false);
	descriptionCombo->addItem(tr("All Descriptions Combined", "Referring to the generic description property"));
	descriptionCombo->setEnabled(false);
	if(b_extra) choicesLayout_extra->addWidget(descriptionCombo, 0, 2);
	else choicesLayout->addWidget(descriptionCombo);
	payeeCombo = NULL;
	if(b_extra) {
		payeeCombo = new QComboBox(settingsWidget);
		payeeCombo->setEditable(false);
		payeeCombo->addItem(tr("All Payees/Payers Combined"));
		payeeCombo->setEnabled(false);
		choicesLayout_extra->addWidget(payeeCombo, 1, 0);
	}

	current_account = NULL;
	current_source = 0;

	settingsLayout->addWidget(new QLabel(tr("Start date:"), settingsWidget), 0, 0);
	QHBoxLayout *monthLayout = new QHBoxLayout();
	settingsLayout->addLayout(monthLayout, 0, 1);
	startDateEdit = new EqonomizeMonthSelector(settingsWidget);
	monthLayout->addWidget(startDateEdit);
	monthLayout->setStretchFactor(startDateEdit, 1);
	monthLayout->addWidget(new QLabel(tr("End date:"), settingsWidget));
	endDateEdit = new EqonomizeMonthSelector(settingsWidget);
	monthLayout->addWidget(endDateEdit);
	monthLayout->setStretchFactor(endDateEdit, 1);
	monthLayout->addStretch(1);

	settingsLayout->addWidget(new QLabel(tr("Value:"), settingsWidget), 1, 0);
	QHBoxLayout *enabledLayout = new QHBoxLayout();
	settingsLayout->addLayout(enabledLayout, 1, 1);
	valueGroup = new QButtonGroup(this);
	valueButton = new QRadioButton(tr("Monthly total"), settingsWidget);
	valueButton->setChecked(settings.value("valueSelected", true).toBool());
	valueGroup->addButton(valueButton, 0);
	enabledLayout->addWidget(valueButton);
	dailyButton = new QRadioButton(tr("Daily average"), settingsWidget);
	dailyButton->setChecked(settings.value("dailyAverageSelected", false).toBool());
	valueGroup->addButton(dailyButton, 1);
	enabledLayout->addWidget(dailyButton);
	countButton = new QRadioButton(tr("Quantity"), settingsWidget);
	countButton->setChecked(settings.value("transactionCountSelected", false).toBool());
	valueGroup->addButton(countButton, 2);
	enabledLayout->addWidget(countButton);
	perButton = new QRadioButton(tr("Average value"), settingsWidget);
	perButton->setChecked(settings.value("valuePerTransactionSelected", false).toBool());
	valueGroup->addButton(perButton, 3);
	enabledLayout->addWidget(perButton);
	enabledLayout->addStretch(1);
	
	settings.endGroup();
	
	resetOptions();

	layout->addWidget(settingsWidget);

#ifdef QT_CHARTS_LIB
	connect(themeCombo, SIGNAL(activated(int)), this, SLOT(themeChanged(int)));
	connect(typeCombo, SIGNAL(activated(int)), this, SLOT(typeChanged(int)));
#endif
	connect(startDateEdit, SIGNAL(monthChanged(const QDate&)), this, SLOT(startMonthChanged(const QDate&)));
	connect(startDateEdit, SIGNAL(yearChanged(const QDate&)), this, SLOT(startYearChanged(const QDate&)));
	connect(endDateEdit, SIGNAL(monthChanged(const QDate&)), this, SLOT(endMonthChanged(const QDate&)));
	connect(endDateEdit, SIGNAL(yearChanged(const QDate&)), this, SLOT(endYearChanged(const QDate&)));
	connect(valueButton, SIGNAL(toggled(bool)), this, SLOT(valueTypeToggled(bool)));
	connect(dailyButton, SIGNAL(toggled(bool)), this, SLOT(valueTypeToggled(bool)));
	connect(countButton, SIGNAL(toggled(bool)), this, SLOT(valueTypeToggled(bool)));
	connect(perButton, SIGNAL(toggled(bool)), this, SLOT(valueTypeToggled(bool)));
	connect(sourceCombo, SIGNAL(activated(int)), this, SLOT(sourceChanged(int)));
	connect(categoryCombo, SIGNAL(activated(int)), this, SLOT(categoryChanged(int)));
	connect(descriptionCombo, SIGNAL(activated(int)), this, SLOT(descriptionChanged(int)));
	if(b_extra) connect(payeeCombo, SIGNAL(activated(int)), this, SLOT(payeeChanged(int)));
	connect(saveButton, SIGNAL(clicked()), this, SLOT(save()));
	connect(printButton, SIGNAL(clicked()), this, SLOT(print()));

}

OverTimeChart::~OverTimeChart() {}

void OverTimeChart::resetOptions() {

#ifdef QT_CHARTS_LIB
	QSettings settings;
	settings.beginGroup("GeneralOptions");
	settings.value("chartTheme", chart->theme()).toInt();
	QChart::ChartTheme theme = (QChart::ChartTheme) settings.value("chartTheme", chart->theme()).toInt();
	themeCombo->setCurrentIndex(theme);
	typeCombo->setCurrentIndex(0);
	chart->setTheme(theme);
	settings.endGroup();
#endif
	sourceCombo->setCurrentIndex(0);
	sourceChanged(0);
	Transaction *trans = budget->transactions.first();
	start_date = QDate();
	while(trans) {
		if(trans->fromAccount()->type() != ACCOUNT_TYPE_ASSETS || trans->toAccount()->type() != ACCOUNT_TYPE_ASSETS) {
			start_date = trans->date();
			if(!budget->isFirstBudgetDay(start_date)) {
				budget->addBudgetMonthsSetFirst(start_date, 1);				
			}
			break;
		}
		trans = budget->transactions.next();
	}
	QDate curmonth = budget->firstBudgetDay(QDate::currentDate());
	if(start_date.isNull() || start_date > curmonth) start_date = curmonth;
	if(start_date == curmonth) {
		budget->addBudgetMonthsSetFirst(start_date, -1);
	}
	
	start_date = budget->budgetDateToMonth(start_date);

	end_date = QDate::currentDate().addDays(-1);
	if(!budget->isLastBudgetDay(end_date)) {
		end_date = budget->lastBudgetDay(end_date);
		budget->addBudgetMonthsSetLast(end_date, -1);
	}
	if(end_date < start_date || budget->isSameBudgetMonth(start_date, end_date)) {
		end_date = budget->lastBudgetDay(QDate::currentDate());
	}
	end_date = budget->firstBudgetDay(end_date);
	
	start_date = budget->budgetDateToMonth(start_date);
	end_date = budget->budgetDateToMonth(end_date);
	
	startDateEdit->setDate(start_date);
	endDateEdit->setDate(end_date);

}

void OverTimeChart::valueTypeToggled(bool b) {
	if(b) updateDisplay();
}

void OverTimeChart::payeeChanged(int index) {
	current_payee = "";
	bool b_income = (current_account && current_account->type() == ACCOUNT_TYPE_INCOMES);
	bool b_subs = (current_account && !current_account->subCategories.isEmpty());
	int d_index = descriptionCombo->currentIndex();
	if(index == 0) {
		if(d_index == 1) current_source = b_income ? 7 : 8;
		else if(b_subs && d_index == 2) current_source = b_income ? 21 : 22;
		else if(d_index == 0) current_source = b_income ? 5 : 6;
		else current_source = b_income ? 9 : 10;
	} else if(index == 1) {
		if(d_index == 1 || (b_subs && d_index == 2)) {
			descriptionCombo->blockSignals(true);
			descriptionCombo->setCurrentIndex(0);
			descriptionCombo->blockSignals(false);
			d_index = 0;
		}
		if(d_index == 0) current_source = b_income ? 11 : 12;
		else current_source = b_income ? 13 : 14;
	} else {
		if(!has_empty_payee || index < payeeCombo->count() - 1) current_payee = payeeCombo->itemText(index);
		if(b_subs && d_index == 1) current_source = b_income ? 23 : 24;
		else if(d_index == 1 || (b_subs && d_index == 2)) current_source = b_income ? 17 : 18;
		else if(d_index == 0) current_source = b_income ? 15 : 16;
		else current_source = b_income ? 19 : 20;
	}
	updateDisplay();
}
void OverTimeChart::descriptionChanged(int index) {
	current_description = "";
	bool b_income = (current_account && current_account->type() == ACCOUNT_TYPE_INCOMES);
	bool b_subs = (current_account && !current_account->subCategories.isEmpty());
	int p_index = 0;
	if(b_extra) p_index = payeeCombo->currentIndex();
	if(index == 0) {
		if(p_index == 1) current_source = b_income ? 11 : 12;
		else if(p_index == 0) current_source = b_income ? 5 : 6;
		else current_source = b_income ? 15 : 16;
	} else if(b_subs && index == 1) {
		if(p_index == 1) {
			payeeCombo->blockSignals(true);
			payeeCombo->setCurrentIndex(0);
			payeeCombo->blockSignals(false);
			p_index = 0;
		}
		if(p_index == 0) current_source = b_income ? 21 : 22;
		else current_source = b_income ? 23 : 24;
	} else if(index == 1 || (b_subs && index == 2)) {
		if(p_index == 1) {
			payeeCombo->blockSignals(true);
			payeeCombo->setCurrentIndex(0);
			payeeCombo->blockSignals(false);
			p_index = 0;
		}
		if(p_index == 0) current_source = b_income ? 7 : 8;
		else current_source = b_income ? 17 : 18;
	} else {
		if(!has_empty_description || index < descriptionCombo->count() - 1) current_description = descriptionCombo->itemText(index);
		if(p_index == 1) current_source = b_income ? 13 : 14;
		else if(p_index == 0) current_source = b_income ? 9 : 10;
		else current_source = b_income ? 19 : 20;
	}
	updateDisplay();
}
void OverTimeChart::categoryChanged(int index) {
	bool b_income = (sourceCombo->currentIndex() == 3);
	descriptionCombo->blockSignals(true);
	int d_index = descriptionCombo->currentIndex();
	descriptionCombo->clear();
	descriptionCombo->addItem(tr("All Descriptions Combined", "Referring to the generic description property"));
	int p_index = 0;
	current_description = "";
	current_payee = "";
	if(b_extra) {
		p_index = payeeCombo->currentIndex();
		payeeCombo->blockSignals(true);
		payeeCombo->clear();
		if(b_income) payeeCombo->addItem(tr("All Payers Combined"));
		else payeeCombo->addItem(tr("All Payees Combined"));
	}
	current_account = NULL;
	if(index == 0) {
		if(b_income) {
			current_source = 1;
		} else {
			current_source = 2;
		}
		descriptionCombo->setEnabled(false);
		if(b_extra) payeeCombo->setEnabled(false);
		payeeCombo->setCurrentIndex(0);
	} else if(index == 1) {
		if(b_income) {
			current_source = (index == 2 ? 25 : 3);
		} else {
			current_source = (index == 2 ? 26 : 4);
		}
		descriptionCombo->setEnabled(false);
		if(b_extra) payeeCombo->setEnabled(false);
		payeeCombo->setCurrentIndex(0);
	} else {
		if(!b_income) {
			int i = categoryCombo->currentIndex() - 2;
			if(i < (int) budget->expensesAccounts.count()) current_account = budget->expensesAccounts.at(i);
		} else {
			int i = categoryCombo->currentIndex() - 2;
			if(i < (int) budget->incomesAccounts.count()) current_account = budget->incomesAccounts.at(i);
		}
		bool b_subs = !current_account->subCategories.isEmpty();
		if(b_subs) {
			descriptionCombo->addItem(tr("All Subcategories Split"));
			descriptionCombo->setItemText(0, tr("All Subcategories and Descriptions Combined", "Referring to the generic description property"));
		}
		descriptionCombo->addItem(tr("All Descriptions Split", "Referring to the generic description property"));
		if(b_extra) {
			if(b_income) payeeCombo->addItem(tr("All Payers Split"));
			else payeeCombo->addItem(tr("All Payees Split"));
		}
		if(!current_account) return categoryChanged(0);
		switch(current_source) {
			case 7: {}
			case 8: {}
			case 17: {}
			case 18: {d_index = b_subs ? 2 : 1; p_index = 0; break;}
			case 11: {}
			case 12: {d_index = 0; p_index = 1; break;}
			case 21: {}
			case 22: {}
			case 23: {}
			case 24: {d_index = b_subs ? 1 : 0; p_index = 0; break;}
			default: {d_index = 0; p_index = 0; break;}
		}
		descriptionCombo->setCurrentIndex(d_index);
		if(b_extra) payeeCombo->setCurrentIndex(p_index);
		if(b_subs && d_index == 1) current_source = b_income ? 21 : 22;
		else if(d_index == 1 || (b_subs && d_index == 2)) current_source = b_income ? 7 : 8;
		else if(p_index == 1) current_source = b_income ? 11 : 12;
		else current_source = b_income ? 5 : 6;
		has_empty_description = false;
		has_empty_payee = false;
		QMap<QString, bool> descriptions, payees;
		Transaction *trans = budget->transactions.first();
		while(trans) {
			if((trans->fromAccount() == current_account || trans->toAccount() == current_account || trans->fromAccount()->topAccount() == current_account || trans->toAccount()->topAccount() == current_account)) {
				if(trans->description().isEmpty()) has_empty_description = true;
				else descriptions[trans->description()] = true;
				if(b_extra) {
					if(trans->type() == TRANSACTION_TYPE_EXPENSE) {
						if(((Expense*) trans)->payee().isEmpty()) has_empty_payee = true;
						else payees[((Expense*) trans)->payee()] = true;
					} else if(trans->type() == TRANSACTION_TYPE_INCOME) {
						if(((Income*) trans)->payer().isEmpty()) has_empty_payee = true;
						else payees[((Income*) trans)->payer()] = true;
					}
				}
			}
			trans = budget->transactions.next();
		}
		QMap<QString, bool>::iterator it_e = descriptions.end();
		for(QMap<QString, bool>::iterator it = descriptions.begin(); it != it_e; ++it) {
			descriptionCombo->addItem(it.key());
		}
		if(has_empty_description) descriptionCombo->addItem(tr("No description", "Referring to the generic description property"));
		descriptionCombo->setEnabled(true);
		if(b_extra) {
			QMap<QString, bool>::iterator it2_e = payees.end();
			for(QMap<QString, bool>::iterator it2 = payees.begin(); it2 != it2_e; ++it2) {
				payeeCombo->addItem(it2.key());
			}
			if(has_empty_payee) {
				if(b_income) payeeCombo->addItem(tr("No payer"));
				else payeeCombo->addItem(tr("No payee"));
			}
			payeeCombo->setEnabled(true);
		}
	}
	descriptionCombo->blockSignals(false);
	if(b_extra) payeeCombo->blockSignals(false);
	updateDisplay();
}
void OverTimeChart::sourceChanged(int index) {
	categoryCombo->blockSignals(true);
	descriptionCombo->blockSignals(true);
	if(b_extra) payeeCombo->blockSignals(true);
	int c_index = 1;
	if(categoryCombo->count() > 1 && categoryCombo->currentIndex() == 0) c_index = 0;
	categoryCombo->clear();
	descriptionCombo->clear();
	descriptionCombo->setEnabled(false);
	descriptionCombo->addItem(tr("All Descriptions Combined", "Referring to the generic description property"));
	if(b_extra) {
		payeeCombo->clear();
		payeeCombo->setEnabled(false);
		if(index == 2) payeeCombo->addItem(tr("All Payers Combined"));
		else if(index == 1) payeeCombo->addItem(tr("All Payees Combined"));
		else payeeCombo->addItem(tr("All Payees/Payers Combined"));
	}
	current_description = "";
	current_payee = "";
	current_account = NULL;
	categoryCombo->addItem(tr("All Categories Combined"));
	if(index == 3) {
		categoryCombo->addItem(tr("All Categories Split"));
		//categoryCombo->addItem(tr("All Categories and Subcategories Split"));
		categoryCombo->setCurrentIndex(c_index);
		Account *account = budget->incomesAccounts.first();
		while(account) {
			categoryCombo->addItem(account->nameWithParent());
			account = budget->incomesAccounts.next();
		}
		categoryCombo->setEnabled(true);
		current_source = 3;
		categoryChanged(c_index);
	} else if(index == 2) {
		categoryCombo->addItem(tr("All Categories Split"));
		//categoryCombo->addItem(tr("All Categories and Subcategories Split"));
		categoryCombo->setCurrentIndex(c_index);
		Account *account = budget->expensesAccounts.first();
		while(account) {
			categoryCombo->addItem(account->nameWithParent());
			account = budget->expensesAccounts.next();
		}
		categoryCombo->setEnabled(true);
		current_source = 4;
		categoryChanged(c_index);
	} else if(index == 1) {
		categoryCombo->setEnabled(false);
		current_source = -1;
		if(countButton->isChecked() || perButton->isChecked()) {
			valueButton->blockSignals(true);
			countButton->blockSignals(true);
			perButton->blockSignals(true);
			valueButton->setChecked(true);
			countButton->setChecked(false);
			perButton->setChecked(false);
			valueButton->blockSignals(false);
			countButton->blockSignals(false);
			perButton->blockSignals(false);
		}
		updateDisplay();
	} else {
		categoryCombo->setEnabled(false);
		current_source = 0;
		updateDisplay();
	}
	countButton->setEnabled(index != 1);
	perButton->setEnabled(index != 1);
	categoryCombo->blockSignals(false);
	descriptionCombo->blockSignals(false);
	if(b_extra) payeeCombo->blockSignals(false);
}

void OverTimeChart::startYearChanged(const QDate &date) {
	bool error = false;
	if(!date.isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		error = true;
	}
	if(!error && date > end_date) {
		end_date = date;
		endDateEdit->blockSignals(true);
		endDateEdit->setDate(end_date);
		endDateEdit->blockSignals(false);
	}
#ifdef QT_CHARTS_LIB
	if(!error && typeCombo->currentIndex() != 0 && budget->calendarMonthsBetweenDates(date, end_date, false) > 11) {
		end_date = date.addMonths(11);
		endDateEdit->blockSignals(true);
		endDateEdit->setDate(end_date);
		endDateEdit->blockSignals(false);
	}
#endif
	if(error) {
		startDateEdit->focusYear();
		startDateEdit->blockSignals(true);
		startDateEdit->setDate(start_date);
		startDateEdit->blockSignals(false);
		return;
	}
	start_date = date;
	updateDisplay();
}
void OverTimeChart::startMonthChanged(const QDate &date) {	
	bool error = false;
	if(!date.isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		error = true;
	}
	if(!error && date > end_date) {
		end_date = date;
		endDateEdit->blockSignals(true);
		endDateEdit->setDate(end_date);
		endDateEdit->blockSignals(false);
	}
#ifdef QT_CHARTS_LIB
	if(!error && typeCombo->currentIndex() != 0 && budget->calendarMonthsBetweenDates(date, end_date, false) > 11) {
		end_date = date.addMonths(11);
		endDateEdit->blockSignals(true);
		endDateEdit->setDate(end_date);
		endDateEdit->blockSignals(false);
	}
#endif
	if(error) {
		startDateEdit->focusMonth();
		startDateEdit->blockSignals(true);
		startDateEdit->setDate(start_date);
		startDateEdit->blockSignals(false);
		return;
	}
	start_date = date;
	updateDisplay();
}
void OverTimeChart::endYearChanged(const QDate &date) {	
	bool error = false;
	if(!date.isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		error = true;
	}
	if(!error && date < start_date) {
		start_date = date;
		startDateEdit->blockSignals(true);
		startDateEdit->setDate(start_date);
		startDateEdit->blockSignals(false);
	}
#ifdef QT_CHARTS_LIB
	if(!error && typeCombo->currentIndex() != 0 && budget->calendarMonthsBetweenDates(start_date, date, false) > 11) {
		start_date = date.addMonths(-11);
		startDateEdit->blockSignals(true);
		startDateEdit->setDate(start_date);
		startDateEdit->blockSignals(false);
	}
#endif
	if(error) {
		endDateEdit->focusYear();
		endDateEdit->blockSignals(true);
		endDateEdit->setDate(end_date);
		endDateEdit->blockSignals(false);
		return;
	}
	end_date = date;
	updateDisplay();
}
void OverTimeChart::endMonthChanged(const QDate &date) {	
	bool error = false;
	if(!date.isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		error = true;
	}
	if(!error && date < start_date) {
		start_date = date;
		startDateEdit->blockSignals(true);
		startDateEdit->setDate(start_date);
		startDateEdit->blockSignals(false);
	}
#ifdef QT_CHARTS_LIB
	if(!error && typeCombo->currentIndex() != 0 && budget->calendarMonthsBetweenDates(start_date, date, false) > 11) {
		start_date = date.addMonths(-11);
		startDateEdit->blockSignals(true);
		startDateEdit->setDate(start_date);
		startDateEdit->blockSignals(false);
	}
#endif
	if(error) {
		endDateEdit->focusMonth();
		endDateEdit->blockSignals(true);
		endDateEdit->setDate(end_date);
		endDateEdit->blockSignals(false);
		return;
	}
	end_date = date;
	updateDisplay();
}
void OverTimeChart::saveConfig() {
	QSettings settings;
	settings.beginGroup("OverTimeChart");
	settings.setValue("size", ((QDialog*) parent())->size());
	settings.setValue("valueEnabled", valueButton->isChecked());
	settings.setValue("dailyAverageEnabled", dailyButton->isChecked());
	settings.setValue("transactionCountEnabled", countButton->isChecked());
	settings.setValue("valuePerTransactionEnabled", perButton->isChecked());
	settings.endGroup();
}
void OverTimeChart::onFilterSelected(QString filter) {
	QMimeDatabase db;
	QFileDialog *fileDialog = qobject_cast<QFileDialog*>(sender());
	if(filter == db.mimeTypeForName("image/gif").filterString()) {
		fileDialog->setDefaultSuffix(db.mimeTypeForName("image/gif").preferredSuffix());
	} else if(filter == db.mimeTypeForName("image/jpeg").filterString()) {
		fileDialog->setDefaultSuffix(db.mimeTypeForName("image/jpeg").preferredSuffix());
	} else if(filter == db.mimeTypeForName("image/tiff").filterString()) {
		fileDialog->setDefaultSuffix(db.mimeTypeForName("image/tiff").preferredSuffix());
	} else if(filter == db.mimeTypeForName("image/x-bmp").filterString()) {
		fileDialog->setDefaultSuffix(db.mimeTypeForName("image/x-bmp").preferredSuffix());
	} else if(filter == db.mimeTypeForName("image/x-eps").filterString()) {
		fileDialog->setDefaultSuffix(db.mimeTypeForName("image/x-eps").preferredSuffix());
	} else {
		fileDialog->setDefaultSuffix(db.mimeTypeForName("image/png").preferredSuffix());
	}
}
void OverTimeChart::save() {
#ifdef QT_CHARTS_LIB
	QGraphicsScene *scene = chart->scene();
#endif	
	if(!scene) return;
	QMimeDatabase db;
	QMimeType image_mime = db.mimeTypeForName("image/*");
	QMimeType png_mime = db.mimeTypeForName("image/png");
	QMimeType gif_mime = db.mimeTypeForName("image/gif");
	QMimeType jpeg_mime = db.mimeTypeForName("image/jpeg");
	QMimeType tiff_mime = db.mimeTypeForName("image/tiff");
	QMimeType bmp_mime = db.mimeTypeForName("image/x-bmp");
	QMimeType eps_mime = db.mimeTypeForName("image/x-eps");
	QString png_filter = png_mime.filterString();
	QString gif_filter = gif_mime.filterString();
	QString jpeg_filter = jpeg_mime.filterString();
	QString tiff_filter = tiff_mime.filterString();
	QString bmp_filter = bmp_mime.filterString();
	QString eps_filter = eps_mime.filterString();
	QStringList filter(png_filter);
	QList<QByteArray> image_formats = QImageWriter::supportedImageFormats();
	if(image_formats.contains("gif")) {
		filter << gif_filter;
	}
	if(image_formats.contains("jpeg")) {
		filter << jpeg_filter;
	}
	if(image_formats.contains("tiff")) {
		filter << tiff_filter;
	}
	if(image_formats.contains("bmp")) {
		filter << bmp_filter;
	}
	if(image_formats.contains("eps")) {
		filter << eps_filter;
	}
	QFileDialog fileDialog(this);
	fileDialog.setNameFilters(filter);
	fileDialog.selectNameFilter(png_filter);
	fileDialog.setDefaultSuffix(png_mime.preferredSuffix());
	fileDialog.setAcceptMode(QFileDialog::AcceptSave);
	fileDialog.setSupportedSchemes(QStringList("file"));
	fileDialog.setDirectory(last_picture_directory);
	connect(&fileDialog, SIGNAL(filterSelected(QString)), this, SLOT(onFilterSelected(QString)));
	QString url;
	if(!fileDialog.exec()) return;
	QStringList urls = fileDialog.selectedFiles();
	if(urls.isEmpty()) return;
	url = urls[0];
	QString selected_filter = fileDialog.selectedNameFilter();
	QMimeType url_mime = db.mimeTypeForFile(url, QMimeDatabase::MatchExtension);
	QSaveFile ofile(url);
	ofile.open(QIODevice::WriteOnly);
	ofile.setPermissions((QFile::Permissions) 0x0660);
	if(!ofile.isOpen()) {
		ofile.cancelWriting();
		QMessageBox::critical(this, tr("Error"), tr("Couldn't open file for writing."));
		return;
	}
	last_picture_directory = fileDialog.directory().absolutePath();

	QRectF rect = scene->sceneRect();
	QPixmap pixmap((int) ceil(rect.width()), (int) ceil(rect.height()));
	QPainter p(&pixmap);
#ifdef QT_CHARTS_LIB
	bool drop_shadow_save = chart->isDropShadowEnabled();
	chart->setDropShadowEnabled(false);
	p.fillRect(pixmap.rect(), chart->backgroundBrush());
#endif
	p.setRenderHint(QPainter::Antialiasing, true);
	scene->render(&p);
#ifdef QT_CHARTS_LIB
	chart->setDropShadowEnabled(drop_shadow_save);
#endif	

	if(url_mime == png_mime) {pixmap.save(&ofile, "PNG");}
	else if(url_mime == bmp_mime) {pixmap.save(&ofile, "BMP");}
	else if(url_mime == eps_mime) {pixmap.save(&ofile, "EPS");}
	else if(url_mime == tiff_mime) {pixmap.save(&ofile, "TIF");}
	else if(url_mime == gif_mime) {pixmap.save(&ofile, "GIF");}
	else if(url_mime == jpeg_mime) {pixmap.save(&ofile, "JPEG");}
	else if(selected_filter == png_filter) {pixmap.save(&ofile, "PNG");}
	else if(selected_filter == bmp_filter) {pixmap.save(&ofile, "BMP");}
	else if(selected_filter == eps_filter) {pixmap.save(&ofile, "EPS");}
	else if(selected_filter == tiff_filter) {pixmap.save(&ofile, "TIF");}
	else if(selected_filter == gif_filter) {pixmap.save(&ofile, "GIF");}
	else if(selected_filter == jpeg_filter) {pixmap.save(&ofile, "JPEG");}
	else pixmap.save(&ofile);
	if(!ofile.commit()) {
		QMessageBox::critical(this, tr("Error"), tr("Error while writing file; file was not saved."));
		return;
	}
}

void OverTimeChart::print() {
#ifdef QT_CHARTS_LIB
	QGraphicsScene *scene = view->scene();
#endif
	if(!scene) return;
	QPrinter pr;
	QPrintDialog dialog(&pr, this);
	if(dialog.exec() == QDialog::Accepted) {
		QPainter p(&pr);
		p.setRenderHint(QPainter::Antialiasing, true);
		scene->render(&p);
		p.end();
	}
}
QColor getColor2(int index) {
	switch(index % 11) {
		case 0: {return Qt::red;}
		case 1: {return Qt::blue;}
		case 2: {return Qt::cyan;}
		case 3: {return Qt::magenta;}
		case 4: {return Qt::green;}
		case 5: {return Qt::darkRed;}
		case 6: {return Qt::darkGreen;}
		case 7: {return Qt::darkBlue;}
		case 8: {return Qt::darkCyan;}
		case 9: {return Qt::darkMagenta;}
		case 10: {return Qt::darkYellow;}
		default: {return Qt::gray;}
	}
}
QPen getPen(int index) {
	QPen pen;
	pen.setWidth(2);
	switch(index / 11) {
		case 0: {pen.setStyle(Qt::SolidLine); break;}
		case 1: {pen.setStyle(Qt::DashLine); break;}
		case 3: {pen.setStyle(Qt::DashDotLine); break;}
		case 4: {pen.setStyle(Qt::DashDotDotLine); break;}
		default: {}
	}
	pen.setColor(getColor2(index));
	return pen;
}
void OverTimeChart::updateDisplay() {

	if(!isVisible()) return;

	QVector<chart_month_info> monthly_incomes, monthly_expenses;
	QMap<Account*, QVector<chart_month_info> > monthly_cats;
	QMap<Account*, chart_month_info*> mi_c;
	QMap<QString, QVector<chart_month_info> > monthly_desc;
	QMap<QString, double> desc_values;
	QMap<QString, chart_month_info*> mi_d;
	chart_month_info *mi_e = NULL, *mi_i = NULL;
	chart_month_info **mi = NULL;
	QVector<chart_month_info> *monthly_values = NULL;
	double *total_value = NULL;
	QMap<Account*, double> scheduled_cats;
	QMap<Account*, double> scheduled_cat_counts;
	QMap<QString, double> scheduled_desc;
	QMap<QString, double> scheduled_desc_counts;
	bool isfirst_i = true, isfirst_e = true;
	QMap<Account*, bool> isfirst_c;
	QMap<QString, bool> isfirst_d;
	bool *isfirst = NULL;
	QStringList desc_order;
	bool exclude_subs = (current_source == 3 || current_source == 4);
	bool is_parent = (current_account && !current_account->subCategories.isEmpty());
	if(current_source == 25) current_source = 3;
	if(current_source == 26) current_source = 4;

	int type = valueGroup->checkedId();

	QString title_string;

	if(current_source == 3 || current_source < 2) {
		Account *account = budget->incomesAccounts.first();
		while(account) {
			if(!exclude_subs || account->topAccount() == account) {
				monthly_cats[account] = QVector<chart_month_info>();
				mi_c[account] = NULL;
				isfirst_c[account] = true;
				scheduled_cats[account] = 0.0;
				scheduled_cat_counts[account] = 0.0;
			}
			account = budget->incomesAccounts.next();
		}
	}

	if(current_source == 4 || current_source < 3) {
		Account *account = budget->expensesAccounts.first();
		while(account) {
			if(!exclude_subs || account->topAccount() == account) {
				monthly_cats[account] = QVector<chart_month_info>();
				mi_c[account] = NULL;
				isfirst_c[account] = true;
				scheduled_cats[account] = 0.0;
				scheduled_cat_counts[account] = 0.0;
			}
			account = budget->expensesAccounts.next();
		}
	} else if(current_source >= 21 && current_source <= 24) {
		if(!current_account) return;
		CategoryAccount *account = current_account->subCategories.first();
		while(account) {
			monthly_cats[account] = QVector<chart_month_info>();
			mi_c[account] = NULL;
			isfirst_c[account] = true;
			scheduled_cats[account] = 0.0;
			scheduled_cat_counts[account] = 0.0;
			if(account == current_account) break;
			account = current_account->subCategories.next();
			if(!account) account = current_account;
		}
	} else if(current_source == 7 || current_source == 8 || current_source == 17 || current_source == 18) {
		if(has_empty_description) descriptionCombo->setItemText(descriptionCombo->count() - 1, "");
		for(int i = 2; i < descriptionCombo->count(); i++) {
			QString str = descriptionCombo->itemText(i);
			monthly_desc[str] = QVector<chart_month_info>();
			desc_values[str] = 0.0;
			mi_d[str] = NULL;
			isfirst_d[str] = true;
			scheduled_desc[str] = 0.0;
			scheduled_desc_counts[str] = 0.0;
		}
	} else if(current_source == 11 || current_source == 12 || current_source == 13 || current_source == 14) {
		if(has_empty_payee) payeeCombo->setItemText(payeeCombo->count() - 1, "");
		for(int i = 2; i < payeeCombo->count(); i++) {
			QString str = payeeCombo->itemText(i);
			monthly_desc[str] = QVector<chart_month_info>();
			desc_values[str] = 0.0;
			mi_d[str] = NULL;
			isfirst_d[str] = true;
			scheduled_desc[str] = 0.0;
			scheduled_desc_counts[str] = 0.0;
		}
	}

	QDate first_date = budget->monthToBudgetMonth(start_date);
	QDate last_date = budget->lastBudgetDay(budget->monthToBudgetMonth(end_date));
	
	double maxvalue = 1.0;
	double minvalue = 0.0;
	double maxcount = 1.0;
	bool started = false;
	Transaction *trans = budget->transactions.first();
	while(trans && trans->date() <= last_date) {
		bool include = false;
		int sign = 1;
		total_value = NULL;
		if(!started && trans->date() >= first_date) {
			started = true;
			first_date = budget->firstBudgetDay(trans->date());
		}
		if(started) {
			switch(current_source) {
				case -1: {}
				case 0: {}
				case 1: {}
				case 2: {}
				case 3: {}
				case 4: {
					if(current_source != 2 && current_source != 4 && trans->fromAccount()->type() == ACCOUNT_TYPE_INCOMES) {
						monthly_values = &monthly_cats[exclude_subs ? trans->fromAccount()->topAccount() : trans->fromAccount()]; mi = &mi_c[exclude_subs ? trans->fromAccount()->topAccount() : trans->fromAccount()]; isfirst = &isfirst_c[exclude_subs ? trans->fromAccount()->topAccount() : trans->fromAccount()];
						sign = 1;
						include = true;
					} else if(current_source != 1 && current_source != 3 && trans->fromAccount()->type() == ACCOUNT_TYPE_EXPENSES) {
						monthly_values = &monthly_cats[exclude_subs ? trans->fromAccount()->topAccount() : trans->fromAccount()]; mi = &mi_c[exclude_subs ? trans->fromAccount()->topAccount() : trans->fromAccount()]; isfirst = &isfirst_c[exclude_subs ? trans->fromAccount()->topAccount() : trans->fromAccount()];
						sign = -1;
						include = true;
					} else if(current_source != 2 && current_source != 4 && trans->toAccount()->type() == ACCOUNT_TYPE_INCOMES) {
						monthly_values = &monthly_cats[exclude_subs ? trans->toAccount()->topAccount() : trans->toAccount()]; mi = &mi_c[exclude_subs ? trans->toAccount()->topAccount() : trans->toAccount()]; isfirst = &isfirst_c[exclude_subs ? trans->toAccount()->topAccount() : trans->toAccount()];
						sign = -1;
						include = true;
					} else if(current_source != 1 && current_source != 3 && trans->toAccount()->type() == ACCOUNT_TYPE_EXPENSES) {
						monthly_values = &monthly_cats[exclude_subs ? trans->toAccount()->topAccount() : trans->toAccount()]; mi = &mi_c[exclude_subs ? trans->toAccount()->topAccount() : trans->toAccount()]; isfirst = &isfirst_c[exclude_subs ? trans->toAccount()->topAccount() : trans->toAccount()];
						sign = 1;
						include = true;
					}
					break;
				}
				case 23: {
					if(trans->type() != TRANSACTION_TYPE_INCOME) break;
					if(((Income*) trans)->payer() != current_payee) break;
				}
				case 21: {
					if(trans->fromAccount()->topAccount() == current_account) {
						monthly_values = &monthly_cats[trans->fromAccount()]; mi = &mi_c[trans->fromAccount()]; isfirst = &isfirst_c[trans->fromAccount()];
						sign = 1;
						include = true;
					} else if(trans->toAccount()->topAccount() == current_account) {
						monthly_values = &monthly_cats[trans->toAccount()]; mi = &mi_c[trans->toAccount()]; isfirst = &isfirst_c[trans->toAccount()];
						sign = -1;
						include = true;
					}
					break;
				}
				case 24: {
					if(trans->type() != TRANSACTION_TYPE_EXPENSE) break;
					if(((Expense*) trans)->payee() != current_payee) break;
				}
				case 22: {
					if(trans->fromAccount()->topAccount() == current_account) {
						monthly_values = &monthly_cats[trans->fromAccount()]; mi = &mi_c[trans->fromAccount()]; isfirst = &isfirst_c[trans->fromAccount()];
						sign = -1;
						include = true;
					} else if(trans->toAccount()->topAccount() == current_account) {
						monthly_values = &monthly_cats[trans->toAccount()]; mi = &mi_c[trans->toAccount()]; isfirst = &isfirst_c[trans->toAccount()];
						sign = 1;
						include = true;
					}
					break;
				}
				case 5: {
					if((is_parent ? trans->fromAccount()->topAccount() : trans->fromAccount()) == current_account) {
						monthly_values = &monthly_incomes; mi = &mi_i; isfirst = &isfirst_i;
						sign = 1;
						include = true;
					} else if((is_parent ? trans->toAccount()->topAccount() : trans->toAccount()) == current_account) {
						monthly_values = &monthly_incomes; mi = &mi_i; isfirst = &isfirst_i;
						sign = -1;
						include = true;
					}
					break;
				}
				case 6: {
					if((is_parent ? trans->fromAccount()->topAccount() : trans->fromAccount()) == current_account) {
						monthly_values = &monthly_expenses; mi = &mi_e; isfirst = &isfirst_e;
						sign = -1;
						include = true;
					} else if((is_parent ? trans->toAccount()->topAccount() : trans->toAccount()) == current_account) {
						monthly_values = &monthly_expenses; mi = &mi_e; isfirst = &isfirst_e;
						sign = 1;
						include = true;
					}
					break;
				}
				case 7: {
					if((is_parent ? trans->fromAccount()->topAccount() : trans->fromAccount()) == current_account) {
						monthly_values = &monthly_desc[trans->description()]; mi = &mi_d[trans->description()]; isfirst = &isfirst_d[trans->description()];
						total_value = &desc_values[trans->description()];
						sign = 1;
						include = true;
					} else if((is_parent ? trans->toAccount()->topAccount() : trans->toAccount()) == current_account) {
						monthly_values = &monthly_desc[trans->description()]; mi = &mi_d[trans->description()]; isfirst = &isfirst_d[trans->description()];
						total_value = &desc_values[trans->description()];
						sign = -1;
						include = true;
					}
					break;
				}
				case 8: {
					if((is_parent ? trans->fromAccount()->topAccount() : trans->fromAccount()) == current_account) {
						monthly_values = &monthly_desc[trans->description()]; mi = &mi_d[trans->description()]; isfirst = &isfirst_d[trans->description()];
						total_value = &desc_values[trans->description()];
						sign = -1;
						include = true;
					} else if((is_parent ? trans->toAccount()->topAccount() : trans->toAccount()) == current_account) {
						monthly_values = &monthly_desc[trans->description()]; mi = &mi_d[trans->description()]; isfirst = &isfirst_d[trans->description()];
						total_value = &desc_values[trans->description()];
						sign = 1;
						include = true;
					}
					break;
				}
				case 9: {
					if((is_parent ? trans->fromAccount()->topAccount() : trans->fromAccount()) == current_account && trans->description() == current_description) {
						monthly_values = &monthly_incomes; mi = &mi_i; isfirst = &isfirst_i;
						sign = 1;
						include = true;
					} else if((is_parent ? trans->toAccount()->topAccount() : trans->toAccount()) == current_account && trans->description() == current_description) {
						monthly_values = &monthly_incomes; mi = &mi_i; isfirst = &isfirst_i;
						sign = -1;
						include = true;
					}
					break;
				}
				case 10: {
					if((is_parent ? trans->fromAccount()->topAccount() : trans->fromAccount()) == current_account && trans->description() == current_description) {
						monthly_values = &monthly_expenses; mi = &mi_e; isfirst = &isfirst_e;
						sign = -1;
						include = true;
					} else if((is_parent ? trans->toAccount()->topAccount() : trans->toAccount()) == current_account && trans->description() == current_description) {
						monthly_values = &monthly_expenses; mi = &mi_e; isfirst = &isfirst_e;
						sign = 1;
						include = true;
					}
					break;
				}
				case 11: {
					if(trans->type() != TRANSACTION_TYPE_INCOME) break;
					Income *income = (Income*) trans;
					if((is_parent ? income->category()->topAccount() : income->category()) == current_account) {
						monthly_values = &monthly_desc[income->payer()]; mi = &mi_d[income->payer()]; isfirst = &isfirst_d[income->payer()];
						total_value = &desc_values[income->payer()];
						sign = 1;
						include = true;
					}
					break;
				}
				case 12: {
					if(trans->type() != TRANSACTION_TYPE_EXPENSE) break;
					Expense *expense = (Expense*) trans;
					if((is_parent ? expense->category()->topAccount() : expense->category()) == current_account) {
						monthly_values = &monthly_desc[expense->payee()]; mi = &mi_d[expense->payee()]; isfirst = &isfirst_d[expense->payee()];
						total_value = &desc_values[expense->payee()];
						sign = 1;
						include = true;
					}
					break;
				}
				case 13: {
					if(trans->type() != TRANSACTION_TYPE_INCOME) break;
					Income *income = (Income*) trans;
					if((is_parent ? income->category()->topAccount() : income->category()) == current_account && income->description() == current_description) {
						monthly_values = &monthly_desc[income->payer()]; mi = &mi_d[income->payer()]; isfirst = &isfirst_d[income->payer()];
						total_value = &desc_values[income->payer()];
						sign = 1;
						include = true;
					}
					break;
				}
				case 14: {
					if(trans->type() != TRANSACTION_TYPE_EXPENSE) break;
					Expense *expense = (Expense*) trans;
					if((is_parent ? expense->category()->topAccount() : expense->category()) == current_account && expense->description() == current_description) {
						monthly_values = &monthly_desc[expense->payee()]; mi = &mi_d[expense->payee()]; isfirst = &isfirst_d[expense->payee()];
						total_value = &desc_values[expense->payee()];
						sign = 1;
						include = true;
					}
					break;
				}
				case 15: {
					if(trans->type() != TRANSACTION_TYPE_INCOME) break;
					Income *income = (Income*) trans;
					if((is_parent ? income->category()->topAccount() : income->category()) == current_account && income->payer() == current_payee) {
						monthly_values = &monthly_incomes; mi = &mi_i; isfirst = &isfirst_i;
						sign = 1;
						include = true;
					}
					break;
				}
				case 16: {
					if(trans->type() != TRANSACTION_TYPE_EXPENSE) break;
					Expense *expense = (Expense*) trans;
					if((is_parent ? expense->category()->topAccount() : expense->category()) == current_account && expense->payee() == current_payee) {
						monthly_values = &monthly_expenses; mi = &mi_e; isfirst = &isfirst_e;
						sign = 1;
						include = true;
					}
					break;
				}
				case 17: {
					if(trans->type() != TRANSACTION_TYPE_INCOME) break;
					Income *income = (Income*) trans;
					if((is_parent ? income->category()->topAccount() : income->category()) == current_account && income->payer() == current_payee) {
						monthly_values = &monthly_desc[income->description()]; mi = &mi_d[income->description()]; isfirst = &isfirst_d[income->description()];
						total_value = &desc_values[income->description()];
						sign = 1;
						include = true;
					}
					break;
				}
				case 18: {
					if(trans->type() != TRANSACTION_TYPE_EXPENSE) break;
					Expense *expense = (Expense*) trans;
					if((is_parent ? expense->category()->topAccount() : expense->category()) == current_account && expense->payee() == current_payee) {
						monthly_values = &monthly_desc[expense->description()]; mi = &mi_d[expense->description()]; isfirst = &isfirst_d[expense->description()];
						total_value = &desc_values[expense->description()];
						sign = 1;
						include = true;
					}
					break;
				}
				case 19: {
					if(trans->type() != TRANSACTION_TYPE_INCOME) break;
					Income *income = (Income*) trans;
					if((is_parent ? income->category()->topAccount() : income->category()) == current_account && income->description() == current_description && income->payer() == current_payee) {
						monthly_values = &monthly_incomes; mi = &mi_i; isfirst = &isfirst_i;
						sign = 1;
						include = true;
					}
					break;
				}
				case 20: {
					if(trans->type() != TRANSACTION_TYPE_EXPENSE) break;
					Expense *expense = (Expense*) trans;
					if((is_parent ? expense->category()->topAccount() : expense->category()) == current_account && expense->description() == current_description && expense->payee() == current_payee) {
						monthly_values = &monthly_expenses; mi = &mi_e; isfirst = &isfirst_e;
						sign = 1;
						include = true;
					}
					break;
				}
			}
		}
		if(include) {
			if(!(*mi) || trans->date() > (*mi)->date) {
				QDate newdate, olddate;
				newdate = budget->lastBudgetDay(trans->date());
				if(*mi) {
					olddate = (*mi)->date;
					budget->addBudgetMonthsSetLast(olddate, 1);
					(*isfirst) = false;
				} else {
					olddate = budget->lastBudgetDay(first_date);
					(*isfirst) = true;
				}
				while(olddate < newdate) {
					monthly_values->append(chart_month_info());
					(*mi) = &monthly_values->back();
					(*mi)->value = 0.0;
					(*mi)->count = 0.0;
					(*mi)->date = olddate;
					budget->addBudgetMonthsSetLast(olddate, 1);
					(*isfirst) = false;
				}
				monthly_values->append(chart_month_info());
				(*mi) = &monthly_values->back();
				(*mi)->value = trans->value() * sign;
				(*mi)->count = trans->quantity();
				(*mi)->date = newdate;
			} else {
				(*mi)->value += trans->value() * sign;
				(*mi)->count += trans->quantity();
			}
			if(total_value) {
				if(type == 2) *total_value += trans->quantity();
				else *total_value += trans->value() * sign;
			}
		}
		trans = budget->transactions.next();
	}

	int source_org = 0;
	switch(current_source) {
		case -1: {}
		case 0: {source_org = 0; break;}
		case 1: {source_org = 3; break;}
		case 2: {source_org = 4; break;}
		case 3: {source_org = 3; break;}
		case 4: {source_org = 4; break;}
		case 5: {source_org = 1; break;}
		case 6: {source_org = 2; break;}
		case 7: {}
		case 8: {source_org = 7; break;}
		case 9: {source_org = 1; break;}
		case 10: {source_org = 2; break;}
		case 11: {}
		case 12: {}
		case 13: {}
		case 14: {source_org = 11; break;}
		case 15: {source_org = 1; break;}
		case 16: {source_org = 2; break;}
		case 17: {}
		case 18: {source_org = 7; break;}
		case 19: {source_org = 1; break;}
		case 20: {source_org = 2; break;}
		case 21: {}
		case 22: {}
		case 23: {}
		case 24: {source_org = 21; break;}
		
	}
	
	int first_desc_i = 2;
	if(source_org == 7 && is_parent) first_desc_i = 3;

	Account *account = NULL;
	int desc_i = first_desc_i;
	int desc_nr = 0;
	bool at_expenses = false;
	if(source_org == 7) desc_nr = descriptionCombo->count();
	else if(source_org == 11) desc_nr = payeeCombo->count();
	else if(source_org == 0) {account = budget->incomesAccounts.first(); if(!account) {at_expenses = true; account = budget->expensesAccounts.first();}}
	else if(source_org == 3) account = budget->incomesAccounts.first();
	else if(source_org == 4) account = budget->expensesAccounts.first();
	else if(source_org == 21) account = current_account->subCategories.first();
	else if(source_org == 2) {mi = &mi_e; monthly_values = &monthly_expenses; isfirst = &isfirst_e;}
	else if(source_org == 1) {mi = &mi_i; monthly_values = &monthly_incomes; isfirst = &isfirst_i;}		
	while(source_org == 1 || source_org == 2 || account || ((source_org == 7 || source_org == 11) && desc_i < desc_nr)) {
		while(exclude_subs && account && account->topAccount() != account) {
			if(source_org == 3) account = budget->incomesAccounts.next();
			else account = budget->expensesAccounts.next();
		}
		if(exclude_subs && !account) break;
		if(source_org == 3 || source_org == 4 || source_org == 0 || source_org == 21) {mi = &mi_c[account]; monthly_values = &monthly_cats[account]; isfirst = &isfirst_c[account];}
		else if(source_org == 7) {mi = &mi_d[descriptionCombo->itemText(desc_i)]; monthly_values = &monthly_desc[descriptionCombo->itemText(desc_i)]; isfirst = &isfirst_d[descriptionCombo->itemText(desc_i)];}
		else if(source_org == 11) {mi = &mi_d[payeeCombo->itemText(desc_i)]; monthly_values = &monthly_desc[payeeCombo->itemText(desc_i)]; isfirst = &isfirst_d[payeeCombo->itemText(desc_i)];}
		if(!(*mi)) {
			monthly_values->append(chart_month_info());
			(*mi) = &monthly_values->back();
			(*mi)->value = 0.0;
			(*mi)->count = 0.0;
			(*mi)->date = budget->lastBudgetDay(first_date);
			(*isfirst) = true;
		}
		while((*mi)->date < last_date) {
			QDate newdate = (*mi)->date;
			budget->addBudgetMonthsSetLast(newdate, 1);
			monthly_values->append(chart_month_info());
			(*mi) = &monthly_values->back();
			(*mi)->value = 0.0;
			(*mi)->count = 0.0;
			(*mi)->date = newdate;
			(*isfirst) = false;
		}
		if(source_org == 7 || source_org == 11) desc_i++;
		else if(source_org == 3) account = budget->incomesAccounts.next();
		else if(source_org == 4) account = budget->expensesAccounts.next();
		else if(source_org == 21) {
			if(account == current_account) break;
			account = current_account->subCategories.next();
			if(!account) account = current_account;
		} else if(source_org == 0) {
			if(!at_expenses) {
				account = budget->incomesAccounts.next();
				if(!account) {
					at_expenses = true;
					account = budget->expensesAccounts.first();
				}
			} else {
				account = budget->expensesAccounts.next();
			}
		} else break;
	}

	ScheduledTransaction *strans = budget->scheduledTransactions.first();

	while(strans && strans->firstOccurrence() <= last_date) {
		trans = strans->transaction();
		bool include = false;
		int sign = 1;
		total_value = NULL;
		switch(current_source) {
			case -1: {}
			case 0: {}
			case 1: {}
			case 2: {}
			case 3: {}
			case 4: {
				if(current_source != 2 && current_source != 4 && trans->fromAccount()->type() == ACCOUNT_TYPE_INCOMES) {
					monthly_values = &monthly_cats[exclude_subs ? trans->fromAccount()->topAccount() : trans->fromAccount()]; mi = &mi_c[exclude_subs ? trans->fromAccount()->topAccount() : trans->fromAccount()]; isfirst = &isfirst_c[exclude_subs ? trans->fromAccount()->topAccount() : trans->fromAccount()];
					sign = 1;
					include = true;
				} else if(current_source != 1 && current_source != 3 && trans->fromAccount()->type() == ACCOUNT_TYPE_EXPENSES) {
					monthly_values = &monthly_cats[exclude_subs ? trans->fromAccount()->topAccount() : trans->fromAccount()]; mi = &mi_c[exclude_subs ? trans->fromAccount()->topAccount() : trans->fromAccount()]; isfirst = &isfirst_c[exclude_subs ? trans->fromAccount()->topAccount() : trans->fromAccount()];
					sign = -1;
					include = true;
				} else if(current_source != 2 && current_source != 4 && trans->toAccount()->type() == ACCOUNT_TYPE_INCOMES) {
					monthly_values = &monthly_cats[exclude_subs ? trans->toAccount()->topAccount() : trans->toAccount()]; mi = &mi_c[exclude_subs ? trans->toAccount()->topAccount() : trans->toAccount()]; isfirst = &isfirst_c[exclude_subs ? trans->toAccount()->topAccount() : trans->toAccount()];
					sign = -1;
					include = true;
				} else if(current_source != 1 && current_source != 3 && trans->toAccount()->type() == ACCOUNT_TYPE_EXPENSES) {
					monthly_values = &monthly_cats[exclude_subs ? trans->toAccount()->topAccount() : trans->toAccount()]; mi = &mi_c[exclude_subs ? trans->toAccount()->topAccount() : trans->toAccount()]; isfirst = &isfirst_c[exclude_subs ? trans->toAccount()->topAccount() : trans->toAccount()];
					sign = 1;
					include = true;
				}
				break;
			}
			case 23: {
				if(trans->type() != TRANSACTION_TYPE_INCOME) break;
				if(((Income*) trans)->payer() != current_payee) break;
			}
			case 21: {
				if(trans->fromAccount()->topAccount() == current_account) {
					monthly_values = &monthly_cats[trans->fromAccount()]; mi = &mi_c[trans->fromAccount()]; isfirst = &isfirst_c[trans->fromAccount()];
					sign = 1;
					include = true;
				} else if(trans->toAccount()->topAccount() == current_account) {
					monthly_values = &monthly_cats[trans->toAccount()]; mi = &mi_c[trans->toAccount()]; isfirst = &isfirst_c[trans->toAccount()];
					sign = -1;
					include = true;
				}
				break;
			}
			case 24: {
				if(trans->type() != TRANSACTION_TYPE_EXPENSE) break;
				if(((Expense*) trans)->payee() != current_payee) break;
			}
			case 22: {
				if(trans->fromAccount()->topAccount() == current_account) {
					monthly_values = &monthly_cats[trans->fromAccount()]; mi = &mi_c[trans->fromAccount()]; isfirst = &isfirst_c[trans->fromAccount()];
					sign = -1;
					include = true;
				} else if(trans->toAccount()->topAccount() == current_account) {
					monthly_values = &monthly_cats[trans->toAccount()]; mi = &mi_c[trans->toAccount()]; isfirst = &isfirst_c[trans->toAccount()];
					sign = 1;
					include = true;
				}
				break;
			}
			case 5: {
				if((is_parent ? trans->fromAccount()->topAccount() : trans->fromAccount()) == current_account) {
					monthly_values = &monthly_incomes; mi = &mi_i; isfirst = &isfirst_i;
					sign = 1;
					include = true;
				} else if((is_parent ? trans->toAccount()->topAccount() : trans->toAccount()) == current_account) {
					monthly_values = &monthly_incomes; mi = &mi_i; isfirst = &isfirst_i;
					sign = -1;
					include = true;
				}
				break;
			}
			case 6: {
				if((is_parent ? trans->fromAccount()->topAccount() : trans->fromAccount()) == current_account) {
					monthly_values = &monthly_expenses; mi = &mi_e; isfirst = &isfirst_e;
					sign = -1;
					include = true;
				} else if((is_parent ? trans->toAccount()->topAccount() : trans->toAccount()) == current_account) {
					monthly_values = &monthly_expenses; mi = &mi_e; isfirst = &isfirst_e;
					sign = 1;
					include = true;
				}
				break;
			}
			case 7: {
				if((is_parent ? trans->fromAccount()->topAccount() : trans->fromAccount()) == current_account) {
					monthly_values = &monthly_desc[trans->description()]; mi = &mi_d[trans->description()]; isfirst = &isfirst_d[trans->description()];
					total_value = &desc_values[trans->description()];
					sign = 1;
					include = true;
				} else if((is_parent ? trans->toAccount()->topAccount() : trans->toAccount()) == current_account) {
					monthly_values = &monthly_desc[trans->description()]; mi = &mi_d[trans->description()]; isfirst = &isfirst_d[trans->description()];
					total_value = &desc_values[trans->description()];
					sign = -1;
					include = true;
				}
				break;
			}
			case 8: {
				if((is_parent ? trans->fromAccount()->topAccount() : trans->fromAccount()) == current_account) {
					monthly_values = &monthly_desc[trans->description()]; mi = &mi_d[trans->description()]; isfirst = &isfirst_d[trans->description()];
					total_value = &desc_values[trans->description()];
					sign = -1;
					include = true;
				} else if((is_parent ? trans->toAccount()->topAccount() : trans->toAccount()) == current_account) {
					monthly_values = &monthly_desc[trans->description()]; mi = &mi_d[trans->description()]; isfirst = &isfirst_d[trans->description()];
					total_value = &desc_values[trans->description()];
					sign = 1;
					include = true;
				}
				break;
			}
			case 9: {
				if((is_parent ? trans->fromAccount()->topAccount() : trans->fromAccount()) == current_account && trans->description() == current_description) {
					monthly_values = &monthly_incomes; mi = &mi_i; isfirst = &isfirst_i;
					sign = 1;
					include = true;
				} else if((is_parent ? trans->toAccount()->topAccount() : trans->toAccount()) == current_account && trans->description() == current_description) {
					monthly_values = &monthly_incomes; mi = &mi_i; isfirst = &isfirst_i;
					sign = -1;
					include = true;
				}
				break;
			}
			case 10: {
				if((is_parent ? trans->fromAccount()->topAccount() : trans->fromAccount()) == current_account && trans->description() == current_description) {
					monthly_values = &monthly_expenses; mi = &mi_e; isfirst = &isfirst_e;
					sign = -1;
					include = true;
				} else if((is_parent ? trans->toAccount()->topAccount() : trans->toAccount()) == current_account && trans->description() == current_description) {
					monthly_values = &monthly_expenses; mi = &mi_e; isfirst = &isfirst_e;
					sign = 1;
					include = true;
				}
				break;
			}
			case 11: {
				if(trans->type() != TRANSACTION_TYPE_INCOME) break;
				Income *income = (Income*) trans;
				if((is_parent ? income->category()->topAccount() : income->category()) == current_account) {
					monthly_values = &monthly_desc[income->payer()]; mi = &mi_d[income->payer()]; isfirst = &isfirst_d[income->payer()];
					total_value = &desc_values[income->payer()];
					sign = 1;
					include = true;
				}
				break;
			}
			case 12: {
				if(trans->type() != TRANSACTION_TYPE_EXPENSE) break;
				Expense *expense = (Expense*) trans;
				if((is_parent ? expense->category()->topAccount() : expense->category()) == current_account) {
					monthly_values = &monthly_desc[expense->payee()]; mi = &mi_d[expense->payee()]; isfirst = &isfirst_d[expense->payee()];
					total_value = &desc_values[expense->payee()];
					sign = 1;
					include = true;
				}
				break;
			}
			case 13: {
				if(trans->type() != TRANSACTION_TYPE_INCOME) break;
				Income *income = (Income*) trans;
				if((is_parent ? income->category()->topAccount() : income->category()) == current_account && income->description() == current_description) {
					monthly_values = &monthly_desc[income->payer()]; mi = &mi_d[income->payer()]; isfirst = &isfirst_d[income->payer()];
					total_value = &desc_values[income->payer()];
					sign = 1;
					include = true;
				}
				break;
			}
			case 14: {
				if(trans->type() != TRANSACTION_TYPE_EXPENSE) break;
				Expense *expense = (Expense*) trans;
				if((is_parent ? expense->category()->topAccount() : expense->category()) == current_account && expense->description() == current_description) {
					monthly_values = &monthly_desc[expense->payee()]; mi = &mi_d[expense->payee()]; isfirst = &isfirst_d[expense->payee()];
					total_value = &desc_values[expense->payee()];
					sign = 1;
					include = true;
				}
				break;
			}
			case 15: {
				if(trans->type() != TRANSACTION_TYPE_INCOME) break;
				Income *income = (Income*) trans;
				if((is_parent ? income->category()->topAccount() : income->category()) == current_account && income->payer() == current_payee) {
					monthly_values = &monthly_incomes; mi = &mi_i; isfirst = &isfirst_i;
					sign = 1;
					include = true;
				}
				break;
			}
			case 16: {
				if(trans->type() != TRANSACTION_TYPE_EXPENSE) break;
				Expense *expense = (Expense*) trans;
				if((is_parent ? expense->category()->topAccount() : expense->category()) == current_account && expense->payee() == current_payee) {
					monthly_values = &monthly_expenses; mi = &mi_e; isfirst = &isfirst_e;
					sign = 1;
					include = true;
				}
				break;
			}
			case 17: {
				if(trans->type() != TRANSACTION_TYPE_INCOME) break;
				Income *income = (Income*) trans;
				if((is_parent ? income->category()->topAccount() : income->category()) == current_account && income->payer() == current_payee) {
					monthly_values = &monthly_desc[income->description()]; mi = &mi_d[income->description()]; isfirst = &isfirst_d[income->description()];
					total_value = &desc_values[income->description()];
					sign = 1;
					include = true;
				}
				break;
			}
			case 18: {
				if(trans->type() != TRANSACTION_TYPE_EXPENSE) break;
				Expense *expense = (Expense*) trans;
				if((is_parent ? expense->category()->topAccount() : expense->category()) == current_account && expense->payee() == current_payee) {
					monthly_values = &monthly_desc[expense->description()]; mi = &mi_d[expense->description()]; isfirst = &isfirst_d[expense->description()];
					total_value = &desc_values[expense->description()];
					sign = 1;
					include = true;
				}
				break;
			}
			case 19: {
				if(trans->type() != TRANSACTION_TYPE_INCOME) break;
				Income *income = (Income*) trans;
				if((is_parent ? income->category()->topAccount() : income->category()) == current_account && income->description() == current_description && income->payer() == current_payee) {
					monthly_values = &monthly_incomes; mi = &mi_i; isfirst = &isfirst_i;
					sign = 1;
					include = true;
				}
				break;
			}
			case 20: {
				if(trans->type() != TRANSACTION_TYPE_EXPENSE) break;
				Expense *expense = (Expense*) trans;
				if((is_parent ? expense->category()->topAccount() : expense->category()) == current_account && expense->description() == current_description && expense->payee() == current_payee) {
					monthly_values = &monthly_expenses; mi = &mi_e; isfirst = &isfirst_e;
					sign = 1;
					include = true;
				}
				break;
			}
		}
		if(include) {
			QDate transdate = strans->firstOccurrence();
			QVector<chart_month_info>::iterator cmi_it = monthly_values->begin();
			do {
				if(transdate >= first_date) {
					while(cmi_it->date < transdate) {
						++cmi_it;
					}
					(*mi) = cmi_it;
					(*mi)->value += trans->value() * sign;
					(*mi)->count += trans->quantity();
					if(total_value) {
						if(type == 2) *total_value += trans->quantity();
						else *total_value += trans->value() * sign;
					}
				}
				if(strans->recurrence()) {
					transdate = strans->recurrence()->nextOccurrence(transdate);
				} else {
					break;
				}
			} while(transdate <= last_date);
		}
		strans = budget->scheduledTransactions.next();
	}

	account = current_account;
	desc_i = first_desc_i;
	desc_nr = 0;
	QDate imonth = budget->lastBudgetDay(QDate::currentDate());

	if(budget->isLastBudgetDay(QDate::currentDate())) {
		budget->addBudgetMonthsSetLast(imonth, 1);
	}

	bool includes_budget = false;
	at_expenses = false;
	if(source_org == 7) {desc_nr = descriptionCombo->count(); account = NULL;}
	else if(source_org == 11) {desc_nr = payeeCombo->count(); account = NULL;}
	else if(source_org == 0) {account = budget->incomesAccounts.first(); if(!account) {at_expenses = true; account = budget->expensesAccounts.first();}}
	else if(source_org == 3) account = budget->incomesAccounts.first();
	else if(source_org == 4) account = budget->expensesAccounts.first();
	else if(source_org == 21) account = current_account->subCategories.first();
	else if(source_org == 2) {mi = &mi_e; monthly_values = &monthly_expenses; isfirst = &isfirst_e;}
	else if(source_org == 1) {mi = &mi_i; monthly_values = &monthly_incomes; isfirst = &isfirst_i;}
	while(source_org == 1 || source_org == 2 || account || ((source_org == 7 || source_org == 11) && desc_i < desc_nr)) {
		while(exclude_subs && account && account->topAccount() != account) {
			if(source_org == 3) account = budget->incomesAccounts.next();
			else account = budget->expensesAccounts.next();
		}
		if(exclude_subs && !account) break;
		if(source_org == 3 || source_org == 4 || source_org == 0 || source_org == 21) {mi = &mi_c[account]; monthly_values = &monthly_cats[account]; isfirst = &isfirst_c[account];}
		else if(source_org == 7) {mi = &mi_d[descriptionCombo->itemText(desc_i)]; monthly_values = &monthly_desc[descriptionCombo->itemText(desc_i)]; isfirst = &isfirst_d[descriptionCombo->itemText(desc_i)];}
		else if(source_org == 11) {mi = &mi_d[payeeCombo->itemText(desc_i)]; monthly_values = &monthly_desc[payeeCombo->itemText(desc_i)]; isfirst = &isfirst_d[payeeCombo->itemText(desc_i)];}
		(*mi) = &monthly_values->front();
		QVector<chart_month_info>::iterator cmi_it = monthly_values->begin();
		bool in_future = false;
		while(cmi_it != monthly_values->end()) {
			(*mi) = cmi_it;
			if(account && type < 2 && current_source < 3) {
				if(!in_future && (*mi)->date >= imonth) {
					in_future = true;
				}
				if(in_future && account->type() != ACCOUNT_TYPE_ASSETS) {
					double d_budget = ((CategoryAccount*) account)->monthlyBudget(budget->budgetYear((*mi)->date), budget->budgetMonth((*mi)->date), false);					
					if(d_budget >= 0.0 && d_budget > (*mi)->value) {
						(*mi)->value = d_budget;
						includes_budget = true;
					}
				}
			}
			switch(type) {
				case 1: {
					if((*mi)->date >= last_date) {
						(*mi)->value /= ((*isfirst) ? (first_date.daysTo(last_date) + 1) : budget->dayOfBudgetMonth(last_date));
					} else {
						(*mi)->value /= ((*isfirst) ? (first_date.daysTo((*mi)->date) + 1) : budget->daysInBudgetMonth((*mi)->date));
					}
					break;
				}
				case 2: {(*mi)->value = (*mi)->count; break;}
				case 3: {if((*mi)->count > 0.0) (*mi)->value /= (*mi)->count; else (*mi)->value = 0.0; break;}
			}
			if((*mi)->value > maxvalue) maxvalue = (*mi)->value;
			else if((*mi)->value < minvalue) minvalue = (*mi)->value;
			if((*mi)->count > maxcount) maxcount = (*mi)->count;
			++cmi_it;
		}

		if(source_org == 7 || source_org == 11) desc_i++;
		else if(source_org == 3) account = budget->incomesAccounts.next();
		else if(source_org == 4) account = budget->expensesAccounts.next();
		else if(source_org == 21) {
			if(account == current_account) break;
			account = current_account->subCategories.next();
			if(!account) account = current_account;
		} else if(source_org == 0) {
			if(!at_expenses) {
				account = budget->incomesAccounts.next();
				if(!account) {
					at_expenses = true;
					account = budget->expensesAccounts.first();
				}
			} else {
				account = budget->expensesAccounts.next();
			}
		} else break;
	}

	bool second_run = false;
	if(current_source < 3) {
		maxvalue = 0.0;
		minvalue = 0.0;
		maxcount = 0.0;
	}

	while(current_source < 3) {
		if(current_source == -1 || current_source == 1 || second_run) monthly_values = &monthly_incomes;
		else monthly_values = &monthly_expenses;
		if(!second_run || current_source != -1) {
			monthly_values->append(chart_month_info());
			(*mi) = &monthly_values->back();
			(*mi)->value = 0.0;
			(*mi)->count = 0.0;
			(*mi)->date = budget->lastBudgetDay(first_date);
			while((*mi)->date < last_date) {
				QDate newdate = (*mi)->date;
				budget->addBudgetMonthsSetLast(newdate, 1);
				monthly_values->append(chart_month_info());
				(*mi) = &monthly_values->back();
				(*mi)->value = 0.0;
				(*mi)->count = 0.0;
				(*mi)->date = newdate;
			}
		}
		if(current_source == 1 || second_run) account = budget->incomesAccounts.first();
		else account = budget->expensesAccounts.first();
		while(account) {
			QVector<chart_month_info>::iterator cmi_it = monthly_values->begin();
			QVector<chart_month_info>::iterator cmi_itc = monthly_cats[account].begin();
			while(cmi_it != monthly_values->end() && cmi_itc != monthly_cats[account].end()) {
				(*mi) = cmi_it;
				if(!second_run && current_source == -1) (*mi)->value -= cmi_itc->value;
				else (*mi)->value += cmi_itc->value;
				(*mi)->count += cmi_itc->count;
				++cmi_it;
				++cmi_itc;
			}
			if(current_source == 1 || second_run) account = budget->incomesAccounts.next();
			else account = budget->expensesAccounts.next();
		}
		if(current_source != -1 || second_run) {
			QVector<chart_month_info>::iterator cmi_it = monthly_values->begin();
			while(cmi_it != monthly_values->end()) {
				(*mi) = cmi_it;
				if((*mi)->value > maxvalue) maxvalue = (*mi)->value;
				else if((*mi)->value < minvalue) minvalue = (*mi)->value;
				if((*mi)->count > maxcount) maxcount = (*mi)->count;
				++cmi_it;
			}
		}
		if(current_source > 0 || second_run) break;
		second_run = true;
	}
	switch(current_source) {
		case -1: {source_org = 1; break;}
		case 0: {source_org = 0; break;}
		case 1: {source_org = 1; break;}
		case 2: {source_org = 2; break;}
		default: {}
	}
#ifdef QT_CHARTS_LIB
	int chart_type = typeCombo->currentIndex() + 1;
#else
	int chart_type = 1;
#endif
	if(source_org == 7 || source_org == 11) {
		int max_series = 7;
		QString r_desc_str;
		if(current_source == 12 || current_source == 14) {
			r_desc_str = tr("Other payees");
		} else if(current_source == 11 || current_source == 13) {
			r_desc_str = tr("Other payers");
		} else {
			r_desc_str = tr("Other descriptions", "Referring to the generic description property");
		}
		monthly_desc[r_desc_str] = QVector<chart_month_info>();
		monthly_values = &monthly_desc[r_desc_str];
		monthly_values->append(chart_month_info());
		(*mi) = &monthly_values->back();
		(*mi)->value = 0.0;
		(*mi)->count = 0.0;
		(*mi)->date = budget->lastBudgetDay(first_date);
		while((*mi)->date < last_date) {
			QDate newdate = (*mi)->date;
			budget->addBudgetMonthsSetLast(newdate, 1);
			monthly_values->append(chart_month_info());
			(*mi) = &monthly_values->back();
			(*mi)->value = 0.0;
			(*mi)->count = 0.0;
			(*mi)->date = newdate;
		}
		desc_i = first_desc_i;
		QString desc_str;
		while(desc_i < desc_nr) {
			if(source_org == 7) desc_str = descriptionCombo->itemText(desc_i);
			else desc_str = payeeCombo->itemText(desc_i);
			bool b_added = false;
			if(desc_values[desc_str] != 0.0) {			
				int i = desc_order.size() - 1;
				b_added = true;
				while(i >= 0) {
					if(desc_values[desc_str] < desc_values[desc_order[i]]) {
						if(i == desc_order.size() - 1) {
							if(i < max_series - 1 || desc_nr - first_desc_i <= max_series) desc_order.push_back(desc_str);
							else b_added = false;
						} else {
							desc_order.insert(i + 1, desc_str);
						}
						break;
					}
					i--;
				}
				if(i < 0) desc_order.push_front(desc_str);
				if(desc_order.size() > max_series - 1 && desc_nr - first_desc_i > max_series) {
					QVector<chart_month_info> *monthly_values2 = &monthly_desc[desc_order.last()];
					QVector<chart_month_info>::iterator it2 = monthly_values2->begin();
					QVector<chart_month_info>::iterator it2_e = monthly_values2->end();
					QVector<chart_month_info>::iterator it = monthly_values->begin();
					QVector<chart_month_info>::iterator it_e = monthly_values->end();
					while(it != it_e && it2 != it2_e) {
						it->value += it2->value;
						++it;
						++it2;
					}
					desc_order.pop_back();
				}
				
			}
			if(!b_added) {
				QVector<chart_month_info> *monthly_values2 = &monthly_desc[desc_str];
				QVector<chart_month_info>::iterator it2 = monthly_values2->begin();
				QVector<chart_month_info>::iterator it2_e = monthly_values2->end();
				QVector<chart_month_info>::iterator it = monthly_values->begin();
				QVector<chart_month_info>::iterator it_e = monthly_values->end();
				while(it != it_e && it2 != it2_e) {
					it->value += it2->value;
					++it;
					++it2;
				}
			}
			desc_i++;
		}
		if(desc_order.isEmpty() && desc_nr > 0) {
			desc_i = first_desc_i;
			while(desc_i < desc_nr && desc_i - first_desc_i < max_series - 1) {
				if(source_org == 7) desc_str = descriptionCombo->itemText(desc_i);
				else desc_str = payeeCombo->itemText(desc_i);
				desc_order.push_back(desc_str);
				desc_i++;
			}
		}
		if(desc_nr - first_desc_i > desc_order.size()) {
			desc_order.push_back(r_desc_str);
		}
		maxvalue = 0.0;
		minvalue = 0.0;
		for(int i = 0; i < desc_order.size(); i++) {
			monthly_values = &monthly_desc[desc_order[i]];
			QVector<chart_month_info>::iterator it = monthly_values->begin();
			QVector<chart_month_info>::iterator it_e = monthly_values->end();
			while(it != it_e) {
				if(it->value > maxvalue) maxvalue = it->value;
				else if(it->value < minvalue) minvalue = it->value;
				++it;
			}
		}
	}
	int months = budget->calendarMonthsBetweenDates(first_date, last_date, true) + 1;
	int years = budget->budgetYear(last_date) - budget->budgetYear(first_date);
	int n = 0;
	QDate monthdate = first_date;
	QDate curmonth = budget->lastBudgetDay(budget->monthToBudgetMonth(end_date));
	while(monthdate <= curmonth) {
		budget->addBudgetMonthsSetLast(monthdate, 1);
		n++;
	}
	QString axis_string;
	switch(type) {
		case 1: {
			if(current_source == 0) axis_string = tr("Daily average value") + QString(" (%1)").arg(QLocale().currencySymbol());
			else if(current_source == -1) axis_string = tr("Daily average profit") + QString(" (%1)").arg(QLocale().currencySymbol());
			else if(current_source % 2 == 1) axis_string = tr("Daily average income") + QString(" (%1)").arg(QLocale().currencySymbol());
			else axis_string = tr("Daily average cost") + QString(" (%1)").arg(QLocale().currencySymbol());
			break;
		}
		case 3: {
			if(current_source == 0) axis_string = tr("Average value") + QString(" (%1)").arg(QLocale().currencySymbol());
			else if(current_source % 2 == 1) axis_string = tr("Average income") + QString(" (%1)").arg(QLocale().currencySymbol());
			else axis_string = tr("Average cost") + QString(" (%1)").arg(QLocale().currencySymbol());
			break;
		}
		default: {
			if(current_source == 0) axis_string = tr("Monthly value") + QString(" (%1)").arg(QLocale().currencySymbol());
			else if(current_source == -1) axis_string = tr("Monthly profit") + QString(" (%1)").arg(QLocale().currencySymbol());
			else if(current_source % 2 == 1) axis_string = tr("Monthly income") + QString(" (%1)").arg(QLocale().currencySymbol());
			else axis_string = tr("Monthly cost") + QString(" (%1)").arg(QLocale().currencySymbol());
			break;
		}
	}
	if(includes_budget) axis_string += QString("<div style=\"font-weight: normal\">(*") + tr("Includes budgeted transactions") + ")</div>";
	
	if(chart_type == 4) {
		QVector<double> total_values;
		desc_i = 0;
		desc_nr = desc_order.size();
		account = NULL;
		if(source_org == 3) account = budget->incomesAccounts.first();
		else if(source_org == 4) account = budget->expensesAccounts.first();
		else if(source_org == 21) account = current_account->subCategories.first();
		else if(source_org == 2) {monthly_values = &monthly_expenses;}
		else if(source_org == 1 || source_org == 0) {monthly_values = &monthly_incomes;}
		while(source_org < 3 || account || ((source_org == 7 || source_org == 11) && desc_i < desc_nr)) {
			while(exclude_subs && account && account->topAccount() != account) {
				if(source_org == 3) account = budget->incomesAccounts.next();
				else account = budget->expensesAccounts.next();
			}
			if(exclude_subs && !account) break;
			if(source_org == 3 || source_org == 4 || source_org == 21) {monthly_values = &monthly_cats[account];}
			else if(source_org == 7 || source_org == 11) {monthly_values = &monthly_desc[desc_order[desc_i]];}

			int index = 0;
			QVector<chart_month_info>::iterator it_e = monthly_values->end();
			for(QVector<chart_month_info>::iterator it = monthly_values->begin(); it != it_e; ++it) {
				if(index >= total_values.count()) total_values << it->value;
				else total_values[index] += it->value;
				index++;
			}
			
			if(source_org == 7 || source_org == 11) desc_i++;
			else if(source_org == 3) account = budget->incomesAccounts.next();
			else if(source_org == 4) account = budget->expensesAccounts.next();
			else if(source_org == 21) {
				if(account == current_account) break;
				account = current_account->subCategories.next();
				if(!account) account = current_account;
			} else if(source_org == 0 && monthly_values != &monthly_expenses) {monthly_values = &monthly_expenses;}
			else break;
		}
		for(int index = 0; index < total_values.count(); index++) {
			if(total_values[index] > maxvalue) maxvalue = total_values[index];
			if(total_values[index] < minvalue) minvalue = total_values[index];
		}
	}
	
	int y_lines = 5, y_minor = 0;

	calculate_minmax_lines(maxvalue, minvalue, y_lines, y_minor, current_source == -1, type != 2);
	
#ifdef QT_CHARTS_LIB
	
	bool show_legend;
	switch(current_source) {
		case -1:
		case 1:
		case 2:
		case 5:
		case 6:
		case 9:
		case 10:
		case 15:
		case 16:
		case 19:
		case 20: {show_legend = false; break;}
		default: show_legend = true;
	}

	switch(current_source) {
		case -1: {title_string = tr("Profits"); break;}
		case 0: {title_string = tr("Incomes & Expenses"); break;}
		case 3:
		case 1: {title_string = tr("Incomes"); break;}
		case 4:
		case 2: {title_string = tr("Expenses"); break;}
		case 21:
		case 11:
		case 7:
		case 5: {title_string = tr("Incomes: %1").arg(current_account->nameWithParent()); break;}
		case 22:
		case 12:
		case 8:
		case 6: {title_string = tr("Expenses: %1").arg(current_account->nameWithParent()); break;}
		case 13:
		case 9: {title_string = tr("Incomes: %2, %1").arg(current_account->nameWithParent()).arg(current_description); break;}
		case 14:
		case 10: {title_string = tr("Expenses: %2, %1").arg(current_account->nameWithParent()).arg(current_description); break;}
		case 19: {title_string = tr("Incomes: %3, %2, %1").arg(current_account->nameWithParent()).arg(current_description).arg(current_payee); break;}
		case 20: {title_string = tr("Expenses: %3, %2, %1").arg(current_account->nameWithParent()).arg(current_description).arg(current_payee); break;}
		case 23:
		case 17:
		case 15: {title_string = tr("Incomes: %2, %1").arg(current_account->nameWithParent()).arg(current_payee); break;}
		case 24:
		case 18:
		case 16: {title_string = tr("Expenses: %2, %1").arg(current_account->nameWithParent()).arg(current_payee); break;}
	}
	chart->removeAllSeries();

	int index = 0;
	account = NULL;
	desc_i = 0;
	desc_nr = desc_order.size();
	
	if(axisX) chart->removeAxis(axisX);
	last_date = budget->firstBudgetDay(last_date);
	
	if(axisY) chart->removeAxis(axisY);
	axisY = new QValueAxis();
	axisY->setTitleText(axis_string);
	
	QAbstractBarSeries *bar_series = NULL;
	
	if(chart_type == 1) {
		QCategoryAxis *c_axisX = new QCategoryAxis();
		axisX = c_axisX;
	
		bool single_month = (first_date == last_date);
		if(single_month) c_axisX->setRange(QDateTime(first_date).toMSecsSinceEpoch() - 1296000000, QDateTime(last_date).toMSecsSinceEpoch() + 1296000000);
		else c_axisX->setRange(QDateTime(first_date).toMSecsSinceEpoch(), QDateTime(last_date).toMSecsSinceEpoch());
	
		if(months > 72) {
			int yearjump = years / 12 + 1;
			QDate axis_date = first_date;		
			if(budget->budgetMonth(axis_date) > 1) {
				axis_date = budget->firstBudgetDayOfYear(axis_date);
				budget->addBudgetMonthsSetFirst(axis_date, 12);
			}
			c_axisX->setStartValue(QDateTime(axis_date).toMSecsSinceEpoch());
			c_axisX->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
			while(axis_date <= last_date) {
				QDate next_axis_date = axis_date;
				budget->addBudgetMonthsSetFirst(next_axis_date, 12 * yearjump);
				if(includes_budget && next_axis_date >= imonth) c_axisX->append(budget->budgetDateToMonth(axis_date).toString("yyyy*"), QDateTime(axis_date).toMSecsSinceEpoch());
				else c_axisX->append(budget->budgetDateToMonth(axis_date).toString("yyyy"), QDateTime(axis_date).toMSecsSinceEpoch());
				axis_date = next_axis_date;
			}
		} else {
			int monthjump = 1;
			if(months >= 48) monthjump = 6;
			else if(months >= 36) monthjump = 4;
			else if(months >= 24) monthjump = 3;
			else if(months >= 12) monthjump = 2;
			QDate axis_date = first_date;
			if(monthjump > 1) {
				int mod_m = (budget->budgetMonth(axis_date) % monthjump);
				if(mod_m == 0) budget->addBudgetMonthsSetFirst(axis_date, 1);
				else if(mod_m != 1) budget->addBudgetMonthsSetFirst(axis_date, monthjump - mod_m + 1);
			}
			c_axisX->setStartValue(QDateTime(axis_date).toMSecsSinceEpoch());
			c_axisX->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
			QString unique_s;
			bool month_shown = false;
			while(axis_date <= last_date) {
				QDate next_axis_date = axis_date;
				budget->addBudgetMonthsSetFirst(next_axis_date, monthjump);
				if(budget->budgetMonth(axis_date) == 1) {
					if(month_shown) unique_s += " ";
					if(includes_budget && next_axis_date > imonth) c_axisX->append(budget->budgetDateToMonth(axis_date).toString("MMM*<br>yyyy"), QDateTime(axis_date).toMSecsSinceEpoch());
					else c_axisX->append(budget->budgetDateToMonth(axis_date).toString("MMM<br>yyyy"), QDateTime(axis_date).toMSecsSinceEpoch());
				} else {
					if(includes_budget && next_axis_date > imonth) c_axisX->append(budget->budgetDateToMonth(axis_date).toString("MMM*%1").arg(unique_s), QDateTime(axis_date).toMSecsSinceEpoch());
					else c_axisX->append(budget->budgetDateToMonth(axis_date).toString("MMM%1").arg(unique_s), QDateTime(axis_date).toMSecsSinceEpoch());
				}
				month_shown = true;
				axis_date = next_axis_date;
			}
		}
	} else {
		QBarCategoryAxis *bc_axisX = new QBarCategoryAxis();
		axisX = bc_axisX;
		QDate axis_date = first_date;
		while(axis_date <= last_date) {
			QDate next_axis_date = axis_date;
			budget->addBudgetMonthsSetFirst(next_axis_date, 1);
			if(includes_budget && next_axis_date > imonth) bc_axisX->append(budget->budgetDateToMonth(axis_date).toString("MMMM*"));
			else bc_axisX->append(budget->budgetDateToMonth(axis_date).toString("MMMM"));			
			axis_date = next_axis_date;
		}
		if(chart_type == 2) bar_series = new QBarSeries();
		else if(chart_type == 3) bar_series = new QHorizontalBarSeries();
		else bar_series = new QStackedBarSeries();
	}
	if(chart_type == 3) {
		chart->addAxis(axisY, Qt::AlignBottom);
		chart->addAxis(axisX, Qt::AlignLeft);
	} else {
		chart->addAxis(axisY, Qt::AlignLeft);
		chart->addAxis(axisX, Qt::AlignBottom);
	}

	axisY->setRange(minvalue, maxvalue);
	axisY->setTickCount(y_lines + 1);
	axisY->setMinorTickCount(y_minor);
	if(type == 2 || (maxvalue - minvalue) >= 50.0) axisY->setLabelFormat(QString("%.0f"));
	else axisY->setLabelFormat(QString("%.%1f").arg(QString::number(MONETARY_DECIMAL_PLACES)));
	
	if(source_org == 3) account = budget->incomesAccounts.first();
	else if(source_org == 4) account = budget->expensesAccounts.first();
	else if(source_org == 21) account = current_account->subCategories.first();
	else if(source_org == 2) {monthly_values = &monthly_expenses;}
	else if(source_org == 1 || source_org == 0) {monthly_values = &monthly_incomes;}
	while(source_org < 3 || account || ((source_org == 7 || source_org == 11) && desc_i < desc_nr)) {
		while(exclude_subs && account && account->topAccount() != account) {
			if(source_org == 3) account = budget->incomesAccounts.next();
			else account = budget->expensesAccounts.next();
		}
		if(exclude_subs && !account) break;
		if(source_org == 3 || source_org == 4 || source_org == 21) {monthly_values = &monthly_cats[account];}
		else if(source_org == 7 || source_org == 11) {monthly_values = &monthly_desc[desc_order[desc_i]];}
		
		QString series_name;
		switch(current_source) {
			case -1: {
				series_name = tr("Profits");
				break;
			}
			case 0: {
				if(index == 0) series_name = tr("Incomes");
				else if(index == 1) series_name = tr("Expenses");
				break;
			}
			case 1: {series_name = tr("Incomes"); break;}
			case 2: {series_name = tr("Expenses"); break;}
			case 3: {}
			case 4: {series_name = account->nameWithParent(); break;}
			case 21: {}
			case 22: {series_name = account->name(); break;}
			case 23: {}
			case 24: {
				series_name = tr("%1/%2", "%1: Category; %2: Payee/Payer").arg(account->name()).arg(current_payee);
				break;
			}
			case 5: {}
			case 6: {series_name = current_account->nameWithParent(); break;}
			case 17: {}
			case 18: {}
			case 7: {}
			case 8: {
				if(desc_order[desc_i].isEmpty()) series_name = tr("No description", "Referring to the generic description property");
				else series_name = desc_order[desc_i];
				break;
			}
			case 9: {}
			case 10: {
				if(current_description.isEmpty()) series_name = tr("No description", "Referring to the generic description property");
				else series_name = current_description;
				break;
			}
			case 13: {}
			case 11: {
				if(desc_order[desc_i].isEmpty()) series_name = tr("No payer");
				else series_name = desc_order[desc_i];
				break;
			}
			case 14: {}
			case 12: {
				if(desc_order[desc_i].isEmpty()) series_name = tr("No payee");
				else series_name = desc_order[desc_i];
				break;
			}
			case 15: {
				if(current_payee.isEmpty()) series_name = tr("No payer");
				else series_name = current_payee;
				break;
			}
			case 16: {
				if(current_payee.isEmpty()) series_name = tr("No payee");
				else series_name = current_payee;
				break;
			}
			case 19: {
				QString str1, str2;
				if(current_payee.isEmpty() && current_description.isEmpty()) {str1 = tr("No description", "Referring to the generic description property"); str2 = tr("no payer");}
				else if(current_payee.isEmpty()) {str1 = current_description; str2 = tr("no payer");}
				else if(current_description.isEmpty()) {str1 = tr("No description", "Referring to the generic description property"); str2 = current_payee;}
				else {str1 = current_description; str2 = current_payee;}
				series_name = tr("%1/%2", "%1: Generic Description; %2: Payer/Payer").arg(str1).arg(str2);
				break;
			}
			case 20: {
				QString str1, str2;
				if(current_payee.isEmpty() && current_description.isEmpty()) {str1 = tr("No description", "Referring to the generic description property"); str2 = tr("no payee");}
				else if(current_payee.isEmpty()) {str1 = current_description; str2 = tr("no payee");}
				else if(current_description.isEmpty()) {str1 = tr("No description", "Referring to the generic description property"); str2 = current_payee;}
				else {str1 = current_description; str2 = current_payee;}
				series_name = tr("%1/%2", "%1: Generic Description; %2: Payee/Payer").arg(str1).arg(str2);
				break;
			}
		}
		
		if(chart_type == 1) {
			QXYSeries *series;
			if(monthly_values->count() == 1) series = new QScatterSeries();
			else series = new QLineSeries();
			
			series->setName(series_name);

			QVector<chart_month_info>::iterator it_e = monthly_values->end();
			for(QVector<chart_month_info>::iterator it = monthly_values->begin(); it != it_e; ++it) {
				QDate date = budget->firstBudgetDay(it->date);
				series->append(QDateTime(date).toMSecsSinceEpoch(), it->value);
			}
			
			chart->addSeries(series);
			series->attachAxis(axisY);
			series->attachAxis(axisX);
		
			if(index >= 5) {
				QPen pen = series->pen();
				if(index >= 10) pen.setStyle(Qt::DashLine);
				else pen.setStyle(Qt::DotLine);
				series->setPen(pen);
				QList<QLegendMarker*> markers = chart->legend()->markers(series);
				if(markers.count() > 0) {
					pen.setWidth(2);
					markers[0]->setPen(pen);
					markers[0]->setBrush(QColor(0, 0, 0, 0));
				}
			}
			connect(series, SIGNAL(hovered(const QPointF&, bool)), this, SLOT(onSeriesHovered(const QPointF&, bool)));
		} else {
			QBarSet *bar_set = new QBarSet(series_name);
			QVector<chart_month_info>::iterator it_e = monthly_values->end();
			for(QVector<chart_month_info>::iterator it = monthly_values->begin(); it != it_e; ++it) {
				bar_set->append(it->value);
			}
			bar_series->append(bar_set);
		}
		
		index++;
		if(source_org == 7 || source_org == 11) desc_i++;
		else if(source_org == 3) account = budget->incomesAccounts.next();
		else if(source_org == 4) account = budget->expensesAccounts.next();
		else if(source_org == 21) {
			if(account == current_account) break;
			account = current_account->subCategories.next();
			if(!account) account = current_account;
		} else if(source_org == 0 && monthly_values != &monthly_expenses) {monthly_values = &monthly_expenses;}
		else break;
	}
	
	if(chart_type != 1) {
		chart->addSeries(bar_series);
		bar_series->attachAxis(axisY);
		bar_series->attachAxis(axisX);
		if(chart_type == 2 || chart_type == 3) {
			if(index > 2) bar_series->setBarWidth(1.0);
			else if(index > 1) bar_series->setBarWidth(0.75);
		}
		connect(bar_series, SIGNAL(hovered(bool, int, QBarSet*)), this, SLOT(onSeriesHovered(bool, int, QBarSet*)));
	}
	
	foreach(QLegendMarker* marker, chart->legend()->markers()) {
		connect(marker, SIGNAL(clicked()), this, SLOT(legendClicked()));
	}
	
	chart->setTitle(QString("<h2>%1<h2>").arg(title_string));
	if(show_legend) {
		chart->legend()->setAlignment(Qt::AlignBottom);
		chart->legend()->show();
	} else {
		chart->legend()->hide();
	}
#else
	QGraphicsScene *oldscene = scene;
	scene = new QGraphicsScene(this);
	scene->setBackgroundBrush(Qt::white);
	QFont legend_font = font();
	QFontMetrics fm(legend_font);
	int fh = fm.height();

	int linelength = (int) ceil(600.0 / n);
	int chart_height = 400;
	int margin = 30 + fh;
	int chart_y = margin + 15;
	int axis_width = 11;
	//int axis_height = 11 + fh;
	int chart_width = linelength * n;

	int max_axis_value_width = 0;
	for(int i = 0; i <= y_lines; i++) {
		int w;
		if(type == 2 || maxvalue - minvalue >= 50.0) w = fm.width(QLocale().toString((int) round(maxvalue - (((maxvalue - minvalue) * i) / y_lines))));
		else w = fm.width(QLocale().toString((maxvalue - (((maxvalue - minvalue) * i) / y_lines))));
		if(w > max_axis_value_width) max_axis_value_width = w;
	}
	axis_width += max_axis_value_width;

	QGraphicsSimpleTextItem *axis_text = new QGraphicsSimpleTextItem();
	
	axis_text->setText(axis_string);
	axis_text->setFont(legend_font);
	axis_text->setBrush(Qt::black);
	if(axis_text->boundingRect().width() / 2 > max_axis_value_width) max_axis_value_width = axis_text->boundingRect().width() / 2;
	axis_text->setPos(margin + axis_width - axis_text->boundingRect().width() / 2, margin - fh);
	scene->addItem(axis_text);

	QPen axis_pen;
	axis_pen.setColor(Qt::black);
	axis_pen.setWidth(1);
	axis_pen.setStyle(Qt::SolidLine);
	QGraphicsLineItem *y_axis = new QGraphicsLineItem();
	y_axis->setLine(margin + axis_width, margin + 3, margin + axis_width, chart_height + chart_y);
	y_axis->setPen(axis_pen);
	scene->addItem(y_axis);

	QGraphicsLineItem *y_axis_dir1 = new QGraphicsLineItem();
	y_axis_dir1->setLine(margin + axis_width, margin + 3, margin + axis_width - 3, margin + 9);
	y_axis_dir1->setPen(axis_pen);
	scene->addItem(y_axis_dir1);

	QGraphicsLineItem *y_axis_dir2 = new QGraphicsLineItem();
	y_axis_dir2->setLine(margin + axis_width, margin + 3, margin + axis_width + 3, margin + 9);
	y_axis_dir2->setPen(axis_pen);
	scene->addItem(y_axis_dir2);

	QGraphicsLineItem *x_axis = new QGraphicsLineItem();
	x_axis->setLine(margin + axis_width, chart_height + chart_y, margin + chart_width + axis_width + 12, chart_height + chart_y);
	x_axis->setPen(axis_pen);
	scene->addItem(x_axis);

	QGraphicsLineItem *x_axis_dir1 = new QGraphicsLineItem();
	x_axis_dir1->setLine(margin + chart_width + axis_width + 6, chart_height + chart_y - 3, margin + chart_width + axis_width + 12, chart_height + chart_y);
	x_axis_dir1->setPen(axis_pen);
	scene->addItem(x_axis_dir1);

	QGraphicsLineItem *x_axis_dir2 = new QGraphicsLineItem();
	x_axis_dir2->setLine(margin + chart_width + axis_width + 6, chart_height + chart_y + 3, margin + chart_width + axis_width + 12, chart_height + chart_y);
	x_axis_dir2->setPen(axis_pen);
	scene->addItem(x_axis_dir2);

	axis_text = new QGraphicsSimpleTextItem(tr("Time"));
	axis_text->setFont(legend_font);
	axis_text->setBrush(Qt::black);
	axis_text->setPos(margin + chart_width + axis_width + 15, chart_y + chart_height - fh / 2);
	scene->addItem(axis_text);

	int x_axis_extra_width = 15 + axis_text->boundingRect().width();

	QPen div_pen;
	div_pen.setColor(Qt::black);
	div_pen.setWidth(1);
	div_pen.setStyle(Qt::DotLine);
	int div_height = chart_height / y_lines;
	for(int i = 0; i < y_lines; i++) {
		QGraphicsLineItem *y_div = new QGraphicsLineItem();
		y_div->setLine(margin + axis_width, chart_y + i * div_height, margin + chart_width + axis_width, chart_y + i * div_height);
		y_div->setPen(div_pen);
		scene->addItem(y_div);
		QGraphicsLineItem *y_mark = new QGraphicsLineItem();
		y_mark->setLine(margin + axis_width - 10, chart_y + i * div_height, margin + axis_width, chart_y + i * div_height);
		y_mark->setPen(axis_pen);
		scene->addItem(y_mark);
		axis_text = new QGraphicsSimpleTextItem();
		if(type == 2 || (maxvalue - minvalue) >= 50.0) axis_text->setText(QLocale().toString((int) round(maxvalue - (((maxvalue - minvalue) * i) / y_lines))));
		else axis_text->setText(QLocale().toString((maxvalue - (((maxvalue - minvalue) * i) / y_lines))));
		axis_text->setFont(legend_font);
		axis_text->setBrush(Qt::black);
		axis_text->setPos(margin + axis_width - axis_text->boundingRect().width() - 11, chart_y + i * div_height - fh / 2);
		scene->addItem(axis_text);
	}

	axis_text = new QGraphicsSimpleTextItem();
	if(minvalue != 0.0) {
		if(type == 2 || (maxvalue - minvalue) >= 50.0) axis_text->setText(QLocale().toString((int) round(minvalue)));
		else axis_text->setText(QLocale().toString(minvalue));
	} else {
		axis_text->setText(QLocale().toString(0));
	}
	axis_text->setFont(legend_font);
	axis_text->setBrush(Qt::black);
	axis_text->setPos(margin + axis_width - axis_text->boundingRect().width() - 11, chart_y + chart_height - fh / 2);
	scene->addItem(axis_text);

	int index = 0;
	int year = first_date.year() - 1;
	bool b_month_names = months <= 12, b_long_month_names = true;
	if(b_month_names) {
		monthdate = first_date;
		while(monthdate <= curmonth) {
			if(b_long_month_names) {
				if(fm.width(QDate::longMonthName(budget->budgetMonth(monthdate), QDate::StandaloneFormat)) > linelength - 8) {
					b_long_month_names = false;
				}
			}
			if(!b_long_month_names) {
				if(fm.width(QDate::shortMonthName(budget->budgetMonth(monthdate), QDate::StandaloneFormat)) > linelength) {
					b_month_names = false;
					break;
				}
			}
			budget->addBudgetMonths(monthdate, 1);
		}
	}
	monthdate = first_date;
	while(monthdate <= curmonth) {
		if(years < 5 || year != budget->budgetYear(monthdate)) {
			QGraphicsLineItem *x_mark = new QGraphicsLineItem();
			x_mark->setLine(margin + axis_width + index * linelength, chart_height + chart_y, margin + axis_width + index * linelength, chart_height + chart_y + 10);
			x_mark->setPen(axis_pen);
			scene->addItem(x_mark);
		}
		if(b_month_names) {
			QGraphicsSimpleTextItem *axis_text = new QGraphicsSimpleTextItem(b_long_month_names ? QDate::longMonthName(budget->budgetMonth(monthdate), QDate::StandaloneFormat) : QDate::shortMonthName(budget->budgetMonth(monthdate), QDate::StandaloneFormat));
			axis_text->setFont(legend_font);
			axis_text->setBrush(Qt::black);
			axis_text->setPos(margin + axis_width + index * linelength + (linelength - axis_text->boundingRect().width()) / 2, chart_height + chart_y + 11);
			scene->addItem(axis_text);
		} else if(year != budget->budgetYear(monthdate)) {
			year = budget->budgetYear(monthdate);
			QGraphicsSimpleTextItem *axis_text = new QGraphicsSimpleTextItem(QString::number(budget->budgetYear(monthdate)));
			axis_text->setFont(legend_font);
			axis_text->setBrush(Qt::black);
			axis_text->setPos(margin + axis_width + index * linelength, chart_height + chart_y + 11);
			scene->addItem(axis_text);
		}
		budget->addBudgetMonths(monthdate, 1);
		index++;
	}
	QGraphicsLineItem *x_mark = new QGraphicsLineItem();
	x_mark->setLine(margin + axis_width + index * linelength, chart_height + chart_y, margin + axis_width + index * linelength, chart_height + chart_y + 10);
	x_mark->setPen(axis_pen);
	scene->addItem(x_mark);

	int line_y = chart_height + chart_y;
	int line_x = margin + axis_width;
	int text_width = 0;
	int legend_x = chart_width + axis_width + margin + x_axis_extra_width + 10;
	int legend_y = margin;
	index = 0;
	account = NULL;
	desc_i = 0;
	desc_nr = desc_order.size();
	
	if(source_org == 3) account = budget->incomesAccounts.first();
	else if(source_org == 4) account = budget->expensesAccounts.first();
	else if(source_org == 21) account = current_account->subCategories.first();
	else if(source_org == 2) {monthly_values = &monthly_expenses;}
	else if(source_org == 1 || source_org == 0) {monthly_values = &monthly_incomes;}
	while(source_org < 3 || account || ((source_org == 7 || source_org == 11) && desc_i < desc_nr)) {
		while(exclude_subs && account && account->topAccount() != account) {
			if(source_org == 3) account = budget->incomesAccounts.next();
			else account = budget->expensesAccounts.next();
		}
		if(exclude_subs && !account) break;
		if(source_org == 3 || source_org == 4 || source_org == 21) {monthly_values = &monthly_cats[account];}
		else if(source_org == 7 || source_org == 11) {monthly_values = &monthly_desc[desc_order[desc_i]];}
		int prev_y = 0;
		int index2 = 0;
		QVector<chart_month_info>::iterator it_e = monthly_values->end();
		for(QVector<chart_month_info>::iterator it = monthly_values->begin(); it != it_e; ++it) {
			if(index2 == 0) {
				prev_y = (int) floor((chart_height * (it->value - minvalue)) / (maxvalue - minvalue)) + 1;
				if(n == 1) {
					QGraphicsEllipseItem *dot = new QGraphicsEllipseItem(-2.5, -2.5, 5, 5);
					dot->setPos(line_x + linelength / 2, line_y - prev_y);
					QBrush brush(getColor2(index));
					dot->setBrush(brush);
					dot->setZValue(10);
					scene->addItem(dot);
				}
			} else {
				int next_y = (int) floor((chart_height * (it->value - minvalue)) / (maxvalue - minvalue)) + 1;
				QGraphicsLineItem *line = new QGraphicsLineItem();
				line->setPen(getPen(index));
				line->setLine(line_x + ((index2 - 1) * linelength) + linelength / 2, line_y - prev_y, line_x + (index2 * linelength) + linelength / 2, line_y - next_y);
				line->setZValue(10);
				prev_y = next_y;
				scene->addItem(line);
			}
			index2++;
		}
		QGraphicsLineItem *legend_line = new QGraphicsLineItem();
		legend_line->setLine(legend_x + 10, legend_y + 10 + (fh + 5) * index + fh / 2, legend_x + 10 + fh, legend_y + 10 + (fh + 5) * index + fh / 2);
		legend_line->setPen(getPen(index));
		scene->addItem(legend_line);
		QGraphicsSimpleTextItem *legend_text = new QGraphicsSimpleTextItem();
		switch(current_source) {
			case -1: {
				legend_text->setText(tr("Profits"));
				break;
			}
			case 0: {
				if(index == 0) legend_text->setText(tr("Incomes"));
				else if(index == 1) legend_text->setText(tr("Expenses"));
				break;
			}
			case 1: {legend_text->setText(tr("Incomes")); break;}
			case 2: {legend_text->setText(tr("Expenses")); break;}
			case 3: {}
			case 4: {legend_text->setText(account->nameWithParent()); break;}
			case 21: {}
			case 22: {legend_text->setText(account->name()); break;}
			case 23: {}
			case 24: {
				legend_text->setText(tr("%1/%2", "%1: Category; %2: Payee/Payer").arg(account->name()).arg(current_payee));
				break;
			}
			case 5: {}
			case 6: {legend_text->setText(current_account->nameWithParent()); break;}
			case 17: {}
			case 18: {}
			case 7: {}
			case 8: {
				if(desc_order[desc_i].isEmpty()) legend_text->setText(tr("No description", "Referring to the generic description property"));
				else legend_text->setText(desc_order[desc_i]);
				break;
			}
			case 9: {}
			case 10: {
				if(current_description.isEmpty()) legend_text->setText(tr("No description", "Referring to the generic description property"));
				else legend_text->setText(current_description);
				break;
			}
			case 13: {}
			case 11: {
				if(desc_order[desc_i].isEmpty()) legend_text->setText(tr("No payer"));
				else legend_text->setText(desc_order[desc_i]);
				break;
			}
			case 14: {}
			case 12: {
				if(desc_order[desc_i].isEmpty()) legend_text->setText(tr("No payee"));
				else legend_text->setText(desc_order[desc_i]);
				break;
			}
			case 15: {
				if(current_payee.isEmpty()) legend_text->setText(tr("No payer"));
				else legend_text->setText(current_payee);
				break;
			}
			case 16: {
				if(current_payee.isEmpty()) legend_text->setText(tr("No payee"));
				else legend_text->setText(current_payee);
				break;
			}
			case 19: {
				QString str1, str2;
				if(current_payee.isEmpty() && current_description.isEmpty()) {str1 = tr("No description", "Referring to the generic description property"); str2 = tr("no payer");}
				else if(current_payee.isEmpty()) {str1 = current_description; str2 = tr("no payer");}
				else if(current_description.isEmpty()) {str1 = tr("No description", "Referring to the generic description property"); str2 = current_payee;}
				else {str1 = current_description; str2 = current_payee;}
				legend_text->setText(tr("%1/%2", "%1: Generic Description; %2: Payer/Payer").arg(str1).arg(str2));
				break;
			}
			case 20: {
				QString str1, str2;
				if(current_payee.isEmpty() && current_description.isEmpty()) {str1 = tr("No description", "Referring to the generic description property"); str2 = tr("no payee");}
				else if(current_payee.isEmpty()) {str1 = current_description; str2 = tr("no payee");}
				else if(current_description.isEmpty()) {str1 = tr("No description", "Referring to the generic description property"); str2 = current_payee;}
				else {str1 = current_description; str2 = current_payee;}
				legend_text->setText(tr("%1/%2", "%1: Generic Description; %2: Payee/Payer").arg(str1).arg(str2));
				break;
			}
		}
		if(legend_text->boundingRect().width() > text_width) text_width = legend_text->boundingRect().width();
		legend_text->setFont(legend_font);
		legend_text->setBrush(Qt::black);
		legend_text->setPos(legend_x + 10 + fh + 5, legend_y + 10 + (fh + 5) * index);
		scene->addItem(legend_text);
		index++;
		if(source_org == 7 || source_org == 11) desc_i++;
		else if(source_org == 3) account = budget->incomesAccounts.next();
		else if(source_org == 4) account = budget->expensesAccounts.next();
		else if(source_org == 21) {
			if(account == current_account) break;
			account = current_account->subCategories.next();
			if(!account) account = current_account;
		} else if(source_org == 0 && monthly_values != &monthly_expenses) {monthly_values = &monthly_expenses;}
		else break;
	}

	QGraphicsRectItem *legend_outline = new QGraphicsRectItem(legend_x, legend_y, 10 + fh + 5 + text_width + 10, 10 + ((fh + 5) * index) + 5);
	legend_outline->setPen(QPen(Qt::black));
	scene->addItem(legend_outline);

	QRectF rect = scene->sceneRect();
	rect.setX(0);
	rect.setY(0);
	rect.setRight(rect.right() + 20);
	rect.setBottom(rect.bottom() + 20);
	scene->setSceneRect(rect);
	scene->update();
	view->setScene(scene);
	view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
	if(oldscene) {
		delete oldscene;
	}
#endif
	
	if(current_source == 7 || current_source == 8 || current_source == 17 || current_source == 18) {
		if(has_empty_description) descriptionCombo->setItemText(descriptionCombo->count() - 1, tr("No description", "Referring to the generic description property"));
	} else if(current_source == 12 || current_source == 14) {
		if(has_empty_payee) payeeCombo->setItemText(payeeCombo->count() - 1, tr("No payee"));
	} else if(current_source == 11 || current_source == 13) {
		if(has_empty_payee) payeeCombo->setItemText(payeeCombo->count() - 1, tr("No payer"));
	}
}
#ifndef QT_CHARTS_LIB
void OverTimeChart::resizeEvent(QResizeEvent *e) {
	QWidget::resizeEvent(e);
	if(scene) {
		view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
	}
}
#endif
void OverTimeChart::updateTransactions() {
	if(descriptionCombo->isEnabled() && current_account) {
		bool b_income = (current_account->type() == ACCOUNT_TYPE_INCOMES);
		int curindex = descriptionCombo->currentIndex();
		if(curindex > 1) {
			curindex = 0;
		}
		int curindex_p = 0;
		if(b_extra) {
			curindex_p = payeeCombo->currentIndex();
			if(curindex_p > 1) {
				curindex_p = 0;
			}
		}
		descriptionCombo->blockSignals(true);
		descriptionCombo->clear();
		descriptionCombo->addItem(tr("All Descriptions Combined", "Referring to the generic description property"));
		descriptionCombo->addItem(tr("All Descriptions Split", "Referring to the generic description property"));
		if(b_extra) {
			if(b_income) payeeCombo->addItem(tr("All Payers Combined"));
			else payeeCombo->addItem(tr("All Payees Combined"));
			if(b_income) payeeCombo->addItem(tr("All Payers Split"));
			else payeeCombo->addItem(tr("All Payees Split"));
		}
		has_empty_description = false;
		has_empty_payee = false;
		QMap<QString, bool> descriptions, payees;
		Transaction *trans = budget->transactions.first();
		while(trans) {
			if((trans->fromAccount() == current_account || trans->toAccount() == current_account)) {
				if(trans->description().isEmpty()) has_empty_description = true;
				else descriptions[trans->description()] = true;
				if(b_extra) {
					if(trans->type() == TRANSACTION_TYPE_EXPENSE) {
						if(((Expense*) trans)->payee().isEmpty()) has_empty_payee = true;
						else payees[((Expense*) trans)->payee()] = true;
					} else if(trans->type() == TRANSACTION_TYPE_INCOME) {
						if(((Income*) trans)->payer().isEmpty()) has_empty_payee = true;
						else payees[((Income*) trans)->payer()] = true;
					}
				}
			}
			trans = budget->transactions.next();
		}
		QMap<QString, bool>::iterator it_e = descriptions.end();
		int i = 2;
		for(QMap<QString, bool>::iterator it = descriptions.begin(); it != it_e; ++it) {
			if((current_source == 9 || current_source == 10 || current_source == 13 || current_source == 14 || current_source == 19 || current_source == 20) && it.key() == current_description) {
				curindex = i;
			}
			descriptionCombo->addItem(it.key());
			i++;
		}
		if(has_empty_description) {
			if((current_source == 9 || current_source == 10 || current_source == 13 || current_source == 14 || current_source == 19 || current_source == 20) && current_description.isEmpty()) curindex = i;
			descriptionCombo->addItem(tr("No description", "Referring to the generic description property"));
		}
		if(b_extra) {
			i = 2;
			QMap<QString, bool>::iterator it2_e = payees.end();
			for(QMap<QString, bool>::iterator it2 = payees.begin(); it2 != it2_e; ++it2) {
				if(it2.key() == current_payee) {
					curindex_p = i;
				}
				payeeCombo->addItem(it2.key());
				i++;
			}
			if(has_empty_payee) {
				if(current_payee.isEmpty()) {
					curindex_p = i;
				}
				if(b_income) payeeCombo->addItem(tr("No payer"));
				else payeeCombo->addItem(tr("No payee"));
			}
		}
		if(curindex < descriptionCombo->count()) {
			descriptionCombo->setCurrentIndex(curindex);
		}
		if(b_extra && curindex_p < payeeCombo->count()) {
			payeeCombo->setCurrentIndex(curindex_p);
		}
		int d_index = descriptionCombo->currentIndex();
		int p_index = 0;
		if(b_extra) p_index = payeeCombo->currentIndex();
		if(d_index <= 1) current_description = "";
		if(p_index <= 1) current_payee = "";
		if(d_index == 0) {
			if(p_index == 0) current_source = b_income ? 5 : 6;
			else if(p_index == 1) current_source = b_income ? 11 : 12;
			else current_source = b_income ? 15 : 16;
		} else if(d_index == 1) {
			if(p_index == 0) current_source = b_income ? 7 : 8;
			else current_source = b_income ? 17 : 18;
		} else {
			if(p_index == 0) current_source = b_income ? 9 : 10;
			else if(p_index == 1) current_source = b_income ? 13 : 14;
			else current_source = b_income ? 19 : 20;
		}
		descriptionCombo->blockSignals(false);
		if(b_extra) payeeCombo->blockSignals(false);
	}
	updateDisplay();
}
void OverTimeChart::updateAccounts() {
	if(categoryCombo->isEnabled()) {
		int curindex = categoryCombo->currentIndex();
		if(curindex > 1) {
			curindex = 1;
		}
		categoryCombo->blockSignals(true);
		descriptionCombo->blockSignals(true);
		if(b_extra) payeeCombo->blockSignals(true);
		categoryCombo->clear();
		categoryCombo->addItem(tr("All Categories Combined"));
		categoryCombo->addItem(tr("All Categories Split"));
		int i = 2;
		if(sourceCombo->currentIndex() == 2) {
			Account *account = budget->expensesAccounts.first();
			while(account) {
				categoryCombo->addItem(account->nameWithParent());
				if(account == current_account) curindex = i;
				account = budget->expensesAccounts.next();
				i++;
			}
		} else {
			Account *account = budget->incomesAccounts.first();
			while(account) {
				categoryCombo->addItem(account->nameWithParent());
				if(account == current_account) curindex = i;
				account = budget->incomesAccounts.next();
				i++;
			}
		}
		if(curindex < categoryCombo->count()) categoryCombo->setCurrentIndex(curindex);
		if(curindex <= 1) {
			descriptionCombo->clear();
			descriptionCombo->setEnabled(false);
			descriptionCombo->addItem(tr("All Descriptions Combined", "Referring to the generic description property"));
			if(b_extra) {
				payeeCombo->clear();
				payeeCombo->setEnabled(false);
				if(sourceCombo->currentIndex() == 3) payeeCombo->addItem(tr("All Payees Combined"));
				else payeeCombo->addItem(tr("All Payers Combined"));
			}
			descriptionCombo->setEnabled(false);
			if(b_extra) payeeCombo->setEnabled(false);
		}
		if(curindex == 0) {
			if(sourceCombo->currentIndex() == 3) {
				current_source = 1;
			} else {
				current_source = 2;
			}
		} else if(curindex == 1) {
			if(sourceCombo->currentIndex() == 3) {
				current_source = 3;
			} else {
				current_source = 4;
			}
		}
		categoryCombo->blockSignals(false);
		descriptionCombo->blockSignals(false);
		if(b_extra) payeeCombo->blockSignals(false);
	}
	updateDisplay();
}
#ifdef QT_CHARTS_LIB
class PointLabel : public QGraphicsItem {

	public:

		PointLabel(QGraphicsItem * parent = 0) : QGraphicsItem(parent) {}

		void setText(const QString &text) {
			m_text = text;
			QFontMetrics metrics(m_font);
			m_textRect = metrics.boundingRect(QRect(0, 0, 150, 150), Qt::AlignLeft, m_text);
			m_textRect.translate(5, 5);
			prepareGeometryChange();
			m_rect = m_textRect.adjusted(-5, -5, 5, 5);
		}
		void setAnchor(QPointF point) {
			m_anchor = point;
		}

		QRectF boundingRect() const {
			QPointF anchor = mapFromParent(m_anchor);
			QRectF rect;
			rect.setLeft(qMin(m_rect.left(), anchor.x()));
			rect.setRight(qMax(m_rect.right(), anchor.x()));
			rect.setTop(qMin(m_rect.top(), anchor.y()));
			rect.setBottom(qMax(m_rect.bottom(), anchor.y()));
			return rect;
		}
		void paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*){
			QPainterPath path;
			path.addRoundedRect(m_rect, 5, 5);

			QPointF anchor = mapFromParent(m_anchor);
			if (!m_rect.contains(anchor)) {
				QPointF point1, point2;

				// establish the position of the anchor point in relation to m_rect
				bool above = anchor.y() <= m_rect.top();
				bool aboveCenter = anchor.y() > m_rect.top() && anchor.y() <= m_rect.center().y();
				bool belowCenter = anchor.y() > m_rect.center().y() && anchor.y() <= m_rect.bottom();
				bool below = anchor.y() > m_rect.bottom();

				bool onLeft = anchor.x() <= m_rect.left();
				bool leftOfCenter = anchor.x() > m_rect.left() && anchor.x() <= m_rect.center().x();
				bool rightOfCenter = anchor.x() > m_rect.center().x() && anchor.x() <= m_rect.right();
				bool onRight = anchor.x() > m_rect.right();

				// get the nearest m_rect corner.
				qreal x = (onRight + rightOfCenter) * m_rect.width();
				qreal y = (below + belowCenter) * m_rect.height();
				bool cornerCase = (above && onLeft) || (above && onRight) || (below && onLeft) || (below && onRight);
				bool vertical = qAbs(anchor.x() - x) > qAbs(anchor.y() - y);

				qreal x1 = x + leftOfCenter * 10 - rightOfCenter * 20 + cornerCase * !vertical * (onLeft * 10 - onRight * 20);
				qreal y1 = y + aboveCenter * 10 - belowCenter * 20 + cornerCase * vertical * (above * 10 - below * 20);;
				point1.setX(x1);
				point1.setY(y1);

				qreal x2 = x + leftOfCenter * 20 - rightOfCenter * 10 + cornerCase * !vertical * (onLeft * 20 - onRight * 10);;
				qreal y2 = y + aboveCenter * 20 - belowCenter * 10 + cornerCase * vertical * (above * 20 - below * 10);;
				point2.setX(x2);
				point2.setY(y2);

				path.moveTo(point1);
				path.lineTo(mapFromParent(m_anchor));
				path.lineTo(point2);
				path = path.simplified();
			}
			painter->setBrush(QColor(255, 255, 255));
			painter->drawPath(path);
			painter->drawText(m_textRect, m_text);
		}

	private:
	
		QString m_text;
		QRectF m_textRect;
		QRectF m_rect;
		QPointF m_anchor;
		QFont m_font;

};
void OverTimeChart::onSeriesHovered(bool state, int index, QBarSet *set) {
	if(state) {
		QAbstractBarSeries *series = qobject_cast<QAbstractBarSeries*>(sender());
		int chart_type = typeCombo->currentIndex() + 1;
		PointLabel *item;
		if(!point_label) {
			item = new PointLabel(chart);
			point_label = item;
		} else {
			item = (PointLabel*) point_label;
		}
		QList<QBarSet*> barsets = series->barSets();
		int set_index = barsets.indexOf(set);
		QPointF pos;
		qreal bar_width = 0.0;
		if(chart_type == 2) {
			pos = chart->mapToPosition(QPointF(index, set->at(index)), series);
			QPointF pos_next = chart->mapToPosition(QPointF(index + 1, set->at(index)), series);
			bar_width = (pos_next.x() - pos.x()) * series->barWidth();
			qreal pos_x = pos.x() - (bar_width / 2);
			pos_x += (set_index + 0.5) * (bar_width / barsets.count());
			pos.setX(pos_x);
		} else if(chart_type == 3) {
			pos = chart->mapToPosition(QPointF(set->at(index), index), series);
			QPointF pos_next = chart->mapToPosition(QPointF(set->at(index), index + 1), series);
			bar_width = (pos_next.y() - pos.y()) * series->barWidth();
			qreal pos_y = pos.y() - (bar_width / 2);
			pos_y += (set_index + 0.5) * (bar_width / barsets.count());
			pos.setY(pos_y);
		} else if(chart_type == 4) {
			qreal acc_value = set->at(index) * 0.75;
			for(int i = 0; i < set_index && i < barsets.count(); i++) {
				acc_value += barsets[i]->at(index);
			}
			pos = chart->mapToPosition(QPointF(index, acc_value), series);
			QPointF pos_next = chart->mapToPosition(QPointF(index + 1, acc_value), series);
			bar_width = (pos_next.x() - pos.x()) * series->barWidth();
			pos.setX(pos.x() + (bar_width / 2));
		}
		QDate date = start_date.addMonths(index);
		item->setText(tr("%1\nValue: %2\nDate: %3").arg(set->label()).arg(QLocale().toCurrencyString(set->at(index))).arg(budget->budgetDateToMonth(date).toString(tr("MMMM yyyy", "Month and year"))));
		item->setAnchor(pos);
		item->setPos(pos + QPoint(10, -50));
		item->setZValue(11);
		if(pos.x() + item->boundingRect().width() + 10 > chart->size().width()) {
			if(chart_type == 4) {
				pos.setX(pos.x() - bar_width);
				item->setAnchor(pos);
				item->setPos(pos + QPoint(10, -50));
			}
			item->setPos(pos + QPoint(-10 - item->boundingRect().width(), -50));
		}
		item->show();
	} else if(point_label) {
		point_label->hide();
	}
}
void OverTimeChart::onSeriesHovered(const QPointF &value, bool state) {
	if(state) {
		QXYSeries *series = qobject_cast<QXYSeries*>(sender());
		PointLabel *item;
		if(!point_label) {
			item = new PointLabel(chart);
			point_label = item;
		} else {
			item = (PointLabel*) point_label;
		}
		QDate date = QDateTime::fromMSecsSinceEpoch(value.x()).date();
		if(budget->dayOfBudgetMonth(date) >= budget->daysInBudgetMonth(date) / 2) {
			date = budget->firstBudgetDay(date);
			budget->addBudgetMonthsSetFirst(date, 1);
		} else {
			date = budget->firstBudgetDay(date);
		}
		qreal value_x = QDateTime(date).toMSecsSinceEpoch();
		qreal value_y = value.y();
		for(int i = 0; i < series->count(); i++) {
			if(series->at(i).x() == value_x) {
				value_y = series->at(i).y();
			}
		}
		QPointF pos = chart->mapToPosition(QPointF(value_x, value_y), series);
		item->setText(tr("%1\nValue: %2\nDate: %3").arg(series->name()).arg(QLocale().toCurrencyString(value_y)).arg(budget->budgetDateToMonth(date).toString(tr("MMMM yyyy", "Month and year"))));
		item->setAnchor(pos);
		item->setPos(pos + QPoint(10, -50));
		item->setZValue(11);
		if(pos.x() + item->boundingRect().width() + 10 > chart->size().width()) item->setPos(pos + QPoint(-10 - item->boundingRect().width(), -50));
		item->show();
	} else if(point_label) {
		point_label->hide();
	}
}
void OverTimeChart::legendClicked() {	
	if(typeCombo->currentIndex() == 0) {
		QLegendMarker* marker = qobject_cast<QLegendMarker*>(sender());
		marker->series()->setVisible(!marker->series()->isVisible());
		marker->setVisible(true);

		qreal alpha = 1.0;
		if(!marker->series()->isVisible()) {
			alpha = 0.5;
		}

		QColor color;
		QBrush brush = marker->labelBrush();
		color = brush.color();
		color.setAlphaF(alpha);
		brush.setColor(color);
		marker->setLabelBrush(brush);

		brush = marker->brush();
		color = brush.color();
		if(color.alphaF() > 0.0) {
			color.setAlphaF(alpha);
			brush.setColor(color);
			marker->setBrush(brush);
		}

		QPen pen = marker->pen();
		color = pen.color();
		color.setAlphaF(alpha);
		pen.setColor(color);
		marker->setPen(pen);
	} else {
		QBarLegendMarker* marker = qobject_cast<QBarLegendMarker*>(sender());
		if(marker->series()->count() == 1) updateDisplay();
		else marker->series()->remove(marker->barset());
	}
}
void OverTimeChart::typeChanged(int index) {
	if(index != 0) endMonthChanged(end_date);
	else updateDisplay();
}
void OverTimeChart::themeChanged(int index) {
	QChart::ChartTheme theme = (QChart::ChartTheme) themeCombo->itemData(index).toInt();
	chart->setTheme(theme);
	QSettings settings;
	settings.beginGroup("GeneralOptions");
	settings.setValue("chartTheme", theme);
	settings.endGroup();
	QList<QAbstractSeries*> series_list = chart->series();
	for(int index = 5; index < series_list.count(); index++) {
		QXYSeries *series = (QXYSeries*) series_list[index];
		QPen pen = series->pen();
		if(index >= 10) pen.setStyle(Qt::DashLine);
		else pen.setStyle(Qt::DotLine);
		series->setPen(pen);
		QList<QLegendMarker*> markers = chart->legend()->markers(series);
		if(markers.count() > 0) {
			pen.setWidth(2);
			markers[0]->setPen(pen);
			markers[0]->setBrush(QColor(0, 0, 0, 0));
		}
	}
}
#endif

