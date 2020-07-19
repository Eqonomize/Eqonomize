/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016-2019 by Hanna Knutsson            *
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

#include "budget.h"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTextStream>
#include <QMap>
#include <QSaveFile>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QProcess>
#include <QTemporaryFile>
#include <math.h>

#include <QDebug>

#include <locale.h>

#include "recurrence.h"

void read_id(QXmlStreamAttributes *attr, qlonglong &id, int &rev1, int &rev2) {
	id = attr->value("id").toLongLong();
	QStringRef rev = attr->value("revisions");
	if(rev.contains(':')) {
		QVector<QStringRef> rev_strs = rev.split(':');
		if(rev_strs.isEmpty()) {
			rev1 = rev.toInt();
		} else if(rev_strs.size() == 1) {
			rev1 = rev_strs[0].toInt();
		} else {
			rev1 = rev_strs[0].toInt();
			rev2 = rev_strs.last().toInt();
		}
	} else {
		rev1 = rev.toInt();
		rev2 = rev1;
	}
	if(rev1 <= 0) rev1 = 1;
	if(rev2 <= rev1) rev2 = rev1;
}

void write_id(QXmlStreamAttributes *attr, qlonglong &id, int &rev1, int &rev2) {
	attr->append("id", QString::number(id));
	if(rev2 == rev1) attr->append("revisions", QString::number(rev1));
	else attr->append("revisions", QString::number(rev1) + ':' + QString::number(rev2));
}
void write_id(QXmlStreamWriter *writer, qlonglong &id, int &rev1, int &rev2) {
	writer->writeAttribute("id", QString::number(id));
	if(rev2 == rev1) writer->writeAttribute("revisions", QString::number(rev1));
	else writer->writeAttribute("revisions", QString::number(rev1) + ':' + QString::number(rev2));
}

int compare_id(const int &id1, const int &id2) {
	if(id1 < 0) {
		if(id2 < 0) {
			if(id1 > id2) return -1;
			if(id1 < id2) return 1;
		} else {
			return 1;
		}
	} else if(id2 < 0) {
		return -1;
	} else {
		if(id1 < id2) return -1;
		if(id1 > id2) return 1;
	}
	return 0;
}

bool transactions_less_than_stamp(Transactions *t1, Transactions *t2) {
	if(t1->timestamp() == 0 && t2->timestamp() == 0) return t1->date() < t2->date() || (t1->date() == t2->date() && t2->description().localeAwareCompare(t1->description()) < 0);
	return t1->timestamp() < t2->timestamp() || (t1->timestamp() == t2->timestamp() && t2->description().localeAwareCompare(t1->description()) < 0);
}
bool transactions_less_than_trade_stamp(Transactions *t1, SecurityTrade *t2) {
	return t1->timestamp() < t2->timestamp || (t1->timestamp() == t2->timestamp && t1->date() < t2->date);
}

bool transaction_list_less_than_stamp(Transaction *t1, Transaction *t2) {
	if(t1->timestamp() < t2->timestamp()) return true;
	if(t1->timestamp() > t2->timestamp()) return false;
	if(t1->timestamp() == 0) {
		if(t1->date() < t2->date()) return true;
		if(t1->date() > t2->date()) return false;
	}
	if(t1->parentSplit()) {
		if(!t2->parentSplit()) {
			return t2->description().localeAwareCompare(t1->parentSplit()->description()) < 0;
		} else if(t1->parentSplit() != t2->parentSplit()) {
			if(t1->parentSplit()->timestamp() != t2->parentSplit()->timestamp()) return t1->parentSplit()->timestamp() < t2->parentSplit()->timestamp();
			int r = t2->parentSplit()->description().localeAwareCompare(t1->parentSplit()->description());
			if(r == 0) return (void*) t1->parentSplit() < (void*) t2->parentSplit();
			else return r < 0;
		}
	} else if(t2->parentSplit()) {
		return t2->parentSplit()->description().localeAwareCompare(t1->description()) < 0;
	}
	return t2->description().localeAwareCompare(t1->description()) < 0;
}
bool transaction_list_less_than(Transaction *t1, Transaction *t2) {
	if(t1->date() < t2->date()) return true;
	if(t1->date() > t2->date()) return false;
	if(t1->timestamp() < t2->timestamp()) return true;
	if(t1->timestamp() > t2->timestamp()) return false;
	if(t1->parentSplit()) {
		if(!t2->parentSplit()) {
			return t2->description().localeAwareCompare(t1->parentSplit()->description()) < 0;
		} else if(t1->parentSplit() != t2->parentSplit()) {
			if(t1->parentSplit()->timestamp() != t2->parentSplit()->timestamp()) return t1->parentSplit()->timestamp() < t2->parentSplit()->timestamp();
			int r = t2->parentSplit()->description().localeAwareCompare(t1->parentSplit()->description());
			if(r == 0) return (void*) t1->parentSplit() < (void*) t2->parentSplit();
			else return r < 0;
		}
	} else if(t2->parentSplit()) {
		return t2->parentSplit()->description().localeAwareCompare(t1->description()) < 0;
	}
	return t2->description().localeAwareCompare(t1->description()) < 0;
}
bool split_list_less_than_stamp(SplitTransaction *t1, SplitTransaction *t2) {
	if(t1->timestamp() == 0 && t2->timestamp() == 0) return split_list_less_than(t1, t2);
	return t1->timestamp() < t2->timestamp() || (t1->timestamp() == t2->timestamp() && t2->description().localeAwareCompare(t1->description()) < 0);
}
bool split_list_less_than(SplitTransaction *t1, SplitTransaction *t2) {
	return t1->date() < t2->date() || (t1->date() == t2->date() && (t1->timestamp() < t2->timestamp() || (t1->timestamp() == t2->timestamp() && t2->description().localeAwareCompare(t1->description()) < 0)));
}
bool schedule_list_less_than_stamp(ScheduledTransaction *t1, ScheduledTransaction *t2) {
	if(t1->timestamp() == 0 && t2->timestamp() == 0) return schedule_list_less_than(t1, t2);
	return t1->timestamp() < t2->timestamp() || (t1->timestamp() == t2->timestamp() && t2->description().localeAwareCompare(t1->description()) < 0);
}
bool schedule_list_less_than(ScheduledTransaction *t1, ScheduledTransaction *t2) {
	return t1->date() < t2->date() || (t1->date() == t2->date() && (t1->timestamp() < t2->timestamp() || (t1->timestamp() == t2->timestamp() && t2->description().localeAwareCompare(t1->description()) < 0)));
}
bool trade_list_less_than_stamp(SecurityTrade *t1, SecurityTrade *t2) {
	return t1->timestamp < t2->timestamp || (t1->timestamp == t2->timestamp && t1->date < t2->date);
}
bool trade_list_less_than(SecurityTrade *t1, SecurityTrade *t2) {
	return t1->date < t2->date || (t1->date == t2->date && t1->timestamp < t2->timestamp);
}
bool security_list_less_than(Security *t1, Security *t2) {
	return QString::localeAwareCompare(t1->name(), t2->name()) < 0;
}

bool is_zero(double value) {
	return value < 0.0000001 && value > -0.0000001;
}

Budget::Budget() {	
	currencies.setAutoDelete(true);
	expenses.setAutoDelete(true);
	incomes.setAutoDelete(true);
	transfers.setAutoDelete(true);
	securityTransactions.setAutoDelete(true);
	transactions.setAutoDelete(false);
	scheduledTransactions.setAutoDelete(true);
	splitTransactions.setAutoDelete(true);
	securities.setAutoDelete(true);
	expensesAccounts.setAutoDelete(true);
	incomesAccounts.setAutoDelete(true);
	assetsAccounts.setAutoDelete(true);
	securityTrades.setAutoDelete(true);
	accounts.setAutoDelete(false);
	o_sync = new BudgetSynchronization();
	o_sync->clear();
	currency_euro = new Currency(this, "EUR", "€", tr("European Euro"), 1.0);
	currency_euro->setAsLocal(false);
	addCurrency(currency_euro);
	loadCurrencies();
	default_currency = currency_euro;
	last_id = 0;
	balancingAccount = new AssetsAccount(this, ASSETS_TYPE_BALANCING, tr("Balancing", "Name of account for transactions that adjust account balances"), 0.0);
	balancingAccount->setCurrency(NULL);
	balancingAccount->setId(0);
	assetsAccounts.append(balancingAccount);
	accounts.append(balancingAccount);
	budgetAccount = NULL;
	i_share_decimals = 4;
	i_quotation_decimals = 4;
	i_budget_day = 1;
	i_budget_month = 1;
	i_revision = 1;
	i_opened_revision = 0;
	last_id = 0;
	b_record_new_tags = false;
	b_record_new_accounts = false;
	b_record_new_securities = false;
	i_tcrd = TRANSACTION_CONVERSION_RATE_AT_DATE;
	null_incomes_account = new IncomesAccount(this, QString());
	setlocale(LC_MONETARY, "");
	struct lconv *lc = localeconv();
	monetary_decimal_separator = QString::fromLocal8Bit(lc->mon_decimal_point);
	if(monetary_decimal_separator.isEmpty()) monetary_decimal_separator = QLocale().decimalPoint();
	monetary_group_separator = QString::fromLocal8Bit(lc->mon_thousands_sep);
	monetary_negative_sign = QString::fromLocal8Bit(lc->negative_sign);
	monetary_positive_sign = QString::fromLocal8Bit(lc->positive_sign);
	if(monetary_negative_sign == "-" && QLocale().negativeSign() == 0x2212) monetary_negative_sign = QLocale().negativeSign();
	monetary_group_format = lc->mon_grouping;
	monetary_decimal_places = 2;
	decimal_separator = QString::fromLocal8Bit(lc->decimal_point);
	if(decimal_separator.isEmpty()) decimal_separator = QLocale().decimalPoint();
	group_separator = QString::fromLocal8Bit(lc->thousands_sep);
	negative_sign = QString::fromLocal8Bit(lc->negative_sign);
	positive_sign = QString::fromLocal8Bit(lc->positive_sign);
	if(negative_sign == "-" && QLocale().negativeSign() == 0x2212) negative_sign = QLocale().negativeSign();
	group_format = lc->grouping;
#ifdef Q_OS_ANDROID
	currency_symbol_precedes = true;
	currency_code_precedes = true;
	currency_symbol_precedes_neg = true;
	currency_code_precedes_neg = true;
	currency_symbol_space = false;
	currency_code_space = true;
	currency_symbol_space_neg = false;
	currency_code_space_neg = true;
	monetary_sign_p_symbol_neg = 1;
	monetary_sign_p_symbol_pos = 1;
	monetary_sign_p_code_neg = 1;
	monetary_sign_p_code_pos = 1;
#else
	currency_symbol_precedes = lc->p_cs_precedes;
	currency_symbol_precedes_neg = lc->n_cs_precedes;
	currency_symbol_space = (lc->p_sep_by_space == 1);
	currency_symbol_space_neg = (lc->n_sep_by_space == 1);
	monetary_sign_p_symbol_neg = (int) lc->n_sign_posn;
	monetary_sign_p_symbol_pos = (int) lc->p_sign_posn;
#	ifdef _WIN32
	currency_code_precedes = currency_symbol_precedes;
	currency_code_precedes_neg = currency_symbol_precedes;
	currency_code_space = true;
	currency_code_space_neg = true;
	monetary_sign_p_code_neg = monetary_sign_p_symbol_neg;
	monetary_sign_p_code_pos = monetary_sign_p_symbol_pos;
#	else
	currency_code_precedes = lc->int_p_cs_precedes;
	currency_code_precedes_neg = lc->int_n_cs_precedes;
	currency_code_space = (lc->int_p_sep_by_space == 1);
	currency_code_space_neg = (lc->int_n_sep_by_space == 1);
	monetary_sign_p_code_neg = (int) lc->int_n_sign_posn;
	monetary_sign_p_code_pos = (int) lc->int_p_sign_posn;
#endif
	if(monetary_sign_p_symbol_neg > 4) monetary_sign_p_symbol_neg = 1;
	if(monetary_sign_p_symbol_pos > 4) monetary_sign_p_symbol_pos = 1;
	if(monetary_sign_p_code_neg > 4) monetary_sign_p_code_neg = 1;
	if(monetary_sign_p_code_pos > 4) monetary_sign_p_code_pos = 1;
	/*monetary_decimal_places = (int) lc->frac_digits;
	if(monetary_decimal_places < 0 || monetary_decimal_places > 20) monetary_decimal_places = 2;*/
#endif
	monetary_group_format = monetary_group_format.trimmed();
	if(monetary_decimal_separator.isEmpty()) monetary_decimal_separator = QLocale().decimalPoint();
	if(monetary_group_separator.isEmpty()) monetary_group_separator = QLocale().groupSeparator();
}
Budget::~Budget() {}

qlonglong Budget::getNewId() {
	last_id++;
	return last_id;
}
int Budget::revision() {return i_revision;}

void Budget::clear() {
	i_revision = 1;
	i_opened_revision = 0;
	last_id = 0;
	transactions.clear();
	scheduledTransactions.clear();
	splitTransactions.clear();
	securities.clear();
	expenses.clear();
	incomes.clear();
	transfers.clear();
	securityTransactions.clear();
	securities.clear();
	accounts.clear();
	securityTrades.clear();
	o_sync->clear();
	assetsAccounts.setAutoDelete(false);
	assetsAccounts.removeRef(balancingAccount);
	assetsAccounts.setAutoDelete(true);
	for(AccountList<IncomesAccount*>::const_iterator it = incomesAccounts.constBegin(); it != incomesAccounts.constEnd(); ++it) {
		CategoryAccount *ca = *it;
		if(ca->parentCategory()) ca->o_parent = NULL;
		ca->subCategories.clear();
	}
	for(AccountList<ExpensesAccount*>::const_iterator it = expensesAccounts.constBegin(); it != expensesAccounts.constEnd(); ++it) {
		CategoryAccount *ca = *it;
		if(ca->parentCategory()) ca->o_parent = NULL;
		ca->subCategories.clear();
	}
	incomesAccounts.clear();
	expensesAccounts.clear();
	assetsAccounts.clear();
	tags.clear();
	assetsAccounts.append(balancingAccount);
	accounts.append(balancingAccount);
	budgetAccount = NULL;
}

QString Budget::formatMoney(double v, int precision, bool show_currency) {
	return default_currency->formatValue(v, precision, show_currency);
}
QString Budget::formatValue(double value, int nr_of_decimals, bool always_show_sign) {
	if(is_zero(value)) value = 0.0;
	QString s;
	bool neg = false;
	if(value == 0.0) {
		s = "0";
		if(nr_of_decimals > 0) {
			s += decimal_separator;
			while(nr_of_decimals > 0) {
				s += '0';
				nr_of_decimals--;
			}
		}
	} else {
		neg = value < 0;
		s = QString::number(neg ? -value : value, 'f', nr_of_decimals);
		int i = s.indexOf('.');
		if(decimal_separator != ".") s.replace(i, 1, decimal_separator);
		if(!group_separator.isEmpty() && !group_format.isEmpty()) {
			int group_size = 3, i_format = 0;
			if(group_format.length() > i_format) {
				group_size = group_format[i_format];
			}
			while(i > group_size) {
				i -= group_size;
				s.insert(i, group_separator);
				if(group_format.length() > i_format) {
					i_format++;
					if(group_format[i_format] == (char) CHAR_MAX) break;
					if(group_format[i_format] > (char) 0) group_size = group_format[i_format];
				}
			}
		}
	}
	if(always_show_sign && value == 0.0) {
		s.insert(0, "±");
	} else if(neg) {
		if(!negative_sign.isEmpty()) s.insert(0, negative_sign);
		else s.insert(0, QLocale().negativeSign());
	} else {
		if(!positive_sign.isEmpty()) s.insert(0, positive_sign);
		else if(always_show_sign) s.insert(0, QLocale().positiveSign());
	}
	return s;
}
QString Budget::formatValue(int value, int nr_of_decimals, bool always_show_sign) {
	QString s;
	bool neg = false;
	if(value == 0) {
		s = "0";
		if(nr_of_decimals > 0) {
			s += decimal_separator;
			while(nr_of_decimals > 0) {
				s += '0';
				nr_of_decimals--;
			}
		}
	} else {
		neg = value < 0;
		s = QString::number(neg ? -value : value);
		if(!group_separator.isEmpty() && !group_format.isEmpty()) {
			int group_size = 3, i_format = 0;
			if(group_format.length() > i_format) {
				group_size = group_format[i_format];
			}
			int i = s.length();
			while(i > group_size) {
				i -= group_size;
				s.insert(i, group_separator);
				if(group_format.length() > i_format) {
					i_format++;
					if(group_format[i_format] == (char) CHAR_MAX) break;
					if(group_format[i_format] > (char) 0) group_size = group_format[i_format];
				}
			}
		}
		if(nr_of_decimals > 0) {
			s += decimal_separator;
			while(nr_of_decimals > 0) {
				s += '0';
				nr_of_decimals--;
			}
		}
	}
	QString sgn;
	if(always_show_sign && value == 0.0) {
		sgn = "±";
	} else if(neg) {
		if(!negative_sign.isEmpty()) sgn = negative_sign;
		else sgn = QLocale().negativeSign();
	} else {
		if(!positive_sign.isEmpty()) sgn = positive_sign;
		else if(always_show_sign) sgn = QLocale().positiveSign();
	}
	s.insert(0, sgn);
	return s;
}

void Budget::loadCurrencies() {
	loadGlobalCurrencies();
	loadLocalCurrencies();
}
void Budget::loadGlobalCurrencies() {
	loadCurrenciesFile(DATA_DIR "/currencies.xml", false);
}
void Budget::loadLocalCurrencies() {
	loadCurrenciesFile(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/currencies.xml", true);
}
void Budget::loadCurrenciesFile(QString filename, bool is_local) {
	QFile file(filename);
	if(is_local && !file.exists()) {
		QString error = saveCurrencies();
		if(!error.isNull()) qCritical() << error;
		return;
	}
	if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qCritical() << tr("Couldn't open %1 for reading").arg(filename);
		return;
	} else if(!file.size()) {
		return;
	}
	QXmlStreamReader xml(&file);
	if(!xml.readNextStartElement()) {
		qCritical() << tr("XML parse error: \"%1\" at line %2, col %3").arg(xml.errorString()).arg(xml.lineNumber()).arg(xml.columnNumber());
		return;
	}
	if(xml.name() != "Eqonomize") {
		qCritical() << tr("Invalid root element %1 in XML document").arg(xml.name().toString());
		return;
	}

	bool oldversion = (xml.attributes().value("version").toString() != QString(VERSION));
	
	int currency_errors = 0;

	while(xml.readNextStartElement()) {
		if(xml.name() == "currency") {
			bool valid = true;
			Currency *currency = new Currency(this, &xml, &valid);
			if(valid) {
				if(is_local) {
					Currency *currency_old = findCurrency(currency->code());
					if(currency_old) {
						currency_old->merge(currency, false);
						delete currency;
					} else {
						currency->setAsLocal();
						currencies.append(currency);
					}
				} else {
					currencies.append(currency);
				}
			} else {
				currency_errors++;
				delete currency;
			}
		
		} else {
			qCritical() << tr("Unknown XML element: \"%1\" at line %2, col %3").arg(xml.name().toString()).arg(xml.lineNumber()).arg(xml.columnNumber());
			xml.skipCurrentElement();
		}
	}
	
	if(currency_errors > 0) {
		qCritical() << tr("Unable to load %n currency/currencies.", "", currency_errors);
	}
	
	currencies.sort();

	if(xml.hasError()) {
		qCritical() << tr("XML parse error: \"%1\" at line %2, col %3").arg(xml.errorString()).arg(xml.lineNumber()).arg(xml.columnNumber());
	}
	
	file.close();

	if(oldversion && is_local) {
		QString error = saveCurrencies();
		if(!error.isNull()) qCritical() << error;
	}
	
}

