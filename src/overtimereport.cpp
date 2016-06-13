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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "overtimereport.h"


#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QObject>
#include <QString>
#include <QPushButton>
#include <QVBoxLayout>
#include <qvector.h>
#include <QTextStream>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QUrl>
#include <QFileDialog>
#include <QSaveFile>
#include <QApplication>
#include <QTemporaryFile>
#include <QMimeDatabase>
#include <QMimeType>

#include <KConfigGroup>
#include <KSharedConfig>
#include <khtml_part.h>
#include <khtmlview.h>
#include <kconfig.h>
#include <kmessagebox.h>
#include <kstdguiitem.h>
#include <klocalizedstring.h>
#include <kio/filecopyjob.h>
#include <kjobwidgets.h>

#include "account.h"
#include "budget.h"
#include "recurrence.h"
#include "transaction.h"

#include <cmath>

extern QString htmlize_string(QString str);
extern double averageMonth(const QDate &date1, const QDate &date2);
extern double averageYear(const QDate &date1, const QDate &date2);

struct month_info {
	double value;
	double count;
	QDate date;
};

OverTimeReport::OverTimeReport(Budget *budg, QWidget *parent) : QWidget(parent), budget(budg) {

	setAttribute(Qt::WA_DeleteOnClose, true);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	QDialogButtonBox *buttons = new QDialogButtonBox(this);
	saveButton = buttons->addButton(QString(), QDialogButtonBox::ActionRole);
	KGuiItem::assign(saveButton, KStandardGuiItem::saveAs());
	printButton = buttons->addButton(QString(), QDialogButtonBox::ActionRole);
	KGuiItem::assign(printButton, KStandardGuiItem::print());
	layout->addWidget(buttons);

	htmlpart = new KHTMLPart(this);
	layout->addWidget(htmlpart->view());

	KConfigGroup config = KSharedConfig::openConfig()->group("Over Time Report");

	QGroupBox *settingsWidget = new QGroupBox(i18n("Options"), this);
	QGridLayout *settingsLayout = new QGridLayout(settingsWidget);


	settingsLayout->addWidget(new QLabel(i18n("Source:"), settingsWidget), 0, 0);
	QHBoxLayout *choicesLayout = new QHBoxLayout();
	settingsLayout->addLayout(choicesLayout, 0, 1);
	sourceCombo = new QComboBox(settingsWidget);
	sourceCombo->setEditable(false);
	sourceCombo->addItem(i18n("Profits"));
	sourceCombo->addItem(i18n("Expenses"));
	sourceCombo->addItem(i18n("Incomes"));
	choicesLayout->addWidget(sourceCombo);
	categoryCombo = new QComboBox(settingsWidget);
	categoryCombo->setEditable(false);
	categoryCombo->addItem(i18n("All Categories Combined"));
	categoryCombo->setEnabled(false);
	choicesLayout->addWidget(categoryCombo);
	descriptionCombo = new QComboBox(settingsWidget);
	descriptionCombo->setEditable(false);
	descriptionCombo->addItem(i18n("All Descriptions Combined"));
	descriptionCombo->setEnabled(false);
	choicesLayout->addWidget(descriptionCombo);

	current_account = NULL;
	current_source = 0;

	settingsLayout->addWidget(new QLabel(i18n("Columns:"), settingsWidget), 1, 0);
	QHBoxLayout *enabledLayout = new QHBoxLayout();
	settingsLayout->addLayout(enabledLayout, 1, 1);
	valueButton = new QCheckBox(i18n("Value"), settingsWidget);
	valueButton->setChecked(config.readEntry("valueEnabled", true));
	enabledLayout->addWidget(valueButton);
	dailyButton = new QCheckBox(i18n("Daily"), settingsWidget);
	dailyButton->setChecked(config.readEntry("dailyAverageEnabled", true));
	enabledLayout->addWidget(dailyButton);
	monthlyButton = new QCheckBox(i18n("Monthly"), settingsWidget);
	monthlyButton->setChecked(config.readEntry("monthlyAverageEnabled", true));
	enabledLayout->addWidget(monthlyButton);
	yearlyButton = new QCheckBox(i18n("Yearly"), settingsWidget);
	yearlyButton->setChecked(config.readEntry("yearlyEnabled", false));
	enabledLayout->addWidget(yearlyButton);
	countButton = new QCheckBox(i18n("Quantity"), settingsWidget);
	countButton->setChecked(config.readEntry("transactionCountEnabled", true));
	enabledLayout->addWidget(countButton);
	perButton = new QCheckBox(i18n("Average value"), settingsWidget);
	perButton->setChecked(config.readEntry("valuePerTransactionEnabled", false));
	enabledLayout->addWidget(perButton);
	enabledLayout->addStretch(1);

	layout->addWidget(settingsWidget);

	connect(valueButton, SIGNAL(toggled(bool)), this, SLOT(updateDisplay()));
	connect(dailyButton, SIGNAL(toggled(bool)), this, SLOT(updateDisplay()));
	connect(monthlyButton, SIGNAL(toggled(bool)), this, SLOT(updateDisplay()));
	connect(yearlyButton, SIGNAL(toggled(bool)), this, SLOT(updateDisplay()));
	connect(countButton, SIGNAL(toggled(bool)), this, SLOT(updateDisplay()));
	connect(perButton, SIGNAL(toggled(bool)), this, SLOT(updateDisplay()));
	connect(sourceCombo, SIGNAL(activated(int)), this, SLOT(sourceChanged(int)));
	connect(categoryCombo, SIGNAL(activated(int)), this, SLOT(categoryChanged(int)));
	connect(descriptionCombo, SIGNAL(activated(int)), this, SLOT(descriptionChanged(int)));
	connect(sourceCombo, SIGNAL(activated(int)), this, SLOT(updateDisplay()));
	connect(saveButton, SIGNAL(clicked()), this, SLOT(save()));
	connect(printButton, SIGNAL(clicked()), this, SLOT(print()));

}

