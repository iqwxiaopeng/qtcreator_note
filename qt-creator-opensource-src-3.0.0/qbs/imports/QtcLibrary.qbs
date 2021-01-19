import qbs.base 1.0
import QtcFunctions

DynamicLibrary {
    Depends { name: "cpp" }
    Depends {
        condition: project.testsEnabled
        name: "Qt.test"
    }

    targetName: QtcFunctions.qtLibraryName(qbs, name)
    destinationDirectory: project.ide_library_path

    cpp.defines: project.generalDefines
    cpp.linkerFlags: {
        if (qbs.buildVariant == "release" && (qbs.toolchain.contains("gcc") || qbs.toolchain.contains("mingw")))
            return ["-Wl,-s"]
        else if (qbs.buildVariant == "debug" && qbs.toolchain.contains("msvc"))
            return ["/INCREMENTAL:NO"] // Speed up startup time when debugging with cdb
    }
    cpp.installNamePrefix: "@rpath/PlugIns/"
    cpp.rpaths: qbs.targetOS.contains("osx")
            ? ["@loader_path/..", "@executable_path/.."]
            : ["$ORIGIN", "$ORIGIN/.."]
    property string libIncludeBase: ".." // #include <lib/header.h>
    cpp.includePaths: [libIncludeBase]

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [libIncludeBase]
    }

    Group {
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: project.ide_library_path
    }
}
