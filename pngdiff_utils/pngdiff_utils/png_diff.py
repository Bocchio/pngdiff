
from png import PNG

import argparse
import os
import zlib


parser = argparse.ArgumentParser()

parser.add_argument('origin', help='Source file')
parser.add_argument('compressible', help='File to compress')

args = parser.parse_args()

origin = PNG()
compressible = PNG()

with open(args.origin, 'rb') as f:
    origin.process(f)

with open(args.compressible, 'rb') as f:
    compressible.process(f)

print('Finished reading')

differences = compressible.difference(origin)

print('Finished calculating differences')


def write_chunk(f, chunk_type, data, compress=False):
    if compress:
        data = zlib.compress(data)

    length = len(data)
    f.write(length.to_bytes(length=4, byteorder='big'))
    f.write(chunk_type)
    f.write(data)
    crc = zlib.crc32(chunk_type + data)
    f.write(crc.to_bytes(length=4, byteorder='big'))


with open(args.compressible + 'diff', 'wb') as f:
    f.write(b'pngdiff')
    path = os.path.join(os.getcwd(), args.origin)
    path = path.encode()
    write_chunk(f, b'path', path)

    compression_types = bytearray()

    difference_data = bytearray()
    for s, scanline in enumerate(differences):
        s = s.to_bytes(length=2, byteorder='big')
        length = len(scanline)*3
        difference_data += s
        if length < origin.width*4:
            compression_types += b'\x00'
            length = length.to_bytes(length=4, byteorder='big')
            difference_data += length
            for pos, val in scanline:
                difference_data += pos
                difference_data += val

    write_chunk(f, b'styp', compression_types, compress=True)
    write_chunk(f, b'idat', difference_data, compress=True)
    write_chunk(f, b'IEND', b'')
