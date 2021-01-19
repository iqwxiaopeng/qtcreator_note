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
	/*һ������ߵ�ģʽ�˵�����  EG: ��ӭ �༭ ��Ƶ�*/
class FancyTab : public QObject
{
    Q_OBJECT

    Q_PROPERTY(float fader READ fader WRITE setFader)
public:
    FancyTab(QWidget *tabbar) : enabled(false), tabbar(tabbar), m_fader(0) {
        animator.setPropertyName("fader"); //ʵ��faderЧ��
        animator.setTargetObject(this); //�����󶨱�����
    }
	//���� ��ȡfader  ���ûᴥ�� tabbar��update����
    float fader() { return m_fader; }
    void setFader(float value);

	//fadein Ч������
    void fadeIn();

	//fadeOut Ч������
    void fadeOut();

    QIcon icon; //ͼ��
    QString text; //����
    QString toolTip; //��ʾ��Ϣ
    bool enabled;   //�Ƿ����

private:

	//  2.���������Ҫʵ�ֶ����Ķ��󣨸ö���Ҫ�̳���QObject����setTargetObject����
	//	3.������������Ҫʵ�ֶ��������� ������ͨ��Q_PROPERTY������������������ԵĶ�д������setPropertyName����
	//	4.�������Ե���ʼֵ����ֵֹ��setStartValue��setEndValue����
	//	5.���ö�������ʱ����setDuration����
	//	6.����������start����
    QPropertyAnimation animator; //��������
    QWidget *tabbar; //�����Ǹ��׶��� 
    float m_fader;
};

//����ߵ�ģʽ�˵��� ��ӭ--���� �Ȱ�ť
class FancyTabBar : public QWidget
{
    Q_OBJECT

public:
    FancyTabBar(QWidget *parent = 0);
    ~FancyTabBar();

	//���� ʵ���Զ���QEvent::ToolTip�¼�
    bool event(QEvent *event);

	//�ػ���Ϣ����  ���Ȼ��Ʋ���Ծ�Ĳ˵��� �����ƻ�Ծ��
    void paintEvent(QPaintEvent *event);
	//��������Ϊ tabIdx�Ĳ˵���
    void paintTab(QPainter *painter, int tabIndex) const;

	//������¼� �������������ؼ�������� ִ��update()������ʱ��
    void mousePressEvent(QMouseEvent *);

	//����ƶ��¼���д ����������ڲ˵�����������,�˵����Ŷ���
    void mouseMoveEvent(QMouseEvent *);

	//�������� ��յ�ǰ���λ�ü�¼
    void enterEvent(QEvent *);

	//����Ƴ� ��յ�ǰ���λ�ü�¼ ���˵��ؼ�ȫ������fadeOut
    void leaveEvent(QEvent *);

	//�жϲ˵��������Ƿ����
    bool validIndex(int index) const { return index >= 0 && index < m_tabs.count(); }

	//�Զ���ؼ�����ʵ�� ���ؿؼ���С
    QSize sizeHint() const;

	//�Զ���ؼ�����ʵ�� ���ؿؼ�����Ҫ����С��С
    QSize minimumSizeHint() const;

	//�������û��߽�ֹ ����Ϊindex�Ŀؼ�
    void setTabEnabled(int index, bool enable);

	//�ж�����Ϊindex�Ŀؼ��Ƿ�����
    bool isTabEnabled(int index) const;

	//��ģʽ�˵�������һ���µĲ˵�
    void insertTab(int index, const QIcon &icon, const QString &label) {
        FancyTab *tab = new FancyTab(this);
        tab->icon = icon;
        tab->text = label;
        m_tabs.insert(index, tab);
        updateGeometry();
    }

	
    void setEnabled(int index, bool enabled);

	//�Ƴ�����Ϊ index�Ŀؼ�
    void removeTab(int index) {
        FancyTab *tab = m_tabs.takeAt(index);
        delete tab;
        updateGeometry(); //���³ߴ���Ϣ
    }

	//���õ�ǰ��Ծ�˵���  ��ȡ��ǰ��Ծ�˵���
    void setCurrentIndex(int index);
    int currentIndex() const { return m_currentIndex; }

	//���� ��ȡ ��index���ؼ�����ʾ��Ϣ
    void setTabToolTip(int index, QString toolTip) { m_tabs[index]->toolTip = toolTip; }
    QString tabToolTip(int index) const { return m_tabs.at(index)->toolTip; }

	//��ȡ�� index���ؼ���ͼ��
    QIcon tabIcon(int index) const { return m_tabs.at(index)->icon; }
	//��ȡ�� index���ؼ����ı�
    QString tabText(int index) const { return m_tabs.at(index)->text; }

