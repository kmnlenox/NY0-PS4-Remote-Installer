#!/usr/bin/env python3
"""Generate a deterministic 512x512 RGB NY0 diagnostic app icon (PNG)."""
import struct, zlib
from pathlib import Path

W = H = 512
rows = []
for y in range(H):
    row = bytearray([0])
    for x in range(W):
        # Graphite field with a restrained gold NY0-style geometric monogram.
        r, g, b = 20, 23, 29
        in_frame = 42 <= x <= 469 and 42 <= y <= 469
        if in_frame and (x < 50 or x > 461 or y < 50 or y > 461):
            r, g, b = 65, 69, 76
        # Gold angular N mark.
        n_mark = (
            (132 <= x <= 181 and 140 <= y <= 372) or
            (331 <= x <= 380 and 140 <= y <= 372) or
            (181 <= x <= 331 and abs((x - 181) - (y - 140) * 150 / 232) <= 24)
        )
        if n_mark:
            r, g, b = 218, 174, 75
        # Small zero ring to the lower-right, geometric and legible at icon scale.
        dx, dy = x - 300, y - 310
        d2 = dx*dx + dy*dy
        if 42*42 <= d2 <= 58*58 and 245 <= x <= 360 and 252 <= y <= 368:
            r, g, b = 235, 237, 240
        row.extend((r, g, b))
    rows.append(bytes(row))
def chunk(kind, data):
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xffffffff)
png = b"\x89PNG\r\n\x1a\n"
png += chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 2, 0, 0, 0))
png += chunk(b"IDAT", zlib.compress(b"".join(rows), 9))
png += chunk(b"IEND", b"")
out = Path("sce_sys/icon0.png")
out.parent.mkdir(parents=True, exist_ok=True)
out.write_bytes(png)
print(f"Generated {out}: {W}x{H} RGB PNG ({len(png)} bytes)")
