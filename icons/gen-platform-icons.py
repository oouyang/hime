#!/usr/bin/env python3
"""
Generate platform-specific icons from HIME source PNGs.

Reads icons/hime.png and icons/blue/*.png, then generates:
  - Android mipmap density variants (mdpi through xxxhdpi)
  - iOS AppIcon asset catalog with all required sizes
  - macOS iconset directory (for iconutil to create .icns)
  - Windows .ico files from mode PNGs (16x16 + 32x32)

Requirements: pip install Pillow

Usage:
    cd icons/
    python3 gen-platform-icons.py

Generated files are committed to the repository; re-run when icons change.
"""

import os
import struct
import sys

try:
    from PIL import Image
except ImportError:
    print("Error: Pillow is required. Install with: pip install Pillow", file=sys.stderr)
    sys.exit(1)

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.dirname(SCRIPT_DIR)
ICONS_DIR = SCRIPT_DIR
BLUE_DIR = os.path.join(ICONS_DIR, "blue")

HIME_PNG = os.path.join(ICONS_DIR, "hime.png")
MODE_PNGS = {
    "hime-tray": os.path.join(BLUE_DIR, "hime-tray.png"),
    "juyin": os.path.join(BLUE_DIR, "juyin.png"),
    "cj5": os.path.join(BLUE_DIR, "cj5.png"),
    "noseeing": os.path.join(BLUE_DIR, "noseeing.png"),
}


def ensure_dir(path):
    os.makedirs(path, exist_ok=True)


def resize_png(src_path, dst_path, size):
    """Resize a PNG to size x size with high-quality resampling."""
    with Image.open(src_path) as img:
        img = img.convert("RGBA")
        resized = img.resize((size, size), Image.LANCZOS)
        resized.save(dst_path, "PNG")
    print(f"  {dst_path} ({size}x{size})")


def generate_android_icons():
    """Generate Android mipmap launcher icons at all density buckets."""
    print("\n=== Android mipmap icons ===")
    android_res = os.path.join(ROOT_DIR, "platform", "android", "app", "src", "main", "res")

    densities = {
        "mipmap-mdpi": 48,
        "mipmap-hdpi": 72,
        "mipmap-xhdpi": 96,
        "mipmap-xxhdpi": 144,
        "mipmap-xxxhdpi": 192,
    }

    for folder, size in densities.items():
        out_dir = os.path.join(android_res, folder)
        ensure_dir(out_dir)
        resize_png(HIME_PNG, os.path.join(out_dir, "ic_launcher.png"), size)


def generate_ios_icons():
    """Generate iOS AppIcon asset catalog with all required sizes."""
    print("\n=== iOS AppIcon asset catalog ===")
    appiconset_dir = os.path.join(
        ROOT_DIR, "platform", "ios", "HIMEApp", "Assets.xcassets", "AppIcon.appiconset"
    )
    ensure_dir(appiconset_dir)

    # iOS icon sizes: (points, scale, idiom)
    icon_specs = [
        # iPhone
        (60, 2, "iphone"),      # 120x120
        (60, 3, "iphone"),      # 180x180
        # iPad
        (76, 2, "ipad"),        # 152x152
        (83.5, 2, "ipad"),      # 167x167
        # App Store
        (1024, 1, "ios-marketing"),  # 1024x1024
    ]

    images_json = []
    for points, scale, idiom in icon_specs:
        pixel_size = int(points * scale)
        filename = f"icon_{pixel_size}x{pixel_size}.png"
        resize_png(HIME_PNG, os.path.join(appiconset_dir, filename), pixel_size)

        size_str = f"{int(points)}x{int(points)}" if points == int(points) else f"{points}x{points}"
        images_json.append(
            f'    {{\n'
            f'      "filename" : "{filename}",\n'
            f'      "idiom" : "{idiom}",\n'
            f'      "scale" : "{scale}x",\n'
            f'      "size" : "{size_str}"\n'
            f'    }}'
        )

    contents = (
        '{\n'
        '  "images" : [\n'
        + ',\n'.join(images_json) + '\n'
        '  ],\n'
        '  "info" : {\n'
        '    "author" : "hime-gen-platform-icons",\n'
        '    "version" : 1\n'
        '  }\n'
        '}\n'
    )

    contents_path = os.path.join(appiconset_dir, "Contents.json")
    with open(contents_path, "w") as f:
        f.write(contents)
    print(f"  {contents_path}")


