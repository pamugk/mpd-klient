// SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>
// SPDX-License-Identifier: LGPL-2.1-or-later

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.statefulapp as StatefulApp
import org.kde.kirigamiaddons.formcard as FormCard

import com.github.pamugk.mpdklient
import com.github.pamugk.mpdklient.settings as Settings

StatefulApp.StatefulWindow {
    id: root

    property int counter: 0

    title: i18nc("@title:window", "mpd-klient")

    windowName: "mpd-klient"

    minimumWidth: Kirigami.Units.gridUnit * 20
    minimumHeight: Kirigami.Units.gridUnit * 20

    application: MPDKlientApplication {
        configurationView: Settings.MPDKlientConfigurationView {}
    }

    ListModel {
        id: libraryModel
    }


    globalDrawer: Sidebar {
        libraryModel: libraryModel
    }

    pageStack.initialPage: MediaLibraryPage {
        libraryModel: libraryModel
    }
}
