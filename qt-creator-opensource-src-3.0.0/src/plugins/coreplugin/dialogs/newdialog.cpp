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

//wizard封装
struct WizardContainer
{
    WizardContainer() : wizard(0), wizardOption(0) {}
    WizardContainer(Core::IWizard *w, int i): wizard(w), wizardOption(i) {}
    Core::IWizard *wizard;
    int wizardOption;
};

//从qt的model条目中取出wizard数据
inline Core::IWizard *wizardOfItem(const QStandardItem *item = 0)
{
    if (!item)
        return 0;
    return item->data(Qt::UserRole).value<WizardContainer>().wizard;
}

//过滤器model代理 (代理共享其代理model的数据,一个数据model可以被多个代理共享)
class PlatformFilterProxyModel : public QSortFilterProxyModel
{
//    Q_OBJECT
public:
    PlatformFilterProxyModel(QObject *parent = 0): QSortFilterProxyModel(parent) {}

	//平台选项设置
    void setPlatform(const QString& platform)
    {
        m_platform = platform;

		//因为要启用自己的过滤方法filterAcceptsRow，因此必须先禁止系统当前使用的方法
        invalidateFilter();
    }

	//过滤方法实现 model的行，model的父索引
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
    {
        if (!sourceParent.isValid())
            return true;

        QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent); //取出数据在model中的索引
        Core::IWizard *wizard = wizardOfItem(qobject_cast<QStandardItemModel*>(sourceModel())->itemFromIndex(sourceIndex)); //根据索引取出数据

		//根据数据所属平台 返回是否被过滤 
        if (wizard)
            return m_platform.isEmpty() || wizard->isAvailable(m_platform); 

        return true;
    }
private:
    QString m_platform;
};

//抽象代理model (model 与其代理model是存在一个映射关系的，比如model的第3条可能对应代理model的第1条，只是index的变化但是数据指针不变)
class TwoLevelProxyModel : public QAbstractProxyModel
{
//    Q_OBJECT
public:
    TwoLevelProxyModel(QObject *parent = 0): QAbstractProxyModel(parent) {}

	/****************以下几个方法参数均是针对代理类的行列，父亲，索引************/
    QModelIndex index(int row, int column, const QModelIndex &parent) const
    {
        QModelIndex ourModelIndex = sourceModel()->index(row, column, mapToSource(parent));
        return createIndex(row, column, ourModelIndex.internalPointer()); //一个index包含 {行，列，以及数据指针}，并且index必须相对于其父亲而言
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


	//把被代理model的index转换到代理类的index {eg: 有10个学生，编号1-10，现在有个代理对象，课代表 1-4，分别指代学生 2，5， 6， 9号}
	//这个参数index是指课代表的1-4号，而返回的要是学生的编号
    QModelIndex	mapFromSource (const QModelIndex &index) const
    {
        if (!index.isValid())
            return QModelIndex();
        return createIndex(index.row(), index.column(), index.internalPointer());
    }

	//把代理对象的index转换到model的index
    QModelIndex	mapToSource (const QModelIndex &index) const
    {
        if (!index.isValid())
            return QModelIndex();
        return static_cast<TwoLevelProxyModel*>(sourceModel())->createIndex(index.row(), index.column(), index.internalPointer());
    }
};

#define ROW_HEIGHT 24

//QItemDelegate 主要是对 view功能的补充，这里是用于对treeview中的item进行特殊绘制
class FancyTopLevelDelegate : public QItemDelegate
{
public:
    FancyTopLevelDelegate(QObject *parent = 0)
        : QItemDelegate(parent) {}

	//绘制函数
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

	//尺寸函数
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

	//定义数据对象以及 选择代理 和 普通代理对象，分别代理model，也就是model是数据，而代理则以不同的方式挑选使用数据，数据就一份
    m_model = new QStandardItemModel(this);
    m_twoLevelProxyModel = new TwoLevelProxyModel(this);
    m_twoLevelProxyModel->setSourceModel(m_model);
    m_filterProxyModel = new PlatformFilterProxyModel(this);
    m_filterProxyModel->setSourceModel(m_model);

	//设置一级项目框使用的模型
    m_ui->templateCategoryView->setModel(m_twoLevelProxyModel);
    m_ui->templateCategoryView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_ui->templateCategoryView->setItemDelegate(new FancyTopLevelDelegate); //设置绘制

	//设置二级小项列表框的图标
    m_ui->templatesView->setIconSize(QSize(ICON_SIZE, ICON_SIZE));

	//一级点击，二级会列出一级包含的条目，而三级是二级的选中条目的说明
    connect(m_ui->templateCategoryView, SIGNAL(clicked(QModelIndex)), //一级变动
        this, SLOT(currentCategoryChanged(QModelIndex)));
    connect(m_ui->templatesView, SIGNAL(clicked(QModelIndex)), //二级变动 就影响三级说明
        this, SLOT(currentItemChanged(QModelIndex)));

    connect(m_ui->templateCategoryView->selectionModel(), //选中变更 一级变动
            SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(currentCategoryChanged(QModelIndex)));
    connect(m_ui->templatesView, //二级被双击 是要直接做出选择相当于OK键被点击
            SIGNAL(doubleClicked(QModelIndex)),
            this, SLOT(okButtonClicked()));

    connect(m_okButton, SIGNAL(clicked()), this, SLOT(okButtonClicked())); //OK点击
    connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject())); //取消点击

	//模版选择更改 一级，二级均会被影响
    connect(m_ui->comboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(setSelectedPlatform(QString)));
}

