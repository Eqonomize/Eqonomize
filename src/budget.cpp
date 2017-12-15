/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016-2017 by Hanna Knutsson            *
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

#include <QDebug>

#include <locale.h>

#include "recurrence.h"


bool transaction_list_less_than(Transaction *t1, Transaction *t2) {
	if(t1->date() < t2->date()) return true;
	if(t1->date() > t2->date()) return false;
	if(t1->timestamp() < t2->timestamp()) return true;
	if(t1->timestamp() < t2->timestamp()) return false;
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
bool split_list_less_than(SplitTransaction *t1, SplitTransaction *t2) {
	return t1->date() < t2->date() || (t1->date() == t2->date() && (t1->timestamp() < t2->timestamp() || (t1->timestamp() == t2->timestamp() && t2->description().localeAwareCompare(t1->description()) < 0)));
}
bool schedule_list_less_than(ScheduledTransaction *t1, ScheduledTransaction *t2) {
	return t1->date() < t2->date() || (t1->date() == t2->date() && (t1->timestamp() < t2->timestamp() || (t1->timestamp() == t2->timestamp() && t2->description().localeAwareCompare(t1->description()) < 0)));
}
bool trade_list_less_than(SecurityTrade *t1, SecurityTrade *t2) {
	return t1->date < t2->date || (t1->date == t2->date && t1->timestamp < t2->timestamp);
}
bool security_list_less_than(Security *t1, Security *t2) {
	return QString::localeAwareCompare(t1->name(), t2->name()) < 0;
}

int currency_frac_digits() {
#ifdef Q_OS_ANDROID
	return 2;
#else
	char fd = localeconv()->frac_digits;
	if(fd == CHAR_MAX) return 2;
	return fd;
#endif
}

bool currency_symbol_precedes() {
#ifdef Q_OS_ANDROID
	return true;
#else
	return localeconv()->p_cs_precedes;
#endif
}

bool is_zero(double value) {
	return value < 0.0000001 && value > -0.0000001;
}

QString format_money(double v, int precision) {
	if(precision == MONETARY_DECIMAL_PLACES) return QLocale().toCurrencyString(v);
	if(currency_symbol_precedes()) {
		return QLocale().currencySymbol() + " " + QLocale().toString(v, 'f', precision);
	}
	return QLocale().toString(v, 'f', precision) + " " + QLocale().currencySymbol();
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
	currency_euro = new Currency(this, "EUR", "€", tr("European Euro"), 1.0);
	currency_euro->setAsLocal(false);
	addCurrency(currency_euro);
	loadCurrencies();
	default_currency = currency_euro;
	balancingAccount = new AssetsAccount(this, ASSETS_TYPE_BALANCING, tr("Balancing", "Name of account for transactions that adjust account balances"), 0.0);
	balancingAccount->setCurrency(NULL);
	balancingAccount->setId(0);
	assetsAccounts.append(balancingAccount);
	accounts.append(balancingAccount);
	budgetAccount = NULL;
	i_share_decimals = 4;
	i_quotation_decimals = 4;
	i_budget_day = 1;
	b_record_new_accounts = false;
	b_record_new_securities = false;
	i_tcrd = TRANSACTION_CONVERSION_RATE_AT_DATE;
	null_incomes_account = new IncomesAccount(this, QString::null);
}
Budget::~Budget() {}

void Budget::clear() {
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
	assetsAccounts.append(balancingAccount);
	accounts.append(balancingAccount);
	budgetAccount = NULL;
}

QString Budget::formatMoney(double v, int precision, bool show_currency) {
	return default_currency->formatValue(v, precision, show_currency);
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
	
	Currency *cur_usd = findCurrency("USD");
	if(!cur_usd) return tr("USD currency missing.");
	double usd_rate = cur_usd->exchangeRate();
	
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
					cur->setExchangeRate(exrate * usd_rate);
					cur->setExchangeRateSource(EXCHANGE_RATE_SOURCE_MYCURRENCY_NET);
				} else if(!cur) {
					cur = new Currency(this, code, QString(), name, exrate * usd_rate);
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
	
	return QString::null;
}

TransactionConversionRateDate Budget::defaultTransactionConversionRateDate() const {return i_tcrd;}
void Budget::setDefaultTransactionConversionRateDate(TransactionConversionRateDate tcrd) {i_tcrd = tcrd;}

QString Budget::loadFile(QString filename, QString &errors, bool *default_currency_created, bool merge, bool rename_duplicate_accounts, bool rename_duplicate_categories, bool rename_duplicate_securities, bool ignore_duplicate_transactions) {

	QFile file(filename);
	if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return tr("Couldn't open %1 for reading").arg(filename);
	} else if(!file.size()) {
		return QString::null;
	}

	QXmlStreamReader xml(&file);
	if(!xml.readNextStartElement()) return tr("Not a valid Eqonomize! file (XML parse error: \"%1\" at line %2, col %3)").arg(xml.errorString()).arg(xml.lineNumber()).arg(xml.columnNumber());
	if(xml.name() != "EqonomizeDoc") return tr("Invalid root element %1 in XML document").arg(xml.name().toString());

	/*QString s_version = xml.attributes().value("version").toString();
	float f_version = s_version.toFloat();*/
	
	if(!merge) clear();
	errors = "";
	int category_errors = 0, account_errors = 0, transaction_errors = 0, security_errors = 0;

	assetsAccounts_id[balancingAccount->id()] = balancingAccount;
	
	Currency *cur = NULL, *prev_default_cur = default_currency;
	if(default_currency_created) *default_currency_created = false;

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
				scheduledTransactions.append(strans);
				if(strans->transaction()) {
					if(strans->transactiontype() == TRANSACTION_TYPE_SECURITY_BUY || strans->transactiontype() == TRANSACTION_TYPE_SECURITY_SELL) {
						((SecurityTransaction*) strans->transaction())->security()->scheduledTransactions.append(strans);
					} else if(strans->transactiontype() == TRANSACTION_TYPE_INCOME && ((Income*) strans->transaction())->security()) {
						if(strans->transactionsubtype() == TRANSACTION_SUBTYPE_REINVESTED_DIVIDEND) ((Income*) strans->transaction())->security()->scheduledReinvestedDividends.append(strans);
						else ((Income*) strans->transaction())->security()->scheduledDividends.append(strans);
					}
				}
			} else if(!valid) {
				transaction_errors++;
				delete strans;
			}
		} else if(xml.name() == "transaction") {
			SplitTransaction *split = NULL;
			QStringRef type = xml.attributes().value("type");
			bool valid = true;
			if(type == "expense" || type == "refund") {
				Expense *expense = new Expense(this, &xml, &valid);
				if(valid && merge && ignore_duplicate_transactions) {
					for(TransactionList<Expense*>::const_iterator it = expenses.constBegin(); it != expenses.constEnd(); ++it) {
						if((*it)->date() > expense->date()) break;
						else if(expense->equals(*it, false)) {delete expense; expense = NULL; break;}
					}
				}
				if(valid && expense) {
					expenses.append(expense);
					transactions.append(expense);
				} else if(!valid) {
					transaction_errors++;
					delete expense;
				}
			} else if(type == "income" || type == "repayment") {
				Income *income = new Income(this, &xml, &valid);
				if(valid && merge && ignore_duplicate_transactions) {
					for(TransactionList<Income*>::const_iterator it = incomes.constBegin(); it != incomes.constEnd(); ++it) {
						if((*it)->date() > income->date()) break;
						else if(income->equals(*it, false)) {delete income; income = NULL; break;}
					}
				}
				if(valid && income) {
					incomes.append(income);
					transactions.append(income);
				} else if(!valid) {
					transaction_errors++;
					delete income;
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
					incomes.append(income);
					transactions.append(income);
					income->security()->dividends.append(income);
				} else if(!valid) {
					transaction_errors++;
					delete income;
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
					incomes.append(rediv);
					transactions.append(rediv);
					rediv->security()->reinvestedDividends.append(rediv);
				} else if(!valid) {
					transaction_errors++;
					delete rediv;
				}
			} else if(type == "security_trade") {
				QXmlStreamAttributes attr = xml.attributes();
				QDate date = QDate::fromString(attr.value("date").toString(), Qt::ISODate);
				double from_shares = attr.value("from_shares").toDouble();
				double to_shares = attr.value("to_shares").toDouble();
				int from_id = attr.value("from_security").toInt();
				int to_id = attr.value("to_security").toInt();
				qint64 i_time = attr.value("timestamp").toLongLong();
				Security *from_security;
				if(securities_id.contains(from_id)) {
					from_security = securities_id[from_id];
				} else {
					from_security = NULL;
				}
				Security *to_security;
				if(securities_id.contains(to_id)) {
					to_security = securities_id[to_id];
				} else {
					to_security = NULL;
				}
				if(date.isValid() && from_security && to_security && from_security != to_security) {
					bool b_dup = false;
					if(merge && ignore_duplicate_transactions) {
						for(SecurityTradeList<SecurityTrade*>::const_iterator it = securityTrades.constBegin(); it != securityTrades.constEnd(); ++it) {
							if((*it)->date > date) break;
							else if(date == (*it)->date && from_shares == (*it)->from_shares && to_shares == (*it)->to_shares && from_security == (*it)->from_security && to_security == (*it)->to_security) {b_dup = true; break;}
						}
					}
					if(!b_dup) {
						SecurityTrade *ts = new SecurityTrade(date, from_shares, from_security, to_shares, to_security);
						ts->timestamp = i_time;
						securityTrades.append(ts);
						from_security->tradedShares.append(ts);
						to_security->tradedShares.append(ts);
					}
				} else {
					transaction_errors++;
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
					transfers.append(transfer);
					transactions.append(transfer);
				} else if(!valid) {
					transaction_errors++;
					delete transfer;
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
					transfers.append(transfer);
					transactions.append(transfer);
				} else if(!valid) {
					transaction_errors++;
					delete transfer;
				}
			} else if(type == "security_buy") {
				SecurityBuy *trans = new SecurityBuy(this, &xml, &valid);
				if(valid && merge && ignore_duplicate_transactions) {
					for(SecurityTransactionList<SecurityTransaction*>::const_iterator it = trans->security()->transactions.constBegin(); it != trans->security()->transactions.constEnd(); ++it) {
						if((*it)->date() > trans->date()) break;
						else if(trans->equals(*it, false)) {delete trans; trans = NULL; break;}
					}
				}
				if(valid && trans) {
					securityTransactions.append(trans);
					trans->security()->transactions.append(trans);
					transactions.append(trans);
				} else if(!valid) {
					transaction_errors++;
					delete trans;
				}
			} else if(type == "security_sell") {
				SecuritySell *trans = new SecuritySell(this, &xml, &valid);
				if(valid && merge && ignore_duplicate_transactions) {
					for(SecurityTransactionList<SecurityTransaction*>::const_iterator it = trans->security()->transactions.constBegin(); it != trans->security()->transactions.constEnd(); ++it) {
						if((*it)->date() > trans->date()) break;
						else if(trans->equals(*it, false)) {delete trans; trans = NULL; break;}
					}
				}
				if(valid && trans) {
					securityTransactions.append(trans);
					trans->security()->transactions.append(trans);
					transactions.append(trans);
				} else if(!valid) {
					transaction_errors++;
					delete trans;
				}
			} else if(type == "multiitem" || type == "split") {
				split = new MultiItemTransaction(this, &xml, &valid);
			} else if(type == "multiaccount") {
				split = new MultiAccountTransaction(this, &xml, &valid);
			} else if(type == "debtpayment") {
				split = new DebtPayment(this, &xml, &valid);
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
					delete split;
					split = NULL;
				} else if(split) {
					splitTransactions.append(split);
					int c = split->count();
					for(int i = 0; i < c; i++) {
						Transaction *trans = split->at(i);
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
						transactions.append(trans);
					}
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
						incomesAccounts_id[account->id()] = account;
						if(old_account) account->setName(QString("%1 (%2)").arg(account->name()).arg(tr("imported")));
						incomesAccounts.append(account);
						accounts.append(account);
					}
				} else {
					category_errors++;
					delete account;
				}
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
	resetDefaultCurrencyChanged();
	return QString::null;
}

