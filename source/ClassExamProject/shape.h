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
        QPointF rotationCenter; // 旋转中心
    };
    struct ControlHandle {
        QPointF pos;
        HandleType type;
        int index;
    };
    virtual void draw(QPainter* painter) = 0;
    bool contains(const QPointF& point) const {
        // 将点转换到局部坐标系（考虑旋转）
        QTransform transform;
        transform.translate(boundingRect.center().x(), boundingRect.center().y());
        transform.rotate(-qRadiansToDegrees(m_rotation)); // 反向旋转
        transform.translate(-boundingRect.center().x(), -boundingRect.center().y());
        QPointF localPoint = transform.map(point);

        // 在局部坐标系中检测;
        if (m_brush != Qt::NoBrush && boundingRect.contains(localPoint)) {
            return true;
        }
        return strokeContains(localPoint);
    }
    virtual QVector<ControlHandle> getControlHandles() const = 0;
    virtual bool checkHandleHit(const QPointF& pos, int& outHandleIndex) const;
    virtual void applyTransform(const QTransform& matrix);
    virtual TransformState getTransformState() const;

    // 通用属性
    void setSelected(bool selected);
    bool isSelected() const;
    virtual void drawControlHandles(QPainter* painter) const;

    // 变换控制
    void setPosition(const QPointF& pos);
    // 方案1：通用实现（推荐）
    virtual void setSize(const QSizeF& size) {
        boundingRect.setSize(size);
    }

    // 方案2：带固定点的版本（如需椭圆特殊逻辑）
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
    //角度接口
    qreal getRotation() const { return m_rotation; }
    void setRotation(qreal angle) { m_rotation = angle; }
    void setRotationCenter(const QPointF& center) { m_rotationCenter = center; }
    QPointF getRotationCenter() const { return m_rotationCenter; }
    //线条接口，改变线条后同时改变私有变量
    void setPen(const QPen& pen) {
        if (m_pen != pen) {
            m_pen = pen;
            markDirty();  // 标记需要更新m_needsUpdate = true;
        }
    }
    void markDirty() { m_needsUpdate = true; }
    bool needsUpdate() const { return m_needsUpdate; }
    void resetUpdateFlag() { m_needsUpdate = false; }
    void setBrush(const QBrush& brush) {
        if (m_brush != brush) {
            m_brush = brush;
            markDirty();  // 标记需要更新m_needsUpdate = true;
        }
    }
    QPen pen() const { return m_pen; }
    QBrush brush() const { return m_brush; }
    QRectF boundingRect;

    //图层顺序相关
    int zValue() const { return m_zValue; }
    void setZValue(int z) { m_zValue = z; }

    virtual Shape* clone() const = 0;  // 纯虚函数声明
    QString text() const { return m_text; }
    bool hasText() const { return !m_text.isEmpty(); }
    void setText(const QString& text, const QFont& font = QFont(), const QColor& color = Qt::black);
    QFont textFont() const { return m_textFont; }
    QColor textColor() const { return m_textColor; }
    // 在Shape类中添加以下方法
    /*qreal getRotation() const { return m_rotation; }
    QPointF getRotationCenter() const { return m_rotationCenter; }*/
    // 修改设置方法
    void setTextFormat(const QFont& font, const QColor& color) {
        m_textFont = font;
        m_textColor = color;
        markDirty();
    }
protected:
    bool m_selected = false;
    static const int HANDLE_SIZE = 6;
    QPointF m_rotationCenter; // 旋转中心点
    virtual bool strokeContains(const QPointF& point) const = 0;
private:
    qreal m_rotation = 0; // 存储旋转角度
    QPen m_pen{ Qt::black, 2, Qt::SolidLine }; // 默认黑色实线
    QBrush m_brush{ Qt::white };              // 默认无填充
    bool m_needsUpdate = false;
    int m_zValue = 0; // 图层顺序值
    QString m_text;
    QFont m_textFont{ "Arial", 12 }; // 默认字体
    QColor m_textColor{ Qt::black }; // 默认黑色
};

class Rectangle : public Shape {
public:
    Rectangle(const QRectF& rect);
    void draw(QPainter* painter) override;
    QVector<ControlHandle> getControlHandles() const override;
    //bool contains(const QPointF& point) const override {
    //    return Shape::contains(point); // 直接使用基类逻辑
    //}
    Shape* clone() const override;  // 明确使用override
protected:
    bool strokeContains(const QPointF& point) const override;
};

class Ellipse : public Shape {
public:
    Ellipse(const QRectF& rect);
    void draw(QPainter* painter) override;
    QVector<ControlHandle> getControlHandles() const override;
    // 显式声明setSize
    void setSize(const QPointF& fixedCorner, const QPointF& movingPos) override; // 方案2
    //bool contains(const QPointF& point) const override {
    //    return Shape::contains(point); // 直接使用基类逻辑
    //}
    Shape* clone() const override;  // 明确使用override
protected:
    bool strokeContains(const QPointF& point) const override;
};

#endif // SHAPE_H