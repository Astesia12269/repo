#include "canvaswidget.h"
#include "shape.h"
#include "MainWindow.h"
#include "TextEditDialog.h"
#include <QPainter>
#include <QMenu>
#include <QFile>
#include <QDataStream>
#include <QDebug>
#include <QList>
#include <QEvent>
#include <QtGlobal>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QColorDialog>
#include <Qapplication>
#include <QFormLayout>
#include <QSpinBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QPushButton>
#include <QInputDialog>
CanvasWidget::CanvasWidget(QWidget* parent)
    : QWidget(parent),
    showGrid(true),
    m_canvasColor(Qt::white)  // ��ʼ������ɫΪ��ɫ// Ĭ����ʾ����
{
    createNewCanvas(800, 600);
    setEditorState(SelectState); // ��ʽ���ó�ʼ״̬

    QAction* copyAction = new QAction("Copy", this);
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, this, &CanvasWidget::copyShape);

    QAction* pasteAction = new QAction("Paste", this);
    pasteAction->setShortcut(QKeySequence::Paste);
    connect(pasteAction, &QAction::triggered, this, &CanvasWidget::pasteShape);

    QAction* cutAction = new QAction("Cut", this);
    cutAction->setShortcut(QKeySequence::Cut);
    connect(cutAction, &QAction::triggered, this, &CanvasWidget::cutShape);

    QAction* deleteAction = new QAction("Delete", this);
    deleteAction->setShortcut(QKeySequence::Delete);
    connect(deleteAction, &QAction::triggered, this, &CanvasWidget::deleteShape);

    this->addActions({ copyAction, pasteAction, cutAction, deleteAction });
}

void CanvasWidget::createNewCanvas(int width, int height)
{
    resizeCanvas(width, height);
    clearCanvas();
}

void CanvasWidget::resizeCanvas(int width, int height)
{
    canvasImage = QImage(width, height, QImage::Format_ARGB32);
    setFixedSize(width, height);
    update();
}

void CanvasWidget::clearCanvas()
{
    canvasImage.fill(Qt::white);
    update();
}

void CanvasWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);

    // 1. �Ȼ��Ʊ�������ɫ��
    painter.fillRect(rect(), m_canvasColor);

    // 2. ���������ߣ���ײ㣩
    if (showGrid) {
        drawGrid(painter);
    }

    // 3. ��������ͼ�Σ����������ߣ�
    drawShapes(painter);

    // 4. ���Ƶ�ǰ���ڴ�����ͼ�Σ����ϲ㣩
    if (isDrawing && currentShape) {
        currentShape->draw(&painter);
    }
}

// �������������
void CanvasWidget::drawGrid(QPainter& painter)
{
    painter.setPen(QPen(Qt::lightGray, 1, Qt::DotLine));
    for (int x = 0; x < width(); x += 20) {
        painter.drawLine(x, 0, x, height());
    }
    for (int y = 0; y < height(); y += 20) {
        painter.drawLine(0, y, width(), y);
    }
}

// ��������������ɼ���
void CanvasWidget::setGridVisible(bool visible)
{
    if (showGrid != visible) {
        showGrid = visible;
        update();  // �����ػ�
    }
}

bool CanvasWidget::saveToFile(const QString& fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file for writing:" << fileName
            << "Error:" << file.errorString();
        return false;
    }

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_5_15);

    // �ļ�ͷ��ʶ�Ͱ汾��
    out << quint32(0x464C4F57); // �ļ�ͷ��ʶ "FLOW"
    out << qint16(2);          // �汾��������2

    // ����������Ϣ
    out << qint32(width()) << qint32(height());
    out << showGrid;

    // ����ͼ������
    out << qint32(shapes.size());

    // ����ÿ��ͼ��
    for (Shape* shape : shapes) {
        // ����ͼ������
        out << qint32(shape->type);

        // �����������
        out << shape->boundingRect;
        out << shape->getRotation();
        out << shape->getRotationCenter();
        out << shape->zValue();

        // ������������
        QPen pen = shape->pen();
        out << pen.color();
        out << pen.width();
        out << qint32(pen.style());

        // �����������
        QBrush brush = shape->brush();
        out << brush.color();
        out << qint32(brush.style());

        // �����ı�����
        out << shape->text();
        out << shape->textFont();
        out << shape->textColor();
    }

    if (out.status() != QDataStream::Ok) {
        qWarning() << "Error during writing";
        file.remove();
        return false;
    }

    file.close();
    return true;
}