QString Budget::saveFile(QString filename, QFile::Permissions permissions) {

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
	QXmlStreamWriter xml(&ofile);
	xml.setCodec("UTF-8");
	xml.setAutoFormatting(true);
	xml.setAutoFormattingIndent(-1);

	xml.writeStartDocument();
	xml.writeDTD("<!DOCTYPE EqonomizeDoc>");
	xml.writeStartElement("EqonomizeDoc");
	xml.writeAttribute("version", VERSION);
	
	int id = 1;
	for(AccountList<Account*>::const_iterator it = accounts.constBegin(); it != accounts.constEnd(); ++it) {
		Account *account = *it;
		if(account != balancingAccount) {
			account->setId(id);
			id++;
		}
	}
	for(SecurityList<Security*>::const_iterator it = securities.constBegin(); it != securities.constEnd(); ++it) {
		Security *security = *it;
		security->setId(id);
		id++;
	}
	
	xml.writeStartElement("budget_period");
	xml.writeTextElement("first_day_of_month", QString::number(i_budget_day));
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
		xml.writeAttribute("from_security", QString::number(ts->from_security->id()));
		xml.writeAttribute("to_security", QString::number(ts->to_security->id()));
		xml.writeAttribute("date", ts->date.toString(Qt::ISODate));
		xml.writeAttribute("timestamp", QString::number(ts->timestamp));
		xml.writeAttribute("from_shares", QString::number(ts->from_shares, 'f', ts->from_security->decimals()));
		xml.writeAttribute("to_shares", QString::number(ts->to_shares, 'f', ts->to_security->decimals()));
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

	return QString::null;

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
	TransactionList<Transaction*>::const_iterator it = qLowerBound(transactions.constBegin(), transactions.constEnd(), trans, transaction_list_less_than);
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
	for(CurrencyList<Currency*>::const_iterator it = currencies.constBegin(); it != currencies.constEnd(); ++it) {
		Currency *cur = *it;
		if(cur->symbol(false) == symbol) {
			if(!require_unique) return cur;
			else if(found_cur) return NULL;
			else found_cur = cur;
		}
	}
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
	if(i_budget_day == 1) return date.daysInYear();
	int ibd = i_budget_day;
	if(i_budget_day < 0) ibd = 31 + i_budget_day;
	if(i_budget_day > 15 || (i_budget_day < 1 && i_budget_day >= -15)) {
		if(date.month() == 12 && date.day() >= ibd) {
			return date.addYears(1).daysInYear();
		}
	} else {
		if(date.month() == 1 && date.day() < ibd) {
			return date.addYears(-1).daysInYear();
		}
	}
	return date.daysInYear();
}
int Budget::dayOfBudgetYear(const QDate &date) const {
	if(i_budget_day == 1) return date.dayOfYear();
	int ibd = i_budget_day;
	if(i_budget_day < 0) ibd = 31 + i_budget_day;
	if(i_budget_day > 15 || (i_budget_day < 1 && i_budget_day >= -15)) {
		if(date.month() == 12 && date.day() >= ibd) {
			return date.day() - ibd + 1;
		}
		return date.dayOfYear() + 31 - ibd + 1;
	} else {
		if(date.month() == 1 && date.day() < ibd) {
			return date.addYears(-1).daysInYear() + date.day() - ibd + 1;
		}
		return date.daysInYear() - ibd + 1;
	}
}
int Budget::dayOfBudgetMonth(const QDate &date) const {
	if(i_budget_day == 1) return date.day();
	return firstBudgetDay(date).daysTo(date) + 1;
}
int Budget::budgetMonth(const QDate &date) const {
	if(i_budget_day == 1) return date.month();
	int ibd = i_budget_day;
	if(i_budget_day < 0) ibd = 31 + i_budget_day;
	if(i_budget_day > 15 || (i_budget_day < 1 && i_budget_day >= -15)) {
		if(date.day() < ibd) return date.month();
		if(date.month() == 12) return 1;
		return date.month() + 1;
	}
	if(date.day() >= ibd) return date.month();
	if(date.month() == 1) return 12;
	return date.month() - 1;
}
int Budget::budgetYear(const QDate &date) const {
	if(i_budget_day == 1) return date.year();
	int ibd = i_budget_day;
	if(i_budget_day < 0) ibd = 31 + i_budget_day;
	if(i_budget_day > 15 || (i_budget_day < 1 && i_budget_day >= -15)) {
		if(date.month() == 12 && date.day() >= ibd) return date.year() + 1;
	} else {
		if(date.month() == 1 && date.day() < ibd) return date.year() - 1;
	}
	return date.year();
}
QDate Budget::firstBudgetDayOfYear(QDate date) const {
	return date.addDays(-(dayOfBudgetYear(date) - 1));
}
QDate Budget::lastBudgetDayOfYear(QDate date) const {
	return firstBudgetDayOfYear(date).addDays(daysInBudgetYear(date) - 1);
}
bool Budget::isFirstBudgetDay(const QDate &date) const {
	return ((i_budget_day > 0 && date.day() == i_budget_day) || (i_budget_day <= 0 && date.day() == date.daysInMonth() + i_budget_day));
}
bool Budget::isLastBudgetDay(const QDate &date) const {
	return ((i_budget_day == 1 && date.day() == date.daysInMonth()) || (i_budget_day > 1 && date.day() == i_budget_day) || (i_budget_day <= 0 && date.day() == date.daysInMonth() + i_budget_day - 1));
}
void Budget::addBudgetMonthsSetLast(QDate &date, int months) const {
	date = date.addMonths(months); 
	if(i_budget_day == 1) date.setDate(date.year(), date.month(), date.daysInMonth());
	else if(i_budget_day < 1) date.setDate(date.year(), date.month(), date.daysInMonth() + i_budget_day - 1);
	else date.setDate(date.year(), date.month(), i_budget_day - 1);
}
void Budget::addBudgetMonthsSetFirst(QDate &date, int months) const {
	date = date.addMonths(months); 
	if(i_budget_day < 1) date.setDate(date.year(), date.month(), date.daysInMonth() + i_budget_day);
}
QDate Budget::budgetDateToMonth(QDate date) const {
	date.setDate(budgetYear(date), budgetMonth(date), 1);
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



