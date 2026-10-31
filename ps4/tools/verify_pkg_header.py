#!/usr/bin/env python3
"""Verify the PS4 PKG header fields consumed by NY0's BGFT manifest builder."""
import pathlib
import struct
import sys

if len(sys.argv) != 2:
    raise SystemExit("usage: verify_pkg_header.py PACKAGE.pkg")

pkg_path = pathlib.Path(sys.argv[1])
size = pkg_path.stat().st_size
with pkg_path.open("rb") as f:
    header = f.read(0x2000)

if len(header) != 0x2000:
    raise SystemExit("FAIL: PKG header is truncated")
if header[:4] != b"\x7fCNT":
    raise SystemExit(f"FAIL: unexpected PKG magic {header[:4]!r}")

content_id = header[0x40:0x40 + 36].decode("ascii", errors="strict")
content_type = struct.unpack(">I", header[0x74:0x78])[0]
content_flags = struct.unpack(">I", header[0x78:0x7C])[0]
package_size = struct.unpack(">Q", header[0x430:0x438])[0]
digest = header[0xFE0:0x1000].hex()

if package_size != size:
    raise SystemExit(f"FAIL: header size {package_size} != file size {size}")
if len(content_id) != 36 or content_id[6] != "-" or content_id[16] != "_" or content_id[19] != "-":
    raise SystemExit(f"FAIL: malformed Content ID {content_id!r}")
if content_type not in (0x1A, 0x1B, 0x1C, 0x1E):
    raise SystemExit(f"FAIL: unsupported content type 0x{content_type:X}")
if digest == "00" * 32:
    raise SystemExit("FAIL: empty package digest")

print("PKG header OK")
print(f"Content ID: {content_id}")
print(f"Content type: 0x{content_type:02X}")
print(f"Content flags: 0x{content_flags:08X}")
print(f"Package size: {package_size}")
print(f"Package digest: {digest}")
