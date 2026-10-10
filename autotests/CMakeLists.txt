# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

include(ECMAddTests)

find_package(Qt6Test ${QT_MIN_VERSION} REQUIRED)

ecm_add_tests(
    filenavigatortest.cpp
    viewerresolvertest.cpp
    thumbnailcachetest.cpp
    LINK_LIBRARIES kpeekcore Qt6::Test
)

# These classes are QML types compiled into the app, so the tests build their sources directly.
ecm_add_test(archivemodeltest.cpp ../src/archivemodel.cpp
    TEST_NAME archivemodeltest
    LINK_LIBRARIES Qt6::Test Qt6::Qml Qt6::Concurrent LibArchive::LibArchive KF6::CoreAddons)
target_include_directories(archivemodeltest PRIVATE ../src)

ecm_add_test(pdfdocumenttest.cpp ../src/pdfdocument.cpp
    TEST_NAME pdfdocumenttest
    LINK_LIBRARIES Qt6::Test Qt6::Quick Qt6::Concurrent Poppler::Qt6)
target_include_directories(pdfdocumenttest PRIVATE ../src)
set_tests_properties(pdfdocumenttest PROPERTIES ENVIRONMENT "KPEEK_LOW_MEMORY=1;QT_QPA_PLATFORM=offscreen")

ecm_add_test(comicbooktest.cpp ../src/comicbook.cpp
    TEST_NAME comicbooktest
    LINK_LIBRARIES Qt6::Test Qt6::Quick Qt6::Concurrent LibArchive::LibArchive)
target_include_directories(comicbooktest PRIVATE ../src)

ecm_add_test(textloadertest.cpp ../src/textloader.cpp
    TEST_NAME textloadertest
    LINK_LIBRARIES Qt6::Test Qt6::Qml)
target_include_directories(textloadertest PRIVATE ../src)

# QML tests (autotests/qml/tst_*.qml), offscreen.
find_package(Qt6QuickTest ${QT_MIN_VERSION} REQUIRED)
add_executable(qmltests qmltests.cpp)
target_link_libraries(qmltests Qt6::QuickTest Qt6::QuickControls2 Qt6::Widgets)
add_test(NAME qmltests COMMAND qmltests -input ${CMAKE_CURRENT_SOURCE_DIR}/qml)
set_tests_properties(qmltests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

ecm_add_test(viewerinputtest.cpp TEST_NAME viewerinputtest LINK_LIBRARIES Qt6::Test Qt6::Quick)
target_compile_definitions(viewerinputtest PRIVATE KPEEK_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/qml")
set_tests_properties(viewerinputtest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
