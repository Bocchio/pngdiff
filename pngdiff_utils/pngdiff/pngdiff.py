
from pngdiff.png import PNG

import logging
import argparse
import os
import zlib

import sys

SIGNATURE = b'pngdiff'

def chunk(chunk_type, data, compress=False):
    chunk_data = bytearray()

    if compress:
        data = zlib.compress(data, level=9)

    length = len(data)
    chunk_data += length.to_bytes(length=4, byteorder='big')
    chunk_data += chunk_type
    chunk_data += data
    crc = zlib.crc32(chunk_type + data)
    chunk_data += crc.to_bytes(length=4, byteorder='big')

    return chunk_data


def write_chunk(f, chunk_type, data, compress=False):
    if compress:
        data = zlib.compress(data, level=9)

    length = len(data)
    print(chunk_type, len(data))
    f.write(length.to_bytes(length=4, byteorder='big'))
    f.write(chunk_type)
    f.write(data)
    crc = zlib.crc32(chunk_type + data)
    f.write(crc.to_bytes(length=4, byteorder='big'))


def compress(base, targets):
    base_image = PNG.from_file(base)
    targets = [*filter(lambda x: x != base, targets)]
    target_images = []
    for filename in targets:
        target_images.append((filename + 'diff', PNG.from_file(filename)))

    path = os.path.join(os.getcwd(), base).encode()
    path_chunk = chunk(chunk_type=b'path', data=path)
    iend_chunk = chunk(chunk_type=b'IEND', data=b'')
    for filename, image in target_images:
        with open(filename, 'wb') as f:
            f.write(SIGNATURE)
            f.write(path_chunk)

            compression_types, difference_data = image.difference(base_image)

            write_chunk(f, b'styp', compression_types, compress=True)
            write_chunk(f, b'idat', difference_data, compress=True)
            f.write(iend_chunk)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('base', help='Base file from which differences are calculated')
    parser.add_argument('-t', '--target', nargs='+', help='File(s) to compress')
    args = parser.parse_args()

    compress(args.base, args.target)

def pyversion():
    print(sys.version)
    print(PNG)
