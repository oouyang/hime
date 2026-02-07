/*
 * HIME Core Library - Cross-platform Header
 *
 * This header defines the platform-independent API for HIME's core
 * input method functionality. It can be used to integrate HIME
 * with any platform's IME framework.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#ifndef HIME_CORE_H
#define HIME_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#ifdef HIME_CORE_EXPORTS
#define HIME_API __declspec(dllexport)
#else
#define HIME_API __declspec(dllimport)
#endif
#else
#define HIME_API
#endif

#include <stdbool.h>
#include <stdint.h>

/* Maximum sizes */
#define HIME_MAX_PREEDIT 256
#define HIME_MAX_CANDIDATES 100
#define HIME_MAX_CANDIDATE_LEN 64
#define HIME_CH_SZ 4

/* Phonetic key type (16-bit) */
typedef uint16_t hime_phokey_t;

/* Preedit attribute flags */
typedef enum {
    HIME_ATTR_NONE = 0,
    HIME_ATTR_UNDERLINE = 1,
    HIME_ATTR_REVERSE = 2,
    HIME_ATTR_HIGHLIGHT = 4
} HimeAttrFlag;

/* Preedit attribute */
typedef struct {
    int flag;
    int start; /* Start byte offset */
    int end;   /* End byte offset */
} HimePreeditAttr;

/* Input method types */
typedef enum {
    HIME_IM_PHO = 0,     /* Phonetic (Bopomofo/Zhuyin) */
    HIME_IM_TSIN = 1,    /* Phrase input */
    HIME_IM_GTAB = 2,    /* Table-based (Cangjie, etc.) */
    HIME_IM_ANTHY = 3,   /* Japanese Anthy */
    HIME_IM_CHEWING = 4, /* Chewing library */
    HIME_IM_INTCODE = 5, /* Unicode/Big5 code input */
    HIME_IM_COUNT        /* Number of input methods */
} HimeInputMethod;

/* Well-known GTAB table IDs */
typedef enum {
    /* Chinese - Cangjie family */
    HIME_GTAB_CJ = 0,      /* Cangjie (倉頡) */
    HIME_GTAB_CJ5 = 1,     /* Cangjie 5 (倉五) */
    HIME_GTAB_CJ543 = 2,   /* Cangjie 543 (五四三倉頡) */
    HIME_GTAB_CJ_PUNC = 3, /* Cangjie + punctuation (標點倉頡) */

    /* Chinese - Simplex family */
    HIME_GTAB_SIMPLEX = 10,      /* Simplex (速成/簡易) */
    HIME_GTAB_SIMPLEX_PUNC = 11, /* Simplex + punctuation (標點簡易) */

    /* Chinese - DaYi (大易) */
    HIME_GTAB_DAYI = 20, /* DaYi 3 (大易三碼) */

    /* Chinese - Array family (行列) */
    HIME_GTAB_ARRAY30 = 30,     /* Array 30 (行列30) */
    HIME_GTAB_ARRAY40 = 31,     /* Array 40 (行列40) */
    HIME_GTAB_ARRAY30_BIG = 32, /* Array 30 Big charset (行列大字集) */

    /* Chinese - Boshiamy (嘸蝦米) */
    HIME_GTAB_BOSHIAMY = 40, /* Boshiamy (嘸蝦米) */

    /* Chinese - Phonetic-based */
    HIME_GTAB_PINYIN = 50,   /* Pinyin table (拼音) */
    HIME_GTAB_JYUTPING = 51, /* Cantonese Jyutping (粵拼) */

    /* Korean */
    HIME_GTAB_HANGUL = 60,       /* Korean Hangul (韓諺) */
    HIME_GTAB_HANGUL_ROMAN = 61, /* Korean Romanization (韓羅) */

    /* Vietnamese */
    HIME_GTAB_VIMS = 70, /* Vietnamese VIMS (越南文) */

    /* Symbols and special characters */
    HIME_GTAB_SYMBOLS = 80,   /* Symbol input (符號) */
    HIME_GTAB_GREEK = 81,     /* Greek letters (希臘文) */
    HIME_GTAB_RUSSIAN = 82,   /* Russian/Cyrillic (俄文) */
    HIME_GTAB_ESPERANTO = 83, /* Esperanto (世界文) */
    HIME_GTAB_LATIN = 84,     /* Latin letters (拉丁字母) */

    /* Custom/user-defined */
    HIME_GTAB_CUSTOM = 99 /* Custom/user table */
} HimeGtabTable;

