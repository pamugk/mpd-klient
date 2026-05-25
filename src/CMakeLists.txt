# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

add_subdirectory(settings)

qt_add_library(mpd-kontrol_static STATIC)
ecm_add_qml_module(mpd-kontrol_static
    URI org.kde.mpd-kontrol
    GENERATE_PLUGIN_SOURCE
    QML_FILES
        qml/Main.qml
        qml/LibraryPage.qml
        qml/GridBrowserDelegate.qml
        qml/Sidebar.qml
    DEPENDENCIES
        org.kde.kirigamiaddons.formcard
        org.kde.kirigamiaddons.statefulapp
        org.kde.kirigami
        org.kde.mpd-kontrol.settings
)

target_sources(mpd-kontrol_static PRIVATE
    mpd-kontrolapplication.cpp
    mpd-kontrolapplication.h
)

target_link_libraries(mpd-kontrol_static PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Svg
    KF6::I18n
    KF6::I18nQml
    KF6::IconThemes
    KF6::CoreAddons
    KF6::ConfigCore
    KF6::ConfigGui
    KirigamiAddonsStatefulApp
)
target_include_directories(mpd-kontrol_static PUBLIC ${CMAKE_BINARY_DIR})

if (ANDROID)
    kirigami_package_breeze_icons(ICONS
        list-add
        help-about
        application-exit
        applications-graphics
    )
else()
    target_link_libraries(mpd-kontrol_static PUBLIC Qt::Widgets)
endif()

kconfig_add_kcfg_files(mpd-kontrol_static GENERATE_MOC mpd-kontrolconfig.kcfgc)

add_executable(mpd-kontrol main.cpp)
target_link_libraries(mpd-kontrol PUBLIC
    mpd-kontrol_static
    mpd-kontrol_staticplugin
    mpd-kontrol_settings
    mpd-kontrol_settingsplugin
)
install(TARGETS mpd-kontrol ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
ecm_add_android_apk(mpd-kontrol ANDROID_DIR ${CMAKE_SOURCE_DIR}/android)
