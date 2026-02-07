/*
 * HIME Input Controller Implementation
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#import "HimeInputController.h"
#import <Carbon/Carbon.h>

/* Key codes are defined in Carbon/HIToolbox/Events.h */

@implementation HimeInputController

#pragma mark - Lifecycle

- (instancetype)initWithServer:(IMKServer *)server delegate:(nullable id)delegate client:(id<IMKTextInput>)client {
    self = [super initWithServer:server delegate:delegate client:client];
    if (self) {
        [self setupEngine];
        [self setupCandidateWindow];
    }
    return self;
}

- (void)setupEngine {
    /* Find data directory in app bundle */
    NSBundle *bundle = [NSBundle bundleForClass:[self class]];
    NSString *dataPath = [bundle resourcePath];

    self.engine = [[HimeEngine alloc] initWithDataPath:dataPath];
    if (!self.engine) {
        NSLog(@"HIME: Failed to initialize engine");
    }
}

- (void)setupCandidateWindow {
    self.candidateWindow = [[IMKCandidates alloc] initWithServer:self.server
                                                       panelType:kIMKSingleColumnScrollingCandidatePanel];
    /* Selection keys: 1-9, 0 (key codes for number row) */
    NSArray *selectionKeys = @[
        @(kVK_ANSI_1),
        @(kVK_ANSI_2),
        @(kVK_ANSI_3),
        @(kVK_ANSI_4),
        @(kVK_ANSI_5),
        @(kVK_ANSI_6),
        @(kVK_ANSI_7),
        @(kVK_ANSI_8),
        @(kVK_ANSI_9),
        @(kVK_ANSI_0)
    ];
    [self.candidateWindow setSelectionKeys:selectionKeys];
    [self.candidateWindow setDismissesAutomatically:YES];
}

- (void)dealloc {
    self.engine = nil;
    self.candidateWindow = nil;
}

#pragma mark - IMKInputController Override

- (BOOL)handleEvent:(NSEvent *)event client:(id)sender {
    if (!self.engine)
        return NO;
    if (event.type != NSEventTypeKeyDown)
        return NO;

    /* Get event properties */
    NSEventModifierFlags modifierFlags = event.modifierFlags;
    unsigned short keyCode = event.keyCode;
    NSString *chars = event.characters;
    unichar character = (chars.length > 0) ? [chars characterAtIndex:0] : 0;

    /* Convert modifier flags */
    HimeModifierFlags himeModifiers = HimeModifierNone;
    if (modifierFlags & NSEventModifierFlagShift)
        himeModifiers |= HimeModifierShift;
    if (modifierFlags & NSEventModifierFlagControl)
        himeModifiers |= HimeModifierControl;
    if (modifierFlags & NSEventModifierFlagOption)
        himeModifiers |= HimeModifierAlt;
    if (modifierFlags & NSEventModifierFlagCapsLock)
        himeModifiers |= HimeModifierCapsLock;

    /* Handle Shift for mode toggle */
    if (keyCode == kVK_Shift || keyCode == kVK_RightShift) {
        if (!(himeModifiers & (HimeModifierControl | HimeModifierAlt))) {
            [self.engine toggleChineseMode];
            [self hideCandidates];
            return YES;
        }
    }

    /* Pass through if not in Chinese mode and no composition */
    if (!self.engine.chineseMode && self.engine.preeditString.length == 0) {
        return NO;
    }

    /* Handle special keys */
    switch (keyCode) {
        case kVK_Escape:
            if (self.engine.preeditString.length > 0 || self.engine.hasCandidates) {
                [self.engine reset];
                [self updateComposition:sender];
                [self hideCandidates];
                return YES;
            }
            return NO;

        case kVK_Return:
            if (self.engine.preeditString.length > 0) {
                /* Commit preedit as-is */
                [self commitString:self.engine.preeditString client:sender];
                [self.engine reset];
                [self hideCandidates];
                return YES;
            }
            return NO;

        case kVK_Delete:
            /* Backspace handling is done in engine */
            break;

        case kVK_PageUp:
            if (self.engine.hasCandidates) {
                [self.engine pageUp];
                [self updateCandidates];
                return YES;
            }
            return NO;

        case kVK_PageDown:
            if (self.engine.hasCandidates) {
                [self.engine pageDown];
                [self updateCandidates];
                return YES;
            }
            return NO;

        case kVK_UpArrow:
        case kVK_DownArrow:
        case kVK_LeftArrow:
        case kVK_RightArrow:
            /* Let candidate window handle arrow keys if visible */
            if (self.engine.hasCandidates) {
                return NO; /* IMKCandidates handles this */
            }
            return NO;
    }

    /* Process key through HIME engine */
    HimeKeyResultType result = [self.engine processKeyCode:keyCode character:character modifiers:himeModifiers];

    switch (result) {
        case HimeKeyResultCommit: {
            NSString *commit = self.engine.commitString;
            if (commit.length > 0) {
                [self commitString:commit client:sender];
                [self.engine clearCommit];
            }
            [self updateComposition:sender];
            [self hideCandidates];
            return YES;
        }

        case HimeKeyResultPreedit:
            [self updateComposition:sender];
            if (self.engine.hasCandidates) {
                [self showCandidates:sender];
            } else {
                [self hideCandidates];
            }
            return YES;

        case HimeKeyResultAbsorbed:
            [self updateComposition:sender];
            [self hideCandidates];
            return YES;

        case HimeKeyResultIgnored:
        default:
            return NO;
    }
}

