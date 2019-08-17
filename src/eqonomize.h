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

#ifndef EQONOMIZE_H
#define EQONOMIZE_H

#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QLabel>
#include <QHash>
#include <QMap>
#include <QModelIndex>
#include <QTextStream>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVector>
#include <QWizardPage>
#include <QUrl>
#include <QApplication>
#include <QMainWindow>
#include <QStyledItemDelegate>
#include <QTranslator>

#ifdef LOAD_EQZICONS_FROM_FILE
	#ifdef RESOURCES_COMPILED
		#define LOAD_APP_ICON(x) QIcon(ICON_DIR "/EQZ/apps/64x64/" x ".png")
		#define LOAD_ICON(x) QIcon(ICON_DIR "/EQZ/actions/64x64/" x ".png")
		#define LOAD_ICON_APP(x) QIcon(ICON_DIR "/EQZ/apps/64x64/" x ".png")
		#define LOAD_ICON_STATUS(x) QIcon(ICON_DIR "/EQZ/status/64x64/" x ".png")
		#define LOAD_ICON2(x, y) QIcon(ICON_DIR "/EQZ/actions/64x64/" y ".png")
	#else
		#define LOAD_APP_ICON(x) QIcon(ICON_DIR "/hicolor/64x64/apps/" x ".png")
		#define LOAD_ICON(x) (QString(x).startsWith("eqz") ? QIcon(ICON_DIR "/hicolor/64x64/actions/" x ".png") : QIcon::fromTheme(x))
		#define LOAD_ICON_APP(x) QIcon::fromTheme(x)
		#define LOAD_ICON_STATUS(x) QIcon::fromTheme(x)
		#define LOAD_ICON2(x, y) (QString(x).startsWith("eqz") ? QIcon(ICON_DIR "/hicolor/64x64/actions/" x ".png") : QIcon::fromTheme(x, QIcon::fromTheme(y)))
	#endif
#else
	#define LOAD_APP_ICON(x) QIcon::fromTheme(x)
	#define LOAD_ICON(x) QIcon::fromTheme(x)
	#define LOAD_ICON_APP(x) QIcon::fromTheme(x)
	#define LOAD_ICON_STATUS(x) QIcon::fromTheme(x)
	#define LOAD_ICON2(x, y) QIcon::fromTheme(x, QIcon::fromTheme(y))
#endif

class QAction;
class QActionGroup;
class QCheckBox;
class QLabel;
class QMenu;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QTabWidget;
class QVBoxLayout;
class QLineEdit;
class QCommandLineParser;
class QToolBar;
class QTabWidget;
class QDateEdit;
class QTextEdit;
class QLocalSocket;
class QLocalServer;
class QPrinter;
class QDialog;
class QNetworkReply;
class TagMenu;

class CategoriesComparisonChart;
class CategoriesComparisonReport;
class CurrencyConversionDialog;
class OverTimeChart;
class OverTimeReport;
class Account;
class AssetsAccount;
class CategoryAccount;
class ExpensesAccount;
class LoanAccount;
class Budget;
class ConfirmScheduleListViewItem;
class EqonomizeMonthSelector;
class EqonomizeValueEdit;
class Expense;
class ExpensesAccount;
class Income;
class IncomesAccount;
class ReinvestedDividend;
class ScheduledTransaction;
class Security;
class SecurityTrade;
class SplitTransaction;
class Transaction;
class Transactions;
class TransactionListWidget;
class Transfer;
class AccountComboBox;

class Eqonomize : public QMainWindow {
	
	Q_OBJECT

	public:

		Eqonomize();
		virtual ~Eqonomize();
		
		void sync(bool do_save = true, bool on_load = false, QWidget *parent = NULL);
		bool saveURL(const QUrl& url, bool do_local_sync = true, bool do_cloud_sync = true, QWidget *parent = NULL);
		bool saveAs(bool do_local_sync = true, bool do_cloud_sync = true, QWidget *parent = NULL);
		bool askSave(bool before_exit = false);
		void createDefaultBudget();
		void readFileDependentOptions();

		Budget *budget;

		bool first_run;
		bool in_batch_edit;

		void startBatchEdit();
		void endBatchEdit();