QString Budget::loadECBData(QByteArray data) {
	
	QXmlStreamReader xml(data);
	
	if(!xml.readNextStartElement()) {
		return tr("XML parse error: \"%1\" at line %2, col %3").arg(xml.errorString()).arg(xml.lineNumber()).arg(xml.columnNumber());
	}
	
	bool had_data = false;
	
	while(xml.readNextStartElement()) {
		if(xml.name() == "Cube") {
			while(xml.readNextStartElement()) {
				if(xml.name() == "Cube") {
					QXmlStreamAttributes attr = xml.attributes();
					QDate date = QDate::fromString(attr.value("time").trimmed().toString(), Qt::ISODate);
					while(xml.readNextStartElement()) {
						if(xml.name() == "Cube") {							
							attr = xml.attributes();
							QString code = attr.value("currency").trimmed().toString();
							double exrate = attr.value("rate").toDouble();
							if(!code.isEmpty() && code != "EUR" && exrate > 0.0 && date.isValid()) {
								if(!had_data) {
									for(CurrencyList<Currency*>::const_iterator it = currencies.constBegin(); it != currencies.constEnd(); ++it) {
										Currency *cur = *it;
										if(cur->exchangeRateSource() == EXCHANGE_RATE_SOURCE_ECB) {
											cur->setExchangeRateSource(EXCHANGE_RATE_SOURCE_NONE);
										}
									}
								}
								Currency *cur = findCurrency(code);
								if(cur) {
									bool keep_old = cur->rates.size() > 1;
									if(!keep_old) {
										for(AccountList<AssetsAccount*>::const_iterator it = assetsAccounts.constBegin(); it != assetsAccounts.constEnd(); ++it) {
											if((*it)->currency() == cur) {keep_old = true; break;}
										}
									}
									if(!keep_old) cur->rates.clear();
									cur->setExchangeRate(exrate, date);
								} else {
									cur = new Currency(this, code, QString(), QString(), exrate, date);
									addCurrency(cur);
								}
								cur->setExchangeRateSource(EXCHANGE_RATE_SOURCE_ECB);
								had_data = true;
							}
						}
						xml.skipCurrentElement();
					}
				}
				xml.skipCurrentElement();
			}
		}
		xml.skipCurrentElement();
	}
	
	if(!had_data) return tr("No exchange rates found.");
	
	return QString();
}

QString Budget::loadMyCurrencyNetData(QByteArray data) {
	
	QJsonDocument jdoc = QJsonDocument::fromJson(data);
	if(!jdoc.isArray()) return tr("No exchange rates found.");
	QJsonArray jarr = jdoc.array();
	
	bool had_data = false;
	
	for(int i = 0; i < jarr.count(); i++) {
		if(jarr[i].isObject()) {
			QJsonObject jobj = jarr[i].toObject();
			QString code = jobj["currency_code"].toString().trimmed();
			double exrate = jobj["rate"].toDouble();
			QString name = jobj["name"].toString().trimmed();
			if(!code.isEmpty() && code != "EUR"  && exrate > 0.0) {
				if(!had_data) {
					for(CurrencyList<Currency*>::const_iterator it = currencies.constBegin(); it != currencies.constEnd(); ++it) {
						Currency *cur = *it;
						if(cur->exchangeRateSource() == EXCHANGE_RATE_SOURCE_MYCURRENCY_NET) {
							cur->setExchangeRateSource(EXCHANGE_RATE_SOURCE_NONE);
						}
					}
				}
				Currency *cur = findCurrency(code);
				if(cur && cur->exchangeRateSource() != EXCHANGE_RATE_SOURCE_ECB) {
					bool keep_old = cur->rates.size() > 1;
					if(!keep_old) {
						for(AccountList<AssetsAccount*>::const_iterator it = assetsAccounts.constBegin(); it != assetsAccounts.constEnd(); ++it) {
							if((*it)->currency() == cur) {keep_old = true; break;}
						}
					}
					if(!keep_old) cur->rates.clear();
					cur->setExchangeRate(exrate);
					cur->setExchangeRateSource(EXCHANGE_RATE_SOURCE_MYCURRENCY_NET);
				} else if(!cur) {
					cur = new Currency(this, code, QString(), name, exrate);
					cur->setExchangeRateSource(EXCHANGE_RATE_SOURCE_MYCURRENCY_NET);
					addCurrency(cur);
				}
				had_data = true;
			}
		}
	}
	
	if(!had_data) return tr("No exchange rates found.");
	
	return QString();
}

QString Budget::loadMyCurrencyNetHtml(QByteArray data) {
	
	Currency *cur_usd = findCurrency("USD");
	if(!cur_usd) return tr("USD currency missing.");
	double usd_rate = cur_usd->exchangeRate();
	
	bool had_data = false;
	
	int i = data.indexOf("class=\'country\'");
	while(i >= 0) {
		QString code, name;
		double exrate = 0.0;
		i += 15;
		int i2 = data.indexOf("data-currency-code=\"", i);
		if(i2 >= 0) {
			i2 += 19;
			int i3 = data.indexOf("\"", i2 + 1);
			if(i3 >= 0) {
				code = data.mid(i2 + 1, i3 - (i2 + 1));
			}
		}
		i2 = data.indexOf("data-currency-name=\'", i);
		if(i2 >= 0) {
			i2 += 19;
			int i3 = data.indexOf("|", i2 + 1);
			if(i3 >= 0) {
				name = data.mid(i2 + 1, i3 - (i2 + 1)).trimmed();
			}
		}
		i2 = data.indexOf("data-rate=\'", i);
		if(i2 >= 0) {
			i2 += 10;
			int i3 = data.indexOf("'", i2 + 1);
			if(i3 >= 0) {
				exrate = data.mid(i2 + 1, i3 - (i2 + 1)).toDouble();
			}
		}
		if(!code.isEmpty() && code != "EUR"  && exrate > 0.0) {
			if(!had_data) {
				for(CurrencyList<Currency*>::const_iterator it = currencies.constBegin(); it != currencies.constEnd(); ++it) {
					Currency *cur = *it;
					if(cur->exchangeRateSource() == EXCHANGE_RATE_SOURCE_MYCURRENCY_NET) {
						cur->setExchangeRateSource(EXCHANGE_RATE_SOURCE_NONE);
					}
				}
			}
			Currency *cur = findCurrency(code);
			if(cur && cur->exchangeRateSource() != EXCHANGE_RATE_SOURCE_ECB) {
				bool keep_old = cur->rates.size() > 1;
				if(!keep_old) {
					for(AccountList<AssetsAccount*>::const_iterator it = assetsAccounts.constBegin(); it != assetsAccounts.constEnd(); ++it) {
						if((*it)->currency() == cur) {keep_old = true; break;}
					}
				}
				if(!keep_old) cur->rates.clear();
				cur->setExchangeRate(exrate * usd_rate);
				cur->setExchangeRateSource(EXCHANGE_RATE_SOURCE_MYCURRENCY_NET);
			} else if(!cur) {
				cur = new Currency(this, code, QString(), name, exrate * usd_rate);
				cur->setExchangeRateSource(EXCHANGE_RATE_SOURCE_MYCURRENCY_NET);
				addCurrency(cur);
			}
			had_data = true;
		}
		i = data.indexOf("class=\'country\'", i);
	}

	if(!had_data) return tr("No exchange rates found.");
	
	return QString();
}

QString Budget::saveCurrencies() {
	
	QString filename = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/currencies.xml";
	
	QFileInfo info(filename);
	if(info.isDir()) {
		return tr("File is a directory");
	}
	info.dir().mkpath(".");

	QSaveFile ofile(filename);
	ofile.open(QIODevice::WriteOnly);
	if(!ofile.isOpen()) {
		ofile.cancelWriting();
		return tr("Couldn't open file for writing");
	}
	QXmlStreamWriter xml(&ofile);
	xml.setCodec("UTF-8");
	xml.setAutoFormatting(true);
	xml.setAutoFormattingIndent(-1);

	xml.writeStartDocument();
	xml.writeStartElement("Eqonomize");
	xml.writeAttribute("version", VERSION);
	
	for(CurrencyList<Currency*>::const_iterator it = currencies.constBegin(); it != currencies.constEnd(); ++it) {
		Currency *currency = *it;
		xml.writeStartElement("currency");
		currency->save(&xml, true);
		xml.writeEndElement();
	}
	xml.writeEndElement();

	if(ofile.error() != QFile::NoError) {
		ofile.cancelWriting();
		return tr("Error while writing file; file was not saved");
	}

	if(!ofile.commit()) {
		return tr("Error while writing file; file was not saved");
	}
	
	return QString();
}

TransactionConversionRateDate Budget::defaultTransactionConversionRateDate() const {return i_tcrd;}
void Budget::setDefaultTransactionConversionRateDate(TransactionConversionRateDate tcrd) {i_tcrd = tcrd;}