// Sort by category. id  定义iwizard对象的比较算法
bool wizardLessThan(const IWizard *w1, const IWizard *w2)
{
    if (const int cc = w1->category().compare(w2->category()))
        return cc < 0;
    return w1->id().compare(w2->id()) < 0;
}

//设置要显示的数据
void NewDialog::setWizards(QList<IWizard*> wizards)
{
    typedef QMap<QString, QStandardItem *> CategoryItemMap;

	//先排序
    qStableSort(wizards.begin(), wizards.end(), wizardLessThan);

    CategoryItemMap platformMap;

    m_model->clear();

	//把扔进来的所有 wizard对象安装类别组织成树，而parentItem就是根节点
    QStandardItem *parentItem = m_model->invisibleRootItem(); //得到所有节点的最终根节点

    QStandardItem *projectKindItem = new QStandardItem(tr("Projects")); //项目
    projectKindItem->setData(IWizard::ProjectWizard, Qt::UserRole);
    projectKindItem->setFlags(0); // disable item to prevent focus
    QStandardItem *filesClassesKindItem = new QStandardItem(tr("Files and Classes"));//文件和类
    filesClassesKindItem->setData(IWizard::FileWizard, Qt::UserRole);
    filesClassesKindItem->setFlags(0); // disable item to prevent focus

	//由此可见  [项目  文件和类]在一个model中
    parentItem->appendRow(projectKindItem);
    parentItem->appendRow(filesClassesKindItem);

    if (m_dummyIcon.isNull())
        m_dummyIcon = QIcon(QLatin1String(Core::Constants::ICON_NEWFILE));

	//获取所有引导支持的模版交集
    QStringList availablePlatforms = IWizard::allAvailablePlatforms();
    m_ui->comboBox->addItem(tr("All Templates"), QString()); //增加 所有模版 选项

	//其他支持的模版逐一添加
    foreach (const QString &platform, availablePlatforms) {
        const QString displayNameForPlatform = IWizard::displayNameForPlatform(platform);
        m_ui->comboBox->addItem(tr("%1 Templates").arg(displayNameForPlatform), platform);
    }

	//模版选项控件设默认显示
    if (!availablePlatforms.isEmpty())
        m_ui->comboBox->setCurrentIndex(1); //First Platform
    else
        m_ui->comboBox->setDisabled(true);

	//按类型对 wizard归类
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
		//wizard属于kindItem这一组，因此添加到kindItem分组下，就是作为kindItem的叶子
        addItem(kindItem, wizard);
    }
	//没数据的分组，把标题条也移走
    if (projectKindItem->columnCount() == 0)
        parentItem->removeRow(0);
}

//运行选择对话框 返回的条件是用户选择了某一个具体wizard项目
Core::IWizard *NewDialog::showDialog()
{
    static QString lastCategory;
    QModelIndex idx;

	//如果不是第一次使用，就按上次给默认索引
    if (!lastCategory.isEmpty())
        foreach (QStandardItem* item, m_categoryItems) {
            if (item->data(Qt::UserRole) == lastCategory)
                idx = m_twoLevelProxyModel->mapToSource(m_model->indexFromItem(item));
    }
	//初次使用，默认开头的为默认索引
    if (!idx.isValid())
        idx = m_twoLevelProxyModel->index(0,0, m_twoLevelProxyModel->index(0,0));

	//设置一级默认索引
    m_ui->templateCategoryView->setCurrentIndex(idx);

    // We need to set ensure that the category has default focus
    m_ui->templateCategoryView->setFocus(Qt::NoFocusReason);

    for (int row = 0; row < m_twoLevelProxyModel->rowCount(); ++row)
        m_ui->templateCategoryView->setExpanded(m_twoLevelProxyModel->index(row, 0), true);

    // Ensure that item description is visible on first show
    currentItemChanged(m_ui->templatesView->rootIndex().child(0,0)); //更新第二级

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
//模版选择 (参考IWizard图片对应函数名) [所有模版，桌面模版]
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

//往topLEvelCategoryItem对应的子类型中增加一个wizard
void NewDialog::addItem(QStandardItem *topLEvelCategoryItem, IWizard *wizard)
{
    const QString categoryName = wizard->category(); //拿引导名
    QStandardItem *categoryItem = 0;
    for (int i = 0; i < topLEvelCategoryItem->rowCount(); i++) {
        if (topLEvelCategoryItem->child(i, 0)->data(Qt::UserRole) == categoryName)
            categoryItem = topLEvelCategoryItem->child(i, 0);
    }
    if (!categoryItem) { //保证每个类别只有一次
        categoryItem = new QStandardItem();
        topLEvelCategoryItem->appendRow(categoryItem);
        m_categoryItems.append(categoryItem); //放所有的类别
        categoryItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        categoryItem->setText(QLatin1String("  ") + wizard->displayCategory()); //设置显示名
        categoryItem->setData(wizard->category(), Qt::UserRole);
    }

    QStandardItem *wizardItem = new QStandardItem(wizard->displayName()); //这块是具体到每一个wizard
    QIcon wizardIcon;

    // spacing hack. Add proper icons instead 没有提供图标 就用默认文件图片
    if (wizard->icon().isNull())
        wizardIcon = m_dummyIcon;
    else
        wizardIcon = wizard->icon();
    wizardItem->setIcon(wizardIcon);
    wizardItem->setData(QVariant::fromValue(WizardContainer(wizard, 0)), Qt::UserRole);
    wizardItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
    categoryItem->appendRow(wizardItem); //增加到topLEvelCategoryItem具体类别条目中

}

//一级改变大类改变
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

//二级改变 小类改变
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
