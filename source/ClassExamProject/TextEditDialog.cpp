#include "TextEditDialog.h"
#include <QVBoxLayout>
#include <QDialogButtonBox>
TextEditDialog::TextEditDialog(QWidget* parent)
    : QDialog(parent), textColor(Qt::black) {

    // 主文本编辑框
    textEdit = new QTextEdit(this);

    // 工具栏
    toolbar = new QToolBar(this);

    // 加粗按钮
    QAction* boldAction = toolbar->addAction("B");
    boldAction->setCheckable(true);
    boldAction->setShortcut(QKeySequence::Bold);
    connect(boldAction, &QAction::triggered, this, &TextEditDialog::onBoldClicked);

    // 斜体按钮
    QAction* italicAction = toolbar->addAction("I");
    italicAction->setCheckable(true);
    italicAction->setShortcut(QKeySequence::Italic);
    connect(italicAction, &QAction::triggered, this, &TextEditDialog::onItalicClicked);

    // 颜色按钮
    QAction* colorAction = toolbar->addAction("Color");
    connect(colorAction, &QAction::triggered, this, &TextEditDialog::onColorClicked);

    // 字体选择
    fontCombo = new QFontComboBox(this);
    toolbar->addWidget(fontCombo);
    connect(fontCombo, &QFontComboBox::currentFontChanged, this, &TextEditDialog::updateFormat);

    // 字号选择
    sizeCombo = new QComboBox(this);
    sizeCombo->addItems({ "8", "10", "12", "14", "18", "24", "36" });
    toolbar->addWidget(sizeCombo);
    connect(sizeCombo, &QComboBox::currentTextChanged, this, &TextEditDialog::updateFormat);

    // 布局
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(toolbar);
    layout->addWidget(textEdit);

    // 对话框按钮
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void TextEditDialog::setText(const QString& text) {
    textEdit->setHtml(text);
}

QString TextEditDialog::getText() const {
    QString html = textEdit->toHtml();

    // 移除DOCTYPE和<html>前的内容
    html.remove(QRegularExpression("^.*?<html[^>]*>", QRegularExpression::DotMatchesEverythingOption));

    // 移除<head>及其中内容
    html.remove(QRegularExpression("<head[^>]*>.*?</head>", QRegularExpression::DotMatchesEverythingOption));

    // 提取<body>中的内容
    int bodyStart = html.indexOf("<body");
    bodyStart = html.indexOf(">", bodyStart) + 1;
    int bodyEnd = html.lastIndexOf("</body>");
    html = html.mid(bodyStart, bodyEnd - bodyStart).trimmed();

    // 移除内联样式、class、注释
    html.replace(QRegularExpression("style=\"((?!color:)[^\"])*\""), ""); // 保留 color
    html.remove(QRegularExpression("class=\"[^\"]*\""));
    html.remove(QRegularExpression("<!--.*?-->", QRegularExpression::DotMatchesEverythingOption));

    // 简化空标签
    html.replace("<p></p>", "<br>");
    html.replace("<span></span>", "");

    return html;
}

void TextEditDialog::onBoldClicked() {
    textEdit->setFontWeight(textEdit->fontWeight() == QFont::Bold ? QFont::Normal : QFont::Bold);
}

void TextEditDialog::onItalicClicked() {
    textEdit->setFontItalic(!textEdit->fontItalic());
}

void TextEditDialog::onColorClicked() {
    QColor color = QColorDialog::getColor(textColor, this);
    if (color.isValid()) {
        textColor = color;
        updateFormat();
    }
}

void TextEditDialog::updateFormat() {
    QTextCharFormat format;
    format.setFont(fontCombo->currentFont());
    format.setFontPointSize(sizeCombo->currentText().toInt());
    format.setForeground(QBrush(textColor));

    // 保持加粗/斜体状态
    format.setFontWeight(textEdit->fontWeight());
    format.setFontItalic(textEdit->fontItalic());

    textEdit->mergeCurrentCharFormat(format);
}

QFont TextEditDialog::getFont() const {
    QFont font = fontCombo->currentFont();
    font.setPointSize(sizeCombo->currentText().toInt());
    font.setBold(textEdit->fontWeight() == QFont::Bold);
    font.setItalic(textEdit->fontItalic());
    return font;
}

QColor TextEditDialog::getColor() const {
    return textColor;
}