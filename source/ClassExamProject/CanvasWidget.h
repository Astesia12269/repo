#ifndef CANVASWIDGET_H
#define CANVASWIDGET_H

#include <QWidget>
#include <QImage>
#include <QList>
#include "shape.h"

/**
 * 编辑器工作状态枚举
 */
enum EditorState {
    InsertState,    // 图形插入模式
    SelectState,    // 选择/编辑模式
    DragState       // 画布拖动模式
};

class CanvasWidget : public QWidget {
    Q_OBJECT

public:
    //=== 构造与基础控制 ===//
    explicit CanvasWidget(QWidget* parent = nullptr);

    CanvasWidget::~CanvasWidget() {
        // 清理图形列表
        qDeleteAll(shapes);
        shapes.clear();

        // 清理剪贴板
        if (m_copiedShape) {
            delete m_copiedShape;
            m_copiedShape = nullptr;
        }
    }

    //=== 状态控制 ===//
    void setEditorState(EditorState state);      // 设置编辑器状态
    void setCurrentShapeType(ShapeType type);    // 设置当前绘制图形类型

    //=== 画布操作 ===//
    void createNewCanvas(int width, int height); // 创建新画布
    bool saveToFile(const QString& fileName);    // 保存到文件
    bool loadFromFile(const QString& fileName);  // 从文件加载
    void clearCanvas();                          // 清空画布
    void setGridVisible(bool visible);           // 网格显示控制
    
    void setSelectedShape(Shape* shape);
    void mouseDoubleClickEvent(QMouseEvent* e);
    QImage toImage() const;  // 新增：将画布转换为QImage
    void setCanvasColor(const QColor& color);
signals:
    void selectionChanged(bool hasSelection);    // 选择状态变化信号

protected:
    //=== Qt事件重写 ===//
    //void keyPressEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    qreal m_scaleFactor = 1.0;
    QPoint m_lastPanPoint;
    bool m_isPanning = false;
    QPointF m_viewOffset;
    void applyZoom(qreal factor, const QPoint& mousePos);
    void ensureVisible(const QRectF& rect);
    QPointF mapToScene(const QPoint& viewPoint) const;

    Shape* m_clipboard = nullptr;  // 用于存储复制/剪切的图形
    //=== 图形数据 ===//
    QList<Shape*> shapes;            // 所有图形对象
    Shape* currentShape = nullptr;   // 当前正在创建的图形
    Shape* selectedShape = nullptr;  // 当前选中的图形
    Shape* m_copiedShape = nullptr; // 剪贴板图形
    QPointF m_pasteOffset{ 10, 10 }; // 粘贴偏移量

    //=== 绘制状态 ===//
    EditorState currentState = SelectState;      // 当前编辑器状态
    ShapeType currentShapeType = ShapeType_Rectangle; // 当前图形类型
    bool showGrid = true;            // 是否显示网格
    QImage canvasImage;              // 画布底层图像

    //=== 交互状态 ===//
    QPointF startPos;                // 鼠标起始位置
    QPointF lastMousePos;            // 鼠标上一位置
    QPointF fixedCorner;             // 拉伸操作固定角坐标
    bool isDrawing = false;          // 是否正在绘制
    int currentHandle = -1;          // 当前操作的控制点索引
    Shape::TransformState transformStartState; // 变换开始状态

    //=== 绘制方法 ===//
    void resizeCanvas(int width, int height);    // 调整画布尺寸
    void drawGrid(QPainter& painter);            // 绘制网格
    void drawShapes(QPainter& painter);          // 绘制所有图形
    void clearSelection();                       // 清除当前选择

    //=== 事件处理 ===//
    // 插入模式
    void startDrawingShape(const QPointF& pos);
    void continueDrawingShape(const QPointF& pos);
    void finishDrawingShape();
    void handleInsertPress(QMouseEvent* e);
    void handleInsertMove(QMouseEvent* e);
    void handleInsertRelease(QMouseEvent* e);

    // 选择模式
    void handleSelectPress(QMouseEvent* e);
    void handleSelectMove(QMouseEvent* e, const QPointF& delta);
    void handleSelectRelease(QMouseEvent* e);
    void updateCursor();                         // 更新鼠标样式

    //=== 编辑操作 ===//
    void copyShape();    // 复制图形
    void cutShape();     // 剪切图形
    void deleteShape();  // 删除图形
    void pasteShape();   // 粘贴图形
    //图层
    void updateZValues(); //更新所有Z值

    QColor m_canvasColor;  // 添加这一行
public slots:
    void moveShapeUp();    // 上移一层
    void moveShapeDown();  // 下移一层
    void moveShapeToTop(); // 置于顶层
    void moveShapeToBottom(); // 置于底层
 
private slots:
    //=== 属性编辑 ===//
    void editLineProperties();
    void editFillProperties();
};

#endif // CANVASWIDGET_H