bool CanvasWidget::loadFromFile(const QString& fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file for reading:" << fileName
            << "Error:" << file.errorString();
        return false;
    }

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_5_15);

    // ��֤�ļ�ͷ
    quint32 magic;
    qint16 version;
    in >> magic >> version;

    if (magic != 0x464C4F57 || (version != 1 && version != 2)) {
        qWarning() << "Invalid file format";
        return false;
    }

    // ��ȡ�����ߴ�
    qint32 width, height;
    in >> width >> height;
    in >> showGrid;

    // ����֤�ߴ������
    if (width <= 0 || height <= 0 || width > 10000 || height > 10000) {
        qWarning() << "Invalid canvas size";
        return false;
    }

    // �����»���
    resizeCanvas(width, height);
    clearCanvas();
    setGridVisible(showGrid);

    // �汾2������ͼ�����ݼ���
    if (version >= 2) {
        qint32 shapeCount;
        in >> shapeCount;

        for (int i = 0; i < shapeCount; ++i) {
            qint32 type;
            in >> type;

            Shape* shape = nullptr;
            QRectF rect;
            qreal rotation;
            QPointF rotationCenter;
            int zValue;

            // ��ȡ��������
            in >> rect >> rotation >> rotationCenter >> zValue;

            // ����ͼ��
            switch (type) {
            case ShapeType_Rectangle:
                shape = new Rectangle(rect);
                break;
            case ShapeType_Ellipse:
                shape = new Ellipse(rect);
                break;
            default:
                qWarning() << "Unknown shape type:" << type;
                continue;
            }

            // ���ñ任����
            shape->setRotation(rotation);
            shape->setRotationCenter(rotationCenter);
            shape->setZValue(zValue);

            // ��ȡ��������
            QColor penColor;
            int penWidth;
            qint32 penStyle;
            in >> penColor >> penWidth >> penStyle;
            shape->setPen(QPen(penColor, penWidth, static_cast<Qt::PenStyle>(penStyle)));

            // ��ȡ�������
            QColor brushColor;
            qint32 brushStyle;
            in >> brushColor >> brushStyle;
            shape->setBrush(QBrush(brushColor, static_cast<Qt::BrushStyle>(brushStyle)));

            // ��ȡ�ı�����
            QString text;
            QFont textFont;
            QColor textColor;
            in >> text >> textFont >> textColor;
            shape->setText(text, textFont, textColor);

            shapes.append(shape);
        }
    }

    file.close();
    update();
    return true;
}

QImage CanvasWidget::toImage() const {
    // �����뻭����ͬ��С��QImage
    QImage image(size(), QImage::Format_ARGB32);
    image.fill(m_canvasColor);  // ��ɫ����

    // ʹ��QPainter�����ݻ��Ƶ�QImage
    QPainter painter(&image);

    // 1. ���Ʊ���
    painter.fillRect(rect(), Qt::white);

    // 2. �������������Ҫ��
    if (showGrid) {
        painter.setPen(QPen(Qt::lightGray, 1, Qt::DotLine));
        for (int x = 0; x < width(); x += 20) {
            painter.drawLine(x, 0, x, height());
        }
        for (int y = 0; y < height(); y += 20) {
            painter.drawLine(0, y, width(), y);
        }
    }

    // 3. ��������ͼ��
    for (Shape* shape : shapes) {
        // ����painter״̬
        painter.save();

        // Ӧ��ͼ�α任
        QPointF center = shape->boundingRect.center();
        painter.translate(center);
        painter.rotate(qRadiansToDegrees(shape->getRotation()));
        painter.translate(-center);

        // ����ͼ��
        shape->draw(&painter);

        // �ָ�painter״̬
        painter.restore();
    }

    return image;
}

// ����ͼ�λ��Ʒ���
void CanvasWidget::drawShapes(QPainter& painter) {
    // ��zֵ��С������ƣ��Ȼ��Ƶ������棩
    for (Shape* shape : shapes) {
        shape->draw(&painter);
    }

    // ��ǰ���ڻ��Ƶ�ͼ����������
    if (isDrawing && currentShape) {
        currentShape->draw(&painter);
    }
}