QString Budget::loadFile(QString filename, QString &errors, bool *default_currency_created, bool merge, bool rename_duplicate_accounts, bool rename_duplicate_categories, bool rename_duplicate_securities, bool ignore_duplicate_transactions) {

	QFile file(filename);
	if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return tr("Couldn't open %1 for reading").arg(filename);
	} else if(!file.size()) {
		return QString();
	}

	QXmlStreamReader xml(&file);
	if(!xml.readNextStartElement()) return tr("Not a valid Eqonomize! file (XML parse error: \"%1\" at line %2, col %3)").arg(xml.errorString()).arg(xml.lineNumber()).arg(xml.columnNumber());
	if(xml.name() != "EqonomizeDoc") return tr("Invalid root element %1 in XML document").arg(xml.name().toString());

	QStringList s_versions = xml.attributes().value("version").toString().split('.');
	int i_version[] = {0, 0, 0};
	if(s_versions.size() > 0) i_version[0] = s_versions[0].toInt();
	if(s_versions.size() > 1) i_version[1] = s_versions[1].toInt();
	if(s_versions.size() > 2) i_version[2] = s_versions[2].toInt();
	
	qint64 curtime = QDateTime::currentMSecsSinceEpoch() / 1000;

	if(!merge) {
		tags.clear();
		clear();
		i_opened_revision = xml.attributes().value("revision").toInt();
		if(i_opened_revision <= 0) i_opened_revision = 1;
		i_revision = i_opened_revision;
		last_id = xml.attributes().value("lastid").toLongLong();
		if(last_id < 0) last_id = 0;
	}

	errors = QString();
	int category_errors = 0, account_errors = 0, transaction_errors = 0, security_errors = 0;

	assetsAccounts_id[balancingAccount->id()] = balancingAccount;
	
	QHash<qlonglong, qlonglong> merge_transaction_ids;
	QList<Transactions*> update_links_list;
	
	Currency *cur = NULL, *prev_default_cur = default_currency;
	if(default_currency_created) *default_currency_created = false;
	
	bool set_ids = false;
	
	if(!merge) o_sync->clear();
	
	i_budget_month = 1;

	while(xml.readNextStartElement()) {
		if(xml.name() == "budget_period") {
			if(merge) {
				xml.skipCurrentElement();
			} else {
				while(xml.readNextStartElement()) {
					if(xml.name() == "first_day_of_month") {
						QString s_day = xml.readElementText();
						bool ok = true;
						int i_day = s_day.toInt(&ok);
						if(ok) setBudgetDay(i_day);
					} else if(xml.name() == "first_month_of_year") {
						QString s_month = xml.readElementText();
						bool ok = true;
						int i_month = s_month.toInt(&ok);
						if(ok) setBudgetMonth(i_month);
					} else {
						xml.skipCurrentElement();
					}
				}
			}
		} else if(xml.name() == "synchronization") {
			if(merge) {
				xml.skipCurrentElement();
			} else {
				o_sync->clear();
				o_sync->autosync = xml.attributes().value("autosync").toInt();
				o_sync->revision = xml.attributes().value("revision").toInt();
				while(xml.readNextStartElement()) {
					if(xml.name() == "url") {
						o_sync->url = xml.readElementText().trimmed();
					} else if(xml.name() == "download") {
						o_sync->download = xml.readElementText().trimmed();
					} else if(xml.name() == "upload") {
						o_sync->upload = xml.readElementText().trimmed();
					} else {
						xml.skipCurrentElement();
					}
				}
			}
		} else if(xml.name() == "currency") {
			QString cur_code = xml.attributes().value("code").trimmed().toString();
			if(!cur && !cur_code.isEmpty()) {
				cur = findCurrency(cur_code);
				if(cur) {
					default_currency = cur;
				} else {
					cur = new Currency(this, cur_code);
					addCurrency(cur);
					default_currency = cur;
					if(default_currency_created) *default_currency_created = true;
				}
			}
			xml.skipCurrentElement();
		} else if(xml.name() == "schedule") {
			bool valid = true;
			ScheduledTransaction *strans = new ScheduledTransaction(this, &xml, &valid);
			if(valid && merge && ignore_duplicate_transactions) {
				for(ScheduledTransactionList<ScheduledTransaction*>::const_iterator it = scheduledTransactions.constBegin(); it != scheduledTransactions.constEnd(); ++it) {
					if((*it)->date() > strans->date()) break;
					else if(strans->equals(*it, false)) {delete strans; strans = NULL; break;}
				}
			}
			if(valid && strans) {
				if(merge) {
					qlonglong old_id = strans->id();
					strans->setId(getNewId());
					merge_transaction_ids[old_id] = strans->id();
					if(strans->linksCount(false) > 0) update_links_list << strans;
					strans->setId(getNewId());
					strans->setFirstRevision(i_revision);
					strans->setLastRevision(i_revision);
					old_id = strans->transaction()->id();
					strans->transaction()->setId(getNewId());
					merge_transaction_ids[old_id] = strans->transaction()->id();
					strans->transaction()->setFirstRevision(i_revision);
					strans->transaction()->setLastRevision(i_revision);
				} else {
					if(!set_ids) set_ids = strans->id() == 0;
					if(!set_ids) set_ids = strans->transaction()->id() == 0;
				}
				if((i_version[0] < 1 || (i_version[0] == 1 && (i_version[1] < 3 || (i_version[1] == 3 && i_version[2] <= 2)))) && strans->timestamp() > curtime) strans->setTimestamp(strans->timestamp() / 1000000L);
				if((i_version[0] < 1 || (i_version[0] == 1 && (i_version[1] < 3 || (i_version[1] == 3 && i_version[2] <= 4)))) && strans->associatedFile().contains(",")) {
					strans->setAssociatedFile(QString("\"") + strans->associatedFile() + "\"");
				}
				scheduledTransactions.append(strans);
				if(strans->transaction()) {
					if(strans->transactiontype() == TRANSACTION_TYPE_SECURITY_BUY || strans->transactiontype() == TRANSACTION_TYPE_SECURITY_SELL) {
						((SecurityTransaction*) strans->transaction())->security()->scheduledTransactions.append(strans);
					} else if(strans->transactiontype() == TRANSACTION_TYPE_INCOME && ((Income*) strans->transaction())->security()) {
						if(strans->transactionsubtype() == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND) ((Income*) strans->transaction())->security()->scheduledReinvestedDividends.append(strans);
						else ((Income*) strans->transaction())->security()->scheduledDividends.append(strans);
					}
					Transactions *trans = strans->transaction();
					for(int i = 0; i < trans->tagsCount(false); i++) {
						if(!tags.contains(trans->getTag(i))) tags << trans->getTag(i);
					}
					if(trans->generaltype() == GENERAL_TRANSACTION_TYPE_SPLIT) {
						SplitTransaction *split = (SplitTransaction*) trans;
						int c = split->count();
						for(int i = 0; i < c; i++) {
							trans = split->at(i);
							for(int i2 = 0; i2 < trans->tagsCount(false); i2++) {
								if(!tags.contains(trans->getTag(i2))) tags << trans->getTag(i2);
							}
						}
					}
				}
			} else if(!valid) {
				transaction_errors++;
				if(strans) delete strans;
			}
		} else if(xml.name() == "transaction") {
			SplitTransaction *split = NULL;
			Transaction *trans = NULL;
			QStringRef type = xml.attributes().value("type");
			bool valid = true;
			if(type.isEmpty()) {
				QXmlStreamAttributes attr = xml.attributes();
				if(attr.hasAttribute("shares") && attr.hasAttribute("security")) {
					if(attr.hasAttribute("cost")) attr.append("type", "security_buy");
					else if(attr.hasAttribute("income")) attr.append("type", "security_sell");
					else if(attr.hasAttribute("from")) attr.append("type", "security_buy");
					else if(attr.hasAttribute("to")) attr.append("type", "security_sell");
				} else if(attr.hasAttribute("cost") || (attr.hasAttribute("category") && attr.hasAttribute("from"))) {
					attr.append("type", "expense");
				} else if(attr.hasAttribute("income") || (attr.hasAttribute("category") && attr.hasAttribute("to"))) {
					attr.append("type", "income");
				} else if(attr.hasAttribute("amount") || attr.hasAttribute("withdrawal")) {
					attr.append("type", "transfer");
				} else if(attr.hasAttribute("from") && attr.hasAttribute("to")) {
					qlonglong id_from = attr.value("from").toLongLong();
					qlonglong id_to = attr.value("to").toLongLong();
					if(expensesAccounts_id.contains(id_to) || expensesAccounts_id.contains(id_from)) attr.append("type", "expense");
					else if(incomesAccounts_id.contains(id_from) || incomesAccounts_id.contains(id_to)) attr.append("type", "income");
					else if(assetsAccounts_id.contains(id_from) && assetsAccounts_id.contains(id_to)) attr.append("type", "transfer");
				}
				type = attr.value("type");
			}
			if(type == "expense" || type == "refund") {
				Expense *expense = new Expense(this, &xml, &valid);
				if(valid && merge && ignore_duplicate_transactions) {
					for(TransactionList<Expense*>::const_iterator it = expenses.constBegin(); it != expenses.constEnd(); ++it) {
						if((*it)->date() > expense->date()) break;
						else if(expense->equals(*it, false)) {delete expense; expense = NULL; break;}
					}
				}
				if(valid && expense) {
					trans = expense;
					expenses.append(expense);
				} else if(!valid) {
					transaction_errors++;
					if(expense) delete expense;
				}
			} else if(type == "income" || type == "repayment") {
				Income *income = new Income(this, &xml, &valid);
				if(valid && merge && ignore_duplicate_transactions) {
					if(income->security()) {
						for(SecurityTransactionList<Income*>::const_iterator it = income->security()->dividends.constBegin(); it != income->security()->dividends.constEnd(); ++it) {
							if((*it)->date() > income->date()) break;
							else if(income->equals(*it, false)) {delete income; income = NULL; break;}
						}
					} else {
						for(TransactionList<Income*>::const_iterator it = incomes.constBegin(); it != incomes.constEnd(); ++it) {
							if((*it)->date() > income->date()) break;
							else if(income->equals(*it, false)) {delete income; income = NULL; break;}
						}
					}
				}
				if(valid && income) {
					trans = income;
					incomes.append(income);
					if(income->security()) income->security()->dividends.append(income);
				} else if(!valid) {
					transaction_errors++;
					if(income) delete income;
				}
			} else if(type == "dividend") {
				Income *income = new Income(this, &xml, &valid);
				if(!income->security()) valid = false;
				if(valid && merge && ignore_duplicate_transactions) {
					for(SecurityTransactionList<Income*>::const_iterator it = income->security()->dividends.constBegin(); it != income->security()->dividends.constEnd(); ++it) {
						if((*it)->date() > income->date()) break;
						else if(income->equals(*it, false)) {delete income; income = NULL; break;}
					}
				}
				if(valid && income) {
					trans = income;
					incomes.append(income);
					income->security()->dividends.append(income);
				} else if(!valid) {
					transaction_errors++;
					if(income) delete income;
				}
			} else if(type == "reinvested_dividend") {
				ReinvestedDividend *rediv = new ReinvestedDividend(this, &xml, &valid);
				if(valid && merge && ignore_duplicate_transactions) {
					for(SecurityTransactionList<ReinvestedDividend*>::const_iterator it = rediv->security()->reinvestedDividends.constBegin(); it != rediv->security()->reinvestedDividends.constEnd(); ++it) {
						if((*it)->date() > rediv->date()) break;
						else if(rediv->equals(*it, false)) {delete rediv; rediv = NULL; break;}
					}
				}
				if(valid && rediv) {
					trans = rediv;
					incomes.append(rediv);
					rediv->security()->reinvestedDividends.append(rediv);
				} else if(!valid) {
					transaction_errors++;
					if(rediv) delete rediv;
				}
			} else if(type == "security_trade") {
				SecurityTrade *ts = new SecurityTrade(this, &xml, &valid);
				if(valid && merge && ignore_duplicate_transactions) {
					for(SecurityTradeList<SecurityTrade*>::const_iterator it = securityTrades.constBegin(); it != securityTrades.constEnd(); ++it) {
						if((*it)->date > ts->date) break;
						else if(ts->date == (*it)->date && ts->from_shares == (*it)->from_shares && ts->to_shares == (*it)->to_shares && ts->from_security == (*it)->from_security && ts->to_security == (*it)->to_security) {delete ts; ts = NULL; break;}
					}
				}
				if(valid && ts) {
					if(merge) {
						ts->id = getNewId();
						ts->first_revision = i_revision;
						ts->last_revision = i_revision;
					} else {
						if(!set_ids) set_ids = ts->id == 0;
					}
					if((i_version[0] < 1 || (i_version[0] == 1 && (i_version[1] < 3 || (i_version[1] == 3 && i_version[2] <= 2)))) && ts->timestamp > curtime) ts->timestamp = ts->timestamp / 1000000L;
					securityTrades.append(ts);
					ts->from_security->tradedShares.append(ts);
					ts->to_security->tradedShares.append(ts);
				} else if(!valid) {
					transaction_errors++;
					if(ts) delete ts;
				}
				xml.skipCurrentElement();
			} else if(type == "transfer") {
				Transfer *transfer = new Transfer(this, &xml, &valid);
				if(valid && merge && ignore_duplicate_transactions) {
					for(TransactionList<Transfer*>::const_iterator it = transfers.constBegin(); it != transfers.constEnd(); ++it) {
						if((*it)->date() > transfer->date()) break;
						else if(transfer->equals(*it, false)) {delete transfer; transfer = NULL; break;}
					}
				}
				if(valid && transfer) {
					trans = transfer;
					transfers.append(transfer);
				} else if(!valid) {
					transaction_errors++;
					if(transfer) delete transfer;
				}
			} else if(type == "balancing") {
				Transfer *transfer = new Balancing(this, &xml, &valid);
				if(valid && merge && ignore_duplicate_transactions) {
					for(TransactionList<Transfer*>::const_iterator it = transfers.constBegin(); it != transfers.constEnd(); ++it) {
						if((*it)->date() > transfer->date()) break;
						else if(transfer->equals(*it, false)) {delete transfer; transfer = NULL; break;}
					}
				}
				if(valid && transfer) {
					trans = transfer;
					transfers.append(transfer);
				} else if(!valid) {
					transaction_errors++;
					if(transfer) delete transfer;
				}
			} else if(type == "security_buy") {
				SecurityBuy *sectrans = new SecurityBuy(this, &xml, &valid);
				if(valid && merge && ignore_duplicate_transactions) {
					for(SecurityTransactionList<SecurityTransaction*>::const_iterator it = sectrans->security()->transactions.constBegin(); it != sectrans->security()->transactions.constEnd(); ++it) {
						if((*it)->date() > sectrans->date()) break;
						else if(sectrans->equals(*it, false)) {delete sectrans; sectrans = NULL; break;}
					}
				}
				if(valid && sectrans) {
					trans = sectrans;
					securityTransactions.append(sectrans);
					sectrans->security()->transactions.append(sectrans);
				} else if(!valid) {
					transaction_errors++;
					if(sectrans) delete sectrans;
				}
			} else if(type == "security_sell") {
				SecuritySell *sectrans = new SecuritySell(this, &xml, &valid);
				if(valid && merge && ignore_duplicate_transactions) {
					for(SecurityTransactionList<SecurityTransaction*>::const_iterator it = sectrans->security()->transactions.constBegin(); it != sectrans->security()->transactions.constEnd(); ++it) {
						if((*it)->date() > sectrans->date()) break;
						else if(sectrans->equals(*it, false)) {delete sectrans; sectrans = NULL; break;}
					}
				}
				if(valid && sectrans) {
					trans = sectrans;
					securityTransactions.append(sectrans);
					sectrans->security()->transactions.append(sectrans);
				} else if(!valid) {
					transaction_errors++;
					if(sectrans) delete sectrans;
				}
			} else if(type == "multiitem" || type == "split") {
				split = new MultiItemTransaction(this, &xml, &valid);
			} else if(type == "multiaccount") {
				split = new MultiAccountTransaction(this, &xml, &valid);
			} else if(type == "debtpayment") {
				split = new DebtPayment(this, &xml, &valid);
			} else {
				xml.skipCurrentElement();
				transaction_errors++;
			}
			if(split) {
				if(valid && merge && ignore_duplicate_transactions) {
					for(SplitTransactionList<SplitTransaction*>::const_iterator it = splitTransactions.constBegin(); it != splitTransactions.constEnd(); ++it) {
						if((*it)->date() > split->date()) break;
						else if(split->equals(*it, false)) {delete split; split = NULL; break;}
					}
				}
				if(!valid) {
					transaction_errors++;
					if(split) delete split;
					split = NULL;
				} else if(split) {
					if(merge) {
						qlonglong old_id = split->id();
						split->setId(getNewId());
						merge_transaction_ids[old_id] = split->id();
						if(split->linksCount(false) > 0) update_links_list << split;
						split->setFirstRevision(i_revision);
						split->setLastRevision(i_revision);
					} else {
						if(!set_ids) set_ids = split->id() == 0;
					}
					if((i_version[0] < 1 || (i_version[0] == 1 && (i_version[1] < 3 || (i_version[1] == 3 && i_version[2] <= 2)))) && split->timestamp() > curtime) split->setTimestamp(split->timestamp() / 1000000L);
					if((i_version[0] < 1 || (i_version[0] == 1 && (i_version[1] < 3 || (i_version[1] == 3 && i_version[2] <= 4)))) && split->associatedFile().contains(",")) {
						split->setAssociatedFile(QString("\"") + split->associatedFile() + "\"");
					}
					splitTransactions.append(split);
					for(int i = 0; i < split->tagsCount(false); i++) {
						if(!tags.contains(split->getTag(i))) tags << split->getTag(i);
					}
					int c = split->count();
					for(int i = 0; i < c; i++) {
						trans = split->at(i);
						if(merge) {
							qlonglong old_id = trans->id();
							trans->setId(getNewId());
							merge_transaction_ids[old_id] = trans->id();
							if(trans->linksCount(false) > 0) update_links_list << trans;
							trans->setFirstRevision(i_revision);
							trans->setLastRevision(i_revision);
						} else {
							if(!set_ids && split->type() != SPLIT_TRANSACTION_TYPE_LOAN) set_ids = trans->id() == 0;
						}
						switch(trans->type()) {
							case TRANSACTION_TYPE_TRANSFER: {
								transfers.append((Transfer*) trans);
								break;
							}
							case TRANSACTION_TYPE_INCOME: {
								incomes.append((Income*) trans);
								if(((Income*) trans)->security()) ((Income*) trans)->security()->dividends.append((Income*) trans);
								break;
							}
							case TRANSACTION_TYPE_EXPENSE: {
								expenses.append((Expense*) trans);
								break;
							}
							case TRANSACTION_TYPE_SECURITY_BUY: {
								securityTransactions.append((SecurityBuy*) trans);
								((SecurityBuy*) trans)->security()->transactions.append((SecurityBuy*) trans);
								break;
							}
							case TRANSACTION_TYPE_SECURITY_SELL: {
								securityTransactions.append((SecuritySell*) trans);
								((SecuritySell*) trans)->security()->transactions.append((SecuritySell*) trans);
								break;
							}
							default: {}
						}
						if((i_version[0] < 1 || (i_version[0] == 1 && (i_version[1] < 3 || (i_version[1] == 3 && i_version[2] <= 4)))) && trans->associatedFile().contains(",")) {
							trans->setAssociatedFile(QString("\"") + trans->associatedFile() + "\"");
						}
						transactions.append(trans);
						for(int i2 = 0; i2 < trans->tagsCount(false); i2++) {
							if(!tags.contains(trans->getTag(i2))) {
								tags << trans->getTag(i2);
							} else if(split->hasTag(trans->getTag(i2), false)) {
								trans->removeTag(trans->getTag(i2));
							}
						}
					}
				}
			} else if(trans) {
				if(merge) {
					qlonglong old_id = trans->id();
					trans->setId(getNewId());
					merge_transaction_ids[old_id] = trans->id();
					if(trans->linksCount(false) > 0) update_links_list << trans;
					trans->setFirstRevision(i_revision);
					trans->setLastRevision(i_revision);
				} else {
					if(!set_ids) set_ids = trans->id() == 0;
				}
				if((i_version[0] < 1 || (i_version[0] == 1 && (i_version[1] < 3 || (i_version[1] == 3 && i_version[2] <= 2)))) && trans->timestamp() > curtime) trans->setTimestamp(trans->timestamp() / 1000000L);
				if((i_version[0] < 1 || (i_version[0] == 1 && (i_version[1] < 3 || (i_version[1] == 3 && i_version[2] <= 4)))) && trans->associatedFile().contains(",")) {
					trans->setAssociatedFile(QString("\"") + trans->associatedFile() + "\"");
				}
				transactions.append(trans);
				for(int i2 = 0; i2 < trans->tagsCount(false); i2++) {
					if(!tags.contains(trans->getTag(i2))) tags << trans->getTag(i2);
				}
			}
		} else if(xml.name() == "category") {
			QStringRef type = xml.attributes().value("type");
			bool valid = true;
			if(type == "expenses") {
				ExpensesAccount *account = new ExpensesAccount(this, &xml, &valid);
				if(valid) {
					ExpensesAccount *old_account = NULL;
					if(merge) old_account = findExpensesAccount(account->name(), NULL);
					if(!rename_duplicate_categories && old_account) {
						expensesAccounts_id[account->id()] = old_account;
						for(AccountList<CategoryAccount*>::const_iterator it = account->subCategories.constBegin(); it != account->subCategories.constEnd();) {
							CategoryAccount *ca = *it;
							ExpensesAccount *old_sub = findExpensesAccount(ca->name(), old_account);
							if(old_sub) {
								expensesAccounts_id[ca->id()] = old_sub;
								removeAccount(ca, true);
								++it;
							} else {
								old_account->addSubCategory(ca);
							}
						}
						delete account;
					} else {
						expensesAccounts_id[account->id()] = account;
						if(old_account) account->setName(QString("%1 (%2)").arg(account->name()).arg(tr("imported")));
						if(merge) {
							account->setId(getNewId());
							account->setFirstRevision(i_revision);
							account->setLastRevision(i_revision);
						}
						expensesAccounts.append(account);
						accounts.append(account);
					}
				} else {
					category_errors++;
					delete account;
				}
			} else if(type == "incomes") {
				IncomesAccount *account = new IncomesAccount(this, &xml, &valid);
				if(valid) {
					IncomesAccount *old_account = NULL;
					if(merge) old_account = findIncomesAccount(account->name(), NULL);
					if(!rename_duplicate_categories && old_account) {
						incomesAccounts_id[account->id()] = old_account;
						for(AccountList<CategoryAccount*>::const_iterator it = account->subCategories.constBegin(); it != account->subCategories.constEnd();) {
							CategoryAccount *ca = *it;
							IncomesAccount *old_sub = findIncomesAccount(ca->name(), old_account);
							if(old_sub) {
								incomesAccounts_id[ca->id()] = old_sub;
								removeAccount(ca, true);
								++it;
							} else {
								old_account->addSubCategory(ca);
							}
						}
						delete account;
					} else {
						incomesAccounts_id[account->id()] = account;
						if(old_account) account->setName(QString("%1 (%2)").arg(account->name()).arg(tr("imported")));
						if(merge) {
							account->setId(getNewId());
							account->setFirstRevision(i_revision);
							account->setLastRevision(i_revision);
						}
						incomesAccounts.append(account);
						accounts.append(account);
					}
				} else {
					category_errors++;
					delete account;
				}
			} else {
				category_errors++;
				xml.skipCurrentElement();
			}
		} else if(xml.name() == "account") {
			if(!cur) {
				if(merge) {
					cur = default_currency;
				} else {
					bool b = resetDefaultCurrency();
					cur = default_currency;
					if(default_currency_created) *default_currency_created = b;
				}
			}
			bool valid = true;
			AssetsAccount *account = new AssetsAccount(this, &xml, &valid);
			if(valid) {
				AssetsAccount *old_account = NULL;
				if(merge) old_account = findAssetsAccount(account->name());
				if(!rename_duplicate_accounts && old_account) {
					assetsAccounts_id[account->id()] = old_account;
				} else {
					assetsAccounts_id[account->id()] = account;
					if(old_account) account->setName(QString("%1 (%2)").arg(account->name()).arg(tr("imported")));
					if(merge) {
						account->setId(getNewId());
						account->setFirstRevision(i_revision);
						account->setLastRevision(i_revision);
					}
					assetsAccounts.append(account);
					accounts.append(account);
				}
			} else {
				account_errors++;
				delete account;
			}
		
		} else if(xml.name() == "security") {
			bool valid = true;
			Security *security = new Security(this, &xml, &valid);
			if(valid) {
				Security *old_security = NULL;
				if(merge) old_security = findSecurity(security->name());
				if(!rename_duplicate_securities && old_security) {
					securities_id[security->id()] = old_security;
				} else {
					securities_id[security->id()] = security;
					if(old_security)security->setName(QString("%1 (%2)").arg(security->name()).arg(tr("imported")));
					if(merge) {
						security->setId(getNewId());
						security->setFirstRevision(i_revision);
						security->setLastRevision(i_revision);
					}
					securities.append(security);
					i_quotation_decimals = security->quotationDecimals();
					i_share_decimals = security->decimals();
				}
			} else {
				security_errors++;
				delete security;
			}
		} else {
			if(!errors.isEmpty()) errors += '\n';
			errors += tr("Unknown XML element: \"%1\" at line %2, col %3").arg(xml.name().toString()).arg(xml.lineNumber()).arg(xml.columnNumber());
			xml.skipCurrentElement();
		}
	}

	if(!cur && !merge) {
		bool b = resetDefaultCurrency();
		cur = defaultCurrency();
		if(default_currency_created) *default_currency_created = b;
	}
	
	if(merge) default_currency = prev_default_cur;

	if (xml.hasError()) {
		if(!errors.isEmpty()) errors += '\n';
		errors += tr("XML parse error: \"%1\" at line %2, col %3").arg(xml.errorString()).arg(xml.lineNumber()).arg(xml.columnNumber());
	}

	incomesAccounts_id.clear();
	expensesAccounts_id.clear();
	assetsAccounts_id.clear();
	securities_id.clear();
	
	if(set_ids) {
		std::sort(transactions.begin(), transactions.end(), transaction_list_less_than_stamp);
		std::sort(scheduledTransactions.begin(), scheduledTransactions.end(), schedule_list_less_than_stamp);
		std::sort(splitTransactions.begin(), splitTransactions.end(), split_list_less_than_stamp);
		std::sort(securityTrades.begin(), securityTrades.end(), trade_list_less_than_stamp);
		TransactionList<Transaction*>::const_iterator it1 = transactions.constBegin();
		ScheduledTransactionList<ScheduledTransaction*>::const_iterator it2 = scheduledTransactions.constBegin();
		SplitTransactionList<SplitTransaction*>::const_iterator it3 = splitTransactions.constBegin();
		SecurityTradeList<SecurityTrade*>::const_iterator it4 = securityTrades.constBegin();
		int it_i = 0;
		while(true) {
			while((it_i == 0 || it_i == 1) && it1 != transactions.constEnd() && ((*it1)->id() != 0 || ((*it1)->parentSplit() && (*it1)->parentSplit()->type() == SPLIT_TRANSACTION_TYPE_LOAN))) ++it1;
			while((it_i == 0 || it_i == 2) && it2 != scheduledTransactions.constEnd() && (*it2)->id() != 0) ++it2;
			while((it_i == 0 || it_i == 3) && it3 != splitTransactions.constEnd() && (*it3)->id() != 0) ++it3;
			while((it_i == 0 || it_i == 4) && it4 != securityTrades.constEnd() && (*it4)->id != 0) ++it4;
			it_i = 4;
			if(it1 != transactions.constEnd() && (it2 == scheduledTransactions.constEnd() || transactions_less_than_stamp(*it1, *it2))) {
				if(it3 == splitTransactions.constEnd() || transactions_less_than_stamp(*it1, *it3)) {
					if(it4 == securityTrades.constEnd() || transactions_less_than_trade_stamp(*it1, *it4)) it_i = 1;
				} else if(it4 == securityTrades.constEnd() || transactions_less_than_trade_stamp(*it3, *it4)) it_i = 3;
			} else if(it2 != scheduledTransactions.constEnd() && (it3 == splitTransactions.constEnd() || transactions_less_than_stamp(*it2, *it3))) {
				if(it4 == securityTrades.constEnd() || transactions_less_than_trade_stamp(*it2, *it4)) it_i = 2;
			} else if(it3 != splitTransactions.constEnd() && (it4 == securityTrades.constEnd() || transactions_less_than_trade_stamp(*it3, *it4))) it_i = 3;
			else if(it4 == securityTrades.constEnd()) break;
			last_id++;
			if(it_i == 1) {
				(*it1)->setId(last_id);
				++it1;
			} else if(it_i == 2) {
				(*it2)->setId(last_id);
				if((*it2)->transaction()->id() == 0) {
					last_id++;
					(*it2)->transaction()->setId(last_id);
				}
				++it2;
			} else if(it_i == 3) {
				(*it3)->setId(last_id);
				++it3;
			} else if(it_i == 4) {
				(*it4)->id = last_id;
				++it4;
			}
		}
	}
	
	for(int i = 0; i < update_links_list.count(); i++) {
		Transactions *trans = update_links_list.at(i);
		int n = trans->linksCount(false);
		for(int i2 = 0; i2 < n; i2++) {
			qlonglong new_id = merge_transaction_ids[trans->getLinkId(0, false)];
			trans->removeLink(0);
			trans->addLinkId(new_id);
		}
	}
	
	i_revision++;

	expenses.sort();
	incomes.sort();
	transfers.sort();
	securityTransactions.sort();
	securityTrades.sort();
	for(SecurityList<Security*>::const_iterator it = securities.constBegin(); it != securities.constEnd(); ++it) {
		Security *security = *it;
		security->dividends.sort();
		security->transactions.sort();
		security->scheduledTransactions.sort();
		security->scheduledDividends.sort();
		security->scheduledReinvestedDividends.sort();
		security->reinvestedDividends.sort();
		security->tradedShares.sort();
	}
	transactions.sort();
	scheduledTransactions.sort();
	splitTransactions.sort();
	expensesAccounts.sort();
	incomesAccounts.sort();
	assetsAccounts.sort();
	accounts.sort();
	securities.sort();

	tags.sort(Qt::CaseInsensitive);
	
	if(account_errors > 0) {
		if(!errors.isEmpty()) errors += '\n';
		errors += tr("Unable to load %n account(s).", "", account_errors);
	}
	if(category_errors > 0) {
		if(!errors.isEmpty()) errors += '\n';
		errors += tr("Unable to load %n category/categories.", "", category_errors);
	}
	if(security_errors > 0) {
		if(!errors.isEmpty()) errors += '\n';
		errors += tr("Unable to load %n security/securities.", "Financial security (e.g. stock, mutual fund)", security_errors);
	}
	if(transaction_errors > 0) {
		if(!errors.isEmpty()) errors += '\n';
		errors += tr("Unable to load %n transaction(s).", "", transaction_errors);
	}
	file.close();

	resetDefaultCurrencyChanged();
	return QString();
}
int Budget::fileRevision(QString filename, QString &error) const {

	QFile file(filename);
	if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		error = tr("Couldn't open %1 for reading").arg(filename);
		return -1;
	} else if(!file.size()) {
		return -1;
	}

	QXmlStreamReader xml(&file);
	if(!xml.readNextStartElement()) {
		error = tr("Not a valid Eqonomize! file (XML parse error: \"%1\" at line %2, col %3)").arg(xml.errorString()).arg(xml.lineNumber()).arg(xml.columnNumber());
		return -1;
	}
	if(xml.name() != "EqonomizeDoc") {
		error = tr("Invalid root element %1 in XML document").arg(xml.name().toString());
		return -1;
	}

	int file_revision = xml.attributes().value("revision").toInt();
	if(file_revision <= 0) file_revision = 1;
	file.close();

	error = QString();
	
	return file_revision;

}
bool Budget::isUnsynced(QString filename, QString &error, int synced_revision) const {

	if(synced_revision < 0) synced_revision = i_opened_revision;

	QFile file(filename);
	if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		error = tr("Couldn't open %1 for reading").arg(filename);
		return false;
	} else if(!file.size()) {
		return false;
	}

	QXmlStreamReader xml(&file);
	if(!xml.readNextStartElement()) {
		error = tr("Not a valid Eqonomize! file (XML parse error: \"%1\" at line %2, col %3)").arg(xml.errorString()).arg(xml.lineNumber()).arg(xml.columnNumber());
		return false;
	}
	if(xml.name() != "EqonomizeDoc") {
		error = tr("Invalid root element %1 in XML document").arg(xml.name().toString());
		return false;
	}

	int file_revision = xml.attributes().value("revision").toInt();
	if(file_revision <= 0) file_revision = 1;
	file.close();
	
	error = QString();
	
	return file_revision > synced_revision;

}
void Budget::cancelSync() {
	if(syncReply) syncReply->abort();
	if(syncProcess) syncProcess->terminate();
}
bool Budget::sync(QString &error, QString &errors, bool do_upload, bool on_load) {
	if(!o_sync->isComplete()) {
		return false;
	}
	syncProcess = NULL;
	syncReply = NULL;
	QTemporaryFile file;
	file.open();
	QString perror;
	if(!o_sync->download.isEmpty()) {
		file.close();
		QString command = o_sync->download;
		command.replace("%f", file.fileName());
		command.replace("%u", o_sync->url);
		QEventLoop loop;
		syncProcess = new QProcess();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
		QStringList list = QProcess::splitCommand(command);
		if(list.isEmpty()) return "Empty command";
		syncProcess->start(list.takeFirst(), list);
#else
		syncProcess->start(command);
#endif
		QObject::connect(syncProcess, SIGNAL(finished(int, QProcess::ExitStatus)), &loop, SLOT(quit()));
		loop.exec();
		if(syncProcess->exitStatus() != QProcess::NormalExit || syncProcess->exitCode() != 0) {
			error = tr("Download command (%1) failed: %2.").arg(command).arg(syncProcess->errorString());
			syncProcess->deleteLater();
			syncProcess = NULL;
			return false;
		}
		perror = syncProcess->errorString();
		syncProcess->deleteLater();
		syncProcess = NULL;
	} else {
		QEventLoop loop;
		syncReply = nam.get(QNetworkRequest(QUrl(o_sync->url)));
		QObject::connect(syncReply, SIGNAL(finished()), &loop, SLOT(quit()));
		loop.exec();
		if(syncReply->error() == QNetworkReply::OperationCanceledError) {
			//canceled by user
			syncReply->deleteLater();
			syncReply = NULL;
			return false;
		} else if(syncReply->error() != QNetworkReply::NoError) {
			syncReply->abort();
			error = tr("Failed to download file from %1: %2.").arg(o_sync->url).arg(syncReply->errorString());
			syncReply->deleteLater();
			syncReply = NULL;
			return false;
		} else {
			file.write(syncReply->readAll());
			file.close();
		}
		syncReply->deleteLater();
		syncReply = NULL;
	}
	int file_revision = fileRevision(file.fileName(), error);
	if(!error.isNull() || file_revision <= o_sync->revision) {
		if(error.isNull() && file_revision < (on_load ? i_revision : i_opened_revision)) {
			int saved_revision = i_revision;
			if(on_load) i_revision = i_opened_revision;
			error = syncUpload(file.fileName());
			i_revision = saved_revision;
		}
		if(!error.isNull() && !perror.isEmpty()) {error += "\n\n"; error += perror;}
		return false;
	}
	error = syncFile(file.fileName(), errors, o_sync->revision);
	if(!error.isNull()) {
		if(!perror.isEmpty()) {error += "\n\n"; error += perror;}
		return false;
	}
	o_sync->revision = file_revision;

	//upload
	if(do_upload) {
		error = syncUpload(file.fileName());
		if(!error.isNull()) return true;
	}

	return true;
}
QString Budget::syncUpload(QString filename) {
	int rev_bak = o_sync->revision;
	o_sync->revision = i_revision;
	QString error = saveFile(filename, QFile::ReadUser | QFile::WriteUser, true);
	o_sync->revision = rev_bak;
	if(!error.isNull()) {
		return error;
	}
	QString command = o_sync->upload;
	command.replace("%f", filename);
	command.replace("%u", o_sync->url);
	QEventLoop loop;
	syncProcess = new QProcess();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
	QStringList list = QProcess::splitCommand(command);
	if(list.isEmpty()) return "Empty command";
	syncProcess->start(list.takeFirst(), list);
#else
	syncProcess->start(command);
#endif
	QObject::connect(syncProcess, SIGNAL(finished(int, QProcess::ExitStatus)), &loop, SLOT(quit()));
	loop.exec();
	if(syncProcess->exitStatus() != QProcess::NormalExit || syncProcess->exitCode() != 0) {
		error = tr("Upload command (%1) failed: %2.").arg(command).arg(syncProcess->errorString());
	} else {
		o_sync->revision = i_revision;
	}
	
	syncProcess->deleteLater();
	syncProcess = NULL;
	return error;
}

