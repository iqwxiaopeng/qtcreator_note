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
variablemanager类管理qc范围内的变量，用户可以输入许多字符串设置。使用字符串时，变量被实际值替换，类似于外壳程序如何扩展环境变量。
第1节变量
变量名基本上可以是任何没有美元符号和大括号的字符串，不过建议只使用7位ASCII，而不使用特殊字符和空格。
如果有多个变量包含同一对象的不同方面，则通常会给它们指定相同的前缀，后跟冒号和描述方面的后缀。
例如currentDocument:filePath和currentDocument:selection。当请求变量管理器替换字符串中的变量时，它会查找用和括起来的变量名，如currentDocument:filePath。
注意变量的名称存储为qbytearray。它们通常是7位的。在不可能的情况下，假定使用UTF-8编码

第1节提供变量值



插件可以通过register variable（）将变量与描述一起注册，然后需要连接到variableupdateRequested（）信号，以便在请求时实际为变量提供值。典型的设置是
\列表1

\ 在extensionSystem:：iplugin:：initialize（）中注册变量：

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
\ 在请求时设置变量值：

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
\列表1结束

如果存在变量无效的条件，则应在updateVariable（）中调用variableManager:：remove（kmyVariable）。
对于引用文件的变量，应使用方便函数variablemanager:：registerfilevariables（）、variablemanager:：filevariablevalue（）和variablemanager:：isfilevariable（）。
函数采用变量前缀，如myfilevariable，并自动处理标准化的后缀，如：filepath、：path和：filebasename，从而产生组合变量，如myfilevariable:filepath。

\第1节提供和扩展参数化字符串
尽管可以向变量管理器索要代码中某个变量的值，但首选的用例是为用户提供参数化字符串的可能性，例如设置。

（如果你曾经考虑过做前者，那就三思而后行。只需直接询问提供变量值的插件，而不需要进行字符串转换，以及通过变量管理器（将进行大规模的轮询）就可以更高效。
更具体一点，使用“提供变量值”部分中的示例：不调用variableManager:：value（“myvariable”），直接使用myplugin:：variablevalue（）提问更有效。）

第二部分用户界面:
如果您想要的参数字符串是由用户设置的，通过QLINED或QTEXID源类别，您应该添加一个变量选择到您的UI，其中允许通过列表浏览到字符串中的变量。See core:Variable chooser for more details.

第2节扩展字符串
在字符串中展开变量值是由“宏展开器”完成的。utils:：abstractMacromExpander是这些变量的基类，变量管理器提供了一个实现，该实现通过
variableManager:：macorexpander（）。有几种不同的方法来展开字符串，涵盖不同的用例，这里按相关性排序。

扩展字符串有几种不同的方法，包括不同的用例，

此处按相关性排序：

列表

\ li使用variableManager:：expandedString（）。这是扩展变量值字符串的最舒适的方法，但也是最不灵活的方法。如果这对你足够，就用它。

\ li使用utils:：expandmacros（）函数。这需要一个字符串和一个宏扩展器（您将使用变量管理器提供的字符串和宏扩展器）。与variableManager：：expandedString（）基本相同，但也有一个变量以内联方式执行替换，而不是返回新字符串。

\ li使用utils:：qtcprocess:：expandmacros（）。这将扩展字符串，同时符合运行它的平台的引用规则。如果字符串将作为命令行参数字符串传递给外部命令，请将此函数与变量管理器的宏扩展器一起使用。

\ li编写自己的宏扩展器来嵌套变量管理器的宏扩展器。然后做上面的一个。这允许您扩展不来自变量管理器的其他“本地”变量/宏。

\结束列表

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
