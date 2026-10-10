# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

# Logic without UI dependencies, shared with the autotests.
add_library(kpeekcore STATIC
    filenavigator.cpp filenavigator.h
    viewerresolver.cpp viewerresolver.h
)
target_include_directories(kpeekcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kpeekcore PUBLIC Qt6::Core Qt6::Gui)

add_executable(kpeek
    main.cpp
    peekcontroller.cpp peekcontroller.h
    textloader.cpp textloader.h
    pdfdocument.cpp pdfdocument.h
    officeconverter.cpp officeconverter.h
    archivemodel.cpp archivemodel.h
)

set_source_files_properties(qml/ScrollKeys.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(kpeek
    URI org.kde.kpeek
    NO_PLUGIN
    QML_FILES
        qml/Main.qml
        qml/PdfView.qml
        qml/ScrollKeys.qml
        qml/viewers/ArchiveViewer.qml
        qml/viewers/FontViewer.qml
        qml/viewers/ImageViewer.qml
        qml/viewers/InfoViewer.qml
        qml/viewers/MarkdownViewer.qml
        qml/viewers/MediaViewer.qml
        qml/viewers/OfficeViewer.qml
        qml/viewers/PdfViewer.qml
        qml/viewers/TextViewer.qml
)

target_include_directories(kpeek PRIVATE ${CMAKE_BINARY_DIR})
target_compile_definitions(kpeek PRIVATE KPEEK_APP_ID="${KPEEK_APP_ID}")
target_link_libraries(kpeek PRIVATE
    kpeekcore
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Widgets
    Qt6::Concurrent
    Poppler::Qt6
    KF6::Archive
    KF6::CoreAddons
    KF6::I18n
    KF6::I18nQml
    KF6::Crash
    KF6::DBusAddons
    KF6::WindowSystem
)

install(TARGETS kpeek ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