void CanvasWidget::mousePressEvent(QMouseEvent * e) {
    if (e->button() == Qt::MiddleButton ||
        (e->button() == Qt::RightButton && !selectedShape)) {
        m_isPanning = true;
        m_lastPanPoint = e->pos();
        setCursor(Qt::ClosedHandCursor);
        e->accept();
        return;
    }
    lastMousePos = e->pos();

    switch (currentState) {
    case InsertState:
        handleInsertPress(e);
        break;
    case SelectState:
        handleSelectPress(e);
        break;
    case DragState:
        // ��ͼ�϶��߼�
        break;
    }
}

void CanvasWidget::mouseMoveEvent(QMouseEvent* e) {
    
    if (m_isPanning) {
        QPoint delta = e->pos() - m_lastPanPoint;
        m_viewOffset += delta / m_scaleFactor;
        m_lastPanPoint = e->pos();
        update();
        e->accept();
        return;
    }
    QPointF delta = e->pos() - lastMousePos;
    lastMousePos = e->pos();

    switch (currentState) {
    case InsertState:
        handleInsertMove(e);
        break;
    case SelectState:
        handleSelectMove(e, delta);
        break;
    case DragState:
        // ��ͼ�϶��߼�
        break;
    }
    update();
}

void CanvasWidget::mouseReleaseEvent(QMouseEvent* e) {
    
    if ((e->button() == Qt::MiddleButton ||
        e->button() == Qt::RightButton) && m_isPanning) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
        e->accept();
        return;
    }
    switch (currentState) {
    case InsertState:
        handleInsertRelease(e);
        break;
    case SelectState:
        handleSelectRelease(e);
        break;
    case DragState:
        // ��ͼ�϶��߼�
        break;
    }
}

void CanvasWidget::handleInsertPress(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        startDrawingShape(e->pos());  // ��Ϊ����ͳһ����
    }
}

void CanvasWidget::handleInsertMove(QMouseEvent* e) {
    if (isDrawing) {
        continueDrawingShape(e->pos());  // ��Ϊ����ͳһ����
    }
}

void CanvasWidget::handleInsertRelease(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton && isDrawing && currentShape) {
        // ȷ��ͼ�δﵽ��С��Ч�ߴ�
        if (currentShape->boundingRect.width() > 10 && currentShape->boundingRect.height() > 10) {
            shapes.append(currentShape);
            currentShape = nullptr;
            isDrawing = false;
            update();

            // �Զ��л���ѡ��ģʽ����ѡ��
            setEditorState(SelectState);
        }
        else {
            // ɾ����С��ͼ��
            delete currentShape;
            currentShape = nullptr;
            isDrawing = false;
        }
    }
}

void CanvasWidget::handleSelectPress(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        clearSelection();

        // ��zֵ�Ӵ�С��飨�����ӵ��������棩
        for (int i = shapes.size() - 1; i >= 0; --i) {
            Shape* shape = shapes[i];
            int handleIndex;
            if (shape->checkHandleHit(e->pos(), handleIndex)) {
                selectedShape = shape;
                selectedShape->setSelected(true);
                currentHandle = handleIndex;
                startPos = e->pos();
                return;
            }

            if (shape->contains(e->pos())) {
                selectedShape = shape;
                selectedShape->setSelected(true);
                currentHandle = -1;
                startPos = e->pos();
                return;
            }
        }
    }
}

void CanvasWidget::moveShapeUp() {
    if (!selectedShape) return;

    int index = shapes.indexOf(selectedShape);
    if (index < shapes.size() - 1) {
        shapes.move(index, index + 1);
        updateZValues();
        update();
    }
}

void CanvasWidget::moveShapeDown() {
    if (!selectedShape) return;

    int index = shapes.indexOf(selectedShape);
    if (index > 0) {
        shapes.move(index, index - 1);
        updateZValues();
        update();
    }
}

void CanvasWidget::moveShapeToTop() {
    if (!selectedShape) return;

    int index = shapes.indexOf(selectedShape);
    if (index < shapes.size() - 1) {
        shapes.move(index, shapes.size() - 1);
        updateZValues();
        update();
    }
}

void CanvasWidget::moveShapeToBottom() {
    if (!selectedShape) return;

    int index = shapes.indexOf(selectedShape);
    if (index > 0) {
        shapes.move(index, 0);
        updateZValues();
        update();
    }
}

