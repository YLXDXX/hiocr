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
class QAction; // 前向声明

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

public slots:
    // 【新增】更新停止服务菜单项的状态
    void updateStopServiceAction(bool isRunning);

    void onExternalProcessTriggered();

signals:
    void recognizeRequested(const QString& prompt, const QString& base64Image);
    void imagePasted(const QImage& image);
    void areaSelected(const QRect& rect);
    void settingsTriggered();
    void screenshotRequested();
    void stopServiceRequested(); // 【新增】停止服务信号

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onPasteImage();
    void onMarkdownSourceChanged();
    void onPromptBarRecognize();
    void onPromptBarAutoRecognize(const QString& prompt);
    void onExternalProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

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

    // 【新增】菜单项
    QAction* m_stopServiceAction = nullptr;
};

#endif // MAINWINDOW_H
