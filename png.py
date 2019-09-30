
import zlib

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
        self.filter_types = []

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
                return False

            if not self.chunkmap[chunk_type](length, data):
                print(chunk_type)
                print("error reading chunk")

        self.image_data = zlib.decompress(self.image_data)
        self.reconstruct_png(self.image_data)

        return True

    def reconstruct_png(self, original):
        scanlines_size = self.width*4
        scanlines_and_filter_size = scanlines_size + 1

        for i in range(self.height):
            filter_type = self.image_data[scanlines_and_filter_size * i]
            for j in range(scanlines_size):
                x = self.image_data[scanlines_and_filter_size * i + j + 1]

                a = 0
                b = 0
                c = 0

                if j >= 4:
                    a = self.reconstructed[i*scanlines_size + j - 4]
                if i > 0:
                    b = self.reconstructed[(i-1)*scanlines_size + j]
                if i > 0 and j >= 4:
                    c = self.reconstructed[(i-1)*scanlines_size + j-4]

                if filter_type == 1:
                    x += a
                elif filter_type == 2:
                    x += b
                elif filter_type == 3:
                    x += (a + b)//2
                elif filter_type == 4:
                    x += paeth_predictor(a, b, c)

                self.reconstructed.append(x % 256)

    def difference(self, other):
        differences = []
        scanlines_size = self.width*4
        for i, (b1, b2) in enumerate(zip(self.reconstructed, other.reconstructed)):
            if i % scanlines_size == 0:
                scanline = i//scanlines_size
                differences.append([])
            if b1 != b2:
                pos = (i-scanline*scanlines_size).to_bytes(2, byteorder='big')
                b1 = b1.to_bytes(1, byteorder='big')
                differences[scanline].append((pos, b1))

        return differences

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

