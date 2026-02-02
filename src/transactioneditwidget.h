/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016-2020 by Hanna Knutsson            *
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

#ifndef TRANSACTION_EDIT_WIDGET_H
#define TRANSACTION_EDIT_WIDGET_H

#include <QDate>
#include <QHash>
#include <QList>
#include <QString>
#include <QVector>
#include <QWidget>
#include <QDialog>
#include <QMenu>
#include <QPushButton>
#include <QPlainTextEdit>

class QCheckBox;
class QLabel;
class QLineEdit;
class QComboBox;
class QHBoxLayout;
class QGridLayout;
class QPlainTextEdit;

class EqonomizeDateEdit;
class QDateEdit;
class TagMenu;
class TagButton;
class LinksWidget;

class Budget;
class Account;
class AccountComboBox;
class EqonomizeValueEdit;
class Security;
class Transaction;
class Transactions;
class MultiAccountTransaction;
class SplitTransaction;
class Currency;

typedef enum {
	SECURITY_ALL_VALUES,
	SECURITY_SHARES_AND_QUOTATION,
	SECURITY_VALUE_AND_SHARES,
	SECURITY_VALUE_AND_QUOTATION
} SecurityValueDefineType;

class TransactionEditWidget : public QWidget {

	Q_OBJECT

	public:

		TransactionEditWidget(bool auto_edit, bool extra_parameters, int transaction_type, Currency *split_currency, bool transfer_to, Security *security, SecurityValueDefineType security_value_type, bool select_security, Budget *budg, QWidget *parent = 0, bool allow_account_creation = false, bool multiaccount = false, bool withloan = false);

		void useMultipleCurrencies(bool b);
		void setTransaction(Transaction *trans);
		void setMultiAccountTransaction(MultiAccountTransaction *split, QDate date = QDate());
		void setTransaction(Transaction *strans, const QDate &date);
		void updateFromAccounts(Account *exclude_account = NULL, Currency *force_currency = NULL, bool set_default = false);
		void updateToAccounts(Account *exclude_account = NULL, Currency *force_currency = NULL, bool set_default = false);
		void updateAccounts(Account *exclude_account = NULL, Currency *force_currency = NULL, bool set_default = false);
		void transactionsReset();
		void setDefaultFromAccount();
		void setDefaultToAccount();
		void setDefaultAccounts();
		void setAccount(Account *account);
		void setToAccount(Account *account);
		void setFromAccount(Account *account);
		void focusFirst();
		bool firstHasFocus() const;
		QHBoxLayout *bottomLayout();
		void transactionRemoved(Transaction *trans);
		void transactionAdded(Transaction *trans);
		void transactionModified(Transaction *trans);
		void tagsModified();
		bool modifyTransaction(Transaction *trans);
		Transaction *createTransaction();
		Transactions *createTransactionWithLoan();
		bool validValues(bool ask_questions = false);
		void setValues(QString description_value, double value_value, double quantity_value, QDate date_value, Account *from_account_value, Account *to_account_value, QString payee_value, QString comment_value);
		void setPayee(QString payee);
		QDate date();
		QString description() const;
		QString payee() const;
		QString comments() const;
		double value() const;
		double quantity() const;
		double quote() const;
		Account *fromAccount() const;
		Account *toAccount() const;
		Security *selectedSecurity();
		bool setQuoteChecked() const;

		void setMaxShares(double max);
		void setMaxSharesDate(QDate quotation_date);
		bool checkAccounts();
		void currentDateChanged(const QDate &olddate, const QDate &newdate);
		bool isCleared();

	protected:

		QHash<QString, Transaction*> default_values;
		QHash<QString, Transaction*> default_payee_values;
		QHash<Account*, Transaction*> default_category_values;
		int transtype;
		bool description_changed, payee_changed;
		Budget *budget;
		Security *security;
		bool b_autoedit, b_sec, b_extra;
		bool value_set, shares_set, sharevalue_set;
		QDate shares_date;
		bool b_create_accounts;
		bool b_multiple_currencies;
		bool b_select_security;
		int b_prev_update_quote;
		Currency *splitcurrency;
		int dateRow, dateLabelCol, dateEditCol, depositRow, depositLabelCol, depositEditCol;

		QLineEdit *descriptionEdit, *lenderEdit, *payeeEdit, *fileEdit, *commentsEditL;
		QWidget *commentsEdit;
		QPlainTextEdit *commentsEditT;
		AccountComboBox *fromCombo, *toCombo;
		QComboBox *securityCombo, *currencyCombo;
		QCheckBox *setQuoteButton;
		QLabel *withdrawalLabel, *depositLabel, *dateLabel, *linksLabelLabel;
		LinksWidget *linksWidget;
		EqonomizeValueEdit *valueEdit, *depositEdit, *downPaymentEdit, *sharesEdit, *quotationEdit, *quantityEdit;
		QPushButton *maxSharesButton;
		TagButton *tagButton;
		EqonomizeDateEdit *dateEdit;
		QHBoxLayout *bottom_layout;
		QGridLayout *editLayout;

