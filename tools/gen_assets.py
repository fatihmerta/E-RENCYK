#!/usr/bin/env python3
"""
Generates custom PSP XMB artwork (ICON0.PNG 144x80, PIC1.PNG 480x272)
and two Y2K stereo demo MP3 tracks with Turkish UTF-8 ID3v2 tags + embedded APIC cover art.
Uses only Python standard library + system ffmpeg (if available).
"""

import math
import os
import shutil
import struct
import subprocess
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ASSETS_DIR = os.path.join(ROOT, "assets")
DEMO_MUSIC_DIR = os.path.join(ROOT, "dist", "PSP", "GAME", "ERENCYK", "music")


def write_png(path: str, width: int, height: int, rgb_bytes: bytes) -> None:
    """Minimal pure-Python 24-bit RGB PNG encoder."""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    raw = bytearray()
    stride = width * 3
    for y in range(height):
        raw.append(0)  # Filter type 0 (None)
        raw.extend(rgb_bytes[y * stride : (y + 1) * stride])

    def chunk(tag: bytes, data: bytes) -> bytes:
        return (
            struct.pack(">I", len(data))
            + tag
            + data
            + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
        )

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    png = (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", ihdr)
        + chunk(b"IDAT", zlib.compress(bytes(raw), 9))
        + chunk(b"IEND", b"")
    )
    with open(path, "wb") as f:
        f.write(png)


def clamp8(v: float) -> int:
    return max(0, min(255, int(v)))


def generate_icon0(path: str) -> None:
    """144x80 PSP XMB Icon: Brushed-silver Sony MiniDisc Walkman + Ice-Cyan LCD + VU."""
    w, h = 144, 80
    buf = bytearray(w * h * 3)
    for y in range(h):
        for x in range(w):
            # Brushed titanium chassis with rounded border feel
            hair = 5 if (y % 2 == 0) else -3
            grad = 212 - int(y * 0.65) - int(x * 0.12) + hair
            r, g, b = grad, grad + 5, grad + 12

            # Outer dark frame & inner specular highlight
            if x in (0, w - 1) or y in (0, h - 1):
                r, g, b = 42, 48, 56
            elif x in (1, 2) or y in (1, 2):
                r, g, b = 240, 245, 252
            elif x in (w - 2,) or y in (h - 2,):
                r, g, b = 95, 104, 116

            # Left circular iridescent MiniDisc window
            dx, dy = x - 34, y - 40
            dist = math.hypot(dx, dy)
            if dist < 27:
                ang = math.atan2(dy, dx)
                if dist < 6.5:
                    r, g, b = 48, 54, 64
                elif dist < 10.5:
                    r, g, b = 185, 195, 208
                else:
                    shimmer = math.sin(ang * 2.0 + 0.6)
                    r = 135 + int(55 * math.sin(ang * 2.0 + 0.2)) + int(25 * shimmer)
                    g = 195 + int(45 * math.sin(ang * 2.0 + 1.8)) + int(25 * shimmer)
                    b = 225 + int(30 * math.sin(ang * 2.0 + 3.4))
            elif 27 <= dist < 29.2:
                r, g, b = 52, 60, 70

            # Right upper Ice-Cyan EL LCD screen (x: 70..135, y: 12..42)
            if 70 <= x <= 135 and 12 <= y <= 42:
                if x in (70, 135) or y in (12, 42):
                    r, g, b = 32, 38, 46
                else:
                    r, g, b = 10, 44, 56
                    if y % 3 == 0:
                        r, g, b = 8, 36, 46
                    # Fake glowing LCD bars/digits
                    if 17 <= y <= 21 and 75 <= x <= 128 and (x % 9 != 0):
                        r, g, b = 156, 248, 255
                    if 26 <= y <= 31 and 75 <= x <= 118 and (x % 5 != 0):
                        r, g, b = 120, 224, 240
                    if 35 <= y <= 38 and 75 <= x <= 130:
                        r, g, b = (165, 250, 255) if x < 112 else (28, 86, 102)

            # Right lower Warm-Amber VU Meter strip (x: 70..135, y: 47..68)
            if 70 <= x <= 135 and 47 <= y <= 68:
                if x in (70, 102, 103, 135) or y in (47, 68):
                    r, g, b = 38, 42, 50
                else:
                    r, g, b = 244, 216, 156
                    # Red overload arc on right side of each mini VU
                    local_x = (x - 71) if x < 102 else (x - 104)
                    if 51 <= y <= 53 and local_x >= 20:
                        r, g, b = 210, 42, 34
                    elif 52 <= y <= 53 and 4 <= local_x < 20:
                        r, g, b = 70, 52, 34
                    # Diagonal red needle
                    if abs((local_x - 16) + (y - 58) * 0.7) < 1.1 and y > 51:
                        r, g, b = 205, 36, 28

            idx = (y * w + x) * 3
            buf[idx] = clamp8(r)
            buf[idx + 1] = clamp8(g)
            buf[idx + 2] = clamp8(b)

    write_png(path, w, h, bytes(buf))


