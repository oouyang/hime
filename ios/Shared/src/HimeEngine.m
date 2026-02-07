/*
 * HIME Engine Objective-C Wrapper Implementation
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#import "HimeEngine.h"
#include "hime-core.h"

@interface HimeEngine () {
    HimeContext *_ctx;
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

@end
