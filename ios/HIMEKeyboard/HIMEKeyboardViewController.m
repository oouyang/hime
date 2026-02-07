/*
 * HIME Keyboard Extension View Controller Implementation
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#import "HIMEKeyboardViewController.h"
#import "HIMEKeyboardView.h"

@interface HIMEKeyboardViewController ()
@end

@implementation HIMEKeyboardViewController

#pragma mark - Lifecycle

- (void)viewDidLoad {
    [super viewDidLoad];

    [self setupEngine];
    [self setupKeyboardView];
}

- (void)setupEngine {
    /* Find data directory in extension bundle */
    NSBundle *bundle = [NSBundle bundleForClass:[self class]];
    NSString *dataPath = [bundle resourcePath];

    self.engine = [[HimeEngine alloc] initWithDataPath:dataPath];
    if (!self.engine) {
        NSLog(@"HIME: Failed to initialize engine");
    }
}

- (void)setupKeyboardView {
    /* Create keyboard view */
    self.keyboardView = [[HIMEKeyboardView alloc] initWithFrame:CGRectZero];
    self.keyboardView.translatesAutoresizingMaskIntoConstraints = NO;
    self.keyboardView.controller = self;

    [self.view addSubview:self.keyboardView];

    /* Set up constraints */
    [NSLayoutConstraint activateConstraints:@[
        [self.keyboardView.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],
        [self.keyboardView.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor],
        [self.keyboardView.topAnchor constraintEqualToAnchor:self.view.topAnchor],
        [self.keyboardView.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor],
    ]];

    /* Set keyboard height */
    NSLayoutConstraint *heightConstraint = [self.keyboardView.heightAnchor
                                            constraintEqualToConstant:260];
    heightConstraint.priority = UILayoutPriorityRequired - 1;
    heightConstraint.active = YES;
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    [self.keyboardView updateCandidates];
}

#pragma mark - UIInputViewController Override

- (void)textWillChange:(id<UITextInput>)textInput {
    /* Called when text is about to change */
}

- (void)textDidChange:(id<UITextInput>)textInput {
    /* Called when text has changed */
    /* Update UI based on text traits if needed */
    UITextAutocapitalizationType autocapType = self.textDocumentProxy.autocapitalizationType;
    (void)autocapType;  /* Can be used for shift state */
}

#pragma mark - Text Input

- (void)insertText:(NSString *)text {
    [self.textDocumentProxy insertText:text];
}

- (void)deleteBackward {
    [self.textDocumentProxy deleteBackward];
}

- (void)advanceToNextInputMode {
    [super advanceToNextInputMode];
}

#pragma mark - Key Handling

- (void)handleKeyPress:(NSString *)key {
    if (!self.engine) return;

    /* Handle special keys */
    if ([key isEqualToString:@"⌫"]) {
        /* Backspace */
        if (self.engine.preeditString.length > 0) {
            HimeKeyResultType result = [self.engine processKeyCode:0x08
                                                         character:0x08
                                                         modifiers:HimeModifierNone];
            [self handleEngineResult:result];
        } else {
            [self deleteBackward];
        }
        return;
    }

    if ([key isEqualToString:@"⏎"]) {
        /* Return/Enter */
        if (self.engine.preeditString.length > 0) {
            [self insertText:self.engine.preeditString];
            [self.engine reset];
            [self.keyboardView updateCandidates];
        } else {
            [self insertText:@"\n"];
        }
        return;
    }

    if ([key isEqualToString:@"␣"]) {
        /* Space - tone 1 or confirm */
        if (self.engine.chineseMode && self.engine.preeditString.length > 0) {
            HimeKeyResultType result = [self.engine processKeyCode:0x20
                                                         character:' '
                                                         modifiers:HimeModifierNone];
            [self handleEngineResult:result];
        } else {
            [self insertText:@" "];
        }
        return;
    }

    if ([key isEqualToString:@"🌐"]) {
        /* Globe - switch keyboard */
        [self advanceToNextInputMode];
        return;
    }

    if ([key isEqualToString:@"中/英"]) {
        /* Toggle mode */
        [self.engine toggleChineseMode];
        [self.keyboardView updateModeIndicator];
        return;
    }

    /* Regular character input */
    if (key.length == 1) {
        unichar ch = [key characterAtIndex:0];

        if (self.engine.chineseMode) {
            HimeKeyResultType result = [self.engine processKeyCode:ch
                                                         character:ch
                                                         modifiers:HimeModifierNone];
            [self handleEngineResult:result];
        } else {
            /* English mode - insert directly */
            [self insertText:key];
        }
    }
}

- (void)handleEngineResult:(HimeKeyResultType)result {
    switch (result) {
        case HimeKeyResultCommit: {
            NSString *commit = self.engine.commitString;
            if (commit.length > 0) {
                [self insertText:commit];
                [self.engine clearCommit];
            }
            [self.keyboardView updateCandidates];
            break;
        }

        case HimeKeyResultPreedit:
        case HimeKeyResultAbsorbed:
            [self.keyboardView updateCandidates];
            break;

        case HimeKeyResultIgnored:
        default:
            break;
    }
}

- (void)selectCandidate:(NSInteger)index {
    if (!self.engine) return;

    HimeKeyResultType result = [self.engine selectCandidateAtIndex:index];
    [self handleEngineResult:result];
}

- (void)pageUp {
    if (self.engine && [self.engine pageUp]) {
        [self.keyboardView updateCandidates];
    }
}

- (void)pageDown {
    if (self.engine && [self.engine pageDown]) {
        [self.keyboardView updateCandidates];
    }
}

@end
