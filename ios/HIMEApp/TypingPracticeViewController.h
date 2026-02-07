/*
 * HIME iOS Typing Practice View Controller
 *
 * Provides a typing practice interface for practicing input methods.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

/* Practice category */
typedef NS_ENUM(NSInteger, HimePracticeCategory) {
    HimePracticeCategoryEnglish = 0,
    HimePracticeCategoryZhuyin = 1,
    HimePracticeCategoryPinyin = 2,
    HimePracticeCategoryCangjie = 3,
    HimePracticeCategoryMixed = 4
};

/* Practice difficulty */
typedef NS_ENUM(NSInteger, HimePracticeDifficulty) {
    HimePracticeDifficultyEasy = 1,
    HimePracticeDifficultyMedium = 2,
    HimePracticeDifficultyHard = 3
};

/**
 * TypingPracticeViewController - Typing practice interface
 *
 * Provides a UI for practicing typing with various input methods.
 * Tracks accuracy, speed, and progress.
 */
@interface TypingPracticeViewController : UIViewController

/* Current practice settings */
@property (nonatomic, assign) HimePracticeCategory category;
@property (nonatomic, assign) HimePracticeDifficulty difficulty;

/* Statistics */
@property (nonatomic, readonly) NSInteger correctCharacters;
@property (nonatomic, readonly) NSInteger incorrectCharacters;
@property (nonatomic, readonly) double accuracy;
@property (nonatomic, readonly) double charactersPerMinute;

/* Session control */
- (void)startNewSession;
- (void)resetSession;
- (void)endSession;

@end

NS_ASSUME_NONNULL_END
