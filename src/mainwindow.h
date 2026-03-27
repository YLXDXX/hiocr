#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include "settingsmanager.h" // 包含完整定义

class ImageViewWidget;
class MarkdownRenderer;
class MarkdownSourceEditor;
class PromptBar;
class QShortcut;
class MarkdownCopyBar;
class QAction;
class QComboBox;
class QPushButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setImage(const QImage& image);
    void setRecognitionResult(const QString& markdown);
    void showError(const QString& message);
    void setBusy(bool busy);
    void setPrompt(const QString& prompt);

    // 【新增】设置当前活动的提示词集合
    void setCurrentPrompts(const QString& text, const QString& formula, const QString& table);

    void startAreaSelection(const QImage& fullImage);
    void copyToClipboard(const QString& text);

public slots:
    void updateServiceSelector(const QList<ServiceProfile>& profiles, const QString& currentId);
    void updateServiceControlButton(const QString& id, bool isRunning);
    void updateStopAllAction(int runningCount);
    void onExternalProcessTriggered();

signals:
    void recognizeRequested(const QString& prompt, const QString& base64Image);
    void imagePasted(const QImage& image);
    void areaSelected(const QRect& rect);
    void settingsTriggered();
    void screenshotRequested();
    void stopServiceRequested();

    void serviceSelected(const QString& id);
    void serviceToggleRequested(const QString& id);

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

    ImageViewWidget* m_imageView;
    MarkdownRenderer* m_markdownRenderer;
    MarkdownSourceEditor* m_markdownSource;
    PromptBar* m_promptBar;
    QShortcut* m_pasteShortcut;
    MarkdownCopyBar* m_copyBar;

    QComboBox* m_serviceSelector = nullptr;
    QPushButton* m_serviceToggleBtn = nullptr;
    QAction* m_stopAllServicesAction = nullptr;
};

#endif // MAINWINDOW_H
