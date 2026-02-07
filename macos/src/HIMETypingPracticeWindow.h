/*
 * HIME macOS Typing Practice Window
 *
 * AppKit-based typing practice interface.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

/* Practice category */
typedef NS_ENUM(NSInteger, HimeMacPracticeCategory) {
    HimeMacPracticeCategoryEnglish = 0,
    HimeMacPracticeCategoryZhuyin = 1,
    HimeMacPracticeCategoryPinyin = 2,
    HimeMacPracticeCategoryCangjie = 3,
    HimeMacPracticeCategoryMixed = 4
};

/* Practice difficulty */
typedef NS_ENUM(NSInteger, HimeMacPracticeDifficulty) {
    HimeMacPracticeDifficultyEasy = 1,
    HimeMacPracticeDifficultyMedium = 2,
    HimeMacPracticeDifficultyHard = 3
};

/**
 * HIMETypingPracticeWindowController - Typing practice window controller
 */
@interface HIMETypingPracticeWindowController : NSWindowController

/* Show the typing practice window */
+ (instancetype)sharedController;
- (void)showWindow:(nullable id)sender;

/* Settings */
@property (nonatomic, assign) HimeMacPracticeCategory category;
@property (nonatomic, assign) HimeMacPracticeDifficulty difficulty;

/* Statistics */
@property (nonatomic, readonly) NSInteger correctCharacters;
@property (nonatomic, readonly) NSInteger incorrectCharacters;
@property (nonatomic, readonly) double accuracy;
@property (nonatomic, readonly) double charactersPerMinute;

/* Session control */
- (void)startNewSession;
- (void)resetSession;

@end

NS_ASSUME_NONNULL_END