bool Budget::autosyncEnabled() const {return o_sync->autosync && o_sync->isComplete();}

QString Budget::syncFile(QString filename, QString &errors, int synced_revision) {

	if(synced_revision < 0) synced_revision = i_opened_revision;

	QFile file(filename);
	if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return tr("Couldn't open %1 for reading").arg(filename);
	} else if(!file.size()) {
		return QString();
	}

	QXmlStreamReader xml(&file);
	if(!xml.readNextStartElement()) return tr("Not a valid Eqonomize! file (XML parse error: \"%1\" at line %2, col %3)").arg(xml.errorString()).arg(xml.lineNumber()).arg(xml.columnNumber());
	if(xml.name() != "EqonomizeDoc") return tr("Invalid root element %1 in XML document").arg(xml.name().toString());

	/*QString s_version = xml.attributes().value("version").toString();
	float f_version = s_version.toFloat();*/

	int file_revision = xml.attributes().value("revision").toInt();
	if(file_revision <= 0) file_revision = 1;
	qlonglong file_last_id = xml.attributes().value("lastid").toLongLong();
	if(file_last_id < 0) file_last_id = 0;
	
	int revision_diff = file_revision - synced_revision;
	if(revision_diff <= 0) {
		file.close();
		return QString();
	}
	
	last_id = file_last_id;
	
	i_revision += revision_diff;
	i_opened_revision = i_revision;
	
	errors = QString();
	int category_errors = 0, account_errors = 0, transaction_errors = 0, security_errors = 0;

	assetsAccounts_id[balancingAccount->id()] = balancingAccount;
	
	QList<Transactions*> update_ids_list;
	
	QHash<qlonglong, IncomesAccount*> old_incomesAccounts_id;
	QHash<qlonglong, ExpensesAccount*> old_expensesAccounts_id;
	QHash<qlonglong, AssetsAccount*> old_assetsAccounts_id;
	QHash<qlonglong, Security*> old_securities_id;
	for(AccountList<AssetsAccount*>::const_iterator it = assetsAccounts.constBegin(); it != assetsAccounts.constEnd(); ++it) {
		if((*it)->lastRevision() > synced_revision) (*it)->setLastRevision((*it)->lastRevision() + revision_diff);
		if((*it)->firstRevision() > synced_revision) {
			(*it)->setFirstRevision((*it)->firstRevision() + revision_diff);
			last_id++; (*it)->setId(last_id);
		} else {
			old_assetsAccounts_id[(*it)->id()] = *it;
		}
	}
	for(AccountList<IncomesAccount*>::const_iterator it = incomesAccounts.constBegin(); it != incomesAccounts.constEnd(); ++it) {
		if((*it)->lastRevision() > synced_revision) (*it)->setLastRevision((*it)->lastRevision() + revision_diff);
		if((*it)->firstRevision() > synced_revision) {
			(*it)->setFirstRevision((*it)->firstRevision() + revision_diff);
			last_id++; (*it)->setId(last_id);
		} else {
			old_incomesAccounts_id[(*it)->id()] = *it;
		}
	}
	for(AccountList<ExpensesAccount*>::const_iterator it = expensesAccounts.constBegin(); it != expensesAccounts.constEnd(); ++it) {
		if((*it)->lastRevision() > synced_revision) (*it)->setLastRevision((*it)->lastRevision() + revision_diff);
		if((*it)->firstRevision() > synced_revision) {
			(*it)->setFirstRevision((*it)->firstRevision() + revision_diff);
			last_id++; (*it)->setId(last_id);
		} else {
			old_expensesAccounts_id[(*it)->id()] = *it;
		}
	}
	for(SecurityList<Security*>::const_iterator it = securities.constBegin(); it != securities.constEnd(); ++it) {
		if((*it)->lastRevision() > synced_revision) (*it)->setLastRevision((*it)->lastRevision() + revision_diff);
		if((*it)->firstRevision() > synced_revision) {
			(*it)->setFirstRevision((*it)->firstRevision() + revision_diff);
			last_id++; (*it)->setId(last_id);
		} else {
			old_securities_id[(*it)->id()] = *it;
		}
	}
	QHash<qlonglong, ScheduledTransaction*> scheduleds_id, file_scheduleds_id;
	QHash<qlonglong, Transactions*> scheduleds_trans_id, file_scheduleds_trans_id;
	QHash<qlonglong, Transactions*> linked_transactions;
	for(ScheduledTransactionList<ScheduledTransaction*>::const_iterator it = scheduledTransactions.constBegin(); it != scheduledTransactions.constEnd(); ++it) {
		if((*it)->lastRevision() > synced_revision) (*it)->setLastRevision((*it)->lastRevision() + revision_diff);
		if((*it)->linksCount(false) > 0) linked_transactions[(*it)->id()] = *it;
		if((*it)->firstRevision() > synced_revision) {
			update_ids_list << (*it);
		} else {
			scheduleds_id[(*it)->id()] = *it;
		}
		if((*it)->transaction()->lastRevision() > synced_revision) (*it)->transaction()->setLastRevision((*it)->transaction()->lastRevision() + revision_diff);
		if((*it)->transaction()->linksCount(false) > 0) linked_transactions[(*it)->transaction()->id()] = (*it)->transaction();
		if((*it)->transaction()->firstRevision() > synced_revision) {
			update_ids_list << (*it)->transaction();
		}
		if((*it)->lastRevision() > synced_revision && (*it)->transaction()->firstRevision() <= synced_revision) scheduleds_trans_id[(*it)->transaction()->id()] = (*it)->transaction();
	}
	QHash<qlonglong, SplitTransaction*> splits_id, file_splits_id;
	for(SplitTransactionList<SplitTransaction*>::const_iterator it = splitTransactions.constBegin(); it != splitTransactions.constEnd(); ++it) {
		if((*it)->lastRevision() > synced_revision) (*it)->setLastRevision((*it)->lastRevision() + revision_diff);
		if((*it)->linksCount(false) > 0) linked_transactions[(*it)->id()] = *it;
		if((*it)->firstRevision() > synced_revision) {
			update_ids_list << (*it);
		}
		splits_id[(*it)->id()] = *it;
	}
	QHash<qlonglong, SecurityTrade*> securitytrades_id, file_securitytrades_id;
	for(SecurityTradeList<SecurityTrade*>::const_iterator it = securityTrades.constBegin(); it != securityTrades.constEnd(); ++it) {
		if((*it)->last_revision > synced_revision) (*it)->last_revision += revision_diff;
		if((*it)->first_revision > synced_revision) {
			(*it)->first_revision = (*it)->first_revision + revision_diff;
			last_id++; (*it)->id = last_id;
		} else {
			securitytrades_id[(*it)->id] = *it;
		}
	}
	QHash<qlonglong, Transaction*> transactions_id, file_transactions_id;
	for(TransactionList<Transaction*>::const_iterator it = transactions.constBegin(); it != transactions.constEnd(); ++it) {
		if((*it)->lastRevision() > synced_revision) (*it)->setLastRevision((*it)->lastRevision() + revision_diff);
		bool b_loan = (*it)->parentSplit() && (*it)->parentSplit()->type() == SPLIT_TRANSACTION_TYPE_LOAN;
		if(!b_loan && (*it)->linksCount(false) > 0) linked_transactions[(*it)->id()] = *it;
		if((*it)->firstRevision() > synced_revision) {
			if(!b_loan) {
				update_ids_list << (*it);
			} else {
				(*it)->setFirstRevision((*it)->firstRevision() + revision_diff);
			}
		} else if(!b_loan) {
			transactions_id[(*it)->id()] = *it;
		}
	}

	for(int i = 0; i < update_ids_list.count(); i++) {
		Transactions *trans = update_ids_list.at(i);
		trans->setFirstRevision(trans->firstRevision() + revision_diff);
		last_id++;
		qlonglong old_id = trans->id();
		if(old_id > 0 && trans->linksCount(false) > 0) {
			for(int i = 0; i < trans->linksCount(); i++) {
				qlonglong lid = trans->getLinkId(i, false);
				QHash<qlonglong, Transactions*>::iterator it = linked_transactions.find(lid);
			 	if(it != linked_transactions.end()) {
					(*it)->removeLink(trans);
					(*it)->addLinkId(last_id);
				}
			}
		}
		trans->setId(last_id);
	}

	QList<Account*> deleted_accounts;
	QList<Security*> deleted_securities;

	while(xml.readNextStartElement()) {
		if(xml.name() == "budget_period") {
			xml.skipCurrentElement();
		} else if(xml.name() == "currency") {
			xml.skipCurrentElement();
		} else if(xml.name() == "synchronization") {
			xml.skipCurrentElement();
		} else if(xml.name() == "schedule") {
			bool valid = true;
			ScheduledTransaction *strans = new ScheduledTransaction(this, &xml, &valid);
			if(valid && strans) {
				QHash<qlonglong, ScheduledTransaction*>::iterator it = scheduleds_id.find(strans->id());
			 	if(it == scheduleds_id.end()) {
			 		if(strans->lastRevision() > synced_revision) {
			 			QHash<qlonglong, Transaction*>::iterator it2 = transactions_id.find(strans->transaction()->id());
					 	if(it2 != transactions_id.end() && (*it2)->lastRevision() > strans->lastRevision()) {
					 		delete strans;
					 	} else {
					 		QHash<qlonglong, SplitTransaction*>::iterator it3 = splits_id.find(strans->transaction()->id());
						 	if(it3 != splits_id.end() && (*it3)->lastRevision() > strans->lastRevision()) {
						 		delete strans;
						 	} else {
					 			addScheduledTransaction(strans);
					 		}
				 		}
			 		} else delete strans;
			 	} else {
			 		if((*it)->lastRevision() >= strans->lastRevision()) {
			 			delete strans;
			 		} else {
			 			removeScheduledTransaction(*it);
						addScheduledTransaction(strans);
			 		}
			 		scheduleds_id.remove(it.key());
			 	}
			} else if(!valid) {
				transaction_errors++;
				if(strans) delete strans;
			}
		} else if(xml.name() == "transaction") {
			SplitTransaction *split = NULL;
			Transaction *trans = NULL;
			QStringRef type = xml.attributes().value("type");
			bool valid = true;
			if(type == "expense" || type == "refund") {
				trans = new Expense(this, &xml, &valid);
			} else if(type == "income" || type == "repayment") {
				trans = new Income(this, &xml, &valid);
			} else if(type == "dividend") {
				trans = new Income(this, &xml, &valid);
				if(!((Income*) trans)->security()) valid = false;
			} else if(type == "reinvested_dividend") {
				trans = new ReinvestedDividend(this, &xml, &valid);
			} else if(type == "security_trade") {
				SecurityTrade *ts = new SecurityTrade(this, &xml, &valid);
				if(valid && ts) {
					QHash<qlonglong, SecurityTrade*>::iterator it = securitytrades_id.find(ts->id);
				 	if(it == securitytrades_id.end()) {
				 		if(ts->last_revision > synced_revision) addSecurityTrade(ts);
				 		else delete ts;
				 	} else {
				 		if((*it)->last_revision >= ts->last_revision) {
				 			delete ts;
				 		} else {
				 			removeSecurityTrade(*it);
							addSecurityTrade(ts);
				 		}
				 		securitytrades_id.remove(it.key());
				 	}
				} else {
					transaction_errors++;
					if(ts) delete ts;
				}
				xml.skipCurrentElement();
			} else if(type == "transfer") {
				trans = new Transfer(this, &xml, &valid);
			} else if(type == "balancing") {
				trans = new Balancing(this, &xml, &valid);
			} else if(type == "security_buy") {
				trans = new SecurityBuy(this, &xml, &valid);
			} else if(type == "security_sell") {
				trans = new SecuritySell(this, &xml, &valid);
			} else if(type == "multiitem" || type == "split") {
				split = new MultiItemTransaction(this, &xml, &valid);
			} else if(type == "multiaccount") {
				split = new MultiAccountTransaction(this, &xml, &valid);
			} else if(type == "debtpayment") {
				split = new DebtPayment(this, &xml, &valid);
			}
			if(split) {
				if(!valid || !split) {
					transaction_errors++;
					if(split) delete split;
					split = NULL;
				} else {
					if(split) {
						QHash<qlonglong, SplitTransaction*>::iterator it = splits_id.find(split->id());
						if(it == splits_id.end()) {
							if(split->lastRevision() > synced_revision && !scheduleds_trans_id.contains(split->id())) {
								addSplitTransaction(split);
							} else {
								if(split->type() != SPLIT_TRANSACTION_TYPE_LOAN) split->clear(true);
								delete split;
							}
						} else {
							if((*it)->lastRevision() >= split->lastRevision()) {
								if(split->type() != SPLIT_TRANSACTION_TYPE_LOAN) split->clear(true);
								delete split;
								transactions_id.remove(it.key());
							} else {
								if((*it)->type() != SPLIT_TRANSACTION_TYPE_LOAN) (*it)->clear(true);
								removeSplitTransaction(*it);
								if(split->type() != SPLIT_TRANSACTION_TYPE_LOAN) {
									int c = split->count();
									for(int i = 0; i < c; i++) {
										trans = split->at(i);
										QHash<qlonglong, Transaction*>::iterator it = transactions_id.find(trans->id());
										if(it == transactions_id.end()) {
											if(trans->lastRevision() > synced_revision && !scheduleds_trans_id.contains(trans->id())) addTransaction(trans);
											else {split->removeTransaction(trans);  i--; c--;}
										} else {
											if((*it)->lastRevision() > synced_revision && (*it)->lastRevision() >= trans->lastRevision()) {
												split->removeTransaction(trans); i--; c--;
											} else {
												removeTransaction(*it);
												addTransaction(trans);
											}
											transactions_id.remove(it.key());
										}
									}
									if(c <= 1) {
										split->clear(true);
										delete split;
										split = NULL;
									}
								}
								if(split) splitTransactions.inSort(split);
							}
							splits_id.remove(it.key());
						}
					}
				}
			} else if(trans) {
				if(!valid) {
					transaction_errors++;
				} else {
				 	QHash<qlonglong, Transaction*>::iterator it = transactions_id.find(trans->id());
				 	if(it == transactions_id.end()) {
				 		if(trans->lastRevision() > synced_revision && !scheduleds_trans_id.contains(trans->id())) {
				 			addTransaction(trans);
				 		} else {
				 			delete trans;
				 		}
				 	} else {
				 		if((*it)->lastRevision() >= trans->lastRevision()) {
				 			delete trans;
				 		} else {
				 			removeTransaction(*it);
							addTransaction(trans);
				 		}
				 		transactions_id.remove(it.key());
				 	}
				}
			}
		} else if(xml.name() == "category") {
			QStringRef type = xml.attributes().value("type");
			bool valid = true;
			if(type == "expenses") {
				ExpensesAccount *account = new ExpensesAccount(this, &xml, &valid);
				if(valid) {
					QHash<qlonglong, ExpensesAccount*>::iterator it = old_expensesAccounts_id.find(account->id());
					if(it != old_expensesAccounts_id.end()) {
						if(account->lastRevision() > (*it)->lastRevision()) {
							if((*it)->lastRevision() > synced_revision) (*it)->setMergeBudgets(account);
							else (*it)->set(account);
						} else if(account->lastRevision() > synced_revision) {
							(*it)->mergeBudgets(account, true);
						}
						expensesAccounts_id[account->id()] = *it;
						QList<Account*> delete_subs;
						for(AccountList<CategoryAccount*>::const_iterator it2 = account->subCategories.constBegin(); it2 != account->subCategories.constEnd(); ++it2) {
							QHash<qlonglong, ExpensesAccount*>::iterator it3 = old_expensesAccounts_id.find((*it2)->id());
							if(it3 != old_expensesAccounts_id.end()) {
								if((*it2)->lastRevision() > (*it3)->lastRevision()) {
									if((*it2)->lastRevision() > synced_revision) (*it3)->setMergeBudgets(*it2);
									else (*it3)->set(*it2);
								} else if((*it2)->lastRevision() > synced_revision) {
									(*it3)->mergeBudgets(*it2, true);
								}
								expensesAccounts_id[(*it2)->id()] = *it3;
								delete_subs << *it2;
							} else {
								if(!(*it)->subCategories.contains(*it2)) (*it)->addSubCategory(*it2);
								if((*it2)->lastRevision() <= synced_revision) {
									deleted_accounts.append(*it2);
								}
							}
						}
						for(QList<Account*>::const_iterator it2 = delete_subs.constBegin(); it2 != delete_subs.constEnd(); ++it2) {
							removeAccount(*it2);
						}
						(*it)->subCategories.sort();
						delete account;
					} else {
						expensesAccounts_id[account->id()] = account;
						expensesAccounts.append(account);
						accounts.append(account);
						if(account->lastRevision() <= synced_revision) {
							deleted_accounts.append(account);
						}
						QList<Account*> delete_subs;
						for(AccountList<CategoryAccount*>::const_iterator it2 = account->subCategories.constBegin(); it2 != account->subCategories.constEnd(); ++it2) {
							QHash<qlonglong, ExpensesAccount*>::iterator it3 = old_expensesAccounts_id.find((*it2)->id());
							if(it3 != old_expensesAccounts_id.end()) {
								if((*it2)->lastRevision() > (*it3)->lastRevision()) {
									if((*it2)->lastRevision() > synced_revision) (*it3)->setMergeBudgets(*it2);
									else (*it3)->set(*it2);
								} else if((*it2)->lastRevision() > synced_revision) {
									(*it3)->mergeBudgets(*it2, true);
								}
								expensesAccounts_id[(*it2)->id()] = *it3;
								delete_subs << *it2;
							}
						}
						for(QList<Account*>::const_iterator it2 = delete_subs.constBegin(); it2 != delete_subs.constEnd(); ++it2) {
							removeAccount(*it2);
						}
						account->subCategories.sort();
					}
				} else if(!valid) {
					category_errors++;
					delete account;
				}
			} else if(type == "incomes") {
				IncomesAccount *account = new IncomesAccount(this, &xml, &valid);
				if(valid) {
					QHash<qlonglong, IncomesAccount*>::iterator it = old_incomesAccounts_id.find(account->id());
					if(it != old_incomesAccounts_id.end()) {
						if(account->lastRevision() > (*it)->lastRevision()) {
							if((*it)->lastRevision() > synced_revision) (*it)->setMergeBudgets(account);
							else (*it)->set(account);
						} else if(account->lastRevision() > synced_revision) {
							(*it)->mergeBudgets(account, true);
						}
						incomesAccounts_id[account->id()] = *it;
						QList<Account*> delete_subs;
						for(AccountList<CategoryAccount*>::const_iterator it2 = account->subCategories.constBegin(); it2 != account->subCategories.constEnd(); ++it2) {
							QHash<qlonglong, IncomesAccount*>::iterator it3 = old_incomesAccounts_id.find((*it2)->id());
							if(it3 != old_incomesAccounts_id.end()) {
								if((*it2)->lastRevision() > (*it3)->lastRevision()) {
									if((*it2)->lastRevision() > synced_revision) (*it3)->setMergeBudgets(*it2);
									else (*it3)->set(*it2);
								} else if((*it2)->lastRevision() > synced_revision) {
									(*it3)->mergeBudgets(*it2, true);
								}
								delete_subs << *it2;
							} else {
								if(!(*it)->subCategories.contains(*it2)) (*it)->addSubCategory(*it2);
								if((*it2)->lastRevision() <= synced_revision) {
									deleted_accounts.append(*it2);
								}
							}
						}
						for(QList<Account*>::const_iterator it2 = delete_subs.constBegin(); it2 != delete_subs.constEnd(); ++it2) {
							removeAccount(*it2);
						}
						(*it)->subCategories.sort();
						delete account;
					} else {
						incomesAccounts_id[account->id()] = account;
						if(account->lastRevision() <= synced_revision) {
							deleted_accounts.append(account);
						}
						incomesAccounts.append(account);
						accounts.append(account);
						QList<Account*> delete_subs;
						for(AccountList<CategoryAccount*>::const_iterator it2 = account->subCategories.constBegin(); it2 != account->subCategories.constEnd(); ++it2) {
							QHash<qlonglong, IncomesAccount*>::iterator it3 = old_incomesAccounts_id.find((*it2)->id());
							if(it3 != old_incomesAccounts_id.end()) {
								if((*it2)->lastRevision() > (*it3)->lastRevision()) {
									if((*it2)->lastRevision() > synced_revision) (*it3)->setMergeBudgets(*it2);
									else (*it3)->set(*it2);
								} else if((*it2)->lastRevision() > synced_revision) {
									(*it3)->mergeBudgets(*it2, true);
								}
								delete_subs << *it2;
							}
						}
						for(QList<Account*>::const_iterator it2 = delete_subs.constBegin(); it2 != delete_subs.constEnd(); ++it2) {
							removeAccount(*it2);
						}
						account->subCategories.sort();
					}
				} else if(!valid) {
					category_errors++;
					delete account;
				}
			}
		} else if(xml.name() == "account") {
			bool valid = true;
			AssetsAccount *account = new AssetsAccount(this, &xml, &valid);
			if(valid) {
				QHash<qlonglong, AssetsAccount*>::iterator it = old_assetsAccounts_id.find(account->id());
				if(it != old_assetsAccounts_id.end()) {
					if(account->lastRevision() > (*it)->lastRevision()) {
						(*it)->set(account);
					}
					assetsAccounts_id[account->id()] = *it;
					delete account;
				} else {
					assetsAccounts_id[account->id()] = account;
					if(account->lastRevision() <= synced_revision) {
						deleted_accounts.append(account);
					}
					assetsAccounts.append(account);
					accounts.append(account);
				}
			} else if(!valid) {
				category_errors++;
				delete account;
			}
		} else if(xml.name() == "security") {
			bool valid = true;
			Security *security = new Security(this, &xml, &valid);
			if(valid) {
				QHash<qlonglong, Security*>::iterator it = old_securities_id.find(security->id());
				if(it != old_securities_id.end()) {
					if(security->lastRevision() > (*it)->lastRevision()) {
						if((*it)->lastRevision() > synced_revision) (*it)->setMergeQuotes(security);
						else (*it)->set(security);
					} else if(security->lastRevision() > synced_revision) {
						(*it)->mergeQuotes(security, true);
					}
					securities_id[security->id()] = *it;
					delete security;
				} else {
					securities_id[security->id()] = security;
					if(security->lastRevision() <= synced_revision) {
						deleted_securities.append(security);
					}
					securities.append(security);
					i_quotation_decimals = security->quotationDecimals();
					i_share_decimals = security->decimals();
				}
			} else {
				security_errors++;
				delete security;
			}
		} else {
			if(!errors.isEmpty()) errors += '\n';
			errors += tr("Unknown XML element: \"%1\" at line %2, col %3").arg(xml.name().toString()).arg(xml.lineNumber()).arg(xml.columnNumber());
			xml.skipCurrentElement();
		}
	}

	if(xml.hasError()) {
		if(!errors.isEmpty()) errors += '\n';
		errors += tr("XML parse error: \"%1\" at line %2, col %3").arg(xml.errorString()).arg(xml.lineNumber()).arg(xml.columnNumber());
	}

	for(QHash<qlonglong, ScheduledTransaction*>::iterator it = scheduleds_id.begin(); it != scheduleds_id.end(); ++it) {
		if((*it)->lastRevision() <= synced_revision) removeScheduledTransaction(*it);
	}
	for(QHash<qlonglong, SplitTransaction*>::iterator it = splits_id.begin(); it != splits_id.end(); ++it) {
		if((*it)->lastRevision() <= synced_revision || ((*it)->type() != SPLIT_TRANSACTION_TYPE_LOAN && (*it)->count() <= 1)) {
			(*it)->clear(true);
			removeSplitTransaction(*it);
		}
	}

	for(QHash<qlonglong, SecurityTrade*>::iterator it = securitytrades_id.begin(); it != securitytrades_id.end(); ++it) {
		if((*it)->last_revision <= synced_revision) removeSecurityTrade(*it);
	}
	for(QHash<qlonglong, Transaction*>::iterator it = transactions_id.begin(); it != transactions_id.end(); ++it) {
		if(!(*it)->parentSplit() && (*it)->lastRevision() <= synced_revision) removeTransaction(*it);
	}

	for(QHash<qlonglong, Security*>::iterator it = old_securities_id.begin(); it != old_securities_id.end(); ++it) {
		if((*it)->lastRevision() <= synced_revision) {
			QHash<qlonglong, Security*>::iterator it2 = securities_id.find((*it)->id());
			if(it2 == securities_id.end() && !securityHasTransactions(*it)) removeSecurity(*it);
		}
	}
	for(QList<Security*>::const_iterator it = deleted_securities.constBegin(); it != deleted_securities.constEnd(); ++it) {
		if(!securityHasTransactions(*it)) {
			removeSecurity(*it);
		}
	}

	for(QHash<qlonglong, ExpensesAccount*>::iterator it = old_expensesAccounts_id.begin(); it != old_expensesAccounts_id.end(); ++it) {
		if((*it)->lastRevision() <= synced_revision) {
			QHash<qlonglong, ExpensesAccount*>::iterator it2 = expensesAccounts_id.find((*it)->id());
			if(it2 == expensesAccounts_id.end() && !accountHasTransactions(*it)) removeAccount(*it);
		}
	}
	for(QHash<qlonglong, IncomesAccount*>::iterator it = old_incomesAccounts_id.begin(); it != old_incomesAccounts_id.end(); ++it) {
		if((*it)->lastRevision() <= synced_revision) {
			QHash<qlonglong, IncomesAccount*>::iterator it2 = incomesAccounts_id.find((*it)->id());
			if(it2 == incomesAccounts_id.end() && !accountHasTransactions(*it)) removeAccount(*it);
		}
	}
	for(QHash<qlonglong, AssetsAccount*>::iterator it = old_assetsAccounts_id.begin(); it != old_assetsAccounts_id.end(); ++it) {
		if((*it)->lastRevision() <= synced_revision) {
			QHash<qlonglong, AssetsAccount*>::iterator it2 = assetsAccounts_id.find((*it)->id());
			if(it2 == assetsAccounts_id.end() && !accountHasTransactions(*it)) removeAccount(*it);
		}
	}
	for(QList<Account*>::const_iterator it = deleted_accounts.constBegin(); it != deleted_accounts.constEnd(); ++it) {
		if(!accountHasTransactions(*it)) removeAccount(*it);
	}
	
	incomesAccounts_id.clear();
	expensesAccounts_id.clear();
	assetsAccounts_id.clear();
	securities_id.clear();
	
	expenses.sort();
	incomes.sort();
	transfers.sort();
	securityTransactions.sort();
	securityTrades.sort();
	for(SecurityList<Security*>::const_iterator it = securities.constBegin(); it != securities.constEnd(); ++it) {
		Security *security = *it;
		security->dividends.sort();
		security->transactions.sort();
		security->scheduledTransactions.sort();
		security->scheduledDividends.sort();
		security->scheduledReinvestedDividends.sort();
		security->reinvestedDividends.sort();
		security->tradedShares.sort();
	}
	transactions.sort();
	scheduledTransactions.sort();
	splitTransactions.sort();
	expensesAccounts.sort();
	incomesAccounts.sort();
	assetsAccounts.sort();
	accounts.sort();
	securities.sort();
	
	if(account_errors > 0) {
		if(!errors.isEmpty()) errors += '\n';
		errors += tr("Unable to load %n account(s).", "", account_errors);
	}
	if(category_errors > 0) {
		if(!errors.isEmpty()) errors += '\n';
		errors += tr("Unable to load %n category/categories.", "", category_errors);
	}
	if(security_errors > 0) {
		if(!errors.isEmpty()) errors += '\n';
		errors += tr("Unable to load %n security/securities.", "Financial security (e.g. stock, mutual fund)", security_errors);
	}
	if(transaction_errors > 0) {
		if(!errors.isEmpty()) errors += '\n';
		errors += tr("Unable to load %n transaction(s).", "", transaction_errors);
	}
	file.close();
	return QString();
}


