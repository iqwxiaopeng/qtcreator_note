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

source("../../shared/qtcreator.py")

cloneUrl = "https://codereview.qt-project.org/p/qt-labs/jom"
cloneDir = "myCloneOfJom"

def verifyCloneLog(targetDir, canceled):
    # Expect fails because of QTCREATORBUG-10531
    cloneLog = waitForObject(":Git Repository Clone.logPlainTextEdit_QPlainTextEdit")
    finish = findObject(":Git Repository Clone.Finish_QPushButton")
    waitFor("finish.enabled", 30000)
    test.xverify(("Executing in " + targetDir + ":" in str(cloneLog.plainText)),
                 "Searching for target directory in clone log")
    test.xverify((" ".join(["clone", cloneUrl, cloneDir]) in str(cloneLog.plainText)),
                 "Searching for git parameters in clone log")
    if canceled:
        test.xverify("Stopping..." in str(cloneLog.plainText),
                     "Searching for 'Stopping...' in clone log")
        result = "The process terminated in an abnormal way."
        summary = "Failed."
    else:
        test.verify(not "Stopping..." in str(cloneLog.plainText),
                    "Searching for 'Stopping...' in clone log")
        test.verify(("'" + cloneDir + "'..." in str(cloneLog.plainText)),
                    "Searching for clone directory in clone log")
        result = "The process terminated with exit code 0."
        summary = "Succeeded."
    test.xverify((result in str(cloneLog.plainText)),
                 "Searching for result (%s) in clone log:\n%s"
                % (result, str(cloneLog.plainText)))
    test.compare(waitForObject(":Git Repository Clone.Result._QLabel").text, summary)

def verifyFiles(targetDir):
    for file in [".gitignore", "CMakeLists.txt", "jom.pro",
                 os.path.join("bin", "ibjom.cmd"),
                 os.path.join("src", "app", "main.cpp")]:
        test.verify(os.path.exists(os.path.join(targetDir, cloneDir, file)),
                    "Verify the existence of %s" % file)

def main():
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    for button in ["Cancel immediately",
                   ":Git Repository Clone.Cancel_QPushButton",
                   ":Git Repository Clone.Finish_QPushButton"]:
        __createProjectOrFileSelectType__("  Import Project", "Git Repository Clone")
        replaceEditorContent(waitForObject(":Repository.repositoryLineEdit_QLineEdit"),
                             cloneUrl)
        targetDir = tempDir()
        replaceEditorContent(waitForObject(":Working Copy_Utils::BaseValidatingLineEdit"),
                             targetDir)
        cloneDirEdit = waitForObject(":Working Copy.checkoutDirectoryLineEdit_QLineEdit")
        test.compare(cloneDirEdit.text, "p-qt-labs-jom")
        replaceEditorContent(cloneDirEdit, cloneDir)
        clickButton(waitForObject(":Next_QPushButton"))
        test.compare(waitForObject(":Git Repository Clone.Result._QLabel").text,
                     "Cloning started...")
        if button == "Cancel immediately":
            clickButton(":Git Repository Clone.Cancel_QPushButton")
            verifyCloneLog(targetDir, True)
            clickButton(":Git Repository Clone.Cancel_QPushButton")
        else:
            verifyCloneLog(targetDir, False)
            verifyFiles(targetDir)
            try:
                clickButton(waitForObject(button))
            except:
                cloneLog = waitForObject(":Git Repository Clone.logPlainTextEdit_QPlainTextEdit")
                test.fatal("Cloning failed",
                           str(cloneLog.plainText))
                clickButton(":Git Repository Clone.Cancel_QPushButton")
                continue
            if button == ":Git Repository Clone.Finish_QPushButton":
                try:
                    # CMake wizard shown
                    clickButton(waitForObject(":CMake Wizard.Cancel_QPushButton", 5000))
                    clickButton(waitForObject(":Cannot Open Project.OK_QPushButton"))
                    test.passes("The checked out project was being opened with CMake.")
                except:
                    try:
                        # QMake Project mode shown
                        clickButton(waitForObject(":*Qt Creator.Cancel_QPushButton", 5000))
                        test.passes("The checked out project was being opened with QMake.")
                    except:
                        clickButton(waitForObject(":Cannot Open Project.Show Details..._QPushButton"))
                        test.fail("The checked out project was not being opened.",
                                  waitForObject(":Cannot Open Project_QTextEdit").plainText)
                        clickButton(waitForObject(":Cannot Open Project.OK_QPushButton"))
    invokeMenuItem("File", "Exit")
