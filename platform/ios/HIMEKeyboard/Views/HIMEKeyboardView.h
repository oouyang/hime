/*
 * HIME Keyboard View
 *
 * Custom keyboard UI for iOS with Bopomofo layout.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@class HIMEKeyboardViewController;

/**
 * HIMEKeyboardView - Keyboard UI
 *
 * Displays the Bopomofo keyboard layout with candidate bar.
 */
@interface HIMEKeyboardView : UIView

/* Reference to controller */
@property (nonatomic, weak, nullable) HIMEKeyboardViewController *controller;

/* UI update methods */
- (void)updateCandidates;
- (void)updateModeIndicator;

@end

NS_ASSUME_NONNULL_END
