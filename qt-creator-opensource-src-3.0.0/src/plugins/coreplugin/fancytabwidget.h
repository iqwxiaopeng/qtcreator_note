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

#ifndef FANCYTABWIDGET_H
#define FANCYTABWIDGET_H

#include <QIcon>
#include <QWidget>

#include <QTimer>
#include <QPropertyAnimation>

QT_BEGIN_NAMESPACE
class QPainter;
class QStackedLayout;
class QStatusBar;
QT_END_NAMESPACE

namespace Core {
namespace Internal {
	/*一个最左边的模式菜单定义  EG: 欢迎 编辑 设计等*/
class FancyTab : public QObject
{
    Q_OBJECT

    Q_PROPERTY(float fader READ fader WRITE setFader)
public:
    FancyTab(QWidget *tabbar) : enabled(false), tabbar(tabbar), m_fader(0) {
        animator.setPropertyName("fader"); //实现fader效果
        animator.setTargetObject(this); //动画绑定本对象
    }
	//设置 获取fader  设置会触发 tabbar的update方法
    float fader() { return m_fader; }
    void setFader(float value);

	//fadein 效果动画
    void fadeIn();

	//fadeOut 效果动画
    void fadeOut();

    QIcon icon; //图标
    QString text; //文字
    QString toolTip; //提示信息
    bool enabled;   //是否可用

private:

	//  2.动画对象绑定要实现动画的对象（该对象要继承于QObject）【setTargetObject】。
	//	3.动画对象设置要实现动画的属性 ，属性通过Q_PROPERTY宏声明，必须包含属性的读写函数【setPropertyName】。
	//	4.设置属性的起始值和终止值【setStartValue和setEndValue】。
	//	5.设置动画运行时长【setDuration】。
	//	6.启动动画【start】。
    QPropertyAnimation animator; //动画对象
    QWidget *tabbar; //好像是父亲对象 
    float m_fader;
};

//最左边的模式菜单栏 欢迎--帮助 等按钮
class FancyTabBar : public QWidget
{
    Q_OBJECT

public:
    FancyTabBar(QWidget *parent = 0);
    ~FancyTabBar();

	//过滤 实现自定义QEvent::ToolTip事件
    bool event(QEvent *event);

	//重绘消息处理  首先绘制不活跃的菜单项 最后绘制活跃项
    void paintEvent(QPaintEvent *event);
	//绘制索引为 tabIdx的菜单项
    void paintTab(QPainter *painter, int tabIndex) const;

	//鼠标点击事件 更新最近鼠标点击控件索引标记 执行update()启动定时器
    void mousePressEvent(QMouseEvent *);

	//鼠标移动事件重写 更新鼠标所在菜单区域与索引,菜单播放动画
    void mouseMoveEvent(QMouseEvent *);

	//鼠标刚移入 清空当前鼠标位置记录
    void enterEvent(QEvent *);

	//鼠标移出 清空当前鼠标位置记录 左侧菜单控件全部播放fadeOut
    void leaveEvent(QEvent *);

	//判断菜单项索引是否合理
    bool validIndex(int index) const { return index >= 0 && index < m_tabs.count(); }

	//自定义控件必须实现 返回控件大小
    QSize sizeHint() const;

	//自定义控件必须实现 返回控件所需要的最小大小
    QSize minimumSizeHint() const;

	//设置启用或者禁止 索引为index的控件
    void setTabEnabled(int index, bool enable);

	//判断索引为index的控件是否被启用
    bool isTabEnabled(int index) const;

	//像模式菜单栏加入一个新的菜单
    void insertTab(int index, const QIcon &icon, const QString &label) {
        FancyTab *tab = new FancyTab(this);
        tab->icon = icon;
        tab->text = label;
        m_tabs.insert(index, tab);
        updateGeometry();
    }

	
    void setEnabled(int index, bool enabled);

	//移除索引为 index的控件
    void removeTab(int index) {
        FancyTab *tab = m_tabs.takeAt(index);
        delete tab;
        updateGeometry(); //更新尺寸信息
    }

	//设置当前活跃菜单项  获取当前活跃菜单项
    void setCurrentIndex(int index);
    int currentIndex() const { return m_currentIndex; }

	//设置 获取 第index个控件的提示信息
    void setTabToolTip(int index, QString toolTip) { m_tabs[index]->toolTip = toolTip; }
    QString tabToolTip(int index) const { return m_tabs.at(index)->toolTip; }

