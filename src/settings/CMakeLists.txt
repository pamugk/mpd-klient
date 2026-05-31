# SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>
# SPDX-License-Identifier: BSD-2-Clause

qt_add_library(mpd-klient_settings STATIC)
ecm_add_qml_module(mpd-klient_settings
    GENERATE_PLUGIN_SOURCE
    URI com.github.pamugk.mpdklient.settings
    QML_FILES
        MPDKlientConfigurationView.qml
        GeneralPage.qml
)
