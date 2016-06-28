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

#include "budget.h"

#include <QDomDocument>
#include <QXmlStreamReader>
#include <QTextStream>
#include <QMap>
#include <QSaveFile>
#include <QFileInfo>

#include <locale.h>

#include "recurrence.h"

int currency_frac_digits() {
	char fd = localeconv()->frac_digits;
	if(fd == CHAR_MAX) return 2;
	return fd;
}

bool currency_symbol_precedes() {
	return localeconv()->p_cs_precedes;
}

QString format_money(double v, int precision) {
	if(precision == MONETARY_DECIMAL_PLACES) return QLocale().toCurrencyString(v);
	if(currency_symbol_precedes()) {
		return QLocale().currencySymbol() + " " + QLocale().toString(v, 'f', precision);
	}
	return QLocale().toString(v, 'f', precision) + " " + QLocale().currencySymbol();
}

Budget::Budget() {
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
	balancingAccount = new AssetsAccount(this, ASSETS_TYPE_BALANCING, tr("Balancing"), 0.0);
	balancingAccount->setId(0);
	assetsAccounts.append(balancingAccount);
	accounts.append(balancingAccount);
	budgetAccount = NULL;
	i_share_decimals = 4;
	i_quotation_decimals = MONETARY_DECIMAL_PLACES;
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
	CategoryAccount *ca = incomesAccounts.first();
	while(ca) {
		if(ca->parentCategory()) ca->o_parent = NULL;
		ca->subCategories.clear();
		ca = incomesAccounts.next();
	}
	ca = expensesAccounts.first();
	while(ca) {
		if(ca->parentCategory()) ca->o_parent = NULL;
		ca->subCategories.clear();
		ca = expensesAccounts.next();
	}
	incomesAccounts.clear();
	expensesAccounts.clear();
	assetsAccounts.clear();
	assetsAccounts.append(balancingAccount);
	accounts.append(balancingAccount);
	budgetAccount = NULL;
}

