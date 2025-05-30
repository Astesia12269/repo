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
    m_canvasColor(Qt::white)  // 初始化背景色为白色// 默认显示网格
{
    createNewCanvas(800, 600);
    setEditorState(SelectState); // 显式设置初始状态

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

    // 1. 先绘制背景（白色）
    painter.fillRect(rect(), m_canvasColor);

    // 2. 绘制网格线（最底层）
    if (showGrid) {
        drawGrid(painter);
    }

    // 3. 绘制所有图形（覆盖网格线）
    drawShapes(painter);

    // 4. 绘制当前正在创建的图形（最上层）
    if (isDrawing && currentShape) {
        currentShape->draw(&painter);
    }
}

// 新增：网格绘制
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

// 新增：设置网格可见性
void CanvasWidget::setGridVisible(bool visible)
{
    if (showGrid != visible) {
        showGrid = visible;
        update();  // 触发重绘
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

    // 文件头标识和版本号
    out << quint32(0x464C4F57); // 文件头标识 "FLOW"
    out << qint16(2);          // 版本号升级到2

    // 画布基本信息
    out << qint32(width()) << qint32(height());
    out << showGrid;

    // 保存图形数量
    out << qint32(shapes.size());

    // 保存每个图形
    for (Shape* shape : shapes) {
        // 保存图形类型
        out << qint32(shape->type);

        // 保存基本属性
        out << shape->boundingRect;
        out << shape->getRotation();
        out << shape->getRotationCenter();
        out << shape->zValue();

        // 保存线条属性
        QPen pen = shape->pen();
        out << pen.color();
        out << pen.width();
        out << qint32(pen.style());

        // 保存填充属性
        QBrush brush = shape->brush();
        out << brush.color();
        out << qint32(brush.style());

        // 保存文本内容
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

    // 验证文件头
    quint32 magic;
    qint16 version;
    in >> magic >> version;

    if (magic != 0x464C4F57 || (version != 1 && version != 2)) {
        qWarning() << "Invalid file format";
        return false;
    }

    // 读取画布尺寸
    qint32 width, height;
    in >> width >> height;
    in >> showGrid;

    // 简单验证尺寸合理性
    if (width <= 0 || height <= 0 || width > 10000 || height > 10000) {
        qWarning() << "Invalid canvas size";
        return false;
    }

    // 创建新画布
    resizeCanvas(width, height);
    clearCanvas();
    setGridVisible(showGrid);

    // 版本2新增的图形数据加载
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

            // 读取基本属性
            in >> rect >> rotation >> rotationCenter >> zValue;

            // 创建图形
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

            // 设置变换属性
            shape->setRotation(rotation);
            shape->setRotationCenter(rotationCenter);
            shape->setZValue(zValue);

            // 读取线条属性
            QColor penColor;
            int penWidth;
            qint32 penStyle;
            in >> penColor >> penWidth >> penStyle;
            shape->setPen(QPen(penColor, penWidth, static_cast<Qt::PenStyle>(penStyle)));

            // 读取填充属性
            QColor brushColor;
            qint32 brushStyle;
            in >> brushColor >> brushStyle;
            shape->setBrush(QBrush(brushColor, static_cast<Qt::BrushStyle>(brushStyle)));

            // 读取文本内容
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
    // 创建与画布相同大小的QImage
    QImage image(size(), QImage::Format_ARGB32);
    image.fill(m_canvasColor);  // 白色背景

    // 使用QPainter将内容绘制到QImage
    QPainter painter(&image);

    // 1. 绘制背景
    painter.fillRect(rect(), Qt::white);

    // 2. 绘制网格（如果需要）
    if (showGrid) {
        painter.setPen(QPen(Qt::lightGray, 1, Qt::DotLine));
        for (int x = 0; x < width(); x += 20) {
            painter.drawLine(x, 0, x, height());
        }
        for (int y = 0; y < height(); y += 20) {
            painter.drawLine(0, y, width(), y);
        }
    }

    // 3. 绘制所有图形
    for (Shape* shape : shapes) {
        // 保存painter状态
        painter.save();

        // 应用图形变换
        QPointF center = shape->boundingRect.center();
        painter.translate(center);
        painter.rotate(qRadiansToDegrees(shape->getRotation()));
        painter.translate(-center);

        // 绘制图形
        shape->draw(&painter);

        // 恢复painter状态
        painter.restore();
    }

    return image;
}

// 新增图形绘制方法
void CanvasWidget::drawShapes(QPainter& painter) {
    // 按z值从小到大绘制（先绘制的在下面）
    for (Shape* shape : shapes) {
        shape->draw(&painter);
    }

    // 当前正在绘制的图形在最上面
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
        // 视图拖动逻辑
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
        // 视图拖动逻辑
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
        // 视图拖动逻辑
        break;
    }
}

void CanvasWidget::handleInsertPress(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        startDrawingShape(e->pos());  // 改为调用统一方法
    }
}

