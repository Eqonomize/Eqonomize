project(eqonomize)

cmake_minimum_required(VERSION 2.8.12)
find_package(ECM 0.0.10 REQUIRED CONFIG)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
include(KDEInstallDirs)
include(KDECompilerSettings)
include(KDECMakeSettings)
include(FeatureSummary)
include(ECMInstallIcons)
find_package(Qt5 REQUIRED COMPONENTS Widgets Xml Gui PrintSupport)
find_package(KF5 REQUIRED I18n XmlGui Completion WidgetsAddons KHtml Config KIO DBusAddons DocTools JobWidgets)

add_subdirectory(src)
add_subdirectory(doc)
add_subdirectory(po)

