// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import HelperWidgets 2.0
import QtQuick.Layouts 1.15
import StudioTheme 1.0 as StudioTheme

Column {
    width: parent.width

    Section {
        width: parent.width
        caption: qsTr("Swipe View")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Interactive")
                tooltip: qsTr("Whether the view is interactive.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.interactive.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.interactive
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Orientation")
                tooltip: qsTr("Orientation of the view.")
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.orientation
                    model: [ "Horizontal", "Vertical" ]
                    scope: "Qt"
                }

                ExpandingSpacer {}
            }
        }
    }

    ContainerSection {}

    ControlSection {}

    PaddingSection {}

    InsetSection {}

    FontSection {
        caption: qsTr("Font Inheritance")
        expanded: false
    }
}
