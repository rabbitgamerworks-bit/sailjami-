/*
 *  Copyright (C) 2026 SailJami contributors
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */
import QtQuick 2.0
import Sailfish.Silica 1.0
import "pages"

ApplicationWindow {
    id: app

    initialPage: Component {
        Page {
            // Startup router: the account wizard when there is no account yet
            // (WelcomePage on Android), the smartlist otherwise.
            id: router
            BusyIndicator {
                anchors.centerIn: parent
                size: BusyIndicatorSize.Large
                running: !Jami.ready
            }
            function route() {
                if (!Jami.ready || status !== PageStatus.Active)
                    return
                pageStack.replace(Qt.resolvedUrl(Jami.hasAccount ? "pages/ConversationsPage.qml"
                                                                 : "pages/WelcomePage.qml"),
                                  {}, PageStackAction.Immediate)
            }
            onStatusChanged: route()
            Connections {
                target: Jami
                onReadyChanged: router.route()
            }
        }
    }

    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    allowedOrientations: defaultAllowedOrientations
}
