#include <QDialog>
#include <QTextEdit>
#include <QToolBar>
#include <QFontComboBox>
#include <QComboBox>
#include <QColorDialog>
#include <QTextBrowser>

class TextEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit TextEditDialog(QWidget* parent = nullptr);
    void setText(const QString& text);
    QString getText() const;
    QFont getFont() const;
    QColor getColor() const;

private slots:
    void onBoldClicked();
    void onItalicClicked();
    void onColorClicked();
    void updateFormat();

private:
    QTextEdit* textEdit;
    QTextBrowser* browser;
    QToolBar* toolbar;
    QFontComboBox* fontCombo;
    QComboBox* sizeCombo;
    QColor textColor;
};
