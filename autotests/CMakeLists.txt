# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

include(ECMAddTests)

find_package(Qt6Test ${QT_MIN_VERSION} REQUIRED)

ecm_add_tests(
    filenavigatortest.cpp
    viewerresolvertest.cpp
    LINK_LIBRARIES kpeekcore Qt6::Test
)

# These classes are QML types compiled into the app, so the tests build their sources directly.
ecm_add_test(archivemodeltest.cpp ../src/archivemodel.cpp
    TEST_NAME archivemodeltest
    LINK_LIBRARIES Qt6::Test Qt6::Qml Qt6::Concurrent KF6::Archive KF6::CoreAddons)
target_include_directories(archivemodeltest PRIVATE ../src)

ecm_add_test(textloadertest.cpp ../src/textloader.cpp
    TEST_NAME textloadertest
    LINK_LIBRARIES Qt6::Test Qt6::Qml)
target_include_directories(textloadertest PRIVATE ../src)
