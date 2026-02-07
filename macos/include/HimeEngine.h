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

/* Character set type */
typedef NS_ENUM(NSInteger, HimeCharsetType) {
    HimeCharsetTraditional = 0,  /* Traditional Chinese */
    HimeCharsetSimplified = 1    /* Simplified Chinese */
};

/* Candidate display style */
typedef NS_ENUM(NSInteger, HimeCandidateStyleType) {
    HimeCandidateStyleHorizontal = 0,
    HimeCandidateStyleVertical = 1,
    HimeCandidateStylePopup = 2
};

/* Color scheme */
typedef NS_ENUM(NSInteger, HimeColorSchemeType) {
    HimeColorSchemeLight = 0,
    HimeColorSchemeDark = 1,
    HimeColorSchemeSystem = 2
};

/* Feedback event type */
typedef NS_ENUM(NSInteger, HimeFeedbackEventType) {
    HimeFeedbackKeyPress = 0,
    HimeFeedbackKeyDelete = 1,
    HimeFeedbackKeyEnter = 2,
    HimeFeedbackKeySpace = 3,
    HimeFeedbackCandidate = 4,
    HimeFeedbackModeChange = 5,
    HimeFeedbackError = 6
};

/* Keyboard layout type */
typedef NS_ENUM(NSInteger, HimeKeyboardLayoutType) {
    HimeKeyboardLayoutStandard = 0,  /* Standard Zhuyin (大千/標準注音) */
    HimeKeyboardLayoutHSU = 1,       /* HSU layout (許氏鍵盤) */
    HimeKeyboardLayoutETen = 2,      /* ETen layout (倚天鍵盤) */
    HimeKeyboardLayoutETen26 = 3,    /* ETen 26-key layout */
    HimeKeyboardLayoutIBM = 4,       /* IBM layout */
    HimeKeyboardLayoutPinyin = 5,    /* Hanyu Pinyin layout */
    HimeKeyboardLayoutDvorak = 6     /* Dvorak-based Zhuyin */
};

/* Feedback callback block */
typedef void (^HimeFeedbackBlock)(HimeFeedbackEventType type);

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

/* Character set (Simplified/Traditional) */
@property (nonatomic, assign) HimeCharsetType charset;
- (HimeCharsetType)toggleCharset;

/* Smart punctuation */
@property (nonatomic, assign) BOOL smartPunctuation;
- (nullable NSString *)convertPunctuation:(char)ascii;
- (void)resetPunctuationState;

/* Pinyin annotation */
@property (nonatomic, assign) BOOL pinyinAnnotation;
- (nullable NSString *)pinyinForCharacter:(NSString *)character;

/* Candidate display style */
@property (nonatomic, assign) HimeCandidateStyleType candidateStyle;

/* Color scheme */
@property (nonatomic, assign) HimeColorSchemeType colorScheme;
- (void)setSystemDarkMode:(BOOL)isDark;

/* Keyboard layout */
@property (nonatomic, assign) HimeKeyboardLayoutType keyboardLayout;
- (BOOL)setKeyboardLayoutByName:(NSString *)layoutName;

/* Sound/Vibration feedback */
@property (nonatomic, assign) BOOL soundEnabled;
@property (nonatomic, assign) BOOL vibrationEnabled;
@property (nonatomic, assign) NSInteger vibrationDuration;  /* milliseconds */
@property (nonatomic, copy, nullable) HimeFeedbackBlock feedbackHandler;

@end

NS_ASSUME_NONNULL_END
