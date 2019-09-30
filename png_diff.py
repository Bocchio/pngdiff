
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

differences = compressible.difference(origin)

with open(args.compressible + 'diff', 'wb') as f:
    f.write(b'pngdiff')
    path = os.path.join(os.getcwd(), args.origin)
    path = path.encode()
    length = len(path)
    f.write(length.to_bytes(length=2, byteorder='big'))
    f.write(path)

    difference_data = bytearray()
    for s, scanline in enumerate(differences):
        s = s.to_bytes(length=2, byteorder='big')
        length = len(scanline)*3
        length = length.to_bytes(length=4, byteorder='big')
        difference_data += s
        difference_data += length
        for pos, val in scanline:
            difference_data += pos
            difference_data += val

    difference_data = zlib.compress(difference_data)
    length = len(difference_data).to_bytes(length=4, byteorder='big')
    f.write(length)
    f.write(difference_data)