def generate_macos_iconset():
    """Generate macOS .iconset directory for iconutil conversion."""
    print("\n=== macOS iconset ===")
    iconset_dir = os.path.join(ROOT_DIR, "platform", "macos", "Resources", "HIME.iconset")
    ensure_dir(iconset_dir)

    # macOS iconset sizes: (base_size, scales)
    sizes = [16, 32, 128, 256, 512]

    for base in sizes:
        # 1x
        resize_png(HIME_PNG, os.path.join(iconset_dir, f"icon_{base}x{base}.png"), base)
        # 2x (Retina)
        retina = base * 2
        resize_png(HIME_PNG, os.path.join(iconset_dir, f"icon_{base}x{base}@2x.png"), retina)

    print(f"\n  To create HIME.icns (on macOS):")
    print(f"    iconutil -c icns {iconset_dir} -o {os.path.join(ROOT_DIR, 'platform', 'macos', 'Resources', 'HIME.icns')}")


def create_ico(png_paths, ico_path, sizes=None):
    """
    Create a Windows .ico file from a PNG source.
    Embeds PNG data directly (PNG-in-ICO, supported since Windows Vista).
    """
    if sizes is None:
        sizes = [16, 32]

    entries = []
    for size in sizes:
        with Image.open(png_paths) as img:
            img = img.convert("RGBA")
            resized = img.resize((size, size), Image.LANCZOS)
            # Save as PNG in memory
            import io
            buf = io.BytesIO()
            resized.save(buf, "PNG")
            png_data = buf.getvalue()
            entries.append((size, png_data))

    # ICO file format
    with open(ico_path, "wb") as f:
        num = len(entries)
        # ICONDIR header: reserved(2) + type(2) + count(2)
        f.write(struct.pack("<HHH", 0, 1, num))

        # Calculate data offset (header + all ICONDIRENTRY)
        offset = 6 + num * 16

        # Write ICONDIRENTRY for each image
        for size, png_data in entries:
            w = size if size < 256 else 0
            h = size if size < 256 else 0
            # width, height, colors, reserved, planes, bitcount, size, offset
            f.write(struct.pack("<BBBBHHII", w, h, 0, 0, 1, 32, len(png_data), offset))
            offset += len(png_data)

        # Write image data
        for _, png_data in entries:
            f.write(png_data)

    print(f"  {ico_path}")


def generate_windows_icons():
    """Generate Windows .ico files for mode icons and copy PNGs for GDI+ loading."""
    print("\n=== Windows icons ===")
    win_icons_dir = os.path.join(ROOT_DIR, "platform", "windows", "icons")
    ensure_dir(win_icons_dir)

    # Copy mode PNGs for GDI+ runtime loading
    for name, src in MODE_PNGS.items():
        if os.path.exists(src):
            dst = os.path.join(win_icons_dir, f"{name}.png")
            with Image.open(src) as img:
                img.save(dst, "PNG")
            print(f"  {dst} (copied)")

    # Generate .ico files (16x16 + 32x32) for each mode
    for name, src in MODE_PNGS.items():
        if os.path.exists(src):
            create_ico(src, os.path.join(win_icons_dir, f"{name}.ico"))

    # Generate app icon .ico from hime.png
    create_ico(HIME_PNG, os.path.join(win_icons_dir, "hime.ico"), sizes=[16, 32, 48, 64])

    # Also update res/hime.ico (embedded in exe/dll resources)
    win_res_dir = os.path.join(ROOT_DIR, "platform", "windows", "res")
    ensure_dir(win_res_dir)
    create_ico(HIME_PNG, os.path.join(win_res_dir, "hime.ico"), sizes=[16, 32, 48, 64])


def main():
    print("HIME Platform Icon Generator")
    print("============================")

    if not os.path.exists(HIME_PNG):
        print(f"Error: {HIME_PNG} not found", file=sys.stderr)
        sys.exit(1)

    for name, path in MODE_PNGS.items():
        if not os.path.exists(path):
            print(f"Warning: {path} not found, skipping {name}", file=sys.stderr)

    generate_android_icons()
    generate_ios_icons()
    generate_macos_iconset()
    generate_windows_icons()

    print("\n=== Done ===")
    print("Generated icons for Android, iOS, macOS, and Windows.")
    print("On macOS, run iconutil to create HIME.icns from the .iconset directory.")


if __name__ == "__main__":
    main()
