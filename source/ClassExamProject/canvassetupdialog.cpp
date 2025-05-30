#include "canvassetupdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPainter>
#include <QColorDialog>
#include <QLabel>
#include <QInputDialog>
#include <QDialogButtonBox>

CanvasSetupDialog::CanvasSetupDialog(QWidget* parent)
    : QDialog(parent), m_canvasSize(1050, 1500), m_canvasColor(Qt::white)
{
    setWindowTitle("Canvas Setup");
    resize(600, 500); // �����Ի����С

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // ����������
    QHBoxLayout* contentLayout = new QHBoxLayout;
    contentLayout->setSpacing(30);

    // ��� - ֽ�ųߴ�ѡ�� (2x2����)
    QWidget* sizeWidget = new QWidget;
    QGridLayout* sizeLayout = new QGridLayout(sizeWidget);
    sizeLayout->setSpacing(20);
    sizeLayout->setContentsMargins(10, 10, 10, 10);

    m_sizeGroup = new QButtonGroup(this);

    // ֽ�ųߴ綨�� (����)
    QList<QPair<QString, QSize>> paperSizes = {
        {"A3 (1500*2100px)", QSize(1500, 2100)},
        {"A4 (1050*1500px)", QSize(1050, 1500)},
        {"A5 (750*1050px)", QSize(750, 1050)},
        {"Custom", QSize(0, 0)}
    };

    for (int i = 0; i < paperSizes.size(); ++i) {
        QPushButton* btn = new QPushButton;
        btn->setFixedSize(150, 150); // ͳһ��ť��С

        QVBoxLayout* btnLayout = new QVBoxLayout(btn);
        btnLayout->setAlignment(Qt::AlignCenter);
        btnLayout->setSpacing(5);

        // ���Ԥ��ͼ
        QLabel* previewLabel = new QLabel;
        previewLabel->setFixedSize(100, 100);
        paintPaperPreview(previewLabel, paperSizes[i].second, i == 3);
        btnLayout->addWidget(previewLabel);

        // �������˵��
        QLabel* textLabel = new QLabel(paperSizes[i].first);
        textLabel->setAlignment(Qt::AlignCenter);
        btnLayout->addWidget(textLabel);

        m_sizeGroup->addButton(btn, i);
        sizeLayout->addWidget(btn, i / 2, i % 2); // 2x2���񲼾�
    }

    contentLayout->addWidget(sizeWidget);

    // �Ҳ� - ��ɫѡ��
    QWidget* colorWidget = new QWidget;
    QGridLayout* colorLayout = new QGridLayout(colorWidget);
    colorLayout->setSpacing(15);
    colorLayout->setContentsMargins(10, 10, 10, 10);

    colorLayout->addWidget(new QLabel("Canvas Color:"), 0, 0, 1, 2);

    QList<QColor> presetColors = {
        Qt::white, Qt::lightGray, Qt::yellow, Qt::cyan,
        Qt::green, Qt::magenta, Qt::blue, Qt::black
    };

    // 7����ɫ��ť + 1��"..."��ť
    for (int i = 0; i < 7; ++i) {
        QPushButton* colorBtn = new QPushButton;
        colorBtn->setFixedSize(50, 50);
        colorBtn->setStyleSheet(QString(
            "background-color: %1; border: 1px solid gray;"
        ).arg(presetColors[i].name()));

        colorLayout->addWidget(colorBtn, i / 2 + 1, i % 2);
        connect(colorBtn, &QPushButton::clicked, [this, color = presetColors[i]]() {
            m_canvasColor = color;
        });
    }

    // ���"..."��ť
    QPushButton* moreColorBtn = new QPushButton("...");
    moreColorBtn->setFixedSize(50, 50);
    moreColorBtn->setStyleSheet("font-size: 20px;");
    colorLayout->addWidget(moreColorBtn, 4, 0, 1, 2);
    connect(moreColorBtn, &QPushButton::clicked, this, &CanvasSetupDialog::onColorButtonClicked);

    contentLayout->addWidget(colorWidget);
    mainLayout->addLayout(contentLayout);

    // ���ȷ��/ȡ����ť
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    // �����źŲ�
    connect(m_sizeGroup, &QButtonGroup::idClicked,
        this, &CanvasSetupDialog::onSizeButtonClicked);
}

    void CanvasSetupDialog::paintPaperPreview(QLabel* label, const QSize& size, bool isCustom)
    {
        QPixmap pixmap(label->size());
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);

        // ����ֽ����ӰЧ��
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(200, 200, 200, 150));
        painter.drawRoundedRect(5, 5, pixmap.width() - 5, pixmap.height() - 5, 5, 5);

        // ����ֽ��
        painter.setBrush(Qt::white);
        painter.setPen(QPen(Qt::black, 1));
        painter.drawRoundedRect(0, 0, pixmap.width() - 5, pixmap.height() - 5, 5, 5);

        if (isCustom) {
            // �Զ���ֽ�Ż����ʺ�
            painter.setFont(QFont("Arial", 24, QFont::Bold));
            painter.drawText(pixmap.rect(), Qt::AlignCenter, "?");
        }
        else {
            // ���Ʊ������ŵ�ֽ��
            QRect paperRect(15, 15, pixmap.width() - 30, pixmap.height() - 30);
            if (size.width() > size.height()) {
                paperRect.setHeight(paperRect.width() * size.height() / size.width());
            }
            else {
                paperRect.setWidth(paperRect.height() * size.width() / size.height());
            }
            paperRect.moveCenter(pixmap.rect().center());

            painter.setBrush(Qt::white);
            painter.setPen(QPen(Qt::black, 1));
            painter.drawRect(paperRect);
        }

        label->setPixmap(pixmap);
    }

void CanvasSetupDialog::onSizeButtonClicked(int id)
{
    switch (id) {
    case 0: m_canvasSize = QSize(1500, 2100); break;  // A3
    case 1: m_canvasSize = QSize(1050, 1500); break;  // A4
    case 2: m_canvasSize = QSize(750, 1050); break;   // A5
    case 3: {
        // �Զ���ߴ�
        bool ok;
        int width = QInputDialog::getInt(this, "Custom Size", "Width (px):",
            1050, 100, 5000, 10, &ok);
        if (!ok) return;

        int height = QInputDialog::getInt(this, "Custom Size", "Height (px):",
            1500, 100, 5000, 10, &ok);
        if (!ok) return;

        m_canvasSize = QSize(width, height);
        break;
    }
    }
}

void CanvasSetupDialog::onColorButtonClicked()
{
    QColor color = QColorDialog::getColor(m_canvasColor, this, "Select Canvas Color");
    if (color.isValid()) {
        m_canvasColor = color;
    }
}