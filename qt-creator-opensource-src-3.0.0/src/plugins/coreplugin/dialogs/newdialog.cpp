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

#include "newdialog.h"
#include "ui_newdialog.h"

#include <coreplugin/coreconstants.h>

#include <QModelIndex>
#include <QAbstractProxyModel>
#include <QSortFilterProxyModel>
#include <QPushButton>
#include <QStandardItem>
#include <QItemDelegate>
#include <QPainter>
#include <QDebug>

Q_DECLARE_METATYPE(Core::IWizard*)


namespace {

const int ICON_SIZE = 22;

//wizard��װ
struct WizardContainer
{
    WizardContainer() : wizard(0), wizardOption(0) {}
    WizardContainer(Core::IWizard *w, int i): wizard(w), wizardOption(i) {}
    Core::IWizard *wizard;
    int wizardOption;
};

//��qt��model��Ŀ��ȡ��wizard����
inline Core::IWizard *wizardOfItem(const QStandardItem *item = 0)
{
    if (!item)
        return 0;
    return item->data(Qt::UserRole).value<WizardContainer>().wizard;
}

//������model���� (�����������model������,һ������model���Ա����������)
class PlatformFilterProxyModel : public QSortFilterProxyModel
{
//    Q_OBJECT
public:
    PlatformFilterProxyModel(QObject *parent = 0): QSortFilterProxyModel(parent) {}

	//ƽ̨ѡ������
    void setPlatform(const QString& platform)
    {
        m_platform = platform;

		//��ΪҪ�����Լ��Ĺ��˷���filterAcceptsRow����˱����Ƚ�ֹϵͳ��ǰʹ�õķ���
        invalidateFilter();
    }

	//���˷���ʵ�� model���У�model�ĸ�����
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
    {
        if (!sourceParent.isValid())
            return true;

        QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent); //ȡ��������model�е�����
        Core::IWizard *wizard = wizardOfItem(qobject_cast<QStandardItemModel*>(sourceModel())->itemFromIndex(sourceIndex)); //��������ȡ������

		//������������ƽ̨ �����Ƿ񱻹��� 
        if (wizard)
            return m_platform.isEmpty() || wizard->isAvailable(m_platform); 

        return true;
    }
private:
    QString m_platform;
};

//�������model (model �������model�Ǵ���һ��ӳ���ϵ�ģ�����model�ĵ�3�����ܶ�Ӧ����model�ĵ�1����ֻ��index�ı仯��������ָ�벻��)
class TwoLevelProxyModel : public QAbstractProxyModel
{
//    Q_OBJECT
public:
    TwoLevelProxyModel(QObject *parent = 0): QAbstractProxyModel(parent) {}

	/****************���¼�����������������Դ���������У����ף�����************/
    QModelIndex index(int row, int column, const QModelIndex &parent) const
    {
        QModelIndex ourModelIndex = sourceModel()->index(row, column, mapToSource(parent));
        return createIndex(row, column, ourModelIndex.internalPointer()); //һ��index���� {�У��У��Լ�����ָ��}������index����������丸�׶���
    }

    QModelIndex parent(const QModelIndex &index) const
    {
        return mapFromSource(mapToSource(index).parent());
    }

    int rowCount(const QModelIndex &index) const
    {
        if (index.isValid() && index.parent().isValid() && !index.parent().parent().isValid())
            return 0;
        else
            return sourceModel()->rowCount(mapToSource(index));
    }

    int columnCount(const QModelIndex &index) const
    {
        return sourceModel()->columnCount(mapToSource(index));
    }


	//�ѱ�����model��indexת�����������index {eg: ��10��ѧ�������1-10�������и�������󣬿δ��� 1-4���ֱ�ָ��ѧ�� 2��5�� 6�� 9��}
	//�������index��ָ�δ����1-4�ţ������ص�Ҫ��ѧ���ı��
    QModelIndex	mapFromSource (const QModelIndex &index) const
    {
        if (!index.isValid())
            return QModelIndex();
        return createIndex(index.row(), index.column(), index.internalPointer());
    }

	//�Ѵ�������indexת����model��index
    QModelIndex	mapToSource (const QModelIndex &index) const
    {
        if (!index.isValid())
            return QModelIndex();
        return static_cast<TwoLevelProxyModel*>(sourceModel())->createIndex(index.row(), index.column(), index.internalPointer());
    }
};

#define ROW_HEIGHT 24

//QItemDelegate ��Ҫ�Ƕ� view���ܵĲ��䣬���������ڶ�treeview�е�item�����������
class FancyTopLevelDelegate : public QItemDelegate
{
public:
    FancyTopLevelDelegate(QObject *parent = 0)
        : QItemDelegate(parent) {}

