/*
 *  Copyright (C) 2026 SailJami contributors
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *
 *  Sailfish counterpart of the Android conversation screen
 *  (frag_conversation.xml + item_conv_msg_*.xml).
 */
import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

Page {
    id: page

    property string conversationId
    property bool isRequest: false
    property string title

    Component.onCompleted: {
        if (!isRequest)
            Jami.openConversation(conversationId)
    }
    onStatusChanged: {
        if (status === PageStatus.Deactivating && _navigation === PageNavigation.Back)
            Jami.closeConversation()
    }

    SilicaListView {
        id: messages
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: inputPanel.top
        }
        clip: true
        model: page.isRequest ? null : Jami.messages
        spacing: Theme.paddingSmall

        header: PageHeader {
            title: (!page.isRequest && Jami.messages.title) || page.title || qsTr("Conversation")
        }

        onCountChanged: positionViewAtEnd()
        Component.onCompleted: positionViewAtEnd()

        delegate: Loader {
            width: messages.width
            sourceComponent: model.type === "info" ? infoComponent : bubbleComponent
            property var msg: model
        }

        ViewPlaceholder {
            enabled: !page.isRequest && messages.count === 0
            text: qsTr("No messages yet")
            hintText: qsTr("Say hello!")
        }

        VerticalScrollDecorator {}
    }

    Component {
        id: infoComponent
        Label {
            x: Theme.horizontalPageMargin
            width: messages.width - 2 * x
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: msg.body
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
        }
    }

    Component {
        id: bubbleComponent
        Item {
            height: bubble.height
            Rectangle {
                id: bubble
                readonly property real maxWidth: messages.width * 0.78
                anchors.right: msg.outgoing ? parent.right : undefined
                anchors.left: msg.outgoing ? undefined : parent.left
                anchors.rightMargin: Theme.horizontalPageMargin
                anchors.leftMargin: Theme.horizontalPageMargin
                width: Math.min(maxWidth, Math.max(bodyLabel.implicitWidth, timeLabel.implicitWidth)
                                + 2 * Theme.paddingMedium)
                height: bodyLabel.height + timeLabel.height + 2 * Theme.paddingSmall
                radius: Theme.paddingMedium
                color: msg.outgoing ? Theme.rgba(Theme.highlightBackgroundColor, 0.45)
                                    : Theme.rgba(Theme.primaryColor, 0.12)

                Label {
                    id: bodyLabel
                    x: Theme.paddingMedium
                    y: Theme.paddingSmall
                    width: bubble.width - 2 * Theme.paddingMedium
                    wrapMode: Text.Wrap
                    text: msg.body
                    textFormat: Text.PlainText
                    color: msg.outgoing ? Theme.highlightColor : Theme.primaryColor
                }
                Label {
                    id: timeLabel
                    anchors {
                        right: parent.right
                        rightMargin: Theme.paddingMedium
                        top: bodyLabel.bottom
                    }
                    text: Format.formatDate(msg.time, Formatter.TimeValue)
                    font.pixelSize: Theme.fontSizeTiny
                    color: Theme.secondaryColor
                }
            }
        }
    }

    // Invitation banner (Android: conversation request with Accept/Decline buttons)
    Column {
        anchors.centerIn: parent
        width: parent.width - 2 * Theme.horizontalPageMargin
        spacing: Theme.paddingLarge
        visible: page.isRequest

        Label {
            width: parent.width
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            color: Theme.highlightColor
            text: qsTr("%1 wants to start a conversation with you.")
                  .arg(page.title || qsTr("Someone"))
        }
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: Theme.paddingLarge
            Button {
                text: qsTr("Accept")
                onClicked: {
                    Jami.acceptRequest(page.conversationId)
                    page.isRequest = false
                    Jami.openConversation(page.conversationId)
                }
            }
            Button {
                text: qsTr("Decline")
                onClicked: {
                    Jami.declineRequest(page.conversationId)
                    pageStack.pop()
                }
            }
        }
    }

    Item {
        id: inputPanel
        visible: !page.isRequest
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: visible ? input.height : 0

        TextArea {
            id: input
            anchors {
                left: parent.left
                right: sendButton.left
            }
            placeholderText: qsTr("Write message")
            label: placeholderText
            labelVisible: false
        }
        IconButton {
            id: sendButton
            anchors {
                right: parent.right
                rightMargin: Theme.horizontalPageMargin
                verticalCenter: input.top
                verticalCenterOffset: input.textVerticalCenterOffset
            }
            icon.source: "image://theme/icon-m-send"
            enabled: input.text.trim().length > 0
            onClicked: {
                Jami.sendMessage(input.text)
                input.text = ""
            }
        }
    }
}
