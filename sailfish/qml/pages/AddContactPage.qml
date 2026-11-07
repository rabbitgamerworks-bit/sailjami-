/*
 *  Copyright (C) 2026 SailJami contributors
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *
 *  Add a contact by Jami ID (Android: search bar of the smartlist +
 *  "Add to contacts" action).
 */
import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    id: dialog

    readonly property string jamiId: idField.text.trim().toLowerCase()
    canAccept: Jami.isValidJamiId(jamiId) && jamiId !== Jami.jamiId

    onAccepted: Jami.addContact(jamiId)

    Column {
        width: parent.width

        DialogHeader {
            acceptText: qsTr("Add")
            title: qsTr("Add contact")
        }

        TextField {
            id: idField
            width: parent.width
            focus: true
            label: idField.text.length === 0 ? qsTr("Jami ID")
                   : dialog.jamiId === Jami.jamiId ? qsTr("This is your own Jami ID")
                   : Jami.isValidJamiId(dialog.jamiId) ? qsTr("Jami ID")
                   : qsTr("A Jami ID has 40 hexadecimal characters")
            placeholderText: qsTr("Jami ID")
            inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
            font.family: "monospace"
            EnterKey.enabled: dialog.canAccept
            EnterKey.iconSource: "image://theme/icon-m-enter-accept"
            EnterKey.onClicked: dialog.accept()
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.secondaryHighlightColor
            text: qsTr("This will send a connection request. The conversation starts when your contact accepts it.")
        }
    }
}
