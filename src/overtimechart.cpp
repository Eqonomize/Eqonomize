/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016 by Hanna Knutsson                 *
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "overtimechart.h"

#include <QButtonGroup>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
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
extern int months_between_dates(const QDate &date1, const QDate &date2);
extern QString last_picture_directory;

OverTimeChart::OverTimeChart(Budget *budg, QWidget *parent, bool extra_parameters) : QWidget(parent), budget(budg), b_extra(extra_parameters) {

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	QDialogButtonBox *buttons = new QDialogButtonBox(this);
	saveButton = buttons->addButton(tr("Save As…"), QDialogButtonBox::ActionRole);
	printButton = buttons->addButton(tr("Print…"), QDialogButtonBox::ActionRole);
	layout->addWidget(buttons);

	scene = NULL;
	view = new QGraphicsView(this);
	view->setRenderHint(QPainter::Antialiasing, true);
	view->setRenderHint(QPainter::HighQualityAntialiasing, true);
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
	descriptionCombo->addItem(tr("All Descriptions Combined"));
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
	sourceCombo->setCurrentIndex(0);
	sourceChanged(0);
	Transaction *trans = budget->transactions.first();
	start_date = QDate();
	while(trans) {
		if(trans->fromAccount()->type() != ACCOUNT_TYPE_ASSETS || trans->toAccount()->type() != ACCOUNT_TYPE_ASSETS) {
			start_date = trans->date();
			if(start_date.day() > 1) {
				start_date = start_date.addMonths(1);
				start_date.setDate(start_date.year(), start_date.month(), 1);
			}
			break;
		}
		trans = budget->transactions.next();
	}
	if(start_date.isNull() || start_date > QDate::currentDate()) start_date = QDate::currentDate();
	if(start_date.month() == QDate::currentDate().month() && start_date.year() == QDate::currentDate().year()) {
		start_date = start_date.addMonths(-1);
		start_date.setDate(start_date.year(), start_date.month(), 1);
	}
	startDateEdit->setDate(start_date);
	end_date = QDate::currentDate().addDays(-1);
	if(end_date.day() < end_date.daysInMonth()) {
		end_date = end_date.addMonths(-1);
		end_date.setDate(end_date.year(), end_date.month(), end_date.daysInMonth());
	}
	if(end_date <= start_date || (start_date.month() == end_date.month() && start_date.year() == end_date.year())) {
		end_date = QDate::currentDate();
		end_date.setDate(end_date.year(), end_date.month(), end_date.daysInMonth());
	}
	endDateEdit->setDate(end_date);
}

void OverTimeChart::valueTypeToggled(bool b) {
	if(b) updateDisplay();
}

