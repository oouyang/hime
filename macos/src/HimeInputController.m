/*
 * HIME Input Controller Implementation
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#import "HimeInputController.h"
#import <Carbon/Carbon.h>

/* macOS virtual key codes */
enum {
    kVK_Return = 0x24,
    kVK_Tab = 0x30,
    kVK_Space = 0x31,
    kVK_Delete = 0x33,
    kVK_Escape = 0x35,
    kVK_Shift = 0x38,
    kVK_CapsLock = 0x39,
    kVK_Control = 0x3B,
    kVK_RightShift = 0x3C,
    kVK_Option = 0x3A,
    kVK_LeftArrow = 0x7B,
    kVK_RightArrow = 0x7C,
    kVK_DownArrow = 0x7D,
    kVK_UpArrow = 0x7E,
    kVK_PageUp = 0x74,
    kVK_PageDown = 0x79,
    kVK_Home = 0x73,
    kVK_End = 0x77,
};

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
    [self.candidateWindow setSelectionKeys:@"1234567890"];
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
