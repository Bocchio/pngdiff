

#include <QFile>

#include <kio/thumbcreator.h>

#include <QImage>
#include <QStringList>
#include <QtEndian>

#include <iostream>


class PNGDIFFThumbCreator : public ThumbCreator
{
public:
    bool create(const QString &path, int, int, QImage &img) override;
    Flags flags() const override;

};


extern "C"
{
  Q_DECL_EXPORT ThumbCreator *new_creator()
  {
    return new PNGDIFFThumbCreator();
  }
};


bool PNGDIFFThumbCreator::create(const QString &path, int width, int height, QImage &img)
{
    img.load(path);

    if (img.isNull()) {
        return false;
    }

    return true;
}

PNGDIFFThumbCreator::Flags PNGDIFFThumbCreator::flags() const
{
  return DrawFrame;
}
