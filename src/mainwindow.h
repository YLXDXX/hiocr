// src/mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include "settingsmanager.h"
#include "copyprocessor.h"

class QCheckBox;
class ImageViewWidget;
class MarkdownRenderer;
class MarkdownSourceEditor;
class PromptBar;
class QShortcut;
class MarkdownCopyBar;
class QAction;
class QComboBox;
class QPushButton;
class QToolBar;
class QToolButton;

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

    void setCurrentPrompts(const QString& text, const QString& formula, const QString& table);

    // 【新增】设置当前服务名称，同步到渲染器和复制栏
    void setCurrentServiceName(const QString& name);

    void startAreaSelection(const QImage& fullImage);

    QString currentMarkdownSource() const;
    bool hasImage() const;
    QImage currentImage() const;

public slots:
    void updateServiceSelector(const QList<ServiceProfile>& profiles, const QString& currentId);
    void updateServiceControlButton(const QString& id, bool isRunning);
    void updateStopAllAction(int runningCount);

    void updateScriptCheckboxes();
    void setRecognizeType(ContentType type);

    void onCopyCurrentImage();
    void updateCopyImageActionState();

    void appendRecognitionResult(const QString& delta);
    void setStreamingMode(bool streaming);
    void showHistoryDialog();

signals:
    void recognizeRequested(const QString& prompt, const QString& base64Image);
    void typedRecognizeRequested(const QString& prompt, const QString& base64Image, ContentType type);
    void autoRecognizeRequested(const QString& prompt, ContentType type);
    void imagePasted(const QImage& image);
    void areaSelected(const QRect& rect);
    void settingsTriggered();
    void screenshotRequested();
    void stopServiceRequested();

    void serviceSelected(const QString& id);
    void serviceToggleRequested(const QString& id);

    void manualProcessRequested(ContentType type);
    void loadHistoryRecordRequested(int recordId);

    void abortRequested();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onPasteImage();
    void onMarkdownSourceChanged();
    void onPromptBarRecognize();
    void onPromptBarAutoRecognize(const QString& prompt);
    void onManualProcessorTriggered(ContentType type);

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

    QToolBar* m_mainToolBar = nullptr;
    QComboBox* m_serviceSelector = nullptr;
    QPushButton* m_serviceToggleBtn = nullptr;
    QAction* m_stopAllServicesAction = nullptr;
    QAction* m_copyImageAction = nullptr;
    QAction* m_viewHistoryAction = nullptr;

    QCheckBox* m_scriptGlobalCheck = nullptr;
    QCheckBox* m_scriptTextCheck = nullptr;
    QCheckBox* m_scriptFormulaCheck = nullptr;
    QCheckBox* m_scriptTableCheck = nullptr;
    QCheckBox* m_scriptPureMathCheck = nullptr;

    bool m_isStreaming = false;
    QAction* m_abortAction = nullptr;
};

#endif // MAINWINDOW_H