void CanvasWidget::handleInsertMove(QMouseEvent* e) {
    if (isDrawing) {
        continueDrawingShape(e->pos());  // 改为调用统一方法
    }
}

void CanvasWidget::handleInsertRelease(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton && isDrawing && currentShape) {
        // 确保图形达到最小有效尺寸
        if (currentShape->boundingRect.width() > 10 && currentShape->boundingRect.height() > 10) {
            shapes.append(currentShape);
            currentShape = nullptr;
            isDrawing = false;
            update();

            // 自动切换回选择模式（可选）
            setEditorState(SelectState);
        }
        else {
            // 删除过小的图形
            delete currentShape;
            currentShape = nullptr;
            isDrawing = false;
        }
    }
}

void CanvasWidget::handleSelectPress(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        clearSelection();

        // 按z值从大到小检查（最后添加的在最上面）
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
        // 整体移动逻辑（不变）
        QTransform transform;
        transform.translate(delta.x(), delta.y());
        selectedShape->applyTransform(transform);
    }
    else {
        QRectF newRect = selectedShape->boundingRect;
        QPointF center = newRect.center();

        if (currentHandle == 8) {
            // 旋转控制点（保持原15度步进逻辑）
            qreal newAngle = std::atan2(e->pos().y() - center.y(),
                e->pos().x() - center.x());
            if (e->modifiers() & Qt::ShiftModifier) {
                const qreal step = 15.0 * M_PI / 180.0;
                newAngle = qRound(newAngle / step) * step;
            }
            selectedShape->setRotation(newAngle);
        }
        else {
            // === 修正后的拉伸逻辑 ===
            QTransform inverseTransform;
            inverseTransform.translate(center.x(), center.y());
            inverseTransform.rotate(-qRadiansToDegrees(selectedShape->getRotation()));
            inverseTransform.translate(-center.x(), -center.y());
            QPointF localPos = inverseTransform.map(e->pos());

            // 保存原始比例（在修改前获取）
            const qreal originalRatio = selectedShape->boundingRect.width() /
                selectedShape->boundingRect.height();

            // 根据控制点更新矩形
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

            // Shift键行为选择
            if (e->modifiers() & Qt::ShiftModifier) {
                // 模式1：角点控制时强制正方形/正圆
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

                    // 选取主导方向，保持原始比例
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
                // 模式2：边线中点控制时锁定原始比例
                else {
                    if (currentHandle == 4 || currentHandle == 6) { // 上下边
                        newRect.setWidth(newRect.height() * originalRatio);
                    }
                    else { // 左右边
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
    case ShapeType_Ellipse:  // 修改
        currentShape = new Ellipse(QRectF(pos, QSizeF(0, 0)));
        break;
    }
    if (auto mw = qobject_cast<MainWindow*>(window())) {
        currentShape->setPen(mw->initialPen());   // 需在MainWindow添加访问方法
        currentShape->setBrush(mw->initialBrush());
    }
    currentShape->setZValue(shapes.size()); // 新图形在最上层
}

void CanvasWidget::continueDrawingShape(const QPointF& pos) {
    if (!currentShape) return;

    qreal width = pos.x() - startPos.x();
    qreal height = pos.y() - startPos.y();

    // 按住Shift键时保持正圆/正方形
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
        // 属性菜单
        QMenu* propertiesMenu = menu.addMenu("Attribute");
        propertiesMenu->addAction("Line setting", this, &CanvasWidget::editLineProperties);
        propertiesMenu->addAction("Fill setting", this, &CanvasWidget::editFillProperties);

        // 图层操作菜单
        QMenu* layerMenu = menu.addMenu("Layer");
        layerMenu->addAction("Bring to Front", this, &CanvasWidget::moveShapeToTop);
        layerMenu->addAction("Bring Forward", this, &CanvasWidget::moveShapeUp);
        layerMenu->addAction("Send Backward", this, &CanvasWidget::moveShapeDown);
        layerMenu->addAction("Send to Back", this, &CanvasWidget::moveShapeToBottom);

        // 编辑操作
        menu.addAction("Copy", this, &CanvasWidget::copyShape);
        menu.addAction("Cut", this, &CanvasWidget::cutShape);
        menu.addAction("Delete", this, &CanvasWidget::deleteShape);
    }

    // 粘贴始终可用
    QAction* pasteAction = menu.addAction("Paste", this, &CanvasWidget::pasteShape);
    pasteAction->setEnabled(m_copiedShape != nullptr);

    menu.exec(event->globalPos());
}

void CanvasWidget::editLineProperties() {
    if (!selectedShape) return;

    QDialog dialog(this);
    QFormLayout layout(&dialog);

    // 获取当前属性（确保使用副本）
    QPen currentPen = selectedShape->pen();
    QColor currentColor = currentPen.color();

    // 颜色选择按钮
    QPushButton colorBtn("Choose Color");
    colorBtn.setStyleSheet(QString("background-color: %1;").arg(currentColor.name()));

    // 线宽设置
    QSpinBox widthSpin;
    widthSpin.setRange(1, 20);
    widthSpin.setValue(currentPen.width());

    // 线型设置
    QComboBox styleCombo;
    styleCombo.addItem("Solid", static_cast<int>(Qt::SolidLine));
    styleCombo.addItem("Dash", static_cast<int>(Qt::DashLine));
    styleCombo.setCurrentIndex(styleCombo.findData(static_cast<int>(currentPen.style())));

    // 按钮组（关键修复：必须保存为成员变量或局部变量）
    QDialogButtonBox btnBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    // 布局
    layout.addRow("Color:", &colorBtn);
    layout.addRow("Width:", &widthSpin);
    layout.addRow("Style:", &styleCombo);
    layout.addRow(&btnBox);

    // 颜色按钮点击
    connect(&colorBtn, &QPushButton::clicked, [&]() {
        QColor newColor = QColorDialog::getColor(currentColor, &dialog);
        if (newColor.isValid()) {
            currentColor = newColor;
            colorBtn.setStyleSheet(QString("background-color: %1;").arg(newColor.name()));
        }
        });

    // 关键修复：正确连接按钮信号（使用accepted()信号）
    connect(&btnBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&btnBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // 执行对话框
    if (dialog.exec() == QDialog::Accepted) {
        // 应用修改
        currentPen.setColor(currentColor);
        currentPen.setWidth(widthSpin.value());
        currentPen.setStyle(static_cast<Qt::PenStyle>(styleCombo.currentData().toInt()));

        selectedShape->setPen(currentPen);
        update(); // 强制重绘

        qDebug() << "Line properties updated:" << currentPen; // 调试输出
    }
}

void CanvasWidget::editFillProperties() {
    if (!selectedShape) return;

    QDialog dialog(this);
    QFormLayout layout(&dialog);

    // 获取当前属性
    QBrush currentBrush = selectedShape->brush();
    bool noFill = (currentBrush.style() == Qt::NoBrush);
    QColor currentColor = noFill ? Qt::black : currentBrush.color();

    // 颜色选择按钮
    QPushButton colorButton("Select Color");
    if (!noFill) {
        colorButton.setStyleSheet(QString("background-color: %1").arg(currentColor.name()));
    }
    colorButton.setEnabled(!noFill);

    // 无填充复选框
    QCheckBox noFillCheckbox("No fill");
    noFillCheckbox.setChecked(noFill);

    // 透明度滑块
    QSlider alphaSlider(Qt::Horizontal);
    alphaSlider.setRange(0, 255);
    alphaSlider.setValue(noFill ? 255 : currentColor.alpha());
    alphaSlider.setEnabled(!noFill);

    // 按钮组（关键修复：必须正确连接信号）
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    // 布局
    layout.addRow("Color:", &colorButton);
    layout.addRow(&noFillCheckbox);
    layout.addRow("Opacity:", &alphaSlider);
    layout.addRow(&buttons);

    // 复选框状态变化
    connect(&noFillCheckbox, &QCheckBox::stateChanged, [&](int state) {
        bool enabled = (state == Qt::Unchecked);
        colorButton.setEnabled(enabled);
        alphaSlider.setEnabled(enabled);

        // 更新按钮颜色显示
        if (enabled) {
            colorButton.setStyleSheet(QString("background-color: %1").arg(currentColor.name()));
        }
        else {
            colorButton.setStyleSheet("");
        }
        });

    // 颜色按钮点击
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

    // 关键修复：正确连接按钮信号
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // 执行对话框
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
        deleteShape(); // 删除原图形
    }
}

void CanvasWidget::deleteShape() {
    if (!selectedShape) return;

    // 使用智能指针更安全（如果使用QSharedPointer）
    shapes.removeOne(selectedShape); // Qt5.4+ 支持

    // 确保不会重复删除
    Shape* toDelete = selectedShape;
    selectedShape = nullptr;

    delete toDelete;
    update();

    emit selectionChanged(false); // 通知选择状态变化
}

void CanvasWidget::copyShape() {
    if (!selectedShape) return;

    // 清理旧图形
    if (m_copiedShape) {
        delete m_copiedShape;
        m_copiedShape = nullptr;
    }

    // 深拷贝
    m_copiedShape = selectedShape->clone();
}

void CanvasWidget::pasteShape() {
    if (!m_copiedShape) return;

    // 创建副本
    Shape* pasted = m_copiedShape->clone();
    pasted->setZValue(shapes.size()); // 新图形在最上层
    // 设置粘贴位置（鼠标位置或默认偏移）
    QPointF pastePos = lastMousePos.isNull() ?
        QPointF(50, 50) :
        lastMousePos - m_copiedShape->boundingRect.center();

    pasted->moveBy(pastePos);
    shapes.append(pasted);

    // 选中新粘贴的图形
    clearSelection();
    selectedShape = pasted;
    selectedShape->setSelected(true);

    update();
}

void CanvasWidget::setEditorState(EditorState state) {
    // 清除前一个状态的残留
    if (currentState == InsertState && state != InsertState) {
        if (currentShape) {
            delete currentShape;
            currentShape = nullptr;
        }
        isDrawing = false;
    }

    currentState = state; // 更新状态
    updateCursor();       // 更新光标
    update();            // 触发重绘
}

void CanvasWidget::setCurrentShapeType(ShapeType type) {
    currentShapeType = type;
    setEditorState(InsertState); // 自动切换到插入模式
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
    // 可以在这里添加释放后的处理逻辑
    // 比如确认变换、保存状态等
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
        update();  // 触发重绘
    }
}

void CanvasWidget::wheelEvent(QWheelEvent* event)
{
    // Ctrl+滚轮：缩放
    if (event->modifiers() & Qt::ControlModifier) {
        qreal zoomFactor = 1.0 + (event->angleDelta().y() > 0 ? 0.1 : -0.1);
        applyZoom(zoomFactor, event->position().toPoint());
        event->accept();
    }
    // 普通滚轮：垂直滚动
    else if (event->angleDelta().y() != 0) {
        m_viewOffset.ry() -= event->angleDelta().y() * 0.2;
        update();
        event->accept();
    }
    // 水平滚轮（某些鼠标支持）
    else if (event->angleDelta().x() != 0) {
        m_viewOffset.rx() -= event->angleDelta().x() * 0.2;
        update();
        event->accept();
    }
}

void CanvasWidget::applyZoom(qreal factor, const QPoint& mousePos)
{
    QPointF scenePosBefore = mapToScene(mousePos);

    // 限制缩放范围 (0.1x - 10x)
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
    // 自动调整视图确保指定区域可见
    QRectF visibleRect(-m_viewOffset / m_scaleFactor, size() / m_scaleFactor);
    if (!visibleRect.contains(rect)) {
        m_viewOffset = -rect.topLeft() * m_scaleFactor;
        update();
    }
}