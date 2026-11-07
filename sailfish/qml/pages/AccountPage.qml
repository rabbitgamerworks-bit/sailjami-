/*
 *  Copyright (C) 2026 SailJami contributors
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *
 *  Sailfish counterpart of the Android account screen (frag_account.xml).
 */
import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

Page {
    id: page

    function stateText(state) {
        switch (state) {
        case "REGISTERED": return qsTr("Online")
        case "TRYING": return qsTr("Connecting…")
        case "INITIALIZING": return qsTr("Creating account…")
        case "UNREGISTERED": return qsTr("Offline")
        case "": return qsTr("Unknown")
        default: return qsTr("Error (%1)").arg(state)
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        PullDownMenu {
            MenuItem {
                text: qsTr("Delete account")
                onClicked: Remorse.popupAction(page, qsTr("Deleting account"), function() {
                    Jami.removeAccount()
                })
            }
        }

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader { title: qsTr("Account") }

            Avatar {
                anchors.horizontalCenter: parent.horizontalCenter
                size: Theme.iconSizeExtraLarge
                name: Jami.displayName || "J"
                seed: Jami.jamiId
            }

            TextField {
                id: nameField
                width: parent.width
                label: qsTr("Display name")
                text: Jami.displayName
                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: {
                    Jami.setDisplayName(text)
                    focus = false
                }
            }

            DetailItem {
                label: qsTr("Status")
                value: stateText(Jami.registrationState)
            }

            SectionHeader { text: qsTr("Jami ID") }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.WrapAnywhere
                font.family: "monospace"
                color: Theme.highlightColor
                text: Jami.jamiId || qsTr("Generating…")
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                enabled: Jami.jamiId.length > 0
                text: qsTr("Copy Jami ID")
                onClicked: Jami.copyToClipboard(Jami.jamiId)
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
                text: qsTr("Share your Jami ID so that others can contact you.")
            }

            SectionHeader { text: qsTr("Network") }
            TextField {
                width: parent.width
                label: qsTr("DHT bootstrap (host:port)")
                placeholderText: qsTr("Default: bootstrap.jami.net")
                text: Jami.bootstrapHosts
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: {
                    Jami.setBootstrapHosts(text)
                    focus = false
                }
            }
            DetailItem {
                label: qsTr("Jami daemon")
                value: Jami.version
            }
        }
    }
}
