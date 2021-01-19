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

#include "fancytabwidget.h"
#include <utils/hostosinfo.h>
#include <utils/stylehelper.h>
#include <utils/styledbar.h>

#include <QDebug>

#include <QColorDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QStyleFactory>
#include <QPainter>
#include <QStackedLayout>
#include <QStatusBar>
#include <QToolTip>

using namespace Core;
using namespace Internal;

const int FancyTabBar::m_rounding = 22;
const int FancyTabBar::m_textPadding = 4;

//fadein 效果动画
void FancyTab::fadeIn()
{
    animator.stop();
    animator.setDuration(80); //设置动画运行时长
    animator.setEndValue(40); //设置终止时长
    animator.start(); //启动动画
}
//fadeOut 效果动画
void FancyTab::fadeOut()
{
    animator.stop();
    animator.setDuration(160);
    animator.setEndValue(0);
    animator.start();
}

void FancyTab::setFader(float value)
{
    m_fader = value;
    tabbar->update();
}

FancyTabBar::FancyTabBar(QWidget *parent)
    : QWidget(parent)
{
    m_hoverIndex = -1;
    m_currentIndex = -1;
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setStyle(QStyleFactory::create(QLatin1String("windows")));
    setMinimumWidth(qMax(2 * m_rounding, 40));
    setAttribute(Qt::WA_Hover, true);
    setFocusPolicy(Qt::NoFocus);
    setMouseTracking(true); // Needed for hover events
    m_triggerTimer.setSingleShot(true);

    // We use a zerotimer to keep the sidebar responsive
    connect(&m_triggerTimer, SIGNAL(timeout()), this, SLOT(emitCurrentIndex()));
}

FancyTabBar::~FancyTabBar()
{
    delete style();
}

//计算左侧菜单栏中一个菜单项的尺寸
QSize FancyTabBar::tabSizeHint(bool minimum) const
{
    QFont boldFont(font());
    boldFont.setPointSizeF(Utils::StyleHelper::sidebarFontSize());
    boldFont.setBold(true);

    QFontMetrics fm(boldFont); //用来获取字体宽度
    int spacing = 8;
    int width = 60 + spacing + 2; //默认宽度 70

	//计算所有子菜单项上字体标签的最大宽度
    int maxLabelwidth = 0;
    for (int tab=0 ; tab<count() ;++tab) {
        int width = fm.width(tabText(tab));
        if (width > maxLabelwidth)
            maxLabelwidth = width;
    }

    int iconHeight = minimum ? 0 : 32;//图标默认高度 32

	//宽度为 max(70, 最大菜单字体宽+2) 高度为(40 + 字体高度)
    return QSize(qMax(width, maxLabelwidth + 4), iconHeight + spacing + fm.height());
}

void FancyTabBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);

    for (int i = 0; i < count(); ++i)
        if (i != currentIndex())
            paintTab(&p, i);

    // paint active tab last, since it overlaps the neighbors
    if (currentIndex() != -1)
        paintTab(&p, currentIndex());
}

// Handle hover events for mouse fade ins
//鼠标移动事件重写 更新鼠标所在菜单区域与索引,菜单播放动画
void FancyTabBar::mouseMoveEvent(QMouseEvent *e)
{
    int newHover = -1;
    for (int i = 0; i < count(); ++i) {
        QRect area = tabRect(i); //拿到每项菜单的区域
        if (area.contains(e->pos())) {  //判断是否鼠标所在区域
            newHover = i;  //鼠标在第i个菜单上
            break;
        }
    }
    if (newHover == m_hoverIndex)
        return;

	//菜单索引合理
    if (validIndex(m_hoverIndex))
        m_tabs[m_hoverIndex]->fadeOut(); //鼠标移动到的菜单项 播放 fadeOut动画

    m_hoverIndex = newHover; //更新当前鼠标所在菜单的索引记录

    if (validIndex(m_hoverIndex)) {
        m_tabs[m_hoverIndex]->fadeIn(); //播放fadeIn
        m_hoverRect = tabRect(m_hoverIndex); //更新鼠标所在控件区域值
    }
}

