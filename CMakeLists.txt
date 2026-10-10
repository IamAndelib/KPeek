# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

cmake_minimum_required(VERSION 3.22)

project(kpeek VERSION 0.1.0 LANGUAGES CXX)

set(QT_MIN_VERSION "6.8.0")
set(KF_MIN_VERSION "6.10.0")
set(KPEEK_APP_ID "io.github.iamandelib.kpeek")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDEClangFormat)
include(KDEGitCommitHooks)
include(ECMSetupVersion)
include(FeatureSummary)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

ecm_setup_version(${PROJECT_VERSION} VARIABLE_PREFIX KPEEK VERSION_HEADER ${CMAKE_CURRENT_BINARY_DIR}/kpeek-version.h)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Gui Widgets Concurrent Qml Quick QuickControls2 Test)
qt_standard_project_setup(REQUIRES 6.8)

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS CoreAddons I18n Crash DBusAddons WindowSystem Archive)
find_package(Poppler REQUIRED COMPONENTS Qt6)

# QML modules used only at runtime by the viewers.
find_package(Qt6Multimedia ${QT_MIN_VERSION})
set_package_properties(Qt6Multimedia PROPERTIES TYPE RUNTIME DESCRIPTION "Video and audio previews")
find_package(KF6SyntaxHighlighting ${KF_MIN_VERSION})
set_package_properties(KF6SyntaxHighlighting PROPERTIES TYPE RUNTIME DESCRIPTION "Syntax highlighting in text previews")
find_package(KF6Kirigami ${KF_MIN_VERSION})
set_package_properties(KF6Kirigami PROPERTIES TYPE RUNTIME DESCRIPTION "QML UI framework used by the preview window")

add_subdirectory(src)

if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES src/*.cpp src/*.h autotests/*.cpp autotests/*.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
