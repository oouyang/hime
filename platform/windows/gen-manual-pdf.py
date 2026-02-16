#!/usr/bin/env python3
"""
Generate the HIME Windows User Manual as a PDF.

Usage:
    python3 gen-manual-pdf.py

Output:
    windows/HIME-Manual.pdf
"""

import os
import sys

from reportlab.lib.pagesizes import A4
from reportlab.lib.units import mm, cm
from reportlab.lib.colors import (
    HexColor, black, white, gray, darkgray, lightgrey,
)
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.enums import TA_LEFT, TA_CENTER, TA_RIGHT, TA_JUSTIFY
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle,
    PageBreak, KeepTogether, Image, HRFlowable,
)
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ICONS_DIR = os.path.join(SCRIPT_DIR, "icons")
ROOT_ICONS = os.path.join(SCRIPT_DIR, "..", "icons")
OUTPUT_PDF = os.path.join(SCRIPT_DIR, "HIME-Manual.pdf")

# ---------------------------------------------------------------------------
# CJK Font Registration
# ---------------------------------------------------------------------------
# Register a CJK font alongside Helvetica. Latin text stays in Helvetica
# (proper bold/italic), CJK characters are wrapped in <font> tags via the
# cjk() helper below.
_CJK_FONT_CANDIDATES = [
    # Linux
    "/usr/share/fonts/truetype/droid/DroidSansFallbackFull.ttf",
    "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
    "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
    "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
    # macOS
    "/System/Library/Fonts/PingFang.ttc",
    "/System/Library/Fonts/STHeiti Light.ttc",
    "/Library/Fonts/Arial Unicode.ttf",
    # Windows
    os.path.expandvars(r"%SystemRoot%\Fonts\msjh.ttc"),
    os.path.expandvars(r"%SystemRoot%\Fonts\msyh.ttc"),
    os.path.expandvars(r"%SystemRoot%\Fonts\mingliu.ttc"),
    os.path.expandvars(r"%SystemRoot%\Fonts\simsun.ttc"),
    r"C:\Windows\Fonts\msjh.ttc",
    r"C:\Windows\Fonts\msyh.ttc",
]

CJK_FONT = "HimeCJK"
_cjk_available = False

for _font_path in _CJK_FONT_CANDIDATES:
    if os.path.exists(_font_path):
        try:
            if _font_path.endswith(".ttc"):
                pdfmetrics.registerFont(TTFont(CJK_FONT, _font_path, subfontIndex=0))
            else:
                pdfmetrics.registerFont(TTFont(CJK_FONT, _font_path))
            _cjk_available = True
            break
        except Exception as e:
            print(f"Warning: failed to load font {_font_path}: {e}")

if not _cjk_available:
    print("WARNING: No CJK font found. Chinese characters may not render correctly.")
    print("Install a CJK font (e.g. Noto Sans CJK, Droid Sans Fallback, or PingFang).")

# ---------------------------------------------------------------------------
# Colors (HIME brand)
# ---------------------------------------------------------------------------
C_PRIMARY = HexColor("#2E5A88")      # Deep blue
C_ACCENT = HexColor("#E85D2C")       # Warm orange
C_LIGHT_BG = HexColor("#F4F7FA")     # Light background
C_DARK_TEXT = HexColor("#1A1A2E")     # Near-black
C_MID_GRAY = HexColor("#666666")
C_TABLE_HEAD = HexColor("#2E5A88")
C_TABLE_ALT = HexColor("#EEF2F7")
C_KEY_BG = HexColor("#E8E8E8")

# ---------------------------------------------------------------------------
# Styles
# ---------------------------------------------------------------------------
styles = getSampleStyleSheet()

S_TITLE = ParagraphStyle(
    "HimeTitle", parent=styles["Title"],
    fontSize=28, leading=34,
    textColor=C_PRIMARY, spaceAfter=4*mm,
    alignment=TA_CENTER,
)
S_SUBTITLE = ParagraphStyle(
    "HimeSubtitle", parent=styles["Normal"],
    fontSize=13, leading=17,
    textColor=C_MID_GRAY, spaceAfter=12*mm,
    alignment=TA_CENTER,
)
S_H1 = ParagraphStyle(
    "HimeH1", parent=styles["Heading1"],
    fontSize=20, leading=26,
    textColor=C_PRIMARY, spaceBefore=10*mm, spaceAfter=4*mm,
    borderWidth=0, borderPadding=0,
)
S_H2 = ParagraphStyle(
    "HimeH2", parent=styles["Heading2"],
    fontSize=14, leading=19,
    textColor=C_PRIMARY, spaceBefore=6*mm, spaceAfter=3*mm,
)
S_H3 = ParagraphStyle(
    "HimeH3", parent=styles["Heading3"],
    fontSize=12, leading=16,
    textColor=C_DARK_TEXT, spaceBefore=4*mm, spaceAfter=2*mm,
)
S_BODY = ParagraphStyle(
    "HimeBody", parent=styles["Normal"],
    fontSize=10.5, leading=16,
    textColor=C_DARK_TEXT, spaceAfter=3*mm,
    alignment=TA_JUSTIFY,
)
S_BODY_C = ParagraphStyle(
    "HimeBodyCenter", parent=S_BODY,
    alignment=TA_CENTER,
)
S_BULLET = ParagraphStyle(
    "HimeBullet", parent=S_BODY,
    leftIndent=14*mm, bulletIndent=6*mm,
    spaceAfter=1.5*mm,
)
S_CODE = ParagraphStyle(
    "HimeCode", parent=styles["Code"],
    fontSize=9.5, leading=13,
    textColor=C_DARK_TEXT,
    backColor=C_LIGHT_BG,
    borderWidth=0.5, borderColor=lightgrey, borderPadding=6,
    spaceAfter=3*mm,
    leftIndent=6*mm,
)
S_NOTE = ParagraphStyle(
    "HimeNote", parent=S_BODY,
    fontSize=9.5, leading=14,
    textColor=C_MID_GRAY,
    leftIndent=8*mm, rightIndent=8*mm,
    spaceBefore=2*mm, spaceAfter=3*mm,
)
S_TH = ParagraphStyle(
    "HimeTH", parent=S_BODY,
    fontSize=10, leading=14,
    textColor=white, alignment=TA_CENTER,
)
S_TD = ParagraphStyle(
    "HimeTD", parent=S_BODY,
    fontSize=10, leading=14,
    textColor=C_DARK_TEXT, alignment=TA_CENTER,
    spaceAfter=0,
)
S_TD_L = ParagraphStyle(
    "HimeTDL", parent=S_TD,
    alignment=TA_LEFT,
)
S_FOOTER = ParagraphStyle(
    "HimeFooter", parent=styles["Normal"],
    fontSize=8, leading=10,
    textColor=C_MID_GRAY, alignment=TA_CENTER,
)
S_COVER_CHAR = ParagraphStyle(
    "HimeCoverChar", parent=styles["Normal"],
    fontSize=72, leading=80,
    textColor=C_PRIMARY, alignment=TA_CENTER,
    spaceAfter=2*mm,
)

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
import re