QString Budget::loadFile(QString filename, QString &errors) {
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
	
	clear();
	errors = "";
	int category_errors = 0, account_errors = 0, transaction_errors = 0, security_errors = 0;

	assetsAccounts_id[balancingAccount->id()] = balancingAccount;
	
	while(xml.readNextStartElement()) {
		if(xml.name() == "schedule") {
			bool valid = true;
			ScheduledTransaction *strans = new ScheduledTransaction(this, &xml, &valid);
			if(valid) {
				scheduledTransactions.append(strans);
				if(strans->transaction()) {
					if(strans->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || strans->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL) {
						((SecurityTransaction*) strans->transaction())->security()->scheduledTransactions.append(strans);
					} else if(strans->transaction()->type() == TRANSACTION_TYPE_INCOME && ((Income*) strans->transaction())->security()) {
						((Income*) strans->transaction())->security()->scheduledDividends.append(strans);
					}
				}
			} else {
				transaction_errors++;
				delete strans;
			}
		} else if(xml.name() == "transaction") {
			QStringRef type = xml.attributes().value("type");
			bool valid = true;
			if(type == "expense" || type == "refund") {
				Expense *expense = new Expense(this, &xml, &valid);
				if(valid) {
					expenses.append(expense);
					transactions.append(expense);
				} else {
					transaction_errors++;
					delete expense;
				}
			} else if(type == "income" || type == "repayment") {
				Income *income = new Income(this, &xml, &valid);
				if(valid) {
					incomes.append(income);
					transactions.append(income);
				} else {
					transaction_errors++;
					delete income;
				}
			} else if(type == "dividend") {
				Income *income = new Income(this, &xml, &valid);
				if(valid && income->security()) {
					incomes.append(income);
					transactions.append(income);
					income->security()->dividends.append(income);
				} else {
					transaction_errors++;
					delete income;
				}
			} else if(type == "reinvested_dividend") {
				QXmlStreamAttributes attr = xml.attributes();
				QDate date = QDate::fromString(attr.value("date").toString(), Qt::ISODate);
				double shares = attr.value("shares").toDouble();
				int id = attr.value("security").toInt();
				Security *security;
				if(securities_id.contains(id)) {
					security = securities_id[id];
				} else {
					security = NULL;
				}
				if(date.isValid() && security) {
					security->reinvestedDividends.append(new ReinvestedDividend(date, shares));
				} else {
					transaction_errors++;
				}
				xml.skipCurrentElement();
			} else if(type == "security_trade") {
				QXmlStreamAttributes attr = xml.attributes();
				QDate date = QDate::fromString(attr.value("date").toString(), Qt::ISODate);
				double value = attr.value("value").toDouble();
				double from_shares = attr.value("from_shares").toDouble();
				double to_shares = attr.value("to_shares").toDouble();
				int from_id = attr.value("from_security").toInt();
				int to_id = attr.value("to_security").toInt();
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
					SecurityTrade *ts = new SecurityTrade(date, value, from_shares, from_security, to_shares, to_security);
					securityTrades.append(ts);
					from_security->tradedShares.append(ts);
					to_security->tradedShares.append(ts);
				} else {
					transaction_errors++;
				}
				xml.skipCurrentElement();
			} else if(type == "transfer") {
				Transfer *transfer = new Transfer(this, &xml, &valid);
				if(valid) {
					transfers.append(transfer);
					transactions.append(transfer);
				} else {
					transaction_errors++;
					delete transfer;
				}
			} else if(type == "balancing") {
				Transfer *transfer = new Balancing(this, &xml, &valid);
				if(valid) {
					transfers.append(transfer);
					transactions.append(transfer);
				} else {
					transaction_errors++;
					delete transfer;
				}
			} else if(type == "security_buy") {
				SecurityBuy *trans = new SecurityBuy(this, &xml, &valid);
				if(valid) {
					securityTransactions.append(trans);
					trans->security()->transactions.append(trans);
					transactions.append(trans);
				} else {
					transaction_errors++;
					delete trans;
				}
			} else if(type == "security_sell") {
				SecuritySell *trans = new SecuritySell(this, &xml, &valid);
				if(valid) {
					securityTransactions.append(trans);
					trans->security()->transactions.append(trans);
					transactions.append(trans);
				} else {
					transaction_errors++;
					delete trans;
				}
			} else if(type == "split") {
				SplitTransaction *split = new SplitTransaction(this, &xml, &valid);
				if(valid) {
					splitTransactions.append(split);
					QVector<Transaction*>::size_type c = split->splits.count();
					for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
						Transaction *trans = split->splits[i];
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
				} else {
					transaction_errors++;
					delete split;
				}
			}
		} else if(xml.name() == "category") {
			QStringRef type = xml.attributes().value("type");
			bool valid = true;
			if(type == "expenses") {
				ExpensesAccount *account = new ExpensesAccount(this, &xml, &valid);
				if(valid) {
					expensesAccounts_id[account->id()] = account;
					expensesAccounts.append(account);
					accounts.append(account);
				} else {
					category_errors++;
					delete account;
				}
			} else if(type == "incomes") {
				IncomesAccount *account = new IncomesAccount(this, &xml, &valid);
				if(valid) {
					incomesAccounts_id[account->id()] = account;
					incomesAccounts.append(account);
					accounts.append(account);
				} else {
					category_errors++;
					delete account;
				}
			}
		} else if(xml.name() == "account") {
			bool valid = true;
			AssetsAccount *account = new AssetsAccount(this, &xml, &valid);
			if(valid) {
				assetsAccounts_id[account->id()] = account;
				assetsAccounts.append(account);
				accounts.append(account);
			} else {
				account_errors++;
				delete account;
			}
		} else if(xml.name() == "security") {
			bool valid = true;
			Security *security = new Security(this, &xml, &valid);
			if(valid) {
				securities_id[security->id()] = security;
				securities.append(security);
				i_quotation_decimals = security->quotationDecimals();
				i_share_decimals = security->decimals();
			} else {
				security_errors++;
				delete security;
			}
		} else {
			errors += tr("Unknown XML element: \"%1\" at line %2, col %3").arg(xml.name().toString()).arg(xml.lineNumber()).arg(xml.columnNumber());
			xml.skipCurrentElement();
		}
	}
	
	if (xml.hasError()) {
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
	Security *security = securities.first();
	while(security) {
		security->dividends.sort();
		security->transactions.sort();
		security->scheduledTransactions.sort();
		security->scheduledDividends.sort();
		security->reinvestedDividends.sort();
		security->tradedShares.sort();
		security = securities.next();
	}
	transactions.sort();
	scheduledTransactions.sort();
	splitTransactions.sort();

	expensesAccounts.sort();
	incomesAccounts.sort();
	assetsAccounts.sort();
	accounts.sort();

	securities.sort();

	bool had_line = false;
	if(account_errors > 0) {
		errors += tr("Unable to load %n account(s).", "", account_errors);
		had_line = true;
	}
	if(category_errors > 0) {
		if(had_line) errors += "\n";
		errors += tr("Unable to load %n category/categories.", "", category_errors);
		had_line = true;
	}
	if(security_errors > 0) {
		if(had_line) errors += "\n";
		errors += tr("Unable to load %n security/securities.", "", security_errors);
		had_line = true;
	}
	if(transaction_errors > 0) {
		if(had_line) errors += "\n";
		errors += tr("Unable to load %n transaction(s).", "", transaction_errors);
		had_line = true;
	}
	file.close();
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

	QTextStream outf(&ofile);
	outf.setCodec("UTF-8");
	QDomDocument doc("EqonomizeDoc");
	doc.appendChild(doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\""));
	QDomElement root = doc.createElement("EqonomizeDoc");
	root.setAttribute("version", "0.5");
	doc.appendChild(root);

	int id = 1;
	Account *account = accounts.first();
	while(account) {
		if(account != balancingAccount) {
			account->setId(id);
			id++;
		}
		account = accounts.next();
	}
	Security *security = securities.first();
	while(security) {
		security->setId(id);
		id++;
		security = securities.next();
	}

	account = accounts.first();
	while(account) {
		if(account != balancingAccount && account->topAccount() == account) {
			switch(account->type()) {
				case ACCOUNT_TYPE_ASSETS: {
					QDomElement e = doc.createElement("account");
					account->save(&e);
					root.appendChild(e);
					break;
				}
				case ACCOUNT_TYPE_INCOMES: {
					QDomElement e = doc.createElement("category");
					e.setAttribute("type", "incomes");
					account->save(&e);
					root.appendChild(e);
					break;
				}
				case ACCOUNT_TYPE_EXPENSES: {
					QDomElement e = doc.createElement("category");
					e.setAttribute("type", "expenses");
					account->save(&e);
					root.appendChild(e);
					break;
				}
			}
		}
		account = accounts.next();
	}

	security = securities.first();
	while(security) {
		QDomElement e = doc.createElement("security");
		security->save(&e);
		root.appendChild(e);
		security = securities.next();
	}

	ScheduledTransaction *strans = scheduledTransactions.first();
	while(strans) {
		QDomElement e = doc.createElement("schedule");
		strans->save(&e);
		root.appendChild(e);
		strans = scheduledTransactions.next();
	}

	SplitTransaction *split = splitTransactions.first();
	while(split) {
		QDomElement e = doc.createElement("transaction");
		e.setAttribute("type", "split");
		split->save(&e);
		root.appendChild(e);
		split = splitTransactions.next();
	}

	security = securities.first();
	while(security) {
		ReinvestedDividend *rediv = security->reinvestedDividends.first();
		while(rediv) {
			QDomElement e = doc.createElement("transaction");
			e.setAttribute("type", "reinvested_dividend");
			e.setAttribute("security", security->id());
			e.setAttribute("date", rediv->date.toString(Qt::ISODate));
			e.setAttribute("shares", QString::number(rediv->shares, 'f', security->decimals()));
			root.appendChild(e);
			rediv = security->reinvestedDividends.next();
		}
		security = securities.next();
	}

	SecurityTrade *ts = securityTrades.first();
	while(ts) {
		QDomElement e = doc.createElement("transaction");
		e.setAttribute("type", "security_trade");
		e.setAttribute("from_security", ts->from_security->id());
		e.setAttribute("to_security", ts->to_security->id());
		e.setAttribute("date", ts->date.toString(Qt::ISODate));
		e.setAttribute("value", QString::number(ts->value, 'f', MONETARY_DECIMAL_PLACES));
		e.setAttribute("from_shares", QString::number(ts->from_shares, 'f', ts->from_security->decimals()));
		e.setAttribute("to_shares", QString::number(ts->to_shares, 'f', ts->to_security->decimals()));
		root.appendChild(e);
		ts = securityTrades.next();
	}

	Transaction *trans = transactions.first();
	while(trans) {
		if(!trans->parentSplit()) {
			QDomElement e = doc.createElement("transaction");
			switch(trans->type()) {
				case TRANSACTION_TYPE_TRANSFER: {
					if(trans->fromAccount() == balancingAccount || trans->toAccount() == balancingAccount) e.setAttribute("type", "balancing");
					else e.setAttribute("type", "transfer");
					break;
				}
				case TRANSACTION_TYPE_INCOME: {
					if(((Income*) trans)->security()) e.setAttribute("type", "dividend");
					else if(trans->value() < 0.0) e.setAttribute("type", "repayment");
					else e.setAttribute("type", "income");
					break;
				}
				case TRANSACTION_TYPE_EXPENSE: {
					if(trans->value() < 0.0) e.setAttribute("type", "refund");
					else e.setAttribute("type", "expense");
					break;
				}
				case TRANSACTION_TYPE_SECURITY_BUY: {
					e.setAttribute("type", "security_buy");
					break;
				}
				case TRANSACTION_TYPE_SECURITY_SELL: {
					e.setAttribute("type", "security_sell");
					break;
				}
			}
			trans->save(&e);
			root.appendChild(e);
		}
		trans = transactions.next();
	}

	outf << doc.toString();

	if(ofile.error() != QFile::NoError) {
		ofile.cancelWriting();
		return tr("Error while writing file; file was not saved");
	}

	if(!ofile.commit()) {
		return tr("Error while writing file; file was not saved");
	}

	return QString::null;

}