		void appendFilterExpense(Expense *expense, bool update_total_cost, bool update_accounts);
		void appendFilterIncome(Income *income, bool update_total_income, bool update_accounts);
		void appendFilterTransfer(Transfer *transfer, bool update_total_amount, bool update_accounts);
		bool filterTransaction(Transaction *trans);
		void subtractScheduledTransactionValue(ScheduledTransaction *strans, bool update_value_display);
		void addScheduledTransactionValue(ScheduledTransaction *strans, bool update_value_display, bool subtract = false);
		void subtractTransactionValue(Transaction *trans, bool update_value_display);
		void addTransactionValue(Transaction *trans, const QDate &transdate, bool update_value_display, bool subtract = false, int n = -1, int b_future = -1, const QDate *monthdate = NULL);
		void appendIncomesAccount(IncomesAccount *account, QTreeWidgetItem *parent_item);
		void appendExpensesAccount(ExpensesAccount *account, QTreeWidgetItem *parent_item);
		void assetsAccountItemHiddenOrRemoved(AssetsAccount *account);
		void assetsAccountItemShownOrAdded(AssetsAccount *account);
		void appendAssetsAccount(AssetsAccount *account);
		void appendLoanAccount(LoanAccount *account);
		void updateMonthlyBudget(Account *account);
		void updateTotalMonthlyExpensesBudget();
		void updateTotalMonthlyIncomesBudget();
		void updateBudgetAccountTitle(AssetsAccount *account = NULL);
		bool editAccount(Account*);
		bool editAccount(Account*, QWidget *parent);
		void balanceAccount(Account*);
		bool checkSchedule(bool update_display, QWidget *parent);
		void updateScheduledTransactions();
		void appendScheduledTransaction(ScheduledTransaction *strans);
		bool editScheduledTransaction(ScheduledTransaction *strans);
		bool editScheduledTransaction(ScheduledTransaction *strans, QWidget *parent, bool clone_trans = false);
		bool editOccurrence(ScheduledTransaction *strans, const QDate &date);
		bool editOccurrence(ScheduledTransaction *strans, const QDate &date, QWidget *parent);
		bool editTransaction(Transaction *trans, QWidget *parent, bool clone_trans = false);
		bool editTransaction(Transaction *trans);
		bool removeScheduledTransaction(ScheduledTransaction *strans);
		bool removeOccurrence(ScheduledTransaction *strans, const QDate &date);
		bool newScheduledTransaction(int transaction_type, Security *security = NULL, bool select_security = false);
		bool newScheduledTransaction(int transaction_type, Security *security, bool select_security, QWidget *parent, Account *account = NULL);
		bool newExpenseWithLoan(QString description_value, double value_value, double quantity_value, QDate date_value, ExpensesAccount *category_value, QString payee_value, QString comment_value);
		bool newExpenseWithLoan(QWidget *parent);
		bool newMultiAccountTransaction(bool create_expenses, QString description_string, CategoryAccount *category_account, double quantity_value, QString comment_string);
		bool newMultiAccountTransaction(QWidget *parent, bool create_expenses);
		bool newMultiItemTransaction(QWidget *parent, AssetsAccount *account = NULL);
		bool newDebtPayment(QWidget *parent, AssetsAccount *loan = NULL, bool only_interest = false);
		bool editTimestamp(QList<Transactions*> trans, QWidget *parent = NULL);
		bool editSplitTransaction(SplitTransaction *split);
		bool editSplitTransaction(SplitTransaction *split, QWidget *parent, bool temporary_split = false, bool clone_trans = false);
		bool splitUpTransaction(SplitTransaction *split);
		bool removeSplitTransaction(SplitTransaction *split);
		bool saveView(QTextStream &file, int fileformat);
		bool exportScheduleList(QTextStream &outf, int fileformat);
		bool exportAccountsList(QTextStream &outf, int fileformat);
		bool exportSecuritiesList(QTextStream &outf, int fileformat);
		void editSecurity(QTreeWidgetItem *i);
		void appendSecurity(Security *security);
		void updateSecurity(Security *security);
		void updateSecurity(QTreeWidgetItem *i);
		void updateSecurityAccount(AssetsAccount *account, bool update_display = true);
		bool editSecurityTrade(SecurityTrade *ts, QWidget *parent);
		void editSecurityTrade(SecurityTrade *ts);
		void setModified(bool has_been_modified = true);
		void showExpenses();
		void showIncomes();
		void showTransfers();
		void updateSecuritiesStatistics();
		bool crashRecovery(QUrl url);
		bool newRefundRepayment(Transactions *trans);
		void readOptions();
		void setCommandLineParser(QCommandLineParser*);
		bool timeToUpdateExchangeRates();
		void addNewSchedule(ScheduledTransaction *strans, QWidget *parent);

