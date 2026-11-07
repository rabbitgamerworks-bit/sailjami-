/*
 *  Copyright (C) 2026 SailJami contributors
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *
 *  Letter avatar, like the generated avatars of the Android client.
 */
import QtQuick 2.0
import Sailfish.Silica 1.0

Rectangle {
    property string name
    property string seed: name
    property int size: Theme.iconSizeMedium

    width: size
    height: size
    radius: size / 2

    // Same idea as the Android client: a stable colour derived from the peer id.
    readonly property var palette: ["#f44336", "#e91e63", "#9c27b0", "#673ab7", "#3f51b5",
                                    "#2196f3", "#009688", "#4caf50", "#ff9800", "#795548"]
    color: {
        var h = 0
        for (var i = 0; i < seed.length; ++i)
            h = (h * 31 + seed.charCodeAt(i)) % 1000003
        return palette[h % palette.length]
    }

    Label {
        anchors.centerIn: parent
        text: name.length > 0 ? name.charAt(0).toUpperCase() : "?"
        color: "white"
        font.pixelSize: parent.size * 0.5
        font.bold: true
    }
}
