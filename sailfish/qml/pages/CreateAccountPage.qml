/*
 *  Copyright (C) 2026 SailJami contributors
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *
 *  Sailfish counterpart of the Android account creation steps
 *  (frag_acc_profile_create.xml + frag_acc_jami_password.xml).
 */
import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    id: dialog

    canAccept: !passwordSwitch.checked
               || (passwordField.text.length >= 6 && passwordField.text === confirmField.text)

    onAccepted: Jami.createAccount(nameField.text, passwordSwitch.checked ? passwordField.text : "",
                                   bootstrapField.text)

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width

            DialogHeader {
                acceptText: qsTr("Create")
                title: qsTr("Create Jami account")
            }

            SectionHeader { text: qsTr("Profile") }

            TextField {
                id: nameField
                width: parent.width
                label: qsTr("Display name")
                placeholderText: label
                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: focus = false
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
                text: qsTr("Your display name is shared with your contacts only.")
            }

            SectionHeader { text: qsTr("Security") }

            TextSwitch {
                id: passwordSwitch
                text: qsTr("Encrypt account with a password")
                description: qsTr("Recommended. The password protects your account archive on this device.")
            }
            PasswordField {
                id: passwordField
                visible: passwordSwitch.checked
                width: parent.width
                label: text.length > 0 && text.length < 6 ? qsTr("At least 6 characters") : qsTr("Password")
            }
            PasswordField {
                id: confirmField
                visible: passwordSwitch.checked
                width: parent.width
                label: text.length > 0 && text !== passwordField.text ? qsTr("Passwords do not match")
                                                                     : qsTr("Confirm password")
            }

            SectionHeader { text: qsTr("Advanced") }

            TextField {
                id: bootstrapField
                width: parent.width
                label: qsTr("DHT bootstrap (host:port)")
                placeholderText: qsTr("Default: bootstrap.jami.net")
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
                text: qsTr("Leave empty to use the public Jami network. "
                           + "Use a private bootstrap node to join a test network.")
            }
        }
    }
}
