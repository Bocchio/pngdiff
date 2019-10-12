
import zlib
import struct
import logging

import random

def paeth_predictor(a, b, c):
    pa = abs(b - c)
    pb = abs(a - c)
    pc = abs(a + b - 2*c)
    if pa <= pb and pa <= pc:
        return a
    elif pb <= pc:
        return b
    else:
        return c

class PNG:

    def __init__(self):
        self.image_data = bytearray()
        self.reconstructed = bytearray()
        self.chunkmap = {b'IHDR': self.ihdr_reader,
                         b'sRGB': self.srgb_reader,
                         b'sBIT': self.sbit_reader,
                         b'IDAT': self.idat_reader}
        self.filter_types = bytearray()

        self.png_data = bytearray()

        self.scanline_size = None

        self.path = ''

    COMPRESSION_IGNORE = b'\x00'
    COMPRESSION_SIMPLE = b'\x01'
    COMPRESSION_CONTINUED = b'\x02'
    COMPRESSION_REPLACE = b'\x03'

    EXTENDED_FLAG = 0x80

    def process(self, file):
        signature = file.read(8);

        while True:
            length = int.from_bytes(file.read(4), byteorder='big')
            chunk_type = file.read(4)
            data = file.read(length)
            crc = int.from_bytes(file.read(4), byteorder='big')

            if chunk_type == b'IEND':
                break

            if chunk_type != b'IDAT':
                self.png_data += length.to_bytes(length=4, byteorder='big')
                self.png_data += chunk_type + data

            if crc != zlib.crc32(chunk_type + data):
                print(chunk_type)
                print("CRC failed", crc, crc32(data, crc32(data)))
                return False

            if not chunk_type in self.chunkmap:
                print(chunk_type)
                continue
                #raise Exception('Unrecognized chunk type')

            if not self.chunkmap[chunk_type](length, data):
                print(chunk_type)
                print("error reading chunk")

        self.image_data = zlib.decompress(self.image_data)
        self.reconstruct_png(self.image_data)
        print('finished reading', end='')

        return True

    @classmethod
    def from_file(cls, filename):
        with open(filename, 'rb') as f:
            new_image = cls()
            new_image.process(f)
        new_image.path = filename
        return new_image

    def reconstruct_png(self, original):
        scanline_size = self.width*4

        # we store it for further use, but avoid the dictionary lookup
        # each time we need to use it
        self.scanline_size = scanline_size

        scanlines_and_filter_size = scanline_size + 1

        for i in range(self.height):
            filter_type = self.image_data[scanlines_and_filter_size * i]
            self.filter_types += filter_type.to_bytes(length=1, byteorder='big')

            row = i*scanline_size
            row_before = (i-1)*scanline_size
            x_offset = scanlines_and_filter_size * i
            for j in range(scanline_size):
                x = self.image_data[x_offset + j + 1]

                a = 0
                b = 0
                c = 0
                if j >= 4:
                    a = self.reconstructed[row + j - 4]
                    if i > 0:
                        b = self.reconstructed[row_before + j]
                        c = self.reconstructed[row_before + j - 4]
                elif i > 0:
                    b = self.reconstructed[row_before + j]


                if filter_type == 1:
                    x += a
                elif filter_type == 2:
                    x += b
                elif filter_type == 3:
                    x += (a + b)//2
                elif filter_type == 4:
                    x += paeth_predictor(a, b, c)

                self.reconstructed.append(x % 256)


    def scanline(self, index):
        start = self.scanline_size*index
        end = start + self.scanline_size
        return self.reconstructed[start:end]


    def is_similar(self, other, n=2):

        difference_data = bytearray()
        compression_types = bytearray()

        weight = 0
        for i in range(n):
            j = random.randint(0, self.height-1)
            current_scanline = self.scanline(j)
            scanlines = zip(current_scanline, other.scanline(j))
            s_difference = [*filter(lambda b: b[1][0] != b[1][1],
                                    enumerate(scanlines))]
            weight += len(s_difference)

        if weight > self.width*n:
            return False

        return True


    def difference(self, other):
        difference_data = bytearray()
        compression_types = bytearray()

        cls = self.__class__

        for s in range(self.height):
            current_scanline = self.scanline(s)
            scanlines = zip(current_scanline, other.scanline(s))

            # we get the postions in which bytes differ
            # s_difference = [*filter(lambda b: b[1][0] != b[1][1],
            #                         enumerate(scanlines))]
            s_difference = []
            for i, (b1, b2) in enumerate(scanlines):
                if b1 != b2:
                    s_difference.append((i, b1))

            difference_size = len(s_difference)*3

            #if difference_size < self.scanline_size:

            if len(s_difference) == 0:
                compression_type = cls.COMPRESSION_IGNORE
                compression_types += compression_type
                continue

            #simple_difference = b''.join(map(lambda x: struct.pack('>HB', x[0], x[1][0]),
            #                                 s_difference))
            continued_difference = bytearray()
            segment = bytearray()
            segment_length = 0
            start = s_difference[0][0]
            last_pos = s_difference[-1][0]
            for pos, b in s_difference:

                # it's cheaper to replace a byte in a segment than to create another
                # segment altogether
                if pos > (start + segment_length + 5) or pos == last_pos:

                    # Extended coding it lets use the msb in each byte as a flag
                    # that the data actually has another byte
                    msb = cls.EXTENDED_FLAG
                    segment_byte_length = 1
                    while segment_length >= msb:
                        low_bits = msb - 1
                        low = segment_length & low_bits
                        high = segment_length & ~low_bits
                        segment_length = (high << 1) + msb + low
                        segment_byte_length += 1
                        msb <<= 8
                    # the byteorder must be little
                    # that's because we check in case we need a bigger length
                    # so the least significant byte should appear first
                    segment_bytes = segment_length.to_bytes(length=segment_byte_length,
                                                            byteorder='little')

                    continued_difference += struct.pack('>H', start) + segment_bytes + segment
                    segment = bytearray()
                    segment_length = 0
                    start = pos
                while pos != start + segment_length:
                    segment += struct.pack('>B', current_scanline[start + segment_length])
                    segment_length += 1
                segment += struct.pack('>B', b)
                segment_length += 1

            #if len(continued_difference) > len(simple_difference):
            #    compression_type = cls.COMPRESSION_SIMPLE
            #    compression_types += compression_type
            #    difference = simple_difference
            if len(current_scanline) > len(continued_difference):
                compression_type = cls.COMPRESSION_CONTINUED
                compression_types += compression_type
                difference = continued_difference
            else:
                compression_type = cls.COMPRESSION_REPLACE
                compression_types += compression_type
                difference = current_scanline


            difference_data += struct.pack('>I', len(difference))
            difference_data += difference

            # else:
            #     compression_type = self.__class__.COMPRESSION_REPLACE
            #     length = difference_size.to_bytes(length=4, byteorder='big')
            #     difference_data += compression_type
            #     difference_data = length
            #     for pos, val in scanline:
            #         difference_data += pos
            #         difference_data += val

        return compression_types, difference_data

    def ihdr_reader(self, length, data):
        self.width = int.from_bytes(data[0:4], byteorder='big')
        self.height = int.from_bytes(data[4:8], byteorder='big')
        self.bit_depth = data[8]
        self.colour_type = data[9]
        self.compression_method = data[10]
        self.filter_method = data[11]
        self.interlace_method = data[12]

        if self.bit_depth != 8:
            print('Unrecognized bit_depth')
            return False

        if self.colour_type != 6:
            print('Unrecognized colour type')
            return False

        if self.compression_method != 0:
            print('Unrecognized compression method')
            return False

        if self.interlace_method != 0:
            print('Unrecognized interlace method')
            return False

        return True

    def srgb_reader(self, length, data):
        return True

    def sbit_reader(self, length, data):
        return True

    def idat_reader(self, length, data):
        self.image_data += data

        return True