//过滤 实现自定义QEvent::ToolTip事件
bool FancyTabBar::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) { //一个 tooltip 请求（QHelpEvent）
        if (validIndex(m_hoverIndex)) {
            QString tt = tabToolTip(m_hoverIndex);
            if (!tt.isEmpty()) {
                QToolTip::showText(static_cast<QHelpEvent*>(event)->globalPos(), tt, this);
                return true;
            }
        }
    }
    return QWidget::event(event);
}

// Resets hover animation on mouse enter
//鼠标刚移入 清空当前鼠标位置记录
void FancyTabBar::enterEvent(QEvent *e)
{
    Q_UNUSED(e)
    m_hoverRect = QRect();
    m_hoverIndex = -1;
}

// Resets hover animation on mouse enter
//鼠标移出 清空当前鼠标位置记录 左侧菜单控件全部播放fadeOut
void FancyTabBar::leaveEvent(QEvent *e)
{
    Q_UNUSED(e)
    m_hoverIndex = -1;
    m_hoverRect = QRect();
    for (int i = 0 ; i < m_tabs.count() ; ++i) {
        m_tabs[i]->fadeOut();
    }
}

//返回控件的尺寸
QSize FancyTabBar::sizeHint() const
{
    QSize sh = tabSizeHint(); //计算单个控件的尺寸
    return QSize(sh.width(), sh.height() * m_tabs.count()); //根据控件数动态调整
}
//同 sizeHint
QSize FancyTabBar::minimumSizeHint() const
{
    QSize sh = tabSizeHint(true);
    return QSize(sh.width(), sh.height() * m_tabs.count());
}

//获得第index个菜单项所在的屏幕区域
QRect FancyTabBar::tabRect(int index) const
{
    QSize sh = tabSizeHint(); //一个控件的尺寸

    if (sh.height() * m_tabs.count() > height()) //高度超过界面高
        sh.setHeight(height() / m_tabs.count()); //平分下界面高度

	//QRect(x,y,w,h) 从屏幕左上方开始 x,y为起始点，画一个宽w，高h的区域
    return QRect(0, index * sh.height(), sh.width(), sh.height()); 

}

// This keeps the sidebar responsive since
// we get a repaint before loading the
// mode itself
void FancyTabBar::emitCurrentIndex()
{
    emit currentChanged(m_currentIndex);
}

