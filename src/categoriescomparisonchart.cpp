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

#include "categoriescomparisonchart.h"

#include "budget.h"
#include "account.h"
#include "transaction.h"
#include "recurrence.h"

#include <QCheckBox>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImage>
#include <QImageWriter>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMap>
#include <QObject>
#include <QMatrix>
#include <QPainter>
#include <QPushButton>
#include <QPrinter>
#include <QPrintDialog>
#include <QPixmap>
#include <QRadioButton>
#include <QResizeEvent>
#include <QString>
#include <QVBoxLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDialog>
#include <QUrl>
#include <QFileDialog>
#include <QApplication>
#include <QDateEdit>
#include <QComboBox>
#include <QMessageBox>
#include <QSettings>
#include <QSaveFile>
#include <QMimeDatabase>
#include <QMimeType>
#include <QStandardPaths>

#include <math.h>

extern double monthsBetweenDates(const QDate &date1, const QDate &date2);
extern QString last_picture_directory;

CategoriesComparisonChart::CategoriesComparisonChart(Budget *budg, QWidget *parent) : QWidget(parent), budget(budg) {

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

	QWidget *settingsWidget = new QWidget(this);
	QHBoxLayout *settingsLayout = new QHBoxLayout(settingsWidget);	

	QHBoxLayout *choicesLayout = new QHBoxLayout();
	settingsLayout->addLayout(choicesLayout);
	fromButton = new QCheckBox(tr("From"), settingsWidget);
	fromButton->setChecked(true);
	choicesLayout->addWidget(fromButton);
	fromEdit = new QDateEdit(settingsWidget);
	fromEdit->setCalendarPopup(true);
	choicesLayout->addWidget(fromEdit);
	choicesLayout->setStretchFactor(fromEdit, 1);
	choicesLayout->addWidget(new QLabel(tr("To"), settingsWidget));
	toEdit = new QDateEdit(settingsWidget);
	toEdit->setCalendarPopup(true);	
	choicesLayout->addWidget(toEdit);
	choicesLayout->setStretchFactor(toEdit, 1);
	prevYearButton = new QPushButton(QIcon::fromTheme("eqz-previous-year"), "", settingsWidget);
	choicesLayout->addWidget(prevYearButton);
	prevMonthButton = new QPushButton(QIcon::fromTheme("eqz-previous-month"), "", settingsWidget);
	choicesLayout->addWidget(prevMonthButton);
	nextMonthButton = new QPushButton(QIcon::fromTheme("eqz-next-month"), "", settingsWidget);
	choicesLayout->addWidget(nextMonthButton);
	nextYearButton = new QPushButton(QIcon::fromTheme("eqz-next-year"), "", settingsWidget);
	choicesLayout->addWidget(nextYearButton);
	settingsLayout->addSpacing(12);
	settingsLayout->setStretchFactor(choicesLayout, 2);

	QHBoxLayout *typeLayout = new QHBoxLayout();
	settingsLayout->addLayout(typeLayout);
	typeLayout->addWidget(new QLabel(tr("Source:"), settingsWidget));
	sourceCombo = new QComboBox(settingsWidget);
	sourceCombo->setEditable(false);
	sourceCombo->addItem(tr("All Expenses"));
	sourceCombo->addItem(tr("All Incomes"));
	sourceCombo->addItem(tr("All Accounts"));
	Account *account = budget->expensesAccounts.first();
	while(account) {
		sourceCombo->addItem(tr("Expenses: %1").arg(account->name()));
		account = budget->expensesAccounts.next();
	}
	account = budget->incomesAccounts.first();
	while(account) {
		sourceCombo->addItem(tr("Incomes: %1").arg(account->name()));
		account = budget->incomesAccounts.next();
	}
	typeLayout->addWidget(sourceCombo);
	typeLayout->setStretchFactor(sourceCombo, 1);
	settingsLayout->setStretchFactor(typeLayout, 1);

	current_account = NULL;

	layout->addWidget(settingsWidget);
	
	resetOptions();

	connect(sourceCombo, SIGNAL(activated(int)), this, SLOT(sourceChanged(int)));
	connect(prevMonthButton, SIGNAL(clicked()), this, SLOT(prevMonth()));
	connect(nextMonthButton, SIGNAL(clicked()), this, SLOT(nextMonth()));
	connect(prevYearButton, SIGNAL(clicked()), this, SLOT(prevYear()));
	connect(nextYearButton, SIGNAL(clicked()), this, SLOT(nextYear()));
	connect(fromButton, SIGNAL(toggled(bool)), fromEdit, SLOT(setEnabled(bool)));
	connect(fromButton, SIGNAL(toggled(bool)), this, SLOT(updateDisplay()));
	connect(fromEdit, SIGNAL(dateChanged(const QDate&)), this, SLOT(fromChanged(const QDate&)));
	connect(toEdit, SIGNAL(dateChanged(const QDate&)), this, SLOT(toChanged(const QDate&)));
	connect(saveButton, SIGNAL(clicked()), this, SLOT(save()));
	connect(printButton, SIGNAL(clicked()), this, SLOT(print()));

}

