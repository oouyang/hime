/*
 * HIME Engine Objective-C Wrapper Implementation
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#import "HimeEngine.h"
#include "hime-core.h"

/* C callback trampoline */
static void feedbackTrampoline(HimeFeedbackType type, void *userData);

@interface HimeEngine () {
    HimeContext *_ctx;
    HimeFeedbackBlock _feedbackBlock;
}
@end

@implementation HimeEngine

- (nullable instancetype)initWithDataPath:(NSString *)dataPath {
    self = [super init];
    if (self) {
        /* Initialize HIME core */
        if (hime_init([dataPath UTF8String]) != 0) {
            NSLog(@"HIME: Failed to initialize with data path: %@", dataPath);
            return nil;
        }

        /* Create context */
        _ctx = hime_context_new();
        if (!_ctx) {
            NSLog(@"HIME: Failed to create context");
            return nil;
        }
    }
    return self;
}

- (void)dealloc {
    if (_ctx) {
        hime_context_free(_ctx);
        _ctx = NULL;
    }
}

#pragma mark - Mode Control

- (BOOL)chineseMode {
    return _ctx ? hime_is_chinese_mode(_ctx) : NO;
}

- (void)setChineseMode:(BOOL)chineseMode {
    if (_ctx) {
        hime_set_chinese_mode(_ctx, chineseMode);
    }
}

- (void)toggleChineseMode {
    if (_ctx) {
        hime_toggle_chinese_mode(_ctx);
    }
}

#pragma mark - Key Processing

- (HimeKeyResultType)processKeyCode:(unsigned short)keyCode
                          character:(unichar)character
                          modifiers:(HimeModifierFlags)modifiers {
    if (!_ctx) {
        return HimeKeyResultIgnored;
    }

    uint32_t himeModifiers = 0;
    if (modifiers & HimeModifierShift) himeModifiers |= HIME_MOD_SHIFT;
    if (modifiers & HimeModifierControl) himeModifiers |= HIME_MOD_CONTROL;
    if (modifiers & HimeModifierAlt) himeModifiers |= HIME_MOD_ALT;
    if (modifiers & HimeModifierCapsLock) himeModifiers |= HIME_MOD_CAPSLOCK;

    HimeKeyResult result = hime_process_key(_ctx, keyCode, character, himeModifiers);

    return (HimeKeyResultType)result;
}

#pragma mark - Preedit String

- (NSString *)preeditString {
    if (!_ctx) return @"";

    char buffer[HIME_MAX_PREEDIT];
    int len = hime_get_preedit(_ctx, buffer, sizeof(buffer));
    if (len <= 0) return @"";

    return [[NSString alloc] initWithBytes:buffer
                                    length:len
                                  encoding:NSUTF8StringEncoding] ?: @"";
}

- (NSUInteger)preeditCursor {
    return _ctx ? hime_get_preedit_cursor(_ctx) : 0;
}

#pragma mark - Commit String

- (NSString *)commitString {
    if (!_ctx) return @"";

    char buffer[HIME_MAX_PREEDIT];
    int len = hime_get_commit(_ctx, buffer, sizeof(buffer));
    if (len <= 0) return @"";

    return [[NSString alloc] initWithBytes:buffer
                                    length:len
                                  encoding:NSUTF8StringEncoding] ?: @"";
}

- (void)clearCommit {
    if (_ctx) {
        hime_clear_commit(_ctx);
    }
}

#pragma mark - Candidates

- (BOOL)hasCandidates {
    return _ctx ? hime_has_candidates(_ctx) : NO;
}

- (NSInteger)candidateCount {
    return _ctx ? hime_get_candidate_count(_ctx) : 0;
}

- (NSInteger)currentPage {
    return _ctx ? hime_get_candidate_page(_ctx) : 0;
}

- (NSInteger)candidatesPerPage {
    return _ctx ? hime_get_candidates_per_page(_ctx) : 10;
}

- (nullable NSString *)candidateAtIndex:(NSInteger)index {
    if (!_ctx) return nil;

    char buffer[HIME_MAX_CANDIDATE_LEN];
    int len = hime_get_candidate(_ctx, (int)index, buffer, sizeof(buffer));
    if (len <= 0) return nil;

    return [[NSString alloc] initWithBytes:buffer
                                    length:len
                                  encoding:NSUTF8StringEncoding];
}

- (NSArray<NSString *> *)candidatesForCurrentPage {
    NSMutableArray *candidates = [NSMutableArray array];

    if (!_ctx) return candidates;

    NSInteger page = self.currentPage;
    NSInteger perPage = self.candidatesPerPage;
    NSInteger start = page * perPage;
    NSInteger count = self.candidateCount;
    NSInteger end = MIN(start + perPage, count);

    for (NSInteger i = start; i < end; i++) {
        NSString *candidate = [self candidateAtIndex:i];
        if (candidate) {
            [candidates addObject:candidate];
        }
    }

    return candidates;
}

- (HimeKeyResultType)selectCandidateAtIndex:(NSInteger)index {
    if (!_ctx) return HimeKeyResultIgnored;

    HimeKeyResult result = hime_select_candidate(_ctx, (int)index);
    return (HimeKeyResultType)result;
}

- (BOOL)pageUp {
    return _ctx ? hime_candidate_page_up(_ctx) : NO;
}

- (BOOL)pageDown {
    return _ctx ? hime_candidate_page_down(_ctx) : NO;
}

#pragma mark - Reset

- (void)reset {
    if (_ctx) {
        hime_context_reset(_ctx);
    }
}

#pragma mark - Character Set

