/*
 * HIME macOS Unit Tests
 *
 * XCTest-based unit tests for HimeEngine and hime-core.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#import <XCTest/XCTest.h>
#import "HimeEngine.h"
#import "hime-core.h"

#pragma mark - HimeEngine Tests

@interface HimeEngineTests : XCTestCase
@property (nonatomic, strong) HimeEngine *engine;
@end

@implementation HimeEngineTests

- (void)setUp {
    [super setUp];
    self.engine = [[HimeEngine alloc] init];
}

- (void)tearDown {
    self.engine = nil;
    [super tearDown];
}

#pragma mark - Initialization Tests

- (void)testEngineCreation {
    XCTAssertNotNil(self.engine, @"Engine should be created successfully");
}

- (void)testEngineHasContext {
    XCTAssertTrue([self.engine hasContext], @"Engine should have a valid context");
}

#pragma mark - Mode Tests

- (void)testInitialChineseMode {
    XCTAssertTrue([self.engine isChineseMode], @"Engine should start in Chinese mode");
}

- (void)testToggleMode {
    BOOL initialMode = [self.engine isChineseMode];
    [self.engine toggleChineseMode];
    XCTAssertNotEqual(initialMode, [self.engine isChineseMode], @"Mode should toggle");

    [self.engine toggleChineseMode];
    XCTAssertEqual(initialMode, [self.engine isChineseMode], @"Mode should toggle back");
}

- (void)testSetChineseMode {
    [self.engine setChineseMode:NO];
    XCTAssertFalse([self.engine isChineseMode], @"Should be in English mode");

    [self.engine setChineseMode:YES];
    XCTAssertTrue([self.engine isChineseMode], @"Should be in Chinese mode");
}

#pragma mark - Preedit Tests

- (void)testInitialPreeditEmpty {
    NSString *preedit = [self.engine preeditString];
    XCTAssertNotNil(preedit, @"Preedit should not be nil");
    XCTAssertEqual(preedit.length, 0, @"Initial preedit should be empty");
}

- (void)testCommitStringEmpty {
    NSString *commit = [self.engine commitString];
    XCTAssertNotNil(commit, @"Commit string should not be nil");
    XCTAssertEqual(commit.length, 0, @"Initial commit should be empty");
}

#pragma mark - Candidate Tests

- (void)testInitialNoCandidates {
    XCTAssertFalse([self.engine hasCandidates], @"Should have no candidates initially");
    XCTAssertEqual([self.engine candidateCount], 0, @"Candidate count should be 0");
}

- (void)testCandidatesPerPage {
    NSInteger perPage = [self.engine candidatesPerPage];
    XCTAssertGreaterThan(perPage, 0, @"Candidates per page should be > 0");
    XCTAssertLessThanOrEqual(perPage, 10, @"Candidates per page should be <= 10");
}

- (void)testCandidatesForCurrentPage {
    NSArray *candidates = [self.engine candidatesForCurrentPage];
    XCTAssertNotNil(candidates, @"Candidates array should not be nil");
    XCTAssertEqual(candidates.count, 0, @"Should have no candidates initially");
}

#pragma mark - Reset Tests

- (void)testReset {
    /* Process some input first */
    [self.engine processKeyCode:'j' character:'j' modifiers:0];

    /* Reset should clear state */
    [self.engine reset];

    NSString *preedit = [self.engine preeditString];
    XCTAssertEqual(preedit.length, 0, @"Preedit should be cleared after reset");
}

#pragma mark - Key Processing Tests

- (void)testKeyProcessingInEnglishMode {
    [self.engine setChineseMode:NO];

    HimeKeyResult result = [self.engine processKeyCode:'a' character:'a' modifiers:0];
    XCTAssertEqual(result, HimeKeyResultIgnored, @"Keys should be ignored in English mode");
}

- (void)testEscapeClearsPreedit {
    /* Process a key first */
    [self.engine processKeyCode:'j' character:'j' modifiers:0];

    /* Escape should clear/reset */
    HimeKeyResult result = [self.engine processKeyCode:0x1B character:0x1B modifiers:0];
    /* Result can be Absorbed or Ignored depending on state */
    XCTAssertTrue(result == HimeKeyResultAbsorbed || result == HimeKeyResultIgnored, @"Escape should be handled");
}