//鼠标点击事件 
void FancyTabBar::mousePressEvent(QMouseEvent *e)
{
    e->accept();
    for (int index = 0; index < m_tabs.count(); ++index) {
        if (tabRect(index).contains(e->pos())) { //找到点击控件

            if (isTabEnabled(index)) { //控件是启用状态
                m_currentIndex = index; //记录最近一次鼠标点击的控件索引
                update(); //更新
                m_triggerTimer.start(0); //启动定时器
            }
            break;
        }
    }
}
//绘制索引为 tabIdx的菜单项
void FancyTabBar::paintTab(QPainter *painter, int tabIndex) const
{
	//当前索引没有菜单 直接返回
    if (!validIndex(tabIndex)) {
        qWarning("invalid index");
        return;
    }
    painter->save();

	//拿到菜单项矩形区域 拿到菜单的选择 与 启用状态
    QRect rect = tabRect(tabIndex);
    bool selected = (tabIndex == m_currentIndex);
    bool enabled = isTabEnabled(tabIndex);

    if (selected) { //选择状态的绘制
        //background  背景色 用过渡色填充
        painter->save();
        QLinearGradient grad(rect.topLeft(), rect.topRight());
        grad.setColorAt(0, QColor(255, 255, 255, 140));
        grad.setColorAt(1, QColor(255, 255, 255, 210));
        painter->fillRect(rect.adjusted(0, 0, 0, -1), grad);
        painter->restore();

        //shadows 绘制阴影
        painter->setPen(QColor(0, 0, 0, 110));
        painter->drawLine(rect.topLeft() + QPoint(1,-1), rect.topRight() - QPoint(0,1));
        painter->drawLine(rect.bottomLeft(), rect.bottomRight());
        painter->setPen(QColor(0, 0, 0, 40));
        painter->drawLine(rect.topLeft(), rect.bottomLeft());

        //highlights 绘制高亮线条
        painter->setPen(QColor(255, 255, 255, 50));
        painter->drawLine(rect.topLeft() + QPoint(0, -2), rect.topRight() - QPoint(0,2));
        painter->drawLine(rect.bottomLeft() + QPoint(0, 1), rect.bottomRight() + QPoint(0,1));
        painter->setPen(QColor(255, 255, 255, 40));
        painter->drawLine(rect.topLeft() + QPoint(0, 0), rect.topRight());
        painter->drawLine(rect.topRight() + QPoint(0, 1), rect.bottomRight() - QPoint(0, 1));
        painter->drawLine(rect.bottomLeft() + QPoint(0,-1), rect.bottomRight()-QPoint(0,1));
    }

    QString tabText(this->tabText(tabIndex));//拿到文本
    QRect tabTextRect(rect); 
    const bool drawIcon = rect.height() > 36; //文本过大 则绘制图标
    QRect tabIconRect(tabTextRect);
    tabTextRect.translate(0, drawIcon ? -2 : 1);
    QFont boldFont(painter->font());
    boldFont.setPointSizeF(Utils::StyleHelper::sidebarFontSize());
    boldFont.setBold(true);
    painter->setFont(boldFont);
    painter->setPen(selected ? QColor(255, 255, 255, 160) : QColor(0, 0, 0, 110));
    const int textFlags = Qt::AlignCenter | (drawIcon ? Qt::AlignBottom : Qt::AlignVCenter) | Qt::TextWordWrap;
    if (enabled) { //菜单项启用状态 绘制文本
        painter->drawText(tabTextRect, textFlags, tabText);
        painter->setPen(selected ? QColor(60, 60, 60) : Utils::StyleHelper::panelTextColor());
    } else {
        painter->setPen(selected ? Utils::StyleHelper::panelTextColor() : QColor(255, 255, 255, 120));
    }

	//没选择 但是被启用的绘制
    if (!Utils::HostOsInfo::isMacHost() && !selected && enabled) {
        painter->save();
        int fader = int(m_tabs[tabIndex]->fader());
        QLinearGradient grad(rect.topLeft(), rect.topRight());
        grad.setColorAt(0, Qt::transparent);
        grad.setColorAt(0.5, QColor(255, 255, 255, fader));
        grad.setColorAt(1, Qt::transparent);
        painter->fillRect(rect, grad);
        painter->setPen(QPen(grad, 1.0));
        painter->drawLine(rect.topLeft(), rect.topRight());
        painter->drawLine(rect.bottomLeft(), rect.bottomRight());
        painter->restore();
    }

    if (!enabled) //没启用绘制透明
        painter->setOpacity(0.7);

    if (drawIcon) { //字体过大 图标收缩点
        int textHeight = painter->fontMetrics().boundingRect(QRect(0, 0, width(), height()), Qt::TextWordWrap, tabText).height();
        tabIconRect.adjust(0, 4, 0, -textHeight);
        Utils::StyleHelper::drawIconWithShadow(tabIcon(tabIndex), tabIconRect, painter, enabled ? QIcon::Normal : QIcon::Disabled);
    }

    painter->translate(0, -1);
    painter->drawText(tabTextRect, textFlags, tabText);
    painter->restore();
}
//设置当前活跃菜单项
void FancyTabBar::setCurrentIndex(int index) {
    if (isTabEnabled(index)) {
        m_currentIndex = index;
        update();
        emit currentChanged(m_currentIndex);
    }
}
//设置启用或者禁止 索引为index的控件
void FancyTabBar::setTabEnabled(int index, bool enable)
{
    Q_ASSERT(index < m_tabs.size());
    Q_ASSERT(index >= 0);

    if (index < m_tabs.size() && index >= 0) {
        m_tabs[index]->enabled = enable;
        update(tabRect(index));
    }
}
//判断索引为index的控件是否被启用
bool FancyTabBar::isTabEnabled(int index) const
{
    Q_ASSERT(index < m_tabs.size());
    Q_ASSERT(index >= 0);

    if (index < m_tabs.size() && index >= 0)
        return m_tabs[index]->enabled;

    return false;
}