void OverTimeReport::descriptionChanged(int index) {
	current_description = "";
	bool b_income = (current_account && current_account->type() == ACCOUNT_TYPE_INCOMES);
	if(index == 0) {
		if(b_income) current_source = 5;
		else current_source = 6;
	} else {
		if(!has_empty_description || index < descriptionCombo->count() - 1) current_description = descriptionCombo->itemText(index);
		if(b_income) current_source = 9;
		else current_source = 10;
	}
	updateDisplay();
}
void OverTimeReport::categoryChanged(int index) {
	descriptionCombo->blockSignals(true);
	descriptionCombo->clear();
	descriptionCombo->addItem(i18n("All Descriptions Combined"));
	current_account = NULL;
	if(index == 0) {
		if(sourceCombo->currentIndex() == 2) {
			current_source = 1;
		} else {
			current_source = 2;
		}
		descriptionCombo->setEnabled(false);
	} else {
		if(sourceCombo->currentIndex() == 1) {
			int i = categoryCombo->currentIndex() - 1;
			if(i < (int) budget->expensesAccounts.count()) {
				current_account = budget->expensesAccounts.at(i);
			}
			current_source = 6;
		} else {
			int i = categoryCombo->currentIndex() - 1;
			if(i < (int) budget->incomesAccounts.count()) {
				current_account = budget->incomesAccounts.at(i);
			}
			current_source = 5;
		}
		has_empty_description = false;
		QMap<QString, bool> descriptions;
		Transaction *trans = budget->transactions.first();
		while(trans) {
			if((trans->fromAccount() == current_account || trans->toAccount() == current_account)) {
				if(trans->description().isEmpty()) has_empty_description = true;
				else descriptions[trans->description()] = true;
			}
			trans = budget->transactions.next();
		}
		QMap<QString, bool>::iterator it_e = descriptions.end();
		for(QMap<QString, bool>::iterator it = descriptions.begin(); it != it_e; ++it) {
			descriptionCombo->addItem(it.key());
		}
		if(has_empty_description) descriptionCombo->addItem(i18n("No description"));
		descriptionCombo->setEnabled(true);
	}
	descriptionCombo->blockSignals(false);
	updateDisplay();
}
void OverTimeReport::sourceChanged(int index) {
	categoryCombo->blockSignals(true);
	descriptionCombo->blockSignals(true);
	categoryCombo->clear();
	descriptionCombo->clear();
	descriptionCombo->setEnabled(false);
	descriptionCombo->addItem(i18n("All Descriptions Combined"));
	current_description = "";
	current_account = NULL;
	categoryCombo->addItem(i18n("All Categories Combined"));
	if(index == 2) {
		Account *account = budget->incomesAccounts.first();
		while(account) {
			categoryCombo->addItem(account->name());
			account = budget->incomesAccounts.next();
		}
		categoryCombo->setEnabled(true);
		current_source = 1;
	} else if(index == 1) {
		Account *account = budget->expensesAccounts.first();
		while(account) {
			categoryCombo->addItem(account->name());
			account = budget->expensesAccounts.next();
		}
		categoryCombo->setEnabled(true);
		current_source = 2;
	} else {
		categoryCombo->setEnabled(false);
		current_source = 0;
	}
	categoryCombo->blockSignals(false);
	descriptionCombo->blockSignals(false);
	updateDisplay();
}


