
########### next target ###############

set(eqonomize_SRCS
    main.cpp
    eqonomize.cpp
    account.cpp
    transaction.cpp
    budget.cpp
    recurrence.cpp
    security.cpp
    eqonomizemonthselector.cpp
    eqonomizevalueedit.cpp
    categoriescomparisonchart.cpp
    overtimechart.cpp
    categoriescomparisonreport.cpp
    importcsvdialog.cpp
    overtimereport.cpp
    editscheduledtransactiondialog.cpp
    editsplitdialog.cpp
    ledgerdialog.cpp
    recurrenceeditwidget.cpp
    transactioneditwidget.cpp
    transactionfilterwidget.cpp
    transactionlistwidget.cpp
    qifimportexport.cpp)

add_executable(eqonomize ${eqonomize_SRCS})

target_link_libraries(eqonomize Qt5::Widgets Qt5::Gui Qt5::Xml Qt5::Network Qt5::PrintSupport)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

install(TARGETS eqonomize ${INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install(FILES  eqonomize.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
find_package(SharedMimeInfo REQUIRED)
install(FILES  x-eqonomize.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})

ecm_install_icons(ICONS 
hi16-apps-eqonomize.png hi16-mimetypes-application-x-eqonomize.png
hi22-apps-eqonomize.png hi22-mimetypes-application-x-eqonomize.png
hi32-apps-eqonomize.png hi32-mimetypes-application-x-eqonomize.png
hi48-apps-eqonomize.png hi48-mimetypes-application-x-eqonomize.png
hi64-apps-eqonomize.png hi64-mimetypes-application-x-eqonomize.png
hi128-apps-eqonomize.png hi128-mimetypes-application-x-eqonomize.png
DESTINATION ${ICON_INSTALL_DIR})

