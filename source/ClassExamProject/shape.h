#ifndef SHAPE_H
#define SHAPE_H

#include <QPainter>
#include <QVector>
#include <QTransform>
#include <QtMath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

enum ShapeType {
    ShapeType_Rectangle,
    ShapeType_Ellipse
};

enum HandleType {
    None,
    Move,
    Rotate,
    Scale
};

class Shape {
public:
    Shape(ShapeType type, const QRectF& rect);
    virtual ~Shape() = default;

    struct TransformState {
        QRectF bounds;
        qreal rotation = 0;
        QPointF rotationCenter; // ��ת����
    };
    struct ControlHandle {
        QPointF pos;
        HandleType type;
        int index;
    };
    virtual void draw(QPainter* painter) = 0;
    bool contains(const QPointF& point) const {
        // ����ת�����ֲ�����ϵ��������ת��
        QTransform transform;
        transform.translate(boundingRect.center().x(), boundingRect.center().y());
        transform.rotate(-qRadiansToDegrees(m_rotation)); // ������ת
        transform.translate(-boundingRect.center().x(), -boundingRect.center().y());
        QPointF localPoint = transform.map(point);

        // �ھֲ�����ϵ�м��;
        if (m_brush != Qt::NoBrush && boundingRect.contains(localPoint)) {
            return true;
        }
        return strokeContains(localPoint);
    }
    virtual QVector<ControlHandle> getControlHandles() const = 0;
    virtual bool checkHandleHit(const QPointF& pos, int& outHandleIndex) const;
    virtual void applyTransform(const QTransform& matrix);
    virtual TransformState getTransformState() const;

    // ͨ������
    void setSelected(bool selected);
    bool isSelected() const;
    virtual void drawControlHandles(QPainter* painter) const;

    // �任����
    void setPosition(const QPointF& pos);
    // ����1��ͨ��ʵ�֣��Ƽ���
    virtual void setSize(const QSizeF& size) {
        boundingRect.setSize(size);
    }

    // ����2�����̶���İ汾��������Բ�����߼���
    virtual void setSize(const QPointF& fixedCorner, const QPointF& movingPos) {
        QRectF newRect(fixedCorner, movingPos);
        boundingRect = newRect.normalized();
    }

    virtual void moveBy(const QPointF& offset) {
        boundingRect.translate(offset);
    }

    ShapeType type;
    QColor borderColor = Qt::black;
    QColor fillColor = Qt::transparent;
    int borderWidth = 1;
    Qt::PenStyle borderStyle = Qt::SolidLine;
    qreal opacity = 1.0;
    //�ǶȽӿ�
    qreal getRotation() const { return m_rotation; }
    void setRotation(qreal angle) { m_rotation = angle; }
    void setRotationCenter(const QPointF& center) { m_rotationCenter = center; }
    QPointF getRotationCenter() const { return m_rotationCenter; }
    //�����ӿڣ��ı�������ͬʱ�ı�˽�б���
    void setPen(const QPen& pen) {
        if (m_pen != pen) {
            m_pen = pen;
            markDirty();  // �����Ҫ����m_needsUpdate = true;
        }
    }
    void markDirty() { m_needsUpdate = true; }
    bool needsUpdate() const { return m_needsUpdate; }
    void resetUpdateFlag() { m_needsUpdate = false; }
    void setBrush(const QBrush& brush) {
        if (m_brush != brush) {
            m_brush = brush;
            markDirty();  // �����Ҫ����m_needsUpdate = true;
        }
    }
    QPen pen() const { return m_pen; }
    QBrush brush() const { return m_brush; }
    QRectF boundingRect;

    //ͼ��˳�����
    int zValue() const { return m_zValue; }
    void setZValue(int z) { m_zValue = z; }

    virtual Shape* clone() const = 0;  // ���麯������
    QString text() const { return m_text; }
    bool hasText() const { return !m_text.isEmpty(); }
    void setText(const QString& text, const QFont& font = QFont(), const QColor& color = Qt::black);
    QFont textFont() const { return m_textFont; }
    QColor textColor() const { return m_textColor; }
    // ��Shape����������·���
    /*qreal getRotation() const { return m_rotation; }
    QPointF getRotationCenter() const { return m_rotationCenter; }*/
    // �޸����÷���
    void setTextFormat(const QFont& font, const QColor& color) {
        m_textFont = font;
        m_textColor = color;
        markDirty();
    }
protected:
    bool m_selected = false;
    static const int HANDLE_SIZE = 6;
    QPointF m_rotationCenter; // ��ת���ĵ�
    virtual bool strokeContains(const QPointF& point) const = 0;
private:
    qreal m_rotation = 0; // �洢��ת�Ƕ�
    QPen m_pen{ Qt::black, 2, Qt::SolidLine }; // Ĭ�Ϻ�ɫʵ��
    QBrush m_brush{ Qt::white };              // Ĭ�������
    bool m_needsUpdate = false;
    int m_zValue = 0; // ͼ��˳��ֵ
    QString m_text;
    QFont m_textFont{ "Arial", 12 }; // Ĭ������
    QColor m_textColor{ Qt::black }; // Ĭ�Ϻ�ɫ
};

class Rectangle : public Shape {
public:
    Rectangle(const QRectF& rect);
    void draw(QPainter* painter) override;
    QVector<ControlHandle> getControlHandles() const override;
    //bool contains(const QPointF& point) const override {
    //    return Shape::contains(point); // ֱ��ʹ�û����߼�
    //}
    Shape* clone() const override;  // ��ȷʹ��override
protected:
    bool strokeContains(const QPointF& point) const override;
};

class Ellipse : public Shape {
public:
    Ellipse(const QRectF& rect);
    void draw(QPainter* painter) override;
    QVector<ControlHandle> getControlHandles() const override;
    // ��ʽ����setSize
    void setSize(const QPointF& fixedCorner, const QPointF& movingPos) override; // ����2
    //bool contains(const QPointF& point) const override {
    //    return Shape::contains(point); // ֱ��ʹ�û����߼�
    //}
    Shape* clone() const override;  // ��ȷʹ��override
protected:
    bool strokeContains(const QPointF& point) const override;
};

#endif // SHAPE_H