void CanvasWidget::updateZValues() {
    for (int i = 0; i < shapes.size(); ++i) {
        shapes[i]->setZValue(i);
    }
}

void CanvasWidget::handleSelectMove(QMouseEvent* e, const QPointF& delta) {
    if (!selectedShape) return;

    if (currentHandle == -1) {
        // �����ƶ��߼������䣩
        QTransform transform;
        transform.translate(delta.x(), delta.y());
        selectedShape->applyTransform(transform);
    }
    else {
        QRectF newRect = selectedShape->boundingRect;
        QPointF center = newRect.center();

        if (currentHandle == 8) {
            // ��ת���Ƶ㣨����ԭ15�Ȳ����߼���
            qreal newAngle = std::atan2(e->pos().y() - center.y(),
                e->pos().x() - center.x());
            if (e->modifiers() & Qt::ShiftModifier) {
                const qreal step = 15.0 * M_PI / 180.0;
                newAngle = qRound(newAngle / step) * step;
            }
            selectedShape->setRotation(newAngle);
        }
        else {
            // === ������������߼� ===
            QTransform inverseTransform;
            inverseTransform.translate(center.x(), center.y());
            inverseTransform.rotate(-qRadiansToDegrees(selectedShape->getRotation()));
            inverseTransform.translate(-center.x(), -center.y());
            QPointF localPos = inverseTransform.map(e->pos());

            // ����ԭʼ���������޸�ǰ��ȡ��
            const qreal originalRatio = selectedShape->boundingRect.width() /
                selectedShape->boundingRect.height();

            // ���ݿ��Ƶ���¾���
            switch (currentHandle) {
            case 0: newRect.setTopLeft(localPos); break;
            case 1: newRect.setTopRight(localPos); break;
            case 2: newRect.setBottomRight(localPos); break;
            case 3: newRect.setBottomLeft(localPos); break;
            case 4: newRect.setTop(localPos.y()); break;
            case 5: newRect.setRight(localPos.x()); break;
            case 6: newRect.setBottom(localPos.y()); break;
            case 7: newRect.setLeft(localPos.x()); break;
            }

            // Shift����Ϊѡ��
            if (e->modifiers() & Qt::ShiftModifier) {
                // ģʽ1���ǵ����ʱǿ��������/��Բ
                if (currentHandle <= 3) {
                    QPointF anchor;
                    switch (currentHandle) {
                    case 0: anchor = newRect.bottomRight(); break;
                    case 1: anchor = newRect.bottomLeft(); break;
                    case 2: anchor = newRect.topLeft(); break;
                    case 3: anchor = newRect.topRight(); break;
                    }

                    qreal dx = localPos.x() - anchor.x();
                    qreal dy = localPos.y() - anchor.y();

                    // ѡȡ�������򣬱���ԭʼ����
                    if (qAbs(dx) > qAbs(dy * originalRatio)) {
                        dy = dx / originalRatio;
                    }
                    else {
                        dx = dy * originalRatio;
                    }

                    QPointF adjustedPos(anchor.x() + dx, anchor.y() + dy);

                    switch (currentHandle) {
                    case 0: newRect.setTopLeft(adjustedPos); break;
                    case 1: newRect.setTopRight(adjustedPos); break;
                    case 2: newRect.setBottomRight(adjustedPos); break;
                    case 3: newRect.setBottomLeft(adjustedPos); break;
                    }
                }
                // ģʽ2�������е����ʱ����ԭʼ����
                else {
                    if (currentHandle == 4 || currentHandle == 6) { // ���±�
                        newRect.setWidth(newRect.height() * originalRatio);
                    }
                    else { // ���ұ�
                        newRect.setHeight(newRect.width() / originalRatio);
                    }
                }
            }

            selectedShape->boundingRect = newRect;
        }
    }
    update();
}

void CanvasWidget::startDrawingShape(const QPointF& pos) {
    startPos = pos;
    isDrawing = true;

    switch (currentShapeType) {
    case ShapeType_Rectangle:
        currentShape = new Rectangle(QRectF(pos, QSizeF(0, 0)));
        break;
    case ShapeType_Ellipse:  // �޸�
        currentShape = new Ellipse(QRectF(pos, QSizeF(0, 0)));
        break;
    }
    if (auto mw = qobject_cast<MainWindow*>(window())) {
        currentShape->setPen(mw->initialPen());   // ����MainWindow��ӷ��ʷ���
        currentShape->setBrush(mw->initialBrush());
    }
    currentShape->setZValue(shapes.size()); // ��ͼ�������ϲ�
}