def generate_pic1(path: str) -> None:
    """480x272 PSP XMB Fullscreen Backdrop: Brushed Gunmetal + Y2K Optical Cyan/Amber."""
    w, h = 480, 272
    buf = bytearray(w * h * 3)
    cx, cy = 355.0, 136.0
    for y in range(h):
        ny = y / float(h - 1)
        hair = 3 if (y % 2 == 0) else -2
        for x in range(w):
            nx = x / float(w - 1)
            base = 28 + int(26 * (1.0 - ny * 0.7)) + int(14 * nx) + hair
            r, g, b = base, base + 6, base + 14

            # Subtle glowing Y2K optical disc rings on right side
            dx, dy = x - cx, y - cy
            dist = math.hypot(dx, dy)
            if 34.0 < dist < 112.0:
                ang = math.atan2(dy, dx)
                lobe = max(0.0, math.sin(ang * 2.0 + 0.7))
                groove = 0.5 + 0.5 * math.sin(dist * 0.45)
                r += int(18 * groove + 22 * lobe)
                g += int(42 * groove + 54 * lobe)
                b += int(56 * groove + 68 * lobe)
            elif abs(dist - 113.0) < 1.5 or abs(dist - 33.0) < 1.5:
                r, g, b = 92, 198, 224

            # Warm amber VU accent line along bottom
            if 246 <= y <= 248 and 24 <= x <= 456:
                r, g, b = (240, 168, 54) if x < 340 else (90, 215, 238)

            idx = (y * w + x) * 3
            buf[idx] = clamp8(r)
            buf[idx + 1] = clamp8(g)
            buf[idx + 2] = clamp8(b)

    write_png(path, w, h, bytes(buf))


def generate_album_cover_png(path: str, palette_rgb: tuple[int, int, int], variant: int) -> None:
    """256x256 rich Y2K synth/MiniDisc cover art PNG."""
    w, h = 256, 256
    buf = bytearray(w * h * 3)
    pr, pg, pb = palette_rgb
    for y in range(h):
        ny = y / 255.0
        for x in range(w):
            nx = x / 255.0
            # Y2K gradient horizon + retro grid + iridescent disc
            r = int(18 + pr * (1.0 - ny) * 0.65 + 30 * nx)
            g = int(22 + pg * (1.0 - ny * 0.8) * 0.65)
            b = int(34 + pb * (0.4 + 0.6 * ny))

            dx, dy = x - 128.0, y - 108.0
            dist = math.hypot(dx, dy)
            if dist < 68.0:
                ang = math.atan2(dy, dx)
                if dist > 16.0:
                    wave = math.sin(ang * (2.0 + variant) + dist * 0.06)
                    r = clamp8(r * 0.35 + 155 + 65 * wave)
                    g = clamp8(g * 0.35 + 185 + 55 * math.sin(ang * 2.0 + 1.4))
                    b = clamp8(b * 0.35 + 215 + 40 * math.cos(ang * 2.0))
                else:
                    r, g, b = 36, 42, 52

            # Perspective Y2K cyber grid on lower third
            if y > 176:
                gy = y - 176
                if gy % 12 == 0 or int((x - 128) * 32 / max(6, gy)) % 8 == 0:
                    r = clamp8(r + 65)
                    g = clamp8(g + 110)
                    b = clamp8(b + 125)

            # Crisp frame border
            if x < 6 or x >= w - 6 or y < 6 or y >= h - 6:
                r, g, b = 220, 228, 238

            idx = (y * w + x) * 3
            buf[idx] = clamp8(r)
            buf[idx + 1] = clamp8(g)
            buf[idx + 2] = clamp8(b)

    write_png(path, w, h, bytes(buf))


