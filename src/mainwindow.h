#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class ImageViewWidget;
class MarkdownRenderer;
class MarkdownSourceEditor;
class PromptBar;
class QShortcut;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // UI 公共接口
    void setImage(const QImage& image);
    void setRecognitionResult(const QString& markdown);
    void showError(const QString& message);
    void setBusy(bool busy);
    void setPrompt(const QString& prompt); // 【新增】设置提示词
    void startAreaSelection(const QImage& fullImage);

signals:
    // 用户意图信号
    void recognizeRequested(const QString& prompt, const QString& base64Image);
    void imagePasted(const QImage& image);
    void areaSelected(const QRect& rect);
    void settingsTriggered();
    void screenshotRequested(); // 【新增】菜单栏截图请求

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onPasteImage();
    void onMarkdownSourceChanged();

    // 【新增】处理 PromptBar 的信号
    void onPromptBarRecognize();
    void onPromptBarAutoRecognize(const QString& prompt);

private:
    void setupUi();
    void setupConnections();
    void setupMenuBar(); // 【新增】独立出菜单构建函数

    // UI 组件
    ImageViewWidget* m_imageView;
    MarkdownRenderer* m_markdownRenderer;
    MarkdownSourceEditor* m_markdownSource;
    PromptBar* m_promptBar;
    QShortcut* m_pasteShortcut;
};

#endif // MAINWINDOW_H