/* GTAB table info structure */
typedef struct {
    char name[64];      /* Display name (UTF-8) */
    char filename[128]; /* Table filename */
    char icon[64];      /* Icon filename */
    int key_count;      /* Number of defined characters */
    int max_keystrokes; /* Max keystrokes per character */
    char selkey[16];    /* Selection keys */
    bool loaded;        /* Whether table is loaded */
} HimeGtabInfo;

/* Intcode input mode */
typedef enum {
    HIME_INTCODE_UNICODE = 0, /* Unicode (hex, e.g., "4E2D" for 中) */
    HIME_INTCODE_BIG5 = 1     /* Big5 (hex, e.g., "A4A4" for 中) */
} HimeIntcodeMode;

/* Key event modifiers */
typedef enum {
    HIME_MOD_NONE = 0,
    HIME_MOD_SHIFT = 1,
    HIME_MOD_CONTROL = 2,
    HIME_MOD_ALT = 4,
    HIME_MOD_CAPSLOCK = 8
} HimeModifier;

/* Key processing result */
typedef enum {
    HIME_KEY_IGNORED = 0,  /* Key not handled, pass through */
    HIME_KEY_ABSORBED = 1, /* Key handled, no output */
    HIME_KEY_COMMIT = 2,   /* Key handled, commit string ready */
    HIME_KEY_PREEDIT = 3   /* Key handled, preedit updated */
} HimeKeyResult;

/* Opaque context handle */
typedef struct HimeContext HimeContext;

/* ========== Initialization ========== */

/**
 * Initialize HIME core library
 * @param data_dir Path to HIME data directory (tables, configs)
 * @return 0 on success, negative on error
 */
HIME_API int hime_init (const char *data_dir);

/**
 * Cleanup HIME core library
 */
HIME_API void hime_cleanup (void);

/**
 * Get HIME version string
 */
HIME_API const char *hime_version (void);

/* ========== Context Management ========== */

/**
 * Create a new input context
 * @return Context handle, or NULL on failure
 */
HIME_API HimeContext *hime_context_new (void);

/**
 * Destroy an input context
 */
HIME_API void hime_context_free (HimeContext *ctx);

/**
 * Reset context to initial state
 */
HIME_API void hime_context_reset (HimeContext *ctx);

/* ========== Input Method Control ========== */

/**
 * Set the active input method
 * @param ctx Context handle
 * @param method Input method type
 * @return 0 on success
 */
HIME_API int hime_set_input_method (HimeContext *ctx, HimeInputMethod method);

/**
 * Get the active input method
 */
HIME_API HimeInputMethod hime_get_input_method (HimeContext *ctx);

/**
 * Toggle Chinese/English mode
 * @return true if now in Chinese mode
 */
HIME_API bool hime_toggle_chinese_mode (HimeContext *ctx);

/**
 * Check if in Chinese mode
 */
HIME_API bool hime_is_chinese_mode (HimeContext *ctx);

/**
 * Set Chinese mode directly
 */
HIME_API void hime_set_chinese_mode (HimeContext *ctx, bool chinese);

/* ========== Key Processing ========== */

/**
 * Process a key press event
 * @param ctx Context handle
 * @param keycode Virtual key code (platform-specific)
 * @param charcode Unicode character code (0 if not printable)
 * @param modifiers Modifier key flags
 * @return Key processing result
 */
HIME_API HimeKeyResult hime_process_key (
    HimeContext *ctx,
    uint32_t keycode,
    uint32_t charcode,
    uint32_t modifiers);

/* ========== Preedit String ========== */

