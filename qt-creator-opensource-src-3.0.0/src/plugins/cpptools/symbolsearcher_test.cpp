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

#include "cpptoolsplugin.h"

#include "builtinindexingsupport.h"
#include "cppmodelmanager.h"
#include "searchsymbols.h"

#include <coreplugin/testdatadir.h>
#include <utils/runextensions.h>

#include <QtTest>

using namespace CppTools;
using namespace CppTools::Internal;

namespace {

class MyTestDataDir : public Core::Internal::Tests::TestDataDir
{
public:
    MyTestDataDir(const QString &testDataDirectory)
        : TestDataDir(QLatin1String(SRCDIR "/../../../tests/cppsymbolsearcher/")
                      + testDataDirectory)
    {}
};

class ResultData
{
public:
    typedef QList<ResultData> ResultDataList;

    ResultData() {}
    ResultData(const QString &symbolName, const QString &scope)
        : m_symbolName(symbolName), m_scope(scope) {}

    bool operator==(const ResultData &other) const
    {
        return m_symbolName == other.m_symbolName && m_scope == other.m_scope;
    }

    static ResultDataList fromSearchResultList(const QList<Find::SearchResultItem> &entries)
    {
        ResultDataList result;
        foreach (const Find::SearchResultItem &entry, entries)
            result << ResultData(entry.text, entry.path.join(QLatin1String("::")));
        return result;
    }

    /// For debugging and creating reference data
    static void printFilterEntries(const ResultDataList &entries)
    {
        QTextStream out(stdout);
        foreach (const ResultData entry, entries) {
            out << "<< ResultData(_(\"" << entry.m_symbolName << "\"), _(\""
                << entry.m_scope <<  "\"))" << endl;
        }
    }

    QString m_symbolName;
    QString m_scope;
};

typedef ResultData::ResultDataList ResultDataList;

class SymbolSearcherTest
{
public:
    /// Takes no ownership of indexingSupportToUse
    SymbolSearcherTest(const QString &testFile, CppIndexingSupport *indexingSupportToUse)
        : m_modelManager(CppModelManager::instance())
        , m_indexingSupportToUse(indexingSupportToUse)
        , m_testFile(testFile)
    {
        QVERIFY(m_indexingSupportToUse);
        QVERIFY(m_modelManager->snapshot().isEmpty());
        m_modelManager->updateSourceFiles(QStringList() << m_testFile).waitForFinished();
        QVERIFY(m_modelManager->snapshot().contains(m_testFile));

        m_indexingSupportToRestore = m_modelManager->indexingSupport();
        m_modelManager->setIndexingSupport(m_indexingSupportToUse);
        QCoreApplication::processEvents();
    }

    ResultDataList run(const SymbolSearcher::Parameters &searchParameters) const
    {
        CppIndexingSupport *indexingSupport = m_modelManager->indexingSupport();
        SymbolSearcher *symbolSearcher
            = indexingSupport->createSymbolSearcher(searchParameters, QSet<QString>() << m_testFile);
        QFuture<Find::SearchResultItem> search
            = QtConcurrent::run(&SymbolSearcher::runSearch, symbolSearcher);
        search.waitForFinished();
        ResultDataList results = ResultData::fromSearchResultList(search.results());
        return results;
    }

    ~SymbolSearcherTest()
    {
        m_modelManager->setIndexingSupport(m_indexingSupportToRestore);
        m_modelManager->GC();
        QVERIFY(m_modelManager->snapshot().isEmpty());
    }

private:
    CppModelManager *m_modelManager;
    CppIndexingSupport *m_indexingSupportToRestore;
    CppIndexingSupport *m_indexingSupportToUse;
    const QString m_testFile;
};

inline QString _(const QByteArray &ba) { return QString::fromLatin1(ba, ba.size()); }

} // anonymous namespace

Q_DECLARE_METATYPE(ResultData)
Q_DECLARE_METATYPE(ResultDataList)

