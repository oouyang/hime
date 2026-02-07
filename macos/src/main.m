/*
 * HIME Input Method - macOS Application Entry Point
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#import <Cocoa/Cocoa.h>
#import <InputMethodKit/InputMethodKit.h>
#import "HimeInputController.h"

/* Global IMK server instance */
static IMKServer *gServer = nil;

/* Connection name must match Info.plist InputMethodConnectionName */
static NSString * const kConnectionName = @"HIME_Connection";

int main(int argc, const char *argv[]) {
    @autoreleasepool {
        /* Initialize the IMK server */
        gServer = [[IMKServer alloc] initWithName:kConnectionName
                                 bundleIdentifier:[[NSBundle mainBundle] bundleIdentifier]];

        if (!gServer) {
            NSLog(@"HIME: Failed to initialize IMK server");
            return 1;
        }

        /* Run the application */
        [[NSApplication sharedApplication] run];
    }

    return 0;
}