void OverTimeReport::saveConfig() {
	KConfigGroup config = KSharedConfig::openConfig()->group("Over Time Report");
	config.writeEntry("size", ((QDialog*) parent())->size());
	config.writeEntry("valueEnabled", valueButton->isChecked());
	config.writeEntry("dailyAverageEnabled", dailyButton->isChecked());
	config.writeEntry("monthlyAverageEnabled", monthlyButton->isChecked());
	config.writeEntry("yearlyAverageEnabled", yearlyButton->isChecked());
	config.writeEntry("transactionCountEnabled", countButton->isChecked());
	config.writeEntry("valuePerTransactionEnabled", perButton->isChecked());
}
void OverTimeReport::save() {
	QMimeDatabase db;
	QUrl url = QFileDialog::getSaveFileUrl(this, QString::null, QUrl(), db.mimeTypeForName("text/html").filterString());
	if(url.isEmpty() && url.isValid()) return;
	if(url.isLocalFile()) {
		if(QFile::exists(url.toLocalFile())) {
			if(KMessageBox::warningYesNo(this, i18n("The selected file already exists. Would you like to overwrite the old copy?")) != KMessageBox::Yes) return;
		}
		QFileInfo info(url.toLocalFile());
		if(info.isDir()) {
			KMessageBox::error(this, i18n("You selected a directory!"));
			return;
		}
		QSaveFile ofile(url.toLocalFile());
		ofile.open(QIODevice::WriteOnly);
		ofile.setPermissions((QFile::Permissions) 0x0660);
		if(!ofile.isOpen()) {
			ofile.cancelWriting();
			KMessageBox::error(this, i18n("Couldn't open file for writing."));
			return;
		}
		QTextStream outf(&ofile);
		outf.setCodec("UTF-8");
		outf << source;
		if(!ofile.commit()) {
			KMessageBox::error(this, i18n("Error while writing file; file was not saved."));
			return;
		}
		return;
	}

	QTemporaryFile tf;
	tf.open();
	tf.setAutoRemove(true);
	QTextStream outf(&tf);
	outf.setCodec("UTF-8");
	outf << source;	
	KIO::FileCopyJob *job = KIO::file_copy(QUrl::fromLocalFile(tf.fileName()), url, KIO::Overwrite);
	KJobWidgets::setWindow(job, this);
	if(!job->exec()) {
		if(job->error()) {
			KMessageBox::error(this, i18n("Failed to upload file to %1: %2", url.toString(), job->errorString()));
		}
	}

}

void OverTimeReport::print() {
	htmlpart->view()->print();
}

