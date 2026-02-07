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

#include <stdint.h>
#include <stdbool.h>

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
    int start;  /* Start byte offset */
    int end;    /* End byte offset */
} HimePreeditAttr;

/* Input method types */
typedef enum {
    HIME_IM_PHO = 0,      /* Phonetic (Bopomofo/Zhuyin) */
    HIME_IM_TSIN = 1,     /* Phrase input */
    HIME_IM_GTAB = 2,     /* Table-based (Cangjie, etc.) */
    HIME_IM_ANTHY = 3,    /* Japanese Anthy */
    HIME_IM_CHEWING = 4   /* Chewing library */
} HimeInputMethod;

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
    HIME_KEY_IGNORED = 0,   /* Key not handled, pass through */
    HIME_KEY_ABSORBED = 1,  /* Key handled, no output */
    HIME_KEY_COMMIT = 2,    /* Key handled, commit string ready */
    HIME_KEY_PREEDIT = 3    /* Key handled, preedit updated */
} HimeKeyResult;

/* Opaque context handle */
typedef struct HimeContext HimeContext;

/* ========== Initialization ========== */

/**
 * Initialize HIME core library
 * @param data_dir Path to HIME data directory (tables, configs)
 * @return 0 on success, negative on error
 */
HIME_API int hime_init(const char *data_dir);

/**
 * Cleanup HIME core library
 */
HIME_API void hime_cleanup(void);

/**
 * Get HIME version string
 */
HIME_API const char *hime_version(void);

/* ========== Context Management ========== */

/**
 * Create a new input context
 * @return Context handle, or NULL on failure
 */
HIME_API HimeContext *hime_context_new(void);

/**
 * Destroy an input context
 */
HIME_API void hime_context_free(HimeContext *ctx);

/**
 * Reset context to initial state
 */
HIME_API void hime_context_reset(HimeContext *ctx);

/* ========== Input Method Control ========== */

/**
 * Set the active input method
 * @param ctx Context handle
 * @param method Input method type
 * @return 0 on success
 */
HIME_API int hime_set_input_method(HimeContext *ctx, HimeInputMethod method);

/**
 * Get the active input method
 */
HIME_API HimeInputMethod hime_get_input_method(HimeContext *ctx);

/**
 * Toggle Chinese/English mode
 * @return true if now in Chinese mode
 */
HIME_API bool hime_toggle_chinese_mode(HimeContext *ctx);

/**
 * Check if in Chinese mode
 */
HIME_API bool hime_is_chinese_mode(HimeContext *ctx);

/**
 * Set Chinese mode directly
 */
HIME_API void hime_set_chinese_mode(HimeContext *ctx, bool chinese);

/* ========== Key Processing ========== */

/**
 * Process a key press event
 * @param ctx Context handle
 * @param keycode Virtual key code (platform-specific)
 * @param charcode Unicode character code (0 if not printable)
 * @param modifiers Modifier key flags
 * @return Key processing result
 */
HIME_API HimeKeyResult hime_process_key(
    HimeContext *ctx,
    uint32_t keycode,
    uint32_t charcode,
    uint32_t modifiers
);

/* ========== Preedit String ========== */

/**
 * Get the current preedit (composition) string
 * @param ctx Context handle
 * @param buffer Output buffer for UTF-8 string
 * @param buffer_size Size of buffer
 * @return Length of preedit string, or negative on error
 */
HIME_API int hime_get_preedit(
    HimeContext *ctx,
    char *buffer,
    int buffer_size
);

/**
 * Get preedit cursor position (byte offset)
 */
HIME_API int hime_get_preedit_cursor(HimeContext *ctx);

/**
 * Get preedit attributes
 * @param ctx Context handle
 * @param attrs Output array for attributes
 * @param max_attrs Maximum number of attributes to return
 * @return Number of attributes
 */
HIME_API int hime_get_preedit_attrs(
    HimeContext *ctx,
    HimePreeditAttr *attrs,
    int max_attrs
);

/* ========== Commit String ========== */

/**
 * Get the commit string (text to insert)
 * @param ctx Context handle
 * @param buffer Output buffer for UTF-8 string
 * @param buffer_size Size of buffer
 * @return Length of commit string, or negative on error
 */
HIME_API int hime_get_commit(
    HimeContext *ctx,
    char *buffer,
    int buffer_size
);

/**
 * Clear the commit string after retrieving
 */
HIME_API void hime_clear_commit(HimeContext *ctx);

/* ========== Candidates ========== */

/**
 * Check if candidate window should be shown
 */
HIME_API bool hime_has_candidates(HimeContext *ctx);

/**
 * Get number of candidates
 */
