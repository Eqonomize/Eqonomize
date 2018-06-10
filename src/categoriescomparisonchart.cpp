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
	point_label = NULL;
	buttons->addWidget(new QLabel(tr("Chart type:"), this));
	typeCombo = new QComboBox(this);
	typeCombo->addItem(tr("Pie Chart"));
	typeCombo->addItem(tr("Bar Chart"));
	buttons->addWidget(typeCombo);
	buttons->addWidget(new QLabel(tr("Theme:"), this));
	themeCombo = new QComboBox(this);
	themeCombo->addItem(tr("Default"), -1);
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
	settings.beginGroup("CategoriesComparisonChart");
	int theme = settings.value("theme", -1).toInt();
	int index = themeCombo->findData(qVariantFromValue(theme));
	if(index < 0) index = 0;
	themeCombo->setCurrentIndex(index);
	typeCombo->setCurrentIndex(0);
	chart->setTheme(theme >= 0 ? (QChart::ChartTheme) theme : QChart::ChartThemeBlueNcs);
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
	fromEdit->blockSignals(true);
	toEdit->blockSignals(true);
	fromEdit->setDate(from_date);
	toEdit->setDate(to_date);
	fromEdit->blockSignals(false);
	toEdit->blockSignals(false);
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
QColor getPieColor(int index, int total) {
	if(total > 8) {
		switch(index % 9) {
			case 0: return QRgb(0x4d004b);
			case 1: return QRgb(0x810f7c);
			case 2: return QRgb(0x88419d);
			case 3: return QRgb(0x8c6bb1);
			case 4: return QRgb(0x8c96c6);
			case 5: return QRgb(0x9ebcda);
			case 6: return QRgb(0xbfd3e6);
			case 7: return QRgb(0xe0ecf4);
			case 8: return QRgb(0xf7fcfd);
		}
	} else if(total == 8) {
		switch(index % 8) {
			case 0: return QRgb(0x6e016b);
			case 1: return QRgb(0x88419d);
			case 2: return QRgb(0x8c6bb1);
			case 3: return QRgb(0x8c96c6);
			case 4: return QRgb(0x9ebcda);
			case 5: return QRgb(0xbfd3e6);
			case 6: return QRgb(0xe0ecf4);
			case 7: return QRgb(0xf7fcfd);
		}
	} else if(total == 7) {
		switch(index % 7) {
			case 0: return QRgb(0x6e016b);
			case 1: return QRgb(0x88419d);
			case 2: return QRgb(0x8c6bb1);
			case 3: return QRgb(0x8c96c6);
			case 4: return QRgb(0x9ebcda);
			case 5: return QRgb(0xbfd3e6);
			case 6: return QRgb(0xedf8fb);
		}
	} else if(total == 6) {
		switch(index % 6) {
			case 0: return QRgb(0x810f7c);
			case 1: return QRgb(0x8856a7);
			case 2: return QRgb(0x8c96c6);
			case 3: return QRgb(0x9ebcda);
			case 4: return QRgb(0xbfd3e6);
			case 5: return QRgb(0xedf8fb);
		}
	} else if(total == 5) {
		switch(index % 5) {
			case 0: return QRgb(0x810f7c);
			case 1: return QRgb(0x8856a7);
			case 2: return QRgb(0x8c96c6);
			case 3: return QRgb(0xb3cde3);
			case 4: return QRgb(0xedf8fb);
		}
	} else if(total == 4) {
		switch(index % 4) {
			case 0: return QRgb(0x88419d);
			case 1: return QRgb(0x8c96c6);
			case 2: return QRgb(0xb3cde3);
			case 3: return QRgb(0xedf8fb);
		}
	} else {
		switch(index % 3) {
			case 0: return QRgb(0x8856a7);
			case 1: return QRgb(0x9ebcda);
			case 2: return QRgb(0xe0ecf4);
		}
	}
	return Qt::black;
}
extern QBrush getBarBrush(int index, int total);
QBrush getPieBrush(int index, int total) {
	QBrush brush;
	if(total > 9) total = 9;
	else if(total <= 0) total = 1;
	switch(index / total) {
		case 0: {brush.setStyle(Qt::SolidPattern); break;}
		case 1: {brush.setStyle(Qt::Dense3Pattern); break;}
		case 2: {brush.setStyle(Qt::DiagCrossPattern); break;}
		case 3: {brush.setStyle(Qt::HorPattern); break;}
		case 4: {brush.setStyle(Qt::VerPattern); break;}
		default: {}
	}
	brush.setColor(getPieColor(index, total));
	return brush;
}

