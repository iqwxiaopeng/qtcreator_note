/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 2.1
import Monitor 1.0
import QtQuick.Controls 1.0

Rectangle {
    id: root

    // ***** properties

    property int singleRowHeight: 30

    property bool dataAvailable: true
    property int eventCount: 0
    property real progress: 0

    property alias selectionLocked : view.selectionLocked
    signal updateLockButton
    property alias selectedItem: view.selectedItem
    signal selectedEventChanged(int eventId)
    property bool lockItemSelection : false

    property real mainviewTimePerPixel : 0

    signal updateCursorPosition
    property string fileName: ""
    property int lineNumber: -1
    property int columnNumber: 0

    signal updateRangeButton
    property bool selectionRangeMode: false

    property bool selectionRangeReady: selectionRange.ready
    property real selectionRangeStart: selectionRange.startTime
    property real selectionRangeEnd: selectionRange.startTime + selectionRange.duration

    signal changeToolTip(string text)

    property bool recordingEnabled: false
    property bool appKilled : false

    property date recordingStartDate
    property real elapsedTime

    // ***** connections with external objects
    Connections {
        target: zoomControl
        onRangeChanged: {
            var startTime = zoomControl.startTime();
            var endTime = zoomControl.endTime();

            mainviewTimePerPixel = Math.abs(endTime - startTime) / root.width;

            backgroundMarks.updateMarks(startTime, endTime);
            view.updateFlickRange(startTime, endTime);
            flick.setContentWidth();
        }
    }


    Connections {
        target: qmlProfilerModelProxy
        onCountChanged: {
            eventCount = qmlProfilerModelProxy.count();
            if (eventCount === 0)
                root.clearAll();
            if (eventCount > 1) {
                root.progress = Math.min(1.0,
                    (qmlProfilerModelProxy.lastTimeMark() -
                    qmlProfilerModelProxy.traceStartTime()) / root.elapsedTime * 1e-9 );
            } else {
                root.progress = 0;
            }
        }
        onStateChanged: {
            switch (qmlProfilerModelProxy.getState()) {
            case 0: {
                root.clearAll();
                break;
            }
            case 1: {
                root.dataAvailable = false;
                break;
            }
            case 2: {
                root.progress = 0.9; // jump to 90%
                break;
            }
            }
        }
        onDataAvailable: {
            view.clearData();
            zoomControl.setRange(0,0);
            progress = 1.0;
            dataAvailable = true;
            view.visible = true;
            view.requestPaint();
            zoomControl.setRange(qmlProfilerModelProxy.traceStartTime(),
                                 qmlProfilerModelProxy.traceStartTime() +
                                 qmlProfilerModelProxy.traceDuration()/10);
        }
    }


    // ***** functions
    function gotoSourceLocation(file,line,column) {
        if (file !== undefined) {
            root.fileName = file;
            root.lineNumber = line;
            root.columnNumber = column;
            root.updateCursorPosition();
        }
    }

    function clearData() {
        view.clearData();
        dataAvailable = false;
        appKilled = false;
        eventCount = 0;
        hideRangeDetails();
        selectionRangeMode = false;
        updateRangeButton();
        zoomControl.setRange(0,0);
    }

    function clearDisplay() {
        clearData();
        view.visible = false;
    }

    function clearAll() {
        clearDisplay();
        elapsedTime = 0;
    }

    function nextEvent() {
        view.selectNext();
    }

    function prevEvent() {
        view.selectPrev();
    }

    function updateZoomCentered(centerX, relativeFactor)
    {
        var min_length = 1e5; // 0.1 ms
        var windowLength = view.endTime - view.startTime;
        if (windowLength < min_length)
            windowLength = min_length;
        var newWindowLength = windowLength * relativeFactor;

        if (newWindowLength > qmlProfilerModelProxy.traceDuration()) {
            newWindowLength = qmlProfilerModelProxy.traceDuration();
            relativeFactor = newWindowLength / windowLength;
        }
        if (newWindowLength < min_length) {
            newWindowLength = min_length;
            relativeFactor = newWindowLength / windowLength;
        }

        var fixedPoint = (centerX - flick.x) * windowLength / flick.width + view.startTime;
        var startTime = fixedPoint - relativeFactor*(fixedPoint - view.startTime);
        zoomControl.setRange(startTime, startTime + newWindowLength);
    }

    function recenter( centerPoint ) {
        var windowLength = view.endTime - view.startTime;
        var newStart = Math.floor(centerPoint - windowLength/2);
        if (newStart < 0)
            newStart = 0;
        if (newStart + windowLength > qmlProfilerModelProxy.traceEndTime())
            newStart = qmlProfilerModelProxy.traceEndTime() - windowLength;
        zoomControl.setRange(newStart, newStart + windowLength);
    }

    function recenterOnItem( modelIndex, itemIndex )
    {
        if (itemIndex === -1)
            return;

        // if item is outside of the view, jump back to its position
        if (qmlProfilerModelProxy.getEndTime(modelIndex, itemIndex) < view.startTime ||
                qmlProfilerModelProxy.getStartTime(modelIndex, itemIndex) > view.endTime) {
            recenter((qmlProfilerModelProxy.getStartTime(modelIndex, itemIndex) +
                      qmlProfilerModelProxy.getEndTime(modelIndex, itemIndex)) / 2);
        }

    }

    function wheelZoom(wheelCenter, wheelDelta) {
        if (qmlProfilerModelProxy.traceEndTime() > qmlProfilerModelProxy.traceStartTime() &&
                wheelDelta !== 0) {
            if (wheelDelta>0)
                updateZoomCentered(wheelCenter, 1/1.2);
            else
                updateZoomCentered(wheelCenter, 1.2);
        }
    }

    function hideRangeDetails() {
        rangeDetails.visible = false;
        rangeDetails.duration = "";
        rangeDetails.label = "";
        //rangeDetails.type = "";
        rangeDetails.file = "";
        rangeDetails.line = -1;
        rangeDetails.column = 0;
        rangeDetails.isBindingLoop = false;
    }

    function selectNextByHash(hash) {
        var eventId = qmlProfilerModelProxy.getEventIdForHash(hash);
        if (eventId !== -1) {
            selectNextById(eventId);
        }
    }

    function selectNextById(eventId)
    {
        // this is a slot responding to events from the other pane
        // which tracks only events from the basic model
        if (!lockItemSelection) {
            lockItemSelection = true;
            var modelIndex = qmlProfilerModelProxy.basicModelIndex();
            var itemIndex = view.nextItemFromId( modelIndex, eventId );
            // select an item, lock to it, and recenter if necessary
            if (view.selectedItem != itemIndex || view.selectedModel != modelIndex) {
                view.selectedModel = modelIndex;
                view.selectedItem = itemIndex;
                if (itemIndex !== -1) {
                    view.selectionLocked = true;
                    recenterOnItem(modelIndex, itemIndex);
                }
            }
            lockItemSelection = false;
        }
    }

    // ***** slots
    onSelectionRangeModeChanged: {
        selectionRangeControl.enabled = selectionRangeMode;
        selectionRange.reset(selectionRangeMode);
    }

    onSelectionLockedChanged: {
        updateLockButton();
    }

    onSelectedItemChanged: {
        if (selectedItem != -1 && !lockItemSelection) {
            lockItemSelection = true;
            // update in other views
            var eventLocation = qmlProfilerModelProxy.getEventLocation(view.selectedModel, view.selectedItem);
            gotoSourceLocation(eventLocation.file, eventLocation.line, eventLocation.column);
            lockItemSelection = false;
        }
    }

    onRecordingEnabledChanged: {
        if (recordingEnabled) {
            recordingStartDate = new Date();
            elapsedTime = 0;
        } else {
            elapsedTime = (new Date() - recordingStartDate)/1000.0;
        }
    }

    Flickable {
        id: vertflick
        flickableDirection: Flickable.VerticalFlick
        anchors.fill: parent
        clip: true
        contentHeight: labels.height
        boundsBehavior: Flickable.StopAtBounds

        // ScrollView will try to deinteractivate it. We don't want that
        // as the horizontal flickable is interactive, too. We do occasionally
        // switch to non-interactive ourselves, though.
        property bool stayInteractive: true
        onInteractiveChanged: interactive = stayInteractive
        onStayInteractiveChanged: interactive = stayInteractive

        // ***** child items
        TimeMarks {
            id: backgroundMarks
            y: vertflick.contentY
            height: vertflick.height
            width: root.width - labels.width
            anchors.left: labels.right
        }

        Flickable {
            function setContentWidth() {
                var duration = Math.abs(zoomControl.endTime() - zoomControl.startTime());
                if (duration > 0)
                    contentWidth = qmlProfilerModelProxy.traceDuration() * width / duration;
            }

            id: flick
            anchors.top: parent.top
            anchors.topMargin: labels.y
            anchors.right: parent.right
            anchors.left: labels.right
            contentWidth: 0
            height: labels.height + labelsTail.height
            flickableDirection: Flickable.HorizontalFlick
            boundsBehavior: Flickable.StopAtBounds

            onContentXChanged: view.updateZoomControl()
            onWidthChanged: setContentWidth()

            clip:true

            SelectionRange {
                id: selectionRange
                visible: root.selectionRangeMode
                height: parent.height
                z: 2
            }

            TimelineRenderer {
                id: view

                profilerModelProxy: qmlProfilerModelProxy

                x: flick.contentX
                y: vertflick.contentY
                width: flick.width
                height: vertflick.height

                onEndTimeChanged: requestPaint()
                onYChanged: requestPaint()
                onHeightChanged: requestPaint()

                function updateZoomControl() {
                    var newStartTime = Math.round(flick.contentX * (endTime - startTime) / flick.width) +
                            qmlProfilerModelProxy.traceStartTime();
                    if (Math.abs(newStartTime - startTime) > 1) {
                        var newEndTime = Math.round((flick.contentX + flick.width) *
                                                    (endTime - startTime) /
                                                    flick.width) +
                                                    qmlProfilerModelProxy.traceStartTime();
                        zoomControl.setRange(newStartTime, newEndTime);
                    }
                }

                function updateFlickRange(start, end) {
                    if (start !== startTime || end !== endTime) {
                        startTime = start;
                        endTime = end;
                        var newStartX = (startTime - qmlProfilerModelProxy.traceStartTime()) *
                                flick.width / (endTime-startTime);
                        if (isFinite(newStartX) && Math.abs(newStartX - flick.contentX) >= 1)
                            flick.contentX = newStartX;
                    }
                }

                onSelectedItemChanged: {
                    if (selectedItem !== -1) {
                        // display details
                        rangeDetails.showInfo(qmlProfilerModelProxy.getEventDetails(selectedModel, selectedItem));
                        rangeDetails.setLocation(qmlProfilerModelProxy.getEventLocation(selectedModel, selectedItem));

                        // center view (horizontally)
                        var windowLength = view.endTime - view.startTime;
                        var eventStartTime = qmlProfilerModelProxy.getStartTime(selectedModel, selectedItem);
                        var eventEndTime = eventStartTime +
                                qmlProfilerModelProxy.getDuration(selectedModel, selectedItem);

                        if (eventEndTime < view.startTime || eventStartTime > view.endTime) {
                            var center = (eventStartTime + eventEndTime)/2;
                            var from = Math.min(qmlProfilerModelProxy.traceEndTime()-windowLength,
                                                Math.max(0, Math.floor(center - windowLength/2)));

                            zoomControl.setRange(from, from + windowLength);

                        }
                    } else {
                        root.hideRangeDetails();
                    }
                }

                onItemPressed: {
                    var location = qmlProfilerModelProxy.getEventLocation(modelIndex, pressedItem);
                    if (location.hasOwnProperty("file")) // not empty
                        root.gotoSourceLocation(location.file, location.line, location.column);
                }

             // hack to pass mouse events to the other mousearea if enabled
                startDragArea: selectionRange.ready ? selectionRange.getLeft() : -flick.contentX
                endDragArea: selectionRange.ready ? selectionRange.getRight() : -flick.contentX-1
            }
            MouseArea {
                id: selectionRangeControl
                enabled: false
                width: flick.width
                height: flick.height
                x: flick.contentX
                hoverEnabled: enabled
                z: 2

                onReleased:  {
                    selectionRange.releasedOnCreation();
                }
                onPressed:  {
                    selectionRange.pressedOnCreation();
                }
                onCanceled: {
                    selectionRange.releasedOnCreation();
                }
                onPositionChanged: {
                    selectionRange.movedOnCreation();
                }
            }
        }

        Rectangle {
            id: labels
            width: 150
            color: "#dcdcdc"
            height: col.height

            property int rowCount: qmlProfilerModelProxy.categories();

            Column {
                id: col
                Repeater {
                    model: labels.rowCount
                    delegate: CategoryLabel { }
                }
            }
        }

        Rectangle {
            id: labelsTail
            anchors.top: labels.bottom
            height: Math.max(0, vertflick.height - labels.height)
            width: labels.width
            color: labels.color
        }

        // Gradient borders
        Item {
            anchors.left: labels.right
            anchors.top: labels.top
            anchors.bottom: labelsTail.bottom
            width: 6
            Rectangle {
                x: parent.width
                transformOrigin: Item.TopLeft
                rotation: 90
                width: parent.height
                height: parent.width
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#00000000"; }
                    GradientStop { position: 1.0; color: "#86000000"; }
                }
            }
        }
    }

    ScrollView {
        contentItem: vertflick
        anchors.fill: parent
    }

    SelectionRangeDetails {
        id: selectionRangeDetails
        visible: root.selectionRangeMode
        startTime: selectionRange.startTimeString
        duration: selectionRange.durationString
        endTime: selectionRange.endTimeString
        showDuration: selectionRange.getWidth() > 1
    }

    RangeDetails {
        id: rangeDetails
    }

    Rectangle {
        objectName: "zoomSliderToolBar"
        color: "#9b9b9b"
        enabled: false
        visible: false
        width: labels.width
        height: 24
        x: 0
        y: 0

        function updateZoomLevel() {
            zoomSlider.externalUpdate = true;
            zoomSlider.value = Math.pow((view.endTime - view.startTime) / qmlProfilerModelProxy.traceDuration(), 1 / zoomSlider.exponent) * zoomSlider.maximumValue;
        }


        Slider {
            id: zoomSlider
            anchors.fill: parent
            minimumValue: 1
            maximumValue: 10000
            stepSize: 100

            property int exponent: 3
            property bool externalUpdate: false
            property int minWindowLength: 1e5 // 0.1 ms

            onValueChanged: {
                if (externalUpdate || qmlProfilerModelProxy.traceEndTime() <= qmlProfilerModelProxy.traceStartTime()) {
                    // Zoom range is independently updated. We shouldn't mess
                    // with it here as otherwise we might introduce rounding
                    // or arithmetic errors.
                    externalUpdate = false;
                    return;
                }

                var windowLength = Math.max(
                            Math.pow(value / maximumValue, exponent) * qmlProfilerModelProxy.traceDuration(),
                            minWindowLength);

                var fixedPoint = (view.startTime + view.endTime) / 2;
                if (view.selectedItem !== -1) {
                    // center on selected item if it's inside the current screen
                    var newFixedPoint = qmlProfilerModelProxy.getStartTime(view.selectedModel, view.selectedItem);
                    if (newFixedPoint >= view.startTime && newFixedPoint < view.endTime)
                        fixedPoint = newFixedPoint;
                }

                var startTime = Math.max(qmlProfilerModelProxy.traceStartTime(), fixedPoint - windowLength / 2)
                zoomControl.setRange(startTime, startTime + windowLength);
            }
        }
    }

    Item {
        anchors.right: root.right
        width: 6
        anchors.top: root.top
        anchors.bottom: root.bottom
        Rectangle {
            x: parent.width
            transformOrigin: Item.TopLeft
            rotation: 90
            width: parent.height
            height: parent.width
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#86000000"; }
                GradientStop { position: 1.0; color: "#00000000"; }
            }
        }
    }

    Rectangle {
        y: root.height - height
        height: 6
        width: root.width
        x: 0
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#00000000"; }
            GradientStop { position: 1.0; color: "#86000000"; }
        }
    }
}
