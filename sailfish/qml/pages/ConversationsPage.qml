/*
 *  Copyright (C) 2026 SailJami contributors
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *
 *  Sailfish counterpart of the Android home smartlist
 *  (activity_home.xml / frag_smartlist + item_smartlist).
 */
import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

Page {
    id: page

    Connections {
        target: Jami
        onHasAccountChanged: {
            if (!Jami.hasAccount)
                pageStack.replaceAbove(null, Qt.resolvedUrl("WelcomePage.qml"))
        }
    }

    SilicaListView {
        id: list
        anchors.fill: parent
        model: Jami.conversations

        header: PageHeader {
            title: qsTr("Conversations")
            description: Jami.displayName || qsTr("Jami")
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("Account")
                onClicked: pageStack.push(Qt.resolvedUrl("AccountPage.qml"))
            }
            MenuItem {
                text: qsTr("Add contact")
                onClicked: pageStack.push(Qt.resolvedUrl("AddContactPage.qml"))
            }
        }

        section.property: "isRequest"
        section.criteria: ViewSection.FullString
        section.delegate: SectionHeader {
            text: section === "true" ? qsTr("Invitations") : qsTr("Conversations")
            visible: Jami.conversations.requestCount > 0
            height: visible ? implicitHeight : 0
        }

        delegate: ListItem {
            id: item
            contentHeight: Theme.itemSizeMedium
            menu: ContextMenu {
                MenuItem {
                    visible: model.isRequest
                    text: qsTr("Accept")
                    onClicked: Jami.acceptRequest(model.conversationId)
                }
                MenuItem {
                    visible: model.isRequest
                    text: qsTr("Decline")
                    onClicked: Jami.declineRequest(model.conversationId)
                }
                MenuItem {
                    visible: !model.isRequest
                    text: qsTr("Copy contact identifier")
                    onClicked: Jami.copyToClipboard(model.peer)
                }
                MenuItem {
                    visible: !model.isRequest
                    text: qsTr("Remove conversation")
                    onClicked: {
                        var id = model.conversationId
                        item.remorseDelete(function() { Jami.removeConversation(id) })
                    }
                }
            }

            onClicked: pageStack.push(Qt.resolvedUrl("ConversationPage.qml"),
                                      { conversationId: model.conversationId,
                                        isRequest: model.isRequest,
                                        title: model.title })

            Avatar {
                id: avatar
                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                name: model.title
                seed: model.peer || model.conversationId
            }

            Column {
                anchors {
                    left: avatar.right
                    leftMargin: Theme.paddingMedium
                    right: meta.left
                    rightMargin: Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }
                Label {
                    width: parent.width
                    text: model.title
                    truncationMode: TruncationMode.Fade
                    color: item.highlighted ? Theme.highlightColor : Theme.primaryColor
                    font.bold: model.unread > 0
                }
                Label {
                    width: parent.width
                    text: model.lastMessage
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeSmall
                    color: model.isRequest ? Theme.highlightColor
                                           : (item.highlighted ? Theme.secondaryHighlightColor
                                                               : Theme.secondaryColor)
                }
            }

            Column {
                id: meta
                anchors {
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                Label {
                    anchors.right: parent.right
                    text: Format.formatDate(model.lastTime, Formatter.TimepointRelativeCurrentDay)
                    visible: !isNaN(model.lastTime)
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                }
                Rectangle {
                    anchors.right: parent.right
                    visible: model.unread > 0
                    width: Math.max(height, unreadLabel.implicitWidth + Theme.paddingSmall * 2)
                    height: unreadLabel.implicitHeight
                    radius: height / 2
                    color: Theme.highlightBackgroundColor
                    Label {
                        id: unreadLabel
                        anchors.centerIn: parent
                        text: model.unread
                        font.pixelSize: Theme.fontSizeExtraSmall
                    }
                }
            }
        }

        ViewPlaceholder {
            enabled: list.count === 0
            text: qsTr("No conversations yet")
            hintText: qsTr("Pull down to add a contact by its Jami ID")
        }

        VerticalScrollDecorator {}
    }
}