CategoriesComparisonChart::~CategoriesComparisonChart() {
}

void CategoriesComparisonChart::resetOptions() {
	QDate first_date;
	Transaction *trans = budget->transactions.first();
	while(trans) {
		if(trans->fromAccount()->type() != ACCOUNT_TYPE_ASSETS || trans->toAccount()->type() != ACCOUNT_TYPE_ASSETS) {
			first_date = trans->date();
			break;
		}
		trans = budget->transactions.next();
	}
	to_date = QDate::currentDate();
	from_date = to_date.addYears(-1).addDays(1);
	if(first_date.isNull()) {
		from_date = to_date;
	} else if(from_date < first_date) {
		from_date = first_date;
	}
	fromEdit->setDate(from_date);
	toEdit->setDate(to_date);
	sourceCombo->setCurrentIndex(0);
	sourceChanged(0);
}
void CategoriesComparisonChart::sourceChanged(int index) {
	fromButton->setEnabled(index != 2);
	fromEdit->setEnabled(index != 2);
	updateDisplay();
}
void CategoriesComparisonChart::saveConfig() {
	QSettings settings;
	settings.beginGroup("CategoriesComparisonChart");
	settings.setValue("size", ((QDialog*) parent())->size());
	settings.endGroup();
}
void CategoriesComparisonChart::toChanged(const QDate &date) {
	bool error = false;
	if(!date.isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		error = true;
	}
	if(!error && fromEdit->date() > date) {
		if(fromButton->isChecked() && fromButton->isEnabled()) {
			QMessageBox::critical(this, tr("Error"), tr("To date is before from date."));
		}
		from_date = date;
		fromEdit->blockSignals(true);
		fromEdit->setDate(from_date);
		fromEdit->blockSignals(false);
	}
	if(error) {
		toEdit->setFocus();
		toEdit->blockSignals(true);
		toEdit->setDate(to_date);
		toEdit->blockSignals(false);
		toEdit->selectAll();
		return;
	}
	to_date = date;
	updateDisplay();
}
void CategoriesComparisonChart::fromChanged(const QDate &date) {
	bool error = false;
	if(!date.isValid()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid date."));
		error = true;
	}
	if(!error && date > toEdit->date()) {
		QMessageBox::critical(this, tr("Error"), tr("From date is after to date."));
		to_date = date;
		toEdit->blockSignals(true);
		toEdit->setDate(to_date);
		toEdit->blockSignals(false);
	}
	if(error) {
		fromEdit->setFocus();
		fromEdit->blockSignals(true);
		fromEdit->setDate(from_date);
		fromEdit->blockSignals(false);
		fromEdit->selectAll();
		return;
	}
	from_date = date;
	if(fromButton->isChecked()) updateDisplay();
}
void CategoriesComparisonChart::prevMonth() {	
	fromEdit->blockSignals(true);
	toEdit->blockSignals(true);
	from_date = from_date.addMonths(-1);
	fromEdit->setDate(from_date);
	if(to_date.day() == to_date.daysInMonth()) {
		to_date = to_date.addMonths(-1);
		to_date.setDate(to_date.year(), to_date.month(), to_date.daysInMonth());
	} else {
		to_date = to_date.addMonths(-1);
	}
	toEdit->setDate(to_date);
	fromEdit->blockSignals(false);
	toEdit->blockSignals(false);
	updateDisplay();
}
void CategoriesComparisonChart::nextMonth() {
	fromEdit->blockSignals(true);
	toEdit->blockSignals(true);
	from_date = from_date.addMonths(1);
	fromEdit->setDate(from_date);
	if(to_date.day() == to_date.daysInMonth()) {
		to_date = to_date.addMonths(1);
		to_date.setDate(to_date.year(), to_date.month(), to_date.daysInMonth());
	} else {
		to_date = to_date.addMonths(1);
	}
	toEdit->setDate(to_date);
	fromEdit->blockSignals(false);
	toEdit->blockSignals(false);
	updateDisplay();
}
void CategoriesComparisonChart::prevYear() {
	fromEdit->blockSignals(true);
	toEdit->blockSignals(true);
	from_date = from_date.addYears(-1);
	fromEdit->setDate(from_date);
	if(to_date.day() == to_date.daysInMonth()) {
		to_date = to_date.addYears(-1);
		to_date.setDate(to_date.year(), to_date.month(), to_date.daysInMonth());
	} else {
		to_date = to_date.addYears(-1);
	}
	toEdit->setDate(to_date);
	fromEdit->blockSignals(false);
	toEdit->blockSignals(false);
	updateDisplay();
}
void CategoriesComparisonChart::nextYear() {
	fromEdit->blockSignals(true);
	toEdit->blockSignals(true);
	from_date = from_date.addYears(1);
	fromEdit->setDate(from_date);
	if(to_date.day() == to_date.daysInMonth()) {
		to_date = to_date.addYears(1);
		to_date.setDate(to_date.year(), to_date.month(), to_date.daysInMonth());
	} else {
		to_date = to_date.addYears(1);
	}
	toEdit->setDate(to_date);
	fromEdit->blockSignals(false);
	toEdit->blockSignals(false);
	updateDisplay();
}
void CategoriesComparisonChart::onFilterSelected(QString filter) {
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
void CategoriesComparisonChart::save() {
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

void CategoriesComparisonChart::print() {
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
QColor getColor(int index) {
	switch(index) {
		case 0: {return Qt::red;}
		case 1: {return Qt::green;}
		case 2: {return Qt::blue;}
		case 3: {return Qt::cyan;}
		case 4: {return Qt::magenta;}
		case 5: {return Qt::yellow;}
		case 6: {return Qt::darkRed;}
		case 7: {return Qt::darkGreen;}
		case 8: {return Qt::darkBlue;}
		case 9: {return Qt::darkCyan;}
		case 10: {return Qt::darkMagenta;}
		case 11: {return Qt::darkYellow;}
		default: {return Qt::gray;}
	}
}
QBrush getBrush(int index) {
	QBrush brush;
	switch(index / 12) {
		case 0: {brush.setStyle(Qt::SolidPattern); break;}
		case 1: {brush.setStyle(Qt::HorPattern); break;}
		case 2: {brush.setStyle(Qt::VerPattern); break;}
		case 3: {brush.setStyle(Qt::Dense5Pattern); break;}
		case 4: {brush.setStyle(Qt::CrossPattern); break;}
		default: {}
	}
	brush.setColor(getColor(index % 12));
	return brush;
}
void CategoriesComparisonChart::updateDisplay() {

	if(!isVisible()) return;

	QMap<Account*, double> values;
	QMap<Account*, double> counts;
	QMap<QString, double> desc_values;
	QMap<QString, double> desc_counts;
	double value_count = 0.0;
	double value = 0.0;
	QStringList desc_order; 

	current_account = NULL;

	AccountType type;
	switch(sourceCombo->currentIndex()) {
		case 0: {
			type = ACCOUNT_TYPE_EXPENSES;
			Account *account = budget->expensesAccounts.first();
			while(account) {
				values[account] = 0.0;
				counts[account] = 0.0;
				account = budget->expensesAccounts.next();
			}
			break;
		}
		case 1: {
			type = ACCOUNT_TYPE_INCOMES;
			Account *account = budget->incomesAccounts.first();
			while(account) {
				values[account] = 0.0;
				counts[account] = 0.0;
				account = budget->incomesAccounts.next();
			}
			break;
		}
		case 2: {
			type = ACCOUNT_TYPE_ASSETS;
			AssetsAccount *account = budget->assetsAccounts.first();
			while(account) {
				if(account != budget->balancingAccount) {
					if(account->accountType() == ASSETS_TYPE_SECURITIES) {
						values[account] = 0.0;
					} else {
						values[account] = account->initialBalance();
						value += account->initialBalance();
					}
					counts[account] = 0.0;
				}
				account = budget->assetsAccounts.next();
			}
			break;
		}
		default: {
			type = ACCOUNT_TYPE_EXPENSES;
			int i = sourceCombo->currentIndex() - 3;
			if(i < (int) budget->expensesAccounts.count()) current_account = budget->expensesAccounts.at(i);
			else current_account = budget->incomesAccounts.at(i - budget->expensesAccounts.count());
			if(current_account) {
				type = current_account->type();
				Transaction *trans = budget->transactions.first();
				while(trans) {
					if((trans->fromAccount() == current_account || trans->toAccount() == current_account)) {
						desc_values[trans->description()] = 0.0;
						desc_counts[trans->description()] = 0.0;
					}
					trans = budget->transactions.next();
				}
			}
		}
	}

	QDate first_date;
	bool first_date_reached = false;
	if(type == ACCOUNT_TYPE_ASSETS) {
		first_date_reached = true;
	} else if(fromButton->isChecked()) {
		first_date = from_date;
	} else {
		Transaction *trans = budget->transactions.first();
		while(trans) {
			if(trans->fromAccount()->type() != ACCOUNT_TYPE_ASSETS || trans->toAccount()->type() != ACCOUNT_TYPE_ASSETS) {
				first_date = trans->date();
				break;
			}
			trans = budget->transactions.next();
		}
		if(first_date.isNull()) first_date = QDate::currentDate();
		if(first_date > to_date) first_date = to_date;
	}
	Transaction *trans = budget->transactions.first();
	while(trans) {
		if(!first_date_reached && trans->date() >= first_date) first_date_reached = true;
		else if(first_date_reached && trans->date() > to_date) break;
		if(first_date_reached) {
			if(current_account) {
				if(trans->fromAccount() == current_account) {
					if(type == ACCOUNT_TYPE_EXPENSES) {desc_values[trans->description()] -= trans->value(); value -= trans->value();}
					else {desc_values[trans->description()] += trans->value(); value += trans->value();}
					desc_counts[trans->description()] += trans->quantity();
					value_count += trans->quantity();
				} else if(trans->toAccount() == current_account) {
					if(type == ACCOUNT_TYPE_EXPENSES) {desc_values[trans->description()] += trans->value(); value += trans->value();}
					else {desc_values[trans->description()] -= trans->value(); value -= trans->value();}
					desc_counts[trans->description()] += trans->quantity();
					value_count += trans->quantity();
				}
			} else if(type == ACCOUNT_TYPE_ASSETS) {
				if(trans->fromAccount()->type() == ACCOUNT_TYPE_ASSETS && trans->fromAccount() != budget->balancingAccount) {
					if(((AssetsAccount*) trans->fromAccount())->accountType() != ASSETS_TYPE_SECURITIES) {
						values[trans->fromAccount()] -= trans->value();
						value -= trans->value();
					}
					if(trans->toAccount() != budget->balancingAccount) {
						counts[trans->fromAccount()] += trans->quantity();
						value_count += trans->quantity();
					}
				}
				if(trans->toAccount()->type() == ACCOUNT_TYPE_ASSETS && trans->toAccount() != budget->balancingAccount) {
					if(((AssetsAccount*) trans->toAccount())->accountType() != ASSETS_TYPE_SECURITIES) {
						values[trans->toAccount()] += trans->value();
						value += trans->value();
					}
					if(trans->fromAccount() != budget->balancingAccount) {
						counts[trans->toAccount()] += trans->quantity();
						value_count += trans->quantity();
					}
				}
			} else if(type == ACCOUNT_TYPE_EXPENSES) {
				if(trans->fromAccount()->type() == ACCOUNT_TYPE_EXPENSES) {
					values[trans->fromAccount()] -= trans->value();
					value -= trans->value();
					counts[trans->fromAccount()] += trans->quantity();
					value_count += trans->quantity();
				} else if(trans->toAccount()->type() == ACCOUNT_TYPE_EXPENSES) {
					values[trans->toAccount()] += trans->value();
					value += trans->value();
					counts[trans->toAccount()] += trans->quantity();
					value_count += trans->quantity();
				}
			} else {
				if(trans->fromAccount()->type() == ACCOUNT_TYPE_INCOMES) {
					values[trans->fromAccount()] += trans->value();
					value += trans->value();
					counts[trans->fromAccount()] += trans->quantity();
					value_count += trans->quantity();
				} else if(trans->toAccount()->type() == ACCOUNT_TYPE_INCOMES) {
					values[trans->toAccount()] -= trans->value();
					value -= trans->value();
					counts[trans->toAccount()] += trans->quantity();
					value_count += trans->quantity();
				}
			}
		}
		trans = budget->transactions.next();
	}
	first_date_reached = false;
	ScheduledTransaction *strans = budget->scheduledTransactions.first();
	while(strans) {
		trans = strans->transaction();
		if(!first_date_reached && trans->date() >= first_date) first_date_reached = true;
		else if(first_date_reached && trans->date() > to_date) break;
		if(first_date_reached) {
			if(current_account) {
				if(trans->fromAccount() == current_account) {
					int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_date, to_date) : 1;
					if(type == ACCOUNT_TYPE_EXPENSES) {desc_values[trans->description()] -= trans->value() * count; value -= trans->value() * count;}
					else {desc_values[trans->description()] += trans->value() * count; value += trans->value() * count;}
					desc_counts[trans->description()] += count *  trans->quantity();
					value_count += count *  trans->quantity();
				} else if(trans->toAccount() == current_account) {
					int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_date, to_date) : 1;
					if(type == ACCOUNT_TYPE_EXPENSES) {desc_values[trans->description()] += trans->value() * count; value += trans->value() * count;}
					else {desc_values[trans->description()] -= trans->value() * count; value -= trans->value() * count;}
					desc_counts[trans->description()] += count *  trans->quantity();
					value_count += count *  trans->quantity();
				}
			} else if(type == ACCOUNT_TYPE_ASSETS) {
				if(trans->fromAccount()->type() == ACCOUNT_TYPE_ASSETS && trans->fromAccount() != budget->balancingAccount) {
					int count = strans->recurrence() ? strans->recurrence()->countOccurrences(to_date) : 1;
					if(trans->toAccount() != budget->balancingAccount) {
						counts[trans->fromAccount()] += count *  trans->quantity();
						value_count += count *  trans->quantity();
					}
					if(((AssetsAccount*) trans->fromAccount())->accountType() != ASSETS_TYPE_SECURITIES) {
						values[trans->fromAccount()] -= trans->value() * count;
						value -= trans->value() * count;
					}
				}
				if(trans->toAccount()->type() == ACCOUNT_TYPE_ASSETS && trans->toAccount() != budget->balancingAccount) {
					int count = strans->recurrence() ? strans->recurrence()->countOccurrences(to_date) : 1;
					if(trans->fromAccount() != budget->balancingAccount) {
						counts[trans->toAccount()] += count *  trans->quantity();
						value_count += count *  trans->quantity();
					}
					if(((AssetsAccount*) trans->toAccount())->accountType() != ASSETS_TYPE_SECURITIES) {
						values[trans->toAccount()] += trans->value() * count;
						value += trans->value() * count;
					}
				}
			} else if(type == ACCOUNT_TYPE_EXPENSES) {
				if(trans->fromAccount()->type() == ACCOUNT_TYPE_EXPENSES) {
					int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_date, to_date) : 1;
					counts[trans->fromAccount()] += count *  trans->quantity();
					values[trans->fromAccount()] -= trans->value() * count;
					value_count += count *  trans->quantity();
					value -= trans->value() * count;
				} else if(trans->toAccount()->type() == ACCOUNT_TYPE_EXPENSES) {
					int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_date, to_date) : 1;
					counts[trans->toAccount()] += count *  trans->quantity();
					values[trans->toAccount()] += trans->value() * count;
					value_count += count *  trans->quantity();
					value += trans->value() * count;
				}
			} else {
				if(trans->fromAccount()->type() == ACCOUNT_TYPE_INCOMES) {
					int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_date, to_date) : 1;
					counts[trans->fromAccount()] += count;
					values[trans->fromAccount()] += trans->value() * count;
					value_count += count;
					value += trans->value() * count;
				} else if(trans->toAccount()->type() == ACCOUNT_TYPE_INCOMES) {
					int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_date, to_date) : 1;
					counts[trans->toAccount()] += count;
					values[trans->toAccount()] -= trans->value() * count;
					value_count += count;
					value -= trans->value() * count;
				}
			}
		}
		strans = budget->scheduledTransactions.next();
	}

	if(type == ACCOUNT_TYPE_ASSETS) {
		Security *security = budget->securities.first();
		while(security) {
			double val = security->value(to_date);
			values[security->account()] += val;
			value += val;
			security = budget->securities.next();
		}
	}

	/*int days = first_date.daysTo(to_date) + 1;
	double months = monthsBetweenDates(first_date, to_date), years = yearsBetweenDates(first_date, to_date);*/

	QGraphicsScene *oldscene = scene;
	scene = new QGraphicsScene(this);
	scene->setBackgroundBrush(Qt::white);

	QFont legend_font = font();
	QFontMetrics fm(legend_font);
	int fh = fm.height();

	int diameter = 430;
	int margin = 35;
	int legend_x = diameter + margin * 2;
	Account *account = NULL;
	QMap<QString, double>::iterator it_desc = desc_values.begin();
	QMap<QString, double>::iterator it_desc_end = desc_values.end();
	
	double remaining_value = 0.0;
	if(current_account) {
		while(it_desc != it_desc_end) {
			int i = desc_order.size() - 1;
			while(i >= 0) {
				if(it_desc.value() < desc_values[desc_order[i]]) {
					if(i == desc_order.size() - 1) {
						if(i < 10) desc_order.push_back(it_desc.key());
						else remaining_value += it_desc.value();
					} else {
						desc_order.insert(i + 1, it_desc.key());
					}
					break;
				}
				i--;
			}
			if(i < 0) desc_order.push_front(it_desc.key());
			if(desc_order.size() > 10) {
				remaining_value += desc_values[desc_order.last()];
				desc_order.pop_back();
			}
			++it_desc;
		}
		if(desc_values.size() > desc_order.size()) {
			desc_order.push_back(tr("Other descriptions", "Referring to the generic description property"));
			desc_values[tr("Other descriptions", "Referring to the generic description property")] = remaining_value;
		}
	}	
	
	int n = 0;
	if(current_account) {
		n = desc_order.size();
	} else if(type == ACCOUNT_TYPE_ASSETS) {
		account = budget->assetsAccounts.first();
		if(account == budget->balancingAccount) account = budget->assetsAccounts.next();
		n = budget->assetsAccounts.count() - 1;
	} else if(type == ACCOUNT_TYPE_EXPENSES) {
		account = budget->expensesAccounts.first();
		n = budget->expensesAccounts.count();
	} else {
		account = budget->incomesAccounts.first();
		n = budget->incomesAccounts.count();
	}
		
	int index = 0;
	int text_width = 0;
	double value_bak = value;
	while((current_account && index < desc_order.size()) || account) {
		QString legend_string;
		double legend_value = 0.0;
		if(current_account) {
			if(desc_order[index].isEmpty()) {
				legend_string = tr("No description", "Referring to the generic description property");
			} else {
				legend_string = desc_order[index];
			}
			legend_value = desc_values[desc_order[index]];
		} else {
			legend_string = account->name();
			legend_value = values[account];
		}
		legend_value = (legend_value * 100) / value_bak;
		int deci = 0;
		if(legend_value < 10.0 && legend_value > -10.0) {
			legend_value = round(legend_value * 10.0) / 10.0;
			deci = 1;
		} else {
			legend_value = round(legend_value);
		}
		QGraphicsSimpleTextItem *legend_text = new QGraphicsSimpleTextItem(QString("%1 (%2%)").arg(legend_string).arg(QLocale().toString(legend_value, 'f', deci)));
		if(legend_text->boundingRect().width() > text_width) text_width = legend_text->boundingRect().width();
		legend_text->setFont(legend_font);
		legend_text->setBrush(Qt::black);
		legend_text->setPos(legend_x + 10 + fh + 5, margin + 10 + (fh + 5) * index);
		scene->addItem(legend_text);
		
		if(current_account) {
			if(desc_values[desc_order[index]] < 0.0) {
				value -= desc_values[desc_order[index]];
				desc_values[desc_order[index]] = 0.0;
			}
		} else {
			if(values[account] < 0.0) {
				value -= values[account];
				values[account] = 0.0;
			}
		}
		if(type == ACCOUNT_TYPE_ASSETS) {
			account = budget->assetsAccounts.next();
			if(account == budget->balancingAccount) account = budget->assetsAccounts.next();
		} else if(type == ACCOUNT_TYPE_EXPENSES) {
			account = budget->expensesAccounts.next();
		} else {
			account = budget->incomesAccounts.next();
		}
		index++;
	}

	if(current_account) {
	} else if(type == ACCOUNT_TYPE_ASSETS) {
		account = budget->assetsAccounts.first();
		if(account == budget->balancingAccount) account = budget->assetsAccounts.next();
	} else if(type == ACCOUNT_TYPE_EXPENSES) {
		account = budget->expensesAccounts.first();
	} else {
		account = budget->incomesAccounts.first();
	}
	index = 0;
	double current_value = 0.0;
	int prev_end = 0;
	while((current_account && index < desc_order.size()) || account) {
		if(current_account) current_value += desc_values[desc_order[index]];
		else current_value += values[account];
		int next_end = (int) lround((current_value * 360 * 16) / value);
		int length = (next_end - prev_end);
		QGraphicsEllipseItem *ellipse = new QGraphicsEllipseItem(-diameter/ 2.0, -diameter/ 2.0, diameter, diameter);
		ellipse->setStartAngle(prev_end);
		ellipse->setSpanAngle(length);
		prev_end = next_end;
		ellipse->setPen(Qt::NoPen);
		ellipse->setBrush(getBrush(index));
		ellipse->setPos(diameter / 2 + margin, diameter / 2 + margin);
		scene->addItem(ellipse);
		QGraphicsRectItem *legend_box = new QGraphicsRectItem(legend_x + 10, margin + 10 + (fh + 5) * index, fh, fh);
		legend_box->setPen(QPen(Qt::black));
		legend_box->setBrush(getBrush(index));
		scene->addItem(legend_box);
		if(type == ACCOUNT_TYPE_ASSETS) {
			account = budget->assetsAccounts.next();
			if(account == budget->balancingAccount) account = budget->assetsAccounts.next();
		} else if(type == ACCOUNT_TYPE_EXPENSES) {
			account = budget->expensesAccounts.next();
		} else {
			account = budget->incomesAccounts.next();
		}
		index++;
	}

	QGraphicsRectItem *legend_outline = new QGraphicsRectItem(legend_x, margin, 10 + fh + 5 + text_width + 10, 10 + ((fh + 5) * n) + 5);
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

}

