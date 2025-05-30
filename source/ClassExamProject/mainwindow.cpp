#include "mainwindow.h"
#include "canvassetupdialog.h"
#include <QMenu>
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QStatusBar>
#include <QFormLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QColor>
#include <QColorDialog>
#include <QCheckBox>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
    canvasWidget(new CanvasWidget(this))
{
    setWindowTitle("Flowchart Painter");
    resize(800, 600);
    setCentralWidget(canvasWidget);
    setupMenu();
    setupSettingsMenu();  // 初始化设置菜单
    setupSelectMenu();  // 显式调用新增的菜单初始化
}

void MainWindow::setupMenu()
{
    QMenu* fileMenu = menuBar()->addMenu("File");
    setupInsertMenu();  // 添加插入菜单
    QAction* newAction = fileMenu->addAction("New");
    QAction* saveAction = fileMenu->addAction("Save");
    QAction* loadAction = fileMenu->addAction("Load");
    QAction* savePngAction = fileMenu->addAction("Save as PNG");  // 新增

    connect(newAction, &QAction::triggered, this, &MainWindow::newCanvasWithSetup);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveCanvas);
    connect(loadAction, &QAction::triggered, this, &MainWindow::loadCanvas);
    connect(savePngAction, &QAction::triggered, this, &MainWindow::saveAsPng);  // 新增
}

void MainWindow::setupInsertMenu() {
    QMenu* insertMenu = menuBar()->addMenu("Insert");

    QAction* rectAction = insertMenu->addAction("Rectangle");
    QAction* ellipseAction = insertMenu->addAction("Ellipse");  // 修改为Ellipse

    connect(rectAction, &QAction::triggered, this, &MainWindow::insertRectangle);
    connect(ellipseAction, &QAction::triggered, this, &MainWindow::insertEllipse);  // 修改
}

void MainWindow::setupSelectMenu() {
    QMenu* selectMenu = menuBar()->addMenu("Select");
    QAction* selectAction = selectMenu->addAction("Enable Selection");
    connect(selectAction, &QAction::triggered, this, &MainWindow::setSelectMode);
}

void MainWindow::insertEllipse() {
    canvasWidget->setCurrentShapeType(ShapeType_Ellipse);  // 修改
}

void MainWindow::insertRectangle() {
    canvasWidget->setCurrentShapeType(ShapeType_Rectangle);
}

void MainWindow::setupSettingsMenu()
{
    QMenu* settingsMenu = menuBar()->addMenu("Settings");

    // 1. 网格显示开关（原有代码）
    gridAction = new QAction("Show Grid", this);
    gridAction->setCheckable(true);
    gridAction->setChecked(true);
    settingsMenu->addAction(gridAction);
    connect(gridAction, &QAction::toggled, this, &MainWindow::toggleGrid);

    // 2. 新增初始化图形属性子菜单
    QMenu* initPropsMenu = settingsMenu->addMenu("Initialize Shape Properties");
    QAction* lineAction = initPropsMenu->addAction("Line Settings");
    QAction* fillAction = initPropsMenu->addAction("Fill Settings");

    // 连接信号槽
    connect(lineAction, &QAction::triggered, this, &MainWindow::editInitialLineProperties);
    connect(fillAction, &QAction::triggered, this, &MainWindow::editInitialFillProperties);
}

void MainWindow::editInitialLineProperties() {
    QDialog dialog(this);
    QFormLayout layout(&dialog);

    // 颜色按钮
    QPushButton colorBtn("Choose Color");
    colorBtn.setStyleSheet(QString("background-color: %1;").arg(m_initialPen.color().name()));

    // 线宽设置
    QSpinBox widthSpin;
    widthSpin.setRange(1, 20);
    widthSpin.setValue(m_initialPen.width());

    // 线型设置
    QComboBox styleCombo;
    styleCombo.addItem("Solid", static_cast<int>(Qt::SolidLine));
    styleCombo.addItem("Dash", static_cast<int>(Qt::DashLine));
    styleCombo.setCurrentIndex(styleCombo.findData(static_cast<int>(m_initialPen.style())));

    // 确认按钮
    QDialogButtonBox btnBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout.addRow("Color:", &colorBtn);
    layout.addRow("Width:", &widthSpin);
    layout.addRow("Style:", &styleCombo);
    layout.addRow(&btnBox);

    // 颜色选择
    connect(&colorBtn, &QPushButton::clicked, [&]() {
        QColor newColor = QColorDialog::getColor(m_initialPen.color(), &dialog);
        if (newColor.isValid()) {
            colorBtn.setStyleSheet(QString("background-color: %1;").arg(newColor.name()));
            m_initialPen.setColor(newColor);
        }
        });

    // 确认保存
    connect(&btnBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&btnBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        m_initialPen.setWidth(widthSpin.value());
        m_initialPen.setStyle(static_cast<Qt::PenStyle>(styleCombo.currentData().toInt()));
    }
}