/**
 * Get the current preedit (composition) string
 * @param ctx Context handle
 * @param buffer Output buffer for UTF-8 string
 * @param buffer_size Size of buffer
 * @return Length of preedit string, or negative on error
 */
HIME_API int hime_get_preedit (
    HimeContext *ctx,
    char *buffer,
    int buffer_size);

/**
 * Get preedit cursor position (byte offset)
 */
HIME_API int hime_get_preedit_cursor (HimeContext *ctx);

/**
 * Get preedit attributes
 * @param ctx Context handle
 * @param attrs Output array for attributes
 * @param max_attrs Maximum number of attributes to return
 * @return Number of attributes
 */
HIME_API int hime_get_preedit_attrs (
    HimeContext *ctx,
    HimePreeditAttr *attrs,
    int max_attrs);

/* ========== Commit String ========== */

/**
 * Get the commit string (text to insert)
 * @param ctx Context handle
 * @param buffer Output buffer for UTF-8 string
 * @param buffer_size Size of buffer
 * @return Length of commit string, or negative on error
 */
HIME_API int hime_get_commit (
    HimeContext *ctx,
    char *buffer,
    int buffer_size);

/**
 * Clear the commit string after retrieving
 */
HIME_API void hime_clear_commit (HimeContext *ctx);

/* ========== Candidates ========== */

/**
 * Check if candidate window should be shown
 */
HIME_API bool hime_has_candidates (HimeContext *ctx);

/**
 * Get number of candidates
 */
HIME_API int hime_get_candidate_count (HimeContext *ctx);

/**
 * Get a candidate string by index
 * @param ctx Context handle
 * @param index Candidate index (0-based)
 * @param buffer Output buffer for UTF-8 string
 * @param buffer_size Size of buffer
 * @return Length of candidate string, or negative on error
 */
HIME_API int hime_get_candidate (
    HimeContext *ctx,
    int index,
    char *buffer,
    int buffer_size);

/**
 * Get current candidate page
 */
HIME_API int hime_get_candidate_page (HimeContext *ctx);

/**
 * Get candidates per page
 */
HIME_API int hime_get_candidates_per_page (HimeContext *ctx);

/**
 * Select a candidate by index
 * @return HIME_KEY_COMMIT if selection successful
 */
HIME_API HimeKeyResult hime_select_candidate (HimeContext *ctx, int index);

/**
 * Move to previous candidate page
 * @return true if page changed
 */
HIME_API bool hime_candidate_page_up (HimeContext *ctx);

/**
 * Move to next candidate page
 * @return true if page changed
 */
HIME_API bool hime_candidate_page_down (HimeContext *ctx);

/* ========== Phonetic-specific ========== */

/**
 * Get the Bopomofo display string for current input
 * @param ctx Context handle
 * @param buffer Output buffer for UTF-8 Bopomofo symbols
 * @param buffer_size Size of buffer
 * @return Length of string
 */
HIME_API int hime_get_bopomofo_string (
    HimeContext *ctx,
    char *buffer,
    int buffer_size);

/* ========== Configuration ========== */

/**
 * Keyboard layout types for phonetic input
 */
typedef enum {
    HIME_KB_STANDARD = 0, /* Standard Zhuyin (default) */
    HIME_KB_HSU = 1,      /* HSU (許氏) layout */
    HIME_KB_ETEN = 2,     /* ETen (倚天) layout */
    HIME_KB_ETEN26 = 3,   /* ETen 26-key layout */
    HIME_KB_IBM = 4,      /* IBM layout */
    HIME_KB_PINYIN = 5,   /* Hanyu Pinyin layout */
    HIME_KB_DVORAK = 6,   /* Dvorak-based Zhuyin */
    HIME_KB_COUNT         /* Number of layouts */
} HimeKeyboardLayout;

/**
 * Set keyboard layout for phonetic input
 * @param ctx Context handle
 * @param layout Keyboard layout type
 * @return 0 on success, -1 on error
 */
HIME_API int hime_set_keyboard_layout (HimeContext *ctx, HimeKeyboardLayout layout);

