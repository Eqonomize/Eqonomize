project(eqonomize)

cmake_minimum_required(VERSION 2.8.12)
find_package(ECM 0.0.10 REQUIRED CONFIG)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
include(FeatureSummary)
include(ECMInstallIcons)
find_package(Qt5 REQUIRED COMPONENTS Widgets Xml Network Gui PrintSupport)

add_subdirectory(src)
add_subdirectory(doc)
add_subdirectory(po)

