#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <QImage>
#include <QString>

class ImageProcessor {
public:
    // 从文件加载QImage，失败返回空QImage
    static QImage loadImage(const QString& filePath);

    // 将图片文件转换为Base64字符串（PNG格式）
    static QString fileToBase64(const QString& filePath);

    // 将QImage转换为Base64字符串（PNG格式）
    static QString imageToBase64(const QImage& image);
};

#endif // IMAGEPROCESSOR_H