		QAction *ActionAP_1, *ActionAP_2, *ActionAP_3, *ActionAP_4, *ActionAP_5, *ActionAP_6, *ActionAP_7, *ActionAP_8;
		QAction *ActionEditSchedule, *ActionEditOccurrence, *ActionDeleteSchedule, *ActionDeleteOccurrence;
		QAction *ActionAddAccount, *ActionNewAssetsAccount, *ActionNewLoan, *ActionNewIncomesAccount, *ActionNewExpensesAccount, *ActionEditAccount, *ActionDeleteAccount, *ActionCloseAccount, *ActionBalanceAccount, *ActionAddAccountMenu;
		QAction *ActionShowAccountTransactions, *ActionShowLedger, *ActionReconcileAccount;
		QAction *ActionNewExpense, *ActionNewIncome, *ActionNewTransfer, *ActionNewMultiItemTransaction;
		QAction *ActionNewMultiAccountExpense, *ActionNewExpenseWithLoan, *ActionNewDebtPayment, *ActionNewDebtInterest;
		QAction *ActionEditTransaction, *ActionEditScheduledTransaction, *ActionEditSplitTransaction;
		QAction *ActionJoinTransactions, *ActionSplitUpTransaction;
		QAction *ActionCloneTransaction;
		QAction *ActionEditTimestamp;
		QAction *ActionTags;
		QAction *ActionSelectAssociatedFile, *ActionOpenAssociatedFile;
		QAction *ActionDeleteTransaction, *ActionDeleteScheduledTransaction, *ActionDeleteSplitTransaction;
		QAction *ActionNewSecurity, *ActionEditSecurity, *ActionBuyShares, *ActionSellShares, *ActionNewDividend, *ActionNewReinvestedDividend, *ActionNewSecurityTrade, *ActionSetQuotation, *ActionEditQuotations, *ActionEditSecurityTransactions, *ActionDeleteSecurity;
		QAction *ActionNewRefund, *ActionNewRepayment, *ActionNewRefundRepayment;
		QAction *ActionFileNew, *ActionFileOpen, *ActionFileSave, *ActionFileSaveAs, *ActionFileReload, *ActionFileSync, *ActionSaveView, *ActionPrintView, *ActionPrintPreview, *ActionQuit;
		QMenu *recentFilesMenu;
		QList<QAction*> recentFileActionList;
		QAction *ActionClearRecentFiles;
		QAction *ActionOverTimeReport, *ActionCategoriesComparisonReport, *ActionOverTimeChart, *ActionCategoriesComparisonChart;
		QAction *ActionImportCSV, *ActionImportQIF, *ActionImportEQZ, *ActionExportQIF;
		QAction *ActionConvertCurrencies, *ActionUpdateExchangeRates;
		QAction *ActionExtraProperties, *ActionUseExchangeRateForTransactionDate, *ActionSetBudgetPeriod, *ActionSetScheduleConfirmationTime, *AIPCurrentMonth, *AIPCurrentYear, *AIPCurrentWholeMonth, *AIPCurrentWholeYear, *AIPRememberLastDates, *ABFDaily, *ABFWeekly, *ABFFortnightly, *ABFMonthly, *ABFNever, *ACSTime[11];
		QAction *ActionSetMainCurrency, *ActionSyncSettings, *ActionSelectFont;
		QActionGroup *ActionSelectInitialPeriod, *ActionSelectBackupFrequency, *ActionSelectLang;
		QAction *ActionHelp, *ActionWhatsThis, *ActionReportBug, *ActionAbout, *ActionAboutQt;
		QAction *ActionNewTag, *ActionRenameTag, *ActionRemoveTag;
		
		TagMenu *tagMenu;
		
	protected:

		void setupActions();
		void updateRecentFiles(QString filePath = QString());		
		void saveOptions();
		void closeEvent(QCloseEvent *event);
		
		void dragEnterEvent(QDragEnterEvent *event);
		void dropEvent(QDropEvent *event);

