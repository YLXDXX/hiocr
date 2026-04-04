// src/mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include "settingsmanager.h"
#include "copyprocessor.h" // 【修改】包含 ContentType 定义

// 【修改】添加前置声明
class QCheckBox;
class ImageViewWidget;
class MarkdownRenderer;
class MarkdownSourceEditor; // 前置声明
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

    void setCurrentPrompts(const QString& text, const QString& formula, const QString& table);

    void startAreaSelection(const QImage& fullImage);

    // 【修改】移除内联实现，改为仅声明，实现在 .cpp 中
    QString currentMarkdownSource() const;

    bool hasImage() const;

public slots:
    void updateServiceSelector(const QList<ServiceProfile>& profiles, const QString& currentId);
    void updateServiceControlButton(const QString& id, bool isRunning);
    void updateStopAllAction(int runningCount);

    void updateScriptCheckboxes();
    void setRecognizeType(ContentType type);

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

    // 【新增】手动处理请求信号
    void manualProcessRequested(ContentType type);

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
    MarkdownSourceEditor* m_markdownSource; // 指针成员
    PromptBar* m_promptBar;
    QShortcut* m_pasteShortcut;
    MarkdownCopyBar* m_copyBar;

    QComboBox* m_serviceSelector = nullptr;
    QPushButton* m_serviceToggleBtn = nullptr;
    QAction* m_stopAllServicesAction = nullptr;

    // 脚本处理控件
    QCheckBox* m_scriptGlobalCheck = nullptr;
    QCheckBox* m_scriptTextCheck = nullptr;
    QCheckBox* m_scriptFormulaCheck = nullptr;
    QCheckBox* m_scriptTableCheck = nullptr;
    QCheckBox* m_scriptPureMathCheck = nullptr;
};

#endif // MAINWINDOW_H