//////
// FancyColorButton
//////
/*重写了颜色选择按钮*/
class FancyColorButton : public QWidget
{
public:
    FancyColorButton(QWidget *parent)
      : m_parent(parent)
    {
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    }

    void mousePressEvent(QMouseEvent *ev)
    {
        if (ev->modifiers() & Qt::ShiftModifier) {
            QColor color = QColorDialog::getColor(Utils::StyleHelper::requestedBaseColor(), m_parent);
            if (color.isValid())
                Utils::StyleHelper::setBaseColor(color);
        }
    }
private:
    QWidget *m_parent;
};

//////
// FancyTabWidget
//////
//参考 源码中 图片介绍->FancyTabWidget类对应布局.png
FancyTabWidget::FancyTabWidget(QWidget *parent)
    : QWidget(parent)
{
	//创建左侧菜单栏对象 欢迎-->帮助项窗口
    m_tabBar = new FancyTabBar(this);

    m_selectionWidget = new QWidget(this);
    QVBoxLayout *selectionLayout = new QVBoxLayout;
    selectionLayout->setSpacing(0); //spacing是针对于layout内部控件的间距
    selectionLayout->setMargin(0);  //页边距为0

    Utils::StyledBar *bar = new Utils::StyledBar;

	//lxp 注释掉 看着没有被使用这段代码
    //QHBoxLayout *layout = new QHBoxLayout(bar);
    //layout->setMargin(0);
    //layout->setSpacing(0);
    //layout->addWidget(new FancyColorButton(this));
    selectionLayout->addWidget(bar);

    selectionLayout->addWidget(m_tabBar, 1); //空间比例值为1
    m_selectionWidget->setLayout(selectionLayout); //m_selectionWidget(V)= bar + m_tabBar
    m_selectionWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    m_cornerWidgetContainer = new QWidget(this); //运行 调试 等菜单项 左下角窗口
    m_cornerWidgetContainer->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    m_cornerWidgetContainer->setAutoFillBackground(false);

    QVBoxLayout *cornerWidgetLayout = new QVBoxLayout; 
    cornerWidgetLayout->setSpacing(0);
    cornerWidgetLayout->setMargin(0);
    cornerWidgetLayout->addStretch();
    m_cornerWidgetContainer->setLayout(cornerWidgetLayout);

    selectionLayout->addWidget(m_cornerWidgetContainer, 0);

    m_modesStack = new QStackedLayout; //主内容显示窗口
    m_statusBar = new QStatusBar; //状态栏窗口
    m_statusBar->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

    QVBoxLayout *vlayout = new QVBoxLayout; //vlayout = m_modesStack + m_statusBar
    vlayout->setMargin(0);
    vlayout->setSpacing(0);
    vlayout->addLayout(m_modesStack);
    vlayout->addWidget(m_statusBar);

    QHBoxLayout *mainLayout = new QHBoxLayout; //主布局(HLayout) = m_selectionWidget-(selectionLayout) + vlayout
    mainLayout->setMargin(0);
    mainLayout->setSpacing(1);
    mainLayout->addWidget(m_selectionWidget);
    mainLayout->addLayout(vlayout);
    setLayout(mainLayout);

    connect(m_tabBar, SIGNAL(currentChanged(int)), this, SLOT(showWidget(int)));
}

