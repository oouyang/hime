#!/usr/bin/env python3
"""
Convert Boshiamy liu-uni.tab binary table to HIME .cin and .gtab formats.

The Boshiamy IME (嘸蝦米) stores its character tables in a proprietary
binary format (liu-uni.tab, liu-uni2.tab, etc.).  This tool decodes the
binary format and produces a standard .cin table that can be used by
HIME, gcin, Rime, and other input method frameworks.

Binary format decoded from UCL_LIU/tools/uni2txt.php by FeatherMountain.

Usage:
    # Generate .cin only
    python3 liu-tab2cin.py /path/to/liu-uni.tab -o liu.cin

    # Generate .cin and .gtab (requires hime-cin2gtab in PATH or repo)
    python3 liu-tab2cin.py /path/to/liu-uni.tab -o liu.cin --gtab

    # Custom table name and description
    python3 liu-tab2cin.py /path/to/liu-uni.tab -o liu.cin \\
        --ename liu --cname 嘸蝦米

Typical source files (from C:\\Program Files\\BoshiamyTIP\\):
    liu-uni.tab   - Main table (1-4 key, ~31k chars)
    liu-uni2.tab  - Extended table 2
    liu-uni3.tab  - Extended table 3
    liu-uni4.tab  - Extended table 4
"""

import argparse
import os
import shutil
import subprocess
import sys

# ---------------------------------------------------------------------------
# Binary format decoder
# ---------------------------------------------------------------------------

def _getint16(data, addr):
    """Read a 16-bit little-endian integer."""
    return data[addr] | (data[addr + 1] << 8)


def _getbits(data, start, nbit, i):
    """Read *nbit* bits for entry *i* from a packed bitfield at *start*."""
    if nbit in (1, 2, 4):
        byte_offset = int(start + i * nbit / 8)
        byte_val = data[byte_offset]
        shift = 8 - nbit - (i * nbit % 8)
        return (byte_val >> shift) & ((1 << nbit) - 1)
    elif nbit > 0 and nbit % 8 == 0:
        nbyte = nbit // 8
        value = 0
        a = start + i * nbyte
        for _ in range(nbyte):
            value = (value << 8) | data[a]
            a += 1
        return value
    else:
        raise ValueError(f"unsupported nbit={nbit}")


def decode_liu_tab(path):
    """Decode a liu-uni*.tab binary file.

    Returns a list of (key, character) tuples sorted by key.
    """
    with open(path, "rb") as f:
        data = list(f.read())

    i1 = _getint16(data, 0)
    i2 = i1 + _getint16(data, 2)
    i3 = i2 + _getint16(data, 6)
    i4 = i3 + _getint16(data, 6)

    rootkey = list(" abcdefghijklmnopqrstuvwxyz,.'[]")

    entries = []
    for i in range(1024):
        key0 = rootkey[i // 32]
        key1 = rootkey[i % 32]
        if key0 == " ":
            continue

        start_ci = _getint16(data, i * 2)
        end_ci = _getint16(data, i * 2 + 2)

        for ci in range(start_ci, end_ci):
            bit24 = _getbits(data, i4, 24, ci)
            hi = _getbits(data, i1, 2, ci)
            lo = bit24 & 0x3FFF

            key2 = rootkey[bit24 >> 19]
            key3 = rootkey[(bit24 >> 14) & 0x1F]

            key_str = (key0 + key1 + key2 + key3).strip()
            codepoint = (hi << 14) | lo
            if codepoint > 0:
                entries.append((key_str, chr(codepoint)))

    return entries


# ---------------------------------------------------------------------------
# .cin writer
# ---------------------------------------------------------------------------

CIN_HEADER_TEMPLATE = """%gen_inp
%ename {ename}
%cname {cname}
%selkey 1234567890
%space_style 2
%keyname begin
a a
b b
c c
d d
e e
f f
g g
h h
i i
j j
k k
l l
m m
n n
o o
p p
q q
r r
s s
t t
u u
v v
w w
x x
y y
z z
, ,
. .
' '
[ [
] ]
%keyname end
%chardef begin
"""


def write_cin(entries, path, ename="liu", cname="\u5606\u8766\u861c"):
    """Write entries as a .cin table file."""
    with open(path, "w", encoding="utf-8") as f:
        f.write(CIN_HEADER_TEMPLATE.format(ename=ename, cname=cname))
        for key, char in entries:
            f.write(f"{key} {char}\n")
        f.write("%chardef end\n")


# ---------------------------------------------------------------------------
# .gtab generation (calls hime-cin2gtab)
# ---------------------------------------------------------------------------

def _find_cin2gtab():
    """Locate hime-cin2gtab executable."""
    # Check PATH first
    exe = shutil.which("hime-cin2gtab")
    if exe:
        return exe

    # Check relative to this script (repo layout: tools/ -> src/)
    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo_root = os.path.dirname(script_dir)
    candidate = os.path.join(repo_root, "src", "hime-cin2gtab")
    if os.path.isfile(candidate) and os.access(candidate, os.X_OK):
        return candidate

    return None


def generate_gtab(cin_path):
    """Run hime-cin2gtab on the .cin file to produce a .gtab alongside it."""
    exe = _find_cin2gtab()
    if not exe:
        print(
            "Warning: hime-cin2gtab not found. Build HIME first or add it to PATH.",
            file=sys.stderr,
        )
        print("  Skipping .gtab generation.", file=sys.stderr)
        return False

    cin_dir = os.path.dirname(os.path.abspath(cin_path))
    cin_name = os.path.basename(cin_path)

    result = subprocess.run(
        [exe, cin_name],
        cwd=cin_dir,
        capture_output=True,
        text=True,
    )

    if result.returncode != 0:
        print(f"hime-cin2gtab failed:\n{result.stderr}", file=sys.stderr)
        return False

    gtab_path = cin_path.rsplit(".", 1)[0] + ".gtab"
    print(result.stdout, end="")
    print(f"Generated {gtab_path}")
    return True


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Convert Boshiamy liu-uni.tab to HIME .cin / .gtab",
    )
    parser.add_argument(
        "input",
        help="Path to liu-uni.tab (or liu-uni2/3/4.tab)",
    )
    parser.add_argument(
        "-o", "--output",
        default=None,
        help="Output .cin path (default: <input-stem>.cin in current dir)",
    )
    parser.add_argument(
        "--gtab",
        action="store_true",
        help="Also generate .gtab using hime-cin2gtab",
    )
    parser.add_argument(
        "--ename",
        default="liu",
        help="English name for the table (default: liu)",
    )
    parser.add_argument(
        "--cname",
        default="嘸蝦米",
        help="Chinese name for the table (default: 嘸蝦米)",
    )

    args = parser.parse_args()

    if not os.path.isfile(args.input):
        print(f"Error: {args.input} not found", file=sys.stderr)
        sys.exit(1)

    # Default output name
    if args.output is None:
        stem = os.path.splitext(os.path.basename(args.input))[0]
        args.output = stem + ".cin"

    # Decode
    print(f"Decoding {args.input} ...")
    entries = decode_liu_tab(args.input)
    print(f"  {len(entries)} entries decoded")

    # Write .cin
    write_cin(entries, args.output, ename=args.ename, cname=args.cname)
    print(f"Generated {args.output}")

    # Optionally generate .gtab
    if args.gtab:
        generate_gtab(args.output)


if __name__ == "__main__":
    main()
