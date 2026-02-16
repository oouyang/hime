/*
 * HIME Input Controller for macOS Input Method Kit
 *
 * This class handles keyboard events from macOS and interfaces
 * with the HIME engine for Bopomofo/Zhuyin input.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#import <Cocoa/Cocoa.h>
#import <InputMethodKit/InputMethodKit.h>
#import "HimeEngine.h"

NS_ASSUME_NONNULL_BEGIN

/**
 * HimeInputController - macOS Input Method Kit controller
 *
 * Subclasses IMKInputController to handle keyboard events and
 * provide Chinese phonetic input via HIME engine.
 */
@interface HimeInputController : IMKInputController

/* HIME engine instance */
@property (nonatomic, strong, nullable) HimeEngine *engine;

/* Candidate window */
@property (nonatomic, strong, nullable) IMKCandidates *candidateWindow;

@end

NS_ASSUME_NONNULL_END
