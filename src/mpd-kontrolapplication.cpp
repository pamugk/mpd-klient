// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

#include "mpd-kontrolapplication.h"
#include <KAuthorized>
#include <KLocalizedString>

using namespace Qt::StringLiterals;

mpd-kontrolApplication::mpd-kontrolApplication(QObject *parent)
    : AbstractKirigamiApplication(parent)
{
    setupActions();
}

void mpd-kontrolApplication::setupActions()
{
    AbstractKirigamiApplication::setupActions();

    auto actionName = "increment_counter"_L1;
    if (KAuthorized::authorizeAction(actionName)) {
        auto action = mainCollection()->addAction(actionName, this, &mpd-kontrolApplication::incrementCounter);
        action->setText(i18nc("@action:inmenu", "Increment"));
        action->setIcon(QIcon::fromTheme(u"list-add-symbolic"_s));
        mainCollection()->addAction(action->objectName(), action);
        mainCollection()->setDefaultShortcut(action, Qt::CTRL | Qt::Key_I);
    }

    readSettings();
}

#include "moc_mpd-kontrolapplication.cpp"