HIME_API int hime_get_candidate_count(HimeContext *ctx);

/**
 * Get a candidate string by index
 * @param ctx Context handle
 * @param index Candidate index (0-based)
 * @param buffer Output buffer for UTF-8 string
 * @param buffer_size Size of buffer
 * @return Length of candidate string, or negative on error
 */
HIME_API int hime_get_candidate(
    HimeContext *ctx,
    int index,
    char *buffer,
    int buffer_size
);

/**
 * Get current candidate page
 */
HIME_API int hime_get_candidate_page(HimeContext *ctx);

/**
 * Get candidates per page
 */
HIME_API int hime_get_candidates_per_page(HimeContext *ctx);

/**
 * Select a candidate by index
 * @return HIME_KEY_COMMIT if selection successful
 */
HIME_API HimeKeyResult hime_select_candidate(HimeContext *ctx, int index);

/**
 * Move to previous candidate page
 * @return true if page changed
 */
HIME_API bool hime_candidate_page_up(HimeContext *ctx);

/**
 * Move to next candidate page
 * @return true if page changed
 */
HIME_API bool hime_candidate_page_down(HimeContext *ctx);

/* ========== Phonetic-specific ========== */

/**
 * Get the Bopomofo display string for current input
 * @param ctx Context handle
 * @param buffer Output buffer for UTF-8 Bopomofo symbols
 * @param buffer_size Size of buffer
 * @return Length of string
 */
HIME_API int hime_get_bopomofo_string(
    HimeContext *ctx,
    char *buffer,
    int buffer_size
);

/* ========== Configuration ========== */

/**
 * Set keyboard layout for phonetic input
 * @param ctx Context handle
 * @param layout_name Layout name ("zo", "et", "hsu", "pinyin", etc.)
 * @return 0 on success
 */
HIME_API int hime_set_keyboard_layout(HimeContext *ctx, const char *layout_name);

/**
 * Set selection keys string (e.g., "1234567890")
 */
HIME_API void hime_set_selection_keys(HimeContext *ctx, const char *keys);

/**
 * Set candidates per page
 */
HIME_API void hime_set_candidates_per_page(HimeContext *ctx, int count);

/* ========== Settings/Preferences (New Features) ========== */

/**
 * Character set type for Simplified/Traditional Chinese
 */
typedef enum {
    HIME_CHARSET_TRADITIONAL = 0,  /* Traditional Chinese (Big5 ordering) */
    HIME_CHARSET_SIMPLIFIED = 1    /* Simplified Chinese (GB ordering) */
} HimeCharset;

/**
 * Candidate display style
 */
typedef enum {
    HIME_CAND_STYLE_HORIZONTAL = 0,  /* Horizontal inline bar */
    HIME_CAND_STYLE_VERTICAL = 1,    /* Vertical list */
    HIME_CAND_STYLE_POPUP = 2        /* Floating popup window */
} HimeCandidateStyle;

/**
 * Feedback event types
 */
typedef enum {
    HIME_FEEDBACK_KEY_PRESS = 0,     /* Regular key press */
    HIME_FEEDBACK_KEY_DELETE = 1,    /* Delete/backspace */
    HIME_FEEDBACK_KEY_ENTER = 2,     /* Enter/return */
    HIME_FEEDBACK_KEY_SPACE = 3,     /* Space bar */
    HIME_FEEDBACK_CANDIDATE = 4,     /* Candidate selection */
    HIME_FEEDBACK_MODE_CHANGE = 5,   /* Mode switch */
    HIME_FEEDBACK_ERROR = 6          /* Invalid input */
} HimeFeedbackType;

/**
 * Feedback callback function type
 * @param type Feedback event type
 * @param user_data User-provided data pointer
 */
typedef void (*HimeFeedbackCallback)(HimeFeedbackType type, void *user_data);

/**
 * Set Simplified/Traditional Chinese mode
 * @param ctx Context handle
 * @param charset Character set to use
 */
HIME_API void hime_set_charset(HimeContext *ctx, HimeCharset charset);

/**
 * Get current character set
 */
HIME_API HimeCharset hime_get_charset(HimeContext *ctx);

/**
 * Toggle between Simplified and Traditional
 * @return New character set value
 */
HIME_API HimeCharset hime_toggle_charset(HimeContext *ctx);

/**
 * Enable/disable smart punctuation conversion
 * When enabled, ASCII punctuation is auto-converted to Chinese punctuation
 * @param ctx Context handle
 * @param enabled true to enable smart punctuation
 */
HIME_API void hime_set_smart_punctuation(HimeContext *ctx, bool enabled);

/**
 * Check if smart punctuation is enabled
 */