		QUrl current_url;
		double period_months, from_to_months;
		bool modified, modified_auto_save, auto_save_timeout;
		QDate from_date, to_date, frommonth_begin, prevmonth_begin;
		QDate securities_from_date, securities_to_date;
		QDate prev_cur_date;
		bool partial_budget;
		bool b_extra;
		QLocalSocket *socket;
		QLocalServer *server;
		QString cr_tmp_file;

		QToolBar *fileToolbar, *accountsToolbar, *transactionsToolbar, *statisticsToolbar;
		QTabWidget *tabs;
		QTabWidget *accountsTabs;
		QCheckBox *budgetButton;
		EqonomizeValueEdit *budgetEdit;
		EqonomizeMonthSelector *budgetMonthEdit;
		QLabel *prevMonthBudgetLabel;
		QWidget *accounts_page, *expenses_page, *incomes_page, *transfers_page, *securities_page, *schedule_page;
		QVBoxLayout *expensesLayout, *incomesLayout, *transfersLayout;
		TransactionListWidget *expensesWidget, *incomesWidget, *transfersWidget;
		QTreeWidget *accountsView, *securitiesView, *scheduleView;
		QTreeWidgetItem *assetsItem, *liabilitiesItem, *incomesItem, *expensesItem, *tagsItem;
		QCheckBox *accountsPeriodFromButton;
		QDateEdit *accountsPeriodFromEdit, *accountsPeriodToEdit;
		QCheckBox *partialBudgetButton;
		QComboBox *accountsPeriodCombo;
		QCheckBox *securitiesPeriodFromButton;
		QDateEdit *securitiesPeriodFromEdit, *securitiesPeriodToEdit;
		QPushButton *newScheduleButton, *editScheduleButton, *removeScheduleButton;
		QMenu *editScheduleMenu, *removeScheduleMenu;
		QPushButton *newSecurityTransactionButton, *newSecurityButton, *setQuotationButton;
		QLabel *securitiesStatLabel;
		QLabel *footer1;
		QCommandLineParser *parser;
		QComboBox *setMainCurrencyCombo;
		QNetworkReply *updateExchangeRatesReply, *checkVersionReply;
		CurrencyConversionDialog *currencyConversionWindow;
		
		int prev_set_main_currency_index;
		double total_value, total_cost, total_profit, total_rate;
		double expenses_accounts_value, incomes_accounts_value, assets_accounts_value, liabilities_accounts_value;
		double expenses_accounts_change, incomes_accounts_change, assets_accounts_change, liabilities_accounts_change;
		double expenses_budget, expenses_budget_diff, incomes_budget, incomes_budget_diff;
		QMap<Account*, double> account_value;
		QMap<Account*, double> account_change;
		QMap<QString, double> assets_group_value;
		QMap<QString, double> assets_group_change;
		QMap<QString, double> liabilities_group_value;
		QMap<QString, double> liabilities_group_change;
		QMap<QString, double> tag_value;
		QMap<QString, double> tag_change;
		QMap<Account*, QMap<QDate, double> > account_month;
		QMap<Account*, double> account_month_begincur;
		QMap<Account*, double> account_month_beginfirst;
		QMap<Account*, double> account_month_endlast;
		QMap<Account*, double> account_budget;
		QMap<Account*, double> account_budget_diff;
		QMap<Account*, double> account_future_diff;
		QMap<Account*, double> account_future_diff_change;
		QMap<QTreeWidgetItem*, Account*> account_items;
		QMap<QTreeWidgetItem*, QString> assets_group_items, liabilities_group_items, tag_items;
		QMap<Account*, QTreeWidgetItem*> item_accounts;
		QMap<QString, QTreeWidgetItem*> item_assets_groups, item_liabilities_groups, item_tags;
		QMap<QString, QVariant> assets_expanded, liabilities_expanded, expenses_expanded, incomes_expanded;

		QMenu *assetsPopupMenu, *accountPopupMenu, *tagPopupMenu, *securitiesPopupMenu, *schedulePopupMenu, *scheduleHeaderPopupMenu;
		
		QDialog *helpDialog, *cccDialog, *ccrDialog, *otcDialog, *otrDialog, *syncDialog;
		
		QLineEdit *syncUploadEdit, *syncUrlEdit, *syncDownloadEdit;
		QPushButton *uploadButton;
		QCheckBox *syncAutoBox;
		QTreeWidgetItem *clicked_item;
		
	protected slots:
	
		void checkAvailableVersion_readdata();
		void languageSelected();

	public slots:

		void saveCrashRecovery();
		void autoSave();
		void onAutoSaveTimeout();
		
