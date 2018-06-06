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

#include "categoriescomparisonchart.h"

#include "budget.h"
#include "account.h"
#include "transaction.h"
#include "recurrence.h"
#include "eqonomize.h"


#ifdef QT_CHARTS_LIB
#include <QtCharts/QLegendMarker>
#include <QtCharts/QBarLegendMarker>
#include <QtCharts/QPieLegendMarker>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QHorizontalBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QPieSeries>
#else
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#endif
#include <QCheckBox>
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

extern QString last_picture_directory;
extern void calculate_minmax_lines(double &maxvalue, double &minvalue, int &y_lines, int &y_minor, bool minmaxequal = false, bool use_deciminor = true);

CategoriesComparisonChart::CategoriesComparisonChart(Budget *budg, QWidget *parent) : QWidget(parent), budget(budg) {

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	QHBoxLayout *buttons = new QHBoxLayout();
#ifdef QT_CHARTS_LIB
	buttons->addWidget(new QLabel(tr("Chart type:"), this));
	typeCombo = new QComboBox(this);
	typeCombo->addItem(tr("Pie Chart"));
	typeCombo->addItem(tr("Vertical Bar Chart"));
	typeCombo->addItem(tr("Horizontal Bar Chart"));
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
	view = new QChartView(chart, this);
	series = NULL;
#else
	scene = NULL;
	view = new QGraphicsView(this);
#endif
	view->setRenderHint(QPainter::Antialiasing, true);
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
	prevYearButton = new QPushButton(LOAD_ICON("eqz-previous-year"), "", settingsWidget);
	choicesLayout->addWidget(prevYearButton);
	prevMonthButton = new QPushButton(LOAD_ICON("eqz-previous-month"), "", settingsWidget);
	choicesLayout->addWidget(prevMonthButton);
	nextMonthButton = new QPushButton(LOAD_ICON("eqz-next-month"), "", settingsWidget);
	choicesLayout->addWidget(nextMonthButton);
	nextYearButton = new QPushButton(LOAD_ICON("eqz-next-year"), "", settingsWidget);
	choicesLayout->addWidget(nextYearButton);
	settingsLayout->addSpacing(12);
	settingsLayout->setStretchFactor(choicesLayout, 2);

	QHBoxLayout *typeLayout = new QHBoxLayout();
	settingsLayout->addLayout(typeLayout);
	typeLayout->addWidget(new QLabel(tr("Source:"), settingsWidget));
	sourceCombo = new QComboBox(settingsWidget);
	sourceCombo->setEditable(false);
	sourceCombo->addItem(tr("All Expenses, without subcategories"));
	sourceCombo->addItem(tr("All Expenses, with subcategories"));
	sourceCombo->addItem(tr("All Incomes, without subcategories"));
	sourceCombo->addItem(tr("All Incomes, with subcategories"));
	sourceCombo->addItem(tr("All Accounts"));
	for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
		Account *account = *it;
		sourceCombo->addItem(tr("Expenses: %1").arg(account->nameWithParent()));
	}
	for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
		Account *account = *it;
		sourceCombo->addItem(tr("Incomes: %1").arg(account->nameWithParent()));
	}
	typeLayout->addWidget(sourceCombo);
	typeLayout->setStretchFactor(sourceCombo, 2);
	accountCombo = new QComboBox(settingsWidget);
	accountCombo->setEditable(false);
	accountCombo->addItem(tr("All Accounts"), qVariantFromValue(NULL));
	for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
		AssetsAccount *account = *it;
		if(account != budget->balancingAccount && account->accountType() != ASSETS_TYPE_SECURITIES) {
			accountCombo->addItem(account->name(), qVariantFromValue((void*) account));
		}
	}
	typeLayout->addWidget(accountCombo);
	typeLayout->setStretchFactor(accountCombo, 1);
	settingsLayout->setStretchFactor(typeLayout, 1);

	current_account = NULL;

	layout->addWidget(settingsWidget);
	
	resetOptions();

#ifdef QT_CHARTS_LIB
	connect(themeCombo, SIGNAL(activated(int)), this, SLOT(themeChanged(int)));
	connect(typeCombo, SIGNAL(activated(int)), this, SLOT(typeChanged(int)));
#endif
	connect(sourceCombo, SIGNAL(activated(int)), this, SLOT(sourceChanged(int)));
	connect(accountCombo, SIGNAL(activated(int)), this, SLOT(updateDisplay()));
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
	QDate first_date;
	for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constBegin(); it != budget->transactions.constEnd(); ++it) {
		Transaction *trans = *it;
		if(trans->fromAccount()->type() != ACCOUNT_TYPE_ASSETS || trans->toAccount()->type() != ACCOUNT_TYPE_ASSETS) {
			first_date = trans->date();
			break;
		}
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
	accountCombo->setCurrentIndex(0);
	sourceCombo->setCurrentIndex(0);
	sourceChanged(0);
}