QString Budget::saveFile(QString filename, QFile::Permissions permissions, bool is_backup) {

	QFileInfo info(filename);
	if(info.isDir()) {
		return tr("File is a directory");
	}
	
	QSaveFile ofile(filename);
	ofile.open(QIODevice::WriteOnly);
	ofile.setPermissions(permissions);
	if(!ofile.isOpen()) {
		ofile.cancelWriting();
		return tr("Couldn't open file for writing");
	}
	
	if(!is_backup) i_opened_revision = i_revision;
	
	QXmlStreamWriter xml(&ofile);
	xml.setCodec("UTF-8");
	xml.setAutoFormatting(true);
	xml.setAutoFormattingIndent(-1);
	
	xml.writeStartDocument();
	xml.writeDTD("<!DOCTYPE EqonomizeDoc>");
	xml.writeStartElement("EqonomizeDoc");
	xml.writeAttribute("version", VERSION);
	xml.writeAttribute("revision", QString::number(i_revision));
	xml.writeAttribute("lastid", QString::number(last_id));
	
	if(o_sync->isComplete()) {
		xml.writeStartElement("synchronization");
		xml.writeAttribute("type", "url");
		if(o_sync->autosync) xml.writeAttribute("autosync", QString::number(o_sync->autosync));
		xml.writeAttribute("revision", QString::number(o_sync->revision));
		if(!o_sync->url.isEmpty()) xml.writeTextElement("url", o_sync->url);
		if(!o_sync->download.isEmpty()) xml.writeTextElement("download", o_sync->download);
		if(!o_sync->upload.isEmpty()) xml.writeTextElement("upload", o_sync->upload);
		xml.writeEndElement();
	}
	xml.writeStartElement("budget_period");
	xml.writeTextElement("first_day_of_month", QString::number(i_budget_day));
	xml.writeTextElement("first_month_of_year", QString::number(i_budget_month));
	xml.writeEndElement();
	xml.writeStartElement("currency");
	xml.writeAttribute("code", default_currency->code());
	xml.writeEndElement();
	for(AccountList<Account*>::const_iterator it = accounts.constBegin(); it != accounts.constEnd(); ++it) {
		Account *account = *it;
		if(account != balancingAccount && account->topAccount() == account) {
			switch(account->type()) {
				case ACCOUNT_TYPE_ASSETS: {
					xml.writeStartElement("account");
					account->save(&xml);
					xml.writeEndElement();
					break;
				}
				case ACCOUNT_TYPE_INCOMES: {
					xml.writeStartElement("category");
					xml.writeAttribute("type", "incomes");
					account->save(&xml);
					xml.writeEndElement();
					break;
				}
				case ACCOUNT_TYPE_EXPENSES: {
					xml.writeStartElement("category");
					xml.writeAttribute("type", "expenses");
					account->save(&xml);
					xml.writeEndElement();
					break;
				}
			}
		}
	}
	for(SecurityList<Security*>::const_iterator it = securities.constBegin(); it != securities.constEnd(); ++it) {
		Security *security = *it;
		xml.writeStartElement("security");
		security->save(&xml);
		xml.writeEndElement();
	}
	for(ScheduledTransactionList<ScheduledTransaction*>::const_iterator it = scheduledTransactions.constBegin(); it != scheduledTransactions.constEnd(); ++it) {
		ScheduledTransaction *strans = *it;
		xml.writeStartElement("schedule");
		strans->save(&xml);
		xml.writeEndElement();
	}
	for(SplitTransactionList<SplitTransaction*>::const_iterator it = splitTransactions.constBegin(); it != splitTransactions.constEnd(); ++it) {
		SplitTransaction *split = *it;
		if(split->count() > 0) {
			xml.writeStartElement("transaction");
			switch(split->type()) {
				case SPLIT_TRANSACTION_TYPE_MULTIPLE_ITEMS: {
					xml.writeAttribute("type", "multiitem");
					break;
				}
				case SPLIT_TRANSACTION_TYPE_MULTIPLE_ACCOUNTS: {
					xml.writeAttribute("type", "multiaccount");
					break;
				}
				case SPLIT_TRANSACTION_TYPE_LOAN: {
					xml.writeAttribute("type", "debtpayment");
					break;
				}
			}
			split->save(&xml);
			xml.writeEndElement();
		}
	}

	for(SecurityTradeList<SecurityTrade*>::const_iterator it = securityTrades.constBegin(); it != securityTrades.constEnd(); ++it) {
		SecurityTrade *ts = *it;
		xml.writeStartElement("transaction");
		xml.writeAttribute("type", "security_trade");
		ts->save(&xml);
		xml.writeEndElement();
	}

	for(TransactionList<Transaction*>::const_iterator it = transactions.constBegin(); it != transactions.constEnd(); ++it) {
		Transaction *trans = *it;
		if(!trans->parentSplit()) {
			xml.writeStartElement("transaction");
			switch(trans->type()) {
				case TRANSACTION_TYPE_TRANSFER: {
					if(trans->fromAccount() == balancingAccount || trans->toAccount() == balancingAccount) xml.writeAttribute("type", "balancing");
					else xml.writeAttribute("type", "transfer");
					break;
				}
				case TRANSACTION_TYPE_INCOME: {
					if(trans->subtype() == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND) xml.writeAttribute("type", "reinvested_dividend");
					else if(((Income*) trans)->security()) xml.writeAttribute("type", "dividend");
					else if(trans->value() < 0.0) xml.writeAttribute("type", "repayment");
					else xml.writeAttribute("type", "income");
					break;
				}
				case TRANSACTION_TYPE_EXPENSE: {
					if(trans->value() < 0.0) xml.writeAttribute("type", "refund");
					else xml.writeAttribute("type", "expense");
					break;
				}
				case TRANSACTION_TYPE_SECURITY_BUY: {
					xml.writeAttribute("type", "security_buy");
					break;
				}
				case TRANSACTION_TYPE_SECURITY_SELL: {
					xml.writeAttribute("type", "security_sell");
					break;
				}
			}
			trans->save(&xml);
			xml.writeEndElement();
		}
	}
	
	xml.writeEndElement();

	if(ofile.error() != QFile::NoError) {
		ofile.cancelWriting();
		return tr("Error while writing file; file was not saved");
	}

	if(!ofile.commit()) {
		return tr("Error while writing file; file was not saved");
	}

	return QString();

}

