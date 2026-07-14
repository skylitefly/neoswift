/*
 * SPDX-FileCopyrightText: Copyright (C) swift Project Community / Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1
 */

import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtLocation 5.12
import QtPositioning 5.12

MapItemGroup {
    id: atcRing

    signal selected(real latitude, real longitude, string frequency)

    property alias position: mainCircle.center
    property alias radius: mainCircle.radius
    property alias cs: idCallsignText.text
    property alias freqAsString: idFrequency.text
    property int freqKhz: 122800

    MapCircle {
        id: mainCircle
        color: "#49aa19"
        border.width: 2
        border.color: "#95de64"
        opacity: 0.28

        MouseArea {
            anchors.fill: parent
            onClicked: {
                idCallsign.visible = !idCallsign.visible
            }
            onDoubleClicked: {
                atcRing.selected(mainCircle.center.latitude, mainCircle.center.longitude, atcRing.freqKhz)
            }
        }
    }

    MapQuickItem {
        id: circleCenter
        sourceItem: Rectangle { width: 8; height: 8; color: "#1677ff"; border.width: 2; border.color: "#69b1ff"; smooth: true; radius: 4 }
        coordinate: mainCircle.center
        opacity:1.0
        anchorPoint: Qt.point(sourceItem.width/2, sourceItem.height/2)
    }

    MapQuickItem {
        id: idCallsign
        visible: false
        coordinate: mainCircle.center
        anchorPoint: Qt.point(-circleCenter.sourceItem.width * 0.5, circleCenter.sourceItem.height * -1.5)

        sourceItem: Item {

            Rectangle {
                color: "#1f1f1f"
                width: idCallsignText.width * 1.3
                height: (idCallsignText.height + idFrequency.height) * 1.3
                border.width: 1
                border.color: "#424242"
                radius: 6
            }

            Text {
                id: idCallsignText
                color: "#d9d9d9"
                font.bold: true
            }

            Text {
                id: idFrequency
                color: "#a6a6a6"
                anchors.top: idCallsignText.bottom
            }
        }
    }
}
