/*
 * HIME macOS Unit Tests - XCTest Suite
 *
 * XCTest-based unit tests for HimeEngine ObjC wrapper and hime-core C API.
 * These tests require macOS with Xcode (BUILD_TESTS=ON).
 *
 * For platform-independent C tests, see test-hime-core.c,
 * test-hime-crypt.c, and test-hime-conf.c in this directory.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#import <XCTest/XCTest.h>
#import "HimeEngine.h"
#import "hime-core.h"

#pragma mark - hime-core C API Tests

@interface HimeCoreXCTests : XCTestCase
@end

@implementation HimeCoreXCTests

- (void)testVersion {
    const char *version = hime_version();
    XCTAssertTrue(version != NULL, @"Version should not be NULL");
    XCTAssertTrue(strlen(version) > 0, @"Version should not be empty");
}

- (void)testContextCreate {
    HimeContext *ctx = hime_context_new();
    XCTAssertTrue(ctx != NULL, @"Context should be created");
    hime_context_free(ctx);
}

- (void)testContextInitialState {
    HimeContext *ctx = hime_context_new();
    XCTAssertTrue(ctx != NULL);
    XCTAssertTrue(hime_is_chinese_mode(ctx));
    XCTAssertEqual(hime_get_input_method(ctx), HIME_IM_PHO);
    XCTAssertFalse(hime_has_candidates(ctx));
    XCTAssertEqual(hime_get_candidate_count(ctx), 0);
    hime_context_free(ctx);
}

- (void)testContextReset {
    HimeContext *ctx = hime_context_new();
    hime_context_reset(ctx);
    XCTAssertTrue(hime_is_chinese_mode(ctx));
    hime_context_free(ctx);
}

- (void)testToggleChineseMode {
    HimeContext *ctx = hime_context_new();
    XCTAssertTrue(hime_is_chinese_mode(ctx));

    bool result = hime_toggle_chinese_mode(ctx);
    XCTAssertFalse(result);
    XCTAssertFalse(hime_is_chinese_mode(ctx));

    result = hime_toggle_chinese_mode(ctx);
    XCTAssertTrue(result);
    XCTAssertTrue(hime_is_chinese_mode(ctx));

    hime_context_free(ctx);
}

- (void)testSetInputMethod {
    HimeContext *ctx = hime_context_new();
    XCTAssertEqual(hime_set_input_method(ctx, HIME_IM_TSIN), 0);
    XCTAssertEqual(hime_get_input_method(ctx), HIME_IM_TSIN);
    XCTAssertEqual(hime_set_input_method(ctx, HIME_IM_PHO), 0);
    XCTAssertEqual(hime_get_input_method(ctx), HIME_IM_PHO);
    hime_context_free(ctx);
}

- (void)testPreeditEmpty {
    HimeContext *ctx = hime_context_new();
    char buf[256];
    int len = hime_get_preedit(ctx, buf, sizeof(buf));
    XCTAssertEqual(len, 0);
    XCTAssertEqual(strcmp(buf, ""), 0);
    hime_context_free(ctx);
}

- (void)testCandidatesPerPage {
    HimeContext *ctx = hime_context_new();
    XCTAssertEqual(hime_get_candidates_per_page(ctx), 10);
    hime_set_candidates_per_page(ctx, 5);
    XCTAssertEqual(hime_get_candidates_per_page(ctx), 5);
    hime_set_candidates_per_page(ctx, 0);
    XCTAssertEqual(hime_get_candidates_per_page(ctx), 1);
    hime_set_candidates_per_page(ctx, 100);
    XCTAssertEqual(hime_get_candidates_per_page(ctx), 10);
    hime_context_free(ctx);
}

- (void)testKeyInEnglishMode {
    HimeContext *ctx = hime_context_new();
    hime_set_chinese_mode(ctx, false);
    XCTAssertEqual(hime_process_key(ctx, 'A', 'a', 0), HIME_KEY_IGNORED);
    hime_context_free(ctx);
}

- (void)testNullContextSafety {
    XCTAssertFalse(hime_is_chinese_mode(NULL));
    XCTAssertEqual(hime_get_input_method(NULL), HIME_IM_PHO);
    XCTAssertEqual(hime_set_input_method(NULL, HIME_IM_PHO), -1);
    XCTAssertFalse(hime_toggle_chinese_mode(NULL));

    char buf[64];
    XCTAssertEqual(hime_get_preedit(NULL, buf, sizeof(buf)), -1);
    XCTAssertEqual(hime_get_commit(NULL, buf, sizeof(buf)), -1);
    XCTAssertEqual(hime_get_preedit_cursor(NULL), 0);
    XCTAssertFalse(hime_has_candidates(NULL));
    XCTAssertEqual(hime_get_candidate_count(NULL), 0);
    XCTAssertEqual(hime_process_key(NULL, 'A', 'a', 0), HIME_KEY_IGNORED);
    XCTAssertEqual(hime_select_candidate(NULL, 0), HIME_KEY_IGNORED);

    hime_context_reset(NULL);
    hime_set_chinese_mode(NULL, true);
    hime_clear_commit(NULL);
    hime_context_free(NULL);
}

- (void)testKeyboardLayout {
    HimeContext *ctx = hime_context_new();
    XCTAssertEqual(hime_get_keyboard_layout(ctx), HIME_KB_STANDARD);
    hime_set_keyboard_layout(ctx, HIME_KB_HSU);
    XCTAssertEqual(hime_get_keyboard_layout(ctx), HIME_KB_HSU);
    hime_context_free(ctx);
}

- (void)testCharset {
    HimeContext *ctx = hime_context_new();
    XCTAssertEqual(hime_get_charset(ctx), HIME_CHARSET_TRADITIONAL);
    hime_set_charset(ctx, HIME_CHARSET_SIMPLIFIED);
    XCTAssertEqual(hime_get_charset(ctx), HIME_CHARSET_SIMPLIFIED);
    XCTAssertEqual(hime_toggle_charset(ctx), HIME_CHARSET_TRADITIONAL);
    hime_context_free(ctx);
}

- (void)testColorScheme {
    HimeContext *ctx = hime_context_new();
    XCTAssertEqual(hime_get_color_scheme(ctx), HIME_COLOR_SCHEME_SYSTEM);
    hime_set_color_scheme(ctx, HIME_COLOR_SCHEME_DARK);
    XCTAssertEqual(hime_get_color_scheme(ctx), HIME_COLOR_SCHEME_DARK);
    hime_context_free(ctx);
}

- (void)testMultipleContexts {
    HimeContext *ctx1 = hime_context_new();
    HimeContext *ctx2 = hime_context_new();
    hime_set_chinese_mode(ctx1, false);
    XCTAssertFalse(hime_is_chinese_mode(ctx1));
    XCTAssertTrue(hime_is_chinese_mode(ctx2));
    hime_context_free(ctx1);
    hime_context_free(ctx2);
}

@end

#pragma mark - Enum Value Tests

@interface HimeEnumXCTests : XCTestCase
@end

@implementation HimeEnumXCTests

- (void)testKeyResultEnumValues {
    XCTAssertEqual(HimeKeyResultIgnored, 0);
    XCTAssertEqual(HimeKeyResultAbsorbed, 1);
    XCTAssertEqual(HimeKeyResultCommit, 2);
    XCTAssertEqual(HimeKeyResultPreedit, 3);
}

- (void)testModifierFlagValues {
    XCTAssertEqual(HimeModifierNone, 0);
    XCTAssertEqual(HimeModifierShift, 1);
    XCTAssertEqual(HimeModifierControl, 2);
    XCTAssertEqual(HimeModifierAlt, 4);
    XCTAssertEqual(HimeModifierCapsLock, 8);
}

- (void)testCharsetEnumValues {
    XCTAssertEqual(HimeCharsetTraditional, 0);
    XCTAssertEqual(HimeCharsetSimplified, 1);
}

- (void)testCandidateStyleEnumValues {
    XCTAssertEqual(HimeCandidateStyleHorizontal, 0);
    XCTAssertEqual(HimeCandidateStyleVertical, 1);
    XCTAssertEqual(HimeCandidateStylePopup, 2);
}

- (void)testColorSchemeEnumValues {
    XCTAssertEqual(HimeColorSchemeLight, 0);
    XCTAssertEqual(HimeColorSchemeDark, 1);
    XCTAssertEqual(HimeColorSchemeSystem, 2);
}

- (void)testKeyboardLayoutEnumValues {
    XCTAssertEqual(HimeKeyboardLayoutStandard, 0);
    XCTAssertEqual(HimeKeyboardLayoutHSU, 1);
    XCTAssertEqual(HimeKeyboardLayoutETen, 2);
    XCTAssertEqual(HimeKeyboardLayoutETen26, 3);
    XCTAssertEqual(HimeKeyboardLayoutIBM, 4);
    XCTAssertEqual(HimeKeyboardLayoutPinyin, 5);
    XCTAssertEqual(HimeKeyboardLayoutDvorak, 6);
}

- (void)testFeedbackEventEnumValues {
    XCTAssertEqual(HimeFeedbackKeyPress, 0);
    XCTAssertEqual(HimeFeedbackKeyDelete, 1);
    XCTAssertEqual(HimeFeedbackKeyEnter, 2);
    XCTAssertEqual(HimeFeedbackKeySpace, 3);
    XCTAssertEqual(HimeFeedbackCandidate, 4);
    XCTAssertEqual(HimeFeedbackModeChange, 5);
    XCTAssertEqual(HimeFeedbackError, 6);
}

- (void)testCApiEnumConsistency {
    XCTAssertEqual((int)HimeKeyResultIgnored, (int)HIME_KEY_IGNORED);
    XCTAssertEqual((int)HimeKeyResultAbsorbed, (int)HIME_KEY_ABSORBED);
    XCTAssertEqual((int)HimeKeyResultCommit, (int)HIME_KEY_COMMIT);
    XCTAssertEqual((int)HimeKeyResultPreedit, (int)HIME_KEY_PREEDIT);

    XCTAssertEqual((int)HimeCharsetTraditional, (int)HIME_CHARSET_TRADITIONAL);
    XCTAssertEqual((int)HimeCharsetSimplified, (int)HIME_CHARSET_SIMPLIFIED);

    XCTAssertEqual((int)HimeColorSchemeLight, (int)HIME_COLOR_SCHEME_LIGHT);
    XCTAssertEqual((int)HimeColorSchemeDark, (int)HIME_COLOR_SCHEME_DARK);
    XCTAssertEqual((int)HimeColorSchemeSystem, (int)HIME_COLOR_SCHEME_SYSTEM);

    XCTAssertEqual((int)HimeKeyboardLayoutStandard, (int)HIME_KB_STANDARD);
    XCTAssertEqual((int)HimeKeyboardLayoutDvorak, (int)HIME_KB_DVORAK);
}

@end