void CategoriesComparisonChart::updateDisplay() {

	if(!isVisible()) return;

	QMap<Account*, double> values;
	QMap<Account*, double> counts;
	QMap<QString, QString> desc_map;
	QMap<QString, double> desc_values;
	QMap<QString, double> desc_counts;
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
					if(type == ACCOUNT_TYPE_EXPENSES) {desc_values[trans->description().toLower()] -= trans->value(!current_assets);}
					else {desc_values[trans->description().toLower()] += trans->value(!current_assets);}
					desc_counts[trans->description().toLower()] += trans->quantity();
				} else if(trans->toAccount() == current_account) {
					if(type == ACCOUNT_TYPE_EXPENSES) {desc_values[trans->description().toLower()] += trans->value(!current_assets);}
					else {desc_values[trans->description().toLower()] -= trans->value(!current_assets);}
					desc_counts[trans->description().toLower()] += trans->quantity();
				}
			} else if(type == ACCOUNT_TYPE_ASSETS) {
				if(trans->fromAccount()->type() == ACCOUNT_TYPE_ASSETS && trans->fromAccount() != budget->balancingAccount) {
					if(((AssetsAccount*) trans->fromAccount())->accountType() != ASSETS_TYPE_SECURITIES) {
						values[trans->fromAccount()] -= trans->fromValue(!current_assets);
					}
					if(trans->toAccount() != budget->balancingAccount) {
						counts[trans->fromAccount()] += trans->quantity();
					}
				}
				if(trans->toAccount()->type() == ACCOUNT_TYPE_ASSETS && trans->toAccount() != budget->balancingAccount) {
					if(((AssetsAccount*) trans->toAccount())->accountType() != ASSETS_TYPE_SECURITIES) {
						values[trans->toAccount()] += trans->toValue(!current_assets);
					}
					if(trans->fromAccount() != budget->balancingAccount) {
						counts[trans->toAccount()] += trans->quantity();
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
						counts[from_account] += trans->quantity();
					} else if(from_account->type() == ACCOUNT_TYPE_INCOMES) {
						values[from_account] += trans->value(!current_assets);
						counts[from_account] += trans->quantity();
					} else if(to_account->type() == ACCOUNT_TYPE_EXPENSES) {
						values[to_account] += trans->value(!current_assets);
						counts[to_account] += trans->quantity();
					} else if(to_account->type() == ACCOUNT_TYPE_INCOMES) {
						values[to_account] -= trans->value(!current_assets);
						counts[to_account] += trans->quantity();
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
					if(type == ACCOUNT_TYPE_EXPENSES) {desc_values[trans->description().toLower()] -= trans->value(!current_assets) * count;}
					else {desc_values[trans->description().toLower()] += trans->value(!current_assets) * count;}
					desc_counts[trans->description().toLower()] += count *  trans->quantity();
				} else if(trans->toAccount() == current_account) {
					int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_date, to_date) : 1;
					if(type == ACCOUNT_TYPE_EXPENSES) {desc_values[trans->description().toLower()] += trans->value(!current_assets) * count;}
					else {desc_values[trans->description().toLower()] -= trans->value(!current_assets) * count;}
					desc_counts[trans->description().toLower()] += count *  trans->quantity();
				}
			} else if(type == ACCOUNT_TYPE_ASSETS) {
				if(trans->fromAccount()->type() == ACCOUNT_TYPE_ASSETS && trans->fromAccount() != budget->balancingAccount) {
					int count = strans->recurrence() ? strans->recurrence()->countOccurrences(to_date) : 1;
					if(trans->toAccount() != budget->balancingAccount) {
						counts[trans->fromAccount()] += count *  trans->quantity();
					}
					if(((AssetsAccount*) trans->fromAccount())->accountType() != ASSETS_TYPE_SECURITIES) {
						values[trans->fromAccount()] -= trans->fromValue(!current_assets) * count;
					}
				}
				if(trans->toAccount()->type() == ACCOUNT_TYPE_ASSETS && trans->toAccount() != budget->balancingAccount) {
					int count = strans->recurrence() ? strans->recurrence()->countOccurrences(to_date) : 1;
					if(trans->fromAccount() != budget->balancingAccount) {
						counts[trans->toAccount()] += count *  trans->quantity();
					}
					if(((AssetsAccount*) trans->toAccount())->accountType() != ASSETS_TYPE_SECURITIES) {
						values[trans->toAccount()] += trans->toValue(!current_assets) * count;
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
					} else if(from_account->type() == ACCOUNT_TYPE_INCOMES) {
						int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_date, to_date) : 1;
						counts[trans->fromAccount()] += count;
						values[trans->fromAccount()] += trans->value(!current_assets) * count;
					} else if(to_account->type() == ACCOUNT_TYPE_EXPENSES) {
						int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_date, to_date) : 1;
						counts[trans->toAccount()] += count *  trans->quantity();
						values[trans->toAccount()] += trans->value(!current_assets) * count;
					} else if(to_account->type() == ACCOUNT_TYPE_INCOMES) {
						int count = strans->recurrence() ? strans->recurrence()->countOccurrences(first_date, to_date) : 1;
						counts[trans->toAccount()] += count;
						values[trans->toAccount()] -= trans->value(!current_assets) * count;
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
		}
	}

	/*int days = first_date.daysTo(to_date) + 1;
	double months = budget->monthsBetweenDates(first_date, to_date, true), years = budget->yearsBetweenDates(first_date, to_date, true);*/