void CategoriesComparisonChart::sourceChanged(int index) {
	fromButton->setEnabled(index != 4);
	fromEdit->setEnabled(index != 4);
	accountCombo->setEnabled(index != 4);
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
	budget->goForwardBudgetMonths(from_date, to_date, -1);
	fromEdit->setDate(from_date);
	toEdit->setDate(to_date);
	fromEdit->blockSignals(false);
	toEdit->blockSignals(false);
	updateDisplay();
}
void CategoriesComparisonChart::nextMonth() {
	fromEdit->blockSignals(true);
	toEdit->blockSignals(true);
	budget->goForwardBudgetMonths(from_date, to_date, 1);
	fromEdit->setDate(from_date);
	toEdit->setDate(to_date);
	fromEdit->blockSignals(false);
	toEdit->blockSignals(false);
	updateDisplay();
}
void CategoriesComparisonChart::prevYear() {
	fromEdit->blockSignals(true);
	toEdit->blockSignals(true);
	budget->goForwardBudgetMonths(from_date, to_date, -12);
	fromEdit->setDate(from_date);
	toEdit->setDate(to_date);
	fromEdit->blockSignals(false);
	toEdit->blockSignals(false);
	updateDisplay();
}
void CategoriesComparisonChart::nextYear() {
	fromEdit->blockSignals(true);
	toEdit->blockSignals(true);
	budget->goForwardBudgetMonths(from_date, to_date, 12);
	fromEdit->setDate(from_date);
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
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
	fileDialog.setSupportedSchemes(QStringList("file"));
#endif
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

void CategoriesComparisonChart::print() {
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
QColor getColor(int index) {
#ifdef QT_CHARTS_LIB
	switch(index % 10) {
		case 7: {return Qt::red;}
		case 8: {return Qt::blue;}
		case 9: {return Qt::green;}
		case 0: {return Qt::darkRed;}
		case 1: {return Qt::darkGreen;}
		case 2: {return Qt::darkBlue;}
		case 3: {return Qt::darkCyan;}
		case 4: {return Qt::darkMagenta;}
		case 5: {return Qt::darkYellow;}
		case 6: {return Qt::gray;}
	}
	return Qt::black;
#else
	switch(index % 13) {
		case 5: return QRgb(0x1db0da);
		case 6: return QRgb(0x88d41e);
		case 7: return QRgb(0xff8e1a);
		case 8: return QRgb(0x398ca3);
		case 0: {return Qt::darkRed;}
		case 1: {return Qt::darkBlue;}
		case 2: {return Qt::darkGreen;}
		case 3: {return Qt::darkCyan;}
		case 4: {return Qt::darkMagenta;}
		case 9: {return Qt::gray;}
		case 10: {return Qt::red;}
		case 11: {return Qt::blue;}
		case 12: {return Qt::green;}		
	}
	return Qt::black;
#endif
}
QBrush getBrush(int index) {
	QBrush brush;
#ifdef QT_CHARTS_LIB
	switch(index / 7) {
		case 0: {brush.setStyle(Qt::SolidPattern); break;}
		case 1: {brush.setStyle(Qt::Dense3Pattern); break;}
		case 2: {brush.setStyle(Qt::Dense5Pattern); break;}
		case 3: {brush.setStyle(Qt::DiagCrossPattern); break;}
		case 4: {brush.setStyle(Qt::CrossPattern); break;}
		default: {}
	}
	brush.setColor(getColor(index % 7));
#else
	switch(index / 13) {
		case 0: {brush.setStyle(Qt::SolidPattern); break;}
		case 1: {brush.setStyle(Qt::Dense2Pattern); break;}
		case 2: {brush.setStyle(Qt::Dense4Pattern); break;}
		case 3: {brush.setStyle(Qt::DiagCrossPattern); break;}
		case 4: {brush.setStyle(Qt::HorPattern); break;}
		case 5: {brush.setStyle(Qt::VerPattern); break;}
		default: {}
	}
	brush.setColor(getColor(index % 13));
#endif
	return brush;
}
void CategoriesComparisonChart::updateDisplay() {

	if(!isVisible()) return;

	QMap<Account*, double> values;
	QMap<Account*, double> counts;
	QMap<QString, QString> desc_map;
	QMap<QString, double> desc_values;
	QMap<QString, double> desc_counts;
	double value_count = 0.0;
	double value = 0.0;
	QStringList desc_order; 

	current_account = NULL;
	AssetsAccount *current_assets = selectedAccount();
	
	bool include_subs = false;
	
	QString title_string = tr("Expenses");
	
	QDate first_date;
	bool first_date_reached = false;
	if(sourceCombo->currentIndex() == 4) {
		first_date_reached = true;
	} else if(fromButton->isChecked()) {
		first_date = from_date;
	} else {
		for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constBegin(); it != budget->transactions.constEnd(); ++it) {
			Transaction *trans = *it;
			if(trans->fromAccount()->type() != ACCOUNT_TYPE_ASSETS || trans->toAccount()->type() != ACCOUNT_TYPE_ASSETS) {
				first_date = trans->date();
				break;
			}
		}
		if(first_date.isNull()) first_date = QDate::currentDate();
		if(first_date > to_date) first_date = to_date;
	}

	AccountType type;
	switch(sourceCombo->currentIndex()) {
		case 1: {
			include_subs = true;
		}
		case 0: {
			type = ACCOUNT_TYPE_EXPENSES;
			if(current_assets) title_string = tr("Expenses, %1").arg(current_assets->name());
			for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
				CategoryAccount *account = *it;
				if(include_subs || !account->parentCategory()) {
					values[account] = 0.0;
					counts[account] = 0.0;
				}
			}
			break;
		}
		case 3: {
			include_subs = true;
		}
		case 2: {
			if(current_assets) title_string = tr("Incomes, %1").arg(current_assets->name());
			else title_string = tr("Incomes");
			type = ACCOUNT_TYPE_INCOMES;
			for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
				CategoryAccount *account = *it;
				if(include_subs || !account->parentCategory()) {
					values[account] = 0.0;
					counts[account] = 0.0;
				}
			}
			break;
		}
		case 4: {
			title_string = tr("Accounts");
			current_assets = NULL;
			type = ACCOUNT_TYPE_ASSETS;
			for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
				AssetsAccount *account = *it;
				if(account != budget->balancingAccount) {
					if(account->accountType() == ASSETS_TYPE_SECURITIES) {
						values[account] = 0.0;
					} else {
						values[account] = account->initialBalance();
						value += account->initialBalance();
					}
					counts[account] = 0.0;
				}
			}
			break;
		}
		default: {
			type = ACCOUNT_TYPE_EXPENSES;
			int i = sourceCombo->currentIndex() - 5;
			if(i < (int) budget->expensesAccounts.count()) current_account = budget->expensesAccounts.at(i);
			else current_account = budget->incomesAccounts.at(i - budget->expensesAccounts.count());
			if(current_account) {
				type = current_account->type();
				if(current_assets) {
					if(type == ACCOUNT_TYPE_EXPENSES) title_string = tr("Expenses, %2: %1").arg(current_account->nameWithParent()).arg(current_assets->name());
					else title_string = tr("Incomes, %2: %1").arg(current_account->nameWithParent()).arg(current_assets->name());
				} else {
					if(type == ACCOUNT_TYPE_EXPENSES) title_string = tr("Expenses: %1").arg(current_account->nameWithParent());
					else title_string = tr("Incomes: %1").arg(current_account->nameWithParent());
				}
				include_subs = !current_account->subCategories.isEmpty();
				if(include_subs) {
					values[current_account] = 0.0;
					counts[current_account] = 0.0;
					for(AccountList<CategoryAccount*>::const_iterator it = current_account->subCategories.constBegin(); it != current_account->subCategories.constEnd(); ++it) {
						Account *account = *it;
						values[account] = 0.0;
						counts[account] = 0.0;
					}
				} else {
					for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constEnd(); it != budget->transactions.constBegin();) {
						--it;
						Transaction *trans = *it;
						if(trans->date() <= to_date && (!current_assets || trans->relatesToAccount(current_assets))) {
							if(trans->date() < first_date) break;
							if((trans->fromAccount() == current_account || trans->toAccount() == current_account) && !desc_map.contains(trans->description().toLower())) {
								desc_map[trans->description().toLower()] = trans->description();
								desc_values[trans->description().toLower()] = 0.0;
								desc_counts[trans->description().toLower()] = 0.0;
							}
						}
					}
				}
			}
		}
	}
	
	Currency *currency = budget->defaultCurrency();
	if(current_assets) currency = current_assets->currency();

	for(TransactionList<Transaction*>::const_iterator it = budget->transactions.constBegin(); it != budget->transactions.constEnd(); ++it) {
		Transaction *trans = *it;
		if(!first_date_reached && trans->date() >= first_date) first_date_reached = true;
		else if(first_date_reached && trans->date() > to_date) break;
		if(first_date_reached && (!current_assets || trans->relatesToAccount(current_assets))) {
			if(current_account && !include_subs) {
				if(trans->fromAccount() == current_account) {
					if(type == ACCOUNT_TYPE_EXPENSES) {desc_values[trans->description().toLower()] -= trans->value(!current_assets); value -= trans->value(!current_assets);}
					else {desc_values[trans->description().toLower()] += trans->value(!current_assets); value += trans->value(!current_assets);}
					desc_counts[trans->description().toLower()] += trans->quantity();
					value_count += trans->quantity();
				} else if(trans->toAccount() == current_account) {
					if(type == ACCOUNT_TYPE_EXPENSES) {desc_values[trans->description().toLower()] += trans->value(!current_assets); value += trans->value(!current_assets);}
					else {desc_values[trans->description().toLower()] -= trans->value(!current_assets); value -= trans->value(!current_assets);}
					desc_counts[trans->description().toLower()] += trans->quantity();
					value_count += trans->quantity();
				}
			} else if(type == ACCOUNT_TYPE_ASSETS) {
				if(trans->fromAccount()->type() == ACCOUNT_TYPE_ASSETS && trans->fromAccount() != budget->balancingAccount) {
					if(((AssetsAccount*) trans->fromAccount())->accountType() != ASSETS_TYPE_SECURITIES) {
						values[trans->fromAccount()] -= trans->fromValue(!current_assets);
						value -= trans->fromValue(!current_assets);
					}
					if(trans->toAccount() != budget->balancingAccount) {
						counts[trans->fromAccount()] += trans->quantity();
						value_count += trans->quantity();
					}
				}
				if(trans->toAccount()->type() == ACCOUNT_TYPE_ASSETS && trans->toAccount() != budget->balancingAccount) {
					if(((AssetsAccount*) trans->toAccount())->accountType() != ASSETS_TYPE_SECURITIES) {
						values[trans->toAccount()] += trans->toValue(!current_assets);
						value += trans->toValue(!current_assets);
					}
					if(trans->fromAccount() != budget->balancingAccount) {
						counts[trans->toAccount()] += trans->quantity();
						value_count += trans->quantity();
					}
				}
			} else {
				Account *from_account = trans->fromAccount();
				if(!include_subs) from_account = from_account->topAccount();
				Account *to_account = trans->toAccount();
				if(!include_subs) to_account = to_account->topAccount();
				if((!current_account || to_account->topAccount() == current_account || from_account->topAccount() == current_account) && (to_account->type() == type || from_account->type() == type)) {
					if(from_account->type() == ACCOUNT_TYPE_EXPENSES) {
						values[from_account] -= trans->value(!current_assets);
						value -= trans->value(!current_assets);
						counts[from_account] += trans->quantity();
						value_count += trans->quantity();
					} else if(from_account->type() == ACCOUNT_TYPE_INCOMES) {
						values[from_account] += trans->value(!current_assets);
						value += trans->value(!current_assets);
						counts[from_account] += trans->quantity();
						value_count += trans->quantity();
					} else if(to_account->type() == ACCOUNT_TYPE_EXPENSES) {
						values[to_account] += trans->value(!current_assets);
						value += trans->value(!current_assets);
						counts[to_account] += trans->quantity();
						value_count += trans->quantity();
					} else if(to_account->type() == ACCOUNT_TYPE_INCOMES) {
						values[to_account] -= trans->value(!current_assets);
						value -= trans->value(!current_assets);
						counts[to_account] += trans->quantity();
						value_count += trans->quantity();
					}
				}
			}
		}
	}
	first_date_reached = false;
	int split_i = 0;
	Transaction *trans = NULL;
	for(ScheduledTransactionList<ScheduledTransaction*>::const_iterator it = budget->scheduledTransactions.constBegin(); it != budget->scheduledTransactions.constEnd();) {
		ScheduledTransaction *strans = *it;
		while(split_i == 0 && strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT && ((SplitTransaction*) strans->transaction())->count() == 0) {
			++it;
			if(it == budget->scheduledTransactions.constEnd()) break;
			strans = *it;
		}
		if(strans->transaction()->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT) {
			trans = ((SplitTransaction*) strans->transaction())->at(split_i);
			split_i++;
		} else {
			trans = (Transaction*) strans->transaction();
		}
		if(!first_date_reached && trans->date() >= first_date) first_date_reached = true;
		else if(first_date_reached && trans->date() > to_date) break;
		if(first_date_reached && (!current_assets || trans->relatesToAccount(current_assets))) {
			if(current_account && !include_subs) {
				if(trans->fromAccount() == current_account) {
					int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_date, to_date) : 1;
					if(type == ACCOUNT_TYPE_EXPENSES) {desc_values[trans->description().toLower()] -= trans->value(!current_assets) * count; value -= trans->value(!current_assets) * count;}
					else {desc_values[trans->description().toLower()] += trans->value(!current_assets) * count; value += trans->value(!current_assets) * count;}
					desc_counts[trans->description().toLower()] += count *  trans->quantity();
					value_count += count *  trans->quantity();
				} else if(trans->toAccount() == current_account) {
					int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_date, to_date) : 1;
					if(type == ACCOUNT_TYPE_EXPENSES) {desc_values[trans->description().toLower()] += trans->value(!current_assets) * count; value += trans->value(!current_assets) * count;}
					else {desc_values[trans->description().toLower()] -= trans->value(!current_assets) * count; value -= trans->value(!current_assets) * count;}
					desc_counts[trans->description().toLower()] += count *  trans->quantity();
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
						values[trans->fromAccount()] -= trans->fromValue(!current_assets) * count;
						value -= trans->fromValue(!current_assets) * count;
					}
				}
				if(trans->toAccount()->type() == ACCOUNT_TYPE_ASSETS && trans->toAccount() != budget->balancingAccount) {
					int count = strans->recurrence() ? strans->recurrence()->countOccurrences(to_date) : 1;
					if(trans->fromAccount() != budget->balancingAccount) {
						counts[trans->toAccount()] += count *  trans->quantity();
						value_count += count *  trans->quantity();
					}
					if(((AssetsAccount*) trans->toAccount())->accountType() != ASSETS_TYPE_SECURITIES) {
						values[trans->toAccount()] += trans->toValue(!current_assets) * count;
						value += trans->toValue(!current_assets) * count;
					}
				}
			} else {
				Account *from_account = trans->fromAccount();
				if(!include_subs) from_account = from_account->topAccount();
				Account *to_account = trans->toAccount();
				if(!include_subs) to_account = to_account->topAccount();
				if((!current_account || to_account->topAccount() == current_account || from_account->topAccount() == current_account) && (to_account->type() == type || from_account->type() == type)) {
					if(from_account->type() == ACCOUNT_TYPE_EXPENSES) {
						int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_date, to_date) : 1;
						counts[trans->fromAccount()] += count *  trans->quantity();
						values[trans->fromAccount()] -= trans->value(!current_assets) * count;
						value_count += count *  trans->quantity();
						value -= trans->value(!current_assets) * count;
					} else if(from_account->type() == ACCOUNT_TYPE_INCOMES) {
						int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_date, to_date) : 1;
						counts[trans->fromAccount()] += count;
						values[trans->fromAccount()] += trans->value(!current_assets) * count;
						value_count += count;
						value += trans->value(!current_assets) * count;
					} else if(to_account->type() == ACCOUNT_TYPE_EXPENSES) {
						int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_date, to_date) : 1;
						counts[trans->toAccount()] += count *  trans->quantity();
						values[trans->toAccount()] += trans->value(!current_assets) * count;
						value_count += count *  trans->quantity();
						value += trans->value(!current_assets) * count;
					} else if(to_account->type() == ACCOUNT_TYPE_INCOMES) {
						int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_date, to_date) : 1;
						counts[trans->toAccount()] += count;
						values[trans->toAccount()] -= trans->value(!current_assets) * count;
						value_count += count;
						value -= trans->value(!current_assets) * count;
					}
				}
			}
		}
		if(strans->transaction()->generaltype() != GENERAL_TRANSACTION_TYPE_SPLIT || split_i >= ((SplitTransaction*) strans->transaction())->count()) {
			++it;
			split_i = 0;
		}
	}

	if(type == ACCOUNT_TYPE_ASSETS) {
		for(SecurityList<Security*>::const_iterator it = budget->securities.constBegin(); it != budget->securities.constEnd(); ++it) {
			Security *security = *it;
			double val = security->value(to_date, true);
			values[security->account()] += val;
			value += val;
		}
	}

	/*int days = first_date.daysTo(to_date) + 1;
	double months = budget->monthsBetweenDates(first_date, to_date, true), years = budget->yearsBetweenDates(first_date, to_date, true);*/
	
	Account *account = NULL;
	QMap<QString, double>::iterator it_desc = desc_values.begin();
	QMap<QString, double>::iterator it_desc_end = desc_values.end();
	
	double remaining_value = 0.0;
	if(current_account && !include_subs) {
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
			desc_order.push_back(tr("Other descriptions", "Referring to the transaction description property (transaction title/generic article name)"));
			desc_values[tr("Other descriptions", "Referring to the transaction description property (transaction title/generic article name)")] = remaining_value;
		}
	}	

	if(!current_account && type == ACCOUNT_TYPE_ASSETS) {
		value = 0.0;
		for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
			account = *it;
			if(account != budget->balancingAccount) {
				if(values[account] > 0.0) value += values[account];
			}
		}
	}

