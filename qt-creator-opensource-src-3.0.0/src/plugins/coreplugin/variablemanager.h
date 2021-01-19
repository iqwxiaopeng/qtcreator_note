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

#ifndef VARIABLEMANAGER_H
#define VARIABLEMANAGER_H

#include "core_global.h"

#include <QObject>
#include <QString>

QT_FORWARD_DECLARE_CLASS(QFileInfo)

namespace Utils {
class AbstractMacroExpander;
}

namespace Core {

namespace Internal { class MainWindow; }
/*
variablemanager�����qc��Χ�ڵı������û�������������ַ������á�ʹ���ַ���ʱ��������ʵ��ֵ�滻����������ǳ��������չ����������
��1�ڱ���
�����������Ͽ������κ�û����Ԫ���źʹ����ŵ��ַ�������������ֻʹ��7λASCII������ʹ�������ַ��Ϳո�
����ж����������ͬһ����Ĳ�ͬ���棬��ͨ���������ָ����ͬ��ǰ׺�����ð�ź���������ĺ�׺��
����currentDocument:filePath��currentDocument:selection������������������滻�ַ����еı���ʱ����������ú��������ı���������currentDocument:filePath��
ע����������ƴ洢Ϊqbytearray������ͨ����7λ�ġ��ڲ����ܵ�����£��ٶ�ʹ��UTF-8����

��1���ṩ����ֵ



�������ͨ��register variable����������������һ��ע�ᣬȻ����Ҫ���ӵ�variableupdateRequested�����źţ��Ա�������ʱʵ��Ϊ�����ṩֵ�����͵�������
\�б�1

\ ��extensionSystem:��iplugin:��initialize������ע�������

\code
static const char kMyVariable[] = "MyVariable";

bool MyPlugin::initialize(const QStringList &arguments, QString *errorString)
{
[...]
VariableManager::registerVariable(kMyVariable, tr("The current value of whatever I want."));
connect(VariableManager::instance(), SIGNAL(variableUpdateRequested(QByteArray)),
this, SLOT(updateVariable(QByteArray)));
[...]
}
\endcode
\ ������ʱ���ñ���ֵ��

\code
void MyPlugin::updateVariable(const QByteArray &variable)
{
if (variable == kMyVariable) {
QString value;
// do whatever is necessary to retrieve the value
[...]
VariableManager::insert(variable, value);
}
}
\endcode
\�б�1����

������ڱ�����Ч����������Ӧ��updateVariable�����е���variableManager:��remove��kmyVariable����
���������ļ��ı�����Ӧʹ�÷��㺯��variablemanager:��registerfilevariables������variablemanager:��filevariablevalue������variablemanager:��isfilevariable������
�������ñ���ǰ׺����myfilevariable�����Զ������׼���ĺ�׺���磺filepath����path�ͣ�filebasename���Ӷ�������ϱ�������myfilevariable:filepath��

\��1���ṩ����չ�������ַ���
���ܿ����������������Ҫ������ĳ��������ֵ������ѡ��������Ϊ�û��ṩ�������ַ����Ŀ����ԣ��������á�

��������������ǹ���ǰ�ߣ��Ǿ���˼�����С�ֻ��ֱ��ѯ���ṩ����ֵ�Ĳ����������Ҫ�����ַ���ת�����Լ�ͨ�������������������д��ģ����ѯ���Ϳ��Ը���Ч��
������һ�㣬ʹ�á��ṩ����ֵ�������е�ʾ����������variableManager:��value����myvariable������ֱ��ʹ��myplugin:��variablevalue�������ʸ���Ч����

�ڶ������û�����:
�������Ҫ�Ĳ����ַ��������û����õģ�ͨ��QLINED��QTEXIDԴ�����Ӧ�����һ������ѡ������UI����������ͨ���б�������ַ����еı�����See core:Variable chooser for more details.

��2����չ�ַ���
���ַ�����չ������ֵ���ɡ���չ��������ɵġ�utils:��abstractMacromExpander����Щ�����Ļ��࣬�����������ṩ��һ��ʵ�֣���ʵ��ͨ��
variableManager:��macorexpander�������м��ֲ�ͬ�ķ�����չ���ַ��������ǲ�ͬ�����������ﰴ���������

��չ�ַ����м��ֲ�ͬ�ķ�����������ͬ��������

�˴������������

�б�

\ liʹ��variableManager:��expandedString������������չ����ֵ�ַ����������ʵķ�������Ҳ������ķ��������������㹻����������

\ liʹ��utils:��expandmacros��������������Ҫһ���ַ�����һ������չ��������ʹ�ñ����������ṩ���ַ����ͺ���չ��������variableManager����expandedString����������ͬ����Ҳ��һ��������������ʽִ���滻�������Ƿ������ַ�����

\ liʹ��utils:��qtcprocess:��expandmacros�������⽫��չ�ַ�����ͬʱ������������ƽ̨�����ù�������ַ�������Ϊ�����в����ַ������ݸ��ⲿ����뽫�˺���������������ĺ���չ��һ��ʹ�á�

\ li��д�Լ��ĺ���չ����Ƕ�ױ����������ĺ���չ����Ȼ���������һ��������������չ�����Ա��������������������ء�����/�ꡣ

\�����б�

*/
class CORE_EXPORT VariableManager : public QObject
{
    Q_OBJECT

public:
    static QObject *instance();

    static void insert(const QByteArray &variable, const QString &value);
    static bool remove(const QByteArray &variable);
    static QString value(const QByteArray &variable, bool *found = 0);

    static QString expandedString(const QString &stringWithVariables);
    static Utils::AbstractMacroExpander *macroExpander();

    static void registerVariable(const QByteArray &variable,
                          const QString &description);

    static void registerFileVariables(const QByteArray &prefix,
                              const QString &heading);
    static bool isFileVariable(const QByteArray &variable, const QByteArray &prefix);
    static QString fileVariableValue(const QByteArray &variable, const QByteArray &prefix,
                              const QString &fileName);
    static QString fileVariableValue(const QByteArray &variable, const QByteArray &prefix,
                              const QFileInfo &fileInfo);

    static QList<QByteArray> variables();
    static QString variableDescription(const QByteArray &variable);

signals:
    void variableUpdateRequested(const QByteArray &variable);

private:
    VariableManager();
    ~VariableManager();

    friend class Core::Internal::MainWindow;
};

} // namespace Core

#endif // VARIABLEMANAGER_H