#ifdef QT_CHARTS_LIB
	int chart_type = typeCombo->currentIndex() + 1;
#else
	int chart_type = 1;
#endif
	
	Account *account = NULL;
	QMap<QString, double>::iterator it_desc = desc_values.begin();
	QMap<QString, double>::iterator it_desc_end = desc_values.end();
	
	double value = 0.0;
	double vmax = 0.0;
	double remaining_value = 0.0;
	if(current_account && !include_subs) {
		while(it_desc != it_desc_end) {
			if(it_desc.value() >= 0.0) value += it_desc.value();
			if(it_desc.value() >= 0.01 || (chart_type != 1 && it_desc.value() <= -0.01)) {
				bool b = false;
				if(abs(it_desc.value()) > vmax) vmax = abs(it_desc.value());
				for(int i = 0; i < desc_order.count(); i++) {
					if(it_desc.value() > desc_values[desc_order.at(i)]) {
						desc_order.insert(i, it_desc.key());
						
						b = true;
						break;
					}
				}
				if(!b) desc_order.push_back(it_desc.key());
			}
			++it_desc;
		}
	}
	
	if((chart_type == 1 && desc_order.size() > 9) || (chart_type == 2 && desc_order.size() > 12)) {
		while((chart_type == 1 && desc_order.size() > 8) || (chart_type == 2 && desc_order.size() > 11)) {
			remaining_value += desc_values[desc_order.back()];
			desc_order.pop_back();
		}
	} else if(desc_order.size() > 5 && chart_type == 1) {
		int n = 0;
		for(int i = desc_order.size() - 1; i > 0; i--) {
			double v = abs(desc_values[desc_order[i]]);
			if((v / value) < 0.01 && v < (vmax / 10)) {
				n++;
			}
		}
		if(n > 1) {
			for(int i = desc_order.size() - 1; i > 0; i--) {
				double v = abs(desc_values[desc_order[i]]);
				if((v / value) < 0.01 && v < (vmax / 10)) {
					remaining_value += desc_values[desc_order[i]];
					desc_order.removeAt(i);
				}
			}
		}
	}

	if(desc_order.count() > 0 && remaining_value != 0.0) {
		desc_order.push_back(tr("Other descriptions", "Referring to the transaction description property (transaction title/generic article name)"));
		desc_values[tr("Other descriptions", "Referring to the transaction description property (transaction title/generic article name)")] = remaining_value;
		desc_map[desc_order.last()] = desc_order.last();
	}

	QList<Account*> account_order;
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
	while(account) {
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
		if(values[account] > 0.0) value += values[account];
		if(values[account] >= 0.01 || (chart_type != 1 && values[account] <= -0.01)) {
			bool b = false;
			if(abs(values[account]) > vmax) vmax = abs(values[account]);
			for(int i = 0; i < account_order.count(); i++) {
				if(values[account] > values[account_order.at(i)]) {
					account_order.insert(i, account);
					
					b = true;
					break;
				}
			}
			if(!b) account_order.push_back(account);
		}
		++account_index;
		if(current_account) {
			if(include_subs && account != current_account) {
				if(account_index < current_account->subCategories.size()) account = current_account->subCategories.at(account_index);
				else if(values[current_account] >= 0.01 || values[current_account] <= -0.01) account = current_account;
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
	}

	if(account_order.size() > 5 && chart_type == 1) {
		if(account_order.size() > 9) {
			values[NULL] = 0.0;
			while(account_order.size() > 8) {
				values[NULL] += values[account_order.back()];
				account_order.pop_back();
			}
			account_order.push_back(NULL);
		} else {
			int n = 0;
			for(int i = account_order.size() - 1; i > 0; i--) {
				double v = abs(values[account_order[i]]);
				if((v / value) < 0.01 && v < (vmax / 10)) {
					n++;
				}
			}
			if(n > 1) {
				values[NULL] = 0.0;
				for(int i = account_order.size() - 1; i > 0; i--) {
					double v = abs(values[account_order[i]]);
					if((v / value) < 0.01 && v < (vmax / 10)) {
						values[NULL] += values[account_order[i]];
						account_order.removeAt(i);
					}
				}
				account_order.push_back(NULL);
			}
		}
	}

#ifdef QT_CHARTS_LIB

	int theme = themeCombo->currentData().toInt();

	QPieSeries *pie_series = NULL;
	QAbstractBarSeries *bar_series = NULL;
	double maxvalue = 0.0, minvalue = 0.0;
	
	if(chart_type == 1) {
		pie_series = new QPieSeries();
	} else if(chart_type == 3) {
		bar_series = new QBarSeries();
	} else {
		bar_series = new QHorizontalBarSeries();
	}

	QBarSet *bar_set = NULL;
	QBarCategoryAxis *b_axis = NULL;
	if(chart_type != 1) {
		bar_set = new QBarSet("");
		if(theme < 0) bar_set->setBrush(getBarBrush(0, 1));
		b_axis = new QBarCategoryAxis();
	}

	account = NULL;
	int index = 0;
	bool show_legend = false;
	if(chart_type == 2) {
		index = account_order.size() - 1;
		if(index < 0) index = desc_order.size() - 1;
	}
	while((chart_type == 2 && index >= 0) || (chart_type != 2 && (index < account_order.count() || (current_account && index < desc_order.size())))) {
		QString legend_string;
		double legend_value = 0.0;
		double current_value = 0.0;
		if(account_order.isEmpty()) {
			if(desc_order[index].isEmpty()) {
				legend_string = tr("No description", "Referring to the transaction description property (transaction title/generic article name)");
			} else {
				legend_string = desc_map[desc_order[index]];
			}
			current_value = desc_values[desc_order[index]];
		} else {
			account = account_order.at(index);
			if(!account) {
				if(type == ACCOUNT_TYPE_ASSETS) legend_string = tr("Other accounts");
				else legend_string = tr("Other categories");
			} else if(current_account) legend_string = account->name();
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
			QPieSlice *slice = pie_series->append(QString("%1 (%2%)").arg(legend_string).arg(currency->formatValue(legend_value, deci, false)), current_value);
			if(theme < 0) {
				slice->setBrush(getPieBrush(index, account_order.size() > 0 ? account_order.size() : desc_order.size()));
				slice->setLabelColor(Qt::black);
				slice->setBorderColor(Qt::white);
				slice->setBorderWidth(1);
			}
			slice->setLabelArmLengthFactor(0.1);
			slice->setExplodeDistanceFactor(0.1);
			if(legend_value >= 8.0) {
				slice->setLabelVisible(true);
			} else {
				show_legend = true;
			}
		} else {
			b_axis->append(legend_string);
			bar_set->append(current_value);
			if(current_value > maxvalue) maxvalue = current_value;
			if(current_value < minvalue) minvalue = current_value;
			show_legend = true;
		}
		
		if(chart_type == 2) index--;
		else index++;
	}
	
	chart->removeAllSeries();
	
	foreach(QAbstractAxis* axis, chart->axes()) {
		chart->removeAxis(axis);
	}
	
	if(theme < 0) {
		chart->setBackgroundBrush(Qt::white);
		chart->setTitleBrush(Qt::black);
		chart->setPlotAreaBackgroundVisible(false);
		chart->legend()->setBackgroundVisible(false);
		chart->legend()->setColor(Qt::white);
		chart->legend()->setLabelColor(Qt::black);
	}
	
	if(chart_type == 1) {
		series = pie_series;
		pie_series->setPieSize(0.78);
		pie_series->setVerticalPosition(0.52);
		connect(pie_series, SIGNAL(hovered(QPieSlice*, bool)), this, SLOT(sliceHovered(QPieSlice*, bool)));
		connect(pie_series, SIGNAL(clicked(QPieSlice*)), this, SLOT(sliceClicked(QPieSlice*)));
		chart->addSeries(series);
	} else {
		series = bar_series;
		bar_series->append(bar_set);
		bar_series->setBarWidth(2.0 / 3.0);
		chart->addSeries(series);

		int y_lines = 5, y_minor = 0;
		calculate_minmax_lines(maxvalue, minvalue, y_lines, y_minor);
		QValueAxis *v_axis = new QValueAxis();
		//v_axis->setAlignment(Qt::AlignTop | Qt::AlignBottom);
		v_axis->setRange(minvalue, maxvalue);
		v_axis->setTickCount(y_lines + 1);
		v_axis->setMinorTickCount(chart_type == 2 ? 0 : y_minor);
		if(type == 3 || (maxvalue - minvalue) >= 50.0) v_axis->setLabelFormat(QString("%.0f"));
		else v_axis->setLabelFormat(QString("%.%1f").arg(QString::number(currency->fractionalDigits())));
		
		if(type == ACCOUNT_TYPE_ASSETS) v_axis->setTitleText(tr("Value") + QString(" (%1)").arg(currency->symbol(true)));
		else if(type == ACCOUNT_TYPE_INCOMES) v_axis->setTitleText(tr("Income") + QString(" (%1)").arg(currency->symbol(true)));
		else v_axis->setTitleText(tr("Cost") + QString(" (%1)").arg(currency->symbol(true)));
		
		if(theme < 0) {
			v_axis->setLinePen(QPen(Qt::darkGray, 1));
			v_axis->setLabelsColor(Qt::black);
			b_axis->setLinePen(QPen(Qt::darkGray, 1));
			b_axis->setLabelsColor(Qt::black);
			v_axis->setTitleBrush(Qt::black);
			b_axis->setTitleBrush(Qt::black);
			b_axis->setGridLineVisible(false);
			v_axis->setGridLinePen(QPen(Qt::darkGray, 1, Qt::DotLine));
			v_axis->setShadesVisible(false);
			b_axis->setShadesVisible(false);
		}

		if(chart_type == 3) {
			chart->setAxisX(b_axis, series);
			chart->setAxisY(v_axis, series);
		} else {
			chart->setAxisY(b_axis, series);
			chart->setAxisX(v_axis, series);
		}
		
		connect(bar_series, SIGNAL(hovered(bool, int, QBarSet*)), this, SLOT(onSeriesHovered(bool, int, QBarSet*)));
		
		show_legend = false;
	}
	
	chart->setTitle(QString("<div align=\"center\"><font size=\"+2\"><b>%1</b></font></div>").arg(title_string));
	if(show_legend) {
#if (QT_CHARTS_VERSION >= QT_CHARTS_VERSION_CHECK(5, 7, 0))
		chart->legend()->setShowToolTips(true);
#endif
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
	
	QVector<QGraphicsItem*> legend_texts;
	
	account = NULL;
	int index = 0;
	int text_width = 0;
	while((index < account_order.count()) || (current_account && index < desc_order.size())) {
		QString legend_string;
		double legend_value = 0.0;
		if(account_order.isEmpty()) {
			if(desc_order[index].isEmpty()) {
				legend_string = tr("No description", "Referring to the transaction description property (transaction title/generic article name)");
			} else {
				legend_string = desc_map[desc_order[index]];
			}
			legend_value = desc_values[desc_order[index]];
		} else {
			account = account_order.at(index);
			if(!account) {
				if(type == ACCOUNT_TYPE_ASSETS) legend_string = tr("Other accounts");
				else legend_string = tr("Other categories");
			} else if(current_account) legend_string = account->name();
			else legend_string = account->nameWithParent();
			legend_value = values[account];
		}
		legend_value = (legend_value * 100) / value;
		int deci = 0;
		if(legend_value < 10.0 && legend_value > -10.0) {
			legend_value = round(legend_value * 10.0) / 10.0;
			deci = 1;
		} else {
			legend_value = round(legend_value);
		}
		QGraphicsSimpleTextItem *legend_text = new QGraphicsSimpleTextItem(QString("%1 (%2%)").arg(legend_string).arg(currency->formatValue(legend_value, deci, false)));
		legend_text->setFont(legend_font);
		legend_text->setBrush(Qt::black);
		if(legend_text->boundingRect().width() > text_width) text_width = legend_text->boundingRect().width();
		legend_texts << legend_text;
		index++;
	}
	
	QGraphicsTextItem *title_text = new QGraphicsTextItem();
	title_text->setHtml(QString("<div align=\"center\"><font size=\"+2\"><b>%1</b></font></div>").arg(title_string));
	title_text->setDefaultTextColor(Qt::black);
	title_text->setFont(legend_font);
	title_text->setPos(view->height() / 2 - title_text->boundingRect().width() / 2, margin);
	scene->addItem(title_text);
	
	int diameter = view->height() - margin * 3 - title_text->boundingRect().height();
	if(diameter + margin * 2 + fh * 1.3 + text_width + margin > view->width()) diameter = view->width() - (margin * 2 + fh * 1.3 + text_width + margin);
	int legend_x = diameter + margin * 2;
	int chart_y = margin * 2 + title_text->boundingRect().height();
	int legend_y = chart_y;

	index = 0;
	while((index < account_order.count()) || (current_account && index < desc_order.size())) {
		QString legend_string;
		double legend_value = 0.0;
		if(account_order.isEmpty()) {
			if(desc_order[index].isEmpty()) {
				legend_string = tr("No description", "Referring to the transaction description property (transaction title/generic article name)");
			} else {
				legend_string = desc_map[desc_order[index]];
			}
			legend_value = desc_values[desc_order[index]];
		} else {
			account = account_order.at(index);
			if(!account) {
				if(type == ACCOUNT_TYPE_ASSETS) legend_string = tr("Other accounts");
				else legend_string = tr("Other categories");
			} else if(current_account) legend_string = account->name();
			else legend_string = account->nameWithParent();
			legend_value = values[account];
		}
		legend_value = (legend_value * 100) / value;
		if(legend_value < 10.0 && legend_value > -10.0) {
			legend_value = round(legend_value * 10.0) / 10.0;
		} else {
			legend_value = round(legend_value);
		}
		legend_texts[index]->setPos(legend_x + fh * 1.3, legend_y + (fh * 1.5) * index);
		scene->addItem(legend_texts[index]);
		index++;
	}


	account = NULL;
	double current_value = 0.0, current_value_1 = 0.0;
	int prev_end = 90 * 16;
	index = account_order.size() - 1;
	if(index < 0) index = desc_order.size() - 1;

	while(index >= 0) {
		if(account_order.isEmpty()) current_value_1 = desc_values[desc_order[index]];
		else current_value_1 = values[account_order[index]];
		current_value += current_value_1;
		int next_end = (int) lround((current_value * 360 * 16) / value) + 90 * 16;
		if(next_end > 360 * 16 && next_end - 360 * 16 > prev_end) next_end -= 360 * 16;
		int length = (next_end - prev_end);
		QGraphicsEllipseItem *ellipse = new QGraphicsEllipseItem(-diameter/ 2.0, -diameter/ 2.0, diameter, diameter);
		ellipse->setStartAngle(prev_end);
		ellipse->setSpanAngle(length);
		prev_end = next_end;
		if(prev_end > 360 * 16) prev_end -= 360 * 16;
		ellipse->setPen(QPen(Qt::white, 1));
		ellipse->setBrush(getPieBrush(index, account_order.size() > 0 ? account_order.size() : desc_order.size()));
		ellipse->setPos(diameter / 2 + margin, diameter / 2 + chart_y);
		scene->addItem(ellipse);
		QGraphicsRectItem *legend_box = new QGraphicsRectItem(legend_x, legend_y + (fh * 1.5) * index, fh, fh);
		legend_box->setPen(QPen(Qt::black));
		legend_box->setBrush(getPieBrush(index, account_order.size() > 0 ? account_order.size() : desc_order.size()));
		scene->addItem(legend_box);
		index--;
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
	int theme = themeCombo->itemData(index).toInt();
	chart->setTheme(theme >= 0 ? (QChart::ChartTheme) theme : QChart::ChartThemeBlueNcs);
	QSettings settings;
	settings.beginGroup("CategoriesComparisonChart");
	settings.setValue("theme", theme);
	settings.endGroup();
	updateDisplay();
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

class PointLabel2 : public QGraphicsItem {

	public:

		PointLabel2(QChart *c) : QGraphicsItem(c), chart(c) {}

		void setText(const QString &text) {
			m_text = text;
			QFontMetrics metrics(chart->font());
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
			painter->setBrush(chart->backgroundBrush());
			QPen pen = chart->axisX()->linePen();
			pen.setWidth(1);
			painter->setPen(pen);
			painter->drawPath(path);
			painter->setPen(QPen(chart->axisX()->labelsBrush().color()));
			painter->setBrush(chart->axisX()->labelsBrush());
			painter->drawText(m_textRect, m_text);
		}

	private:
	
		QString m_text;
		QRectF m_textRect;
		QRectF m_rect;
		QPointF m_anchor;
		QChart *chart;

};

void CategoriesComparisonChart::onSeriesHovered(bool state, int index, QBarSet *set) {
	if(state) {
		QAbstractBarSeries *series = qobject_cast<QAbstractBarSeries*>(sender());
		PointLabel2 *item;
		if(!point_label) {
			item = new PointLabel2(chart);
			point_label = item;
		} else {
			item = (PointLabel2*) point_label;
		}
		QList<QBarSet*> barsets = series->barSets();
		int set_index = barsets.indexOf(set);
		QPointF pos;
		qreal bar_width = 0.0;
		pos = chart->mapToPosition(QPointF(set->at(index), index), series);
		QPointF pos_next = chart->mapToPosition(QPointF(set->at(index), index + 1), series);
		bar_width = (pos_next.y() - pos.y()) * series->barWidth();
		qreal pos_y = pos.y() - (bar_width / 2);
		pos_y += (set_index + 0.5) * (bar_width / barsets.count());
		pos.setY(pos_y);
		Currency *currency = budget->defaultCurrency();
		if(selectedAccount()) currency = selectedAccount()->currency();
		item->setText(tr("%1\nValue: %2").arg(((QBarCategoryAxis*) chart->axisY())->at(index)).arg(currency->formatValue(set->at(index))));
		item->setAnchor(pos);
		item->setPos(pos + QPoint(10, -50));
		item->setZValue(11);
		if(pos.x() + item->boundingRect().width() + 10 > chart->size().width()) {
			item->setPos(pos + QPoint(-10 - item->boundingRect().width(), -50));
		}
		item->show();
	} else if(point_label) {
		point_label->hide();
	}
}

#endif