	//���ƺ���
    void drawDisplay(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QString &text) const
    {
        QStyleOptionViewItem newoption = option;
        if (!(option.state & QStyle::State_Enabled)) {
            QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
            gradient.setColorAt(0, option.palette.window().color().lighter(106));
            gradient.setColorAt(1, option.palette.window().color().darker(106));
            painter->fillRect(rect, gradient);
            painter->setPen(option.palette.window().color().darker(130));
            if (rect.top())
                painter->drawLine(rect.topRight(), rect.topLeft());
            painter->drawLine(rect.bottomRight(), rect.bottomLeft());

            // Fake enabled state
            newoption.state |= newoption.state | QStyle::State_Enabled;
        }

        QItemDelegate::drawDisplay(painter, newoption, rect, text);
    }

	//�ߴ纯��
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QSize size = QItemDelegate::sizeHint(option, index);


        size = size.expandedTo(QSize(0, ROW_HEIGHT));

        return size;
    }
};

}

Q_DECLARE_METATYPE(WizardContainer)

using namespace Core;
using namespace Core::Internal;

NewDialog::NewDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Core::Internal::Ui::NewDialog),
    m_okButton(0)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->setupUi(this);
    QPalette p = m_ui->frame->palette();
    p.setColor(QPalette::Window, p.color(QPalette::Base));
    m_ui->frame->setPalette(p);
    m_okButton = m_ui->buttonBox->button(QDialogButtonBox::Ok);
    m_okButton->setDefault(true);
    m_okButton->setText(tr("&Choose..."));

	//�������ݶ����Լ� ѡ����� �� ��ͨ������󣬷ֱ����model��Ҳ����model�����ݣ����������Բ�ͬ�ķ�ʽ��ѡʹ�����ݣ����ݾ�һ��
    m_model = new QStandardItemModel(this);
    m_twoLevelProxyModel = new TwoLevelProxyModel(this);
    m_twoLevelProxyModel->setSourceModel(m_model);
    m_filterProxyModel = new PlatformFilterProxyModel(this);
    m_filterProxyModel->setSourceModel(m_model);

	//����һ����Ŀ��ʹ�õ�ģ��
    m_ui->templateCategoryView->setModel(m_twoLevelProxyModel);
    m_ui->templateCategoryView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_ui->templateCategoryView->setItemDelegate(new FancyTopLevelDelegate); //���û���

	//���ö���С���б���ͼ��
    m_ui->templatesView->setIconSize(QSize(ICON_SIZE, ICON_SIZE));

	//һ��������������г�һ����������Ŀ���������Ƕ�����ѡ����Ŀ��˵��
    connect(m_ui->templateCategoryView, SIGNAL(clicked(QModelIndex)), //һ���䶯
        this, SLOT(currentCategoryChanged(QModelIndex)));
    connect(m_ui->templatesView, SIGNAL(clicked(QModelIndex)), //�����䶯 ��Ӱ������˵��
        this, SLOT(currentItemChanged(QModelIndex)));

    connect(m_ui->templateCategoryView->selectionModel(), //ѡ�б�� һ���䶯
            SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(currentCategoryChanged(QModelIndex)));
    connect(m_ui->templatesView, //������˫�� ��Ҫֱ������ѡ���൱��OK�������
            SIGNAL(doubleClicked(QModelIndex)),
            this, SLOT(okButtonClicked()));

    connect(m_okButton, SIGNAL(clicked()), this, SLOT(okButtonClicked())); //OK���
    connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject())); //ȡ�����

	//ģ��ѡ����� һ�����������ᱻӰ��
    connect(m_ui->comboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(setSelectedPlatform(QString)));
}

// Sort by category. id  ����iwizard����ıȽ��㷨
bool wizardLessThan(const IWizard *w1, const IWizard *w2)
{
    if (const int cc = w1->category().compare(w2->category()))
        return cc < 0;
    return w1->id().compare(w2->id()) < 0;
}