/**
 * Get current keyboard layout
 * @param ctx Context handle
 * @return Current keyboard layout
 */
HIME_API HimeKeyboardLayout hime_get_keyboard_layout (HimeContext *ctx);

/**
 * Set keyboard layout by name (for compatibility)
 * @param ctx Context handle
 * @param layout_name Layout name ("standard", "hsu", "eten", "eten26", "ibm", "pinyin", "dvorak")
 * @return 0 on success, -1 if layout not found
 */
HIME_API int hime_set_keyboard_layout_by_name (HimeContext *ctx, const char *layout_name);

/**
 * Set selection keys string (e.g., "1234567890")
 */
HIME_API void hime_set_selection_keys (HimeContext *ctx, const char *keys);

/**
 * Set candidates per page
 */
HIME_API void hime_set_candidates_per_page (HimeContext *ctx, int count);

/* ========== GTAB (Table-based) Input ========== */

/**
 * Get number of available GTAB tables
 * @return Number of registered tables
 */
HIME_API int hime_gtab_get_table_count (void);

/**
 * Get GTAB table info by index
 * @param index Table index (0-based)
 * @param info Output structure for table info
 * @return 0 on success, -1 if index out of range
 */
HIME_API int hime_gtab_get_table_info (int index, HimeGtabInfo *info);

/**
 * Load a GTAB table by filename
 * @param ctx Context handle
 * @param filename Table filename (e.g., "cj.gtab")
 * @return 0 on success, -1 on error
 */
HIME_API int hime_gtab_load_table (HimeContext *ctx, const char *filename);

/**
 * Load a GTAB table by well-known ID
 * @param ctx Context handle
 * @param table_id Well-known table ID
 * @return 0 on success, -1 on error
 */
HIME_API int hime_gtab_load_table_by_id (HimeContext *ctx, HimeGtabTable table_id);

/**
 * Get current GTAB table name
 * @param ctx Context handle
 * @return Table name or empty string
 */
HIME_API const char *hime_gtab_get_current_table (HimeContext *ctx);

/**
 * Get GTAB key display string (shows entered keys)
 * @param ctx Context handle
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @return Length of string
 */
HIME_API int hime_gtab_get_key_string (
    HimeContext *ctx,
    char *buffer,
    int buffer_size);

/* ========== TSIN (Phrase) Input ========== */

/**
 * Load TSIN phrase database
 * @param ctx Context handle
 * @param filename Phrase database filename (e.g., "tsin32")
 * @return 0 on success, -1 on error
 */
HIME_API int hime_tsin_load_database (HimeContext *ctx, const char *filename);

/**
 * Get current phrase buffer (composed phrase)
 * @param ctx Context handle
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @return Length of phrase
 */
HIME_API int hime_tsin_get_phrase (
    HimeContext *ctx,
    char *buffer,
    int buffer_size);

/**
 * Commit current phrase and clear buffer
 * @param ctx Context handle
 * @return Length of committed string
 */
HIME_API int hime_tsin_commit_phrase (HimeContext *ctx);

/* ========== Intcode (Unicode/Big5) Input ========== */

/**
 * Set intcode input mode
 * @param ctx Context handle
 * @param mode Unicode or Big5 mode
 */
HIME_API void hime_intcode_set_mode (HimeContext *ctx, HimeIntcodeMode mode);

/**
 * Get current intcode mode
 */
HIME_API HimeIntcodeMode hime_intcode_get_mode (HimeContext *ctx);

/**
 * Get current intcode input buffer (hex digits entered)
 * @param ctx Context handle
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @return Length of hex string
 */
HIME_API int hime_intcode_get_buffer (
    HimeContext *ctx,
    char *buffer,
    int buffer_size);

/**
 * Convert hex code to character
 * @param ctx Context handle
 * @param hex_code Hex string (e.g., "4E2D" for Unicode, "A4A4" for Big5)
 * @param buffer Output buffer for UTF-8 character
 * @param buffer_size Buffer size
 * @return Length of UTF-8 character, 0 if invalid code
 */