def cjk(text):
    """Wrap CJK characters in <font> tags so they render with the CJK font.

    Keeps Helvetica for Latin text (proper bold/italic) while switching to
    the registered CJK font for Chinese/Japanese/Korean characters.
    Returns the original text unchanged if no CJK font is available.
    """
    if not _cjk_available:
        return text
    # Match runs of CJK Unified Ideographs, CJK symbols, Bopomofo,
    # fullwidth forms, and CJK punctuation
    _CJK_RE = re.compile(
        r'([\u2E80-\u2FFF'   # CJK radicals, Kangxi
        r'\u3000-\u303F'      # CJK symbols and punctuation
        r'\u3100-\u312F'      # Bopomofo
        r'\u31A0-\u31BF'      # Bopomofo Extended
        r'\u3400-\u4DBF'      # CJK Extension A
        r'\u4E00-\u9FFF'      # CJK Unified Ideographs
        r'\uF900-\uFAFF'      # CJK Compatibility
        r'\uFE30-\uFE4F'      # CJK Compatibility Forms
        r'\uFF00-\uFFEF'      # Fullwidth Forms
        r'\U00020000-\U0002A6DF'  # CJK Extension B
        r']+)'
    )
    return _CJK_RE.sub(
        rf'<font face="{CJK_FONT}">\1</font>', text
    )


def icon_path(name):
    p = os.path.join(ICONS_DIR, name)
    if os.path.exists(p):
        return p
    p2 = os.path.join(ROOT_ICONS, name)
    if os.path.exists(p2):
        return p2
    return None

def img_or_none(name, w=12*mm, h=12*mm):
    p = icon_path(name)
    if p:
        return Image(p, width=w, height=h)
    return None

def hr():
    return HRFlowable(width="100%", thickness=0.5, color=lightgrey,
                       spaceBefore=2*mm, spaceAfter=2*mm)

def kbd(key):
    """Render a keyboard key inline."""
    return f'<font face="Courier" size="10" color="#333333"><b>&nbsp;{key}&nbsp;</b></font>'

def P(text, style):
    """Create a Paragraph with CJK characters auto-wrapped."""
    return Paragraph(cjk(text), style)


def make_table(headers, rows, col_widths=None):
    """Create a styled table."""
    data = [[P(h, S_TH) for h in headers]]
    for row in rows:
        data.append([P(str(c), S_TD_L if i == 0 else S_TD)
                      for i, c in enumerate(row)])

    t = Table(data, colWidths=col_widths, repeatRows=1)
    style_cmds = [
        ("BACKGROUND", (0, 0), (-1, 0), C_TABLE_HEAD),
        ("TEXTCOLOR", (0, 0), (-1, 0), white),
        ("ALIGN", (0, 0), (-1, 0), "CENTER"),
        ("VALIGN", (0, 0), (-1, -1), "MIDDLE"),
        ("GRID", (0, 0), (-1, -1), 0.5, lightgrey),
        ("ROWBACKGROUNDS", (0, 1), (-1, -1), [white, C_TABLE_ALT]),
        ("TOPPADDING", (0, 0), (-1, -1), 4),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 4),
        ("LEFTPADDING", (0, 0), (-1, -1), 6),
        ("RIGHTPADDING", (0, 0), (-1, -1), 6),
    ]
    t.setStyle(TableStyle(style_cmds))
    return t

# ---------------------------------------------------------------------------
# Page template callbacks
# ---------------------------------------------------------------------------
def _on_first_page(canvas, doc):
    pass  # Cover page — no header/footer

