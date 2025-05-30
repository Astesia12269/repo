#include "shape.h"
#include <QtMath>
#include <QApplication>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QTextDocument>
#include <QTextCursor>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


void Shape::setSelected(bool selected) {
    m_selected = selected;
}

bool Shape::isSelected() const {
    return m_selected;
}

void Shape::drawControlHandles(QPainter* painter) const {
    if (!m_selected) return;

    painter->save();

    // 应用旋转
    QPointF center = boundingRect.center();
    painter->translate(center);
    painter->rotate(qRadiansToDegrees(m_rotation));
    painter->translate(-center);

    // 绘制旋转后的虚线边界框
    painter->setPen(QPen(QColor(0, 0, 255, 150), 1, Qt::DashLine));
    painter->drawRect(boundingRect);

    painter->restore();

    // 绘制控制点（已通过getControlHandles()返回旋转后坐标）
    QVector<ControlHandle> handles = getControlHandles();
    for (const ControlHandle& h : handles) {
        painter->setPen(QPen(Qt::white, 2));
        painter->setBrush(h.type == Rotate ? Qt::green : Qt::red);
        painter->drawEllipse(h.pos, 6, 6);
    }
}

Shape::Shape(ShapeType type, const QRectF& rect)
    : type(type), boundingRect(rect) {}

void Shape::setPosition(const QPointF& pos) {
    boundingRect.moveTo(pos);
}

// 矩形实现
Rectangle::Rectangle(const QRectF& rect) : Shape(ShapeType_Rectangle, rect) {}

void Shape::setText(const QString& text, const QFont& font, const QColor& color) {
    m_text = text;
    m_textFont = font;
    m_textColor = color;
    markDirty();
}

void Rectangle::draw(QPainter* painter) {
    painter->save();

    // 应用旋转
    QPointF center = boundingRect.center();
    painter->translate(center);
    painter->rotate(qRadiansToDegrees(getRotation()));
    painter->translate(-center);

    // 先绘制填充（覆盖网格线）
    painter->setBrush(brush());
    painter->setPen(Qt::NoPen); // 填充时不需要边框
    painter->drawRect(boundingRect);

    // 再绘制边框（如果有）
    if (pen().style() != Qt::NoPen) {
        painter->setPen(pen());
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(boundingRect);
    }

    painter->restore();

    // 绘制控制点（选中时）
    if (isSelected()) {
        drawControlHandles(painter);
    }
    if (!text().isEmpty()) {
        painter->save();
        painter->setFont(textFont());
        painter->setPen(textColor());

        QTextDocument doc;
        doc.setHtml(text());
        doc.setDefaultFont(textFont());
        doc.setTextWidth(boundingRect.width() * 0.9); // 留边距

        // 居中对齐段落
        QTextCursor cursor(&doc);
        QTextBlockFormat fmt;
        fmt.setAlignment(Qt::AlignCenter);
        cursor.select(QTextCursor::Document);
        cursor.mergeBlockFormat(fmt);

        QSizeF docSize = doc.size();

        painter->translate(boundingRect.center());
        painter->rotate(qRadiansToDegrees(getRotation()));

        qreal xOffset = -docSize.width() / 2;
        qreal yOffset = -docSize.height() / 2;
        painter->translate(xOffset, yOffset);

        doc.drawContents(painter);

        painter->restore();
    }
}

// 椭圆实现
Ellipse::Ellipse(const QRectF& rect) : Shape(ShapeType_Ellipse, rect) {}


Shape::TransformState Shape::getTransformState() const {
    TransformState state;
    state.bounds = boundingRect;
    state.rotation = 0; // 基础形状初始无旋转
    return state;
}