void Budget::addTransaction(Transaction *trans) {
	switch(trans->type()) {
		case TRANSACTION_TYPE_EXPENSE: {expenses.inSort((Expense*) trans); break;}
		case TRANSACTION_TYPE_INCOME: {
			incomes.inSort((Income*) trans);
			if(((Income*) trans)->security()) {
				((Income*) trans)->security()->dividends.inSort((Income*) trans);
			}
			break;
		}
		case TRANSACTION_TYPE_TRANSFER: {transfers.inSort((Transfer*) trans); break;}
		case TRANSACTION_TYPE_SECURITY_BUY: {}
		case TRANSACTION_TYPE_SECURITY_SELL: {
			SecurityTransaction *sectrans = (SecurityTransaction*) trans;
			securityTransactions.inSort(sectrans);
			sectrans->security()->transactions.inSort(sectrans);
			sectrans->security()->setQuotation(sectrans->date(), sectrans->shareValue(), true);
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
			if(keep) expenses.setAutoDelete(false);
			expenses.removeRef((Expense*) trans);
			if(keep) expenses.setAutoDelete(true);
			break;
		}
		case TRANSACTION_TYPE_INCOME: {
			if(keep) incomes.setAutoDelete(false);
			if(((Income*) trans)->security()) {
				((Income*) trans)->security()->dividends.removeRef((Income*) trans);
			}
			incomes.removeRef((Income*) trans);
			if(keep) incomes.setAutoDelete(true);
			break;
		}
		case TRANSACTION_TYPE_TRANSFER: {
			if(keep) transfers.setAutoDelete(false);
			transfers.removeRef((Transfer*) trans);
			if(keep) transfers.setAutoDelete(true);
			break;
		}
		case TRANSACTION_TYPE_SECURITY_BUY: {}
		case TRANSACTION_TYPE_SECURITY_SELL: {
			SecurityTransaction *sectrans = (SecurityTransaction*) trans;
			sectrans->security()->removeQuotation(sectrans->date(), true);
			if(keep) securityTransactions.setAutoDelete(false);
			sectrans->security()->transactions.removeRef(sectrans);
			securityTransactions.removeRef(sectrans);
			if(keep) securityTransactions.setAutoDelete(true);
			break;
		}
	}
}
void Budget::addSplitTransaction(SplitTransaction *split) {
	splitTransactions.inSort(split);
	QVector<Transaction*>::size_type c = split->splits.count();
	for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
		addTransaction(split->splits[i]);
	}
}
void Budget::removeSplitTransaction(SplitTransaction *split, bool keep) {
	QVector<Transaction*>::size_type c = split->splits.count();
	for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
		Transaction *trans = split->splits[i];
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
				if(((Income*) trans)->security()) ((Income*) trans)->security()->dividends.removeRef((Income*) trans);
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
	if(strans->transaction()) {
		if(strans->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || strans->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL) {
			((SecurityTransaction*) strans->transaction())->security()->scheduledTransactions.inSort(strans);
		} else if(strans->transaction()->type() == TRANSACTION_TYPE_INCOME && ((Income*) strans->transaction())->security()) {
			((Income*) strans->transaction())->security()->scheduledDividends.inSort(strans);
		}
	}
}
void Budget::removeScheduledTransaction(ScheduledTransaction *strans, bool keep) {
	if(strans->transaction()) {
		 if(strans->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || strans->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL) {
			((SecurityTransaction*) strans->transaction())->security()->scheduledTransactions.removeRef(strans);
		 } else if(strans->transaction()->type() == TRANSACTION_TYPE_INCOME && ((Income*) strans->transaction())->security()) {
			((Income*) strans->transaction())->security()->scheduledDividends.removeRef(strans);
		}
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
}
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
		CategoryAccount *subcat = ((CategoryAccount*) account)->subCategories.first();
		while(subcat) {
			if(!keep) subcat->o_parent = NULL;
			removeAccount(subcat, keep);
			subcat = ((CategoryAccount*) account)->subCategories.next();			
		}
		if(!keep) ((CategoryAccount*) account)->subCategories.clear();
	}
	if(accountHasTransactions(account, false)) {
		Security *security = securities.first();
		while(security) {
			if(security->account() == account) {
				removeSecurity(security);
				security = securities.current();
			} else {
				security = securities.next();
			}
		}
		SplitTransaction *split = splitTransactions.first();
		while(split) {
			if(split->account() == account) {
				removeSplitTransaction(split);
				split = splitTransactions.current();
			}
			split = splitTransactions.next();
		}
		Transaction *trans = transactions.first();
		while(trans) {
			if(trans->fromAccount() == account || trans->toAccount() == account) {
				removeTransaction(trans);
				trans = transactions.current();
			} else {
				trans = transactions.next();
			}
		}
		ScheduledTransaction *strans = scheduledTransactions.first();
		while(strans) {
			if(strans->transaction()->fromAccount() == account || strans->transaction()->toAccount() == account) {
				removeScheduledTransaction(strans);
				strans = scheduledTransactions.current();
			} else {
				strans = scheduledTransactions.next();
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
	Security *security = securities.first();
	while(security) {
		if(security->account() == account) return true;
		security = securities.next();
	}
	SplitTransaction *split = splitTransactions.first();
	while(split) {
		if(split->account() == account) return true;
		split = splitTransactions.next();
	}
	Transaction *trans = transactions.first();
	while(trans) {
		if(trans->fromAccount() == account || trans->toAccount() == account) return true;
		trans = transactions.next();
	}
	ScheduledTransaction *strans = scheduledTransactions.first();
	while(strans) {
		if(strans->transaction()->fromAccount() == account || strans->transaction()->toAccount() == account) return true;
		strans = scheduledTransactions.next();
	}
	if(check_subs && (account->type() == ACCOUNT_TYPE_INCOMES || account->type() == ACCOUNT_TYPE_EXPENSES)) {
		CategoryAccount *subcat = ((CategoryAccount*) account)->subCategories.first();
		while(subcat) {
			if(accountHasTransactions(subcat, true)) return true;
			subcat = ((CategoryAccount*) account)->subCategories.next();
		}
	}
	return false;
}
void Budget::moveTransactions(Account *account, Account *new_account, bool move_from_subs) {
	if(move_from_subs && (account->type() == ACCOUNT_TYPE_INCOMES || account->type() == ACCOUNT_TYPE_EXPENSES)) {
		CategoryAccount *subcat = ((CategoryAccount*) account)->subCategories.first();
		while(subcat) {
			moveTransactions(subcat, new_account);
			subcat = ((CategoryAccount*) account)->subCategories.next();
		}
	}
	if(account->type() == ACCOUNT_TYPE_ASSETS && new_account->type() == ACCOUNT_TYPE_ASSETS) {
		Security *security = securities.first();
		while(security) {
			if(security->account() == account) security->setAccount((AssetsAccount*) new_account);
			security = securities.next();
		}
	}
	SplitTransaction *split = splitTransactions.first();
	while(split) {
		if(split->account() == account) split->setAccount((AssetsAccount*) new_account);
		split = splitTransactions.next();
	}
	Transaction *trans = transactions.first();
	while(trans) {
		if(trans->fromAccount() == account) trans->setFromAccount(new_account);
		if(trans->toAccount() == account) trans->setToAccount(new_account);
		trans = transactions.next();
	}
	ScheduledTransaction *strans = scheduledTransactions.first();
	while(strans) {
		if(strans->transaction()->fromAccount() == account) strans->transaction()->setFromAccount(new_account);
		if(strans->transaction()->toAccount() == account) strans->transaction()->setToAccount(new_account);
		strans = scheduledTransactions.next();
	}
}
void Budget::transactionDateModified(Transaction *t, const QDate &olddate) {
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
				if(i->security()->dividends.removeRef(i)) i->security()->dividends.inSort(i);
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
			tr->security()->removeQuotation(olddate, true);
			tr->security()->setQuotation(tr->date(), tr->shareValue(), true);
			break;
		}
	}
}
void Budget::scheduledTransactionDateModified(ScheduledTransaction *strans) {
	if(strans->transaction()) {
		if(strans->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || strans->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL) {
			if(((SecurityTransaction*) strans->transaction())->security()->scheduledTransactions.removeRef(strans)) ((SecurityTransaction*) strans->transaction())->security()->scheduledTransactions.inSort(strans);
		} else if(strans->transaction()->type() == TRANSACTION_TYPE_INCOME && ((Income*) strans->transaction())->security()) {
			if(((Income*) strans->transaction())->security()->scheduledDividends.removeRef(strans)) ((Income*) strans->transaction())->security()->scheduledDividends.inSort(strans);
		}
	}
	scheduledTransactions.setAutoDelete(false);
	if(scheduledTransactions.removeRef(strans)) scheduledTransactions.inSort(strans);
	scheduledTransactions.setAutoDelete(true);
}
void Budget::splitTransactionDateModified(SplitTransaction *split, const QDate&) {
	splitTransactions.setAutoDelete(false);
	if(splitTransactions.removeRef(split)) splitTransactions.inSort(split);
	splitTransactions.setAutoDelete(true);
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
}
void Budget::removeSecurity(Security *security, bool keep) {
	if(securityHasTransactions(security)) {
		SecurityTransaction *trans = security->transactions.first();
		while(trans) {
			transactions.removeRef(trans);
			securityTransactions.removeRef(trans);
			trans = security->transactions.next();
		}
		Income *i = security->dividends.first();
		while(i) {
			transactions.removeRef(i);
			incomes.removeRef(i);
			i = security->dividends.next();
		}
		ScheduledTransaction *strans = security->scheduledTransactions.first();
		while(strans) {
			scheduledTransactions.removeRef(strans);
			strans = security->scheduledTransactions.next();
		}
		strans = security->scheduledDividends.first();
		while(strans) {
			scheduledTransactions.removeRef(strans);
			strans = security->scheduledDividends.next();
		}
		SecurityTrade *ts = security->tradedShares.first();
		while(ts) {
			ts->from_security->tradedShares.removeRef(ts);
			ts->to_security->tradedShares.removeRef(ts);
			securityTrades.removeRef(ts);
			ts = security->tradedShares.next();
		}
	}
	if(keep) securities.setAutoDelete(false);
	securities.removeRef(security);
	if(keep) securities.setAutoDelete(true);
}
bool Budget::securityHasTransactions(Security *security) {
	return security->reinvestedDividends.count() > 0 || security->tradedShares.count() > 0 || security->transactions.count() > 0 || security->dividends.count() > 0 || security->scheduledTransactions.count() > 0 || security->scheduledDividends.count() > 0;
}
void Budget::securityNameModified(Security *security) {
	securities.setAutoDelete(false);
	if(securities.removeRef(security)) {
		securities.inSort(security);
	}
	securities.setAutoDelete(true);
}
Security *Budget::findSecurity(QString name) {
	Security *sec = securities.first();
	while(sec) {
		if(sec->name() == name) return sec;
		sec = securities.next();
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
	ts->from_security->setQuotation(ts->date, ts->value / ts->from_shares, true);
	ts->to_security->setQuotation(ts->date, ts->value / ts->to_shares, true);
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
	ts->from_security->setQuotation(ts->date, ts->value / ts->from_shares, true);
	ts->to_security->setQuotation(ts->date, ts->value / ts->to_shares, true);
}
Account *Budget::findAccount(QString name) {
	Account *account = accounts.first();
	while(account) {
		if(account->name() == name) return account;
		account = accounts.next();
	}
	return NULL;
}
AssetsAccount *Budget::findAssetsAccount(QString name) {
	AssetsAccount *account = assetsAccounts.first();
	while(account) {
		if(account->name() == name) return account;
		account = assetsAccounts.next();
	}
	return NULL;
}
IncomesAccount *Budget::findIncomesAccount(QString name) {
	IncomesAccount *account = incomesAccounts.first();
	while(account) {
		if(account->name() == name) return account;
		account = incomesAccounts.next();
	}
	return NULL;
}
ExpensesAccount *Budget::findExpensesAccount(QString name) {
	ExpensesAccount *account = expensesAccounts.first();
	while(account) {
		if(account->name() == name) return account;
		account = expensesAccounts.next();
	}
	return NULL;
}