#ifdef QT_CHARTS_LIB
	QPieSeries *pie_series = NULL;
	QAbstractBarSeries *bar_series = NULL;
	int chart_type = typeCombo->currentIndex() + 1;
	double maxvalue = 0.0, minvalue = 0.0;
	
	if(chart_type == 1) {
		pie_series = new QPieSeries();
	} else if(chart_type == 3) {
		bar_series = new QHorizontalBarSeries();
	} else {
		bar_series = new QBarSeries();
	}
	int account_index = 0;
	account = NULL;
	if(current_account) {
		if(include_subs) {
			if(account_index < current_account->subCategories.size()) account = current_account->subCategories.at(account_index);
		}
	} else if(type == ACCOUNT_TYPE_ASSETS) {
		if(account_index < budget->assetsAccounts.size()) {
			account = budget->assetsAccounts.at(account_index);
			if(account == budget->balancingAccount) {
				++account_index;
				if(account_index < budget->assetsAccounts.size()) {
					account = budget->assetsAccounts.at(account_index);
				}
			}
		}
	} else if(type == ACCOUNT_TYPE_EXPENSES) {
		if(account_index < budget->expensesAccounts.size()) account = budget->expensesAccounts.at(account_index);
	} else {
		if(account_index < budget->incomesAccounts.size()) account = budget->incomesAccounts.at(account_index);
	}
	int index = 0;
	bool show_legend = false;
	while(account || (current_account && index < desc_order.size())) {
		if(!current_account && include_subs) {
			while(account && ((CategoryAccount*) account)->subCategories.size() > 0) {
				if(values[account] != 0.0) break;
				++account_index;
				account = NULL;
				if(type == ACCOUNT_TYPE_EXPENSES && account_index < budget->expensesAccounts.size()) account = budget->expensesAccounts.at(account_index);
				else if(type == ACCOUNT_TYPE_INCOMES && account_index < budget->incomesAccounts.size()) account = budget->incomesAccounts.at(account_index);
			}
			if(!account) break;
		} else if(!current_account && type != ACCOUNT_TYPE_ASSETS) {
			while(account && account->topAccount() != account) {
				++account_index;
				account = NULL;
				if(type == ACCOUNT_TYPE_EXPENSES && account_index < budget->expensesAccounts.size()) account = budget->expensesAccounts.at(account_index);
				else if(type == ACCOUNT_TYPE_INCOMES && account_index < budget->incomesAccounts.size()) account = budget->incomesAccounts.at(account_index);
			}
			if(!account) break;
		}

		QString legend_string;
		double legend_value = 0.0;
		double current_value = 0.0;
		if(current_account && !include_subs) {
			if(desc_order[index].isEmpty()) {
				legend_string = tr("No description", "Referring to the transaction description property (transaction title/generic article name)");
			} else {
				legend_string = desc_map[desc_order[index]];
			}
			current_value = desc_values[desc_order[index]];
		} else {
			if(current_account) legend_string = account->name();
			else legend_string = account->nameWithParent();
			current_value = values[account];
		}
		legend_value = (current_value * 100) / value;
		int deci = 0;
		if(legend_value < 10.0 && legend_value > -10.0) {
			legend_value = round(legend_value * 10.0) / 10.0;
			deci = 1;
		} else {
			legend_value = round(legend_value);
		}

		if(chart_type == 1) {
			if(current_value >= 0.01) {
				QPieSlice *slice = pie_series->append(QString("%1 (%2%)").arg(legend_string).arg(currency->formatValue(legend_value, deci, false)), current_value);
				if(legend_value >= 8.0) {
					slice->setLabelVisible(true);
				} else {
					show_legend = true;
				}
			}
		} else {
			if(current_value >= 0.01 || current_value <= -0.01 || (value < 0.01 && value > -0.01)) {
				QBarSet *set = new QBarSet(QString("%1 (%2%)").arg(legend_string).arg(currency->formatValue(legend_value, deci, false)));
				set->append(current_value);
				if(current_value > maxvalue) maxvalue = current_value;
				if(current_value < minvalue) minvalue = current_value;
				bar_series->append(set);
				show_legend = true;
			}
		}
		
		++account_index;
		if(current_account) {
			if(include_subs && account != current_account) {
				if(account_index < current_account->subCategories.size()) account = current_account->subCategories.at(account_index);
				else if(values[current_account] != 0.0) account = current_account;
				else account = NULL;
			} else {
				account = NULL;
			}
		} else if(type == ACCOUNT_TYPE_ASSETS) {
			account = NULL;
			if(account_index < budget->assetsAccounts.size()) {
				account = budget->assetsAccounts.at(account_index);
				if(account == budget->balancingAccount) {
					++account_index;
					if(account_index < budget->assetsAccounts.size()) {
						account = budget->assetsAccounts.at(account_index);
					}
				}
			}
		} else if(type == ACCOUNT_TYPE_EXPENSES) {
			account = NULL;
			if(account_index < budget->expensesAccounts.size()) account = budget->expensesAccounts.at(account_index);
		} else {
			account = NULL;
			if(account_index < budget->incomesAccounts.size()) account = budget->incomesAccounts.at(account_index);
		}
		index++;
	}
	
	if(bar_series && bar_series->count() >= 5) {
		QList<QBarSet*> bar_sets = bar_series->barSets();
		for(int i = 5; i < bar_sets.count(); i++) {
			bar_sets.at(i)->setBrush(getBrush(i - 5));
		}
	}
	
	chart->removeAllSeries();
	
	foreach(QAbstractAxis* axis, chart->axes()) {
		chart->removeAxis(axis);
	}
	
	if(chart_type == 1) {
		series = pie_series;
		connect(pie_series, SIGNAL(hovered(QPieSlice*, bool)), this, SLOT(sliceHovered(QPieSlice*, bool)));
		connect(pie_series, SIGNAL(clicked(QPieSlice*)), this, SLOT(sliceClicked(QPieSlice*)));
		chart->addSeries(series);
	} else {
		series = bar_series;
		bar_series->setBarWidth(1.0);
		chart->addSeries(series);
		QBarCategoryAxis *b_axis = new QBarCategoryAxis();
		b_axis->append("");

		int y_lines = 5, y_minor = 0;
		calculate_minmax_lines(maxvalue, minvalue, y_lines, y_minor);
		QValueAxis *v_axis = new QValueAxis();
		v_axis->setRange(minvalue, maxvalue);
		v_axis->setTickCount(y_lines + 1);
		v_axis->setMinorTickCount(y_minor);
		if(type == 2 || (maxvalue - minvalue) >= 50.0) v_axis->setLabelFormat(QString("%.0f"));
		else v_axis->setLabelFormat(QString("%.%1f").arg(QString::number(currency->fractionalDigits())));
		
		if(type == ACCOUNT_TYPE_ASSETS) v_axis->setTitleText(tr("Value") + QString(" (%1)").arg(currency->symbol(true)));
		else if(type == ACCOUNT_TYPE_INCOMES) v_axis->setTitleText(tr("Income") + QString(" (%1)").arg(currency->symbol(true)));
		else v_axis->setTitleText(tr("Cost") + QString(" (%1)").arg(currency->symbol(true)));

		if(chart_type == 3) {
			chart->setAxisY(b_axis, series);
			chart->setAxisX(v_axis, series);
		} else {
			chart->setAxisX(b_axis, series);
			chart->setAxisY(v_axis, series);
		}
	}
	
	chart->setTitle(QString("<div align=\"center\"><font size=\"+2\"><b>%1</b></font></div>").arg(title_string));
	if(show_legend) {
		chart->legend()->setAlignment(Qt::AlignRight);
		chart->legend()->show();
		foreach(QLegendMarker* marker, chart->legend()->markers()) {
			QObject::connect(marker, SIGNAL(clicked()), this, SLOT(legendClicked()));
		}
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
	int margin = 15 + fh;
	
	QGraphicsTextItem *title_text = new QGraphicsTextItem();
	title_text->setHtml(QString("<div align=\"center\"><font size=\"+2\"><b>%1</b></font></div>").arg(title_string));
	title_text->setDefaultTextColor(Qt::black);
	title_text->setFont(legend_font);
	title_text->setPos(view->height() / 2 - title_text->boundingRect().width() / 2, margin);
	scene->addItem(title_text);
	
	int diameter = view->height() - margin * 3 - title_text->boundingRect().height();
	int legend_x = diameter + margin * 2;
	int chart_y = margin * 2 + title_text->boundingRect().height();
	int legend_y = chart_y;
	
	int account_index = 0;
	account = NULL;
	if(current_account) {
		if(include_subs) {
			if(account_index < current_account->subCategories.size()) account = current_account->subCategories.at(account_index);
		}
	} else if(type == ACCOUNT_TYPE_ASSETS) {
		if(account_index < budget->assetsAccounts.size()) {
			account = budget->assetsAccounts.at(account_index);
			if(account == budget->balancingAccount) {
				++account_index;
				if(account_index < budget->assetsAccounts.size()) {
					account = budget->assetsAccounts.at(account_index);
				}
			}
		}
	} else if(type == ACCOUNT_TYPE_EXPENSES) {
		if(account_index < budget->expensesAccounts.size()) account = budget->expensesAccounts.at(account_index);
	} else {
		if(account_index < budget->incomesAccounts.size()) account = budget->incomesAccounts.at(account_index);
	}
	
	int index = 0, lcount = 0;
	int text_width = 0;
	while(account || (current_account && index < desc_order.size())) {
		if(!current_account && include_subs) {
			while(account && ((CategoryAccount*) account)->subCategories.size() > 0) {
				if(values[account] != 0.0) break;
				++account_index;
				account = NULL;
				if(type == ACCOUNT_TYPE_EXPENSES && account_index < budget->expensesAccounts.size()) account = budget->expensesAccounts.at(account_index);
				else if(type == ACCOUNT_TYPE_EXPENSES && account_index < budget->incomesAccounts.size()) account = budget->incomesAccounts.at(account_index);
			}
			if(!account) break;
		} else if(!current_account && type != ACCOUNT_TYPE_ASSETS) {
			while(account && account->topAccount() != account) {
				++account_index;
				account = NULL;
				if(type == ACCOUNT_TYPE_EXPENSES && account_index < budget->expensesAccounts.size()) account = budget->expensesAccounts.at(account_index);
				else if(type == ACCOUNT_TYPE_EXPENSES && account_index < budget->incomesAccounts.size()) account = budget->incomesAccounts.at(account_index);
			}
			if(!account) break;
		}
		QString legend_string;
		double legend_value = 0.0;
		if(current_account && !include_subs) {
			if(desc_order[index].isEmpty()) {
				legend_string = tr("No description", "Referring to the transaction description property (transaction title/generic article name)");
			} else {
				legend_string = desc_map[desc_order[index]];
			}
			legend_value = desc_values[desc_order[index]];
		} else {
			if(current_account) legend_string = account->name();
			else legend_string = account->nameWithParent();
			legend_value = values[account];
		}
		if(!is_zero(legend_value) && legend_value > 0.0) {
			legend_value = (legend_value * 100) / value;
			int deci = 0;
			if(legend_value < 10.0 && legend_value > -10.0) {
				legend_value = round(legend_value * 10.0) / 10.0;
				deci = 1;
			} else {
				legend_value = round(legend_value);
			}
			QGraphicsSimpleTextItem *legend_text = new QGraphicsSimpleTextItem(QString("%1 (%2%)").arg(legend_string).arg(currency->formatValue(legend_value, deci, false)));
			if(legend_text->boundingRect().width() > text_width) text_width = legend_text->boundingRect().width();
			legend_text->setFont(legend_font);
			legend_text->setBrush(Qt::black);
			legend_text->setPos(legend_x + 10 + fh + 5, legend_y + 10 + (fh + 5) * lcount);
			scene->addItem(legend_text);
			lcount++;
		}
		
		++account_index;
		if(current_account) {
			if(include_subs && account != current_account) {
				if(account_index < current_account->subCategories.size()) account = current_account->subCategories.at(account_index);
				else if(values[current_account] != 0.0) account = current_account;
				else account = NULL;
			} else {
				account = NULL;
			}
		} else if(type == ACCOUNT_TYPE_ASSETS) {
			account = NULL;
			if(account_index < budget->assetsAccounts.size()) {
				account = budget->assetsAccounts.at(account_index);
				if(account == budget->balancingAccount) {
					++account_index;
					if(account_index < budget->assetsAccounts.size()) {
						account = budget->assetsAccounts.at(account_index);
					}
				}
			}
		} else if(type == ACCOUNT_TYPE_EXPENSES) {
			account = NULL;
			if(account_index < budget->expensesAccounts.size()) account = budget->expensesAccounts.at(account_index);
		} else {
			account = NULL;
			if(account_index < budget->incomesAccounts.size()) account = budget->incomesAccounts.at(account_index);
		}
		index++;
	}


	account_index = 0;
	account = NULL;
	if(current_account) {
		if(include_subs) {
			if(account_index < current_account->subCategories.size()) account = current_account->subCategories.at(account_index);
		}
	} else if(type == ACCOUNT_TYPE_ASSETS) {
		if(account_index < budget->assetsAccounts.size()) {
			account = budget->assetsAccounts.at(account_index);
			if(account == budget->balancingAccount) {
				++account_index;
				if(account_index < budget->assetsAccounts.size()) {
					account = budget->assetsAccounts.at(account_index);
				}
			}
		}
	} else if(type == ACCOUNT_TYPE_EXPENSES) {
		if(account_index < budget->expensesAccounts.size()) account = budget->expensesAccounts.at(account_index);
	} else {
		if(account_index < budget->incomesAccounts.size()) account = budget->incomesAccounts.at(account_index);
	}
	index = 0;
	lcount = 0;
	double current_value = 0.0, current_value_1 = 0.0;
	int prev_end = 0;
	while(account || (current_account && index < desc_order.size())) {
		if(!current_account && include_subs) {
			while(account && ((CategoryAccount*) account)->subCategories.size() > 0) {
				if(values[account] != 0.0) break;
				++account_index;
				account = NULL;
				if(type == ACCOUNT_TYPE_EXPENSES && account_index < budget->expensesAccounts.size()) account = budget->expensesAccounts.at(account_index);
				else if(type == ACCOUNT_TYPE_EXPENSES && account_index < budget->incomesAccounts.size()) account = budget->incomesAccounts.at(account_index);
			}
			if(!account) break;
		} else if(!current_account && type != ACCOUNT_TYPE_ASSETS) {
			while(account && account->topAccount() != account) {
				++account_index;
				account = NULL;
				if(type == ACCOUNT_TYPE_EXPENSES && account_index < budget->expensesAccounts.size()) account = budget->expensesAccounts.at(account_index);
				else if(type == ACCOUNT_TYPE_EXPENSES && account_index < budget->incomesAccounts.size()) account = budget->incomesAccounts.at(account_index);
			}
			if(!account) break;
		}
		if(current_account && !include_subs) current_value_1 = desc_values[desc_order[index]];
		else current_value_1 = values[account];
		if(!is_zero(current_value_1) && current_value_1 > 0.0) {
			current_value += current_value_1;
			int next_end = (int) lround((current_value * 360 * 16) / value);
			int length = (next_end - prev_end);
			QGraphicsEllipseItem *ellipse = new QGraphicsEllipseItem(-diameter/ 2.0, -diameter/ 2.0, diameter, diameter);
			ellipse->setStartAngle(prev_end);
			ellipse->setSpanAngle(length);
			prev_end = next_end;
			ellipse->setPen(Qt::NoPen);
			ellipse->setBrush(getBrush(index));
			ellipse->setPos(diameter / 2 + margin, diameter / 2 + chart_y);
			scene->addItem(ellipse);
			QGraphicsRectItem *legend_box = new QGraphicsRectItem(legend_x + 10, chart_y + 10 + (fh + 5) * lcount, fh, fh);
			legend_box->setPen(QPen(Qt::black));
			legend_box->setBrush(getBrush(index));
			scene->addItem(legend_box);
			lcount++;
		}
		++account_index;
		if(current_account) {
			if(include_subs && account != current_account) {
				if(account_index < current_account->subCategories.size()) account = current_account->subCategories.at(account_index);
				else if(values[current_account] != 0.0) account = current_account;
				else account = NULL;
			} else {
				account = NULL;
			}
		} else if(type == ACCOUNT_TYPE_ASSETS) {
			account = NULL;
			if(account_index < budget->assetsAccounts.size()) {
				account = budget->assetsAccounts.at(account_index);
				if(account == budget->balancingAccount) {
					++account_index;
					if(account_index < budget->assetsAccounts.size()) {
						account = budget->assetsAccounts.at(account_index);
					}
				}
			}
		} else if(type == ACCOUNT_TYPE_EXPENSES) {
			account = NULL;
			if(account_index < budget->expensesAccounts.size()) account = budget->expensesAccounts.at(account_index);
		} else {
			account = NULL;
			if(account_index < budget->incomesAccounts.size()) account = budget->incomesAccounts.at(account_index);
		}
		index++;
	}

	if(lcount > 0) {
		QGraphicsRectItem *legend_outline = new QGraphicsRectItem(legend_x, legend_y, 10 + fh + 5 + text_width + 10, 10 + ((fh + 5) * lcount) + 5);
		legend_outline->setPen(QPen(Qt::black));
		scene->addItem(legend_outline);
	}

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
}

