# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

add_subdirectory(settings)

qt_add_library(mpd-klient_static STATIC)
ecm_add_qml_module(mpd-klient_static
    URI com.github.pamugk.mpdklient
    GENERATE_PLUGIN_SOURCE
    QML_FILES
        qml/Main.qml
        qml/MediaLibraryPage.qml
        qml/GridBrowserDelegate.qml
        qml/Sidebar.qml
    DEPENDENCIES
        org.kde.kirigamiaddons.formcard
        org.kde.kirigamiaddons.statefulapp
        org.kde.kirigami
        com.github.pamugk.mpd_klient.settings
)

target_sources(mpd-klient_static PRIVATE
    mpdclient.cpp
    mpdclient.h
    mpdklientapplication.cpp
    mpdklientapplication.h
)

target_link_libraries(mpd-klient_static PUBLIC
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
target_link_libraries(mpd-klient_static PRIVATE PkgConfig::LIBMPDCLIENT)
target_include_directories(mpd-klient_static
    PUBLIC ${CMAKE_BINARY_DIR}
    PRIVATE ${LIBMPDCLIENT_INCLUDE_DIRECTORIES}
)

if (ANDROID)
    kirigami_package_breeze_icons(ICONS
        list-add
        help-about
        application-exit
        applications-graphics
    )
else()
    target_link_libraries(mpd-klient_static PUBLIC Qt::Widgets)
endif()

kconfig_add_kcfg_files(mpd-klient_static GENERATE_MOC mpdklientconfig.kcfgc)

add_executable(mpd-klient main.cpp)
target_link_libraries(mpd-klient PUBLIC
    mpd-klient_static
    mpd-klient_staticplugin
    mpd-klient_settings
    mpd-klient_settingsplugin
)
install(TARGETS mpd-klient ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
ecm_add_android_apk(mpd-klient ANDROID_DIR ${CMAKE_SOURCE_DIR}/android)
