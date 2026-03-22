#include "imageprocessor.h"
#include <QFile>
#include <QByteArray>
#include <QBuffer>
#include <QImageReader>
#include <QDebug>

QImage ImageProcessor::loadImage(const QString& filePath) {
    QImageReader reader(filePath);
    if (!reader.canRead()) {
        qWarning() << "无法读取图片：" << filePath;
        return QImage();
    }
    return reader.read();
}

QString ImageProcessor::fileToBase64(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件：" << filePath;
        return QString();
    }
    QByteArray data = file.readAll();
    return data.toBase64();
}

QString ImageProcessor::imageToBase64(const QImage& image) {
    if (image.isNull()) return QString();
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return bytes.toBase64();
}
