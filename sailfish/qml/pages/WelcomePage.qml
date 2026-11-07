/*
 *  Copyright (C) 2026 SailJami contributors
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *
 *  Sailfish counterpart of the Android account wizard home
 *  (frag_acc_home_create.xml).
 */
import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    Connections {
        target: Jami
        onHasAccountChanged: {
            if (Jami.hasAccount)
                pageStack.replaceAbove(null, Qt.resolvedUrl("ConversationsPage.qml"))
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader { title: qsTr("Welcome to Jami") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeMedium
                text: qsTr("Jami is a free and universal communication platform that respects the "
                           + "privacy and freedoms of its users.")
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                color: Theme.secondaryHighlightColor
                font.pixelSize: Theme.fontSizeSmall
                text: qsTr("Your account is created on this device. No phone number or e-mail "
                           + "address is needed: you are identified by your Jami ID.")
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                preferredWidth: Theme.buttonWidthLarge
                text: qsTr("Create Jami account")
                onClicked: pageStack.push(Qt.resolvedUrl("CreateAccountPage.qml"))
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                text: qsTr("Jami daemon %1").arg(Jami.version)
            }
        }
    }
}
