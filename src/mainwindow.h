#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>

class ImageViewWidget;
class MarkdownRenderer;
class MarkdownSourceEditor;
class PromptBar;
class QShortcut;
class MarkdownCopyBar;

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
    void setPrompt(const QString& prompt);
    void startAreaSelection(const QImage& fullImage);

signals:
    void recognizeRequested(const QString& prompt, const QString& base64Image);
    void imagePasted(const QImage& image);
    void areaSelected(const QRect& rect);
    void settingsTriggered();
    void screenshotRequested();

protected:
    // 【新增】重写按键事件
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onPasteImage();
    void onMarkdownSourceChanged();
    void onPromptBarRecognize();
    void onPromptBarAutoRecognize(const QString& prompt);

    void onExternalProcessTriggered(); // 新增槽函数
    void onExternalProcessFinished(int exitCode, QProcess::ExitStatus exitStatus); // 新增

private:
    void setupUi();
    void setupConnections();
    void setupMenuBar();

    // UI 组件
    ImageViewWidget* m_imageView;
    MarkdownRenderer* m_markdownRenderer;
    MarkdownSourceEditor* m_markdownSource;
    PromptBar* m_promptBar;
    QShortcut* m_pasteShortcut;
    MarkdownCopyBar* m_copyBar;
};

#endif // MAINWINDOW_H