void CanvasWidget::continueDrawingShape(const QPointF& pos) {
    if (!currentShape) return;

    qreal width = pos.x() - startPos.x();
    qreal height = pos.y() - startPos.y();

    // ��סShift��ʱ������Բ/������
    if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
        qreal size = qMax(qAbs(width), qAbs(height));
        width = width < 0 ? -size : size;
        height = height < 0 ? -size : size;
    }

    currentShape->setSize(QSizeF(width, height));
}

void CanvasWidget::finishDrawingShape() {
    if (!currentShape) return;

    shapes.append(currentShape);
    currentShape = nullptr;
    isDrawing = false;
}

void CanvasWidget::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);

    if (selectedShape) {
        // ���Բ˵�
        QMenu* propertiesMenu = menu.addMenu("Attribute");
        propertiesMenu->addAction("Line setting", this, &CanvasWidget::editLineProperties);
        propertiesMenu->addAction("Fill setting", this, &CanvasWidget::editFillProperties);

        // ͼ������˵�
        QMenu* layerMenu = menu.addMenu("Layer");
        layerMenu->addAction("Bring to Front", this, &CanvasWidget::moveShapeToTop);
        layerMenu->addAction("Bring Forward", this, &CanvasWidget::moveShapeUp);
        layerMenu->addAction("Send Backward", this, &CanvasWidget::moveShapeDown);
        layerMenu->addAction("Send to Back", this, &CanvasWidget::moveShapeToBottom);

        // �༭����
        menu.addAction("Copy", this, &CanvasWidget::copyShape);
        menu.addAction("Cut", this, &CanvasWidget::cutShape);
        menu.addAction("Delete", this, &CanvasWidget::deleteShape);
    }

    // ճ��ʼ�տ���
    QAction* pasteAction = menu.addAction("Paste", this, &CanvasWidget::pasteShape);
    pasteAction->setEnabled(m_copiedShape != nullptr);

    menu.exec(event->globalPos());
}