void OverTimeChart::payeeChanged(int index) {
	current_payee = "";
	bool b_income = (current_account && current_account->type() == ACCOUNT_TYPE_INCOMES);
	int d_index = descriptionCombo->currentIndex();
	if(index == 0) {
		if(d_index == 1) current_source = b_income ? 7 : 8;
		else if(d_index == 0) current_source = b_income ? 5 : 6;
		else current_source = b_income ? 9 : 10;
	} else if(index == 1) {
		if(d_index == 1) {
			descriptionCombo->blockSignals(true);
			descriptionCombo->setCurrentIndex(0);
			descriptionCombo->blockSignals(false);
			d_index = 0;
		}
		if(d_index == 0) current_source = b_income ? 11 : 12;
		else current_source = b_income ? 13 : 14;
	} else {
		if(!has_empty_payee || index < payeeCombo->count() - 1) current_payee = payeeCombo->itemText(index);
		if(d_index == 1) current_source = b_income ? 17 : 18;
		else if(d_index == 0) current_source = b_income ? 15 : 16;
		else current_source = b_income ? 19 : 20;
	}
	updateDisplay();
}
void OverTimeChart::descriptionChanged(int index) {
	current_description = "";
	bool b_income = (current_account && current_account->type() == ACCOUNT_TYPE_INCOMES);
	int p_index = 0;
	if(b_extra) p_index = payeeCombo->currentIndex();
	if(index == 0) {
		if(p_index == 1) current_source = b_income ? 11 : 12;
		else if(p_index == 0) current_source = b_income ? 5 : 6;
		else current_source = b_income ? 15 : 16;
	} else if(index == 1) {
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
	descriptionCombo->addItem(tr("All Descriptions Combined"));
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
	} else if(index == 1) {
		if(b_income) {
			current_source = 3;
		} else {
			current_source = 4;
		}
		descriptionCombo->setEnabled(false);
		if(b_extra) payeeCombo->setEnabled(false);
	} else {
		descriptionCombo->addItem(tr("All Descriptions Split"));
		if(d_index == 1) descriptionCombo->setCurrentIndex(1);
		if(b_extra) {
			if(b_income) payeeCombo->addItem(tr("All Payers Split"));
			else payeeCombo->addItem(tr("All Payees Split"));
			if(p_index == 1 && d_index != 1) payeeCombo->setCurrentIndex(1);
		}
		if(!b_income) {
			int i = categoryCombo->currentIndex() - 2;
			if(i < (int) budget->expensesAccounts.count()) {
				current_account = budget->expensesAccounts.at(i);
			}
			if(d_index == 1) current_source = 8;
			else if(p_index == 1) current_source = 12;
			else current_source = 6;
		} else {
			int i = categoryCombo->currentIndex() - 2;
			if(i < (int) budget->incomesAccounts.count()) {
				current_account = budget->incomesAccounts.at(i);
			}
			if(d_index == 1) current_source = 7;
			else if(p_index == 1) current_source = 11;
			else current_source = 5;
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
		for(QMap<QString, bool>::iterator it = descriptions.begin(); it != it_e; ++it) {
			descriptionCombo->addItem(it.key());
		}
		if(has_empty_description) descriptionCombo->addItem(tr("No description"));
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
	descriptionCombo->addItem(tr("All Descriptions Combined"));
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
		categoryCombo->setCurrentIndex(c_index);
		Account *account = budget->incomesAccounts.first();
		while(account) {
			categoryCombo->addItem(account->name());
			account = budget->incomesAccounts.next();
		}
		categoryCombo->setEnabled(true);
		current_source = 3;
	} else if(index == 2) {
		categoryCombo->addItem(tr("All Categories Split"));
		categoryCombo->setCurrentIndex(c_index);
		Account *account = budget->expensesAccounts.first();
		while(account) {
			categoryCombo->addItem(account->name());
			account = budget->expensesAccounts.next();
		}
		categoryCombo->setEnabled(true);
		current_source = 4;
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
	} else {
		categoryCombo->setEnabled(false);
		current_source = 0;
	}
	countButton->setEnabled(index != 1);
	perButton->setEnabled(index != 1);
	categoryCombo->blockSignals(false);
	descriptionCombo->blockSignals(false);
	if(b_extra) payeeCombo->blockSignals(false);
	updateDisplay();
}

void OverTimeChart::startYearChanged(const QDate &date) {	
	bool error = false;
	if(!date.isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		error = true;
	}
	/*if(!error && date > QDate::currentDate()) {
		if(date.year() == QDate::currentDate().year()) {
			start_date = QDate::currentDate();
			start_date.setDate(start_date.year(), start_date.month(), 1);
			startDateEdit->blockSignals(true);
			startDateEdit->setDate(start_date);
			startDateEdit->blockSignals(false);
			updateDisplay();
			return;
		} else {
			QMessageBox::critical(this, tr("Error"), tr("Future dates are not allowed."));
			error = true;
		}
	}*/
	if(!error && date > end_date) {
		end_date = date;
		end_date.setDate(end_date.year(), end_date.month(), end_date.daysInMonth());
		endDateEdit->blockSignals(true);
		endDateEdit->setDate(end_date);
		endDateEdit->blockSignals(false);
	}
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
	/*if(!error && date > QDate::currentDate()) {
		QMessageBox::critical(this, tr("Error"), tr("Future dates are not allowed."));
		error = true;
	}*/
	if(!error && date > end_date) {
		end_date = date;
		end_date.setDate(end_date.year(), end_date.month(), end_date.daysInMonth());
		endDateEdit->blockSignals(true);
		endDateEdit->setDate(end_date);
		endDateEdit->blockSignals(false);
	}
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
	/*if(!error && date > QDate::currentDate()) {
		if(date.year() == QDate::currentDate().year()) {
			end_date = QDate::currentDate();
			end_date.setDate(end_date.year(), end_date.month(), 1);
			endDateEdit->blockSignals(true);
			endDateEdit->setDate(end_date);
			endDateEdit->blockSignals(false);
			updateDisplay();
			return;
		} else {
			QMessageBox::critical(this, tr("Error"), tr("Future dates are not allowed."));
			error = true;
		}
	}*/
	if(!error && date < start_date) {
		start_date = date;
		startDateEdit->blockSignals(true);
		startDateEdit->setDate(start_date);
		startDateEdit->blockSignals(false);
	}
	if(error) {
		endDateEdit->focusYear();
		endDateEdit->blockSignals(true);
		endDateEdit->setDate(end_date);
		endDateEdit->blockSignals(false);
		return;
	}
	end_date = date;
	end_date.setDate(end_date.year(), end_date.month(), end_date.daysInMonth());
	updateDisplay();
}
void OverTimeChart::endMonthChanged(const QDate &date) {	
	bool error = false;
	if(!date.isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		error = true;
	}
	/*if(!error && date > QDate::currentDate()) {
		QMessageBox::critical(this, tr("Error"), tr("Future dates are not allowed."));
		error = true;
	}*/
	if(!error && date < start_date) {
		start_date = date;
		startDateEdit->blockSignals(true);
		startDateEdit->setDate(start_date);
		startDateEdit->blockSignals(false);
	}
	if(error) {
		endDateEdit->focusMonth();
		endDateEdit->blockSignals(true);
		endDateEdit->setDate(end_date);
		endDateEdit->blockSignals(false);
		return;
	}
	end_date = date;
	end_date.setDate(end_date.year(), end_date.month(), end_date.daysInMonth());
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
	rect.setX(0);
	rect.setY(0);
	rect.setRight(rect.right() + 20);
	rect.setBottom(rect.bottom() + 20);
	QPixmap pixmap((int) ceil(rect.width()), (int) ceil(rect.height()));
	QPainter p(&pixmap);
	p.setRenderHint(QPainter::Antialiasing, true);
	scene->render(&p, QRectF(), rect);
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
	if(!scene) return;
	QPrinter pr;
	QPrintDialog dialog(&pr, this);
	if(dialog.exec() == QDialog::Accepted) {
		QPainter p(&pr);
		p.setRenderHint(QPainter::Antialiasing, true);
		QRectF rect = scene->sceneRect();
		rect.setX(0);
		rect.setY(0);
		rect.setRight(rect.right() + 20);
		rect.setBottom(rect.bottom() + 20);
		scene->render(&p, QRectF(), rect);
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

	int type = valueGroup->checkedId();

	if(current_source == 3 || current_source < 2) {
		Account *account = budget->incomesAccounts.first();
		while(account) {
			monthly_cats[account] = QVector<chart_month_info>();
			mi_c[account] = NULL;
			isfirst_c[account] = true;
			scheduled_cats[account] = 0.0;
			scheduled_cat_counts[account] = 0.0;
			account = budget->incomesAccounts.next();
		}
	}

	if(current_source == 4 || current_source < 3) {
		Account *account = budget->expensesAccounts.first();
		while(account) {
			monthly_cats[account] = QVector<chart_month_info>();
			mi_c[account] = NULL;
			isfirst_c[account] = true;
			scheduled_cats[account] = 0.0;
			scheduled_cat_counts[account] = 0.0;
			account = budget->expensesAccounts.next();
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

	QDate first_date = start_date;
	QDate last_date = end_date;

	double maxvalue = 1.0;
	double minvalue = 0.0;
	double maxcount = 1.0;
	bool started = false;
	Transaction *trans = budget->transactions.first();
	while(trans && trans->date() <= last_date) {
		bool include = false;
		int sign = 1;
		total_value = NULL;
		if(!started && trans->date() >= first_date) started = true;
		if(started) {
			switch(current_source) {
				case -1: {}
				case 0: {}
				case 1: {}
				case 2: {}
				case 3: {}
				case 4: {
					if(current_source != 2 && current_source != 4 && trans->fromAccount()->type() == ACCOUNT_TYPE_INCOMES) {
						monthly_values = &monthly_cats[trans->fromAccount()]; mi = &mi_c[trans->fromAccount()]; isfirst = &isfirst_c[trans->fromAccount()];
						sign = 1;
						include = true;
					} else if(current_source != 1 && current_source != 3 && trans->fromAccount()->type() == ACCOUNT_TYPE_EXPENSES) {
						monthly_values = &monthly_cats[trans->fromAccount()]; mi = &mi_c[trans->fromAccount()]; isfirst = &isfirst_c[trans->fromAccount()];
						sign = -1;
						include = true;
					} else if(current_source != 2 && current_source != 4 && trans->toAccount()->type() == ACCOUNT_TYPE_INCOMES) {
						monthly_values = &monthly_cats[trans->toAccount()]; mi = &mi_c[trans->toAccount()]; isfirst = &isfirst_c[trans->toAccount()];
						sign = -1;
						include = true;
					} else if(current_source != 1 && current_source != 3 && trans->toAccount()->type() == ACCOUNT_TYPE_EXPENSES) {
						monthly_values = &monthly_cats[trans->toAccount()]; mi = &mi_c[trans->toAccount()]; isfirst = &isfirst_c[trans->toAccount()];
						sign = 1;
						include = true;
					}
					break;
				}
				case 5: {
					if(trans->fromAccount() == current_account) {
						monthly_values = &monthly_incomes; mi = &mi_i; isfirst = &isfirst_i;
						sign = 1;
						include = true;
					} else if(trans->toAccount() == current_account) {
						monthly_values = &monthly_incomes; mi = &mi_i; isfirst = &isfirst_i;
						sign = -1;
						include = true;
					}
					break;
				}
				case 6: {
					if(trans->fromAccount() == current_account) {
						monthly_values = &monthly_expenses; mi = &mi_e; isfirst = &isfirst_e;
						sign = -1;
						include = true;
					} else if(trans->toAccount() == current_account) {
						monthly_values = &monthly_expenses; mi = &mi_e; isfirst = &isfirst_e;
						sign = 1;
						include = true;
					}
					break;
				}
				case 7: {
					if(trans->fromAccount() == current_account) {
						monthly_values = &monthly_desc[trans->description()]; mi = &mi_d[trans->description()]; isfirst = &isfirst_d[trans->description()];
						total_value = &desc_values[trans->description()];
						sign = 1;
						include = true;
					} else if(trans->toAccount() == current_account) {
						monthly_values = &monthly_desc[trans->description()]; mi = &mi_d[trans->description()]; isfirst = &isfirst_d[trans->description()];
						total_value = &desc_values[trans->description()];
						sign = -1;
						include = true;
					}
					break;
				}
				case 8: {
					if(trans->fromAccount() == current_account) {
						monthly_values = &monthly_desc[trans->description()]; mi = &mi_d[trans->description()]; isfirst = &isfirst_d[trans->description()];
						total_value = &desc_values[trans->description()];
						sign = -1;
						include = true;
					} else if(trans->toAccount() == current_account) {
						monthly_values = &monthly_desc[trans->description()]; mi = &mi_d[trans->description()]; isfirst = &isfirst_d[trans->description()];
						total_value = &desc_values[trans->description()];
						sign = 1;
						include = true;
					}
					break;
				}
				case 9: {
					if(trans->fromAccount() == current_account && trans->description() == current_description) {
						monthly_values = &monthly_incomes; mi = &mi_i; isfirst = &isfirst_i;
						sign = 1;
						include = true;
					} else if(trans->toAccount() == current_account && trans->description() == current_description) {
						monthly_values = &monthly_incomes; mi = &mi_i; isfirst = &isfirst_i;
						sign = -1;
						include = true;
					}
					break;
				}
				case 10: {
					if(trans->fromAccount() == current_account && trans->description() == current_description) {
						monthly_values = &monthly_expenses; mi = &mi_e; isfirst = &isfirst_e;
						sign = -1;
						include = true;
					} else if(trans->toAccount() == current_account && trans->description() == current_description) {
						monthly_values = &monthly_expenses; mi = &mi_e; isfirst = &isfirst_e;
						sign = 1;
						include = true;
					}
					break;
				}
				case 11: {
					if(trans->type() != TRANSACTION_TYPE_INCOME) break;
					Income *income = (Income*) trans;
					if(income->category() == current_account) {
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
					if(expense->category() == current_account) {
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
					if(income->category() == current_account && income->description() == current_description) {
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
					if(expense->category() == current_account && expense->description() == current_description) {
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
					if(income->category() == current_account && income->payer() == current_payee) {
						monthly_values = &monthly_incomes; mi = &mi_i; isfirst = &isfirst_i;
						sign = 1;
						include = true;
					}
					break;
				}
				case 16: {
					if(trans->type() != TRANSACTION_TYPE_EXPENSE) break;
					Expense *expense = (Expense*) trans;
					if(expense->category() == current_account && expense->payee() == current_payee) {
						monthly_values = &monthly_expenses; mi = &mi_e; isfirst = &isfirst_e;
						sign = 1;
						include = true;
					}
					break;
				}
				case 17: {
					if(trans->type() != TRANSACTION_TYPE_INCOME) break;
					Income *income = (Income*) trans;
					if(income->category() == current_account && income->payer() == current_payee) {
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
					if(expense->category() == current_account && expense->payee() == current_payee) {
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
					if(income->category() == current_account && income->description() == current_description && income->payer() == current_payee) {
						monthly_values = &monthly_incomes; mi = &mi_i; isfirst = &isfirst_i;
						sign = 1;
						include = true;
					}
					break;
				}
				case 20: {
					if(trans->type() != TRANSACTION_TYPE_EXPENSE) break;
					Expense *expense = (Expense*) trans;
					if(expense->category() == current_account && expense->description() == current_description && expense->payee() == current_payee) {
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
				newdate.setDate(trans->date().year(), trans->date().month(), trans->date().daysInMonth());
				if(*mi) {
					olddate = (*mi)->date.addMonths(1);
					olddate.setDate(olddate.year(), olddate.month(), olddate.daysInMonth());
					(*isfirst) = false;
				} else {
					olddate.setDate(first_date.year(), first_date.month(), first_date.daysInMonth());
					(*isfirst) = true;
				}
				while(olddate < newdate) {
					monthly_values->append(chart_month_info());
					(*mi) = &monthly_values->back();
					(*mi)->value = 0.0;
					(*mi)->count = 0.0;
					(*mi)->date = olddate;
					olddate = olddate.addMonths(1);
					olddate.setDate(olddate.year(), olddate.month(), olddate.daysInMonth());
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
		case -1: {source_org = 0; break;}
		case 0: {source_org = 0; break;}
		case 1: {source_org = 3; break;}
		case 2: {source_org = 4; break;}
		case 3: {source_org = 3; break;}
		case 4: {source_org = 4; break;}
		case 5: {source_org = 1; break;}
		case 6: {source_org = 2; break;}
		case 7: {source_org = 7; break;}
		case 8: {source_org = 7; break;}
		case 9: {source_org = 1; break;}
		case 10: {source_org = 2; break;}
		case 11: {source_org = 11; break;}
		case 12: {source_org = 11; break;}
		case 13: {source_org = 11; break;}
		case 14: {source_org = 11; break;}
		case 15: {source_org = 1; break;}
		case 16: {source_org = 2; break;}
		case 17: {source_org = 7; break;}
		case 18: {source_org = 7; break;}
		case 19: {source_org = 1; break;}
		case 20: {source_org = 2; break;}
	}

	Account *account = NULL;
	int desc_i = 2;
	int desc_nr = 0;
	bool at_expenses = false;
	if(source_org == 7) desc_nr = descriptionCombo->count();
	else if(source_org == 11) desc_nr = payeeCombo->count();
	else if(source_org == 0) {account = budget->incomesAccounts.first(); if(!account) {at_expenses = true; account = budget->expensesAccounts.first();}}
	else if(source_org == 3) account = budget->incomesAccounts.first();
	else if(source_org == 4) account = budget->expensesAccounts.first();
	else if(source_org == 2) {mi = &mi_e; monthly_values = &monthly_expenses; isfirst = &isfirst_e;}
	else if(source_org == 1) {mi = &mi_i; monthly_values = &monthly_incomes; isfirst = &isfirst_i;}		
	while(source_org == 1 || source_org == 2 || account || ((source_org == 7 || source_org == 11) && desc_i < desc_nr)) {
		if(source_org == 3 || source_org == 4 || source_org == 0) {mi = &mi_c[account]; monthly_values = &monthly_cats[account]; isfirst = &isfirst_c[account];}
		else if(source_org == 7) {mi = &mi_d[descriptionCombo->itemText(desc_i)]; monthly_values = &monthly_desc[descriptionCombo->itemText(desc_i)]; isfirst = &isfirst_d[descriptionCombo->itemText(desc_i)];}
		else if(source_org == 11) {mi = &mi_d[payeeCombo->itemText(desc_i)]; monthly_values = &monthly_desc[payeeCombo->itemText(desc_i)]; isfirst = &isfirst_d[payeeCombo->itemText(desc_i)];}
		if(!(*mi)) {
			monthly_values->append(chart_month_info());
			(*mi) = &monthly_values->back();
			(*mi)->value = 0.0;
			(*mi)->count = 0.0;
			(*mi)->date.setDate(first_date.year(), first_date.month(), first_date.daysInMonth());
			(*isfirst) = true;
		}
		while((*mi)->date < last_date) {
			QDate newdate = (*mi)->date.addMonths(1);
			newdate.setDate(newdate.year(), newdate.month(), newdate.daysInMonth());
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
		else if(source_org == 0) {
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
					monthly_values = &monthly_cats[trans->fromAccount()]; mi = &mi_c[trans->fromAccount()]; isfirst = &isfirst_c[trans->fromAccount()];
					sign = 1;
					include = true;
				} else if(current_source != 1 && current_source != 3 && trans->fromAccount()->type() == ACCOUNT_TYPE_EXPENSES) {
					monthly_values = &monthly_cats[trans->fromAccount()]; mi = &mi_c[trans->fromAccount()]; isfirst = &isfirst_c[trans->fromAccount()];
					sign = -1;
					include = true;
				} else if(current_source != 2 && current_source != 4 && trans->toAccount()->type() == ACCOUNT_TYPE_INCOMES) {
					monthly_values = &monthly_cats[trans->toAccount()]; mi = &mi_c[trans->toAccount()]; isfirst = &isfirst_c[trans->toAccount()];
					sign = -1;
					include = true;
				} else if(current_source != 1 && current_source != 3 && trans->toAccount()->type() == ACCOUNT_TYPE_EXPENSES) {
					monthly_values = &monthly_cats[trans->toAccount()]; mi = &mi_c[trans->toAccount()]; isfirst = &isfirst_c[trans->toAccount()];
					sign = 1;
					include = true;
				}
				break;
			}
			case 5: {
				if(trans->fromAccount() == current_account) {
					monthly_values = &monthly_incomes; mi = &mi_i; isfirst = &isfirst_i;
					sign = 1;
					include = true;
				} else if(trans->toAccount() == current_account) {
					monthly_values = &monthly_incomes; mi = &mi_i; isfirst = &isfirst_i;
					sign = -1;
					include = true;
				}
				break;
			}
			case 6: {
				if(trans->fromAccount() == current_account) {
					monthly_values = &monthly_expenses; mi = &mi_e; isfirst = &isfirst_e;
					sign = -1;
					include = true;
				} else if(trans->toAccount() == current_account) {
					monthly_values = &monthly_expenses; mi = &mi_e; isfirst = &isfirst_e;
					sign = 1;
					include = true;
				}
				break;
			}
			case 7: {
				if(trans->fromAccount() == current_account) {
					monthly_values = &monthly_desc[trans->description()]; mi = &mi_d[trans->description()]; isfirst = &isfirst_d[trans->description()];
					total_value = &desc_values[trans->description()];
					sign = 1;
					include = true;
				} else if(trans->toAccount() == current_account) {
					monthly_values = &monthly_desc[trans->description()]; mi = &mi_d[trans->description()]; isfirst = &isfirst_d[trans->description()];
					total_value = &desc_values[trans->description()];
					sign = -1;
					include = true;
				}
				break;
			}
			case 8: {
				if(trans->fromAccount() == current_account) {
					monthly_values = &monthly_desc[trans->description()]; mi = &mi_d[trans->description()]; isfirst = &isfirst_d[trans->description()];
					total_value = &desc_values[trans->description()];
					sign = -1;
					include = true;
				} else if(trans->toAccount() == current_account) {
					monthly_values = &monthly_desc[trans->description()]; mi = &mi_d[trans->description()]; isfirst = &isfirst_d[trans->description()];
					total_value = &desc_values[trans->description()];
					sign = 1;
					include = true;
				}
				break;
			}
			case 9: {
				if(trans->fromAccount() == current_account && trans->description() == current_description) {
					monthly_values = &monthly_incomes; mi = &mi_i; isfirst = &isfirst_i;
					sign = 1;
					include = true;
				} else if(trans->toAccount() == current_account && trans->description() == current_description) {
					monthly_values = &monthly_incomes; mi = &mi_i; isfirst = &isfirst_i;
					sign = -1;
					include = true;
				}
				break;
			}
			case 10: {
				if(trans->fromAccount() == current_account && trans->description() == current_description) {
					monthly_values = &monthly_expenses; mi = &mi_e; isfirst = &isfirst_e;
					sign = -1;
					include = true;
				} else if(trans->toAccount() == current_account && trans->description() == current_description) {
					monthly_values = &monthly_expenses; mi = &mi_e; isfirst = &isfirst_e;
					sign = 1;
					include = true;
				}
				break;
			}
			case 11: {
				if(trans->type() != TRANSACTION_TYPE_INCOME) break;
				Income *income = (Income*) trans;
				if(income->category() == current_account) {
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
				if(expense->category() == current_account) {
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
				if(income->category() == current_account && income->description() == current_description) {
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
				if(expense->category() == current_account && expense->description() == current_description) {
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
				if(income->category() == current_account && income->payer() == current_payee) {
					monthly_values = &monthly_incomes; mi = &mi_i; isfirst = &isfirst_i;
					sign = 1;
					include = true;
				}
				break;
			}
			case 16: {
				if(trans->type() != TRANSACTION_TYPE_EXPENSE) break;
				Expense *expense = (Expense*) trans;
				if(expense->category() == current_account && expense->payee() == current_payee) {
					monthly_values = &monthly_expenses; mi = &mi_e; isfirst = &isfirst_e;
					sign = 1;
					include = true;
				}
				break;
			}
			case 17: {
				if(trans->type() != TRANSACTION_TYPE_INCOME) break;
				Income *income = (Income*) trans;
				if(income->category() == current_account && income->payer() == current_payee) {
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
				if(expense->category() == current_account && expense->payee() == current_payee) {
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
				if(income->category() == current_account && income->description() == current_description && income->payer() == current_payee) {
					monthly_values = &monthly_incomes; mi = &mi_i; isfirst = &isfirst_i;
					sign = 1;
					include = true;
				}
				break;
			}
			case 20: {
				if(trans->type() != TRANSACTION_TYPE_EXPENSE) break;
				Expense *expense = (Expense*) trans;
				if(expense->category() == current_account && expense->description() == current_description && expense->payee() == current_payee) {
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
	desc_i = 2;
	desc_nr = 0;
	int imonth = QDate::currentDate().month();

	if(QDate::currentDate().day() == QDate::currentDate().daysInMonth()) {
		imonth++;
	}

	at_expenses = false;
	if(source_org == 7) {desc_nr = descriptionCombo->count(); account = NULL;}
	else if(source_org == 11) {desc_nr = payeeCombo->count(); account = NULL;}
	else if(source_org == 0) {account = budget->incomesAccounts.first(); if(!account) {at_expenses = true; account = budget->expensesAccounts.first();}}
	else if(source_org == 3) account = budget->incomesAccounts.first();
	else if(source_org == 4) account = budget->expensesAccounts.first();
	else if(source_org == 2) {mi = &mi_e; monthly_values = &monthly_expenses; isfirst = &isfirst_e;}
	else if(source_org == 1) {mi = &mi_i; monthly_values = &monthly_incomes; isfirst = &isfirst_i;}
	while(source_org == 1 || source_org == 2 || account || ((source_org == 7 || source_org == 11) && desc_i < desc_nr)) {
		if(source_org == 3 || source_org == 4 || source_org == 0) {mi = &mi_c[account]; monthly_values = &monthly_cats[account]; isfirst = &isfirst_c[account];}
		else if(source_org == 7) {mi = &mi_d[descriptionCombo->itemText(desc_i)]; monthly_values = &monthly_desc[descriptionCombo->itemText(desc_i)]; isfirst = &isfirst_d[descriptionCombo->itemText(desc_i)];}
		else if(source_org == 11) {mi = &mi_d[payeeCombo->itemText(desc_i)]; monthly_values = &monthly_desc[payeeCombo->itemText(desc_i)]; isfirst = &isfirst_d[payeeCombo->itemText(desc_i)];}
		(*mi) = &monthly_values->front();
		QVector<chart_month_info>::iterator cmi_it = monthly_values->begin();
		bool in_future = false;
		while(cmi_it != monthly_values->end()) {
			(*mi) = cmi_it;
			if(account && type < 2 && current_source < 7) {
				if(!in_future && (*mi)->date.month() >= imonth) {
					in_future = true;
				}
				if(in_future && account->type() != ACCOUNT_TYPE_ASSETS) {
					double d_budget = ((CategoryAccount*) account)->monthlyBudget((*mi)->date.year(), (*mi)->date.month(), false);
					if(d_budget >= 0.0 && d_budget > (*mi)->value) {
						(*mi)->value = d_budget;
					}
				}
			}
			switch(type) {
				case 1: {
					if((*mi)->date >= last_date) {
						(*mi)->value /= ((*isfirst) ? (first_date.daysTo(last_date) + 1) : last_date.day());
					} else {
						(*mi)->value /= ((*isfirst) ? (first_date.daysTo((*mi)->date) + 1) : (*mi)->date.daysInMonth());
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
		else if(source_org == 0) {
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
			(*mi)->date.setDate(first_date.year(), first_date.month(), first_date.daysInMonth());
			while((*mi)->date < last_date) {
				QDate newdate = (*mi)->date.addMonths(1);
				newdate.setDate(newdate.year(), newdate.month(), newdate.daysInMonth());
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
	
	if(source_org == 7 || source_org == 11) {
		QString r_desc_str;
		if(current_source == 12 || current_source == 14) {
			r_desc_str = tr("Other payees");
		} else if(current_source == 11 || current_source == 13) {
			r_desc_str = tr("Other payers");
		} else {
			r_desc_str = tr("Other descriptions");
		}
		monthly_desc[r_desc_str] = QVector<chart_month_info>();
		monthly_values = &monthly_desc[r_desc_str];
		monthly_values->append(chart_month_info());
		(*mi) = &monthly_values->back();
		(*mi)->value = 0.0;
		(*mi)->count = 0.0;
		(*mi)->date.setDate(first_date.year(), first_date.month(), first_date.daysInMonth());
		while((*mi)->date < last_date) {
			QDate newdate = (*mi)->date.addMonths(1);
			newdate.setDate(newdate.year(), newdate.month(), newdate.daysInMonth());
			monthly_values->append(chart_month_info());
			(*mi) = &monthly_values->back();
			(*mi)->value = 0.0;
			(*mi)->count = 0.0;
			(*mi)->date = newdate;
		}
		desc_i = 2;
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
							if(i < 6) desc_order.push_back(desc_str);
							else b_added = false;
						} else {
							desc_order.insert(i + 1, desc_str);
						}
						break;
					}
					i--;
				}
				if(i < 0) desc_order.push_front(desc_str);
				if(desc_order.size() > 6) {
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
			desc_i = 2;
			while(desc_i < desc_nr && desc_i - 2 < 6) {
				if(source_org == 7) desc_str = descriptionCombo->itemText(desc_i);
				else desc_str = payeeCombo->itemText(desc_i);
				desc_order.push_back(desc_str);
				desc_i++;
			}
		}
		if(desc_nr > desc_order.size()) {
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
	

	QGraphicsScene *oldscene = scene;
	scene = new QGraphicsScene(this);
	scene->setBackgroundBrush(Qt::white);
	QFont legend_font = font();
	QFontMetrics fm(legend_font);
	int fh = fm.height();

	int years = last_date.year() - first_date.year();
	int months = months_between_dates(first_date, last_date) + 1;

	int y_lines = 5;
	if(type == 2) {
		int intmax = (int) ceil(maxcount);
		if(intmax % 5 > 0) maxvalue = ((intmax / 5) + 1) * 5;
		else maxvalue = intmax;
	} else {
		if(-minvalue > maxvalue) maxvalue = -minvalue;
		double maxvalue_exp = round(log10(maxvalue)) - 1.0;
		if(maxvalue_exp <= 0.0) {
			maxvalue /= 2.0;
			maxvalue = ceil(maxvalue) * 2.0;
		} else {
			maxvalue /= pow(10, maxvalue_exp);
			maxvalue = ceil(maxvalue) * pow(10, maxvalue_exp);
		}
		if(minvalue < 0.0) {
			if(current_source == -1) {
				minvalue = -maxvalue;
				y_lines = 4;
			} else {
				if(maxvalue / -minvalue >= 4.0) {
					minvalue = -maxvalue / 4;
					y_lines = 5;
				} else if(maxvalue / -minvalue > 2.0) {
					minvalue = -maxvalue / 2;
					y_lines = 6;
				} else {
					minvalue = -maxvalue;
					y_lines = 4;
				}
			}
		}
	}
	int n = 0;
	QDate monthdate = first_date;
	QDate curmonth = end_date;
	while(monthdate <= curmonth) {
		monthdate = monthdate.addMonths(1);
		n++;
	}

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
	if(type == 2) axis_text->setText(tr("Quantity"));
	else if(current_source == 0) axis_text->setText(tr("Value (%1)").arg(QLocale().currencySymbol()));
	else if(current_source == -1) axis_text->setText(tr("Profit (%1)").arg(QLocale().currencySymbol()));
	else if(current_source % 2 == 1) axis_text->setText(tr("Income (%1)").arg(QLocale().currencySymbol()));
	else axis_text->setText(tr("Cost (%1)").arg(QLocale().currencySymbol()));
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
				if(fm.width(QDate::longMonthName(monthdate.month(), QDate::StandaloneFormat)) > linelength - 8) {
					b_long_month_names = false;
				}
			}
			if(!b_long_month_names) {
				if(fm.width(QDate::shortMonthName(monthdate.month(), QDate::StandaloneFormat)) > linelength) {
					b_month_names = false;
					break;
				}
			}
			monthdate = monthdate.addMonths(1);
		}
	}
	monthdate = first_date;
	while(monthdate <= curmonth) {
		if(years < 5 || year != monthdate.year()) {
			QGraphicsLineItem *x_mark = new QGraphicsLineItem();
			x_mark->setLine(margin + axis_width + index * linelength, chart_height + chart_y, margin + axis_width + index * linelength, chart_height + chart_y + 10);
			x_mark->setPen(axis_pen);
			scene->addItem(x_mark);
		}
		if(b_month_names) {
			QGraphicsSimpleTextItem *axis_text = new QGraphicsSimpleTextItem(b_long_month_names ? QDate::longMonthName(monthdate.month(), QDate::StandaloneFormat) : QDate::shortMonthName(monthdate.month(), QDate::StandaloneFormat));
			axis_text->setFont(legend_font);
			axis_text->setBrush(Qt::black);
			axis_text->setPos(margin + axis_width + index * linelength + (linelength - axis_text->boundingRect().width()) / 2, chart_height + chart_y + 11);
			scene->addItem(axis_text);
		} else if(year != monthdate.year()) {
			year = monthdate.year();
			QGraphicsSimpleTextItem *axis_text = new QGraphicsSimpleTextItem(QLocale().toString(monthdate.year()));
			axis_text->setFont(legend_font);
			axis_text->setBrush(Qt::black);
			axis_text->setPos(margin + axis_width + index * linelength, chart_height + chart_y + 11);
			scene->addItem(axis_text);
		}
		monthdate = monthdate.addMonths(1);
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
	else if(source_org == 2) {monthly_values = &monthly_expenses;}
	else if(source_org == 1 || source_org == 0) {monthly_values = &monthly_incomes;}
	while(source_org < 3 || account || ((source_org == 7 || source_org == 11) && desc_i < desc_nr)) {
		if(source_org == 3 || source_org == 4) {monthly_values = &monthly_cats[account];}
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
			case 4: {legend_text->setText(account->name()); break;}
			case 5: {}
			case 6: {legend_text->setText(current_account->name()); break;}
			case 17: {}
			case 18: {}
			case 7: {}
			case 8: {
				if(desc_order[desc_i].isEmpty()) legend_text->setText(tr("No description"));
				else legend_text->setText(desc_order[desc_i]);
				break;
			}
			case 9: {}
			case 10: {
				if(current_description.isEmpty()) legend_text->setText(tr("No description"));
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
				if(current_payee.isEmpty() && current_description.isEmpty()) {str1 = tr("No description"); str2 = tr("no payer");}
				else if(current_payee.isEmpty()) {str1 = current_description; str2 = tr("no payer");}
				else if(current_description.isEmpty()) {str1 = tr("No description"); str2 = current_payee;}
				else {str1 = current_description; str2 = current_payee;}
				legend_text->setText(tr("%1/%2", "%1: Description; %2: Payer").arg(str1).arg(str2));
				break;
			}
			case 20: {
				QString str1, str2;
				if(current_payee.isEmpty() && current_description.isEmpty()) {str1 = tr("No description"); str2 = tr("no payee");}
				else if(current_payee.isEmpty()) {str1 = current_description; str2 = tr("no payee");}
				else if(current_description.isEmpty()) {str1 = tr("No description"); str2 = current_payee;}
				else {str1 = current_description; str2 = current_payee;}
				legend_text->setText(tr("%1/%2", "%1: Description; %2: Payee").arg(str1).arg(str2));
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
		else if(source_org == 0 && monthly_values != &monthly_expenses) {monthly_values = &monthly_expenses;}
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
	if(current_source == 7 || current_source == 8 || current_source == 17 || current_source == 18) {
		if(has_empty_description) descriptionCombo->setItemText(descriptionCombo->count() - 1, tr("No description"));
	} else if(current_source == 12 || current_source == 14) {
		if(has_empty_payee) payeeCombo->setItemText(payeeCombo->count() - 1, tr("No payee"));
	} else if(current_source == 11 || current_source == 13) {
		if(has_empty_payee) payeeCombo->setItemText(payeeCombo->count() - 1, tr("No payer"));
	}
}

void OverTimeChart::resizeEvent(QResizeEvent *e) {
	QWidget::resizeEvent(e);
	if(scene) {
		view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
	}
}

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
		descriptionCombo->addItem(tr("All Descriptions Combined"));
		descriptionCombo->addItem(tr("All Descriptions Split"));
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
			descriptionCombo->addItem(tr("No description"));
		}
		if(b_extra) {
			i = 2;
			QMap<QString, bool>::iterator it2_e = payees.end();
			for(QMap<QString, bool>::iterator it2 = payees.begin(); it2 != it2_e; ++it2) {
				if((current_source >= 15 || current_source <= 20) && it2.key() == current_payee) {
					curindex_p = i;
				}
				payeeCombo->addItem(it2.key());
				i++;
			}
			if(has_empty_payee) {
				if((current_source >= 15 || current_source <= 20) && current_payee.isEmpty()) {
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
				categoryCombo->addItem(account->name());
				if(account == current_account) curindex = i;
				account = budget->expensesAccounts.next();
				i++;
			}
		} else {
			Account *account = budget->incomesAccounts.first();
			while(account) {
				categoryCombo->addItem(account->name());
				if(account == current_account) curindex = i;
				account = budget->incomesAccounts.next();
				i++;
			}
		}
		if(curindex < categoryCombo->count()) categoryCombo->setCurrentIndex(curindex);
		if(curindex <= 1) {
			descriptionCombo->clear();
			descriptionCombo->setEnabled(false);
			descriptionCombo->addItem(tr("All Descriptions Combined"));
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

