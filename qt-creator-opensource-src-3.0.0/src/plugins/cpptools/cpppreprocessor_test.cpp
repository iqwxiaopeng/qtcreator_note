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

#include "cppmodelmanager.h"
#include "cpppreprocessertesthelper.h"
#include "cpppreprocessor.h"

#include <cplusplus/CppDocument.h>
#include <utils/fileutils.h>

#include <QFile>
#include <QFileInfo>
#include <QtTest>

using namespace CPlusPlus;
using namespace CppTools;
using namespace CppTools::Internal;

typedef Document::Include Include;

class SourcePreprocessor
{
public:
    SourcePreprocessor()
        : m_cmm(CppModelManager::instance())
    {
        cleanUp();
    }

    Document::Ptr run(const QByteArray &source)
    {
        const QString fileName = TestIncludePaths::directoryOfTestFile()
                + QLatin1String("/file.cpp");
        if (QFileInfo(fileName).exists())
            return Document::Ptr(); // Test file was not removed.

        Utils::FileSaver srcSaver(fileName);
        srcSaver.write(source);
        srcSaver.finalize();

        CppPreprocessor pp((QPointer<CppModelManager>(m_cmm)));
        pp.setIncludePaths(QStringList(TestIncludePaths::directoryOfTestFile()));
        pp.run(fileName);

        Document::Ptr document = m_cmm->snapshot().document(fileName);
        QFile(fileName).remove();
        return document;
    }

    ~SourcePreprocessor()
    {
        cleanUp();
    }

private:
    void cleanUp()
    {
        m_cmm->GC();
        QVERIFY(m_cmm->snapshot().isEmpty());
    }

private:
    CppModelManager *m_cmm;
};

void CppToolsPlugin::test_cpppreprocessor_includes()
{
    QByteArray source =
        "#include \"header.h\"\n"
        "#include \"notresolvable.h\"\n"
        "\n"
        ;

    SourcePreprocessor processor;
    Document::Ptr document = processor.run(source);
    QVERIFY(document);

    const QList<Document::Include> resolvedIncludes = document->resolvedIncludes();
    QVERIFY(resolvedIncludes.size() == 1);
    QVERIFY(resolvedIncludes.at(0).type() == Client::IncludeLocal);
    QCOMPARE(resolvedIncludes.at(0).unresolvedFileName(), QLatin1String("header.h"));
    const QString expectedResolvedFileName
        = TestIncludePaths::directoryOfTestFile() + QLatin1String("/header.h");
    QCOMPARE(resolvedIncludes.at(0).resolvedFileName(), expectedResolvedFileName);

    const QList<Document::Include> unresolvedIncludes = document->unresolvedIncludes();
    QVERIFY(unresolvedIncludes.size() == 1);
    QVERIFY(unresolvedIncludes.at(0).type() == Client::IncludeLocal);
    QCOMPARE(unresolvedIncludes.at(0).unresolvedFileName(), QLatin1String("notresolvable.h"));
    QVERIFY(unresolvedIncludes.at(0).resolvedFileName().isEmpty());
}

