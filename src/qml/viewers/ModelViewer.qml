// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick3D
import QtQuick3D.AssetUtils
import QtQuick3D.Helpers
import org.kde.kirigami as Kirigami
import io.github.iamandelib.kpeek

/**
 * 3D models (glTF/GLB, OBJ, STL, 3MF) through Qt Quick 3D: drag to orbit, the wheel moves the camera
 * in and out, and the universal zoom (Ctrl+wheel, header buttons) scales the fitted distance.
 * Qt Quick 3D needs a GPU; with the software renderer the info card is shown instead.
 * Importing runs on the GUI thread and needs many times the file size in memory (a 15 MB scan:
 * 0.9 s and 560 MB; 50 MB: 3.4 s and 1.4 GB), so large models load only when asked to.
 */
Rectangle {
    id: root

    property real zoom: 1.0
    readonly property bool hardware: GraphicsInfo.api !== GraphicsInfo.Software
    property bool loadRequested: PeekController.fileSize <= (PeekController.lowMemory ? 4 : 10) * 1024 * 1024

    color: Kirigami.Theme.backgroundColor

    // The software renderer can't create a 3D scene at all (Qt Quick 3D asserts), so the scene
    // is only instantiated on a hardware-accelerated renderer.
    Loader {
        anchors.fill: parent
        active: !root.hardware || !root.loadRequested
        source: "InfoViewer.qml"
    }

    QQC2.Button {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Kirigami.Units.gridUnit * 2
        visible: root.hardware && !root.loadRequested
        icon.name: "view-preview"
        text: i18nc("@action:button", "Load 3D Model")
        onClicked: root.loadRequested = true
    }

    Loader {
        anchors.fill: parent
        active: root.hardware && root.loadRequested
        sourceComponent: Item {
            id: scene

            // Camera distance that fits the whole model in view; set once its bounds are known.
            property real fitDistance: 0
            readonly property real zoom: root.zoom

            function fitCamera() {
                const min = model.bounds.minimum;
                const max = model.bounds.maximum;
                const radius = max.minus(min).length() / 2;
                if (radius <= 0) {
                    return;
                }
                // Centre the model on the orbit origin and back off until its bounding sphere fits the view.
                model.position = min.plus(max).times(-0.5);
                // Start at a three-quarter view: straight down an axis, many models look flat.
                origin.eulerRotation = Qt.vector3d(-20, 35, 0);
                // The bounding sphere touches the view cone at distance r / sin(fov / 2) (not tan: that cuts it off).
                fitDistance = radius / Math.sin(camera.fieldOfView * Math.PI / 360) * 1.05;
                camera.position = Qt.vector3d(0, 0, fitDistance / zoom);
            }

            onZoomChanged: {
                if (fitDistance > 0) {
                    camera.position = camera.position.normalized().times(fitDistance / zoom);
                }
            }

            View3D {
                id: view
                anchors.fill: parent

                environment: SceneEnvironment {
                    backgroundMode: SceneEnvironment.Color
                    clearColor: Kirigami.Theme.backgroundColor
                    antialiasingMode: SceneEnvironment.MSAA
                    // 4× MSAA costs ~90 MB of (often shared) video memory at 1080p; low-memory machines get 2×.
                    antialiasingQuality: PeekController.lowMemory ? SceneEnvironment.Medium : SceneEnvironment.High
                }

                Node {
                    id: origin

                    PerspectiveCamera {
                        id: camera
                        z: 300

                        // A light that moves with the camera, so the side being looked at is always lit.
                        DirectionalLight {
                            brightness: 0.9
                            ambientColor: Qt.rgba(0.35, 0.35, 0.35, 1)
                        }
                    }
                }

                DirectionalLight {
                    eulerRotation.x: -45
                    eulerRotation.y: 30
                    brightness: 0.5
                }

                RuntimeLoader {
                    id: model
                    source: PeekController.url
                    onBoundsChanged: scene.fitCamera()
                }
            }

            OrbitCameraController {
                anchors.fill: view
                origin: origin
                camera: camera
                automaticClipping: true
            }

            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                width: parent.width - Kirigami.Units.gridUnit * 4
                visible: model.status === RuntimeLoader.Error
                icon.name: "dialog-error"
                text: i18nc("@info", "This model could not be loaded")
                explanation: model.errorString
            }
        }
    }
}
