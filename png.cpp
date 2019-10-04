
#include "png.h"
#include "crc_utils.h"

#include <QtEndian>
#include <QDebug>

uint8_t paeth_predictor(uint8_t a, uint8_t b, uint8_t c)
{
    int pa = abs(b - c);
    int pb = abs(a - c);
    int pc = abs(a + b - c - c);
    int Pr;
    if ((pa <= pb) && (pa <= pc)) {
        Pr = a;
    } else if (pb <= pc) {
        Pr = b;
    } else {
        Pr = c;
    }
    return (uint8_t) Pr;
}

PNG::PNG() {
    chunkmap = initChunkMap();
}

QMap<QByteArray, PNG::chunk_reader>
PNG::initChunkMap() {
    QMap<QByteArray, PNG::chunk_reader> map;
    PNG_INSERT_LABEL(IHDR);
    PNG_INSERT_LABEL(sRGB);
    PNG_INSERT_LABEL(sBIT);
    PNG_INSERT_LABEL(IDAT);
    return map;
}

bool
PNG::process(QByteArray file_data) {
    uint index = 8;
    QByteArray signature = file_data.left(8);

    while (true) {
        uint32_t length = qFromBigEndian<quint32>(file_data.mid(index, 4).data());
        index += 4;
        QByteArray chunk_type = file_data.mid(index, 4);
        index += 4;
        QByteArray data = file_data.mid(index, length);
        index += length;
        uint32_t crc = qFromBigEndian<quint32>(file_data.mid(index, 4).data());
        index += 4;

        if (chunk_type == IEND)
            break;

        if (crc != crc32(chunk_type + data)) {
            qDebug() << chunk_type;
            qDebug() << "CRC failed" << crc << crc32(data, crc32(data));
            return false;
        }

        if (!chunkmap.contains(chunk_type)) {
            qDebug() << chunk_type;
            return false;
        }

        PNG::chunk_reader reader = chunkmap[chunk_type];
        if(!(this->*reader)(length, data)){
            qDebug() << chunk_type;
            qDebug() << "error reading chunk";
        }
    }

    uint32_t length;
    qToBigEndian<quint32>(image_data.size(), &length);

    qDebug() << "Finished reading";

    image_data = QByteArray((const char *) &length, 4) + image_data;
    image_data = qUncompress(image_data);

    qDebug() << "Finished uncompressing";

    image_data = reconstruct_png(image_data);

    qDebug() << "Finished reconstructing";

    return true;
}

QByteArray
PNG::reconstruct_png(QByteArray &original) {
    uint scanlines_size = width*4;
    uint scanlines_and_filter_size = scanlines_size + 1;
    QByteArray reconstructed;

    for (uint i = 0; i < height; i++) {
        uint row_original = scanlines_and_filter_size * i;
        uint row_reconstructed = scanlines_size * i;
        uint row_up = scanlines_size * (i-1);
        uint8_t filter_type = original[row_original];
        for (uint j = 0; j < scanlines_size; j++) {
            uint8_t x = original[row_original + j + 1];

            uint8_t a = 0;
            uint8_t b = 0;
            uint8_t c = 0;

            if (j >= 4)
                a = (uint8_t) reconstructed[row_reconstructed + j - 4];
            if ((i > 0) && (filter_type > 1))
                b = (uint8_t) reconstructed[row_up + j];
            if ((filter_type == 4) && (i > 0) && (j >= 4))
                c = (uint8_t) reconstructed[row_up + j - 4];

            switch(filter_type) {
                case 1:
                    x += a;
                    break;
                case 2:
                    x += b;
                    break;
                case 3:
                    x += ((a + b)/2);
                    break;
                 case 4:
                    x += paeth_predictor(a, b, c);
                    break;
            }

            reconstructed.append(x);
        }
    }
    return reconstructed;
}


bool
PNG::change_bytes(uint16_t scanline, QByteArray differences) {
    int scanline_offset = width*4*scanline;
    for (int j = 0; j < differences.size(); j += 3) {
        uint32_t pos = qFromBigEndian<quint16>(differences.mid(j, 2).data());
        image_data[scanline_offset + pos] = differences[j+2];
    }

    return true;
}

bool
PNG::change_byte(uint16_t scanline, uint32_t pos, uint8_t byte) {
    int scanlines_size = width*4;
    image_data[scanline*scanlines_size + pos] = byte;

    return true;
}

QColor
PNG::getPixel(int x, int y) {
    int index = width*4*y + x*4;
    uint8_t r = image_data[index    ];
    uint8_t g = image_data[index + 1];
    uint8_t b = image_data[index + 2];
    uint8_t a = image_data[index + 3];

    return QColor(r, g, b, a);
}

bool
PNG::IHDRReader(uint32_t length, QByteArray &data) {
    Q_UNUSED(length);

    width = qFromBigEndian<quint32>(data.mid(0, 4).data());
    height = qFromBigEndian<quint32>(data.mid(4, 4).data());
    bit_depth = *data.mid(8, 1).data();
    colour_type = *data.mid(9, 1).data();
    compression_method = *data.mid(10, 1).data();
    filter_method = *data.mid(11, 1).data();
    interlace_method = *data.mid(12, 1).data();

    if(bit_depth != 8)
        return false;

    if(colour_type != 6)
        return false;

    if(compression_method != 0)
        return false;

    if(interlace_method != 0)
        return false;

    return true;
}

bool
PNG::sRGBReader(uint32_t length, QByteArray &data) {
    Q_UNUSED(length);
    Q_UNUSED(data);

    return true;
}

bool
PNG::sBITReader(uint32_t length, QByteArray &data) {
    Q_UNUSED(length);
    Q_UNUSED(data);

    return true;
}

bool
PNG::IDATReader(uint32_t length, QByteArray &data) {
    Q_UNUSED(length);

    image_data += data;

    return true;
}