//����Ҫ��ʾ������
void NewDialog::setWizards(QList<IWizard*> wizards)
{
    typedef QMap<QString, QStandardItem *> CategoryItemMap;

	//������
    qStableSort(wizards.begin(), wizards.end(), wizardLessThan);

    CategoryItemMap platformMap;

    m_model->clear();

	//���ӽ��������� wizard����װ�����֯��������parentItem���Ǹ��ڵ�
    QStandardItem *parentItem = m_model->invisibleRootItem(); //�õ����нڵ�����ո��ڵ�

    QStandardItem *projectKindItem = new QStandardItem(tr("Projects")); //��Ŀ
    projectKindItem->setData(IWizard::ProjectWizard, Qt::UserRole);
    projectKindItem->setFlags(0); // disable item to prevent focus
    QStandardItem *filesClassesKindItem = new QStandardItem(tr("Files and Classes"));//�ļ�����
    filesClassesKindItem->setData(IWizard::FileWizard, Qt::UserRole);
    filesClassesKindItem->setFlags(0); // disable item to prevent focus

	//�ɴ˿ɼ�  [��Ŀ  �ļ�����]��һ��model��
    parentItem->appendRow(projectKindItem);
    parentItem->appendRow(filesClassesKindItem);

    if (m_dummyIcon.isNull())
        m_dummyIcon = QIcon(QLatin1String(Core::Constants::ICON_NEWFILE));

	//��ȡ��������֧�ֵ�ģ�潻��
    QStringList availablePlatforms = IWizard::allAvailablePlatforms();
    m_ui->comboBox->addItem(tr("All Templates"), QString()); //���� ����ģ�� ѡ��

	//����֧�ֵ�ģ����һ���
    foreach (const QString &platform, availablePlatforms) {
        const QString displayNameForPlatform = IWizard::displayNameForPlatform(platform);
        m_ui->comboBox->addItem(tr("%1 Templates").arg(displayNameForPlatform), platform);
    }

	//ģ��ѡ��ؼ���Ĭ����ʾ
    if (!availablePlatforms.isEmpty())
        m_ui->comboBox->setCurrentIndex(1); //First Platform
    else
        m_ui->comboBox->setDisabled(true);

	//�����Ͷ� wizard����
    foreach (IWizard *wizard, wizards) {
        QStandardItem *kindItem;
        switch (wizard->kind()) {
        case IWizard::ProjectWizard:
            kindItem = projectKindItem;
            break;
        case IWizard::ClassWizard:
        case IWizard::FileWizard:
        default:
            kindItem = filesClassesKindItem;
            break;
        }
		//wizard����kindItem��һ�飬�����ӵ�kindItem�����£�������ΪkindItem��Ҷ��
        addItem(kindItem, wizard);
    }
	//û���ݵķ��飬�ѱ�����Ҳ����
    if (projectKindItem->columnCount() == 0)
        parentItem->removeRow(0);
}

//����ѡ��Ի��� ���ص��������û�ѡ����ĳһ������wizard��Ŀ
Core::IWizard *NewDialog::showDialog()
{
    static QString lastCategory;
    QModelIndex idx;

	//������ǵ�һ��ʹ�ã��Ͱ��ϴθ�Ĭ������
    if (!lastCategory.isEmpty())
        foreach (QStandardItem* item, m_categoryItems) {
            if (item->data(Qt::UserRole) == lastCategory)
                idx = m_twoLevelProxyModel->mapToSource(m_model->indexFromItem(item));
    }
	//����ʹ�ã�Ĭ�Ͽ�ͷ��ΪĬ������
    if (!idx.isValid())
        idx = m_twoLevelProxyModel->index(0,0, m_twoLevelProxyModel->index(0,0));

	//����һ��Ĭ������
    m_ui->templateCategoryView->setCurrentIndex(idx);

    // We need to set ensure that the category has default focus
    m_ui->templateCategoryView->setFocus(Qt::NoFocusReason);

    for (int row = 0; row < m_twoLevelProxyModel->rowCount(); ++row)
        m_ui->templateCategoryView->setExpanded(m_twoLevelProxyModel->index(row, 0), true);

    // Ensure that item description is visible on first show
    currentItemChanged(m_ui->templatesView->rootIndex().child(0,0)); //���µڶ���

    updateOkButton();

    const int retVal = exec();

    idx = m_ui->templateCategoryView->currentIndex();
    QStandardItem *currentItem = m_model->itemFromIndex(m_twoLevelProxyModel->mapToSource(idx));
    if (currentItem)
        lastCategory = currentItem->data(Qt::UserRole).toString();

    if (retVal != Accepted)
        return 0;

    return currentWizard();
}
//ģ��ѡ�� (�ο�IWizardͼƬ��Ӧ������) [����ģ�棬����ģ��]
QString NewDialog::selectedPlatform() const
{
    int index = m_ui->comboBox->currentIndex();

    return m_ui->comboBox->itemData(index).toString();
}

int NewDialog::selectedWizardOption() const
{
    QStandardItem *item = m_model->itemFromIndex(m_ui->templatesView->currentIndex());
    return item->data(Qt::UserRole).value<WizardContainer>().wizardOption;
}

NewDialog::~NewDialog()
{
    delete m_ui;
}