def _on_later_pages(canvas, doc):
    canvas.saveState()
    canvas.setFont("Helvetica", 8)
    canvas.setFillColor(C_MID_GRAY)
    w, h = A4
    canvas.drawString(20*mm, 10*mm, "HIME Input Method Editor — User Manual")
    canvas.drawRightString(w - 20*mm, 10*mm, f"Page {doc.page}")
    # Top line
    canvas.setStrokeColor(lightgrey)
    canvas.setLineWidth(0.5)
    canvas.line(20*mm, h - 18*mm, w - 20*mm, h - 18*mm)
    # Bottom line
    canvas.line(20*mm, 14*mm, w - 20*mm, 14*mm)
    canvas.restoreState()

# ---------------------------------------------------------------------------
# Content builder
# ---------------------------------------------------------------------------
def build_content():
    story = []

    # ===== COVER PAGE =====
    story.append(Spacer(1, 40*mm))
    story.append(P("姬", S_COVER_CHAR))
    story.append(P("HIME Input Method Editor", S_TITLE))
    story.append(P("Windows User Manual", S_SUBTITLE))
    story.append(Spacer(1, 10*mm))

    # Icon row
    icon_names = ["juyin.png", "cj5.png", "noseeing.png", "hime-tray.png"]
    icon_imgs = []
    for name in icon_names:
        img = img_or_none(name, w=16*mm, h=16*mm)
        if img:
            icon_imgs.append(img)
        else:
            icon_imgs.append(P("?", S_BODY_C))
    if icon_imgs:
        t = Table([icon_imgs], colWidths=[30*mm]*len(icon_imgs))
        t.setStyle(TableStyle([
            ("ALIGN", (0,0), (-1,-1), "CENTER"),
            ("VALIGN", (0,0), (-1,-1), "MIDDLE"),
        ]))
        story.append(t)

    story.append(Spacer(1, 15*mm))
    story.append(P(
        "A lightweight input method editor for Traditional Chinese,<br/>"
        "supporting Zhuyin (注音), Cangjie (倉頡), and Boshiamy (嘸蝦米).",
        S_BODY_C
    ))
    story.append(Spacer(1, 20*mm))
    story.append(P("Version 0.10.1 — February 2026", S_NOTE))
    story.append(P("https://github.com/nicehime/hime", S_NOTE))
    story.append(PageBreak())

    # ===== TABLE OF CONTENTS =====
    story.append(P("Table of Contents", S_H1))
    story.append(hr())
    toc_items = [
        ("1.", "Installation", "3"),
        ("2.", "Getting Started", "4"),
        ("3.", "System Tray &amp; Mode Switching", "5"),
        ("4.", "Zhuyin (注音) Input", "7"),
        ("5.", "Cangjie 5 (倉五) Input", "9"),
        ("6.", "Boshiamy (嘸蝦米) Input", "10"),
        ("7.", "Right-Click Menu", "11"),
        ("8.", "Settings", "12"),
        ("9.", "Floating Toolbar", "13"),
        ("10.", "Keyboard Shortcuts", "14"),
        ("11.", "Troubleshooting", "15"),
    ]
    toc_data = []
    for num, title, page in toc_items:
        toc_data.append([
            P(f'<b>{num}</b>', S_TD_L),
            P(cjk(title), S_TD_L),
            P(page, S_TD),
        ])
    toc_t = Table(toc_data, colWidths=[12*mm, 110*mm, 15*mm])
    toc_t.setStyle(TableStyle([
        ("VALIGN", (0,0), (-1,-1), "MIDDLE"),
        ("TOPPADDING", (0,0), (-1,-1), 3),
        ("BOTTOMPADDING", (0,0), (-1,-1), 3),
        ("LINEBELOW", (0,0), (-1,-1), 0.3, lightgrey),
    ]))
    story.append(toc_t)
    story.append(PageBreak())

    # ===== 1. INSTALLATION =====
    story.append(P("1. Installation", S_H1))
    story.append(hr())

    story.append(P("1.1 System Requirements", S_H2))
    for item in [
        "Windows 10 or Windows 11 (64-bit)",
        "50 MB free disk space",
        "Administrator privileges (for installation only)",
    ]:
        story.append(P(f"• {item}", S_BULLET))

    story.append(P("1.2 Install", S_H2))
    story.append(P(
        "Run <b>hime-install.exe</b>. An administrator prompt (UAC) will appear — "
        "click <b>Yes</b> to continue.",
        S_BODY,
    ))
    for step in [
        "The installer copies files to <b>C:\\Program Files\\HIME\\</b>.",
        "It registers the IME with Windows automatically.",
        "An entry appears in <b>Settings → Apps</b> for uninstalling later.",
    ]:
        story.append(P(f"• {step}", S_BULLET))
    story.append(P(
        "<i>To upgrade, simply run hime-install.exe again — it removes the old version first.</i>",
        S_NOTE,
    ))

    story.append(P("1.3 Enable HIME in Windows", S_H2))
    for i, step in enumerate([
        "Open <b>Settings → Time &amp; Language → Language &amp; region</b>.",
        "Click your language (e.g. 中文 (繁體，台灣)) → <b>Language options</b>.",
        'Under "Keyboards", click <b>Add a keyboard</b>.',
        'Select <b>"姬 HIME"</b> (or "姬 HIME [DEBUG]" for debug builds).',
    ], 1):
        story.append(P(f"{i}. {step}", S_BULLET))
    story.append(P(
        "HIME will now appear in the language bar / system tray when you switch to it.",
        S_BODY,
    ))

    story.append(P("1.4 Uninstall", S_H2))
    story.append(P(
        "Run <b>hime-uninstall.exe</b> from C:\\Program Files\\HIME\\, "
        "or use <b>Settings → Apps → HIME Input Method Editor → Uninstall</b>.",
        S_BODY,
    ))
    story.append(PageBreak())

    # ===== 2. GETTING STARTED =====
    story.append(P("2. Getting Started", S_H1))
    story.append(hr())

    story.append(P("2.1 Switching to HIME", S_H2))
    story.append(P(
        "Press <b>Win + Space</b> to cycle through installed input methods. "
        "When HIME is active, you will see the HIME icon (姬) in the system tray "
        "alongside a mode indicator icon showing your current input method.",
        S_BODY,
    ))

    story.append(P("2.2 System Tray Icons", S_H2))
    story.append(P(
        "When HIME is active, two icons appear in the system tray area:",
        S_BODY,
    ))

    tray_headers = ["Icon", "Meaning"]
    tray_rows = [
        ["姬 (purple)", "HIME app icon — right-click for menu"],
        ["注 (blue)", "Zhuyin (注音) mode active"],
        ["倉 (green)", "Cangjie 5 (倉五) mode active"],
        ["蝦 (orange)", "Boshiamy (嘸蝦米) mode active"],
        ["EN (gray)", "English passthrough mode"],
    ]
    story.append(make_table(tray_headers, tray_rows, col_widths=[40*mm, 100*mm]))

    story.append(P("2.3 Quick Mode Switching", S_H2))
    story.append(P(
        "Click the <b>mode indicator icon</b> (注/倉/蝦/EN) to cycle through "
        "enabled input methods. You can also use keyboard shortcuts:",
        S_BODY,
    ))
    for item in [
        "<b>Ctrl + `</b> (backtick) — Cycle: EN → Zhuyin → Cangjie → [Boshiamy] → EN",
        "<b>F4</b> — Same cycle as Ctrl + `",
    ]:
        story.append(P(f"• {item}", S_BULLET))
    story.append(P(
        "<i>Only enabled methods participate in the cycle. "
        "Use Settings to enable/disable methods.</i>",
        S_NOTE,
    ))
    story.append(PageBreak())

    # ===== 3. SYSTEM TRAY & MODE SWITCHING =====
    story.append(P("3. System Tray &amp; Mode Switching", S_H1))
    story.append(hr())

    story.append(P("3.1 Notification Area Icon", S_H2))
    story.append(P(
        "HIME places a <b>standalone icon</b> in the Windows notification area "
        "(system tray, near the clock). This icon is always visible when HIME is active, "
        "regardless of language bar settings — unlike the TSF language bar buttons which "
        "Windows 10/11 hides behind the system input indicator.",
        S_BODY,
    ))
    story.append(P(
        "<b>Left-click</b> the tray icon to cycle through enabled input methods.",
        S_BULLET,
    ))
    story.append(P(
        "<b>Right-click</b> the tray icon to open a popup menu with input method "
        "selection, toolbar toggle, Settings, and About.",
        S_BULLET,
    ))
    story.append(P(
        "The icon updates to reflect the current mode "
        "(注 for Zhuyin, 倉 for Cangjie, EN for English, etc.), "
        "and the tooltip shows the active method name.",
        S_BULLET,
    ))

    story.append(P("3.2 Language Bar Icons", S_H2))
    story.append(P(
        "In addition to the notification area icon, HIME also registers "
        "<b>two TSF language bar buttons</b>:",
        S_BODY,
    ))
    story.append(P(
        "<b>① HIME App Icon (姬)</b> — The main language bar button. "
        "Left-click cycles modes. Right-click opens the context menu.",
        S_BULLET,
    ))
    story.append(P(
        "<b>② Mode Indicator</b> — Shows the current input mode "
        "(注 for Zhuyin, 倉 for Cangjie, 蝦 for Boshiamy, EN for English). "
        "Left-click cycles modes.",
        S_BULLET,
    ))
    story.append(P(
        "<i>Note: On Windows 10/11, the language bar buttons may only be visible if you "
        "enable the desktop language bar in Settings → Time &amp; Language → Typing → "
        "Advanced keyboard settings. The notification area icon works regardless.</i>",
        S_NOTE,
    ))

    story.append(P("3.3 Mode Cycle Order", S_H2))
    story.append(P(
        "Each click or Ctrl+` press moves to the next enabled method in this order:",
        S_BODY,
    ))
    # Cycle diagram as a table
    cycle_data = [
        [P("<b>EN</b><br/>(English)", S_TD),
         P("→", S_TD),
         P("<b>注音</b><br/>(Zhuyin)", S_TD),
         P("→", S_TD),
         P("<b>倉五</b><br/>(Cangjie)", S_TD),
         P("→", S_TD),
         P("<b>嘸蝦米</b><br/>(Boshiamy)", S_TD),
         P("→", S_TD),
         P("<b>EN</b><br/>...", S_TD)],
    ]
    cycle_t = Table(cycle_data, colWidths=[20*mm, 8*mm, 20*mm, 8*mm, 20*mm, 8*mm, 20*mm, 8*mm, 20*mm])
    cycle_t.setStyle(TableStyle([
        ("ALIGN", (0,0), (-1,-1), "CENTER"),
        ("VALIGN", (0,0), (-1,-1), "MIDDLE"),
        ("BOX", (0,0), (0,0), 1, C_PRIMARY),
        ("BOX", (2,0), (2,0), 1, HexColor("#005AB4")),
        ("BOX", (4,0), (4,0), 1, HexColor("#00823C")),
        ("BOX", (6,0), (6,0), 1, HexColor("#B45A00")),
        ("BOX", (8,0), (8,0), 1, C_PRIMARY),
        ("BACKGROUND", (0,0), (0,0), HexColor("#F0F0F0")),
        ("BACKGROUND", (2,0), (2,0), HexColor("#E8F0FF")),
        ("BACKGROUND", (4,0), (4,0), HexColor("#E8FFE8")),
        ("BACKGROUND", (6,0), (6,0), HexColor("#FFF0E0")),
        ("BACKGROUND", (8,0), (8,0), HexColor("#F0F0F0")),
    ]))
    story.append(cycle_t)
    story.append(Spacer(1, 3*mm))
    story.append(P(
        "<i>Disabled methods are skipped. Boshiamy only appears if liu.gtab is installed.</i>",
        S_NOTE,
    ))

    story.append(P("3.4 Switching Directly via Menu", S_H2))
    story.append(P(
        "Right-click the HIME app icon (姬) and select the desired input method "
        "from the menu. A checkmark (✓) indicates the currently active method.",
        S_BODY,
    ))
    story.append(PageBreak())

    # ===== 4. ZHUYIN (注音) INPUT =====
    story.append(P("4. Zhuyin (注音) Input", S_H1))
    story.append(hr())

    story.append(P(
        "Zhuyin (Bopomofo) is the standard phonetic input method for Traditional Chinese, "
        "widely used in Taiwan.",
        S_BODY,
    ))

    story.append(P("4.1 Keyboard Layout", S_H2))
    story.append(P(
        "The standard Zhuyin keyboard layout maps Bopomofo symbols to letter keys:",
        S_BODY,
    ))

    # Keyboard layout table - row 1 (number row)
    kb_headers = ["Key", "Symbol", "Key", "Symbol", "Key", "Symbol", "Key", "Symbol"]

    kb_row1 = [
        ["1", "ㄅ", "q", "ㄆ", "2", "ㄉ", "w", "ㄊ"],
        ["3", "ˇ", "e", "ㄍ", "4", "ˋ", "r", "ㄐ"],
        ["5", "ㄓ", "t", "ㄔ", "6", "ˊ", "y", "ㄗ"],
        ["7", "˙", "u", "ㄧ", "8", "ㄚ", "i", "ㄛ"],
        ["9", "ㄞ", "o", "ㄟ", "0", "ㄢ", "p", "ㄣ"],
    ]
    story.append(make_table(kb_headers, kb_row1,
                            col_widths=[12*mm, 14*mm]*4))

    kb_row2 = [
        ["a", "ㄇ", "s", "ㄋ", "d", "ㄎ", "f", "ㄑ"],
        ["g", "ㄕ", "h", "ㄘ", "j", "ㄨ", "k", "ㄜ"],
        ["l", "ㄠ", ";", "ㄤ", "-", "ㄦ", "", ""],
    ]
    story.append(Spacer(1, 2*mm))
    story.append(make_table(kb_headers, kb_row2,
                            col_widths=[12*mm, 14*mm]*4))

    kb_row3 = [
        ["z", "ㄈ", "x", "ㄌ", "c", "ㄏ", "v", "ㄒ"],
        ["b", "ㄖ", "n", "ㄙ", "m", "ㄩ", ",", "ㄝ"],
        [".", "ㄡ", "/", "ㄥ", "", "", "", ""],
    ]
    story.append(Spacer(1, 2*mm))
    story.append(make_table(kb_headers, kb_row3,
                            col_widths=[12*mm, 14*mm]*4))

    story.append(P("4.2 Tone Keys", S_H2))
    tone_headers = ["Key", "Tone", "Name", "Example"]
    tone_rows = [
        ["Space", "First (ˉ)", "陰平 yīnpíng", "媽 mā"],
        ["6", "Second (ˊ)", "陽平 yángpíng", "麻 má"],
        ["3", "Third (ˇ)", "上聲 shàngshēng", "馬 mǎ"],
        ["4", "Fourth (ˋ)", "去聲 qùshēng", "罵 mà"],
        ["7", "Neutral (˙)", "輕聲 qīngshēng", "嗎 ma"],
    ]
    story.append(make_table(tone_headers, tone_rows,
                            col_widths=[18*mm, 28*mm, 42*mm, 30*mm]))

    story.append(P("4.3 Input Flow", S_H2))
    for i, step in enumerate([
        "Type the Bopomofo consonant, medial, and vowel keys.",
        "Press a tone key (Space for 1st tone, 3/4/6/7 for others).",
        "A candidate list appears — press a number key (1–9, 0) to select.",
        "The selected character is inserted into your document.",
    ], 1):
        story.append(P(f"<b>{i}.</b> {step}", S_BULLET))

    story.append(P("4.4 Example: Typing 你好 (nǐ hǎo)", S_H2))
    example_headers = ["Step", "Keys", "Preedit", "Result"]
    example_rows = [
        ["1", "j (ㄋ) → u (ㄧ)", "[注]ㄋㄧ", "—"],
        ["2", "3 (third tone)", "[注]ㄋㄧˇ 1.你 2.妳", "—"],
        ["3", "1 (select)", "", "你"],
        ["4", "c (ㄏ) → 8 (ㄚ) → o (ㄟ)", "[注]ㄏㄠ", "—"],
        ["5", "3 (third tone)", "[注]ㄏㄠˇ 1.好 2.郝", "—"],
        ["6", "1 (select)", "", "你好"],
    ]
    story.append(make_table(example_headers, example_rows,
                            col_widths=[14*mm, 40*mm, 46*mm, 20*mm]))
    story.append(PageBreak())

    # ===== 5. CANGJIE 5 (倉五) INPUT =====
    story.append(P("5. Cangjie 5 (倉五) Input", S_H1))
    story.append(hr())

    story.append(P(
        "Cangjie is a shape-based input method that decomposes Chinese characters "
        "into components mapped to letter keys. Cangjie 5 is the most widely used version.",
        S_BODY,
    ))

    story.append(P("5.1 Key Layout", S_H2))
    story.append(P(
        "Each letter A–Y maps to a Cangjie radical. Z is not used in standard Cangjie.",
        S_BODY,
    ))
    cj_headers = ["Key", "Radical", "Key", "Radical", "Key", "Radical", "Key", "Radical", "Key", "Radical"]
    cj_rows = [
        ["A", "日", "B", "月", "C", "金", "D", "木", "E", "水"],
        ["F", "火", "G", "土", "H", "竹", "I", "戈", "J", "十"],
        ["K", "大", "L", "中", "M", "一", "N", "弓", "O", "人"],
        ["P", "心", "Q", "手", "R", "口", "S", "尸", "T", "廿"],
        ["U", "山", "V", "女", "W", "田", "X", "難", "Y", "卜"],
    ]
    story.append(make_table(cj_headers, cj_rows,
                            col_widths=[10*mm, 12*mm]*5))

    story.append(P("5.2 Input Flow", S_H2))
    for i, step in enumerate([
        "Decompose the character into Cangjie radicals (1–5 keys).",
        "Type the corresponding letter keys.",
        "If a unique character is found, it is committed automatically.",
        "If multiple candidates exist, press a number key to select.",
        "Press <b>Space</b> to confirm the first candidate.",
    ], 1):
        story.append(P(f"<b>{i}.</b> {step}", S_BULLET))

    story.append(P("5.3 Examples", S_H2))
    cj_ex_headers = ["Character", "Code", "Keys", "Meaning"]
    cj_ex_rows = [
        ["明", "日月", "A B", "Bright"],
        ["好", "女弓木", "V N D", "Good"],
        ["中", "中", "L", "Middle"],
        ["國", "田戈口一", "W I R M", "Country"],
        ["你", "人弓火", "O N F", "You"],
    ]
    story.append(make_table(cj_ex_headers, cj_ex_rows,
                            col_widths=[20*mm, 28*mm, 28*mm, 30*mm]))
    story.append(P(
        "<i>The preedit display shows the method label and radicals: "
        "[倉]竹手一 1.嗨</i>",
        S_NOTE,
    ))
    story.append(PageBreak())

    # ===== 6. BOSHIAMY (嘸蝦米) INPUT =====
    story.append(P("6. Boshiamy (嘸蝦米) Input", S_H1))
    story.append(hr())

    story.append(P(
        "Boshiamy (嘸蝦米) is a shape-based input method using letter keys to represent "
        "character components. It requires a separate table file (<b>liu.gtab</b>) "
        "that is not included with HIME by default.",
        S_BODY,
    ))

    story.append(P("6.1 Installing the Boshiamy Table", S_H2))
    for i, step in enumerate([
        "Obtain <b>liu-uni.tab</b> from your Boshiamy installation "
        "(typically at C:\\Program Files\\BoshiamyTIP\\liu-uni.tab).",
        "Convert it using the provided script:<br/>"
        "<font face='Courier' size='9'>"
        "python3 scripts/liu-tab2cin.py liu-uni.tab -o data/liu.cin --gtab"
        "</font>",
        "Copy the resulting <b>liu.gtab</b> to "
        "C:\\Program Files\\HIME\\data\\.",
        "Restart the application or re-register HIME.",
    ], 1):
        story.append(P(f"<b>{i}.</b> {step}", S_BULLET))

    story.append(P("6.2 Usage", S_H2))
    story.append(P(
        "Once installed, Boshiamy appears in the mode cycle (Ctrl+`, F4, or click) "
        "and in the right-click menu. The tray icon shows the Boshiamy icon (嘸蝦米 logo) "
        "when active.",
        S_BODY,
    ))
    story.append(P(
        "Boshiamy input works the same as Cangjie: type letter codes, then select "
        "from candidates. Refer to the official Boshiamy manual for the code table.",
        S_BODY,
    ))

    story.append(P("6.3 Enabling/Disabling", S_H2))
    story.append(P(
        "If you have Boshiamy installed but don't want it in the cycle, "
        "open <b>Settings</b> (right-click menu → Settings) and uncheck "
        "嘸蝦米 (Boshiamy). It will be skipped when cycling modes.",
        S_BODY,
    ))
    story.append(PageBreak())

    # ===== 7. RIGHT-CLICK MENU =====
    story.append(P("7. Right-Click Menu", S_H1))
    story.append(hr())

    story.append(P(
        "Right-click the <b>HIME notification area icon</b> or the "
        "<b>HIME language bar button</b> (姬) to open the context menu:",
        S_BODY,
    ))

    menu_headers = ["Menu Item", "Description"]
    menu_rows = [
        ["✓ 注音 (Zhuyin)", "Switch to Zhuyin input mode. Checkmark shows current mode."],
        ["倉五 (Cangjie 5)", "Switch to Cangjie 5 input mode."],
        ["嘸蝦米 (Boshiamy)", "Switch to Boshiamy mode (only if liu.gtab installed)."],
        ["─────────", "Separator"],
        ["☐ IME Toolbar", "Toggle the floating toolbar on/off. Checkmark when visible."],
        ["─────────", "Separator"],
        ["Settings...", "Open the Settings dialog to enable/disable input methods."],
        ["About HIME", "Show version and project information."],
    ]
    story.append(make_table(menu_headers, menu_rows,
                            col_widths=[42*mm, 98*mm]))
    story.append(Spacer(1, 3*mm))
    story.append(P(
        "Only <b>enabled</b> input methods appear in the menu. "
        "Use Settings to control which methods are available.",
        S_NOTE,
    ))
    story.append(PageBreak())

    # ===== 8. SETTINGS =====
    story.append(P("8. Settings", S_H1))
    story.append(hr())

    story.append(P(
        "Open Settings from the right-click menu → <b>Settings...</b>",
        S_BODY,
    ))

    story.append(P("8.1 Input Method Selection", S_H2))
    story.append(P(
        "The Settings dialog shows checkboxes for each available input method:",
        S_BODY,
    ))

    settings_data = [
        [P("☑", S_TD), P("注音 (Zhuyin)", S_TD_L),
         P("Phonetic/Bopomofo input", S_TD_L)],
        [P("☑", S_TD), P("倉五 (Cangjie 5)", S_TD_L),
         P("Shape-based Cangjie input", S_TD_L)],
        [P("☐", S_TD), P("嘸蝦米 (Boshiamy)", S_TD_L),
         P("Boshiamy input (requires liu.gtab)", S_TD_L)],
    ]
    st = Table(settings_data, colWidths=[12*mm, 45*mm, 70*mm])
    st.setStyle(TableStyle([
        ("GRID", (0,0), (-1,-1), 0.5, lightgrey),
        ("VALIGN", (0,0), (-1,-1), "MIDDLE"),
        ("ROWBACKGROUNDS", (0,0), (-1,-1), [white, C_TABLE_ALT]),
        ("TOPPADDING", (0,0), (-1,-1), 4),
        ("BOTTOMPADDING", (0,0), (-1,-1), 4),
        ("LEFTPADDING", (0,0), (-1,-1), 6),
    ]))
    story.append(st)

    story.append(P("8.2 Rules", S_H2))
    for item in [
        "<b>At least one</b> input method must remain enabled at all times.",
        "If you disable the currently active method, HIME automatically switches "
        "to the first enabled method.",
        "If Boshiamy (liu.gtab) is not installed, its checkbox is grayed out.",
        "English (EN) passthrough mode is always available regardless of settings.",
        "Only enabled methods participate in the Ctrl+`/F4 and click cycle.",
    ]:
        story.append(P(f"• {item}", S_BULLET))

    story.append(P("8.3 About Dialog", S_H2))
    story.append(P(
        "Select <b>About HIME</b> from the right-click menu to see:",
        S_BODY,
    ))
    about_box = Table(
        [[P(
            "<b>姬 HIME Input Method Editor</b><br/><br/>"
            "Version: 0.10.1<br/>"
            "Updated: 2026-02-15<br/><br/>"
            "https://github.com/nicehime/hime",
            S_TD_L
        )]],
        colWidths=[100*mm],
    )
    about_box.setStyle(TableStyle([
        ("BOX", (0,0), (-1,-1), 1, C_PRIMARY),
        ("BACKGROUND", (0,0), (-1,-1), C_LIGHT_BG),
        ("TOPPADDING", (0,0), (-1,-1), 8),
        ("BOTTOMPADDING", (0,0), (-1,-1), 8),
        ("LEFTPADDING", (0,0), (-1,-1), 12),
    ]))
    story.append(about_box)
    story.append(PageBreak())

    # ===== 9. FLOATING TOOLBAR =====
    story.append(P("9. Floating Toolbar", S_H1))
    story.append(hr())

    story.append(P(
        "The floating toolbar is a small popup window that provides quick access "
        "to common functions without needing the system tray.",
        S_BODY,
    ))

    story.append(P("9.1 Enabling the Toolbar", S_H2))
    story.append(P(
        "Right-click the HIME icon → select <b>IME Toolbar</b>. "
        "A checkmark indicates the toolbar is visible. Select again to hide it.",
        S_BODY,
    ))

    story.append(P("9.2 Toolbar Layout", S_H2))
    toolbar_data = [
        [P("<b>注</b>", S_TD),
         P("<b>Ａ</b>", S_TD),
         P("<b>⚙</b>", S_TD)],
    ]
    toolbar_labels = [
        [P("Mode<br/>Icon", S_TD),
         P("Full/<br/>Half", S_TD),
         P("Settings", S_TD)],
    ]
    tb = Table(toolbar_data, colWidths=[24*mm, 24*mm, 24*mm])
    tb.setStyle(TableStyle([
        ("BOX", (0,0), (-1,-1), 2, C_PRIMARY),
        ("INNERGRID", (0,0), (-1,-1), 0.5, lightgrey),
        ("ALIGN", (0,0), (-1,-1), "CENTER"),
        ("VALIGN", (0,0), (-1,-1), "MIDDLE"),
        ("BACKGROUND", (0,0), (-1,-1), HexColor("#F0F0F0")),
        ("TOPPADDING", (0,0), (-1,-1), 6),
        ("BOTTOMPADDING", (0,0), (-1,-1), 6),
    ]))
    story.append(tb)
    tbl = Table(toolbar_labels, colWidths=[24*mm, 24*mm, 24*mm])
    tbl.setStyle(TableStyle([
        ("ALIGN", (0,0), (-1,-1), "CENTER"),
        ("TOPPADDING", (0,0), (-1,-1), 2),
        ("TEXTCOLOR", (0,0), (-1,-1), C_MID_GRAY),
    ]))
    story.append(tbl)

    story.append(Spacer(1, 3*mm))
    tb_func_headers = ["Area", "Click Action"]
    tb_func_rows = [
        ["Mode Icon (left)", "Cycle to next input method"],
        ["Full/Half (middle)", "Toggle full-width ↔ half-width characters"],
        ["Settings Gear (right)", "Open Settings dialog"],
    ]
    story.append(make_table(tb_func_headers, tb_func_rows,
                            col_widths=[45*mm, 95*mm]))

    story.append(P("9.3 Position", S_H2))
    story.append(P(
        "The toolbar appears in the bottom-right corner of the screen, "
        "above the Windows taskbar. It stays on top of other windows.",
        S_BODY,
    ))
    story.append(PageBreak())

    # ===== 10. KEYBOARD SHORTCUTS =====
    story.append(P("10. Keyboard Shortcuts", S_H1))
    story.append(hr())

    shortcut_headers = ["Shortcut", "Function", "Context"]
    shortcut_rows = [
        ["Win + Space", "Switch between installed IMEs", "Any time"],
        ["Ctrl + ` (backtick)", "Cycle HIME modes (EN/注/倉/蝦)", "When HIME is active"],
        ["F4", "Same as Ctrl + `", "When HIME is active"],
        ["Space", "1st tone / Confirm selection", "During input"],
        ["3 / 4 / 6 / 7", "2nd / 3rd / 4th / Neutral tone", "Zhuyin input"],
        ["1 – 9, 0", "Select candidate", "Candidate list shown"],
        ["Escape", "Cancel current input", "During input"],
        ["Backspace", "Delete last component", "During input"],
    ]
    story.append(make_table(shortcut_headers, shortcut_rows,
                            col_widths=[38*mm, 55*mm, 40*mm]))
    story.append(PageBreak())

    # ===== 11. TROUBLESHOOTING =====
    story.append(P("11. Troubleshooting", S_H1))
    story.append(hr())

    problems = [
        ("HIME doesn't appear in language settings",
         [
             "Ensure you ran <b>hime-install.exe</b> as Administrator.",
             "Check that both hime-core.dll and hime-tsf.dll exist in "
             "C:\\Program Files\\HIME\\.",
             "Try manual registration: open Command Prompt as Administrator and run:<br/>"
             '<font face="Courier" size="9">'
             'regsvr32 "C:\\Program Files\\HIME\\hime-tsf.dll"</font>',
         ]),
        ("No characters appear when typing",
         [
             "Verify that pho.tab2 exists in C:\\Program Files\\HIME\\data\\.",
             "Make sure you are in Chinese mode (icon shows 注/倉/蝦, not EN).",
             "Some legacy applications do not support TSF — try Notepad or a modern app.",
         ]),
        ("Boshiamy is not available",
         [
             "Check that liu.gtab exists in C:\\Program Files\\HIME\\data\\.",
             "See section 6.1 for conversion instructions.",
             "After placing liu.gtab, restart the application or re-register HIME.",
         ]),
        ("DLL files are 'blocked' by Windows",
         [
             "Right-click the DLL file → Properties → check <b>Unblock</b> → Apply.",
             "This may happen if the DLLs were downloaded from the internet.",
         ]),
        ("System tray icons don't update",
         [
             "Click another application and click back — this refreshes the tray.",
             "If icons are missing, the icons/ folder may not be in the install directory.",
         ]),
    ]

    for title, fixes in problems:
        story.append(P(f"<b>{title}</b>", S_H3))
        for fix in fixes:
            story.append(P(f"• {fix}", S_BULLET))
        story.append(Spacer(1, 2*mm))

    story.append(Spacer(1, 10*mm))
    story.append(hr())
    story.append(P(
        "<b>Need more help?</b> Visit https://github.com/nicehime/hime/issues",
        S_BODY_C,
    ))
    story.append(Spacer(1, 15*mm))
    story.append(P(
        "© 2026 The HIME Team, Taiwan — GNU LGPL v2.1",
        S_NOTE,
    ))

    return story

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main():
    doc = SimpleDocTemplate(
        OUTPUT_PDF,
        pagesize=A4,
        leftMargin=20*mm, rightMargin=20*mm,
        topMargin=22*mm, bottomMargin=18*mm,
        title="HIME Input Method Editor — Windows User Manual",
        author="HIME Team",
        subject="User manual for HIME IME on Windows",
    )

    story = build_content()
    doc.build(story, onFirstPage=_on_first_page, onLaterPages=_on_later_pages)
    print(f"Generated: {OUTPUT_PDF}")
    print(f"Size: {os.path.getsize(OUTPUT_PDF) / 1024:.0f} KB")

if __name__ == "__main__":
    main()
