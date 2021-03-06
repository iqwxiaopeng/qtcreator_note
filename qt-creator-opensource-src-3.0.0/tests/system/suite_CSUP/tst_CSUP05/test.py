#############################################################################
##
## Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
## Contact: http://www.qt-project.org/legal
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and Digia.  For licensing terms and
## conditions see http://qt.digia.com/licensing.  For further information
## use the contact form at http://qt.digia.com/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU Lesser General Public License version 2.1 requirements
## will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Digia gives you certain additional
## rights.  These rights are described in the Digia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

source("../../shared/suites_qtta.py")
source("../../shared/qtcreator.py")

# entry of test
def main():
    # prepare example project
    sourceExample = os.path.abspath(sdkPath + "/Examples/4.7/declarative/animation/basics/property-animation")
    proFile = "propertyanimation.pro"
    if not neededFilePresent(os.path.join(sourceExample, proFile)):
        return
    # copy example project to temp directory
    templateDir = prepareTemplate(sourceExample)
    examplePath = os.path.join(templateDir, proFile)
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    # open example project
    openQmakeProject(examplePath)
    # wait for parsing to complete
    progressBarWait(30000)
    # open .cpp file in editor
    if not openDocument("propertyanimation.Sources.main\\.cpp"):
        test.fatal("Could not open main.cpp")
        invokeMenuItem("File", "Exit")
        return
    test.verify(checkIfObjectExists(":Qt Creator_CppEditor::Internal::CPPEditorWidget"),
                "Verifying if: .cpp file is opened in Edit mode.")
    # select some word for example "viewer" and press Ctrl+F.
    editorWidget = findObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
    if not placeCursorToLine(editorWidget, "QmlApplicationViewer viewer;"):
        invokeMenuItem("File", "Exit")
        return
    type(editorWidget, "<Left>")
    markText(editorWidget, "Left", 6)
    type(editorWidget, "<Ctrl+F>")
    # verify if find toolbar exists and if search text contains selected word
    test.verify(checkIfObjectExists(":*Qt Creator.Find_Find::Internal::FindToolBar"),
                "Verifying if: Find/Replace pane is displayed at the bottom of the view.")
    test.compare(waitForObject(":*Qt Creator.findEdit_Utils::FilterLineEdit").displayText, "viewer",
                 "Verifying if: Find line edit contains 'viewer' text.")
    # insert some word to "Replace with:" field and select "Replace All".
    replaceEditorContent(waitForObject(":Qt Creator.replaceEdit_Utils::FilterLineEdit"), "find")
    oldCodeText = str(editorWidget.plainText)
    clickButton(waitForObject(":Qt Creator.Replace All_QToolButton"))
    mouseClick(waitForObject(":Qt Creator.replaceEdit_Utils::FilterLineEdit"), 5, 5, 0, Qt.LeftButton)
    newCodeText = str(editorWidget.plainText)
    test.compare(newCodeText, oldCodeText.replace("viewer", "find").replace("Viewer", "find"),
                 "Verifying if: Found text is replaced with new word properly.")
    # select some other word in .cpp file and select "Edit" -> "Find/Replace".
    clickButton(waitForObject(":Qt Creator.CloseFind_QToolButton"))
    placeCursorToLine(editorWidget, "find.setOrientation(QmlApplicationfind::ScreenOrientationAuto);")
    for i in range(25):
        type(editorWidget, "<Left>")
    markText(editorWidget, "Left", 18)
    invokeMenuItem("Edit", "Find/Replace", "Find/Replace")
    replaceEditorContent(waitForObject(":Qt Creator.replaceEdit_Utils::FilterLineEdit"), "QmlApplicationViewer")
    oldCodeText = str(editorWidget.plainText)
    clickButton(waitForObject(":Qt Creator.Replace_QToolButton"))
    newCodeText = str(editorWidget.plainText)
    # "::" is used to replace only one occurrence by python
    test.compare(newCodeText, oldCodeText.replace("QmlApplicationfind::", "QmlApplicationViewer::"),
                 "Verifying if: Only selected word is replaced, the rest of found words are not replaced.")
    # close Find/Replace tab.
    clickButton(waitForObject(":Qt Creator.CloseFind_QToolButton"))
    test.verify(checkIfObjectExists(":*Qt Creator.Find_Find::Internal::FindToolBar", False),
                "Verifying if: Find/Replace tab is closed.")
    # exit qt creator
    invokeMenuItem("File", "Save All")
    invokeMenuItem("File", "Exit")