IWizard *NewDialog::currentWizard() const
{
    QModelIndex index = m_filterProxyModel->mapToSource(m_ui->templatesView->currentIndex());
    return wizardOfItem(m_model->itemFromIndex(index));
}

//��topLEvelCategoryItem��Ӧ��������������һ��wizard
void NewDialog::addItem(QStandardItem *topLEvelCategoryItem, IWizard *wizard)
{
    const QString categoryName = wizard->category(); //��������
    QStandardItem *categoryItem = 0;
    for (int i = 0; i < topLEvelCategoryItem->rowCount(); i++) {
        if (topLEvelCategoryItem->child(i, 0)->data(Qt::UserRole) == categoryName)
            categoryItem = topLEvelCategoryItem->child(i, 0);
    }
    if (!categoryItem) { //��֤ÿ�����ֻ��һ��
        categoryItem = new QStandardItem();
        topLEvelCategoryItem->appendRow(categoryItem);
        m_categoryItems.append(categoryItem); //�����е����
        categoryItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        categoryItem->setText(QLatin1String("  ") + wizard->displayCategory()); //������ʾ��
        categoryItem->setData(wizard->category(), Qt::UserRole);
    }

    QStandardItem *wizardItem = new QStandardItem(wizard->displayName()); //����Ǿ��嵽ÿһ��wizard
    QIcon wizardIcon;

    // spacing hack. Add proper icons instead û���ṩͼ�� ����Ĭ���ļ�ͼƬ
    if (wizard->icon().isNull())
        wizardIcon = m_dummyIcon;
    else
        wizardIcon = wizard->icon();
    wizardItem->setIcon(wizardIcon);
    wizardItem->setData(QVariant::fromValue(WizardContainer(wizard, 0)), Qt::UserRole);
    wizardItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
    categoryItem->appendRow(wizardItem); //���ӵ�topLEvelCategoryItem���������Ŀ��

}

//һ���ı����ı�
void NewDialog::currentCategoryChanged(const QModelIndex &index)
{
    if (index.parent() != m_model->invisibleRootItem()->index()) {
        m_ui->templatesView->setModel(m_filterProxyModel);
        QModelIndex sourceIndex = m_twoLevelProxyModel->mapToSource(index);
        sourceIndex = m_filterProxyModel->mapFromSource(sourceIndex);
        m_ui->templatesView->setRootIndex(sourceIndex);
        // Focus the first item by default
        m_ui->templatesView->setCurrentIndex(m_ui->templatesView->rootIndex().child(0,0));

        connect(m_ui->templatesView->selectionModel(),
                SIGNAL(currentChanged(QModelIndex,QModelIndex)),
                this, SLOT(currentItemChanged(QModelIndex)));
    }
}

//�����ı� С��ı�
void NewDialog::currentItemChanged(const QModelIndex &index)
{
    QModelIndex sourceIndex = m_filterProxyModel->mapToSource(index);
    QStandardItem* cat = (m_model->itemFromIndex(sourceIndex));
    if (const IWizard *wizard = wizardOfItem(cat)) {
        QString desciption = wizard->description();
        QStringList displayNamesForSupporttedPlatforms;
        foreach (const QString &platform, wizard->supportedPlatforms())
            displayNamesForSupporttedPlatforms << IWizard::displayNameForPlatform(platform);
        if (!Qt::mightBeRichText(desciption))
            desciption.replace(QLatin1Char('\n'), QLatin1String("<br>"));
        desciption += QLatin1String("<br><br><b>");
        if (wizard->flags().testFlag(IWizard::PlatformIndependent))
            desciption += tr("Platform independent") + QLatin1String("</b>");
        else
            desciption += tr("Supported Platforms")
                    + QLatin1String("</b>: <tt>")
                    + displayNamesForSupporttedPlatforms.join(QLatin1String(" "))
                    + QLatin1String("</tt>");

        m_ui->templateDescription->setHtml(desciption);

        if (!wizard->descriptionImage().isEmpty()) {
            m_ui->imageLabel->setVisible(true);
            m_ui->imageLabel->setPixmap(wizard->descriptionImage());
        } else {
            m_ui->imageLabel->setVisible(false);
        }

    } else {
        m_ui->templateDescription->setText(QString());
    }
    updateOkButton();
}

void NewDialog::okButtonClicked()
{
    if (m_ui->templatesView->currentIndex().isValid())
        accept();
}

void NewDialog::updateOkButton()
{
    m_okButton->setEnabled(currentWizard() != 0);
}

void NewDialog::setSelectedPlatform(const QString & /*platform*/)
{
    //The static cast allows us to keep PlatformFilterProxyModel anonymous
    static_cast<PlatformFilterProxyModel*>(m_filterProxyModel)->setPlatform(selectedPlatform());
}