#ifndef QT_CHARTS_LIB
void CategoriesComparisonChart::resizeEvent(QResizeEvent *e) {
	QWidget::resizeEvent(e);
	if(scene) {
		view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
	}
}
#endif

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
	sourceCombo->addItem(tr("All Expenses, without subcategories"));
	sourceCombo->addItem(tr("All Expenses, with subcategories"));
	sourceCombo->addItem(tr("All Incomes, without subcategories"));
	sourceCombo->addItem(tr("All Incomes, with subcategories"));
	sourceCombo->addItem(tr("All Accounts"));
	int i = 3;
	for(AccountList<ExpensesAccount*>::const_iterator it = budget->expensesAccounts.constBegin(); it != budget->expensesAccounts.constEnd(); ++it) {
		Account *account = *it;
		sourceCombo->addItem(tr("Expenses: %1").arg(account->nameWithParent()));
		if(account == current_account) curindex = i;
		i++;
	}
	for(AccountList<IncomesAccount*>::const_iterator it = budget->incomesAccounts.constBegin(); it != budget->incomesAccounts.constEnd(); ++it) {
		Account *account = *it;
		sourceCombo->addItem(tr("Incomes: %1").arg(account->nameWithParent()));
		if(account == current_account) curindex = i;
		i++;
	}
	if(curindex < sourceCombo->count()) sourceCombo->setCurrentIndex(curindex);
	sourceCombo->blockSignals(false);
	accountCombo->blockSignals(true);
	AssetsAccount *current_assets = selectedAccount();
	accountCombo->clear();
	accountCombo->addItem(tr("All Accounts"), qVariantFromValue(NULL));
	for(AccountList<AssetsAccount*>::const_iterator it = budget->assetsAccounts.constBegin(); it != budget->assetsAccounts.constEnd(); ++it) {
		AssetsAccount *account = *it;
		if(account != budget->balancingAccount && account->accountType() != ASSETS_TYPE_SECURITIES) {
			accountCombo->addItem(account->name(), qVariantFromValue((void*) account));
		}
	}
	int index = 0;
	if(current_assets) index = accountCombo->findData(qVariantFromValue((void*) current_assets));
	if(index >= 0) accountCombo->setCurrentIndex(index);
	else accountCombo->setCurrentIndex(0);
	accountCombo->blockSignals(false);
	updateDisplay();
}
AssetsAccount *CategoriesComparisonChart::selectedAccount() {
	if(!accountCombo->currentData().isValid()) return NULL;
	return (AssetsAccount*) accountCombo->currentData().value<void*>();
}

