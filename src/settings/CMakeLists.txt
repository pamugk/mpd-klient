# SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>
# SPDX-License-Identifier: BSD-2-Clause

qt_add_library(mpd-kontrol_settings STATIC)
ecm_add_qml_module(mpd-kontrol_settings
    GENERATE_PLUGIN_SOURCE
    URI org.kde.mpd-kontrol.settings
    QML_FILES
        mpd-kontrolConfigurationView.qml
        GeneralPage.qml
)