#pragma mark - Composition Management

- (void)updateComposition:(id)client {
    NSString *preedit = self.engine.preeditString;

    if (preedit.length > 0) {
        /* Set marked text (composition string) */
        NSDictionary *attrs = [self markForStyle:kTSMHiliteSelectedConvertedText
                                         atRange:NSMakeRange(0, preedit.length)];
        NSAttributedString *attrString = [[NSAttributedString alloc] initWithString:preedit attributes:attrs];

        [client setMarkedText:attrString
               selectionRange:NSMakeRange(preedit.length, 0)
             replacementRange:NSMakeRange(NSNotFound, NSNotFound)];
    } else {
        /* Clear marked text */
        [client setMarkedText:@""
               selectionRange:NSMakeRange(0, 0)
             replacementRange:NSMakeRange(NSNotFound, NSNotFound)];
    }
}

- (void)commitString:(NSString *)string client:(id)client {
    [client insertText:string replacementRange:NSMakeRange(NSNotFound, NSNotFound)];
}

#pragma mark - Candidate Management

- (NSArray *)candidates:(id)sender {
    if (!self.engine.hasCandidates) {
        return @[];
    }

    return [self.engine candidatesForCurrentPage];
}

- (void)candidateSelected:(NSAttributedString *)candidateString {
    NSString *text = candidateString.string;

    /* Extract character before any annotation */
    NSRange spaceRange = [text rangeOfString:@" "];
    if (spaceRange.location != NSNotFound) {
        text = [text substringToIndex:spaceRange.location];
    }

    /* Commit selected candidate */
    id client = [self client];
    [self commitString:text client:client];
    [self.engine reset];
    [self hideCandidates];
}

- (void)showCandidates:(id)sender {
    if (self.engine.hasCandidates) {
        [self.candidateWindow updateCandidates];
        [self.candidateWindow show:kIMKLocateCandidatesBelowHint];
    }
}

- (void)hideCandidates {
    [self.candidateWindow hide];
}

- (void)updateCandidates {
    [self.candidateWindow updateCandidates];
}

#pragma mark - Session Management

- (void)activateServer:(id)sender {
    [super activateServer:sender];
    /* Reset state when activated */
    [self.engine reset];
}

- (void)deactivateServer:(id)sender {
    /* Commit any pending text */
    if (self.engine.preeditString.length > 0) {
        [self commitString:self.engine.preeditString client:sender];
        [self.engine reset];
    }
    [self hideCandidates];
    [super deactivateServer:sender];
}

@end
