
import zlib
import struct
import logging

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

        self.scanline_size = None

    COMPRESSION_IGNORE = b'\x00'
    COMPRESSION_SIMPLE = b'\x01'
    COMPRESSION_CONTINUED = b'\x02'
    COMPRESSION_REPLACE = b'\x03'

    def process(self, file):
        signature = file.read(8);

        while True:
            length = int.from_bytes(file.read(4), byteorder='big')
            chunk_type = file.read(4)
            data = file.read(length)
            crc = int.from_bytes(file.read(4), byteorder='big')

            if chunk_type == b'IEND':
                break

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
        print('finished decompressing')
        self.reconstruct_png(self.image_data)
        print('finished reconstructing')

        return True

    @classmethod
    def from_file(cls, filename):
        with open(filename, 'rb') as f:
            new_image = cls()
            new_image.process(f)
        return new_image

    def reconstruct_png(self, original):
        scanline_size = self.width*4

        # we store it for further use, but avoid the dictionary lookup
        # each time we need to use it
        self.scanline_size = scanline_size

        scanlines_and_filter_size = scanline_size + 1

        for i in range(self.height):
            filter_type = self.image_data[scanlines_and_filter_size * i]
            # self.filter_types += filter_type
            for j in range(scanline_size):
                x = self.image_data[scanlines_and_filter_size * i + j + 1]

                if j >= 4:
                    a = self.reconstructed[i*scanline_size + j - 4]
                else:
                    a = 0
                if i > 0:
                    b = self.reconstructed[(i-1)*scanline_size + j]
                else:
                    b = 0
                if i > 0 and j >= 4:
                    c = self.reconstructed[(i-1)*scanline_size + j-4]
                else:
                    c = 0

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


    def difference(self, other):
        difference_data = bytearray()
        compression_types = bytearray()

        for s in range(self.height):
            current_scanline = self.scanline(s)
            scanlines = zip(current_scanline, other.scanline(s))

            # we get the postions in which bytes differ
            s_difference = [*filter(lambda b: b[1][0] != b[1][1],
                                    enumerate(scanlines))]
            difference_size = len(s_difference)*3

            #if difference_size < self.scanline_size:

            if len(s_difference) == 0:
                compression_type = self.__class__.COMPRESSION_IGNORE
                compression_types += compression_type
                continue

            simple_difference = b''.join(map(lambda x: struct.pack('>HB', x[0], x[1][0]),
                                             s_difference))
            continued_difference = bytearray()
            segment = bytearray()
            segment_length = 0
            start = 0
            for pos, (b, dummy) in s_difference:
                if (pos != start + segment_length) or (segment_length == 255) :
                    continued_difference += struct.pack('>HB', start, segment_length) + segment
                    segment = bytearray()
                    segment_length = 0
                    start = pos
                segment += struct.pack('>B', b)
                segment_length += 1

            if len(continued_difference) > len(simple_difference):
                compression_type = self.__class__.COMPRESSION_SIMPLE
                compression_types += compression_type
                difference = simple_difference
            elif len(current_scanline) > len(continued_difference):
                compression_type = self.__class__.COMPRESSION_CONTINUED
                compression_types += compression_type
                difference = continued_difference
            else:
                compression_type = self.__class__.COMPRESSION_REPLACE
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
            return False

        if self.colour_type != 6:
            return False

        if self.compression_method != 0:
            return False

        if self.interlace_method != 0:
            return False

        return True

    def srgb_reader(self, length, data):
        return True

    def sbit_reader(self, length, data):
        return True

    def idat_reader(self, length, data):
        self.image_data += data

        return True