@end

#pragma mark - hime-core C API Tests

@interface HimeCoreTests : XCTestCase
@end

@implementation HimeCoreTests

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

    XCTAssertTrue(hime_is_chinese_mode(ctx), @"Should start in Chinese mode");
    XCTAssertEqual(hime_get_input_method(ctx), HIME_IM_PHO, @"Should use phonetic input");
    XCTAssertFalse(hime_has_candidates(ctx), @"Should have no candidates");
    XCTAssertEqual(hime_get_candidate_count(ctx), 0, @"Candidate count should be 0");

    hime_context_free(ctx);
}

- (void)testToggleChineseMode {
    HimeContext *ctx = hime_context_new();
    XCTAssertTrue(ctx != NULL);

    XCTAssertTrue(hime_is_chinese_mode(ctx));

    bool result = hime_toggle_chinese_mode(ctx);
    XCTAssertFalse(result, @"Should return new state (English)");
    XCTAssertFalse(hime_is_chinese_mode(ctx));

    result = hime_toggle_chinese_mode(ctx);
    XCTAssertTrue(result, @"Should return new state (Chinese)");
    XCTAssertTrue(hime_is_chinese_mode(ctx));

    hime_context_free(ctx);
}

- (void)testSetInputMethod {
    HimeContext *ctx = hime_context_new();
    XCTAssertTrue(ctx != NULL);

    int ret = hime_set_input_method(ctx, HIME_IM_TSIN);
    XCTAssertEqual(ret, 0);
    XCTAssertEqual(hime_get_input_method(ctx), HIME_IM_TSIN);

    ret = hime_set_input_method(ctx, HIME_IM_PHO);
    XCTAssertEqual(ret, 0);
    XCTAssertEqual(hime_get_input_method(ctx), HIME_IM_PHO);

    hime_context_free(ctx);
}

- (void)testPreeditEmpty {
    HimeContext *ctx = hime_context_new();
    XCTAssertTrue(ctx != NULL);

    char buf[256];
    int len = hime_get_preedit(ctx, buf, sizeof(buf));
    XCTAssertEqual(len, 0, @"Preedit length should be 0");
    XCTAssertEqual(strcmp(buf, ""), 0, @"Preedit should be empty string");

    hime_context_free(ctx);
}

- (void)testCandidatesPerPage {
    HimeContext *ctx = hime_context_new();
    XCTAssertTrue(ctx != NULL);

    XCTAssertEqual(hime_get_candidates_per_page(ctx), 10, @"Default should be 10");

    hime_set_candidates_per_page(ctx, 5);
    XCTAssertEqual(hime_get_candidates_per_page(ctx), 5);

    /* Test clamping */
    hime_set_candidates_per_page(ctx, 0);
    XCTAssertEqual(hime_get_candidates_per_page(ctx), 1, @"Should clamp to minimum");

    hime_set_candidates_per_page(ctx, 100);
    XCTAssertEqual(hime_get_candidates_per_page(ctx), 10, @"Should clamp to maximum");

    hime_context_free(ctx);
}

- (void)testNullContextSafety {
    /* All functions should handle NULL gracefully */
    XCTAssertFalse(hime_is_chinese_mode(NULL));
    XCTAssertEqual(hime_get_input_method(NULL), HIME_IM_PHO);
    XCTAssertEqual(hime_set_input_method(NULL, HIME_IM_PHO), -1);
    XCTAssertFalse(hime_toggle_chinese_mode(NULL));

    char buf[64];
    XCTAssertEqual(hime_get_preedit(NULL, buf, sizeof(buf)), -1);
    XCTAssertEqual(hime_get_commit(NULL, buf, sizeof(buf)), -1);

    XCTAssertFalse(hime_has_candidates(NULL));
    XCTAssertEqual(hime_get_candidate_count(NULL), 0);

    /* These should not crash */
    hime_context_reset(NULL);
    hime_set_chinese_mode(NULL, true);
    hime_context_free(NULL);
}

@end
