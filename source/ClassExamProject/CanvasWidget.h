#ifndef CANVASWIDGET_H
#define CANVASWIDGET_H

#include <QWidget>
#include <QImage>
#include <QList>
#include "shape.h"

/**
 * �༭������״̬ö��
 */
enum EditorState {
    InsertState,    // ͼ�β���ģʽ
    SelectState,    // ѡ��/�༭ģʽ
    DragState       // �����϶�ģʽ
};

class CanvasWidget : public QWidget {
    Q_OBJECT

public:
    //=== ������������� ===//
    explicit CanvasWidget(QWidget* parent = nullptr);

    CanvasWidget::~CanvasWidget() {
        // ����ͼ���б�
        qDeleteAll(shapes);
        shapes.clear();

        // ���������
        if (m_copiedShape) {
            delete m_copiedShape;
            m_copiedShape = nullptr;
        }
    }

    //=== ״̬���� ===//
    void setEditorState(EditorState state);      // ���ñ༭��״̬
    void setCurrentShapeType(ShapeType type);    // ���õ�ǰ����ͼ������

    //=== �������� ===//
    void createNewCanvas(int width, int height); // �����»���
    bool saveToFile(const QString& fileName);    // ���浽�ļ�
    bool loadFromFile(const QString& fileName);  // ���ļ�����
    void clearCanvas();                          // ��ջ���
    void setGridVisible(bool visible);           // ������ʾ����
    
    void setSelectedShape(Shape* shape);
    void mouseDoubleClickEvent(QMouseEvent* e);
    QImage toImage() const;  // ������������ת��ΪQImage
    void setCanvasColor(const QColor& color);
signals:
    void selectionChanged(bool hasSelection);    // ѡ��״̬�仯�ź�

protected:
    //=== Qt�¼���д ===//
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

    Shape* m_clipboard = nullptr;  // ���ڴ洢����/���е�ͼ��
    //=== ͼ������ ===//
    QList<Shape*> shapes;            // ����ͼ�ζ���
    Shape* currentShape = nullptr;   // ��ǰ���ڴ�����ͼ��
    Shape* selectedShape = nullptr;  // ��ǰѡ�е�ͼ��
    Shape* m_copiedShape = nullptr; // ������ͼ��
    QPointF m_pasteOffset{ 10, 10 }; // ճ��ƫ����

    //=== ����״̬ ===//
    EditorState currentState = SelectState;      // ��ǰ�༭��״̬
    ShapeType currentShapeType = ShapeType_Rectangle; // ��ǰͼ������
    bool showGrid = true;            // �Ƿ���ʾ����
    QImage canvasImage;              // �����ײ�ͼ��

    //=== ����״̬ ===//
    QPointF startPos;                // �����ʼλ��
    QPointF lastMousePos;            // �����һλ��
    QPointF fixedCorner;             // ��������̶�������
    bool isDrawing = false;          // �Ƿ����ڻ���
    int currentHandle = -1;          // ��ǰ�����Ŀ��Ƶ�����
    Shape::TransformState transformStartState; // �任��ʼ״̬

    //=== ���Ʒ��� ===//
    void resizeCanvas(int width, int height);    // ���������ߴ�
    void drawGrid(QPainter& painter);            // ��������
    void drawShapes(QPainter& painter);          // ��������ͼ��
    void clearSelection();                       // �����ǰѡ��

    //=== �¼����� ===//
    // ����ģʽ
    void startDrawingShape(const QPointF& pos);
    void continueDrawingShape(const QPointF& pos);
    void finishDrawingShape();
    void handleInsertPress(QMouseEvent* e);
    void handleInsertMove(QMouseEvent* e);
    void handleInsertRelease(QMouseEvent* e);

    // ѡ��ģʽ
    void handleSelectPress(QMouseEvent* e);
    void handleSelectMove(QMouseEvent* e, const QPointF& delta);
    void handleSelectRelease(QMouseEvent* e);
    void updateCursor();                         // ���������ʽ

    //=== �༭���� ===//
    void copyShape();    // ����ͼ��
    void cutShape();     // ����ͼ��
    void deleteShape();  // ɾ��ͼ��
    void pasteShape();   // ճ��ͼ��
    //ͼ��
    void updateZValues(); //��������Zֵ

    QColor m_canvasColor;  // �����һ��
public slots:
    void moveShapeUp();    // ����һ��
    void moveShapeDown();  // ����һ��
    void moveShapeToTop(); // ���ڶ���
    void moveShapeToBottom(); // ���ڵײ�
 
private slots:
    //=== ���Ա༭ ===//
    void editLineProperties();
    void editFillProperties();
};

#endif // CANVASWIDGET_H