HIME_API int hime_intcode_convert (
    HimeContext *ctx,
    const char *hex_code,
    char *buffer,
    int buffer_size);

/* ========== Input Method Availability ========== */

/**
 * Check if an input method is available/supported
 * @param method Input method type
 * @return true if available
 */
HIME_API bool hime_is_method_available (HimeInputMethod method);

/**
 * Get input method display name
 * @param method Input method type
 * @return Display name string
 */
HIME_API const char *hime_get_method_name (HimeInputMethod method);

/* ========== Settings/Preferences (New Features) ========== */

/**
 * Character set type for Simplified/Traditional Chinese
 */
typedef enum {
    HIME_CHARSET_TRADITIONAL = 0, /* Traditional Chinese (Big5 ordering) */
    HIME_CHARSET_SIMPLIFIED = 1   /* Simplified Chinese (GB ordering) */
} HimeCharset;

/**
 * Candidate display style
 */
typedef enum {
    HIME_CAND_STYLE_HORIZONTAL = 0, /* Horizontal inline bar */
    HIME_CAND_STYLE_VERTICAL = 1,   /* Vertical list */
    HIME_CAND_STYLE_POPUP = 2       /* Floating popup window */
} HimeCandidateStyle;

/**
 * Feedback event types
 */
typedef enum {
    HIME_FEEDBACK_KEY_PRESS = 0,   /* Regular key press */
    HIME_FEEDBACK_KEY_DELETE = 1,  /* Delete/backspace */
    HIME_FEEDBACK_KEY_ENTER = 2,   /* Enter/return */
    HIME_FEEDBACK_KEY_SPACE = 3,   /* Space bar */
    HIME_FEEDBACK_CANDIDATE = 4,   /* Candidate selection */
    HIME_FEEDBACK_MODE_CHANGE = 5, /* Mode switch */
    HIME_FEEDBACK_ERROR = 6        /* Invalid input */
} HimeFeedbackType;

/**
 * Feedback callback function type
 * @param type Feedback event type
 * @param user_data User-provided data pointer
 */
typedef void (*HimeFeedbackCallback) (HimeFeedbackType type, void *user_data);

/**
 * Set Simplified/Traditional Chinese mode
 * @param ctx Context handle
 * @param charset Character set to use
 */
HIME_API void hime_set_charset (HimeContext *ctx, HimeCharset charset);

/**
 * Get current character set
 */
HIME_API HimeCharset hime_get_charset (HimeContext *ctx);

/**
 * Toggle between Simplified and Traditional
 * @return New character set value
 */
HIME_API HimeCharset hime_toggle_charset (HimeContext *ctx);

/**
 * Enable/disable smart punctuation conversion
 * When enabled, ASCII punctuation is auto-converted to Chinese punctuation
 * @param ctx Context handle
 * @param enabled true to enable smart punctuation
 */
HIME_API void hime_set_smart_punctuation (HimeContext *ctx, bool enabled);

/**
 * Check if smart punctuation is enabled
 */
HIME_API bool hime_get_smart_punctuation (HimeContext *ctx);

/**
 * Enable/disable Pinyin annotation display
 * When enabled, candidates show pronunciation hints
 * @param ctx Context handle
 * @param enabled true to show Pinyin annotations
 */
HIME_API void hime_set_pinyin_annotation (HimeContext *ctx, bool enabled);

/**
 * Check if Pinyin annotation is enabled
 */
HIME_API bool hime_get_pinyin_annotation (HimeContext *ctx);

/**
 * Get Pinyin annotation for a character
 * @param ctx Context handle
 * @param character UTF-8 character to look up
 * @param buffer Output buffer for Pinyin string
 * @param buffer_size Size of buffer
 * @return Length of Pinyin string, or 0 if not found
 */
HIME_API int hime_get_pinyin_for_char (
    HimeContext *ctx,
    const char *character,
    char *buffer,
    int buffer_size);

/**
 * Set candidate display style
 * @param ctx Context handle
 * @param style Display style to use
 */