- (HimeCharsetType)charset {
    return _ctx ? (HimeCharsetType)hime_get_charset(_ctx) : HimeCharsetTraditional;
}

- (void)setCharset:(HimeCharsetType)charset {
    if (_ctx) {
        hime_set_charset(_ctx, (HimeCharset)charset);
    }
}

- (HimeCharsetType)toggleCharset {
    if (_ctx) {
        return (HimeCharsetType)hime_toggle_charset(_ctx);
    }
    return HimeCharsetTraditional;
}

#pragma mark - Smart Punctuation

- (BOOL)smartPunctuation {
    return _ctx ? hime_get_smart_punctuation(_ctx) : NO;
}

- (void)setSmartPunctuation:(BOOL)smartPunctuation {
    if (_ctx) {
        hime_set_smart_punctuation(_ctx, smartPunctuation);
    }
}

- (nullable NSString *)convertPunctuation:(char)ascii {
    if (!_ctx) return nil;

    char buffer[16];
    int len = hime_convert_punctuation(_ctx, ascii, buffer, sizeof(buffer));
    if (len <= 0) return nil;

    return [[NSString alloc] initWithBytes:buffer
                                    length:len
                                  encoding:NSUTF8StringEncoding];
}

- (void)resetPunctuationState {
    if (_ctx) {
        hime_reset_punctuation_state(_ctx);
    }
}

#pragma mark - Pinyin Annotation

- (BOOL)pinyinAnnotation {
    return _ctx ? hime_get_pinyin_annotation(_ctx) : NO;
}

- (void)setPinyinAnnotation:(BOOL)pinyinAnnotation {
    if (_ctx) {
        hime_set_pinyin_annotation(_ctx, pinyinAnnotation);
    }
}

- (nullable NSString *)pinyinForCharacter:(NSString *)character {
    if (!_ctx || !character.length) return nil;

    char buffer[64];
    int len = hime_get_pinyin_for_char(_ctx, [character UTF8String], buffer, sizeof(buffer));
    if (len <= 0) return nil;

    return [[NSString alloc] initWithBytes:buffer
                                    length:len
                                  encoding:NSUTF8StringEncoding];
}

#pragma mark - Candidate Style

- (HimeCandidateStyleType)candidateStyle {
    return _ctx ? (HimeCandidateStyleType)hime_get_candidate_style(_ctx) : HimeCandidateStyleHorizontal;
}

- (void)setCandidateStyle:(HimeCandidateStyleType)candidateStyle {
    if (_ctx) {
        hime_set_candidate_style(_ctx, (HimeCandidateStyle)candidateStyle);
    }
}

#pragma mark - Color Scheme

- (HimeColorSchemeType)colorScheme {
    return _ctx ? (HimeColorSchemeType)hime_get_color_scheme(_ctx) : HimeColorSchemeLight;
}

- (void)setColorScheme:(HimeColorSchemeType)colorScheme {
    if (_ctx) {
        hime_set_color_scheme(_ctx, (HimeColorScheme)colorScheme);
    }
}

- (void)setSystemDarkMode:(BOOL)isDark {
    if (_ctx) {
        hime_set_system_dark_mode(_ctx, isDark);
    }
}

#pragma mark - Keyboard Layout

- (HimeKeyboardLayoutType)keyboardLayout {
    return _ctx ? (HimeKeyboardLayoutType)hime_get_keyboard_layout(_ctx) : HimeKeyboardLayoutStandard;
}

- (void)setKeyboardLayout:(HimeKeyboardLayoutType)keyboardLayout {
    if (_ctx) {
        hime_set_keyboard_layout(_ctx, (HimeKeyboardLayout)keyboardLayout);
    }
}

- (BOOL)setKeyboardLayoutByName:(NSString *)layoutName {
    if (!_ctx || !layoutName.length) return NO;
    return hime_set_keyboard_layout_by_name(_ctx, [layoutName UTF8String]) == 0;
}

#pragma mark - Feedback

- (BOOL)soundEnabled {
    return _ctx ? hime_get_sound_enabled(_ctx) : NO;
}

- (void)setSoundEnabled:(BOOL)soundEnabled {
    if (_ctx) {
        hime_set_sound_enabled(_ctx, soundEnabled);
    }
}

- (BOOL)vibrationEnabled {
    return _ctx ? hime_get_vibration_enabled(_ctx) : NO;
}

- (void)setVibrationEnabled:(BOOL)vibrationEnabled {
    if (_ctx) {
        hime_set_vibration_enabled(_ctx, vibrationEnabled);
    }
}

- (NSInteger)vibrationDuration {
    return _ctx ? hime_get_vibration_duration(_ctx) : 20;
}

- (void)setVibrationDuration:(NSInteger)vibrationDuration {
    if (_ctx) {
        hime_set_vibration_duration(_ctx, (int)vibrationDuration);
    }
}

- (HimeFeedbackBlock)feedbackHandler {
    return _feedbackBlock;
}

- (void)setFeedbackHandler:(HimeFeedbackBlock)feedbackHandler {
    _feedbackBlock = [feedbackHandler copy];
    if (_ctx) {
        if (_feedbackBlock) {
            hime_set_feedback_callback(_ctx, feedbackTrampoline, (__bridge void *)self);
        } else {
            hime_set_feedback_callback(_ctx, NULL, NULL);
        }
    }
}

@end

/* C callback trampoline implementation */
static void feedbackTrampoline(HimeFeedbackType type, void *userData) {
    HimeEngine *engine = (__bridge HimeEngine *)userData;
    if (engine && engine->_feedbackBlock) {
        engine->_feedbackBlock((HimeFeedbackEventType)type);
    }
}