		void onActivateRequested(const QStringList&, const QString&);

		void useExtraProperties(bool);
		
		void useExchangeRateForTransactionDate(bool);

		void updateBudgetDay();
		void setBudgetPeriod();
		
		void setScheduleConfirmationTime();
		
		void selectFont();
		
		void showFilter();
		
		void showHelp();
		void reportBug();
		void showAbout();
		void showAboutQt();
	
		void importCSV();
		void importQIF();
		void importEQZ();
		void exportQIF();
		
		void checkAvailableVersion();
		
		void checkExchangeRatesTimeOut();
		void updateExchangeRates(bool do_currencies_modified = true);
		void cancelUpdateExchangeRates();
		void currenciesModified();
		void warnAndAskForExchangeRate();
		void setMainCurrency();
		void setMainCurrencyIndexChanged(int index);
		void updateUsesMultipleCurrencies();
		void openCurrencyConversion();
		
		void cancelSync();
		void fileSynchronize();
		void openSynchronizationSettings();
		void syncUploadChanged(const QString&);
		void uploadClicked();
		
		void serverNewConnection();
		void socketReadyRead();

		void reloadBudget();

		void showOverTimeReport();
		void showCategoriesComparisonReport();
		void showOverTimeChart();
		void showCategoriesComparisonChart();
		void printPreviewPaint(QPrinter*);
		void showPrintPreview();
		void printView();
		void saveView();

		void newSecurity();
		void editSecurity();
		void deleteSecurity();
		void buySecurities();
		void sellSecurities();
		void newDividend();
		void newReinvestedDividend();
		void newSecurityTrade();
		void setQuotation();
		void editQuotations();
		void editSecurityTransactions();
		void securitiesSelectionChanged();
		void securitiesExecuted(QTreeWidgetItem*);
		void securitiesExecuted(QTreeWidgetItem*, int);
		void popupSecuritiesMenu(const QPoint&);
		void updateSecurities();
		
		void newExpenseWithLoan();
		void newMultiAccountExpense();
		void newMultiAccountIncome();
		void newMultiItemTransaction();
		void newDebtPayment();
		void newDebtInterest();
		void newScheduledExpense();
		void newScheduledIncome();
		void newScheduledTransfer();
		void editScheduledTransaction();
		void cloneScheduledTransaction();
		void editOccurrence();
		void removeScheduledTransaction();
		void removeOccurrence();
		void scheduleSelectionChanged();
		void scheduleExecuted(QTreeWidgetItem*);
		void hideScheduleColumn(bool);
		void popupScheduleHeaderMenu(const QPoint&);
		void popupScheduleMenu(const QPoint&);

		void editSelectedTimestamp();
		void editSelectedScheduledTransaction();
		void editSelectedTransaction();
		void cloneSelectedTransaction();
		void editSelectedSplitTransaction();
		void deleteSelectedScheduledTransaction();
		void deleteSelectedTransaction();
		void deleteSelectedSplitTransaction();
		void joinSelectedTransactions();
		void splitUpSelectedTransaction();
		
		void selectAssociatedFile();
		void openAssociatedFile();

		void newRefund();
		void newRepayment();
		void newRefundRepayment();

		void onPageChange(int);

		void showAccountTransactions(bool = false);
		void showLedger();
		void reconcileAccount();

		void updateTransactionActions();
		
		bool openURL(const QUrl&, bool merge = false);
		void fileNew();
		void fileOpen();
		void fileOpenRecent(const QUrl&);
		void fileReload();
		bool fileSave();
		void onFilterSelected(QString);
		bool fileSaveAs();
		void optionsPreferences();
		void clearRecentFiles();
		void openRecent();

		void checkSchedule();

		void checkDate();

		void popupAccountsMenu(const QPoint&);

		void addAccount();
		void accountAdded(Account *account);
		void securityAdded(Security *security);
		void newAssetsAccount();
		void newLoan();
		void newIncomesAccount(IncomesAccount *default_parent = NULL);
		void newExpensesAccount(ExpensesAccount *default_parent = NULL);
		void accountExecuted(QTreeWidgetItem*, int);
		void accountClicked(QTreeWidgetItem*, int);
		void accountClickedTimeout();
		void accountExecuted(QTreeWidgetItem*);
		void accountExpanded(QTreeWidgetItem*);
		void accountCollapsed(QTreeWidgetItem*);
		void accountMoved(QTreeWidgetItem*, QTreeWidgetItem*);
		void balanceAccount();
		void editAccount();
		void deleteAccount();
		void closeAccount();
		void accountsSelectionChanged();
		
