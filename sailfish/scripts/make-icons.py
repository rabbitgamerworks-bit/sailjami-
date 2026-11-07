#!/usr/bin/env python3
# Generate the SailJami launcher icons (a generic chat bubble, not the Jami logo).
# SPDX-License-Identifier: GPL-3.0-or-later
import os
import struct
import sys
import zlib

SIZES = [86, 108, 128, 172]


def png(width, height, pixels):
    raw = b"".join(b"\x00" + bytes(row) for row in pixels)

    def chunk(tag, data):
        c = struct.pack(">I", len(data)) + tag + data
        return c + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    return (b"\x89PNG\r\n\x1a\n"
            + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(raw, 9))
            + chunk(b"IEND", b""))


def icon(n):
    ss = 4  # supersampling for anti-aliasing
    rows = []
    c = n / 2
    for y in range(n):
        row = []
        for x in range(n):
            cov_bg = cov_fg = 0
            for sy in range(ss):
                for sx in range(ss):
                    px = x + (sx + 0.5) / ss
                    py = y + (sy + 0.5) / ss
                    dx, dy = px - c, py - c
                    if dx * dx + dy * dy <= (n * 0.48) ** 2:
                        cov_bg += 1
                        # speech bubble: ellipse + tail
                        ex, ey = (px - c) / (n * 0.30), (py - c * 0.92) / (n * 0.22)
                        tail = (py > c * 1.05 and py < c * 1.45
                                and px > c - n * 0.18 and px < c - n * 0.05
                                and (px - (c - n * 0.18)) > (py - c * 1.05) * 0.4)
                        if ex * ex + ey * ey <= 1 or tail:
                            cov_fg += 1
            total = ss * ss
            a_bg = cov_bg / total
            a_fg = cov_fg / total
            t = y / n
            bg = (int(0x0c + t * 0x10), int(0x6a - t * 0x20), int(0xc4 - t * 0x30))
            r = int(bg[0] * (1 - a_fg) + 255 * a_fg)
            g = int(bg[1] * (1 - a_fg) + 255 * a_fg)
            b = int(bg[2] * (1 - a_fg) + 255 * a_fg)
            row += [r, g, b, int(255 * a_bg)]
        rows.append(row)
    return png(n, n, rows)


def main(outdir):
    for n in SIZES:
        d = os.path.join(outdir, "%dx%d" % (n, n))
        os.makedirs(d, exist_ok=True)
        with open(os.path.join(d, "sailjami.png"), "wb") as f:
            f.write(icon(n))


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "icons")