HIME_API void hime_set_candidate_style (HimeContext *ctx, HimeCandidateStyle style);

/**
 * Get current candidate display style
 */
HIME_API HimeCandidateStyle hime_get_candidate_style (HimeContext *ctx);

/**
 * Register feedback callback for sound/vibration
 * @param ctx Context handle
 * @param callback Function to call on feedback events
 * @param user_data User data passed to callback
 */
HIME_API void hime_set_feedback_callback (
    HimeContext *ctx,
    HimeFeedbackCallback callback,
    void *user_data);

/**
 * Enable/disable sound feedback
 * @param ctx Context handle
 * @param enabled true to enable sound
 */
HIME_API void hime_set_sound_enabled (HimeContext *ctx, bool enabled);

/**
 * Check if sound feedback is enabled
 */
HIME_API bool hime_get_sound_enabled (HimeContext *ctx);

/**
 * Enable/disable vibration feedback
 * @param ctx Context handle
 * @param enabled true to enable vibration
 */
HIME_API void hime_set_vibration_enabled (HimeContext *ctx, bool enabled);

/**
 * Check if vibration feedback is enabled
 */
HIME_API bool hime_get_vibration_enabled (HimeContext *ctx);

/**
 * Set vibration duration in milliseconds
 * @param ctx Context handle
 * @param duration_ms Duration (typically 10-50ms)
 */
HIME_API void hime_set_vibration_duration (HimeContext *ctx, int duration_ms);

/**
 * Get vibration duration
 */
HIME_API int hime_get_vibration_duration (HimeContext *ctx);

/* ========== Smart Punctuation Conversion ========== */

/**
 * Punctuation pairs for smart conversion
 * These maintain state for paired punctuation like quotes
 */

/**
 * Reset punctuation pairing state
 * Call when focus changes to new text field
 */
HIME_API void hime_reset_punctuation_state (HimeContext *ctx);

/**
 * Convert ASCII punctuation to Chinese punctuation
 * @param ctx Context handle
 * @param ascii ASCII punctuation character
 * @param buffer Output buffer for UTF-8 Chinese punctuation
 * @param buffer_size Size of buffer
 * @return Length of converted string, or 0 if no conversion
 */
HIME_API int hime_convert_punctuation (
    HimeContext *ctx,
    char ascii,
    char *buffer,
    int buffer_size);

/* ========== Theme/Color Support ========== */

/**
 * Color scheme type
 */
typedef enum {
    HIME_COLOR_SCHEME_LIGHT = 0,
    HIME_COLOR_SCHEME_DARK = 1,
    HIME_COLOR_SCHEME_SYSTEM = 2 /* Follow system setting */
} HimeColorScheme;

/**
 * Set color scheme
 * @param ctx Context handle
 * @param scheme Color scheme to use
 */
HIME_API void hime_set_color_scheme (HimeContext *ctx, HimeColorScheme scheme);

/**
 * Get current color scheme
 */
HIME_API HimeColorScheme hime_get_color_scheme (HimeContext *ctx);

/**
 * Notify library of system dark mode state
 * Used when HIME_COLOR_SCHEME_SYSTEM is set
 * @param ctx Context handle
 * @param is_dark true if system is in dark mode
 */
HIME_API void hime_set_system_dark_mode (HimeContext *ctx, bool is_dark);

/* ========== Extended Candidate Info ========== */

/**
 * Get candidate with annotation
 * Returns candidate text along with optional Pinyin annotation
 * @param ctx Context handle
 * @param index Candidate index
 * @param text_buffer Output buffer for candidate text
 * @param text_size Size of text buffer
 * @param annotation_buffer Output buffer for annotation (can be NULL)
 * @param annotation_size Size of annotation buffer
 * @return Length of candidate text, or negative on error
 */
HIME_API int hime_get_candidate_with_annotation (
    HimeContext *ctx,
    int index,
    char *text_buffer,
    int text_size,
    char *annotation_buffer,
    int annotation_size);

/* ========== Input Method Search (UI Support) ========== */

/**
 * Search filter for input methods
 */
