/*
 * HIME Engine Objective-C Wrapper
 *
 * Wraps the hime-core C library for use with macOS Input Method Kit.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/* Key processing result */
typedef NS_ENUM(NSInteger, HimeKeyResultType) {
    HimeKeyResultIgnored = 0,   /* Key not handled, pass through */
    HimeKeyResultAbsorbed = 1,  /* Key handled, no output */
    HimeKeyResultCommit = 2,    /* Key handled, commit string ready */
    HimeKeyResultPreedit = 3    /* Key handled, preedit updated */
};

/* Modifier flags */
typedef NS_OPTIONS(NSUInteger, HimeModifierFlags) {
    HimeModifierNone = 0,
    HimeModifierShift = 1 << 0,
    HimeModifierControl = 1 << 1,
    HimeModifierAlt = 1 << 2,
    HimeModifierCapsLock = 1 << 3
};

/**
 * HimeEngine - Objective-C wrapper for HIME core input engine
 *
 * This class provides an Objective-C interface to the HIME phonetic
 * input engine for use with macOS Input Method Kit.
 */
@interface HimeEngine : NSObject

/* Initialization */
- (nullable instancetype)initWithDataPath:(NSString *)dataPath;

/* Mode control */
@property (nonatomic, assign) BOOL chineseMode;
- (void)toggleChineseMode;

/* Key processing */
- (HimeKeyResultType)processKeyCode:(unsigned short)keyCode
                          character:(unichar)character
                          modifiers:(HimeModifierFlags)modifiers;

/* Preedit string (composition) */
@property (nonatomic, readonly, copy) NSString *preeditString;
@property (nonatomic, readonly) NSUInteger preeditCursor;

/* Commit string */
@property (nonatomic, readonly, copy) NSString *commitString;
- (void)clearCommit;

/* Candidates */
@property (nonatomic, readonly) BOOL hasCandidates;
@property (nonatomic, readonly) NSInteger candidateCount;
@property (nonatomic, readonly) NSInteger currentPage;
@property (nonatomic, readonly) NSInteger candidatesPerPage;
- (nullable NSString *)candidateAtIndex:(NSInteger)index;
- (NSArray<NSString *> *)candidatesForCurrentPage;
- (HimeKeyResultType)selectCandidateAtIndex:(NSInteger)index;
- (BOOL)pageUp;
- (BOOL)pageDown;

/* Reset state */
- (void)reset;

@end

NS_ASSUME_NONNULL_END
