# Rime/Weasel Feature Comparison & Adoption Roadmap for HIME

## Context

[Rime](https://rime.im) (中州韻輸入法引擎) is the most feature-rich open-source Chinese IME framework. Its Windows frontend is **Weasel** (小狼毫), macOS is **Squirrel** (鼠鬚管), and Linux uses ibus-rime/fcitx5-rime. This document compares Rime's features with HIME and identifies practical features to adopt.

## Architecture Comparison

| Aspect | Rime | HIME |
|--------|------|------|
| Core library | `librime` (C API, BSD-3) | `hime-core` (C API, LGPL) |
| Frontend separation | Complete — frontends are thin shells | Partial — Linux daemon is monolithic, Windows/macOS/Android are separate |
| Config format | YAML schemas with patch system | GConf/GSettings (Linux), native APIs (macOS/Windows) |
| Plugin system | Loadable modules (Lua, n-gram, prediction) | None |
| Dictionary format | LevelDB + marisa-trie | Binary `.gtab` tables |
| Build/deploy model | YAML compile step ("Deploy") | `hime-cin2gtab` table compilation |

## Feature Gap Analysis

### What HIME Already Has

- Cross-platform core with C API (`hime-core.h`)
- Multiple input methods: Zhuyin, Cangjie, Pinyin, Jyutping, etc.
- Traditional/Simplified conversion (`hime_convert_trad_to_simp`)
- Candidate pagination and selection keys
- Keyboard layout variants (Standard, HSU, ETen, etc.)
- Color scheme API (Light/Dark/System)
- Feedback callback system for mode changes
- **System tray language bar** (Windows)
- **Standalone installer/uninstaller** (Windows, with UAC elevation and Add/Remove Programs integration)
- Smart punctuation conversion
- Typing practice system

### What Rime Has That HIME Lacks

#### Tier 1 — High Impact, Moderate Effort

| Feature | Rime Implementation | Recommended HIME Approach |
|---------|-------------------|--------------------------|
| **Per-app ASCII mode** | `app_options:` in YAML config maps exe names to settings | Add a JSON/INI config file (`hime-apps.conf`) mapping process names to default mode. Check on focus change. Windows: `GetModuleFileNameW` on focus. macOS: `bundleIdentifier`. |
| **Candidate window (Windows)** | Custom Direct2D/GDI+ rendered popup with full theming | Implement a basic Win32 popup window positioned at the caret. Start with simple white background + candidate list. Phase 2: add color themes. |
| **Inline preedit toggle** | `inline_preedit: true/false` per frontend | Windows TSF already uses inline preedit. Add option to show a floating preedit window instead (useful for apps with poor TSF support). |
| **Status notifications** | Toast-style popup showing "中文"/"English" on mode switch | Show a brief overlay near the caret when mode changes. Fade after 1 second. Cheap to implement on all platforms. |

#### Tier 2 — High Impact, Higher Effort

| Feature | Rime Implementation | Recommended HIME Approach |
|---------|-------------------|--------------------------|
| **Unified config file** | YAML with `*.custom.yaml` patch overlays | Adopt a single `hime.conf` (JSON or TOML) per platform. Store in `%AppData%\HIME` (Win), `~/Library/HIME` (macOS), `~/.config/hime` (Linux). |
| **Fuzzy pinyin** | Spelling algebra (regex transforms on input) | Add optional fuzzy rules to `hime_process_key` — e.g., `zh→z`, `sh→s`, `eng→en`. Can be table-driven with a small config. |
| **User dictionary sync** | File-based sync via configurable directory | Export/import user phrase database to a portable format. Sync via cloud drive (user manages the folder). |
| **Reverse lookup** | Type pinyin to find Cangjie codes (cross-method lookup) | Add `hime_reverse_lookup(ctx, "ma", HIME_IM_GTAB, buffer)` API. Show Cangjie decomposition as candidate annotation. Useful for learners. |

#### Tier 3 — Niche or Aspirational

| Feature | Rime Implementation | Notes |
|---------|-------------------|-------|
| Schema DSL (YAML engine pipeline) | Full processor/segmentor/translator/filter pipeline | Too complex for HIME's lightweight goal. HIME's fixed pipeline is fine. |
| Lua scripting plugin | `librime-lua` for custom processors/filters | Consider if community demand arises. |
| Language model (n-gram) | `librime-octagram` | Heavyweight, requires large data files. Not aligned with HIME's lightweight goal. |
| Next-word prediction | `librime-predict` | Better suited for mobile. Consider for Android only. |
| 20+ color theme presets | Named themes in `weasel.yaml` | Start with 3-4 themes (Light, Dark, Blue, Classic). |

## Recommended Adoption Priorities

### Phase 1: Quick Wins (Windows focus)

These can be done with minimal core changes:

1. **Per-app ASCII mode** — Config file + focus-change hook
2. **Mode change notification overlay** — Brief "注音"/"English" popup near caret
3. **Basic candidate window** — Win32 popup for candidate display (currently candidates only show in preedit text)

### Phase 2: Cross-Platform Config

4. **Unified config file format** — JSON or TOML, one file per platform, same schema
5. **Config API in hime-core** — `hime_config_get_string()`, `hime_config_set_int()`, etc.
6. **Per-app settings on all platforms**

### Phase 3: Input Intelligence

7. **Fuzzy pinyin rules** — Table-driven spelling transforms
8. **Reverse lookup** — Cross-method code display in candidate annotations
9. **User dictionary export/import** — Portable phrase sync

### Phase 4: Visual Polish

10. **Color theme system** — Named themes with candidate window styling
11. **Font/layout configuration** — Candidate window spacing, font size, corner radius
12. **Dark mode auto-detection** — Already have API, need platform integration

## Rime Features to Skip

These don't fit HIME's lightweight philosophy:

- **YAML schema DSL** — Over-engineered for HIME's use case. HIME's compiled table approach is simpler and faster.
- **Plugin/module system** — Adds complexity without clear user demand.
- **Language models** — Requires large data files (50MB+), contrary to lightweight goal.
- **Chord typing** — Niche feature with minimal user demand.
- **Vertical text rendering** — Rarely needed in modern applications.

## Weasel-Specific Features Worth Studying

| Feature | Source File | Notes |
|---------|-----------|-------|
| Candidate window rendering | `WeaselPanel.cpp` | Direct2D rendering, rounded corners, shadow |
| Per-app config | `WeaselTSF.cpp` | `GetModuleFileName` on `OnSetFocus` |
| Tray icon with mode label | `WeaselTray.cpp` | Similar to our `HimeLangBarButton` |
| Auto-update | WinSparkle integration | Consider for future releases |
| Status notification | `WeaselPanel.cpp` | Brief overlay showing mode name |

## Weasel Candidate Window Deep Dive

Weasel's candidate window (`weasel.yaml` `style:` section) is the most polished part of the
Windows frontend and worth studying in detail for HIME's candidate window implementation.

### Layout Properties

| Property | Type | Description |
|----------|------|-------------|
| `layout/type` | enum | `horizontal`, `vertical`, `vertical_text`, `vertical+fullscreen`, `horizontal+fullscreen` |
| `layout/border_width` | int | Border thickness (px) |
| `layout/margin_x`, `margin_y` | int | Outer margin; negative values hide the candidate frame |
| `layout/spacing` | int | Gap between preedit and candidate areas |
| `layout/candidate_spacing` | int | Gap between candidates (horizontal mode) |
| `layout/hilite_spacing` | int | Gap between label and candidate text |
| `layout/hilite_padding` | int | Padding inside the highlight box |
| `layout/corner_radius` | int | Window corner radius |
| `layout/round_corner` | int | Highlight box corner radius |
| `layout/shadow_radius` | int | Shadow blur radius (0 = no shadow) |
| `layout/shadow_offset_x/y` | int | Shadow offset |
| `layout/max_width`, `max_height` | int | Auto-fold when exceeded (0 = disabled) |
| `layout/min_width`, `min_height` | int | Minimum dimensions |

### Font Configuration

Weasel allows separate fonts for candidate text, labels, and annotations with fallback chains:
```yaml
style:
  font_face: "Microsoft YaHei, Segoe UI Emoji"
  label_font_face: "Segoe UI"
  comment_font_face: "Microsoft YaHei"
  font_point: 14
  label_font_point: 12
  comment_font_point: 12
```

Font entries support Unicode range restrictions: `"Segoe UI Emoji:30:39:Bold"` uses Emoji
font only for codepoints 0x30–0x39. Weight and style are parsed from the first font entry.

### Color Scheme Properties (~20 per scheme)

Each named scheme defines colors for every UI element:
- `text_color`, `back_color`, `border_color`, `shadow_color`
- `hilited_text_color`, `hilited_back_color` (preedit highlight)
- `hilited_candidate_text_color`, `hilited_candidate_back_color` (selected candidate)
- `candidate_text_color`, `candidate_back_color` (non-selected)
- `comment_text_color`, `label_color`
- `hilited_mark_color` (selection indicator; empty string = Windows 11-style mark)
- `prevpage_color`, `nextpage_color` (paging arrows)

Separate `color_scheme` and `color_scheme_dark` for OS dark mode detection (Win 10 1809+).
Per-schema overrides are supported (e.g., different scheme for Wubi vs Pinyin).

### Per-App Settings

```yaml
app_options:
  cmd.exe:
    ascii_mode: true      # Start in English mode
  gvim.exe:
    ascii_mode: true
    vim_mode: true         # Esc switches to ASCII
  firefox.exe:
    inline_preedit: true   # Render preedit in-app
```

### Other Notable UI Features

- **Preedit modes**: `composition` (raw input), `preview` (selected candidate), `preview_all` (all candidates inline)
- **Mouse interaction**: `hover_type` = `none`/`hilite`/`semi_hilite`; `paging_on_scroll` for scroll wheel paging
- **Mode notifications**: Toast popup on mode switch with configurable duration (`show_notifications_time`)
- **Global ASCII**: `global_ascii: true` makes Shift toggle affect all windows, not just current
- **Anti-aliasing**: `antialias_mode` = `default`/`cleartype`/`grayscale`/`aliased`

### Takeaway for HIME

For HIME's Windows candidate window, a phased approach:
1. **Phase 1**: Simple GDI popup — white background, black text, highlight bar, positioned at caret
2. **Phase 2**: Add 3-4 named color schemes (Light, Dark, Blue, Classic) via `hime.conf`
3. **Phase 3**: Add layout tunables (corner radius, spacing, font size) — same config file
4. **Phase 4**: Per-app settings and mouse interaction

This avoids Weasel's complexity (Direct2D, YAML deployment) while providing the most impactful user-facing features.

## References

- Rime official site: https://rime.im
- Weasel source: https://github.com/rime/weasel
- librime source: https://github.com/rime/librime
- Rime wiki: https://github.com/rime/home/wiki
- Rime CustomizationGuide: https://github.com/rime/home/wiki/CustomizationGuide