typedef struct {
    const char *query;           /* Search query (name pattern, case-insensitive) */
    HimeInputMethod method_type; /* Filter by method type (-1 for all) */
    bool include_disabled;       /* Include disabled methods */
} HimeSearchFilter;

/**
 * Search result entry
 */
typedef struct {
    int index;                   /* Index in registry */
    char name[64];               /* Display name (UTF-8) */
    char filename[128];          /* Table/module filename */
    HimeInputMethod method_type; /* Input method type */
    HimeGtabTable gtab_id;       /* GTAB table ID (if applicable) */
    int match_score;             /* Match quality (higher = better) */
} HimeSearchResult;

/**
 * Search input methods by name pattern
 * @param filter Search filter criteria
 * @param results Output array for search results
 * @param max_results Maximum number of results to return
 * @return Number of results found
 */
HIME_API int hime_search_methods (
    const HimeSearchFilter *filter,
    HimeSearchResult *results,
    int max_results);

/**
 * Search GTAB tables by name pattern
 * @param query Search string (case-insensitive, partial match)
 * @param results Output array for matching table info
 * @param max_results Maximum number of results
 * @return Number of results found
 */
HIME_API int hime_gtab_search_tables (
    const char *query,
    HimeGtabInfo *results,
    int max_results);

/**
 * Find input method by exact name (UTF-8)
 * @param name Method name to find
 * @return Method index or -1 if not found
 */
HIME_API int hime_find_method_by_name (const char *name);

/**
 * Get all input methods for display in UI
 * @param results Output array for results
 * @param max_results Maximum results to return
 * @return Number of methods returned
 */
HIME_API int hime_get_all_methods (
    HimeSearchResult *results,
    int max_results);

/* ========== Simplified/Traditional Chinese Conversion ========== */

/**
 * Output Chinese variant mode
 */
typedef enum {
    HIME_OUTPUT_TRADITIONAL = 0, /* Output Traditional Chinese (default) */
    HIME_OUTPUT_SIMPLIFIED = 1,  /* Output Simplified Chinese */
    HIME_OUTPUT_BOTH = 2         /* Show both variants in candidates */
} HimeOutputVariant;

/**
 * Set output Chinese variant
 * @param ctx Context handle
 * @param variant Output variant mode
 */
HIME_API void hime_set_output_variant (HimeContext *ctx, HimeOutputVariant variant);

/**
 * Get current output variant
 */
HIME_API HimeOutputVariant hime_get_output_variant (HimeContext *ctx);

/**
 * Toggle between Simplified and Traditional output
 * @param ctx Context handle
 * @return New output variant
 */
HIME_API HimeOutputVariant hime_toggle_output_variant (HimeContext *ctx);

/**
 * Convert Traditional Chinese string to Simplified Chinese
 * @param input UTF-8 Traditional Chinese string
 * @param output Output buffer for Simplified Chinese
 * @param output_size Size of output buffer
 * @return Length of output string, or negative on error
 */
HIME_API int hime_convert_trad_to_simp (
    const char *input,
    char *output,
    int output_size);

/**
 * Convert Simplified Chinese string to Traditional Chinese
 * @param input UTF-8 Simplified Chinese string
 * @param output Output buffer for Traditional Chinese
 * @param output_size Size of output buffer
 * @return Length of output string, or negative on error
 */
HIME_API int hime_convert_simp_to_trad (
    const char *input,
    char *output,
    int output_size);

/**
 * Convert string based on current output variant setting
 * @param ctx Context handle
 * @param input UTF-8 input string
 * @param output Output buffer
 * @param output_size Size of output buffer
 * @return Length of output string
 */
HIME_API int hime_convert_to_output_variant (
    HimeContext *ctx,
    const char *input,
    char *output,
    int output_size);

/**
 * Apply output variant conversion to all current candidates
 * Call this after candidates are generated to convert them
 * @param ctx Context handle
 */
HIME_API void hime_convert_candidates_to_variant (HimeContext *ctx);

#ifdef __cplusplus
}
#endif

#endif /* HIME_CORE_H */
