
/*
PNG DIFF file format
*/

#include "pngdiff.h"
#include "png.h"

#include <iostream>

#include <QFile>
#include <QImage>
#include <QDataStream>
#include <QDebug>
#include <QImageIOPlugin>

#include <QtEndian>
#include <QMap>


PNGDIFFHandler::PNGDIFFHandler()
{
}

bool PNGDIFFHandler::read(QImage *outImage)
{
    QByteArray signature = device()->read(PNGDIFF_SIGNATURE_SIZE);
    uint16_t path_length = qFromBigEndian<quint16>(device()->read(2).data());
    QByteArray path = device()->read(path_length);
    QFile archivo_referenciado(path);

    archivo_referenciado.open(QIODevice::ReadOnly);
    QByteArray image_data = archivo_referenciado.readAll();
    archivo_referenciado.close();

    PNG imagen_leida;
    imagen_leida.process(image_data);

    QByteArray diff_data = device()->readAll();
    diff_data = qUncompress(diff_data);

    uint index = 0;

    for (uint i = 0; i < imagen_leida.height; i++) {
        uint16_t scanline = qFromBigEndian<quint16>(diff_data.mid(index, 2).data());
        index += 2;
        uint32_t length = qFromBigEndian<quint32>(diff_data.mid(index, 4).data());
        index += 4;
        QByteArray differences = diff_data.mid(index, length);
        index += length;

        for (int j = 0; j < differences.size(); j += 3) {
            uint32_t pos = qFromBigEndian<quint16>(differences.mid(j, 2).data());
            uint8_t val = differences[j+2];
            imagen_leida.change_byte(scanline, pos, val);
        }
    }

    try {
        int width = imagen_leida.width;
        int height = imagen_leida.height;

        QImage image(width, height, QImage::Format_ARGB32);
        if (image.isNull()) {
            return false;
        }

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                image.setPixelColor(x, y, imagen_leida.getPixel(x, y));
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