#ifdef QT_CHARTS_LIB
void CategoriesComparisonChart::themeChanged(int index) {
	QChart::ChartTheme theme = (QChart::ChartTheme) themeCombo->itemData(index).toInt();
	chart->setTheme(theme);
	QSettings settings;
	settings.beginGroup("GeneralOptions");
	settings.setValue("chartTheme", theme);
	settings.endGroup();
	if(typeCombo->currentIndex() > 0) updateDisplay();
}
void CategoriesComparisonChart::typeChanged(int) {
	updateDisplay();
}
void CategoriesComparisonChart::sliceHovered(QPieSlice *slice, bool state) {
	if(!slice || slice->isExploded() || slice->percentage() >= 0.08) return;
	slice->setLabelVisible(state);
}
void CategoriesComparisonChart::sliceClicked(QPieSlice *slice) {
	if(!slice) return;
	if(slice->isExploded()) {
		slice->setLabelVisible(slice->percentage() >= 0.08);
		slice->setExploded(false);
	} else {
		slice->setLabelVisible(true);
		slice->setExploded(true);
	}
}
void CategoriesComparisonChart::legendClicked() {
	if(typeCombo->currentIndex() == 0) {
		QPieLegendMarker* marker = qobject_cast<QPieLegendMarker*>(sender());
		sliceClicked(marker->slice());
	} else {
		QBarLegendMarker* marker = qobject_cast<QBarLegendMarker*>(sender());
		if(marker->series()->count() == 1) updateDisplay();
		else marker->series()->remove(marker->barset());
	}
}
#endif