void Budget::addTransactions(Transactions *trans) {
	switch(trans->generaltype()) {
		case GENERAL_TRANSACTION_TYPE_SINGLE: {addTransaction((Transaction*) trans); break;}
		case GENERAL_TRANSACTION_TYPE_SPLIT: {addSplitTransaction((SplitTransaction*) trans); break;}
		case GENERAL_TRANSACTION_TYPE_SCHEDULE: {addScheduledTransaction((ScheduledTransaction*) trans); break;}
	}
}
void Budget::removeTransactions(Transactions *trans, bool keep) {
	switch(trans->generaltype()) {
		case GENERAL_TRANSACTION_TYPE_SINGLE: {removeTransaction((Transaction*) trans, keep); break;}
		case GENERAL_TRANSACTION_TYPE_SPLIT: {removeSplitTransaction((SplitTransaction*) trans, keep); break;}
		case GENERAL_TRANSACTION_TYPE_SCHEDULE: {removeScheduledTransaction((ScheduledTransaction*) trans, keep); break;}
	}
}

void Budget::addTransaction(Transaction *trans) {
	if(trans->id() == 0) trans->setId(getNewId());
	if(trans->firstRevision() == 0) trans->setFirstRevision(i_revision);
	if(trans->lastRevision() == 0) trans->setLastRevision(i_revision);
	switch(trans->type()) {
		case TRANSACTION_TYPE_EXPENSE: {expenses.inSort((Expense*) trans); break;}
		case TRANSACTION_TYPE_INCOME: {
			incomes.inSort((Income*) trans);
			if(((Income*) trans)->security()) {
				if(trans->subtype() == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND) ((Income*) trans)->security()->reinvestedDividends.inSort((ReinvestedDividend*) trans);
				else ((Income*) trans)->security()->dividends.inSort((Income*) trans);
			}
			break;
		}
		case TRANSACTION_TYPE_TRANSFER: {transfers.inSort((Transfer*) trans); break;}
		case TRANSACTION_TYPE_SECURITY_BUY: {}
		case TRANSACTION_TYPE_SECURITY_SELL: {
			SecurityTransaction *sectrans = (SecurityTransaction*) trans;
			securityTransactions.inSort(sectrans);
			sectrans->security()->transactions.inSort(sectrans);
			//if(sectrans->shareValue() > 0.0) sectrans->security()->setQuotation(sectrans->date(), sectrans->shareValue(), true);
			break;
		}
	}
	transactions.inSort(trans);
}
void Budget::removeTransaction(Transaction *trans, bool keep) {
	if(trans->parentSplit()) {
		trans->parentSplit()->removeTransaction(trans, keep);
		return;
	}
	transactions.removeRef(trans);
	switch(trans->type()) {
		case TRANSACTION_TYPE_EXPENSE: {
			expenses.setAutoDelete(false);
			expenses.removeRef((Expense*) trans);
			expenses.setAutoDelete(true);
			if(!keep) delete trans;
			break;
		}
		case TRANSACTION_TYPE_INCOME: {
			incomes.setAutoDelete(false);
			if(((Income*) trans)->security()) {
				if(trans->subtype() == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND) ((Income*) trans)->security()->reinvestedDividends.removeRef((ReinvestedDividend*) trans);
				else ((Income*) trans)->security()->dividends.removeRef((Income*) trans);
			}
			incomes.removeRef((Income*) trans);
			incomes.setAutoDelete(true);
			if(!keep) delete trans;
			break;
		}
		case TRANSACTION_TYPE_TRANSFER: {
			transfers.setAutoDelete(false);
			transfers.removeRef((Transfer*) trans);
			transfers.setAutoDelete(true);
			if(!keep) delete trans;
			break;
		}
		case TRANSACTION_TYPE_SECURITY_BUY: {}
		case TRANSACTION_TYPE_SECURITY_SELL: {
			SecurityTransaction *sectrans = (SecurityTransaction*) trans;
			sectrans->security()->removeQuotation(sectrans->date(), true);
			securityTransactions.setAutoDelete(false);
			sectrans->security()->transactions.removeRef(sectrans);
			securityTransactions.removeRef(sectrans);
			securityTransactions.setAutoDelete(true);
			if(!keep) delete trans;
			break;
		}
	}
}
void Budget::addSplitTransaction(SplitTransaction *split) {
	if(split->id() == 0) split->setId(getNewId());
	if(split->firstRevision() == 0) split->setFirstRevision(i_revision);
	if(split->lastRevision() == 0) split->setLastRevision(i_revision);
	splitTransactions.inSort(split);
	int c = split->count();
	for(int i = 0; i < c; i++) {
		addTransaction(split->at(i));
	}
}
void Budget::removeSplitTransaction(SplitTransaction *split, bool keep) {
	int c = split->count();
	for(int i = 0; i < c; i++) {
		Transaction *trans = split->at(i);
		transactions.removeRef(trans);
		switch(trans->type()) {
			case TRANSACTION_TYPE_EXPENSE: {
				expenses.setAutoDelete(false);
				expenses.removeRef((Expense*) trans);
				expenses.setAutoDelete(true);
				break;
			}
			case TRANSACTION_TYPE_INCOME: {
				incomes.setAutoDelete(false);
				incomes.removeRef((Income*) trans);
				if(trans->subtype() == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND) ((Income*) trans)->security()->reinvestedDividends.removeRef((ReinvestedDividend*) trans);
				else if(((Income*) trans)->security()) ((Income*) trans)->security()->dividends.removeRef((Income*) trans);
				incomes.setAutoDelete(true);
				break;
			}
			case TRANSACTION_TYPE_TRANSFER: {
				transfers.setAutoDelete(false);
				transfers.removeRef((Transfer*) trans);
				transfers.setAutoDelete(true);
				break;
			}
			case TRANSACTION_TYPE_SECURITY_BUY: {}
			case TRANSACTION_TYPE_SECURITY_SELL: {
				SecurityTransaction *sectrans = (SecurityTransaction*) trans;
				sectrans->security()->removeQuotation(sectrans->date(), true);
				securityTransactions.setAutoDelete(false);
				sectrans->security()->transactions.removeRef(sectrans);
				securityTransactions.removeRef(sectrans);
				securityTransactions.setAutoDelete(true);
				break;
			}
		}
	}
	if(keep) splitTransactions.setAutoDelete(false);
	splitTransactions.removeRef(split);
	if(keep) splitTransactions.setAutoDelete(true);
}
void Budget::addScheduledTransaction(ScheduledTransaction *strans) {
	if(strans->id() == 0) strans->setId(getNewId());
	if(strans->firstRevision() == 0) strans->setFirstRevision(i_revision);
	if(strans->lastRevision() == 0) strans->setLastRevision(i_revision);
	scheduledTransactions.inSort(strans);
	if(strans->transactiontype() == TRANSACTION_TYPE_SECURITY_BUY || strans->transactiontype() == TRANSACTION_TYPE_SECURITY_SELL) {
		((SecurityTransaction*) strans->transaction())->security()->scheduledTransactions.inSort(strans);
	} else if(strans->transactiontype() == TRANSACTION_TYPE_INCOME && ((Income*) strans->transaction())->security()) {
		if(strans->transactionsubtype() == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND) ((Income*) strans->transaction())->security()->scheduledReinvestedDividends.inSort(strans);
		else ((Income*) strans->transaction())->security()->scheduledDividends.inSort(strans);
	}
}
void Budget::removeScheduledTransaction(ScheduledTransaction *strans, bool keep) {
	 if(strans->transactiontype() == TRANSACTION_TYPE_SECURITY_BUY || strans->transactiontype() == TRANSACTION_TYPE_SECURITY_SELL) {
		((SecurityTransaction*) strans->transaction())->security()->scheduledTransactions.removeRef(strans);
	 } else if(strans->transactiontype() == TRANSACTION_TYPE_INCOME && ((Income*) strans->transaction())->security()) {
	 	if(strans->transactionsubtype() == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND) ((Income*) strans->transaction())->security()->scheduledReinvestedDividends.removeRef(strans);
		else ((Income*) strans->transaction())->security()->scheduledDividends.removeRef(strans);
	}
	if(keep) scheduledTransactions.setAutoDelete(false);
	scheduledTransactions.removeRef(strans);
	if(keep) scheduledTransactions.setAutoDelete(true);
}
void Budget::addAccount(Account *account) {
	if(account->id() == 0) account->setId(getNewId());
	if(account->firstRevision() == 0) account->setFirstRevision(i_revision);
	if(account->lastRevision() == 0) account->setLastRevision(i_revision);
	switch(account->type()) {
		case ACCOUNT_TYPE_EXPENSES: {expensesAccounts.inSort((ExpensesAccount*) account); break;}
		case ACCOUNT_TYPE_INCOMES: {incomesAccounts.inSort((IncomesAccount*) account); break;}
		case ACCOUNT_TYPE_ASSETS: {assetsAccounts.inSort((AssetsAccount*) account); break;}
	}
	accounts.inSort(account);
	if(b_record_new_accounts) newAccounts << account;
}
void Budget::setRecordNewAccounts(bool rna) {b_record_new_accounts = rna;}
void Budget::accountModified(Account *account) {
	switch(account->type()) {
		case ACCOUNT_TYPE_EXPENSES: {expensesAccounts.sort(); break;}
		case ACCOUNT_TYPE_INCOMES: {incomesAccounts.sort(); break;}
		case ACCOUNT_TYPE_ASSETS: {assetsAccounts.sort(); break;}
	}
	accounts.sort();
}
void Budget::removeAccount(Account *account, bool keep) {
	if(account->type() == ACCOUNT_TYPE_INCOMES || account->type() == ACCOUNT_TYPE_EXPENSES) {
		for(AccountList<CategoryAccount*>::const_iterator it = ((CategoryAccount*) account)->subCategories.constBegin(); it != ((CategoryAccount*) account)->subCategories.constEnd(); ++it) {
			CategoryAccount *subcat = *it;
			if(!keep) subcat->o_parent = NULL;
			removeAccount(subcat, keep);
		}
		if(!keep) ((CategoryAccount*) account)->subCategories.clear();
	}
	if(accountHasTransactions(account, false)) {
		for(int i = 0; i < securities.size(); ++i) {
			Security *security = securities.at(i);
			if(security->account() == account) {
				removeSecurity(security);
				--i;
			}
		}
		for(int i = 0; i < splitTransactions.size(); ++i) {
			SplitTransaction *split = splitTransactions.at(i);
			if(split->relatesToAccount(account, true, true)) {
				removeSplitTransaction(split);
				--i;
			}
		}
		for(int i = 0; i < transactions.size(); ++i) {
			Transaction *trans = transactions.at(i);
			if(trans->relatesToAccount(account, true, true)) {
				removeTransaction(trans);
				--i;
			}
		}
		for(int i = 0; i < scheduledTransactions.size(); ++i) {
			ScheduledTransaction *strans = scheduledTransactions.at(i);
			if(strans->relatesToAccount(account, true, true)) {
				removeScheduledTransaction(strans);
				--i;
			}
		}
	}
	accounts.removeRef(account);
	switch(account->type()) {
		case ACCOUNT_TYPE_EXPENSES: {
			if(keep) expensesAccounts.setAutoDelete(false);
			expensesAccounts.removeRef((ExpensesAccount*) account);
			if(keep) expensesAccounts.setAutoDelete(true);
			break;
		}
		case ACCOUNT_TYPE_INCOMES: {
			if(keep) incomesAccounts.setAutoDelete(false);
			incomesAccounts.removeRef((IncomesAccount*) account);
			if(keep) incomesAccounts.setAutoDelete(true);
			break;
		}
		case ACCOUNT_TYPE_ASSETS: {
			if(keep) assetsAccounts.setAutoDelete(false);
			assetsAccounts.removeRef((AssetsAccount*) account);
			if(keep) assetsAccounts.setAutoDelete(true);
			break;
		}
	}
}
bool Budget::accountHasTransactions(Account *account, bool check_subs) {
	for(SecurityList<Security*>::const_iterator it = securities.constBegin(); it != securities.constEnd(); ++it) {
		Security *security = *it;
		if(security->account() == account) return true;
	}
	for(SplitTransactionList<SplitTransaction*>::const_iterator it = splitTransactions.constBegin(); it != splitTransactions.constEnd(); ++it) {
		SplitTransaction *split = *it;
		if(split->relatesToAccount(account, true, true)) return true;
	}
	for(TransactionList<Transaction*>::const_iterator it = transactions.constBegin(); it != transactions.constEnd(); ++it) {
		Transaction *trans = *it;
		if(trans->relatesToAccount(account, true, true)) return true;
	}
	for(ScheduledTransactionList<ScheduledTransaction*>::const_iterator it = scheduledTransactions.constBegin(); it != scheduledTransactions.constEnd(); ++it) {
		ScheduledTransaction *strans = *it;
		if(strans->relatesToAccount(account, true, true)) return true;
	}
	if(check_subs && (account->type() == ACCOUNT_TYPE_INCOMES || account->type() == ACCOUNT_TYPE_EXPENSES)) {
		for(AccountList<CategoryAccount*>::const_iterator it = ((CategoryAccount*) account)->subCategories.constBegin(); it != ((CategoryAccount*) account)->subCategories.constEnd(); ++it) {
			CategoryAccount *subcat = *it;
			if(accountHasTransactions(subcat, true)) return true;
		}
	}
	return false;
}
void Budget::moveTransactions(Account *account, Account *new_account, bool move_from_subs) {
	if(move_from_subs && (account->type() == ACCOUNT_TYPE_INCOMES || account->type() == ACCOUNT_TYPE_EXPENSES)) {
		for(AccountList<CategoryAccount*>::const_iterator it = ((CategoryAccount*) account)->subCategories.constBegin(); it != ((CategoryAccount*) account)->subCategories.constEnd(); ++it) {
			CategoryAccount *subcat = *it;
			moveTransactions(subcat, new_account);
		}
	}
	if(account->type() == ACCOUNT_TYPE_ASSETS && new_account->type() == ACCOUNT_TYPE_ASSETS) {
		for(SecurityList<Security*>::const_iterator it = securities.constBegin(); it != securities.constEnd(); ++it) {
			Security *security = *it;
			if(security->account() == account) security->setAccount((AssetsAccount*) new_account);
		}
	}
	for(SplitTransactionList<SplitTransaction*>::const_iterator it = splitTransactions.constBegin(); it != splitTransactions.constEnd(); ++it) {
		SplitTransaction *split = *it;
		split->replaceAccount(account, new_account);
	}
	for(TransactionList<Transaction*>::const_iterator it = transactions.constBegin(); it != transactions.constEnd(); ++it) {
		Transaction *trans = *it;
		trans->replaceAccount(account, new_account);
	}
	for(ScheduledTransactionList<ScheduledTransaction*>::const_iterator it = scheduledTransactions.constBegin(); it != scheduledTransactions.constEnd(); ++it) {
		ScheduledTransaction *strans = *it;
		strans->replaceAccount(account, new_account);
	}
}
void Budget::transactionsSortModified(Transactions *trans) {
	switch(trans->generaltype()) {
		case GENERAL_TRANSACTION_TYPE_SINGLE: {transactionSortModified((Transaction*) trans); break;}
		case GENERAL_TRANSACTION_TYPE_SPLIT: {splitTransactionSortModified((SplitTransaction*) trans); break;}
		case GENERAL_TRANSACTION_TYPE_SCHEDULE: {scheduledTransactionSortModified((ScheduledTransaction*) trans); break;}
	}
}
void Budget::transactionSortModified(Transaction *t) {
	if(transactions.removeRef(t)) transactions.inSort(t);
	switch(t->type()) {
		case TRANSACTION_TYPE_EXPENSE: {
			Expense *e = (Expense*) t;
			expenses.setAutoDelete(false);
			if(expenses.removeRef(e)) expenses.inSort(e);
			expenses.setAutoDelete(true);
			break;
		}
		case TRANSACTION_TYPE_INCOME: {
			Income *i = (Income*) t;
			if(i->security()) {
				if(i->subtype() == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND && i->security()->reinvestedDividends.removeRef((ReinvestedDividend*) i)) i->security()->reinvestedDividends.inSort((ReinvestedDividend*) i);
				else if(i->security()->dividends.removeRef(i)) i->security()->dividends.inSort(i);
			}
			incomes.setAutoDelete(false);
			if(incomes.removeRef(i)) incomes.inSort(i);
			incomes.setAutoDelete(true);
			break;
		}
		case TRANSACTION_TYPE_TRANSFER: {
			Transfer *tr = (Transfer*) t;
			transfers.setAutoDelete(false);
			if(transfers.removeRef(tr)) transfers.inSort(tr);
			transfers.setAutoDelete(true);
			break;
		}
		case TRANSACTION_TYPE_SECURITY_BUY: {}
		case TRANSACTION_TYPE_SECURITY_SELL: {
			SecurityTransaction *tr = (SecurityTransaction*) t;
			if(tr->security()->transactions.removeRef(tr)) tr->security()->transactions.inSort(tr);
			securityTransactions.setAutoDelete(false);
			if(securityTransactions.removeRef(tr)) securityTransactions.inSort(tr);
			securityTransactions.setAutoDelete(true);
			break;
		}
	}
}
void Budget::transactionDateModified(Transaction*, const QDate&) {
/*	switch(t->type()) {
		case TRANSACTION_TYPE_SECURITY_BUY: {}
		case TRANSACTION_TYPE_SECURITY_SELL: {
			SecurityTransaction *tr = (SecurityTransaction*) t;
			if(tr->shareValue() > 0.0) {
				tr->security()->removeQuotation(olddate, true);
				tr->security()->setQuotation(tr->date(), tr->shareValue(), true);
			}
			break;
		}
		default: {
			break;
		}
	}*/
}
void Budget::scheduledTransactionDateModified(ScheduledTransaction*) {
}
void Budget::scheduledTransactionSortModified(ScheduledTransaction *strans) {
	if(strans->transactiontype() == TRANSACTION_TYPE_SECURITY_BUY || strans->transactiontype() == TRANSACTION_TYPE_SECURITY_SELL) {
		if(((SecurityTransaction*) strans->transaction())->security()->scheduledTransactions.removeRef(strans)) ((SecurityTransaction*) strans->transaction())->security()->scheduledTransactions.inSort(strans);
	} else if(strans->transactiontype() == TRANSACTION_TYPE_INCOME && ((Income*) strans->transaction())->security()) {
		if(strans->transactionsubtype() == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND && ((Income*) strans->transaction())->security()->scheduledReinvestedDividends.removeRef(strans)) ((Income*) strans->transaction())->security()->scheduledReinvestedDividends.inSort(strans);
		if(((Income*) strans->transaction())->security()->scheduledDividends.removeRef(strans)) ((Income*) strans->transaction())->security()->scheduledDividends.inSort(strans);
	}
	scheduledTransactions.setAutoDelete(false);
	if(scheduledTransactions.removeRef(strans)) scheduledTransactions.inSort(strans);
	scheduledTransactions.setAutoDelete(true);
}
void Budget::splitTransactionSortModified(SplitTransaction *split) {
	splitTransactions.setAutoDelete(false);
	if(splitTransactions.removeRef(split)) splitTransactions.inSort(split);
	splitTransactions.setAutoDelete(true);
}
void Budget::splitTransactionDateModified(SplitTransaction*, const QDate&) {}

