// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick3D 6.0

View3D {
    id: sceneView
    anchors.fill: parent

    property bool usePerspective: false
    property alias showSceneLight: sceneLight.visible
    property alias showGrid: helperGrid.visible
    property alias gridColor: helperGrid.gridColor
    property alias sceneHelpers: sceneHelpers
    property alias perspectiveCamera: scenePerspectiveCamera
    property alias orthoCamera: sceneOrthoCamera
    property double cameraZoomFactor: .55;

    // Empirical cameraZoomFactor values at which the grid zoom level is doubled. The values are
    // approximately uniformally distributed over the non-linear range of cameraZoomFactor.
    readonly property var grid_thresholds: [0.55, 1.10, 2.35, 4.9, 10.0, 20.5, 42.0, 85.0, 999999.0]
    property var thresIdx: 1
    property var thresPerc: 1.0 // percentage of cameraZoomFactor to the current grid zoom threshold (0.0 - 1.0)

    camera: usePerspective ? scenePerspectiveCamera : sceneOrthoCamera

    onCameraZoomFactorChanged: {
        thresIdx = Math.max(1, grid_thresholds.findIndex(v => v > cameraZoomFactor));
        thresPerc = (grid_thresholds[thresIdx] - cameraZoomFactor) / (grid_thresholds[thresIdx] - grid_thresholds[thresIdx - 1]);
    }

    environment: sceneEnv
    SceneEnvironment {
        id: sceneEnv
        antialiasingMode: SceneEnvironment.MSAA
        antialiasingQuality: SceneEnvironment.High
    }

    Node {
        id: sceneHelpers

        HelperGrid {
            id: helperGrid
            lines: Math.pow(2, grid_thresholds.length - thresIdx - 1);
            step: 100 * grid_thresholds[0] * Math.pow(2, thresIdx - 1);
            subdivAlpha: thresPerc;
        }

        PointLight {
            id: sceneLight
            position: usePerspective ? scenePerspectiveCamera.position
                                     : sceneOrthoCamera.position
            quadraticFade: 0
            linearFade: 0
        }

        // Initial camera position and rotation should be such that they look at origin.
        // Otherwise EditCameraController._lookAtPoint needs to be initialized to correct
        // point.
        PerspectiveCamera {
            id: scenePerspectiveCamera
            z: 600
            y: 600
            eulerRotation.x: -45
            clipFar: 100000
            clipNear: 1
        }

        OrthographicCamera {
            id: sceneOrthoCamera
            z: 600
            y: 600
            eulerRotation.x: -45
            clipFar: 100000
            clipNear: -10000
        }
    }
}