QT_BEGIN_NAMESPACE
namespace QTest {

template<> char *toString(const ResultData &data)
{
    QByteArray ba = "\"" + data.m_symbolName.toUtf8() + "\", \"" + data.m_scope.toUtf8() + "\"";
    return qstrdup(ba.data());
}

} // namespace QTest
QT_END_NAMESPACE

void CppToolsPlugin::test_builtinsymbolsearcher()
{
    QFETCH(QString, testFile);
    QFETCH(SymbolSearcher::Parameters, searchParameters);
    QFETCH(ResultDataList, expectedResults);

    QScopedPointer<CppIndexingSupport> builtinIndexingSupport(new BuiltinIndexingSupport);

    SymbolSearcherTest test(testFile, builtinIndexingSupport.data());
    const ResultDataList results = test.run(searchParameters);
    QCOMPARE(results, expectedResults);
}

void CppToolsPlugin::test_builtinsymbolsearcher_data()
{
    QTest::addColumn<QString>("testFile");
    QTest::addColumn<SymbolSearcher::Parameters>("searchParameters");
    QTest::addColumn<ResultDataList>("expectedResults");

    MyTestDataDir testDirectory(QLatin1String("testdata_basic"));
    const QString testFile = testDirectory.file(QLatin1String("file1.cpp"));

    QScopedPointer<CppIndexingSupport> builtinIndexingSupport(new BuiltinIndexingSupport);

    SymbolSearcher::Parameters searchParameters;

    // Check All Symbol Types
    searchParameters = SymbolSearcher::Parameters();
    searchParameters.text = _("");
    searchParameters.flags = 0;
    searchParameters.types = SearchSymbols::AllTypes;
    searchParameters.scope = SymbolSearcher::SearchGlobal;
    QTest::newRow("BuiltinSymbolSearcher::AllTypes")
        << testFile
        << searchParameters
        << (ResultDataList()
            << ResultData(_("int myVariable"), _(""))
            << ResultData(_("myFunction(bool, int)"), _(""))
            << ResultData(_("MyEnum"), _(""))
            << ResultData(_("int V1"), _("MyEnum"))
            << ResultData(_("int V2"), _("MyEnum"))
            << ResultData(_("MyClass"), _(""))
            << ResultData(_("MyClass()"), _("MyClass"))
            << ResultData(_("functionDeclaredOnly()"), _("MyClass"))
            << ResultData(_("functionDefinedInClass(bool, int)"), _("MyClass"))
            << ResultData(_("functionDefinedOutSideClass(char)"), _("MyClass"))
            << ResultData(_("functionDefinedOutSideClass(char)"), _("MyClass"))
            << ResultData(_("int myVariable"), _("MyNamespace"))
            << ResultData(_("myFunction(bool, int)"), _("MyNamespace"))
            << ResultData(_("MyEnum"), _("MyNamespace"))
            << ResultData(_("int V1"), _("MyNamespace::MyEnum"))
            << ResultData(_("int V2"), _("MyNamespace::MyEnum"))
            << ResultData(_("MyClass"), _("MyNamespace"))
            << ResultData(_("MyClass()"), _("MyNamespace::MyClass"))
            << ResultData(_("functionDeclaredOnly()"), _("MyNamespace::MyClass"))
            << ResultData(_("functionDefinedInClass(bool, int)"), _("MyNamespace::MyClass"))
            << ResultData(_("functionDefinedOutSideClass(char)"), _("MyNamespace::MyClass"))
            << ResultData(_("functionDefinedOutSideClassAndNamespace(float)"), _("MyNamespace::MyClass"))
            << ResultData(_("functionDefinedOutSideClass(char)"), _("MyNamespace::MyClass"))
            << ResultData(_("functionDefinedOutSideClassAndNamespace(float)"), _("MyNamespace::MyClass"))
            << ResultData(_("int myVariable"), _("<anonymous namespace>"))
            << ResultData(_("myFunction(bool, int)"), _("<anonymous namespace>"))
            << ResultData(_("MyEnum"), _("<anonymous namespace>"))
            << ResultData(_("int V1"), _("<anonymous namespace>::MyEnum"))
            << ResultData(_("int V2"), _("<anonymous namespace>::MyEnum"))
            << ResultData(_("MyClass"), _("<anonymous namespace>"))
            << ResultData(_("MyClass()"), _("<anonymous namespace>::MyClass"))
            << ResultData(_("functionDeclaredOnly()"), _("<anonymous namespace>::MyClass"))
            << ResultData(_("functionDefinedInClass(bool, int)"), _("<anonymous namespace>::MyClass"))
            << ResultData(_("functionDefinedOutSideClass(char)"), _("<anonymous namespace>::MyClass"))
            << ResultData(_("functionDefinedOutSideClass(char)"), _("<anonymous namespace>::MyClass"))
            << ResultData(_("main()"), _(""))

        );

    // Check Classes
    searchParameters = SymbolSearcher::Parameters();
    searchParameters.text = _("myclass");
    searchParameters.flags = 0;
    searchParameters.types = SymbolSearcher::Classes;
    searchParameters.scope = SymbolSearcher::SearchGlobal;
    QTest::newRow("BuiltinSymbolSearcher::Classes")
        << testFile
        << searchParameters
        << (ResultDataList()
            << ResultData(_("MyClass"), _(""))
            << ResultData(_("MyClass"), _("MyNamespace"))
            << ResultData(_("MyClass"), _("<anonymous namespace>"))
        );

    // Check Functions
    searchParameters = SymbolSearcher::Parameters();
    searchParameters.text = _("fun");
    searchParameters.flags = 0;
    searchParameters.types = SymbolSearcher::Functions;
    searchParameters.scope = SymbolSearcher::SearchGlobal;
    QTest::newRow("BuiltinSymbolSearcher::Functions")
        << testFile
        << searchParameters
        << (ResultDataList()
            << ResultData(_("myFunction(bool, int)"), _(""))
            << ResultData(_("functionDefinedInClass(bool, int)"), _("MyClass"))
            << ResultData(_("functionDefinedOutSideClass(char)"), _("MyClass"))
            << ResultData(_("myFunction(bool, int)"), _("MyNamespace"))
            << ResultData(_("functionDefinedInClass(bool, int)"), _("MyNamespace::MyClass"))
            << ResultData(_("functionDefinedOutSideClass(char)"), _("MyNamespace::MyClass"))
            << ResultData(_("functionDefinedOutSideClassAndNamespace(float)"), _("MyNamespace::MyClass"))
            << ResultData(_("myFunction(bool, int)"), _("<anonymous namespace>"))
            << ResultData(_("functionDefinedInClass(bool, int)"), _("<anonymous namespace>::MyClass"))
            << ResultData(_("functionDefinedOutSideClass(char)"), _("<anonymous namespace>::MyClass"))
        );

    // Check Enums
    searchParameters = SymbolSearcher::Parameters();
    searchParameters.text = _("enum");
    searchParameters.flags = 0;
    searchParameters.types = SymbolSearcher::Enums;
    searchParameters.scope = SymbolSearcher::SearchGlobal;
    QTest::newRow("BuiltinSymbolSearcher::Enums")
        << testFile
        << searchParameters
        << (ResultDataList()
            << ResultData(_("MyEnum"), _(""))
            << ResultData(_("MyEnum"), _("MyNamespace"))
            << ResultData(_("MyEnum"), _("<anonymous namespace>"))
        );

    // Check Declarations
    searchParameters = SymbolSearcher::Parameters();
    searchParameters.text = _("myvar");
    searchParameters.flags = 0;
    searchParameters.types = SymbolSearcher::Declarations;
    searchParameters.scope = SymbolSearcher::SearchGlobal;
    QTest::newRow("BuiltinSymbolSearcher::Declarations")
        << testFile
        << searchParameters
        << (ResultDataList()
            << ResultData(_("int myVariable"), _(""))
            << ResultData(_("int myVariable"), _("MyNamespace"))
            << ResultData(_("int myVariable"), _("<anonymous namespace>"))
        );
}