Transaction *Budget::findDuplicateTransaction(Transaction *trans) {
	TransactionList<Transaction*>::const_iterator it = std::lower_bound(transactions.constBegin(), transactions.constEnd(), trans, transaction_list_less_than);
	while(it != transactions.constEnd()) {
		if((*it)->date() > trans->date()) return NULL;
		if(trans->equals(*it, false)) return *it;
		++it;
	}
	return NULL;
}

void Budget::accountNameModified(Account *account) {
	if(accounts.removeRef(account)) accounts.inSort(account);
	switch(account->type()) {
		case ACCOUNT_TYPE_EXPENSES: {
			ExpensesAccount *eaccount = (ExpensesAccount*) account;
			expensesAccounts.setAutoDelete(false);
			if(expensesAccounts.removeRef(eaccount)) expensesAccounts.inSort(eaccount);
			expensesAccounts.setAutoDelete(true);
			break;
		}
		case ACCOUNT_TYPE_INCOMES: {
			IncomesAccount *iaccount = (IncomesAccount*) account;
			incomesAccounts.setAutoDelete(false);
			if(incomesAccounts.removeRef(iaccount)) incomesAccounts.inSort(iaccount);
			incomesAccounts.setAutoDelete(true);
			break;
		}
		case ACCOUNT_TYPE_ASSETS: {
			AssetsAccount *aaccount = (AssetsAccount*) account;
			assetsAccounts.setAutoDelete(false);
			if(assetsAccounts.removeRef(aaccount)) assetsAccounts.inSort(aaccount);
			assetsAccounts.setAutoDelete(true);
			break;
		}
	}
}
void Budget::addSecurity(Security *security) {
	if(security->id() == 0) security->setId(getNewId());
	if(security->firstRevision() == 0) security->setFirstRevision(i_revision);
	if(security->lastRevision() == 0) security->setLastRevision(i_revision);
	securities.inSort(security);
	i_quotation_decimals = security->quotationDecimals();
	i_share_decimals = security->decimals();
	if(b_record_new_securities) newSecurities << security;
}
void Budget::setRecordNewSecurities(bool rns) {b_record_new_securities = rns;}
void Budget::removeSecurity(Security *security, bool keep) {
	if(securityHasTransactions(security)) {
		for(SecurityTransactionList<SecurityTransaction*>::const_iterator it = security->transactions.constBegin(); it != security->transactions.constEnd(); ++it) {
			SecurityTransaction *trans = *it;
			transactions.removeRef(trans);
			securityTransactions.removeRef(trans);
		}
		for(SecurityTransactionList<Income*>::const_iterator it = security->dividends.constBegin(); it != security->dividends.constEnd(); ++it) {
			Income *i = *it;
			transactions.removeRef(i);
			incomes.removeRef(i);
		}
		for(SecurityTransactionList<ReinvestedDividend*>::const_iterator it = security->reinvestedDividends.constBegin(); it != security->reinvestedDividends.constEnd(); ++it) {
			Income *i = *it;
			transactions.removeRef(i);
			incomes.removeRef(i);
		}
		for(ScheduledSecurityTransactionList<ScheduledTransaction*>::const_iterator it = security->scheduledTransactions.constBegin(); it != security->scheduledTransactions.constEnd(); ++it) {
			ScheduledTransaction *strans = *it;
			scheduledTransactions.removeRef(strans);
		}
		for(ScheduledSecurityTransactionList<ScheduledTransaction*>::const_iterator it = security->scheduledDividends.constBegin(); it != security->scheduledDividends.constEnd(); ++it) {
			ScheduledTransaction *strans = *it;
			scheduledTransactions.removeRef(strans);
		}
		for(ScheduledSecurityTransactionList<ScheduledTransaction*>::const_iterator it = security->scheduledReinvestedDividends.constBegin(); it != security->scheduledReinvestedDividends.constEnd(); ++it) {
			ScheduledTransaction *strans = *it;
			scheduledTransactions.removeRef(strans);
		}
		for(TradedSharesList<SecurityTrade*>::const_iterator it = security->tradedShares.constBegin(); it != security->tradedShares.constEnd(); ++it) {
			SecurityTrade *ts = *it;
			if(ts->to_security == security) ts->from_security->tradedShares.removeRef(ts);
			else ts->to_security->tradedShares.removeRef(ts);
			securityTrades.removeRef(ts);
		}
	}
	if(keep) securities.setAutoDelete(false);
	securities.removeRef(security);
	if(keep) securities.setAutoDelete(true);
}
bool Budget::securityHasTransactions(Security *security) {
	return security->reinvestedDividends.count() > 0 || security->scheduledReinvestedDividends.count() > 0 || security->tradedShares.count() > 0 || security->transactions.count() > 0 || security->dividends.count() > 0 || security->scheduledTransactions.count() > 0 || security->scheduledDividends.count() > 0;
}
void Budget::securityNameModified(Security *security) {
	securities.setAutoDelete(false);
	if(securities.removeRef(security)) {
		securities.inSort(security);
	}
	securities.setAutoDelete(true);
}
Security *Budget::findSecurity(QString name) {
	for(SecurityList<Security*>::const_iterator it = securities.constBegin(); it != securities.constEnd(); ++it) {
		Security *sec = *it;
		if(sec->name() == name) return sec;
	}
	return NULL;
}

int Budget::defaultShareDecimals() const {return i_share_decimals;}
int Budget::defaultQuotationDecimals() const {return i_quotation_decimals;}
void Budget::setDefaultShareDecimals(int new_decimals) {i_share_decimals = new_decimals;}
void Budget::setDefaultQuotationDecimals(int new_decimals) {i_quotation_decimals = new_decimals;}

void Budget::addSecurityTrade(SecurityTrade *ts) {
	if(ts->id == 0) ts->id = getNewId();
	if(ts->first_revision == 0) ts->first_revision = i_revision;
	if(ts->last_revision == 0) ts->last_revision = i_revision;
	securityTrades.inSort(ts);
	ts->from_security->tradedShares.inSort(ts);
	ts->to_security->tradedShares.inSort(ts);
}
void Budget::removeSecurityTrade(SecurityTrade *ts, bool keep) {
	ts->from_security->tradedShares.removeRef(ts);
	ts->to_security->tradedShares.removeRef(ts);
	ts->from_security->removeQuotation(ts->date, true);
	ts->to_security->removeQuotation(ts->date, true);
	if(keep) securityTrades.setAutoDelete(false);
	securityTrades.removeRef(ts);
	if(keep) securityTrades.setAutoDelete(true);
}
void Budget::securityTradeDateModified(SecurityTrade *ts, const QDate &olddate) {
	securityTrades.setAutoDelete(false);
	if(securityTrades.removeRef(ts)) {
		securityTrades.inSort(ts);
	}
	securityTrades.setAutoDelete(true);
	if(ts->from_security->tradedShares.removeRef(ts)) {
		ts->from_security->tradedShares.inSort(ts);
	}
	if(ts->to_security->tradedShares.removeRef(ts)) {
		ts->to_security->tradedShares.inSort(ts);
	}
	ts->from_security->removeQuotation(olddate, true);
	ts->to_security->removeQuotation(olddate, true);
}
Account *Budget::findAccount(QString name) {
	for(AccountList<Account*>::const_iterator it = accounts.constBegin(); it != accounts.constEnd(); ++it) {
		Account *account = *it;
		if(account->name() == name) return account;
	}
	return NULL;
}
AssetsAccount *Budget::findAssetsAccount(QString name) {
	for(AccountList<AssetsAccount*>::const_iterator it = assetsAccounts.constBegin(); it != assetsAccounts.constEnd(); ++it) {
		AssetsAccount *account = *it;
		if(account->name() == name) return account;
	}
	return NULL;
}
IncomesAccount *Budget::findIncomesAccount(QString name) {
	for(AccountList<IncomesAccount*>::const_iterator it = incomesAccounts.constBegin(); it != incomesAccounts.constEnd(); ++it) {
		IncomesAccount *account = *it;
		if(account->name() == name) return account;
	}
	return NULL;
}
ExpensesAccount *Budget::findExpensesAccount(QString name) {
	for(AccountList<ExpensesAccount*>::const_iterator it = expensesAccounts.constBegin(); it != expensesAccounts.constEnd(); ++it) {
		ExpensesAccount *account = *it;
		if(account->name() == name) return account;
	}
	return NULL;
}
IncomesAccount *Budget::findIncomesAccount(QString name, CategoryAccount *parent_acc) {
	for(AccountList<IncomesAccount*>::const_iterator it = incomesAccounts.constBegin(); it != incomesAccounts.constEnd(); ++it) {
		IncomesAccount *account = *it;
		if(account->name() == name && account->parentCategory() == parent_acc) return account;
	}
	return NULL;
}
ExpensesAccount *Budget::findExpensesAccount(QString name, CategoryAccount *parent_acc) {
	for(AccountList<ExpensesAccount*>::const_iterator it = expensesAccounts.constBegin(); it != expensesAccounts.constEnd(); ++it) {
		ExpensesAccount *account = *it;
		if(account->name() == name && account->parentCategory() == parent_acc) return account;
	}
	return NULL;
}

Currency *Budget::defaultCurrency() {
	return default_currency;
}
void Budget::setDefaultCurrency(Currency *cur) {
	Currency *prev_default = default_currency;
	if(!cur) default_currency = currency_euro;
	else default_currency = cur;
	if(prev_default != default_currency) b_default_currency_changed = true;
}
bool Budget::resetDefaultCurrency() {
	Currency *prev_default = default_currency;
	QString default_code = QLocale().currencySymbol(QLocale::CurrencyIsoCode);
	if(default_code.isEmpty()) default_code = "USD";
	default_currency = findCurrency(default_code);
	if(!default_currency) {
		default_currency = new Currency(this, default_code, QLocale().currencySymbol(QLocale::CurrencySymbol), QLocale().currencySymbol(QLocale::CurrencyDisplayName));
		b_default_currency_changed = true;
		addCurrency(default_currency);
		return true;
	}
	if(prev_default != default_currency) b_default_currency_changed = true;
	return false;
}
bool Budget::defaultCurrencyChanged() {return b_default_currency_changed;}
void Budget::resetDefaultCurrencyChanged() {b_default_currency_changed = false;}
bool Budget::currenciesModified() {return b_currency_modified;}
void Budget::resetCurrenciesModified() {b_currency_modified = false;}
void Budget::addCurrency(Currency *cur) {
	currencies.inSort(cur);
}
void Budget::currencyModified(Currency*) {
	b_currency_modified = true;
}
void Budget::removeCurrency(Currency *cur) {
	currencies.removeRef(cur);
}
Currency *Budget::findCurrency(QString code) {
	for(CurrencyList<Currency*>::const_iterator it = currencies.constBegin(); it != currencies.constEnd(); ++it) {
		Currency *cur = *it;
		if(cur->code() == code) return cur;
	}
	return NULL;
}
Currency *Budget::findCurrencySymbol(QString symbol, bool require_unique)  {
	Currency *found_cur = NULL;
	bool found_multiple = false;
	for(CurrencyList<Currency*>::const_iterator it = currencies.constBegin(); it != currencies.constEnd(); ++it) {
		Currency *cur = *it;
		if(cur->symbol(false) == symbol) {
			if(!require_unique || cur == defaultCurrency() || (defaultCurrency()->symbol(false) != symbol && (cur->code() == QLocale().currencySymbol(QLocale::CurrencyIsoCode) || (symbol != QLocale().currencySymbol(QLocale::CurrencySymbol) && (cur->code() == "USD" || cur->code() == "GBP" || cur->code() == "EUR" || cur->code() == "JPY"))))) return cur;
			else if(found_cur) found_multiple = true;
			else found_cur = cur;
		}
	}
	if(found_multiple) return NULL;
	return found_cur;
}
bool Budget::usesMultipleCurrencies() {
	for(AccountList<AssetsAccount*>::const_iterator it = assetsAccounts.constBegin(); it != assetsAccounts.constEnd(); ++it) {
		AssetsAccount *acc = *it;
		if(acc->currency() != NULL && acc->currency() != default_currency) {
			return true;
		}
	}
	return false;
}

void Budget::setBudgetDay(int day_of_month) {if(day_of_month <= 28 && day_of_month >= -26) i_budget_day = day_of_month;}
int Budget::budgetDay() const {return i_budget_day;}
void Budget::setBudgetMonth(int month_of_year) {if(month_of_year <= 12 && month_of_year >= 1) i_budget_month = month_of_year;}
int Budget::budgetMonth() const {return i_budget_month;}

bool isLeapYear(long int year) {
	return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}
int daysPerYear(long int year, int basis = 1) {
	switch(basis) {
		case 0: {
			return 360;
		}
		case 1: {
			if(isLeapYear(year)) {
				return 366;
			} else {
				return 365;
			}
		}
		case 2: {
			return 360;
		}		
		case 3: {
			return 365;
		} 
		case 4: {
			return 360;
		}
	}
	return -1;
}
int daysPerMonth(int month, long int year) {
	switch(month) {
		case 1: {} case 3: {} case 5: {} case 7: {} case 8: {} case 10: {} case 12: {
			return 31;
		}
		case 2:	{
			if(isLeapYear(year)) return 29;
			else return 28;
		}
		default: {
			return 30;
		}
	}
}