		void tagAdded(QString);
		void newTag();
		void deleteTag();
		void renameTag();
		void tagsChanged();

		void setPartialBudget(bool);

		void budgetEditReturnPressed();
		void budgetMonthChanged(const QDate&);
		void budgetChanged(double);
		void budgetToggled(bool);
		void updateBudgetEdit();
		
		void accountsPeriodFromChanged(const QDate&);
		void accountsPeriodToChanged(const QDate&);
		void periodSelected(QAction*);
		void prevMonth();
		void nextMonth();
		void currentMonth();
		void prevYear();
		void nextYear();
		void currentYear();

		void securitiesPeriodFromChanged(const QDate&);
		void securitiesPeriodToChanged(const QDate&);
		void securitiesPrevMonth();
		void securitiesNextMonth();
		void securitiesCurrentMonth();
		void securitiesPrevYear();
		void securitiesNextYear();
		void securitiesCurrentYear();
		
		void transactionAdded(Transactions*);
		void transactionModified(Transactions*, Transactions*);
		void transactionRemoved(Transactions*, Transactions* = NULL);

		void filterAccounts();

	signals:

		void accountsModified();
		void transactionsModified();
		void budgetUpdated();
		void timeToSaveConfig();
		void tagsModified();

};

class OverTimeReportDialog : public QDialog {
	
	Q_OBJECT
	
	public:
		
		OverTimeReportDialog(Budget *budg, QWidget *parent);
		OverTimeReport *report;

	public slots:
		
		void reject();
		
};

class CategoriesComparisonReportDialog : public QDialog {
	
	Q_OBJECT
	
	public:
		
		CategoriesComparisonReportDialog(bool extra_parameters, Budget *budg, QWidget *parent);
		CategoriesComparisonReport *report;

	public slots:
		
		void reject();
		
};

class CategoriesComparisonChartDialog : public QDialog {
	
	Q_OBJECT
		
	public:
		
		CategoriesComparisonChartDialog(Budget *budg, QWidget *parent);		
		CategoriesComparisonChart *chart;

	public slots:
		
		void reject();
		
};

class OverTimeChartDialog : public QDialog {
	
	Q_OBJECT
		
	public:
		
		OverTimeChartDialog(bool extra_parameters, Budget *budg, QWidget *parent);		
		OverTimeChart *chart;

	public slots:
		
		void reject();
		
};

class ConfirmScheduleDialog : public QDialog {
	
	Q_OBJECT
	
	protected:

		QTreeWidget *transactionsView;
		Budget *budget;
		bool b_extra;
		QPushButton *editButton, *removeButton, *postponeButton;
		int current_index;
		
	public:
		
		ConfirmScheduleDialog(bool extra_parameters, Budget *budg, QWidget *parent, QString title);

		Transactions *firstTransaction();
		Transactions *nextTransaction();

	public slots:
		
		void transactionSelectionChanged();
		void remove();
		void edit();
		void postpone();
		void updateTransactions();
		
};

class EditSecurityDialog : public QDialog {

	Q_OBJECT

	protected:

		QLineEdit *nameEdit;
		QTextEdit *descriptionEdit;
		EqonomizeValueEdit *sharesEdit, *quotationEdit;
		QDateEdit *quotationDateEdit;
		QComboBox *typeCombo;
		AccountComboBox *accountCombo;
		QSpinBox *decimalsEdit, *quotationDecimalsEdit;
		QLabel *quotationLabel, *quotationDateLabel;
		Budget *budget;
		bool b_create_accounts;

	public:
		
		EditSecurityDialog(Budget *budg, QWidget *parent, QString title, bool allow_account_creation = false);
		Security *newSecurity();
		bool modifySecurity(Security *security);
		void setSecurity(Security *security);
		bool checkAccount();

	protected slots:

		void decimalsChanged(int);
		void quotationDecimalsChanged(int);
		void accountActivated(Account*);
		void accept();
		void newAccount();

};

struct q_csv_info;

class EditQuotationsDialog : public QDialog {

	Q_OBJECT

	protected:

		QLabel *titleLabel;
		QTreeWidget *quotationsView;
		EqonomizeValueEdit *quotationEdit;
		QDateEdit *dateEdit;
		QPushButton *changeButton, *addButton, *deleteButton;
		int i_quotation_decimals;
		Budget *budget;
		Security *security;
		
		bool import(QString url, bool test, q_csv_info *ci);

	public:

		EditQuotationsDialog(Security *sec, QWidget *parent);

		void setSecurity(Security *sec);
		void modifyQuotations();

	protected slots:
		
		void onSelectionChanged();
		void addQuotation();
		void changeQuotation();
		void deleteQuotation();
		void importQuotations();
		void exportQuotations();
		void onFilterSelected(QString filter);

};

class RefundDialog : public QDialog {

	Q_OBJECT

	protected:

		Transactions *transaction;
		EqonomizeValueEdit *valueEdit, *quantityEdit;
		QDateEdit *dateEdit;
		AccountComboBox *accountCombo;
		QLineEdit *commentsEdit;
		
		void keyPressEvent(QKeyEvent*);

	public:

		RefundDialog(Transactions *trans, QWidget *parent);

		Transaction *createRefund();
		bool validValues();

	protected slots:
		
		void accept();
		void accountActivated(Account*);

};

class EditSecurityTradeDialog : public QDialog {

	Q_OBJECT

	protected:

		Budget *budget;
		QComboBox *fromSecurityCombo, *toSecurityCombo;
		QLabel *sharesFromLabel, *shareToLabel;
		EqonomizeValueEdit *fromSharesEdit, *toSharesEdit;
		QDateEdit *dateEdit;
		int prev_from_index;
		
		void keyPressEvent(QKeyEvent *e);

	public:

		EditSecurityTradeDialog(Budget *budg, Security *sec, QWidget *parent);

		void setSecurityTrade(SecurityTrade *ts);
		SecurityTrade *createSecurityTrade();
		bool validValues();
		bool checkSecurities();

	protected slots:
		
		void accept();
		void maxShares();
		Security *selectedFromSecurity();
		Security *selectedToSecurity();
		void fromSecurityChanged(bool = false);
		void toSecurityChanged();
		void focusDate();

};

class SecurityTransactionsDialog : public QDialog {
	
	Q_OBJECT
	
	protected:

		Security *security;
		Eqonomize *mainWin;
		QTreeWidget *transactionsView;
		QPushButton *editButton, *removeButton;

		void updateTransactions();
		
	public:
		
		SecurityTransactionsDialog(Security *sec, Eqonomize *parent, QString title);

	protected slots:
		
		void remove();
		void edit();
		void edit(QTreeWidgetItem*);
		void transactionSelectionChanged();
		
};

class EqonomizeTreeWidget : public QTreeWidget {

	Q_OBJECT
	
	public:
	
		EqonomizeTreeWidget(QWidget *parent);
		EqonomizeTreeWidget();
	
	protected:
	
		void dropEvent(QDropEvent *event);

	protected slots:
	
		void keyPressEvent(QKeyEvent*);

	signals:
	
		void returnPressed(QTreeWidgetItem*);
		void spacePressed(QTreeWidgetItem*);
		void itemMoved(QTreeWidgetItem*, QTreeWidgetItem*);

};

class QIFWizardPage : public QWizardPage {
	protected:
		bool is_complete;
	public:		
		QIFWizardPage();
		bool isComplete() const;
		void setComplete(bool b);
};

class EqonomizeComboBox : public QComboBox {

	Q_OBJECT
	
	protected:
	
		bool block_selected;

	public:
	
		EqonomizeComboBox(QWidget *parent);
	
	protected slots:
	
		void keyPressEvent(QKeyEvent *event);
		void itemActivated(int);
		
	signals:
	
		void returnPressed();
		void itemSelected(int);

};

class EqonomizeItemDelegate : public QStyledItemDelegate {

	Q_OBJECT

	public:
		EqonomizeItemDelegate(QObject *parent = NULL);

	public slots:
	
		bool helpEvent(QHelpEvent *e, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index);
		void paint(QPainter *painter, const QStyleOptionViewItem& option, const QModelIndex &index) const;

};


class EqonomizeTranslator : public QTranslator {
	
	Q_OBJECT
	
	public:

		EqonomizeTranslator();
		
		QString	translate(const char *context, const char *sourceText, const char *disambiguation = NULL, int n = -1) const;

};

void open_file_list(QString);

#endif