HIME_API bool hime_get_smart_punctuation(HimeContext *ctx);

/**
 * Enable/disable Pinyin annotation display
 * When enabled, candidates show pronunciation hints
 * @param ctx Context handle
 * @param enabled true to show Pinyin annotations
 */
HIME_API void hime_set_pinyin_annotation(HimeContext *ctx, bool enabled);

/**
 * Check if Pinyin annotation is enabled
 */
HIME_API bool hime_get_pinyin_annotation(HimeContext *ctx);

/**
 * Get Pinyin annotation for a character
 * @param ctx Context handle
 * @param character UTF-8 character to look up
 * @param buffer Output buffer for Pinyin string
 * @param buffer_size Size of buffer
 * @return Length of Pinyin string, or 0 if not found
 */
HIME_API int hime_get_pinyin_for_char(
    HimeContext *ctx,
    const char *character,
    char *buffer,
    int buffer_size
);

/**
 * Set candidate display style
 * @param ctx Context handle
 * @param style Display style to use
 */
HIME_API void hime_set_candidate_style(HimeContext *ctx, HimeCandidateStyle style);

/**
 * Get current candidate display style
 */
HIME_API HimeCandidateStyle hime_get_candidate_style(HimeContext *ctx);

/**
 * Register feedback callback for sound/vibration
 * @param ctx Context handle
 * @param callback Function to call on feedback events
 * @param user_data User data passed to callback
 */
HIME_API void hime_set_feedback_callback(
    HimeContext *ctx,
    HimeFeedbackCallback callback,
    void *user_data
);

/**
 * Enable/disable sound feedback
 * @param ctx Context handle
 * @param enabled true to enable sound
 */
HIME_API void hime_set_sound_enabled(HimeContext *ctx, bool enabled);

/**
 * Check if sound feedback is enabled
 */
HIME_API bool hime_get_sound_enabled(HimeContext *ctx);

/**
 * Enable/disable vibration feedback
 * @param ctx Context handle
 * @param enabled true to enable vibration
 */
HIME_API void hime_set_vibration_enabled(HimeContext *ctx, bool enabled);

/**
 * Check if vibration feedback is enabled
 */
HIME_API bool hime_get_vibration_enabled(HimeContext *ctx);

/**
 * Set vibration duration in milliseconds
 * @param ctx Context handle
 * @param duration_ms Duration (typically 10-50ms)
 */
HIME_API void hime_set_vibration_duration(HimeContext *ctx, int duration_ms);

/**
 * Get vibration duration
 */
HIME_API int hime_get_vibration_duration(HimeContext *ctx);

/* ========== Smart Punctuation Conversion ========== */

/**
 * Punctuation pairs for smart conversion
 * These maintain state for paired punctuation like quotes
 */

/**
 * Reset punctuation pairing state
 * Call when focus changes to new text field
 */
HIME_API void hime_reset_punctuation_state(HimeContext *ctx);

/**
 * Convert ASCII punctuation to Chinese punctuation
 * @param ctx Context handle
 * @param ascii ASCII punctuation character
 * @param buffer Output buffer for UTF-8 Chinese punctuation
 * @param buffer_size Size of buffer
 * @return Length of converted string, or 0 if no conversion
 */
HIME_API int hime_convert_punctuation(
    HimeContext *ctx,
    char ascii,
    char *buffer,
    int buffer_size
);

/* ========== Theme/Color Support ========== */

/**
 * Color scheme type
 */
typedef enum {
    HIME_COLOR_SCHEME_LIGHT = 0,
    HIME_COLOR_SCHEME_DARK = 1,
    HIME_COLOR_SCHEME_SYSTEM = 2   /* Follow system setting */
} HimeColorScheme;

/**
 * Set color scheme
 * @param ctx Context handle
 * @param scheme Color scheme to use
 */
HIME_API void hime_set_color_scheme(HimeContext *ctx, HimeColorScheme scheme);

/**
 * Get current color scheme
 */
HIME_API HimeColorScheme hime_get_color_scheme(HimeContext *ctx);

/**
 * Notify library of system dark mode state
 * Used when HIME_COLOR_SCHEME_SYSTEM is set
 * @param ctx Context handle
 * @param is_dark true if system is in dark mode
 */
HIME_API void hime_set_system_dark_mode(HimeContext *ctx, bool is_dark);

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
HIME_API int hime_get_candidate_with_annotation(
    HimeContext *ctx,
    int index,
    char *text_buffer,
    int text_size,
    char *annotation_buffer,
    int annotation_size
);

#ifdef __cplusplus
}
#endif

#endif /* HIME_CORE_H */