bool Budget::isSameBudgetMonth(const QDate &date1, const QDate &date2) const {
	return budgetYear(date1) == budgetYear(date2) && budgetMonth(date1) == budgetMonth(date2);
}
int Budget::daysInBudgetMonth(const QDate &date) const {
	if(i_budget_day == 1) {
		return date.daysInMonth();
	} else if(i_budget_day > 0) {
		if(date.day() >= i_budget_day) return date.daysInMonth();
		return date.addMonths(-1).daysInMonth();
	} else {
		return firstBudgetDay(date).daysTo(lastBudgetDay(date)) + 1;
	}
}
int Budget::daysInBudgetYear(const QDate &date) const {
	if(i_budget_day == 1 && i_budget_month == 1) return date.daysInYear();
	int i_year = budgetYear(date);
	if(i_budget_month > 3 ||  (i_budget_month == 3 && (i_budget_day <= 15 || i_budget_day >= 29) && (i_budget_day >= 0 || i_budget_day < -15))) i_year++;
	return daysPerYear(i_year);
}
int Budget::dayOfBudgetYear(const QDate &date) const {
	if(i_budget_day == 1 && i_budget_month == 1) return date.dayOfYear();
	return firstBudgetDayOfYear(date).daysTo(date) + 1;
}
int Budget::dayOfBudgetMonth(const QDate &date) const {
	if(i_budget_day == 1) return date.day();
	return firstBudgetDay(date).daysTo(date) + 1;
}
int Budget::budgetMonth(const QDate &date) const {
	int i_month = 1;
	if(i_budget_day == 1) {
		i_month = date.month();
	} else {
		int ibd = i_budget_day;
		if(i_budget_day <= 0) ibd = date.daysInMonth() + i_budget_day;
		if(i_budget_day > 15 || (i_budget_day < 1 && i_budget_day >= -15)) {
			if(date.day() < ibd) i_month = date.month();
			else if(date.month() == 12) i_month = 1;
			else i_month = date.month() + 1;
		} else {
			if(date.day() >= ibd) i_month = date.month();
			else if(date.month() == 1) i_month = 12;
			else i_month = date.month() - 1;
		}
	}
	return i_month;
}
int Budget::budgetYear(const QDate &date) const {
	if(i_budget_day == 1 && i_budget_month == 1) return date.year();
	int ibd = i_budget_day;
	int year = date.year();
	if(i_budget_day <= 0) ibd = daysPerMonth(i_budget_month == 1 ? 12 : i_budget_month - 1, year) + i_budget_day;
	if(i_budget_month > 1 && date.month() < i_budget_month) year--;
	if(i_budget_day > 15 || (i_budget_day < 1 && i_budget_day >= -15)) {
		if(date.month() == (i_budget_month == 1 ? 12 : i_budget_month - 1) && date.day() >= ibd) year++;
	} else {
		if(date.month() == i_budget_month && date.day() < ibd) year--;
	}
	return year;
}
QString Budget::budgetYearString(const QDate &date, bool short_format) const {
	return budgetYearString(budgetYear(date), short_format);
}
QString Budget::budgetYearString(int year, bool short_format) const {
	if(i_budget_month == 1) return QString::number(year);
	//: Financial year when first month is not January (e.g. 2018-19).
	QString str = tr("yyyy-yy");
	int y1 = -1, y2 = -1, y3 = -1;
	y1 = str.indexOf("yyyy");
	if(y1 >= 0) y2 = str.indexOf("yyyy", y1 + 4);
	if(y2 <= 0) y3 = str.indexOf("yy", y1 >= 0 ? y2 + 4 : 0);
	if(y1 < 0 && y3 < 0) return str;
	if(y1 < 0) {
		str.replace(y3, 2, year % 100 <= 9 ? QString("0") + QString::number(year % 100) : QString::number(year % 100));
		str.replace("yy", year % 100 < 9 ? QString("0") + QString::number((year + 1) % 100) : QString::number((year + 1) % 100));
	} else if(y3 < 0) {
		str.replace(y1, 4, short_format ? (year % 100 <= 9 ? QString("0") + QString::number((year) % 100) : QString::number((year) % 100)) : QString::number(year));
		str.replace("yyyy", short_format ? (year % 100 < 9 ? QString("0") + QString::number((year + 1) % 100) : QString::number((year + 1) % 100)) : QString::number(year + 1));
	} else if(y3 < y1) {
		str.replace(y3, 2, year % 100 <= 9 ? QString("0") + QString::number(year % 100) : QString::number(year % 100));
		str.replace("yyyy", short_format ? (year % 100 < 9 ? QString("0") + QString::number((year + 1) % 100) : QString::number((year + 1) % 100)) : QString::number(year + 1));
	} else {
		str.replace(y1, 4, short_format ? (year % 100 <= 9 ? QString("0") + QString::number((year) % 100) : QString::number((year) % 100)) : QString::number(year));
		str.replace("yy", (short_format && year % 100 == 99) ? QString::number(year + 1) : (year % 100 < 9 ? QString("0") + QString::number((year + 1) % 100) : QString::number((year + 1) % 100)));
	}
	return str;
}

QDate Budget::firstBudgetDayOfYear(QDate date) const {
	if(i_budget_month == 1 && i_budget_day == 1) return QDate(date.year(), 1, 1);
	int i_year = budgetYear(date);
	if(i_budget_month == 1 && (i_budget_day > 15 || (i_budget_day < 1 && i_budget_day >= -15))) i_year--; 
	int ibd = i_budget_day;
	if(i_budget_day <= 0) ibd = daysPerMonth(i_budget_month == 1 ? 12 : i_budget_month - 1, i_year) + i_budget_day;
	return QDate(i_year, ibd > 15 ? (i_budget_month == 1 ? 12 : i_budget_month - 1) : i_budget_month, ibd);
}
QDate Budget::lastBudgetDayOfYear(QDate date) const {
	if(i_budget_month == 1 && i_budget_day == 1) return QDate(date.year(), 12, 31);
	int i_year = budgetYear(date);
	if(i_budget_month == 1 && (i_budget_day > 15 || (i_budget_day < 1 && i_budget_day >= -15))) i_year--; 
	int ibd = i_budget_day;
	if(i_budget_day <= 0) ibd = daysPerMonth(i_budget_month == 1 ? 12 : i_budget_month - 1, i_year + 1) + i_budget_day;
	return QDate(i_year + 1, ibd > 15 ? (i_budget_month == 1 ? 12 : i_budget_month - 1) : i_budget_month, ibd).addDays(-1);
}
bool Budget::isFirstBudgetDay(const QDate &date) const {
	return ((i_budget_day > 0 && date.day() == i_budget_day) || (i_budget_day <= 0 && date.day() == date.daysInMonth() + i_budget_day));
}
bool Budget::isLastBudgetDay(const QDate &date) const {
	return ((i_budget_day == 1 && date.day() == date.daysInMonth()) || (i_budget_day > 1 && date.day() == i_budget_day - 1) || (i_budget_day <= 0 && date.day() == date.daysInMonth() + i_budget_day - 1));
}
void Budget::addBudgetMonthsSetLast(QDate &date, int months) const {
	if(i_budget_day <= 0) {
		int dfl = date.daysInMonth() - date.day();
		date = date.addMonths(months); 
		if(date.daysInMonth() > dfl) date.setDate(date.year(), date.month(), date.daysInMonth() - dfl);
	} else {
		date = date.addMonths(months); 
	}
	date = lastBudgetDay(date);
}
void Budget::addBudgetMonthsSetFirst(QDate &date, int months) const {
	if(i_budget_day <= 0) {
		int dfl = date.daysInMonth() - date.day();
		date = date.addMonths(months); 
		if(date.daysInMonth() > dfl) date.setDate(date.year(), date.month(), date.daysInMonth() - dfl);
	} else {
		date = date.addMonths(months); 
	}
	date = firstBudgetDay(date);
}
QDate Budget::budgetDateToMonth(QDate date) const {
	if(i_budget_day == 1 && i_budget_month == 1) return date;
	int ibd = i_budget_day;
	int year = date.year();
	int month = date.month();
	if(i_budget_day <= 0) ibd = daysPerMonth(12, year) + i_budget_day;
	if(i_budget_day > 15 || (i_budget_day < 1 && i_budget_day >= -15)) {
		if(date.day() >= ibd) {
			if(date.month() == 12) {year++; month = 1;}
			else month++;
		}
	} else if(date.day() < ibd) {
		if(date.month() == 1) {year--; month = 12;}
		else month--;
	}
	date.setDate(year, month, 1);
	return date;
}
QDate Budget::firstBudgetDay(QDate date) const {
	int ibd = i_budget_day;
	if(i_budget_day < 1) ibd = date.daysInMonth() + i_budget_day;
	if(date.day() < ibd) {
		date = date.addMonths(-1);
		if(i_budget_day < 1) ibd = date.daysInMonth() + i_budget_day;
	}
	date.setDate(date.year(), date.month(), ibd);
	return date;
}
QDate Budget::lastBudgetDay(QDate date) const {
	int ibd = i_budget_day;
	if(i_budget_day < 1) ibd = date.daysInMonth() + i_budget_day;
	if(ibd == 1) {
		date.setDate(date.year(), date.month(), date.daysInMonth());
	} else {
		if(date.day() >= ibd) {
			date = date.addMonths(1);
			if(i_budget_day < 1) ibd = date.daysInMonth() + i_budget_day;
		}
		date.setDate(date.year(), date.month(), ibd - 1);
	}
	return date;
}
QDate Budget::monthToBudgetMonth(QDate date) const {
	int ibd = i_budget_day;
	if(i_budget_day < 1) ibd = date.daysInMonth() + i_budget_day;
	if(date.day() == ibd) return date;
	if(date.day() < ibd && (i_budget_day > 15 || (i_budget_day < 1 && i_budget_day >= -15))) {
		date = date.addMonths(-1);
		if(i_budget_day < 1) ibd = date.daysInMonth() + i_budget_day;
	}
	date.setDate(date.year(), date.month(), ibd);
	return date;
}
void Budget::goForwardBudgetMonths(QDate &from_date, QDate &to_date, int months) const {
	if(isFirstBudgetDay(from_date)) addBudgetMonthsSetFirst(from_date, months);
	else from_date = from_date.addMonths(months);
	if((to_date == QDate::currentDate() && isFirstBudgetDay(from_date))) {
		to_date = lastBudgetDay(to_date);
		addBudgetMonthsSetLast(to_date, months);
	} else if(isLastBudgetDay(to_date)) {
		addBudgetMonthsSetLast(to_date, months);
	} else if(to_date.day() == to_date.daysInMonth()) {
		to_date = to_date.addMonths(months);
		to_date.setDate(to_date.year(), to_date.month(), to_date.daysInMonth());
	} else {
		to_date = to_date.addMonths(months);
	}
}
double Budget::averageMonth(const QDate &date1, const QDate &date2, bool use_budget_months) {
	int saved_i_budget_day = i_budget_day;
	if(!use_budget_months) i_budget_day = 1;
	double average_month = (double) daysInBudgetYear(date1) / (double) 12;
	int years = 1;
	QDate ydate = firstBudgetDayOfYear(date1);
	addBudgetMonthsSetFirst(ydate, 12);
	while(budgetYear(ydate) <= budgetYear(date2)) {
		average_month += (double) daysInBudgetYear(ydate) / (double) 12;
		years++;
		addBudgetMonthsSetFirst(ydate, 12);
	}
	i_budget_day = saved_i_budget_day;
	return average_month / years;
}
double Budget::averageYear(const QDate &date1, const QDate &date2, bool use_budget_months) {
	int saved_i_budget_day = i_budget_day;
	if(!use_budget_months) i_budget_day = 1;
	double average_year = daysInBudgetYear(date1);
	int years = 1;
	QDate ydate = firstBudgetDayOfYear(date1);
	addBudgetMonthsSetFirst(ydate, 12);
	while(budgetYear(ydate) <= budgetYear(date2)) {
		average_year += daysInBudgetYear(ydate);
		years++;
		addBudgetMonthsSetFirst(ydate, 12);
	}
	i_budget_day = saved_i_budget_day;
	return average_year / years;
}
double Budget::yearsBetweenDates(const QDate &date1, const QDate &date2, bool use_budget_months) {
	int saved_i_budget_day = i_budget_day;
	if(!use_budget_months) i_budget_day = 1;
	double years = 0.0;
	if(budgetYear(date1) == budgetYear(date2)) {
		int days = date1.daysTo(date2) + 1;
		years = (double) days / (double) daysInBudgetYear(date2);
	} else {
		years += (1.0 - (dayOfBudgetYear(date1) - 1.0) / (double) daysInBudgetYear(date1));
		years += budgetYear(date2) - (budgetYear(date1) + 1);
		years += (double) dayOfBudgetYear(date2) / (double) daysInBudgetYear(date2);
	}
	i_budget_day = saved_i_budget_day;
	return years;
}
double Budget::monthsBetweenDates(const QDate &date1, const QDate &date2, bool use_budget_months) {
	int saved_i_budget_day = i_budget_day;
	if(!use_budget_months) i_budget_day = 1;
	double months = 0.0;
	if(budgetYear(date1) == budgetYear(date2)) {
		if(budgetMonth(date1) == budgetMonth(date2)) {
			int days = date1.daysTo(date2) + 1;
			months = (double) days / (double) daysInBudgetMonth(date2);
		} else {
			months += (1.0 - ((dayOfBudgetMonth(date1) - 1.0) / (double) daysInBudgetMonth(date1)));
			months += (budgetMonth(date2) - budgetMonth(date1) - 1);
			months += (double) dayOfBudgetMonth(date2) / (double) daysInBudgetMonth(date2);
		}
	} else {
		months += (1.0 - ((dayOfBudgetMonth(date1) - 1.0) / (double) daysInBudgetMonth(date1)));
		months += (12 - budgetMonth(date1));
		months += (budgetYear(date2) - (budgetYear(date1) + 1)) * 12;
		months += (double) dayOfBudgetMonth(date2) / (double) daysInBudgetMonth(date2);
		months += budgetMonth(date2) - 1;
	}
	i_budget_day = saved_i_budget_day;
	return months;
}
int Budget::calendarMonthsBetweenDates(const QDate &date1, const QDate &date2, bool use_budget_months) {
	int saved_i_budget_day = i_budget_day;
	if(!use_budget_months) i_budget_day = 1;
	if(budgetYear(date1) == budgetYear(date2)) {
		return budgetMonth(date2) - budgetMonth(date1);
	}
	int months = 12 - budgetMonth(date1);
	months += (budgetYear(date2) - (budgetYear(date1) + 1)) * 12;
	months += budgetMonth(date2);
	i_budget_day = saved_i_budget_day;
	return months;
}

int Budget::getAccountType(const QString &type, bool localized, bool plural) {
	if(plural && localized) {
		if(type == tr("Transaction Accounts")) {
			return ASSETS_TYPE_CURRENT;
		} else if(type == tr("Savings Accounts")) {
			return  ASSETS_TYPE_SAVINGS;
		} else if(type == tr("Credit Cards")) {
			return  ASSETS_TYPE_CREDIT_CARD;
		} else if(type == tr("Debts")) {
			return  ASSETS_TYPE_LIABILITIES;
		} else if(type == tr("Securities", "Financial security (e.g. stock, mutual fund)")) {
			return  ASSETS_TYPE_SECURITIES;
		} else if(type == tr("Cash")) {
			return ASSETS_TYPE_CASH;
		}
	} else if(localized) {
		if(type == tr("Transaction Account")) {
			return ASSETS_TYPE_CURRENT;
		} else if(type == tr("Savings Account")) {
			return  ASSETS_TYPE_SAVINGS;
		} else if(type == tr("Credit Card")) {
			return  ASSETS_TYPE_CREDIT_CARD;
		} else if(type == tr("Debt")) {
			return  ASSETS_TYPE_LIABILITIES;
		} else if(type == tr("Securities", "Financial security (e.g. stock, mutual fund)")) {
			return  ASSETS_TYPE_SECURITIES;
		} else if(type == tr("Cash")) {
			return  ASSETS_TYPE_CASH;
		}
	} else {
		if(type == "current") {
			return ASSETS_TYPE_CURRENT;
		} else if(type == "savings") {
			return  ASSETS_TYPE_SAVINGS;
		} else if(type == "credit card") {
			return  ASSETS_TYPE_CREDIT_CARD;
		} else if(type == "liabilities") {
			return  ASSETS_TYPE_LIABILITIES;
		} else if(type == "securities") {
			return  ASSETS_TYPE_SECURITIES;
		} else if(type == "balancing") {
			return  ASSETS_TYPE_BALANCING;
		} else if(type == "cash") {
			return  ASSETS_TYPE_CASH;
		}
	}
	return ASSETS_TYPE_OTHER;
}
QString Budget::getAccountTypeName(int at_type, bool localized, bool plural) {
	if(plural && localized) {
		switch(at_type) {
			case ASSETS_TYPE_CASH: return tr("Cash");
			case ASSETS_TYPE_CURRENT: return tr("Transaction Accounts");
			case ASSETS_TYPE_SAVINGS: return tr("Savings Accounts");
			case ASSETS_TYPE_CREDIT_CARD: return tr("Credit Cards");
			case ASSETS_TYPE_LIABILITIES: return tr("Debts");
			case ASSETS_TYPE_SECURITIES: return tr("Securities", "Financial security (e.g. stock, mutual fund)");
			default: return "";
		}
	} else if(localized) {
		switch(at_type) {
			case ASSETS_TYPE_CASH: return tr("Cash");
			case ASSETS_TYPE_CURRENT: return tr("Transaction Account");
			case ASSETS_TYPE_SAVINGS: return tr("Savings Account");
			case ASSETS_TYPE_CREDIT_CARD: return tr("Credit Card");
			case ASSETS_TYPE_LIABILITIES: return tr("Debt");
			case ASSETS_TYPE_SECURITIES: return tr("Securities", "Financial security (e.g. stock, mutual fund)");
			default: return tr("Other");
		}
	} else {
		switch(at_type) {
			case ASSETS_TYPE_CURRENT: return "current";
			case ASSETS_TYPE_SAVINGS: return "savings";
			case ASSETS_TYPE_CREDIT_CARD: return "credit card";
			case ASSETS_TYPE_LIABILITIES: return "liabilities";
			case ASSETS_TYPE_SECURITIES: return "securities";
			case ASSETS_TYPE_BALANCING: return "balancing";
			case ASSETS_TYPE_CASH: return "cash";
			default: return "other";
		}
	}
	return "other";
}
bool Budget::accountTypeIsDebt(int at_type) {
	return at_type == ASSETS_TYPE_LIABILITIES;
}
bool Budget::accountTypeIsSecurities(int at_type) {
	return at_type == ASSETS_TYPE_SECURITIES;
}
bool Budget::accountTypeIsLiabilities(int at_type) {
	return at_type == ASSETS_TYPE_LIABILITIES || at_type == ASSETS_TYPE_CREDIT_CARD;
}
bool Budget::accountTypeIsCreditCard(int at_type) {
	return at_type == ASSETS_TYPE_CREDIT_CARD;
}
bool Budget::accountTypeIsOther(int at_type) {
	return at_type == ASSETS_TYPE_OTHER;
}

void Budget::tagAdded(const QString &tag) {
	tags << tag;
	tags.sort(Qt::CaseInsensitive);
	if(b_record_new_tags) newTags << tag;
}
void Budget::tagRemoved(const QString &tag) {
	tags.removeAll(tag);
}
QString Budget::findTag(const QString &tag) {
	for(int i = 0; i < tags.count(); i++) {
		int c = tags[i].compare(tag, Qt::CaseInsensitive);
		if(c > 0) break;
		if(c == 0) return tags[i];
	}
	return QString();
}
void Budget::setRecordNewTags(bool rnt) {b_record_new_tags = rnt;}

Transactions *Budget::getTransaction(qlonglong lid) {
	for(TransactionList<Transaction*>::const_iterator it = transactions.constBegin(); it != transactions.constEnd(); ++it) {
		if((*it)->id() == lid) return *it;
	}
	for(TransactionList<SplitTransaction*>::const_iterator it = splitTransactions.constBegin(); it != splitTransactions.constEnd(); ++it) {
		if((*it)->id() == lid) return *it;
	}
	for(TransactionList<ScheduledTransaction*>::const_iterator it = scheduledTransactions.constBegin(); it != scheduledTransactions.constEnd(); ++it) {
		if((*it)->id() == lid) return *it;
		if((*it)->transaction() && (*it)->transaction()->id() == lid) return *it;
	}
	return NULL;
}

