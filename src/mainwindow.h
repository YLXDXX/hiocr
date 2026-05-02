// src/mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QSplitter>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QInputDialog>
#include <QLineEdit>
#include <QKeyEvent>
#include <QStatusBar>
#include <QLabel>
#include <QComboBox>
#include <QWidgetAction>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QToolBar>
#include <QToolButton>
#include <QDialog>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QSpinBox>
#include "settingsmanager.h"
#include "copyprocessor.h"
#include "imageviewwidget.h"
#include "markdownrenderer.h"
#include "markdownsourceeditor.h"
#include "promptbar.h"
#include "imageprocessor.h"
#include "screenshotareaselector.h"
#include "markdowncopybar.h"
#include "historymanager.h"
#include "constants.h"

class QShortcut;

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

    void setCurrentServiceName(const QString& name);
    void setCopyProcessor(CopyProcessor* processor);

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
    QCheckBox* m_formatterToolCheck = nullptr;

    bool m_isStreaming = false;
    QAction* m_abortAction = nullptr;
};

#include "mainwindow_impl.h"

#endif // MAINWINDOW_H
