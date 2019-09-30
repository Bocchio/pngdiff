
#ifndef PNG__HPP
#define PNG__HPP

#include <QByteArray>
#include <QMap>
#include <QColor>

/* because I don't want to write static constexpr char * IEND = (char *) "IEND"
 * each time there's a valid chunk_type */
#define PNG_LABEL(label) static constexpr char * label = (char *) #label

uint8_t paeth_predictor(uint8_t a, uint8_t b, uint8_t c);

class PNG
{
    typedef bool (PNG::*chunk_reader)(uint16_t length, QByteArray data);

    public:
        PNG_LABEL(IHDR);
        PNG_LABEL(sRGB);
        PNG_LABEL(sBIT);
        PNG_LABEL(IDAT);
        PNG_LABEL(IEND);

        QMap<QByteArray, chunk_reader> chunkmap;
        uint32_t width;
        uint32_t height;
        uint8_t bit_depth;
        uint8_t colour_type;
        uint8_t compression_method;
        uint8_t filter_method;
        uint8_t interlace_method;

        QByteArray image_data;

        PNG();

        QMap<QByteArray, chunk_reader> initChunkMap();

        bool process(QByteArray file_data);

        QByteArray reconstruct_png(QByteArray &original);

        bool change_byte(uint16_t scanline, uint32_t pos, uint8_t byte);

        QColor getPixel(int x, int y);

        bool ihdrReader(uint16_t length, QByteArray data);
        bool srgbReader(uint16_t length, QByteArray data);
        bool sbitReader(uint16_t length, QByteArray data);
        bool idatReader(uint16_t length, QByteArray data);
};


#endif  // PNG__HPP
