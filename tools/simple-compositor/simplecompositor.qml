/*
 * Copyright (c) 2015 - 2019 Jolla Ltd.
 * Copyright (c) 2019 Open Mobile Platform LLC.
 *
 * This file is part of lipstick.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * and appearing in the file LICENSE.LGPL included in the packaging
 * of this file.
 */

import org.nemomobile.lipstick 0.1
import QtQuick 2.2

Compositor
{
    id: root

    color: "transparent"

    property bool doAsync: false

    HwcImage {
        id: image1
        asynchronous: root.doAsync
        textureSize: Qt.size(540, 960);
    }

    HwcImage {
        id: image2
        asynchronous: root.doAsync
        x: 100
        y: 100
        textureSize: Qt.size(540, 960);
    }

    Timer {
        running: true
        interval: 1000
        repeat: false
        onTriggered: {
            print("setting the first batch of sources...", root.doAsync ? "async" : "sync");
            // Note: This is default content of the main user.
            image1.source = "/home/nemo/Pictures/Jolla/Jolla_01.jpg"
            image2.source = "/home/nemo/Pictures/Jolla/Jolla_02.jpg"
            // image3.source = "/home/nemo/Pictures/Jolla/Jolla_03.jpg"
            // image4.source = "/home/nemo/Pictures/Jolla/Jolla_04.jpg"
            print("All done setting sources...")
        }
    }

    Timer {
        running: true
        interval: 400
        repeat: true
        onTriggered: {
            print("toggling visibility...");
            image1.visible = !image1.visible;
        }
    }

    // Text {
    //     color: "white"
    //     anchors.centerIn: parent
    //     text: "Simple Compositor..."
    //     RotationAnimator on rotation { from: 0; to: 360; duration: 5000; loops: -1 }
    //     font.pixelSize: 20
    // }

}