def generate_demo_mp3s() -> None:
    ffmpeg = shutil.which("ffmpeg") or "/opt/homebrew/bin/ffmpeg"
    if not os.path.exists(ffmpeg):
        print("ffmpeg not found; skipping optional demo MP3 generation.")
        return

    os.makedirs(DEMO_MUSIC_DIR, exist_ok=True)
    cov1_png = os.path.join(ASSETS_DIR, "demo_cover_1.png")
    cov2_png = os.path.join(ASSETS_DIR, "demo_cover_2.png")
    cov1_jpg = os.path.join(ASSETS_DIR, "demo_cover_1.jpg")
    cov2_jpg = os.path.join(ASSETS_DIR, "demo_cover_2.jpg")

    generate_album_cover_png(cov1_png, (40, 195, 235), 0)
    generate_album_cover_png(cov2_png, (235, 145, 50), 1)

    # Also place a folder cover.png in demo music directory
    shutil.copyfile(cov1_png, os.path.join(DEMO_MUSIC_DIR, "cover.png"))

    for src_png, dst_jpg in [(cov1_png, cov1_jpg), (cov2_png, cov2_jpg)]:
        subprocess.run(
            [ffmpeg, "-y", "-loglevel", "error", "-i", src_png, "-q:v", "3", dst_jpg],
            check=True,
        )

    # Demo Track 1: Stereo Y2K Warm Chord Progression with L/R stereo movement (18s)
    mp3_1 = os.path.join(DEMO_MUSIC_DIR, "01 - Midnight MiniDisc.mp3")
    expr_l1 = (
        "0.20*sin(2*PI*110*t)*(0.6+0.4*sin(2*PI*2*t))"
        "+0.14*sin(2*PI*220*t)+0.12*sin(2*PI*261.63*t)"
        "+0.10*sin(2*PI*329.63*t)*(0.5+0.5*sin(2*PI*0.5*t))"
        "+0.07*sin(2*PI*659.25*t)*(0.5+0.5*cos(2*PI*3*t))"
    )
    expr_r1 = (
        "0.20*sin(2*PI*110*t)*(0.6+0.4*cos(2*PI*2*t))"
        "+0.14*sin(2*PI*220.5*t)+0.12*sin(2*PI*329.63*t)"
        "+0.10*sin(2*PI*392.00*t)*(0.5+0.5*cos(2*PI*0.5*t))"
        "+0.07*sin(2*PI*783.99*t)*(0.5+0.5*sin(2*PI*3*t))"
    )
    subprocess.run(
        [
            ffmpeg, "-y", "-loglevel", "error",
            "-f", "lavfi",
            "-i", f"aevalsrc='{expr_l1}|{expr_r1}':s=44100:d=18",
            "-i", cov1_jpg,
            "-map", "0:a", "-map", "1:v",
            "-c:a", "libmp3lame", "-b:a", "192k",
            "-c:v", "mjpeg",
            "-id3v2_version", "3",
            "-metadata:s:v", "title=Album cover",
            "-metadata:s:v", "comment=Cover (front)",
            "-metadata", "title=Midnight MiniDisc (Y2K Mix)",
            "-metadata", "artist=E-RENCYK Analog Lab",
            "-metadata", "album=2001 Portable Hi-Fi Sessions",
            "-metadata", "date=2001",
            "-af", "afade=t=in:st=0:d=0.6,afade=t=out:st=16.5:d=1.5",
            mp3_1,
        ],
        check=True,
    )

    # Demo Track 2: Upbeat Y2K Electro-Chill Pulse (18s)
    mp3_2 = os.path.join(DEMO_MUSIC_DIR, "02 - Silver Walkman Dreams.mp3")
    expr_l2 = (
        "0.22*sin(2*PI*98*t)*(0.55+0.45*sin(2*PI*4*t))"
        "+0.14*sin(2*PI*196*t)+0.12*sin(2*PI*293.66*t)"
        "+0.10*sin(2*PI*440*t)*(0.5+0.5*sin(2*PI*1.5*t))"
        "+0.06*sin(2*PI*880*t)*(0.5+0.5*sin(2*PI*6*t))"
    )
    expr_r2 = (
        "0.22*sin(2*PI*98*t)*(0.55+0.45*sin(2*PI*4*t+0.8))"
        "+0.14*sin(2*PI*246.94*t)+0.12*sin(2*PI*369.99*t)"
        "+0.10*sin(2*PI*587.33*t)*(0.5+0.5*cos(2*PI*1.5*t))"
        "+0.06*sin(2*PI*1174.66*t)*(0.5+0.5*cos(2*PI*6*t))"
    )
    subprocess.run(
        [
            ffmpeg, "-y", "-loglevel", "error",
            "-f", "lavfi",
            "-i", f"aevalsrc='{expr_l2}|{expr_r2}':s=44100:d=18",
            "-i", cov2_jpg,
            "-map", "0:a", "-map", "1:v",
            "-c:a", "libmp3lame", "-b:a", "192k",
            "-c:v", "mjpeg",
            "-id3v2_version", "3",
            "-metadata:s:v", "title=Album cover",
            "-metadata:s:v", "comment=Cover (front)",
            "-metadata", "title=Silver Walkman Dreams",
            "-metadata", "artist=E-RENCYK Analog Lab",
            "-metadata", "album=2001 Portable Hi-Fi Sessions",
            "-metadata", "date=2002",
            "-af", "afade=t=in:st=0:d=0.6,afade=t=out:st=16.5:d=1.5",
            mp3_2,
        ],
        check=True,
    )

    for tmp in (cov1_png, cov2_png, cov1_jpg, cov2_jpg):
        if os.path.exists(tmp):
            os.remove(tmp)


def main() -> None:
    os.makedirs(ASSETS_DIR, exist_ok=True)
    icon_path = os.path.join(ASSETS_DIR, "ICON0.PNG")
    pic1_path = os.path.join(ASSETS_DIR, "PIC1.PNG")
    if not os.path.exists(icon_path):
        generate_icon0(icon_path)
    if not os.path.exists(pic1_path):
        generate_pic1(pic1_path)
    generate_demo_mp3s()
    print("Assets ready:", icon_path, pic1_path)


if __name__ == "__main__":
    main()
