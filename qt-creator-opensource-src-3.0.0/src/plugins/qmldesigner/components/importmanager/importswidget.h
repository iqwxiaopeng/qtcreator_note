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

#ifndef QMLDESIGNER_IMPORTSWIDGET_H
#define QMLDESIGNER_IMPORTSWIDGET_H

#include <import.h>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace QmlDesigner {

class ImportLabel;

class ImportsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ImportsWidget(QWidget *parent = 0);

    void setImports(const QList<Import> &imports);
    void removeImports();

    void setPossibleImports(const QList<Import> &possibleImports);
    void removePossibleImports();

    void setUsedImports(const QList<Import> &possibleImports);
    void removeUsedImports();

signals:
    void removeImport(const Import &import);
    void addImport(const Import &import);

protected:
    void updateLayout();

private slots:
    void addSelectedImport(int addImportComboBoxIndex);

private:
    QList<ImportLabel*> m_importLabels;
    QComboBox *m_addImportComboBox;
};

} // namespace QmlDesigner

#endif // QMLDESIGNER_IMPORTSWIDGET_H
