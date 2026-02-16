/*
 * HIME Keyboard Extension View Controller
 *
 * iOS custom keyboard extension for Bopomofo/Zhuyin input.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#import <UIKit/UIKit.h>
#import "HimeEngine.h"

NS_ASSUME_NONNULL_BEGIN

@class HIMEKeyboardView;

/**
 * HIMEKeyboardViewController - iOS Keyboard Extension Controller
 *
 * Subclasses UIInputViewController to provide a custom keyboard
 * for Chinese phonetic input using the HIME engine.
 */
@interface HIMEKeyboardViewController : UIInputViewController

/* HIME engine instance */
@property (nonatomic, strong, nullable) HimeEngine *engine;

/* Keyboard view */
@property (nonatomic, strong, nullable) HIMEKeyboardView *keyboardView;

/* Insert text into the text field */
- (void)insertText:(NSString *)text;

/* Delete backward */
- (void)deleteBackward;

/* Switch to next keyboard */
- (void)advanceToNextInputMode;

@end

NS_ASSUME_NONNULL_END
