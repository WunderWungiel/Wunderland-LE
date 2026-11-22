"""Builds the small rating stars (star_p_s / star_g_s, 9x8) used by the app list
from the full-size web store stars (star_p / star_g, 14x13).

Area-averaged downscale with the colour weighted by alpha, so the edges stay
smooth (scaling at draw time with DrawBitmapMasked is nearest-neighbour).
Writes 24-bit colour BMPs and 8-bit grey alpha masks, the format bmconv's
"c24,8" in group/Wunderland.mmp expects. Run from anywhere:
    python gfx/gen_small_stars.py
"""
import os
import struct

DIR = os.path.dirname(os.path.abspath(__file__))
OUT_W, OUT_H = 9, 8


def read_bmp(path):
    """Returns (width, height, rows) with rows top-down; pixels are (b, g, r) or ints."""
    data = open(path, 'rb').read()
    offset, = struct.unpack_from('<I', data, 10)
    width, height = struct.unpack_from('<ii', data, 18)
    bpp, = struct.unpack_from('<H', data, 28)
    stride = (width * bpp // 8 + 3) & ~3
    rows = []
    for r in range(height):
        base = offset + r * stride
        if bpp == 24:
            rows.append([tuple(data[base + 3 * c: base + 3 * c + 3]) for c in range(width)])
        elif bpp == 8:
            # grey palette: index == level
            rows.append([data[base + c] for c in range(width)])
        else:
            raise ValueError('%s: %d bpp not supported' % (path, bpp))
    rows.reverse()   # BMP rows are stored bottom-up
    return width, height, rows


def weights(src, dst):
    """For each destination index: [(source index, coverage)], coverages sum to src/dst."""
    scale = src / dst
    result = []
    for d in range(dst):
        start, end = d * scale, (d + 1) * scale
        cells = []
        s = int(start)
        while s < end and s < src:
            cover = min(end, s + 1) - max(start, s)
            if cover > 0:
                cells.append((s, cover))
            s += 1
        result.append(cells)
    return result


def downscale(colour, alpha, w, h):
    wx, wy = weights(w, OUT_W), weights(h, OUT_H)
    out_c, out_a = [], []
    for y in range(OUT_H):
        crow, arow = [], []
        for x in range(OUT_W):
            area = a_sum = 0.0
            c_sum = [0.0, 0.0, 0.0]
            for sy, cy in wy[y]:
                for sx, cx in wx[x]:
                    cover = cx * cy
                    a = alpha[sy][sx] / 255.0
                    area += cover
                    a_sum += cover * a
                    for i in range(3):
                        c_sum[i] += cover * a * colour[sy][sx][i]
            arow.append(int(round(255 * a_sum / area)))
            if a_sum > 0:
                crow.append(tuple(int(round(v / a_sum)) for v in c_sum))
            else:
                crow.append((0, 0, 0))
        out_c.append(crow)
        out_a.append(arow)
    return out_c, out_a


def write_bmp(path, rows, bpp):
    width, height = len(rows[0]), len(rows)
    stride = (width * bpp // 8 + 3) & ~3
    palette = b''.join(bytes((i, i, i, 0)) for i in range(256)) if bpp == 8 else b''
    offset = 14 + 40 + len(palette)
    pixels = bytearray()
    for row in reversed(rows):   # bottom-up
        line = bytearray()
        for p in row:
            line += bytes(p) if bpp == 24 else bytes((p,))
        line += b'\0' * (stride - len(line))
        pixels += line
    header = struct.pack('<2sIHHI', b'BM', offset + len(pixels), 0, 0, offset)
    info = struct.pack('<IiiHHIIiiII', 40, width, height, 1, bpp, 0, len(pixels),
                       2835, 2835, 256 if bpp == 8 else 0, 0)
    with open(path, 'wb') as f:
        f.write(header + info + palette + pixels)


def main():
    for name in ('star_p', 'star_g'):
        w, h, colour = read_bmp(os.path.join(DIR, name + '.bmp'))
        mw, mh, alpha = read_bmp(os.path.join(DIR, name + '_mask.bmp'))
        assert (w, h) == (mw, mh)
        small_c, small_a = downscale(colour, alpha, w, h)
        write_bmp(os.path.join(DIR, name + '_s.bmp'), small_c, 24)
        write_bmp(os.path.join(DIR, name + '_s_mask.bmp'), small_a, 8)
        print('%s: %dx%d -> %dx%d' % (name, w, h, OUT_W, OUT_H))


if __name__ == '__main__':
    main()