void CanvasWidget::editLineProperties() {
    if (!selectedShape) return;

    QDialog dialog(this);
    QFormLayout layout(&dialog);

    // ��ȡ��ǰ���ԣ�ȷ��ʹ�ø�����
    QPen currentPen = selectedShape->pen();
    QColor currentColor = currentPen.color();

    // ��ɫѡ��ť
    QPushButton colorBtn("Choose Color");
    colorBtn.setStyleSheet(QString("background-color: %1;").arg(currentColor.name()));

    // �߿�����
    QSpinBox widthSpin;
    widthSpin.setRange(1, 20);
    widthSpin.setValue(currentPen.width());

    // ��������
    QComboBox styleCombo;
    styleCombo.addItem("Solid", static_cast<int>(Qt::SolidLine));
    styleCombo.addItem("Dash", static_cast<int>(Qt::DashLine));
    styleCombo.setCurrentIndex(styleCombo.findData(static_cast<int>(currentPen.style())));

    // ��ť�飨�ؼ��޸������뱣��Ϊ��Ա������ֲ�������
    QDialogButtonBox btnBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    // ����
    layout.addRow("Color:", &colorBtn);
    layout.addRow("Width:", &widthSpin);
    layout.addRow("Style:", &styleCombo);
    layout.addRow(&btnBox);

    // ��ɫ��ť���
    connect(&colorBtn, &QPushButton::clicked, [&]() {
        QColor newColor = QColorDialog::getColor(currentColor, &dialog);
        if (newColor.isValid()) {
            currentColor = newColor;
            colorBtn.setStyleSheet(QString("background-color: %1;").arg(newColor.name()));
        }
        });

    // �ؼ��޸�����ȷ���Ӱ�ť�źţ�ʹ��accepted()�źţ�
    connect(&btnBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&btnBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // ִ�жԻ���
    if (dialog.exec() == QDialog::Accepted) {
        // Ӧ���޸�
        currentPen.setColor(currentColor);
        currentPen.setWidth(widthSpin.value());
        currentPen.setStyle(static_cast<Qt::PenStyle>(styleCombo.currentData().toInt()));

        selectedShape->setPen(currentPen);
        update(); // ǿ���ػ�

        qDebug() << "Line properties updated:" << currentPen; // �������
    }
}

void CanvasWidget::editFillProperties() {
    if (!selectedShape) return;

    QDialog dialog(this);
    QFormLayout layout(&dialog);

    // ��ȡ��ǰ����
    QBrush currentBrush = selectedShape->brush();
    bool noFill = (currentBrush.style() == Qt::NoBrush);
    QColor currentColor = noFill ? Qt::black : currentBrush.color();

    // ��ɫѡ��ť
    QPushButton colorButton("Select Color");
    if (!noFill) {
        colorButton.setStyleSheet(QString("background-color: %1").arg(currentColor.name()));
    }
    colorButton.setEnabled(!noFill);

    // ����临ѡ��
    QCheckBox noFillCheckbox("No fill");
    noFillCheckbox.setChecked(noFill);

    // ͸���Ȼ���
    QSlider alphaSlider(Qt::Horizontal);
    alphaSlider.setRange(0, 255);
    alphaSlider.setValue(noFill ? 255 : currentColor.alpha());
    alphaSlider.setEnabled(!noFill);

    // ��ť�飨�ؼ��޸���������ȷ�����źţ�
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    // ����
    layout.addRow("Color:", &colorButton);
    layout.addRow(&noFillCheckbox);
    layout.addRow("Opacity:", &alphaSlider);
    layout.addRow(&buttons);

    // ��ѡ��״̬�仯
    connect(&noFillCheckbox, &QCheckBox::stateChanged, [&](int state) {
        bool enabled = (state == Qt::Unchecked);
        colorButton.setEnabled(enabled);
        alphaSlider.setEnabled(enabled);

        // ���°�ť��ɫ��ʾ
        if (enabled) {
            colorButton.setStyleSheet(QString("background-color: %1").arg(currentColor.name()));
        }
        else {
            colorButton.setStyleSheet("");
        }
        });

    // ��ɫ��ť���
    connect(&colorButton, &QPushButton::clicked, [&]() {
        QColor newColor = QColorDialog::getColor(currentColor, &dialog,
            "Select Fill Color",
            QColorDialog::ShowAlphaChannel);
        if (newColor.isValid()) {
            currentColor = newColor;
            colorButton.setStyleSheet(QString("background-color: %1").arg(newColor.name()));
            alphaSlider.setValue(newColor.alpha());
        }
        });

    // �ؼ��޸�����ȷ���Ӱ�ť�ź�
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // ִ�жԻ���
    if (dialog.exec() == QDialog::Accepted) {
        QBrush newBrush;

        if (noFillCheckbox.isChecked()) {
            newBrush = QBrush(Qt::NoBrush);
        }
        else {
            currentColor.setAlpha(alphaSlider.value());
            newBrush = QBrush(currentColor);
        }

        selectedShape->setBrush(newBrush);
        update();

        qDebug() << "Fill properties updated:" << newBrush;
    }
}

void CanvasWidget::cutShape() {
    copyShape();
    if (m_copiedShape) {
        deleteShape(); // ɾ��ԭͼ��
    }
}

void CanvasWidget::deleteShape() {
    if (!selectedShape) return;

    // ʹ������ָ�����ȫ�����ʹ��QSharedPointer��
    shapes.removeOne(selectedShape); // Qt5.4+ ֧��

    // ȷ�������ظ�ɾ��
    Shape* toDelete = selectedShape;
    selectedShape = nullptr;

    delete toDelete;
    update();

    emit selectionChanged(false); // ֪ͨѡ��״̬�仯
}

void CanvasWidget::copyShape() {
    if (!selectedShape) return;

    // �����ͼ��
    if (m_copiedShape) {
        delete m_copiedShape;
        m_copiedShape = nullptr;
    }

    // ���
    m_copiedShape = selectedShape->clone();
}

void CanvasWidget::pasteShape() {
    if (!m_copiedShape) return;

    // ��������
    Shape* pasted = m_copiedShape->clone();
    pasted->setZValue(shapes.size()); // ��ͼ�������ϲ�
    // ����ճ��λ�ã����λ�û�Ĭ��ƫ�ƣ�
    QPointF pastePos = lastMousePos.isNull() ?
        QPointF(50, 50) :
        lastMousePos - m_copiedShape->boundingRect.center();

    pasted->moveBy(pastePos);
    shapes.append(pasted);

    // ѡ����ճ����ͼ��
    clearSelection();
    selectedShape = pasted;
    selectedShape->setSelected(true);

    update();
}

void CanvasWidget::setEditorState(EditorState state) {
    // ���ǰһ��״̬�Ĳ���
    if (currentState == InsertState && state != InsertState) {
        if (currentShape) {
            delete currentShape;
            currentShape = nullptr;
        }
        isDrawing = false;
    }

    currentState = state; // ����״̬
    updateCursor();       // ���¹��
    update();            // �����ػ�
}

void CanvasWidget::setCurrentShapeType(ShapeType type) {
    currentShapeType = type;
    setEditorState(InsertState); // �Զ��л�������ģʽ
}

void CanvasWidget::clearSelection() {
    if (selectedShape) {
        selectedShape->setSelected(false);
        selectedShape = nullptr;
    }
    currentHandle = -1;
    update();
}

void CanvasWidget::handleSelectRelease(QMouseEvent* e) {
    Q_UNUSED(e);
    // ��������������ͷź�Ĵ����߼�
    // ����ȷ�ϱ任������״̬��
}

void CanvasWidget::updateCursor() {
    switch (currentState) {
    case InsertState:
        setCursor(Qt::CrossCursor);
        break;
    case SelectState:
        setCursor(selectedShape ? Qt::SizeAllCursor : Qt::ArrowCursor);
        break;
    case DragState:
        setCursor(Qt::ClosedHandCursor);
        break;
    }
}

void CanvasWidget::mouseDoubleClickEvent(QMouseEvent* e) {
    if (currentState != SelectState) return;

    for (Shape* shape : shapes) {
        if (shape->contains(e->pos())) {
            TextEditDialog dialog(this);
            dialog.setText(shape->text());

            if (dialog.exec() == QDialog::Accepted) {
                shape->setText(
                    dialog.getText(),
                    dialog.getFont(),
                    dialog.getColor()
                );
                update();
            }
            return;
        }
    }
}

void CanvasWidget::setCanvasColor(const QColor& color)
{
    if (m_canvasColor != color) {
        m_canvasColor = color;
        update();  // �����ػ�
    }
}

void CanvasWidget::wheelEvent(QWheelEvent* event)
{
    // Ctrl+���֣�����
    if (event->modifiers() & Qt::ControlModifier) {
        qreal zoomFactor = 1.0 + (event->angleDelta().y() > 0 ? 0.1 : -0.1);
        applyZoom(zoomFactor, event->position().toPoint());
        event->accept();
    }
    // ��ͨ���֣���ֱ����
    else if (event->angleDelta().y() != 0) {
        m_viewOffset.ry() -= event->angleDelta().y() * 0.2;
        update();
        event->accept();
    }
    // ˮƽ���֣�ĳЩ���֧�֣�
    else if (event->angleDelta().x() != 0) {
        m_viewOffset.rx() -= event->angleDelta().x() * 0.2;
        update();
        event->accept();
    }
}

void CanvasWidget::applyZoom(qreal factor, const QPoint& mousePos)
{
    QPointF scenePosBefore = mapToScene(mousePos);

    // �������ŷ�Χ (0.1x - 10x)
    qreal newScale = qBound(0.1, m_scaleFactor * factor, 10.0);
    if (qFuzzyCompare(newScale, m_scaleFactor))
        return;

    m_scaleFactor = newScale;

    QPointF scenePosAfter = mapToScene(mousePos);
    m_viewOffset += scenePosAfter - scenePosBefore;

    update();
}

QPointF CanvasWidget::mapToScene(const QPoint& viewPoint) const
{
    return (QPointF(viewPoint) - m_viewOffset) / m_scaleFactor;
}

void CanvasWidget::ensureVisible(const QRectF& rect)
{
    // �Զ�������ͼȷ��ָ������ɼ�
    QRectF visibleRect(-m_viewOffset / m_scaleFactor, size() / m_scaleFactor);
    if (!visibleRect.contains(rect)) {
        m_viewOffset = -rect.topLeft() * m_scaleFactor;
        update();
    }
}