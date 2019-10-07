
#ifndef PNG__HPP
#define PNG__HPP

#include <QByteArray>
#include <QMap>
#include <QColor>

/* because I don't want to write static constexpr char * IEND = (char *) "IEND"
 * each time there's a valid chunk_type
 * so we use three macros:
 *      the arguments of a generic chunk reader
 *      a way to include labels and the functions associated with them
 *      and a way to put the reader and the label into a dictionary */
#define PNG_READER_ARGS uint32_t, QByteArray &
#define PNG_LABEL(label) static constexpr char * label = (char *) #label; \
    bool label ## Reader(PNG_READER_ARGS);
#define PNG_INSERT_LABEL(label) \
    map.insert(QByteArray(PNG::label), &PNG::label ## Reader)

uint8_t paeth_predictor(uint8_t a, uint8_t b, uint8_t c);

class PNG
{
    typedef bool (PNG::*chunk_reader)(PNG_READER_ARGS);

    public:
        PNG_LABEL(IHDR)
        PNG_LABEL(sRGB)
        PNG_LABEL(sBIT)
        PNG_LABEL(IDAT)
        PNG_LABEL(IEND)

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
        bool change_bytes(uint16_t scanline, QByteArray differences);
        bool change_segment(uint scanline, uint pos, QByteArray differences);


        QColor getPixel(int x, int y);
};


#endif  // PNG__HPP
