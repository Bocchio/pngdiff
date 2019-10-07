
/*
    This is a comment, I love comments
*/

#ifndef IMG_PNGDIFF__HPP
#define IMG_PNGDIFF__HPP

#include "png.h"

#include <QImageIOPlugin>
#include <QMap>

#define PNGDIFF_SIGNATURE_SIZE 7
#define PNGDIFF_SIGNATURE "pngdiff"
#define PNGDIFF_FORMAT_KEY "pngdiff"

// I want to reduce the refactoring as much as possible
#define PNGDIFF_READER_ARGS uint32_t, QByteArray
#define PNGDIFF_LABEL(label) static constexpr char * label = (char *) #label; \
    bool label ## Reader(PNGDIFF_READER_ARGS);
#define PNGDIFF_INSERT_LABEL(label) \
    map.insert(QByteArray(PNGDIFF::label), &PNGDIFF::label ## Reader)

#define PNGDIFF_COMPRESSION_TYPE_IGNORE     0x00
#define PNGDIFF_COMPRESSION_TYPE_SIMPLE     0x01
#define PNGDIFF_COMPRESSION_TYPE_CONTINUED  0x02

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


class PNGDIFF
{
    typedef bool (PNGDIFF::*chunk_reader)(PNGDIFF_READER_ARGS);

  public:
    PNGDIFF_LABEL(path)  // path
    PNGDIFF_LABEL(itra)  // image transforms
    PNGDIFF_LABEL(styp)  // scanline compression type
    PNGDIFF_LABEL(idat)  // image data
    PNGDIFF_LABEL(IEND)  // image data

    QMap<QByteArray, chunk_reader> chunkmap;
    QByteArray original_image_data;
    PNG original_image;
    QByteArray scanline_compression_types;

    PNGDIFF();
    QMap<QByteArray, chunk_reader> initChunkMap();

    bool process(QByteArray data);
};


#endif // PNGDIFF__HPP
