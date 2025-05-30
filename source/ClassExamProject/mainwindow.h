#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenu>
#include "canvaswidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    const QPen& initialPen() const { return m_initialPen; }
    const QBrush& initialBrush() const { return m_initialBrush; }

private slots:
    void newCanvas();
    void saveCanvas();
    void saveAsPng();  // 新增：保存为PNG
    void loadCanvas();
    void toggleGrid(bool show);  // 新增：切换网格显示

    void insertRectangle();
    void insertEllipse();
    void setSelectMode();
    void editInitialLineProperties();  // 初始化线条属性
    void editInitialFillProperties();  // 初始化填充属性
    void newCanvasWithSetup();  // 替换原来的newCanvas

private:
    void setupMenu();
    void setupSettingsMenu();  // 新增：设置菜单
    void setupSelectMenu();
    CanvasWidget* canvasWidget;
    QAction* gridAction;  // 新增：网格动作
    void setupInsertMenu();  // 新增插入菜单
    QPen m_initialPen{ Qt::black, 2, Qt::SolidLine };  // 默认初始线条：黑色、宽度2
    QBrush m_initialBrush{ Qt::white };                // 默认初始填充：白色
};

#endif // MAINWINDOW_H