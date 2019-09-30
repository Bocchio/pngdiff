# pngdiff

PNGDIFF file format

## What's PNGDIFF?

PNGDIFF is a simple file format designed to compress similar png images.

I have a reasonable amount of similar PNG images, mainly because of screenshots. Tried to archive them but since PNG images are somewhat big and already compressed, no compression algorithm reduced the total size of them.

Searched a bit, and couldn't find any already implemented solution.

So I designed a simple file format that has all the information necessary to make a PNG image out of another one.

The implementation comes with simple utils to compress and decompress images.
But more importantly, you can still preview and view these images with Dolphin/Gwenview or any other KDE program.
