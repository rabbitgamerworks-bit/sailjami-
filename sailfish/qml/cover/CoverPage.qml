/*
 *  Copyright (C) 2026 SailJami contributors
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */
import QtQuick 2.0
import Sailfish.Silica 1.0

CoverBackground {
    readonly property int unreadTotal: {
        var n = 0
        for (var i = 0; i < unreadModel.count; ++i)
            n += unreadModel.itemAt(i) ? unreadModel.itemAt(i).unread : 0
        return n
    }

    Repeater {
        id: unreadModel
        model: Jami.conversations
        delegate: Item { property int unread: model.unread }
    }

    Column {
        anchors.centerIn: parent
        width: parent.width - 2 * Theme.paddingLarge
        spacing: Theme.paddingMedium

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Jami"
            font.pixelSize: Theme.fontSizeLarge
            color: Theme.highlightColor
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: Jami.hasAccount
            text: Jami.displayName || Jami.jamiId.substring(0, 8)
            truncationMode: TruncationMode.Fade
            width: Math.min(implicitWidth, parent.width)
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: unreadTotal > 0
            text: qsTr("%n new message(s)", "", unreadTotal)
            color: Theme.highlightColor
            font.pixelSize: Theme.fontSizeSmall
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: Jami.conversations.requestCount > 0
            text: qsTr("%n invitation(s)", "", Jami.conversations.requestCount)
            font.pixelSize: Theme.fontSizeSmall
        }
    }
}