QVector<Shape::ControlHandle> Shape::getControlHandles() const {
    QVector<ControlHandle> handles;
    const QRectF& rect = boundingRect;

    // 基本控制点（未旋转时的位置）
    QVector<QPointF> basePoints = {
        rect.topLeft(),     rect.topRight(),
        rect.bottomRight(), rect.bottomLeft(),
        QPointF(rect.center().x(), rect.top()),
        QPointF(rect.right(), rect.center().y()),
        QPointF(rect.center().x(), rect.bottom()),
        QPointF(rect.left(), rect.center().y()),
        QPointF(rect.center().x(), rect.top() - 20) // 旋转控制点
    };

    // 应用当前旋转
    QTransform transform;
    transform.translate(rect.center().x(), rect.center().y());
    transform.rotate(qRadiansToDegrees(m_rotation));
    transform.translate(-rect.center().x(), -rect.center().y());

    for (int i = 0; i < basePoints.size(); ++i) {
        handles.append({
            transform.map(basePoints[i]),  // 旋转后的坐标
            (i == 8) ? Rotate : Scale,    // 第9个点是旋转控制点
            i
            });
    }
    return handles;
}

QVector<Shape::ControlHandle> Rectangle::getControlHandles() const {
    QVector<Shape::ControlHandle> handles;
    const QRectF& rect = boundingRect;

    // 基本控制点（未旋转时的位置）
    QVector<QPointF> basePoints = {
        rect.topLeft(),      // 0: 左上角
        rect.topRight(),     // 1: 右上角
        rect.bottomRight(),  // 2: 右下角
        rect.bottomLeft(),   // 3: 左下角
        QPointF(rect.center().x(), rect.top()),    // 4: 上边中点
        QPointF(rect.right(), rect.center().y()),  // 5: 右边中点
        QPointF(rect.center().x(), rect.bottom()), // 6: 下边中点
        QPointF(rect.left(), rect.center().y()),   // 7: 左边中点
        QPointF(rect.center().x(), rect.top() - 20) // 8: 旋转控制点
    };

    // 应用当前旋转
    QTransform transform;
    transform.translate(rect.center().x(), rect.center().y());
    transform.rotate(qRadiansToDegrees(getRotation()));
    transform.translate(-rect.center().x(), -rect.center().y());

    // 生成旋转后的控制点
    for (int i = 0; i < basePoints.size(); ++i) {
        handles.append({
            transform.map(basePoints[i]),  // 应用旋转后的坐标
            (i == 8) ? Rotate : Scale,    // 第9个点是旋转控制点
            i                            // 控制点索引
            });
    }
    return handles;
}

// ellipse.cpp
void Ellipse::setSize(const QPointF& fixedCorner, const QPointF& movingPos) {
    // 计算新边界框（保持椭圆参数方程特性）
    qreal left = qMin(fixedCorner.x(), movingPos.x());
    qreal right = qMax(fixedCorner.x(), movingPos.x());
    qreal top = qMin(fixedCorner.y(), movingPos.y());
    qreal bottom = qMax(fixedCorner.y(), movingPos.y());

    boundingRect = QRectF(QPointF(left, top),
        QSizeF(right - left, bottom - top));
}

QVector<Shape::ControlHandle> Ellipse::getControlHandles() const {
    QVector<Shape::ControlHandle> handles;
    const QRectF& rect = boundingRect;

    // 椭圆使用与矩形相同的控制点布局（可自定义）
    QVector<QPointF> basePoints = {
        rect.topLeft(),      // 0: 左上角
        rect.topRight(),     // 1: 右上角
        rect.bottomRight(),  // 2: 右下角
        rect.bottomLeft(),   // 3: 左下角
        QPointF(rect.center().x(), rect.top()),    // 4: 上边中点
        QPointF(rect.right(), rect.center().y()),  // 5: 右边中点
        QPointF(rect.center().x(), rect.bottom()), // 6: 下边中点
        QPointF(rect.left(), rect.center().y()),   // 7: 左边中点
        QPointF(rect.center().x(), rect.top() - 30) // 8: 旋转控制点（更远）
    };

    // 应用旋转（与矩形相同逻辑）
    QTransform transform;
    transform.translate(rect.center().x(), rect.center().y());
    transform.rotate(qRadiansToDegrees(getRotation()));
    transform.translate(-rect.center().x(), -rect.center().y());

    for (int i = 0; i < basePoints.size(); ++i) {
        handles.append({
            transform.map(basePoints[i]),
            (i == 8) ? Rotate : Scale,
            i
            });
    }
    return handles;
}

