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

#include <KConfigGroup>
#include <KSharedConfig>
#include <kconfig.h>
#include <kdateedit.h>
#include <kmessagebox.h>
#include <kstdguiitem.h>
#include <klocalizedstring.h>

#include <math.h>

extern double monthsBetweenDates(const QDate &date1, const QDate &date2);
extern void saveSceneImage(QWidget*, QGraphicsScene*);

CategoriesComparisonChart::CategoriesComparisonChart(Budget *budg, QWidget *parent) : QWidget(parent), budget(budg) {

	setAttribute(Qt::WA_DeleteOnClose, true);	

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

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	QDialogButtonBox *buttons = new QDialogButtonBox(this);
	saveButton = buttons->addButton(QString(), QDialogButtonBox::ActionRole);
	KGuiItem::assign(saveButton, KStandardGuiItem::saveAs());
	printButton = buttons->addButton(QString(), QDialogButtonBox::ActionRole);
	KGuiItem::assign(printButton, KStandardGuiItem::print());
	layout->addWidget(buttons);

	scene = NULL;
	view = new QGraphicsView(this);
	view->setRenderHint(QPainter::Antialiasing, true);
	view->setRenderHint(QPainter::HighQualityAntialiasing, true);
	layout->addWidget(view);

	QGroupBox *settingsWidget = new QGroupBox(i18n("Options"), this);
	QVBoxLayout *settingsLayout = new QVBoxLayout(settingsWidget);

	QHBoxLayout *choicesLayout = new QHBoxLayout();
	settingsLayout->addLayout(choicesLayout);
	fromButton = new QCheckBox(i18n("From"), settingsWidget);
	fromButton->setChecked(true);
	choicesLayout->addWidget(fromButton);
	fromEdit = new KDateEdit(settingsWidget);
	fromEdit->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
	fromEdit->setDate(from_date);
	choicesLayout->addWidget(fromEdit);
	choicesLayout->addWidget(new QLabel(i18n("To"), settingsWidget));
	toEdit = new KDateEdit(settingsWidget);
	toEdit->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
	toEdit->setDate(to_date);
	choicesLayout->addWidget(toEdit);
	prevYearButton = new QPushButton(QIcon::fromTheme("arrow-left-double"), "", settingsWidget);
	choicesLayout->addWidget(prevYearButton);
	prevMonthButton = new QPushButton(QIcon::fromTheme("arrow-left"), "", settingsWidget);
	choicesLayout->addWidget(prevMonthButton);
	nextMonthButton = new QPushButton(QIcon::fromTheme("arrow-right"), "", settingsWidget);
	choicesLayout->addWidget(nextMonthButton);
	nextYearButton = new QPushButton(QIcon::fromTheme("arrow-right-double"), "", settingsWidget);
	choicesLayout->addWidget(nextYearButton);
	choicesLayout->addStretch(1);

	QHBoxLayout *typeLayout = new QHBoxLayout();
	settingsLayout->addLayout(typeLayout);
	typeLayout->addWidget(new QLabel(i18n("Source:"), settingsWidget));
	sourceCombo = new QComboBox(settingsWidget);
	sourceCombo->setEditable(false);
	sourceCombo->addItem(i18n("All Expenses"));
	sourceCombo->addItem(i18n("All Incomes"));
	sourceCombo->addItem(i18n("All Accounts"));
	Account *account = budget->expensesAccounts.first();
	while(account) {
		sourceCombo->addItem(i18n("Expenses: %1", account->name()));
		account = budget->expensesAccounts.next();
	}
	account = budget->incomesAccounts.first();
	while(account) {
		sourceCombo->addItem(i18n("Incomes: %1", account->name()));
		account = budget->incomesAccounts.next();
	}
	typeLayout->addWidget(sourceCombo);
	typeLayout->addStretch(1);

	current_account = NULL;

	layout->addWidget(settingsWidget);

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

void CategoriesComparisonChart::sourceChanged(int index) {
	fromButton->setEnabled(index != 2);
	fromEdit->setEnabled(index != 2);
	updateDisplay();
}
void CategoriesComparisonChart::saveConfig() {
	KConfigGroup config = KSharedConfig::openConfig()->group("Categories Comparison Chart");
	config.writeEntry("size", ((QDialog*) parent())->size());
}
void CategoriesComparisonChart::toChanged(const QDate &date) {
	bool error = false;
	if(!date.isValid()) {
		KMessageBox::error(this, i18n("Invalid date."));
		error = true;
	}
	if(!error && fromEdit->date() > date) {
		if(fromButton->isChecked() && fromButton->isEnabled()) {
			KMessageBox::error(this, i18n("To date is before from date."));
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
		toEdit->lineEdit()->selectAll();
		return;
	}
	to_date = date;
	updateDisplay();
}
void CategoriesComparisonChart::fromChanged(const QDate &date) {
	bool error = false;
	if(!date.isValid()) {
		KMessageBox::error(this, i18n("Invalid date."));
		error = true;
	}
	if(!error && date > toEdit->date()) {
		KMessageBox::error(this, i18n("From date is after to date."));
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
		fromEdit->lineEdit()->selectAll();
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

void CategoriesComparisonChart::save() {
	saveSceneImage(this, scene);
}

void CategoriesComparisonChart::print() {
	if(!scene) return;
	QPrinter pr;
	QPrintDialog *dialog = new QPrintDialog(&pr, this);
	if(dialog->exec() == QDialog::Accepted) {
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

	QMap<Account*, double> values;
	QMap<Account*, double> counts;
	QMap<QString, double> desc_values;
	QMap<QString, double> desc_counts;
	double value_count = 0.0;
	double value = 0.0;

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
	int n = 0;
	if(current_account) {
		n = desc_values.count();
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
	while((current_account && it_desc != it_desc_end) || account) {

		bool b_empty = false;
		QString legend_string;
		double legend_value = 0.0;
		if(current_account) {
			if(it_desc.key().isEmpty()) {
				legend_string = i18n("No description");
			} else {
				legend_string = it_desc.key();
			}
			legend_value = it_desc.value();
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
		QGraphicsSimpleTextItem *legend_text = new QGraphicsSimpleTextItem(QString("%1 (%2\%)").arg(legend_string).arg(QLocale().toString(legend_value, 'f', deci)));
		int index_bak = index;
		if(b_empty) index = n - 1;
		if(legend_text->boundingRect().width() > text_width) text_width = legend_text->boundingRect().width();
		legend_text->setFont(legend_font);
		legend_text->setBrush(Qt::black);
		legend_text->setPos(legend_x + 10 + fh + 5, margin + 10 + (fh + 5) * index);
		scene->addItem(legend_text);

		if(current_account) {
			if(it_desc.value() < 0.0) {
				value -= it_desc.value();
				*it_desc = 0.0;
			}
		} else {
			if(values[account] < 0.0) {
				value -= values[account];
				values[account] = 0.0;
			}
		}
		if(current_account) {
			++it_desc;
		} else if(type == ACCOUNT_TYPE_ASSETS) {
			account = budget->assetsAccounts.next();
			if(account == budget->balancingAccount) account = budget->assetsAccounts.next();
		} else if(type == ACCOUNT_TYPE_EXPENSES) account = budget->expensesAccounts.next();
		else  account = budget->incomesAccounts.next();
		if(b_empty) index = index_bak;
		else index++;
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
	it_desc = desc_values.begin();
	it_desc_end = desc_values.end();
	index = 0;
	double current_value = 0.0;
	int prev_end = 0;
	while((current_account && it_desc != it_desc_end) || account) {
		if(current_account) current_value += it_desc.value();
		else current_value += values[account];
		int next_end = (int) lround((current_value * 360 * 16) / value);
		int length = (next_end - prev_end);
		QGraphicsEllipseItem *ellipse = new QGraphicsEllipseItem(-diameter/ 2.0, -diameter/ 2.0, diameter, diameter);
		ellipse->setStartAngle(prev_end);
		ellipse->setSpanAngle(length);
		prev_end = next_end;
		bool b_empty = false;
		if(current_account && it_desc.key().isEmpty()) {b_empty = true;}
		int index_bak = index;
		if(b_empty) index = n - 1;
		ellipse->setPen(Qt::NoPen);
		ellipse->setBrush(getBrush(index));
		ellipse->setPos(diameter / 2 + margin, diameter / 2 + margin);
		scene->addItem(ellipse);
		QGraphicsRectItem *legend_box = new QGraphicsRectItem(legend_x + 10, margin + 10 + (fh + 5) * index, fh, fh);
		legend_box->setPen(QPen(Qt::black));
		legend_box->setBrush(getBrush(index));
		scene->addItem(legend_box);
		if(current_account) {
			++it_desc;
		} else if(type == ACCOUNT_TYPE_ASSETS) {
			account = budget->assetsAccounts.next();
			if(account == budget->balancingAccount) account = budget->assetsAccounts.next();
		} else if(type == ACCOUNT_TYPE_EXPENSES) account = budget->expensesAccounts.next();
		else  account = budget->incomesAccounts.next();
		if(b_empty) index = index_bak;
		else index++;
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
	sourceCombo->addItem(i18n("All Expenses"));
	sourceCombo->addItem(i18n("All Incomes"));
	sourceCombo->addItem(i18n("All Accounts"));
	int i = 3;
	Account *account = budget->expensesAccounts.first();
	while(account) {
		sourceCombo->addItem(i18n("Expenses: %1", account->name()));
		if(account == current_account) curindex = i;
		account = budget->expensesAccounts.next();
		i++;
	}
	account = budget->incomesAccounts.first();
	while(account) {
		sourceCombo->addItem(i18n("Incomes: %1", account->name()));
		if(account == current_account) curindex = i;
		account = budget->incomesAccounts.next();
		i++;
	}
	if(curindex < sourceCombo->count()) sourceCombo->setCurrentIndex(curindex);
	sourceCombo->blockSignals(false);
	updateDisplay();
}

#include "categoriescomparisonchart.moc"
