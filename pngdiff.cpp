
/*
PNG DIFF file format
*/

#include "pngdiff.h"
#include "crc_utils.h"

#include <iostream>

#include <QFile>
#include <QImage>
#include <QDataStream>
#include <QDebug>
#include <QImageIOPlugin>

#include <QtEndian>
#include <QMap>

PNGDIFF::PNGDIFF() {
    chunkmap = initChunkMap();
}

QMap<QByteArray, PNGDIFF::chunk_reader>
PNGDIFF::initChunkMap() {
    QMap<QByteArray, PNGDIFF::chunk_reader> map;
    PNGDIFF_INSERT_LABEL(path);
    PNGDIFF_INSERT_LABEL(itra);
    PNGDIFF_INSERT_LABEL(styp);
    PNGDIFF_INSERT_LABEL(idat);
    return map;
}

bool PNGDIFF::pathReader(uint32_t length, QByteArray data)
{
    Q_UNUSED(length);

    QFile original_file(data);
    original_file.open(QIODevice::ReadOnly);
    original_image_data = original_file.readAll();
    original_file.close();
    original_image.process(original_image_data);

    return true;
}

bool PNGDIFF::itraReader(uint32_t length, QByteArray data)
{
    Q_UNUSED(length); Q_UNUSED(data);
    return true;
}

bool PNGDIFF::stypReader(uint32_t length, QByteArray data)
{
    length = qToBigEndian(length);
    data = QByteArray((const char *) &length, 4) + data;
    scanline_compression_types = qUncompress(data);

    if (scanline_compression_types.size() != original_image.height) {
        qDebug() << scanline_compression_types.size();
        qDebug() << original_image.height;
        qDebug() << "heights differ";
    }

    return true;
}

bool PNGDIFF::idatReader(uint32_t length, QByteArray data)
{
    length = qToBigEndian(length);
    data = QByteArray((const char *) &length, 4) + data;
    data = qUncompress(data);

    uint index = 0;

    for (uint i = 0; i < original_image.height; i++) {
        uint8_t compression_type = scanline_compression_types[i];
        if (compression_type == PNGDIFF_COMPRESSION_TYPE_IGNORE) {
            continue;
        } else if (compression_type == PNGDIFF_COMPRESSION_TYPE_SIMPLE) {
            //uint16_t scanline = qFromBigEndian<quint16>(data.mid(index, 2).data());
            //index += 2;
            uint16_t scanline = i;

            uint32_t length = qFromBigEndian<quint32>(data.mid(index, 4).data());
            index += 4;
            QByteArray differences = data.mid(index, length);
            index += length;
            original_image.change_bytes(scanline, differences);
        } else if (compression_type == PNGDIFF_COMPRESSION_TYPE_CONTINUED) {
            //uint16_t scanline = qFromBigEndian<quint16>(data.mid(index, 2).data());
            //index += 2;
            uint16_t scanline = i;

            uint32_t length = qFromBigEndian<quint32>(data.mid(index, 4).data());
            index += 4;
            QByteArray differences = data.mid(index, length);
            index += length;

            uint diff_index = 0;
            while (diff_index < differences.size()) {
                uint pos = qFromBigEndian<quint16>(differences.mid(diff_index, 2));
                diff_index += 2;
                uint msb = 0x80;
                uint low_bits = msb - 1;
                uint buffer = differences.mid(diff_index, 1)[0];
                diff_index += 1;
                uint byte_length = 0;
                length = buffer & low_bits;
                while (buffer & msb) {
                    buffer = differences.mid(diff_index, 1)[0];
                    diff_index += 1;
                    byte_length += 1;
                    length += (buffer & low_bits) << byte_length*7;
                }
                QByteArray segment_data = differences.mid(diff_index, length);
                diff_index += length;
                original_image.change_segment(scanline, pos, segment_data);
            }
        }
    }

    return true;
}

bool PNGDIFF::process(QByteArray data)
{
    uint index = PNGDIFF_SIGNATURE_SIZE;
    QByteArray signature = data.left(PNGDIFF_SIGNATURE_SIZE);

    while (true) {
        uint32_t length = qFromBigEndian<quint32>(data.mid(index, 4).data());
        index += 4;
        QByteArray chunk_type = data.mid(index, 4);
        index += 4;

        QByteArray chunk_data = data.mid(index, length);
        index += length;
        uint32_t crc = qFromBigEndian<quint32>(data.mid(index, 4).data());
        index += 4;

        if (chunk_type == IEND)
            break;

        qDebug() << chunk_type;

        if (crc != crc32(chunk_type + chunk_data)) {
            qDebug() << chunk_type;
            qDebug() << "CRC failed" << crc << crc32(data, crc32(data));
            return false;
        }

        if (!chunkmap.contains(chunk_type)) {
            qDebug() << chunk_type;
            qDebug() << "Unrecognized chunk type";
            return false;
        }

        PNGDIFF::chunk_reader reader = chunkmap[chunk_type];
        if(!(this->*reader)(length, chunk_data)){
            qDebug() << chunk_type;
            qDebug() << "error reading chunk";
        }
    }

    return false;
}

PNGDIFFHandler::PNGDIFFHandler()
{
}

bool PNGDIFFHandler::read(QImage *outImage)
{
    PNGDIFF pngdiff;
    pngdiff.process(device()->readAll());

    try {
        int width = pngdiff.original_image.width;
        int height = pngdiff.original_image.height;

        QImage image(width, height, QImage::Format_ARGB32);
        if (image.isNull()) {
            return false;
        }

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                image.setPixelColor(x, y, pngdiff.original_image.getPixel(x, y));
            }
        }

        *outImage = image;

        return true;
    } catch (const std::exception &exc) {
        // qDebug() << exc.what();
        return false;
    }
}

bool PNGDIFFHandler::canRead() const
{
    if (canRead(device())) {
        setFormat(PNGDIFF_FORMAT_KEY);
        return true;
    }
    return false;
}

bool PNGDIFFHandler::canRead(QIODevice *device)
{
    if (!device) {
        qWarning("PNGDIFFHandler::canRead() called with no device");
        return false;
    }

    const QByteArray head = device->peek(PNGDIFF_SIGNATURE_SIZE);
    if (head == PNGDIFF_SIGNATURE) {
        return true;
    }
    return false;
}

QImageIOPlugin::Capabilities PNGDIFFPlugin::capabilities(QIODevice *device, const QByteArray &format) const
{
    if (format == PNGDIFF_FORMAT_KEY) {
        return Capabilities(CanRead);
    }
    if (!format.isEmpty()) {
        return {};
    }
    if (!device->isOpen()) {
        return {};
    }

    Capabilities cap;
    if (device->isReadable() && PNGDIFFHandler::canRead(device)) {
        cap |= CanRead;
    }
    return cap;
}

QImageIOHandler *PNGDIFFPlugin::create(QIODevice *device, const QByteArray &format) const
{
    QImageIOHandler *handler = new PNGDIFFHandler;
    handler->setDevice(device);
    handler->setFormat(format);
    return handler;
}