void OverTimeReport::updateDisplay() {	

	int columns = 2;
	bool enabled[6];
	enabled[0] = valueButton->isChecked();
	enabled[1] = dailyButton->isChecked();
	enabled[2] = monthlyButton->isChecked();
	enabled[3] = yearlyButton->isChecked();
	enabled[4] = countButton->isChecked();
	enabled[5] = perButton->isChecked();
	for(size_t i = 0; i < 6; i++) {
		if(enabled[i]) columns++;
	}

	QVector<month_info> monthly_values;
	month_info *mi = NULL;
	QDate first_date;
	AccountType at = ACCOUNT_TYPE_EXPENSES;
	Account *account = NULL;
	int type = 0;
	QString title, valuetitle, pertitle;
	switch(current_source) {
		case 0: {
			type = 0;
			title = i18n("Profits");
			pertitle = i18n("Average Profit");
			valuetitle = title;
			break;
		}
		case 1: {
			title = i18n("Incomes");
			pertitle = i18n("Average Income");
			valuetitle = title;
			type = 1;
			at = ACCOUNT_TYPE_INCOMES;
			break;
		}
		case 2: {
			title = i18n("Expenses");
			pertitle = i18n("Average Cost");
			valuetitle = title;
			type = 1;
			at = ACCOUNT_TYPE_EXPENSES;
			break;
		}
		case 5: {
			account = current_account;
			title = i18n("Incomes: %1", account->name());
			pertitle = i18n("Average Income");
			valuetitle = i18n("Incomes");
			type = 2;
			at = ACCOUNT_TYPE_INCOMES;
			break;
		}
		case 6: {
			account = current_account;
			title = i18n("Expenses: %1", account->name());
			pertitle = i18n("Average Cost");
			valuetitle = i18n("Expenses");
			type = 2;
			at = ACCOUNT_TYPE_EXPENSES;
			break;
		}
		case 9: {
			account = current_account;
			title = i18n("Incomes: %2, %1", account->name(), current_description.isEmpty() ? i18n("No description") : current_description);
			pertitle = i18n("Average Income");
			valuetitle = i18n("Incomes");
			type = 3;
			at = ACCOUNT_TYPE_INCOMES;
			break;
		}
		case 10: {
			account = current_account;
			title = i18n("Expenses: %2, %1",account->name(),current_description.isEmpty() ? i18n("No description") : current_description);
			pertitle = i18n("Average Cost");
			valuetitle = i18n("Expenses");
			type = 3;
			at = ACCOUNT_TYPE_EXPENSES;
			break;
		}
	}

	Transaction *trans = budget->transactions.first();
	QDate start_date;
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
	first_date = start_date;

	QDate curdate = QDate::currentDate().addDays(-1);
	if(curdate.day() < curdate.daysInMonth()) {
		curdate = curdate.addMonths(-1);
		curdate.setDate(curdate.year(), curdate.month(), curdate.daysInMonth());
	}
	if(curdate <= first_date || (start_date.month() == curdate.month() && start_date.year() == curdate.year())) {
		curdate = QDate::currentDate();
	}

	bool started = false;
	trans = budget->transactions.first();
	while(trans && trans->date() <= curdate) {
		bool include = false;
		int sign = 1;
		if(!started && trans->date() >= first_date) started = true;
		if(started) {
			if((type == 1 && trans->fromAccount()->type() == at) || (type == 2 && trans->fromAccount() == account) || (type == 3 && trans->fromAccount() == account && trans->description() == current_description) || (type == 0 && trans->fromAccount()->type() != ACCOUNT_TYPE_ASSETS)) {
				if(type == 0) sign = 1;
				else if(at == ACCOUNT_TYPE_INCOMES) sign = 1;
				else sign = -1;
				include = true;
			} else if((type == 1 && trans->toAccount()->type() == at) || (type == 2 && trans->toAccount() == account) || (type == 3 && trans->toAccount() == account && trans->description() == current_description) || (type == 0 && trans->toAccount()->type() != ACCOUNT_TYPE_ASSETS)) {
				if(type == 0) sign = -1;
				else if(at == ACCOUNT_TYPE_INCOMES) sign = -1;
				else sign = 1;
				include = true;
			}
		}
		if(include) {
			if(!mi || trans->date() > mi->date) {
				QDate newdate, olddate;
				newdate.setDate(trans->date().year(), trans->date().month(), trans->date().daysInMonth());
				if(mi) {
					olddate = mi->date.addMonths(1);
					olddate.setDate(olddate.year(), olddate.month(), olddate.daysInMonth());
				} else {
					olddate.setDate(first_date.year(), first_date.month(), first_date.daysInMonth());
				}
				while(olddate < newdate) {
					monthly_values.append(month_info());
					mi = &monthly_values.back();
					mi->value = 0.0;
					mi->count = 0.0;
					mi->date = olddate;
					olddate = olddate.addMonths(1);
					olddate.setDate(olddate.year(), olddate.month(), olddate.daysInMonth());
				}
				monthly_values.append(month_info());
				mi = &monthly_values.back();
				mi->value = trans->value() * sign;
				mi->count = trans->quantity();
				mi->date = newdate;
			} else {
				mi->value += trans->value() * sign;
				mi->count += trans->quantity();
			}
		}
		trans = budget->transactions.next();
	}
	if(mi) {
		while(mi->date < curdate) {
			QDate newdate = mi->date.addMonths(1);
			newdate.setDate(newdate.year(), newdate.month(), newdate.daysInMonth());
			monthly_values.append(month_info());
			mi = &monthly_values.back();
			mi->value = 0.0;
			mi->count = 0.0;
			mi->date = newdate;
		}
	}
	double scheduled_value = 0.0;
	double scheduled_count = 0.0;
	if(mi) {
		ScheduledTransaction *strans = budget->scheduledTransactions.first();
		while(strans && strans->transaction()->date() <= mi->date) {
			trans = strans->transaction();
			bool include = false;
			int sign = 1;
			if((type == 1 && trans->fromAccount()->type() == at) || (type == 2 && trans->fromAccount() == account) || (type == 3 && trans->fromAccount() == account && trans->description() == current_description) || (type == 0 && trans->fromAccount()->type() != ACCOUNT_TYPE_ASSETS)) {
				if(type == 0) sign = 1;
				else if(at == ACCOUNT_TYPE_INCOMES) sign = 1;
				else sign = -1;
				include = true;
			} else if((type == 1 && trans->toAccount()->type() == at) || (type == 2 && trans->toAccount() == account) || (type == 3 && trans->toAccount() == account && trans->description() == current_description) || (type == 0 && trans->toAccount()->type() != ACCOUNT_TYPE_ASSETS)) {
				if(type == 0) sign = -1;
				else if(at == ACCOUNT_TYPE_INCOMES) sign = -1;
				else sign = 1;
				include = true;
			}
			if(include) {
				int count = (strans->recurrence() ? strans->recurrence()->countOccurrences(mi->date) : 1);
				scheduled_value += (trans->value() * sign * count);
				scheduled_count += count * trans->quantity();
			}
			strans = budget->scheduledTransactions.next();
		}
	}
	double average_month = averageMonth(first_date, curdate);
	double average_year = averageYear(first_date, curdate);
	source = "";
	QTextStream outf(&source, QIODevice::WriteOnly);
	outf << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">" << '\n';
	outf << "<html>" << '\n';
	outf << "\t<head>" << '\n';
	outf << "\t\t<title>";
	outf << htmlize_string(title);
	outf << "</title>" << '\n';
	outf << "\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" << '\n';
	outf << "\t\t<meta name=\"GENERATOR\" content=\"Eqonomize!\">" << '\n';
	outf << "\t</head>" << '\n';
	outf << "\t<body>" << '\n';
	outf << "\t\t<h2>" << htmlize_string(title) << "</h2>" << '\n';
	outf << "\t\t<table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"5\">" << '\n';
	outf << "\t\t\t<thead align=\"left\">" << '\n';
	outf << "\t\t\t\t<tr>" << '\n';
	outf << "\t\t\t\t\t<th style=\"border-bottom: thin solid\">" << htmlize_string(i18n("Year")) << "</th>";
	outf << "\t\t\t\t\t<th style=\"border-bottom: thin solid\">" << htmlize_string(i18n("Month")) << "</th>";
	bool use_footer1 = false;
	if(enabled[0]) {
		outf << "\t\t\t\t\t<th style=\"border-bottom: thin solid\">" << htmlize_string(valuetitle);
		if(mi && mi->date > curdate) {outf << "*"; use_footer1 = true;}
		outf<< "</th>";
	}
	if(enabled[1]) outf << "\t\t\t\t\t<th style=\"border-bottom: thin solid\">" << htmlize_string(i18n("Daily Average")) << "</th>";
	if(enabled[2]) outf << "\t\t\t\t\t<th style=\"border-bottom: thin solid\">" << htmlize_string(i18n("Monthly Average")) << "**"  << "</th>";
	if(enabled[3]) outf << "\t\t\t\t\t<th style=\"border-bottom: thin solid\">" << htmlize_string(i18n("Yearly Average")) << "**"  << "</th>";
	if(enabled[4]) {
		outf << "\t\t\t\t\t<th style=\"border-bottom: thin solid\">" << htmlize_string(i18n("Quantity"));
		if(mi && mi->date > curdate) {outf << "*"; use_footer1 = true;}
		outf<< "</th>";
	}
	if(enabled[5]) {
		outf << "\t\t\t\t\t<th style=\"border-bottom: thin solid\">" << htmlize_string(pertitle);
		if(mi && mi->date > curdate) {outf << "*"; use_footer1 = true;}
		outf<< "</th>";
	}
	outf << "\t\t\t\t</tr>" << '\n';
	outf << "\t\t\t</thead>" << '\n';
	outf << "\t\t\t<tfoot>" << '\n';
	outf << "\t\t\t\t<tr>" << '\n';
	outf << "\t\t\t\t\t<th align=\"right\" colspan=\"" << QString::number(columns) << "\" style=\"font-weight: normal; border-top: thin solid\">" << "<small>";
	bool had_footer = false;
	if(use_footer1) {
		had_footer = true;
		outf << "*" << htmlize_string(i18n("Includes scheduled transactions"));
	}
	if(enabled[2] || enabled[3]) {
		if(had_footer) outf << "<br>";
		outf << "**" << htmlize_string(i18n("Adjusted for the average month / year (%1 / %2 days)",QLocale().toString(average_month, 'f', 1), QLocale().toString(average_year, 'f', 1)));
	}
	outf << "</small>" << "</th>" << '\n';
	outf << "\t\t\t\t</tr>" << '\n';
	outf << "\t\t\t</tfoot>" << '\n';
	outf << "\t\t\t<tbody>" << '\n';
	QVector<month_info>::iterator it_b = monthly_values.begin();
	QVector<month_info>::iterator it_e = monthly_values.end();
	if(it_e != it_b) --it_e;
	QVector<month_info>::iterator it = monthly_values.end();
	int year = 0;
	double yearly_value = 0.0, total_value = 0.0;
	double yearly_count = 0.0, total_count = 0.0;
	QDate year_date;
	bool first_year = true, first_month = true;
	bool multiple_months = monthly_values.size() > 1;
	bool multiple_years = multiple_months && first_date.year() != curdate.year();
	int i_count_frac = 0;
	double intpart = 0.0;
	while(it != it_b) {
		--it;
		if(modf(it->count, &intpart) != 0.0) {
			i_count_frac = 2;
			break;
		}
	}
	it = monthly_values.end();
	while(it != it_b) {
		--it;
		outf << "\t\t\t\t<tr>" << '\n';
		if(first_month || year != it->date.year()) {
			if(!first_month && multiple_years) {
				outf << "\t\t\t\t\t<td></td>";
				outf << "\t\t\t\t\t<td align=\"left\" style=\"border-right: thin solid; border-top: thin solid; border-bottom: thin solid\"><b>" << htmlize_string(i18n("Subtotal")) << "</b></td>";
				if(enabled[0]) outf << "<td nowrap align=\"right\" style=\"border-top: thin solid; border-bottom: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(first_year ? (yearly_value + scheduled_value) : yearly_value)) << "</b></td>";
				int days = 1;
				if(first_year) {
					days = curdate.dayOfYear();
				} else if(year == first_date.year()) {
					days = year_date.daysInYear();
					days -= (first_date.dayOfYear() - 1);
				} else {
					days= year_date.daysInYear();
				}
				if(enabled[1]) outf << "<td nowrap align=\"right\" style=\"border-top: thin solid; border-bottom: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(yearly_value / days)) << "</b></td>";
				if(enabled[2]) outf << "<td nowrap align=\"right\" style=\"border-top: thin solid; border-bottom: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(((yearly_value * average_month) / days))) << "</b></td>";
				if(enabled[3]) outf << "<td nowrap align=\"right\" style=\"border-top: thin solid; border-bottom: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString((yearly_value * average_year) / days)) << "</b></td>";
				if(enabled[4]) outf << "<td nowrap align=\"right\" style=\"border-top: thin solid; border-bottom: thin solid\"><b>" << htmlize_string(QLocale().toString(first_year ? (yearly_count + scheduled_count) : yearly_count, 'f', i_count_frac)) << "</b></td>";
				double pervalue = 0.0;
				if(first_year) {
					pervalue = (((yearly_count + scheduled_count) == 0.0) ? 0.0 : ((yearly_value + scheduled_value) / (yearly_count + scheduled_count)));
				} else {
					pervalue = (yearly_count == 0.0 ? 0.0 : (yearly_value / yearly_count));
				}
				if(enabled[5]) outf << "<td nowrap align=\"right\" style=\"border-top: thin solid; border-bottom: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(pervalue)) << "</b></td>";
				first_year = false;
				outf << "\n";
				outf << "\t\t\t\t</tr>" << '\n';
				outf << "\t\t\t\t<tr>" << '\n';
			}
			year = it->date.year();
			yearly_value = it->value;
			yearly_count = it->count;
			year_date = it->date;
			outf << "\t\t\t\t\t<td align=\"left\">" << htmlize_string(QString::number(it->date.year())) << "</td>";
		} else {
			yearly_value += it->value;
			yearly_count += it->count;
			outf << "\t\t\t\t\t<td></td>";
		}
		total_value += it->value;
		total_count += it->count;
		outf << "\t\t\t\t\t<td align=\"left\" style=\"border-right: thin solid\">" << htmlize_string(QDate::longMonthName(it->date.month(), QDate::StandaloneFormat)) << "</td>";
		if(enabled[0]) outf << "<td nowrap align=\"right\">" << htmlize_string(QLocale().toCurrencyString(first_month ? (it->value + scheduled_value) : it->value)) << "</td>";
		int days = 0;
		if(first_month) {
			days = curdate.day();
		} else if(it == it_b) {
			days = it->date.daysInMonth();
			days -= (first_date.day() - 1);
		} else {
			days = it->date.daysInMonth();
		}
		if(enabled[1]) outf << "<td nowrap align=\"right\">" << htmlize_string(QLocale().toCurrencyString(it->value / days)) << "</td>";
		if(enabled[2]) outf << "<td nowrap align=\"right\">" << htmlize_string(QLocale().toCurrencyString((it->value * average_month) / days)) << "</td>";
		if(enabled[3]) outf << "<td nowrap align=\"right\">" << htmlize_string(QLocale().toCurrencyString((it->value * average_year) / days)) << "</td>";
		if(enabled[4]) outf << "<td nowrap align=\"right\">" << htmlize_string(QLocale().toString(first_month ? (it->count + scheduled_count) : it->count, 'f', i_count_frac)) << "</td>";
		double pervalue = 0.0;
		if(first_month) {
			pervalue = (((it->count + scheduled_count) == 0.0) ? 0.0 : ((it->value + scheduled_value) / (it->count + scheduled_count)));
		} else {
			pervalue = (it->count == 0.0 ? 0.0 : (it->value / it->count));
		}
		if(enabled[5]) outf << "<td nowrap align=\"right\">" << htmlize_string(QLocale().toCurrencyString(pervalue)) << "</td>";
		first_month = false;
		outf << "\n";
		outf << "\t\t\t\t</tr>" << '\n';
	}
	if(multiple_years) {
		outf << "\t\t\t\t<tr>" << '\n';
		outf << "\t\t\t\t\t<td></td>";
		outf << "\t\t\t\t\t<td align=\"left\" style=\"border-right: thin solid; border-top: thin solid\"><b>" << htmlize_string(i18n("Subtotal")) << "</b></td>";
		if(enabled[0]) outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(yearly_value)) << "</b></td>";
		int days = year_date.daysInYear();
		days -= (first_date.dayOfYear() - 1);
		if(enabled[1]) outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(yearly_value / days)) << "</b></td>";
		if(enabled[2]) outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString((yearly_value * average_month) / days)) << "</b></td>";
		if(enabled[3]) outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString((yearly_value * average_year) / days)) << "</b></td>";
		if(enabled[4]) outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toString(yearly_count, 'f', i_count_frac)) << "</b></td>";
		if(enabled[5]) outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(yearly_count == 0.0 ? 0.0 : (yearly_value / yearly_count))) << "</b></td>";
		outf << "\n";
		outf << "\t\t\t\t</tr>" << '\n';
	}
	if(multiple_months) {
		outf << "\t\t\t\t<tr>" << '\n';
		int days = first_date.daysTo(curdate) + 1;
		outf << "\t\t\t\t\t<td align=\"left\" style=\"border-top: thin solid\"><b>" << htmlize_string(i18n("Total")) << "</b></td>";
		outf << "\t\t\t\t\t<td style=\"border-right: thin solid; border-top: thin solid\"></td>";
		if(enabled[0]) outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(total_value + scheduled_value)) << "</b></td>";
		if(enabled[1]) outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(total_value / days)) << "</b></td>";
		if(enabled[2]) outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString((total_value * average_month) / days)) << "</b></td>";
		if(enabled[3]) outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString((total_value * average_year) / days)) << "</b></td>";
		if(enabled[4]) outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toString(total_count + scheduled_count, 'f', i_count_frac)) << "</b></td>";
		if(enabled[5]) outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString((total_count + scheduled_count) == 0.0 ? 0.0 : ((total_value + scheduled_value) / (total_count + scheduled_count)))) << "</b></td>";
		outf << "\n";
		outf << "\t\t\t\t</tr>" << '\n';
	}
	outf << "\t\t\t</tbody>" << '\n';
	outf << "\t\t</table>" << '\n';
	outf << "\t</body>" << '\n';
	outf << "</html>" << '\n';
	htmlpart->begin();
	htmlpart->write(source);
	htmlpart->end();
}
void OverTimeReport::updateTransactions() {
	if(descriptionCombo->isEnabled() && current_account) {
		int curindex = 0;
		descriptionCombo->blockSignals(true);
		descriptionCombo->clear();
		descriptionCombo->addItem(i18n("All Descriptions Combined"));
		has_empty_description = false;
		QMap<QString, bool> descriptions;
		Transaction *trans = budget->transactions.first();
		while(trans) {
			if((trans->fromAccount() == current_account || trans->toAccount() == current_account)) {
				if(trans->description().isEmpty()) has_empty_description = true;
				else descriptions[trans->description()] = true;
			}
			trans = budget->transactions.next();
		}
		QMap<QString, bool>::iterator it_e = descriptions.end();
		int i = 1;
		for(QMap<QString, bool>::iterator it = descriptions.begin(); it != it_e; ++it) {
			if((current_source == 9 || current_source == 10) && it.key() == current_description) {
				curindex = i;
			}
			descriptionCombo->addItem(it.key());
			i++;
		}
		if(has_empty_description) {
			if((current_source == 9 || current_source == 10) && current_description.isEmpty()) curindex = i;
			descriptionCombo->addItem(i18n("No description"));
		}
		if(curindex < descriptionCombo->count()) {
			descriptionCombo->setCurrentIndex(curindex);
		}
		bool b_income = (current_account && current_account->type() == ACCOUNT_TYPE_INCOMES);
		if(descriptionCombo->currentIndex() == 0) {
			if(b_income) current_source = 5;
			else current_source = 6;
			current_description = "";
		}
		descriptionCombo->blockSignals(false);
	}
	updateDisplay();
}
void OverTimeReport::updateAccounts() {
	if(categoryCombo->isEnabled()) {
		int curindex = 0;
		categoryCombo->blockSignals(true);
		descriptionCombo->blockSignals(true);
		categoryCombo->clear();
		categoryCombo->addItem(i18n("All Categories Combined"));
		int i = 1;
		if(sourceCombo->currentIndex() == 1) {
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
		if(curindex == 0) {
			descriptionCombo->clear();
			descriptionCombo->setEnabled(false);
			descriptionCombo->addItem(i18n("All Descriptions Combined"));
			if(sourceCombo->currentIndex() == 2) {
				current_source = 1;
			} else {
				current_source = 2;
			}
			descriptionCombo->setEnabled(false);
		}
		categoryCombo->blockSignals(false);
		descriptionCombo->blockSignals(false);
	}
	updateDisplay();
}

#include "overtimereport.moc"