	//获取第 index个控件的图标
    QIcon tabIcon(int index) const { return m_tabs.at(index)->icon; }
	//获取第 index个控件的文本
    QString tabText(int index) const { return m_tabs.at(index)->text; }

	//获取模式栏 菜单数
    int count() const {return m_tabs.count(); }

	//获得第index个菜单项所在的屏幕区域
    QRect tabRect(int index) const;

signals:
    void currentChanged(int); //活跃菜单项改变消息

public slots:
    void emitCurrentIndex();//活跃菜单项改变消息

private:
    static const int m_rounding; //22
    static const int m_textPadding; //4

	//以下2个值会随着鼠标移动变化 鼠标移出区域会清空
    QRect m_hoverRect; //当前鼠标所在的菜单区域 也就是鼠标下面的菜单的area
    int m_hoverIndex; //当前鼠标所在的菜单项索引值


    int m_currentIndex; //记录最近一次鼠标点击菜单项索引
    QList<FancyTab*> m_tabs; //最左边的工具栏上的元素 欢迎--帮助 等按钮
    QTimer m_triggerTimer;

	//计算左侧菜单栏中一个菜单项的尺寸  宽度为 max(70, 最大菜单字体宽+2) 高度为(40 + 字体高度)
    QSize tabSizeHint(bool minimum = false) const;

};

/*FancyTabWidget用于创建最左边的工具栏,它包括FancyTabBar和CornerWidget两个部分,同时它还管理着一个StackedLayout和状态栏
  用于显示在不同模式下的不同Widget布局*/
////参考 源码中 图片介绍->FancyTabWidget类对应布局.png
class FancyTabWidget : public QWidget
{
    Q_OBJECT

public:
	//构造 此类铺满了整个窗体
    FancyTabWidget(QWidget *parent = 0);

	//增加一个窗体 和窗体对应的选择栏中TabBar的按钮项  eg:比如增加了欢迎按钮 和欢迎界面 则 tab就是欢迎界面,而index是欢迎按钮
	//位置，icon是按钮图标,label是按钮文字
    void insertTab(int index, QWidget *tab, const QIcon &icon, const QString &label);

	//移除一个tab栏控制的窗体  
    void removeTab(int index);

	//设置选择栏的背景色   m_tabBar + m_cornerWidgetContainer
    void setBackgroundBrush(const QBrush &brush);

	//为 m_cornerWidgetContainer 增加一个点击项为 eg:左边展示 调试 构建等的那几个菜单栏目 增加build按钮
    void addCornerWidget(QWidget *widget);
	//为 m_cornerWidgetContainer 插入一个点击项
    void insertCornerWidget(int pos, QWidget *widget);

	//获取 m_cornerWidgetContainer 的点击项个数
    int cornerWidgetCount() const;

	//设置 Tab栏第index个控件的提示信息
    void setTabToolTip(int index, const QString &toolTip);
	//重绘消息
    void paintEvent(QPaintEvent *event);

	//获取Tab栏当前活跃菜单项索引值
    int currentIndex() const;
    QStatusBar *statusBar() const;

	//设置Tab栏启用或者禁止 索引为index的控件 
    void setTabEnabled(int index, bool enable);

	//判断Tab栏索引为index的控件是否被启用
    bool isTabEnabled(int index) const;

	//判断选择栏是否显示
    bool isSelectionWidgetVisible() const;

signals:
    void currentAboutToShow(int index); //界面显示前对应消息
    void currentChanged(int index); //tab选项变化消息

public slots:
    void setCurrentIndex(int index);
	////设置是否显示选择栏
    void setSelectionWidgetVisible(bool visible);

private slots:
    //显示第i个tab项对应的界面
    void showWidget(int index);

private:
    FancyTabBar *m_tabBar; //左边展示模式的那几个菜单
    QWidget *m_cornerWidgetContainer; //左边展示 调试 构建等的那几个菜单 实际上第一个条目是一个FancyActionBar
    QStackedLayout *m_modesStack; //各个不同的模式对应的主窗体 eg:欢迎按钮对应欢迎的展示窗体,编辑对应编辑的
    QWidget *m_selectionWidget; //选择栏 = Utils::StyledBar+ m_tabBar + m_cornerWidgetContainer
    QStatusBar *m_statusBar; //状态栏
};

} // namespace Internal
} // namespace Core

#endif // FANCYTABWIDGET_H
