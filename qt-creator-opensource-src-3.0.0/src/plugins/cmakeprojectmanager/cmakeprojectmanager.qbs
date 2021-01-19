import qbs.base 1.0

import QtcPlugin

QtcPlugin {
    name: "CMakeProjectManager"

    Depends { name: "Qt.widgets" }
    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "CPlusPlus" }
    Depends { name: "Locator" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "QtSupport" }

    pluginRecommends: [
        "Designer"
    ]

    files: [
        "CMakeProjectManager.mimetypes.xml",
        "cmakebuildconfiguration.cpp",
        "cmakebuildconfiguration.h",
        "cmakebuildinfo.h",
        "cmakeeditor.cpp",
        "cmakeeditor.h",
        "cmakeeditorfactory.cpp",
        "cmakeeditorfactory.h",
        "cmakefilecompletionassist.cpp",
        "cmakefilecompletionassist.h",
        "cmakehighlighter.cpp",
        "cmakehighlighter.h",
        "cmakehighlighterfactory.cpp",
        "cmakehighlighterfactory.h",
        "cmakelocatorfilter.cpp",
        "cmakelocatorfilter.h",
        "cmakeopenprojectwizard.cpp",
        "cmakeopenprojectwizard.h",
        "cmakeparser.cpp",
        "cmakeparser.h",
        "cmakeproject.cpp",
        "cmakeproject.h",
        "cmakeproject.qrc",
        "cmakeprojectconstants.h",
        "cmakeprojectmanager.cpp",
        "cmakeprojectmanager.h",
        "cmakeprojectnodes.cpp",
        "cmakeprojectnodes.h",
        "cmakeprojectplugin.cpp",
        "cmakeprojectplugin.h",
        "cmakerunconfiguration.cpp",
        "cmakerunconfiguration.h",
        "cmakevalidator.cpp",
        "cmakevalidator.h",
        "makestep.cpp",
        "makestep.h",
    ]
}
