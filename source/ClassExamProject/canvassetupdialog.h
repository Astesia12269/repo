#ifndef CANVASSETUPDIALOG_H
#define CANVASSETUPDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QButtonGroup>
#include <QSize>
#include <QLabel>

class CanvasSetupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CanvasSetupDialog(QWidget* parent = nullptr);
    QSize getCanvasSize() const { return m_canvasSize; }
    QColor getCanvasColor() const { return m_canvasColor; }

private slots:
    void onSizeButtonClicked(int id);
    void onColorButtonClicked();

private:
    void setupSizeButtons();
    void setupColorButtons();
    void paintPaperPreview(QLabel* label, const QSize& size, bool isCustom = false);

    QSize m_canvasSize;
    QColor m_canvasColor;
    QButtonGroup* m_sizeGroup;
};

#endif // CANVASSETUPDIALOG_H