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

#include "cppcodemodelsettingspage.h"
#include "cpptoolsconstants.h"
#include "ui_cppcodemodelsettingspage.h"

#include <coreplugin/icore.h>

#include <QTextStream>

using namespace CppTools;
using namespace CppTools::Internal;

CppCodeModelSettingsWidget::CppCodeModelSettingsWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::CppCodeModelSettingsPage)
{
    m_ui->setupUi(this);

    m_ui->theGroupBox->setVisible(false);
}

CppCodeModelSettingsWidget::~CppCodeModelSettingsWidget()
{
    delete m_ui;
}

void CppCodeModelSettingsWidget::setSettings(const QSharedPointer<CppCodeModelSettings> &s)
{
    m_settings = s;

    applyToWidget(m_ui->cChooser, QLatin1String(Constants::C_SOURCE_MIMETYPE));
    applyToWidget(m_ui->cppChooser, QLatin1String(Constants::CPP_SOURCE_MIMETYPE));
    applyToWidget(m_ui->objcChooser, QLatin1String(Constants::OBJECTIVE_C_SOURCE_MIMETYPE));
    applyToWidget(m_ui->objcppChooser, QLatin1String(Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE));

    m_ui->ignorePCHCheckBox->setChecked(s->pchUsage() == CppCodeModelSettings::PchUse_None);
}

void CppCodeModelSettingsWidget::applyToWidget(QComboBox *chooser, const QString &mimeType) const
{
    chooser->clear();

    QStringList names = m_settings->availableModelManagerSupportersByName().keys();
    qSort(names);
    foreach (const QString &name, names) {
        const QString &id = m_settings->availableModelManagerSupportersByName()[name];
        chooser->addItem(name, id);
        if (id == m_settings->modelManagerSupportId(mimeType))
            chooser->setCurrentIndex(chooser->count() - 1);
    }
    chooser->setEnabled(names.size() > 1);
}

void CppCodeModelSettingsWidget::applyToSettings() const
{
    bool changed = false;
    changed |= applyToSettings(m_ui->cChooser, QLatin1String(Constants::C_SOURCE_MIMETYPE));
    changed |= applyToSettings(m_ui->cppChooser, QLatin1String(Constants::CPP_SOURCE_MIMETYPE));
    changed |= applyToSettings(m_ui->objcChooser,
                               QLatin1String(Constants::OBJECTIVE_C_SOURCE_MIMETYPE));
    changed |= applyToSettings(m_ui->objcppChooser,
                               QLatin1String(Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE));

    if (m_ui->ignorePCHCheckBox->isChecked() !=
            (m_settings->pchUsage() == CppCodeModelSettings::PchUse_None)) {
        m_settings->setPCHUsage(
                   m_ui->ignorePCHCheckBox->isChecked() ? CppCodeModelSettings::PchUse_None
                                                        : CppCodeModelSettings::PchUse_BuildSystem);
        changed = true;
    }

    if (changed)
        m_settings->toSettings(Core::ICore::settings());
}

QString CppCodeModelSettingsWidget::searchKeywords() const
{
    QString rc;
    QTextStream ts(&rc);
    ts << m_ui->theGroupBox->title()
       << ' ' << m_ui->cLabel->text()
       << ' ' << m_ui->cppLabel->text()
       << ' ' << m_ui->objcLabel->text()
       << ' ' << m_ui->objcppLabel->text()
       << ' ' << m_ui->anotherGroupBox->title()
       << ' ' << m_ui->ignorePCHCheckBox->text();
    foreach (const QString &mmsNames, m_settings->availableModelManagerSupportersByName().keys())
          ts << ' ' << mmsNames;
    rc.remove(QLatin1Char('&'));
    return rc;
}

bool CppCodeModelSettingsWidget::applyToSettings(QComboBox *chooser, const QString &mimeType) const
{
    QString newId = chooser->itemData(chooser->currentIndex()).toString();
    QString &currentId = m_settings->modelManagerSupportId(mimeType);
    if (newId == currentId)
        return false;

    currentId = newId;
    return true;
}

CppCodeModelSettingsPage::CppCodeModelSettingsPage(QSharedPointer<CppCodeModelSettings> &settings,
                                                   QObject *parent)
    : Core::IOptionsPage(parent)
    , m_settings(settings)
{
    setId(Constants::CPP_CODE_MODEL_SETTINGS_ID);
    setDisplayName(QCoreApplication::translate("CppTools",Constants::CPP_CODE_MODEL_SETTINGS_NAME));
    setCategory(Constants::CPP_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("CppTools",Constants::CPP_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Constants::SETTINGS_CATEGORY_CPP_ICON));
}

QWidget *CppCodeModelSettingsPage::createPage(QWidget *parent)
{
    m_widget = new CppCodeModelSettingsWidget(parent);
    m_widget->setSettings(m_settings);
    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_widget->searchKeywords();
    return m_widget;
}

void CppCodeModelSettingsPage::apply()
{
    if (m_widget)
        m_widget->applyToSettings();
}

bool CppCodeModelSettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}