	signals:

		void addmodify();
		void dateChanged(const QDate&);
		void accountAdded(Account*);
		void currenciesModified();
		void multipleAccountsRequested();
		void newLoanRequested();
		void propertyChanged();
		void tagAdded(QString);

	public slots:

		void selectFile();
		void openFile();
		void newFromAccount();
		void newToAccount();
		void valueNextField();
		void fromActivated();
		void toActivated();
		void fromChanged(Account*);
		void toChanged(Account*);
		void focusDate();
		void valueEditingFinished();
		void valueChanged(double);
		void securityChanged(int = -1);
		void currencyChanged(int);
		void sharesChanged(double);
		void quotationChanged(double);
		void descriptionChanged(const QString&);
		void setDefaultValue();
		void payeeChanged(const QString&);
		void setDefaultValueFromPayee();
		void setDefaultValueFromCategory();
		void maxShares();
		void setQuoteToggled(bool);
		void newTag();

};

class TransactionEditDialog : public QDialog {

	Q_OBJECT

	public:

		TransactionEditDialog(bool extra_parameters, int transaction_type, Currency *split_currency, bool transfer_to, Security *security, SecurityValueDefineType security_value_type, bool select_security, Budget *budg, QWidget *parent, bool allow_account_creation = false, bool multiaccount = false, bool withloan = false);
		TransactionEditWidget *editWidget;

	protected:

		void keyPressEvent(QKeyEvent*);

	protected slots:

		void accept();

};

class MultipleTransactionsEditDialog : public QDialog {

	Q_OBJECT

	public:

		MultipleTransactionsEditDialog(bool extra_parameters, int transaction_type, Budget *budg, QWidget *parent = 0, bool allow_account_creation = false);
		void setTransaction(Transaction *trans);
		void setTransaction(Transaction *strans, const QDate &date);
		void updateAccounts();
		bool modifyTransaction(Transaction *trans, bool change_parent = false);
		bool modifySplitTransaction(SplitTransaction *trans);
		bool validValues();
		bool checkAccounts();
		QDate date();

		QCheckBox *descriptionButton, *valueButton, *categoryButton, *dateButton, *payeeButton, *accountButton;

	protected:

		int transtype;
		Budget *budget;
		bool b_extra;
		bool b_create_accounts;
		QVector<Account*> categories;
		QLineEdit *descriptionEdit, *payeeEdit;
		AccountComboBox *categoryCombo, *accountCombo;
		EqonomizeValueEdit *valueEdit;
		QDateEdit *dateEdit;
		Account *added_account;

	protected slots:

		void accept();

};

class TagMenu : public QMenu {

	Q_OBJECT

	public:

		TagMenu(Budget*, QWidget *parent = NULL, bool allow_new = false);

		void setTransaction(Transactions *trans);
		void setTransactions(QList<Transactions*> list);
		void modifyTransaction(Transactions *trans, bool append = false);
		int selectedTagsCount();
		QString selectedTagsText();
		void setTagSelected(QString, bool b = true, bool inconsistent = false);
		QString createTag();

	protected:

		QHash<QString, QAction*> tag_actions;

		void keyPressEvent(QKeyEvent *e);
		void mouseReleaseEvent(QMouseEvent *e);

		Budget *budget;
		bool allow_new;

	protected slots:

		void tagToggled();

	public slots:

		void updateTags();

	signals:

		void selectedTagsChanged();
		void newTagRequested();

};

class TagButton : public QPushButton {

	Q_OBJECT

	public:

		TagButton(bool small_button, bool allow_new_tag, Budget *budg, QWidget *parent = NULL);
		void setTagSelected(QString tag, bool b = true, bool inconsistent = false);
		void setTransaction(Transactions *trans);
		void setTransactions(QList<Transactions*> list);
		void modifyTransaction(Transactions *trans, bool append = false);
		QString createTag();

		bool icon_shown;

	public slots:

		void resizeTagMenu();
		void updateText();
		void updateTags();

	protected:

		TagMenu *tagMenu;
		bool b_small;

		void keyPressEvent(QKeyEvent *e);

	signals:

		void returnPressed();
		void newTagRequested();

};

class LinksWidget : public QWidget {

	Q_OBJECT

	protected:

		QLabel *linksLabel;
		QPushButton *removeButton;
		bool b_editable, b_links;
		int first_parent_link;
		QList<Transactions*> links;

	public:

		LinksWidget(QWidget *parent, bool is_active = true);

		void setTransaction(Transactions *trans);
		void updateTransaction(Transactions *trans);
		void updateLabel();
		bool isEmpty();

	public slots:

		void linkClicked(const QString&);
		void removeLink();

};

class CommentsTextEdit : public QPlainTextEdit {

	public:

		CommentsTextEdit(QWidget *parent = 0);
		QSize sizeHint() const;

	protected:

		void keyPressEvent(QKeyEvent *e);

};

#endif