void MainWindow::editInitialFillProperties() {
    QDialog dialog(this);
    QFormLayout layout(&dialog);

    // 颜色按钮
    QPushButton colorButton("Select Color");
    bool noFill = (m_initialBrush.style() == Qt::NoBrush);
    colorButton.setStyleSheet(noFill ? "" : QString("background-color: %1").arg(m_initialBrush.color().name()));
    colorButton.setEnabled(!noFill);

    // 无填充复选框
    QCheckBox noFillCheckbox("No fill");
    noFillCheckbox.setChecked(noFill);

    // 透明度滑块
    QSlider alphaSlider(Qt::Horizontal);
    alphaSlider.setRange(0, 255);
    alphaSlider.setValue(noFill ? 255 : m_initialBrush.color().alpha());
    alphaSlider.setEnabled(!noFill);

    // 按钮组
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout.addRow("Color:", &colorButton);
    layout.addRow(&noFillCheckbox);
    layout.addRow("Opacity:", &alphaSlider);
    layout.addRow(&buttons);

    // 复选框事件
    connect(&noFillCheckbox, &QCheckBox::stateChanged, [&](int state) {
        bool enabled = (state == Qt::Unchecked);
        colorButton.setEnabled(enabled);
        alphaSlider.setEnabled(enabled);
        if (enabled) {
            QColor color = m_initialBrush.style() == Qt::NoBrush ? Qt::white : m_initialBrush.color();
            colorButton.setStyleSheet(QString("background-color: %1").arg(color.name()));
        }
        else {
            colorButton.setStyleSheet("");
        }
        });

    // 颜色选择
    connect(&colorButton, &QPushButton::clicked, [&]() {
        QColor newColor = QColorDialog::getColor(
            m_initialBrush.color(), &dialog, "Select Fill Color", QColorDialog::ShowAlphaChannel);
        if (newColor.isValid()) {
            m_initialBrush.setColor(newColor);
            colorButton.setStyleSheet(QString("background-color: %1").arg(newColor.name()));
            alphaSlider.setValue(newColor.alpha());
        }
        });

    // 确认保存
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        if (noFillCheckbox.isChecked()) {
            m_initialBrush = QBrush(Qt::NoBrush);
        }
        else {
            QColor color = m_initialBrush.color();
            color.setAlpha(alphaSlider.value());
            m_initialBrush = QBrush(color);
        }
    }
}

void MainWindow::toggleGrid(bool show)
{
    canvasWidget->setGridVisible(show);
}

void MainWindow::newCanvas()
{
    bool ok;
    int width = QInputDialog::getInt(this, "New Canvas", "Width:", 800, 100, 5000, 10, &ok);
    if (!ok) return;

    int height = QInputDialog::getInt(this, "New Canvas", "Height:", 600, 100, 5000, 10, &ok);
    if (!ok) return;

    canvasWidget->createNewCanvas(width, height);
}

void MainWindow::saveCanvas()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Save Flowchart",
        "",
        "Flowchart Files (*.flow)"
    );

    if (!fileName.isEmpty()) {
        // 确保文件扩展名正确
        if (!fileName.endsWith(".flow", Qt::CaseInsensitive)) {
            fileName += ".flow";
        }

        // 检查文件是否可写
        QFile file(fileName);
        if (file.exists() && !QFileInfo(fileName).isWritable()) {
            QMessageBox::warning(this, "Error", "File is read-only");
            return;
        }

        if (canvasWidget->saveToFile(fileName)) {
            statusBar()->showMessage("File saved successfully", 2000);
        }
        else {
            QMessageBox::warning(this, "Error",
                QString("Failed to save file.\n"
                    "Please check:\n"
                    "1. File path is valid\n"
                    "2. You have write permission\n"
                    "3. Disk has enough space"));
        }
    }
}

void MainWindow::loadCanvas()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Load Flowchart", "", "Flowchart Files (*.flow)");
    if (!fileName.isEmpty()) {
        if (!canvasWidget->loadFromFile(fileName)) {
            QMessageBox::warning(this, "Error", "Failed to load file");
        }
    }
}

void MainWindow::setSelectMode() {
    canvasWidget->setEditorState(SelectState);
    // 可以更新UI状态，比如按钮高亮等
}

void MainWindow::saveAsPng() {
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Save Flowchart as PNG",
        "",
        "PNG Images (*.png)"
    );

    if (!fileName.isEmpty()) {
        // 确保文件扩展名正确
        if (!fileName.endsWith(".png", Qt::CaseInsensitive)) {
            fileName += ".png";
        }

        // 获取画布widget的尺寸
        QSize canvasSize = canvasWidget->size();

        // 创建与画布相同大小的QPixmap
        QPixmap pixmap(canvasSize);
        pixmap.fill(Qt::white);  // 白色背景

        // 使用QPainter将画布内容绘制到QPixmap
        QPainter painter(&pixmap);
        canvasWidget->render(&painter);
        painter.end();

        // 保存为PNG文件
        if (pixmap.save(fileName, "PNG")) {
            statusBar()->showMessage("PNG saved successfully", 2000);
        }
        else {
            QMessageBox::warning(this, "Error",
                "Failed to save PNG file.\n"
                "Possible reasons:\n"
                "1. Invalid file path\n"
                "2. No write permission\n"
                "3. Disk full");
        }
    }
}

void MainWindow::newCanvasWithSetup()
{
    CanvasSetupDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        // 创建新画布
        canvasWidget->createNewCanvas(dialog.getCanvasSize().width(),
            dialog.getCanvasSize().height());
        // 设置画布背景色
        canvasWidget->setCanvasColor(dialog.getCanvasColor());
    }
}