void CategoriesComparisonChart::resizeEvent(QResizeEvent *e) {
	QWidget::resizeEvent(e);
	if(scene) {
		view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
	}
}

void CategoriesComparisonChart::updateTransactions() {
	updateDisplay();
}
void CategoriesComparisonChart::updateAccounts() {
	int curindex = sourceCombo->currentIndex();
	if(curindex > 2) {
		curindex = 0;
	}
	sourceCombo->blockSignals(true);
	sourceCombo->clear();
	sourceCombo->addItem(tr("All Expenses"));
	sourceCombo->addItem(tr("All Incomes"));
	sourceCombo->addItem(tr("All Accounts"));
	int i = 3;
	Account *account = budget->expensesAccounts.first();
	while(account) {
		sourceCombo->addItem(tr("Expenses: %1").arg(account->name()));
		if(account == current_account) curindex = i;
		account = budget->expensesAccounts.next();
		i++;
	}
	account = budget->incomesAccounts.first();
	while(account) {
		sourceCombo->addItem(tr("Incomes: %1").arg(account->name()));
		if(account == current_account) curindex = i;
		account = budget->incomesAccounts.next();
		i++;
	}
	if(curindex < sourceCombo->count()) sourceCombo->setCurrentIndex(curindex);
	sourceCombo->blockSignals(false);
	updateDisplay();
}

