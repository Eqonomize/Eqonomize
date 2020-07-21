# Eqonomize
Eqonomize! is a personal accounting software, with focus on efficiency and ease of use for small households. Eqonomize! provides a complete solution, with bookkeeping by double entry and support for scheduled recurring transactions, security investments, and budgeting. It gives a clear overview of past and present transactions, and development of incomes and expenses, with descriptive tables and charts, as well as an approximation of future account values.

![Image of Eqonomize](https://github.com/Eqonomize/eqonomize.github.io/blob/master/images/accounts.png?raw=true)

## Requirements
* Qt 5 (preferably with Qt Charts module)

## Installation
- Windows
    - [Download the latest version here (file with the `.msi `extension,)](https://github.com/Eqonomize/Eqonomize/releases)
    - Run the installer after downloading
- Snap package
    - `sudo snap install eqonomize-hk`
- AppImage
    - [Download the latest version here](https://github.com/Eqonomize/Eqonomize/releases) (file with the `.msi` extension)
    - Then just run the file
        - You may need to give execution permissions for the file: `chmod +x Eqonomize-*.AppImage`
- Archlinux (AUR) - Unofficial
    - `yay -S eqonomize`
    - Tip: you can set the environment variable `MAKEFLAGS="-j$(nproc)"` before installing the package to make the build more faster
- Linux binary
    - [Download the latest version here](https://github.com/Eqonomize/Eqonomize/releases)
    - Extract the file e give execution permissions for the file: `chmod +x eqonomize`
    - Just run the binary with `eqonomize`
    - Info: with this method, you will not have the entries in your system menu, you will have to create them manually
- Build from source
    1.  Get latest release from GitHub by cloning the repo  
        `git clone https://github.com/Eqonomize/Eqonomize/`
    2.  Change to repo directory  
        `cd Eqonomize`
    3.  Make sure you have the necessary dependencies (`qt5-charts` and `qt5-base`).  
        If you don't have it, just use your distribution's package manager to install them.
    4.  Generate the make file  
        `qmake`
    5.  Update the translation files (not required if using a release source tarball, only if using the git version)  
        `lrelease Eqonomize.pro`
    6.  Run the build  
        Using a single core of your cpu  
        `make` or  
        Using all (much more faster)  
        `make -j$(nproc)`
        For windows use *`nmake`*
    8.  Do the installation  
        `sudo make install`
    9.  Run the application  
        `eqonomize`

## Features
* Bookkeeping
  * Bookkeeping by double entry.
  * Transactions: expenses, incomes, transfers, and security transactions.
  * Transaction properties: description, value, quantity, date, payee/payer, tags, comments, from/to account/category, and additional for securities.
  * Split transactions.
  * Refunds and repayments.
  * Explicit support for loans/debts with interest and fee payments.
  * Schedules transactions, with support for a wide range of recurrency schemes, and confirmation of occurrences.
  * Support for multiple currencies, with selectable currency for each account. Supported currencies are automatically updated.
  * Reconciliation.
  * Parameters of the last entered transaction, with the same description (or category or payee/payer), is automatically filled in when a description (or category/payer/payee) is entered (with auto-completion)
  * Value input fields support arithmetics and currency conversion.
* Budgeting
  * Monthly budget for incomes and expenses categories.
  * Ability to exclude categories from the budget.
  * Custom start day of budget month.
  * Displays previous months performance.
  * Predicts future account values based on the budget and scheduled transactions.
* Securities
  * Stocks, bonds, and mutual funds.
  * Supported transactions: buy and sell of shares, trade of shares between different securities, dividends, and reinvested dividends.
  * Displays value, cost, profit and yearly rate, with present total or for a specific period.
  * Estimates future value and profit based on previous quotation changes and dividends.
* Statistics
  * The main account view displays total values of accounts and categories for at present or a specified date, and value change, as well as budget/remaining budget, for a period.
  * Each transaction list displays basic descriptive statistics and supports filtering of transactions based on date, value, category/account, description, and payee/payer.
  * Line charts, bar charts and tables for display of change of profits, incomes and expenses over time, for all categories, a specific category, or a specific description within a category. Can display value, daily average, monthly averge, yearly average, quantity, and average value for a quantity.
  * Pie charts, bar charts, and tables for comparison of expenses or incomes between different categories, descriptions or payees/payers. Can display value, daily average, monthly averge, yearly average, quantity, and average value for a quantity.
* Saving, import, and export
  * Data is saved in a human readable and editable xml file.
  * Flexible QIF import and export.
  * Displayed data can be saved to a html or csv file, for display online and editing in a spreadsheet.
  * Can import transactions from a csv file, for example a spreadsheet file, with a customizable number of variable transaction parameters.
  * Tables can be saved as html files and charts in a number of different image formats, including png and jpeg.

