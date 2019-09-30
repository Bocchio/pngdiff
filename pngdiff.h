
/*
    This is a comment, I love comments
*/

#ifndef IMG_PNGDIFF__HPP
#define IMG_PNGDIFF__HPP

#include <QImageIOPlugin>

#define PNGDIFF_SIGNATURE_SIZE 7
#define PNGDIFF_SIGNATURE "pngdiff"
#define PNGDIFF_FORMAT_KEY "pngdiff"

class PNGDIFFHandler : public QImageIOHandler
{
public:
    PNGDIFFHandler();

    bool canRead() const override;
    bool read(QImage *outImage) override;

    static bool canRead(QIODevice *device);
};

class PNGDIFFPlugin : public QImageIOPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QImageIOHandlerFactoryInterface" FILE "pngdiff.json")

public:
    Capabilities capabilities(QIODevice *device, const QByteArray &format) const override;
    QImageIOHandler *create(QIODevice *device, const QByteArray &format = QByteArray()) const override;
};

#endif // PNGDIFF__HPP
