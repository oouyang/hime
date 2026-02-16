/*
 * HIME Input Method - macOS Application Entry Point
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <InputMethodKit/InputMethodKit.h>
#import "HimeInputController.h"

/* Global IMK server instance */
static IMKServer *gServer = nil;

/**
 * Register this input method with the system if not already registered.
 * This helps ensure the input method appears in System Preferences.
 */
static void registerInputSource(void) {
    NSURL *bundleURL = [[NSBundle mainBundle] bundleURL];
    NSLog(@"HIME: Attempting to register input source at: %@", bundleURL);

    /* Check if already registered */
    CFArrayRef inputSources = TISCreateInputSourceList(NULL, true);
    if (inputSources) {
        NSString *bundleId = [[NSBundle mainBundle] bundleIdentifier];
        BOOL found = NO;

        for (CFIndex i = 0; i < CFArrayGetCount(inputSources); i++) {
            TISInputSourceRef source = (TISInputSourceRef) CFArrayGetValueAtIndex(inputSources, i);
            NSString *sourceId = (__bridge NSString *) TISGetInputSourceProperty(source, kTISPropertyInputSourceID);
            if ([sourceId hasPrefix:bundleId]) {
                found = YES;
                NSLog(@"HIME: Input source already registered: %@", sourceId);
            }
        }
        CFRelease(inputSources);

        if (!found) {
            /* Register the input source */
            OSStatus status = TISRegisterInputSource((__bridge CFURLRef) bundleURL);
            if (status == noErr) {
                NSLog(@"HIME: Successfully registered input source");
            } else {
                NSLog(@"HIME: Failed to register input source, status=%d", (int) status);
            }
        }
    }
}

/* Connection name must match Info.plist InputMethodConnectionName
 * The format is: BundleIdentifier_Connection
 * This is derived from the bundle identifier at runtime to ensure consistency
 */
static NSString *kConnectionName = nil;

int main(int argc, const char *argv[]) {
    @autoreleasepool {
        NSLog(@"HIME: main() starting");

        NSString *bundleId = [[NSBundle mainBundle] bundleIdentifier];
        NSLog(@"HIME: Bundle identifier: %@", bundleId);
        NSLog(@"HIME: Bundle path: %@", [[NSBundle mainBundle] bundlePath]);

        /* Derive connection name from bundle identifier */
        kConnectionName = [NSString stringWithFormat:@"%@_Connection", bundleId];
        NSLog(@"HIME: Connection name: %@", kConnectionName);

        /* Initialize NSApplication first */
        [NSApplication sharedApplication];
        NSLog(@"HIME: NSApplication initialized");

        /* Initialize the IMK server */
        NSLog(@"HIME: Creating IMKServer with connection=%@ bundleId=%@", kConnectionName, bundleId);

        gServer = [[IMKServer alloc] initWithName:kConnectionName bundleIdentifier:bundleId];

        if (!gServer) {
            NSLog(@"HIME: Failed to initialize IMK server");
            return 1;
        }
        NSLog(@"HIME: IMKServer created successfully: %@", gServer);

        /* Log the controller class */
        Class controllerClass = NSClassFromString(@"HimeInputController");
        NSLog(@"HIME: Controller class found: %@", controllerClass ? @"YES" : @"NO");

        /* Register input source if needed */
        registerInputSource();

        /* Run the application */
        NSLog(@"HIME: Starting run loop");
        [[NSApplication sharedApplication] run];
    }

    return 0;
}