	//��ȡģʽ�� �˵���
    int count() const {return m_tabs.count(); }

	//��õ�index���˵������ڵ���Ļ����
    QRect tabRect(int index) const;

signals:
    void currentChanged(int); //��Ծ�˵���ı���Ϣ

public slots:
    void emitCurrentIndex();//��Ծ�˵���ı���Ϣ

private:
    static const int m_rounding; //22
    static const int m_textPadding; //4

	//����2��ֵ����������ƶ��仯 ����Ƴ���������
    QRect m_hoverRect; //��ǰ������ڵĲ˵����� Ҳ�����������Ĳ˵���area
    int m_hoverIndex; //��ǰ������ڵĲ˵�������ֵ


    int m_currentIndex; //��¼���һ��������˵�������
    QList<FancyTab*> m_tabs; //����ߵĹ������ϵ�Ԫ�� ��ӭ--���� �Ȱ�ť
    QTimer m_triggerTimer;

	//�������˵�����һ���˵���ĳߴ�  ���Ϊ max(70, ���˵������+2) �߶�Ϊ(40 + ����߶�)
    QSize tabSizeHint(bool minimum = false) const;

};

/*FancyTabWidget���ڴ�������ߵĹ�����,������FancyTabBar��CornerWidget��������,ͬʱ����������һ��StackedLayout��״̬��
  ������ʾ�ڲ�ͬģʽ�µĲ�ͬWidget����*/
////�ο� Դ���� ͼƬ����->FancyTabWidget���Ӧ����.png
class FancyTabWidget : public QWidget
{
    Q_OBJECT

public:
	//���� ������������������
    FancyTabWidget(QWidget *parent = 0);

	//����һ������ �ʹ����Ӧ��ѡ������TabBar�İ�ť��  eg:���������˻�ӭ��ť �ͻ�ӭ���� �� tab���ǻ�ӭ����,��index�ǻ�ӭ��ť
	//λ�ã�icon�ǰ�ťͼ��,label�ǰ�ť����
    void insertTab(int index, QWidget *tab, const QIcon &icon, const QString &label);

	//�Ƴ�һ��tab�����ƵĴ���  
    void removeTab(int index);

	//����ѡ�����ı���ɫ   m_tabBar + m_cornerWidgetContainer
    void setBackgroundBrush(const QBrush &brush);

	//Ϊ m_cornerWidgetContainer ����һ�������Ϊ eg:���չʾ ���� �����ȵ��Ǽ����˵���Ŀ ����build��ť
    void addCornerWidget(QWidget *widget);
	//Ϊ m_cornerWidgetContainer ����һ�������
    void insertCornerWidget(int pos, QWidget *widget);

	//��ȡ m_cornerWidgetContainer �ĵ�������
    int cornerWidgetCount() const;

	//���� Tab����index���ؼ�����ʾ��Ϣ
    void setTabToolTip(int index, const QString &toolTip);
	//�ػ���Ϣ
    void paintEvent(QPaintEvent *event);

	//��ȡTab����ǰ��Ծ�˵�������ֵ
    int currentIndex() const;
    QStatusBar *statusBar() const;

	//����Tab�����û��߽�ֹ ����Ϊindex�Ŀؼ� 
    void setTabEnabled(int index, bool enable);

	//�ж�Tab������Ϊindex�Ŀؼ��Ƿ�����
    bool isTabEnabled(int index) const;

	//�ж�ѡ�����Ƿ���ʾ
    bool isSelectionWidgetVisible() const;

signals:
    void currentAboutToShow(int index); //������ʾǰ��Ӧ��Ϣ
    void currentChanged(int index); //tabѡ��仯��Ϣ

public slots:
    void setCurrentIndex(int index);
	////�����Ƿ���ʾѡ����
    void setSelectionWidgetVisible(bool visible);

private slots:
    //��ʾ��i��tab���Ӧ�Ľ���
    void showWidget(int index);

private:
    FancyTabBar *m_tabBar; //���չʾģʽ���Ǽ����˵�
    QWidget *m_cornerWidgetContainer; //���չʾ ���� �����ȵ��Ǽ����˵� ʵ���ϵ�һ����Ŀ��һ��FancyActionBar
    QStackedLayout *m_modesStack; //������ͬ��ģʽ��Ӧ�������� eg:��ӭ��ť��Ӧ��ӭ��չʾ����,�༭��Ӧ�༭��
    QWidget *m_selectionWidget; //ѡ���� = Utils::StyledBar+ m_tabBar + m_cornerWidgetContainer
    QStatusBar *m_statusBar; //״̬��
};

} // namespace Internal
} // namespace Core

#endif // FANCYTABWIDGET_H