// ellipse.cpp
void Ellipse::draw(QPainter* painter) {
    painter->save();

    // 应用旋转
    QPointF center = boundingRect.center();
    painter->translate(center);
    painter->rotate(qRadiansToDegrees(getRotation()));
    painter->translate(-center);

    // 先绘制填充（覆盖网格线）
    painter->setBrush(brush());
    painter->setPen(Qt::NoPen); // 填充时不需要边框
    painter->drawEllipse(boundingRect);

    // 再绘制边框（如果有）
    if (pen().style() != Qt::NoPen) {
        painter->setPen(pen());
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(boundingRect);
    }

    painter->restore();

    // 绘制控制点（选中时）
    if (isSelected()) {
        drawControlHandles(painter);
    }
    if (!text().isEmpty()) {
        painter->save();
        painter->setFont(textFont());
        painter->setPen(textColor());

        QTextDocument doc;
        doc.setHtml(text());
        doc.setDefaultFont(textFont());
        doc.setTextWidth(boundingRect.width() * 0.9); // 留边距

        // 居中对齐段落
        QTextCursor cursor(&doc);
        QTextBlockFormat fmt;
        fmt.setAlignment(Qt::AlignCenter);
        cursor.select(QTextCursor::Document);
        cursor.mergeBlockFormat(fmt);

        QSizeF docSize = doc.size();

        painter->translate(boundingRect.center());
        painter->rotate(qRadiansToDegrees(getRotation()));

        qreal xOffset = -docSize.width() / 2;
        qreal yOffset = -docSize.height() / 2;
        painter->translate(xOffset, yOffset);

        doc.drawContents(painter);

        painter->restore();
    }
}


//直接用基类实现所以删除
//void Ellipse::drawControlHandles(QPainter* painter) const {
//    // ...原有控制点绘制...
//
//    // 添加旋转控制线（绿色虚线）
//    painter->setPen(QPen(Qt::green, 1, Qt::DashLine));
//    painter->drawLine(boundingRect.center(),
//        QPointF(boundingRect.center().x(),
//            boundingRect.top() - 15)); // 短15像素避免重叠
//}

bool Shape::checkHandleHit(const QPointF& pos, int& outHandleIndex) const {
    auto handles = getControlHandles(); // 获取旋转后的控制点
    for (int i = 0; i < handles.size(); ++i) {
        if (QLineF(pos, handles[i].pos).length() < 10) { // 10像素命中半径
            outHandleIndex = i;
            return true;
        }
    }
    return false;
}

void Shape::applyTransform(const QTransform& matrix) {
    // 变换边界框
    QPolygonF poly = matrix.map(QPolygonF(boundingRect));
    boundingRect = poly.boundingRect();

    // 更新旋转角度（通过矩阵分解获取旋转分量）
    qreal dx = matrix.m11();
    qreal dy = matrix.m22();
    qreal shear = matrix.m12();
    m_rotation += qAtan2(shear, dx);
}

// Rectangle.cpp
bool Rectangle::strokeContains(const QPointF& point) const {
    QPainterPath path;
    path.addRect(boundingRect);
    QPainterPathStroker stroker(pen());
    return stroker.createStroke(path).contains(point);
}

// Ellipse.cpp 
bool Ellipse::strokeContains(const QPointF& point) const {
    QPainterPath path;
    path.addEllipse(boundingRect);
    QPainterPathStroker stroker(pen());
    return stroker.createStroke(path).contains(point);
}

// Rectangle.cpp
Shape* Rectangle::clone() const {
    Rectangle* newRect = new Rectangle(*this); // 调用拷贝构造函数
    newRect->boundingRect = this->boundingRect;
    newRect->setPen(this->pen());
    newRect->setBrush(this->brush());
    return newRect;
}

Shape* Ellipse::clone() const {
    Ellipse* newEllipse = new Ellipse(*this);
    newEllipse->boundingRect = this->boundingRect;
    newEllipse->setPen(this->pen());
    newEllipse->setBrush(this->brush());
    return newEllipse;
}