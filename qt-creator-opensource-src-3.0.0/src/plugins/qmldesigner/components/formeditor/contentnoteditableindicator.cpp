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

#include "contentnoteditableindicator.h"
#include "nodemetainfo.h"

namespace QmlDesigner {

ContentNotEditableIndicator::ContentNotEditableIndicator(LayerItem *layerItem)
    : m_layerItem(layerItem)
{

}

ContentNotEditableIndicator::ContentNotEditableIndicator()
{
}

ContentNotEditableIndicator::~ContentNotEditableIndicator()
{
    clear();
}

void ContentNotEditableIndicator::clear()
{
    foreach (const EntryPair &entryPair, m_entryList) {
        delete entryPair.second;
        entryPair.first->blurContent(false);
    }

    m_entryList.clear();
}

bool operator ==(const ContentNotEditableIndicator::EntryPair &firstPair, const ContentNotEditableIndicator::EntryPair &secondPair)
{
    return firstPair.first == secondPair.first;
}

void ContentNotEditableIndicator::setItems(const QList<FormEditorItem*> &itemList)
{
    removeEntriesWhichAreNotInTheList(itemList);
    addAddiationEntries(itemList);
}

void ContentNotEditableIndicator::addAddiationEntries(const QList<FormEditorItem *> &itemList)
{
    foreach (FormEditorItem *formEditorItem, itemList) {
        if (formEditorItem->qmlItemNode().modelNode().metaInfo().isSubclassOf("QtQuick.Loader", -1, -1)) {

            if (!m_entryList.contains(EntryPair(formEditorItem, 0))) {
                QGraphicsRectItem *indicatorShape = new QGraphicsRectItem(m_layerItem);
                QRectF boundingRectangleInSceneSpace = formEditorItem->qmlItemNode().instanceSceneTransform().mapRect(formEditorItem->qmlItemNode().instanceBoundingRect());
                indicatorShape->setRect(boundingRectangleInSceneSpace);
                static QBrush brush(QColor(0, 0, 0, 130), Qt::BDiagPattern);
                indicatorShape->setBrush(brush);

                formEditorItem->blurContent(true);

                m_entryList.append(EntryPair(formEditorItem, indicatorShape));
            }

        }
    }
}

void ContentNotEditableIndicator::removeEntriesWhichAreNotInTheList(const QList<FormEditorItem *> &itemList)
{
    QMutableListIterator<EntryPair> entryIterator(m_entryList);

    while (entryIterator.hasNext()) {
        EntryPair &entryPair = entryIterator.next();
        if (!itemList.contains(entryPair.first)) {
            delete entryPair.second;
            entryPair.first->blurContent(false);
            entryIterator.remove();
        }
    }
}

} // namespace QmlDesigner
