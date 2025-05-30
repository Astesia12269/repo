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

    // Ӧ����ת
    QPointF center = boundingRect.center();
    painter->translate(center);
    painter->rotate(qRadiansToDegrees(m_rotation));
    painter->translate(-center);

    // ������ת������߽߱��
    painter->setPen(QPen(QColor(0, 0, 255, 150), 1, Qt::DashLine));
    painter->drawRect(boundingRect);

    painter->restore();

    // ���ƿ��Ƶ㣨��ͨ��getControlHandles()������ת�����꣩
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

// ����ʵ��
Rectangle::Rectangle(const QRectF& rect) : Shape(ShapeType_Rectangle, rect) {}

void Shape::setText(const QString& text, const QFont& font, const QColor& color) {
    m_text = text;
    m_textFont = font;
    m_textColor = color;
    markDirty();
}

void Rectangle::draw(QPainter* painter) {
    painter->save();

    // Ӧ����ת
    QPointF center = boundingRect.center();
    painter->translate(center);
    painter->rotate(qRadiansToDegrees(getRotation()));
    painter->translate(-center);

    // �Ȼ�����䣨���������ߣ�
    painter->setBrush(brush());
    painter->setPen(Qt::NoPen); // ���ʱ����Ҫ�߿�
    painter->drawRect(boundingRect);

    // �ٻ��Ʊ߿�����У�
    if (pen().style() != Qt::NoPen) {
        painter->setPen(pen());
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(boundingRect);
    }

    painter->restore();

    // ���ƿ��Ƶ㣨ѡ��ʱ��
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
        doc.setTextWidth(boundingRect.width() * 0.9); // ���߾�

        // ���ж������
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

// ��Բʵ��
Ellipse::Ellipse(const QRectF& rect) : Shape(ShapeType_Ellipse, rect) {}


Shape::TransformState Shape::getTransformState() const {
    TransformState state;
    state.bounds = boundingRect;
    state.rotation = 0; // ������״��ʼ����ת
    return state;
}

QVector<Shape::ControlHandle> Shape::getControlHandles() const {
    QVector<ControlHandle> handles;
    const QRectF& rect = boundingRect;

    // �������Ƶ㣨δ��תʱ��λ�ã�
    QVector<QPointF> basePoints = {
        rect.topLeft(),     rect.topRight(),
        rect.bottomRight(), rect.bottomLeft(),
        QPointF(rect.center().x(), rect.top()),
        QPointF(rect.right(), rect.center().y()),
        QPointF(rect.center().x(), rect.bottom()),
        QPointF(rect.left(), rect.center().y()),
        QPointF(rect.center().x(), rect.top() - 20) // ��ת���Ƶ�
    };

    // Ӧ�õ�ǰ��ת
    QTransform transform;
    transform.translate(rect.center().x(), rect.center().y());
    transform.rotate(qRadiansToDegrees(m_rotation));
    transform.translate(-rect.center().x(), -rect.center().y());

    for (int i = 0; i < basePoints.size(); ++i) {
        handles.append({
            transform.map(basePoints[i]),  // ��ת�������
            (i == 8) ? Rotate : Scale,    // ��9��������ת���Ƶ�
            i
            });
    }
    return handles;
}

QVector<Shape::ControlHandle> Rectangle::getControlHandles() const {
    QVector<Shape::ControlHandle> handles;
    const QRectF& rect = boundingRect;

    // �������Ƶ㣨δ��תʱ��λ�ã�
    QVector<QPointF> basePoints = {
        rect.topLeft(),      // 0: ���Ͻ�
        rect.topRight(),     // 1: ���Ͻ�
        rect.bottomRight(),  // 2: ���½�
        rect.bottomLeft(),   // 3: ���½�
        QPointF(rect.center().x(), rect.top()),    // 4: �ϱ��е�
        QPointF(rect.right(), rect.center().y()),  // 5: �ұ��е�
        QPointF(rect.center().x(), rect.bottom()), // 6: �±��е�
        QPointF(rect.left(), rect.center().y()),   // 7: ����е�
        QPointF(rect.center().x(), rect.top() - 20) // 8: ��ת���Ƶ�
    };

    // Ӧ�õ�ǰ��ת
    QTransform transform;
    transform.translate(rect.center().x(), rect.center().y());
    transform.rotate(qRadiansToDegrees(getRotation()));
    transform.translate(-rect.center().x(), -rect.center().y());

    // ������ת��Ŀ��Ƶ�
    for (int i = 0; i < basePoints.size(); ++i) {
        handles.append({
            transform.map(basePoints[i]),  // Ӧ����ת�������
            (i == 8) ? Rotate : Scale,    // ��9��������ת���Ƶ�
            i                            // ���Ƶ�����
            });
    }
    return handles;
}

// ellipse.cpp
void Ellipse::setSize(const QPointF& fixedCorner, const QPointF& movingPos) {
    // �����±߽�򣨱�����Բ�����������ԣ�
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

    // ��Բʹ���������ͬ�Ŀ��Ƶ㲼�֣����Զ��壩
    QVector<QPointF> basePoints = {
        rect.topLeft(),      // 0: ���Ͻ�
        rect.topRight(),     // 1: ���Ͻ�
        rect.bottomRight(),  // 2: ���½�
        rect.bottomLeft(),   // 3: ���½�
        QPointF(rect.center().x(), rect.top()),    // 4: �ϱ��е�
        QPointF(rect.right(), rect.center().y()),  // 5: �ұ��е�
        QPointF(rect.center().x(), rect.bottom()), // 6: �±��е�
        QPointF(rect.left(), rect.center().y()),   // 7: ����е�
        QPointF(rect.center().x(), rect.top() - 30) // 8: ��ת���Ƶ㣨��Զ��
    };

    // Ӧ����ת���������ͬ�߼���
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

    // Ӧ����ת
    QPointF center = boundingRect.center();
    painter->translate(center);
    painter->rotate(qRadiansToDegrees(getRotation()));
    painter->translate(-center);

    // �Ȼ�����䣨���������ߣ�
    painter->setBrush(brush());
    painter->setPen(Qt::NoPen); // ���ʱ����Ҫ�߿�
    painter->drawEllipse(boundingRect);

    // �ٻ��Ʊ߿�����У�
    if (pen().style() != Qt::NoPen) {
        painter->setPen(pen());
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(boundingRect);
    }

    painter->restore();

    // ���ƿ��Ƶ㣨ѡ��ʱ��
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
        doc.setTextWidth(boundingRect.width() * 0.9); // ���߾�

        // ���ж������
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


//ֱ���û���ʵ������ɾ��
//void Ellipse::drawControlHandles(QPainter* painter) const {
//    // ...ԭ�п��Ƶ����...
//
//    // �����ת�����ߣ���ɫ���ߣ�
//    painter->setPen(QPen(Qt::green, 1, Qt::DashLine));
//    painter->drawLine(boundingRect.center(),
//        QPointF(boundingRect.center().x(),
//            boundingRect.top() - 15)); // ��15���ر����ص�
//}

bool Shape::checkHandleHit(const QPointF& pos, int& outHandleIndex) const {
    auto handles = getControlHandles(); // ��ȡ��ת��Ŀ��Ƶ�
    for (int i = 0; i < handles.size(); ++i) {
        if (QLineF(pos, handles[i].pos).length() < 10) { // 10�������а뾶
            outHandleIndex = i;
            return true;
        }
    }
    return false;
}

void Shape::applyTransform(const QTransform& matrix) {
    // �任�߽��
    QPolygonF poly = matrix.map(QPolygonF(boundingRect));
    boundingRect = poly.boundingRect();

    // ������ת�Ƕȣ�ͨ������ֽ��ȡ��ת������
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
    Rectangle* newRect = new Rectangle(*this); // ���ÿ������캯��
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