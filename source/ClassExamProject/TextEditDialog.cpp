#include "TextEditDialog.h"
#include <QVBoxLayout>
#include <QDialogButtonBox>
TextEditDialog::TextEditDialog(QWidget* parent)
    : QDialog(parent), textColor(Qt::black) {

    // ���ı��༭��
    textEdit = new QTextEdit(this);

    // ������
    toolbar = new QToolBar(this);

    // �Ӵְ�ť
    QAction* boldAction = toolbar->addAction("B");
    boldAction->setCheckable(true);
    boldAction->setShortcut(QKeySequence::Bold);
    connect(boldAction, &QAction::triggered, this, &TextEditDialog::onBoldClicked);

    // б�尴ť
    QAction* italicAction = toolbar->addAction("I");
    italicAction->setCheckable(true);
    italicAction->setShortcut(QKeySequence::Italic);
    connect(italicAction, &QAction::triggered, this, &TextEditDialog::onItalicClicked);

    // ��ɫ��ť
    QAction* colorAction = toolbar->addAction("Color");
    connect(colorAction, &QAction::triggered, this, &TextEditDialog::onColorClicked);

    // ����ѡ��
    fontCombo = new QFontComboBox(this);
    toolbar->addWidget(fontCombo);
    connect(fontCombo, &QFontComboBox::currentFontChanged, this, &TextEditDialog::updateFormat);

    // �ֺ�ѡ��
    sizeCombo = new QComboBox(this);
    sizeCombo->addItems({ "8", "10", "12", "14", "18", "24", "36" });
    toolbar->addWidget(sizeCombo);
    connect(sizeCombo, &QComboBox::currentTextChanged, this, &TextEditDialog::updateFormat);

    // ����
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(toolbar);
    layout->addWidget(textEdit);

    // �Ի���ť
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

    // �Ƴ�DOCTYPE��<html>ǰ������
    html.remove(QRegularExpression("^.*?<html[^>]*>", QRegularExpression::DotMatchesEverythingOption));

    // �Ƴ�<head>����������
    html.remove(QRegularExpression("<head[^>]*>.*?</head>", QRegularExpression::DotMatchesEverythingOption));

    // ��ȡ<body>�е�����
    int bodyStart = html.indexOf("<body");
    bodyStart = html.indexOf(">", bodyStart) + 1;
    int bodyEnd = html.lastIndexOf("</body>");
    html = html.mid(bodyStart, bodyEnd - bodyStart).trimmed();

    // �Ƴ�������ʽ��class��ע��
    html.replace(QRegularExpression("style=\"((?!color:)[^\"])*\""), ""); // ���� color
    html.remove(QRegularExpression("class=\"[^\"]*\""));
    html.remove(QRegularExpression("<!--.*?-->", QRegularExpression::DotMatchesEverythingOption));

    // �򻯿ձ�ǩ
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

    // ���ּӴ�/б��״̬
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