//设置是否显示选择栏
void FancyTabWidget::setSelectionWidgetVisible(bool visible)
{
    m_selectionWidget->setVisible(visible);
}
//判断选择栏是否显示
bool FancyTabWidget::isSelectionWidgetVisible() const
{
    return m_selectionWidget->isVisible();
}
//增加一个窗体 和窗体对应的选择栏中TabBar的按钮项  eg:比如增加了欢迎按钮 和欢迎界面 则 tab就是欢迎界面,而index是欢迎按钮
//位置，icon是按钮图标,label是按钮文字
void FancyTabWidget::insertTab(int index, QWidget *tab, const QIcon &icon, const QString &label)
{
    m_modesStack->insertWidget(index, tab);
    m_tabBar->insertTab(index, icon, label);
}
//移除一个tab栏控制的窗体  
void FancyTabWidget::removeTab(int index)
{
    m_modesStack->removeWidget(m_modesStack->widget(index));
    m_tabBar->removeTab(index);
}
//设置选择栏的背景色   m_tabBar + m_cornerWidgetContainer
void FancyTabWidget::setBackgroundBrush(const QBrush &brush)
{
    QPalette pal = m_tabBar->palette();
    pal.setBrush(QPalette::Mid, brush);
    m_tabBar->setPalette(pal);
    pal = m_cornerWidgetContainer->palette();
    pal.setBrush(QPalette::Mid, brush);
    m_cornerWidgetContainer->setPalette(pal);
}

void FancyTabWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    if (m_selectionWidget->isVisible()) { //选择栏显示状态 则绘制
        QPainter painter(this);

        QRect rect = m_selectionWidget->rect().adjusted(0, 0, 1, 0);
        rect = style()->visualRect(layoutDirection(), geometry(), rect);
        Utils::StyleHelper::verticalGradient(&painter, rect, rect);
        painter.setPen(Utils::StyleHelper::borderColor());
        painter.drawLine(rect.topRight(), rect.bottomRight());

        QColor light = Utils::StyleHelper::sidebarHighlight();
        painter.setPen(light);
        painter.drawLine(rect.bottomLeft(), rect.bottomRight());
    }
}
//为 m_cornerWidgetContainer 插入一个点击项
void FancyTabWidget::insertCornerWidget(int pos, QWidget *widget)
{
    QVBoxLayout *layout = static_cast<QVBoxLayout *>(m_cornerWidgetContainer->layout());
    layout->insertWidget(pos, widget);
}
//获取 m_cornerWidgetContainer 的点击项个数
int FancyTabWidget::cornerWidgetCount() const
{
    return m_cornerWidgetContainer->layout()->count();
}
//为 m_cornerWidgetContainer 增加一个点击项为 eg:左边展示 调试 构建等的那几个菜单栏目 增加build按钮
void FancyTabWidget::addCornerWidget(QWidget *widget)
{
    m_cornerWidgetContainer->layout()->addWidget(widget);
}
// 获取Tab栏当前活跃菜单项索引
int FancyTabWidget::currentIndex() const
{
    return m_tabBar->currentIndex();
}

QStatusBar *FancyTabWidget::statusBar() const
{
    return m_statusBar;
}
//设置Tab栏当前活跃菜单项
void FancyTabWidget::setCurrentIndex(int index)
{
    if (m_tabBar->isTabEnabled(index))
        m_tabBar->setCurrentIndex(index);
}
//显示第i个tab项对应的界面
void FancyTabWidget::showWidget(int index)
{
    emit currentAboutToShow(index); //界面显示前的回调
    m_modesStack->setCurrentIndex(index);
    emit currentChanged(index); //tab选项变更消息
}

//设置Tab栏 第index个控件的提示信息
void FancyTabWidget::setTabToolTip(int index, const QString &toolTip)
{
    m_tabBar->setTabToolTip(index, toolTip);
}
//设置Tab栏启用或者禁止 索引为index的控件
void FancyTabWidget::setTabEnabled(int index, bool enable)
{
    m_tabBar->setTabEnabled(index, enable);
}
//判断Tab栏索引为index的控件是否被启用
bool FancyTabWidget::isTabEnabled(int index) const
{
    return m_tabBar->isTabEnabled(index);
}
