/*
 * HIME macOS Typing Practice Window Implementation
 *
 * AppKit-based typing practice interface.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#import "HIMETypingPracticeWindow.h"
#import "../../shared/include/hime-core.h"

@interface HIMETypingPracticeWindowController () <NSTextFieldDelegate>

/* UI Components */
@property (nonatomic, strong) NSPopUpButton *categoryPopup;
@property (nonatomic, strong) NSPopUpButton *difficultyPopup;
@property (nonatomic, strong) NSTextField *practiceTextLabel;
@property (nonatomic, strong) NSTextField *hintLabel;
@property (nonatomic, strong) NSProgressIndicator *progressIndicator;
@property (nonatomic, strong) NSTextField *inputField;
@property (nonatomic, strong) NSTextField *statsLabel;
@property (nonatomic, strong) NSButton *newTextButton;
@property (nonatomic, strong) NSButton *resetButton;

/* State */
@property (nonatomic, copy) NSString *currentPracticeText;
@property (nonatomic, copy) NSString *currentHint;
@property (nonatomic, assign) NSInteger currentPosition;
@property (nonatomic, assign) NSInteger totalCharacters;
@property (nonatomic, assign) NSInteger correctCount;
@property (nonatomic, assign) NSInteger incorrectCount;
@property (nonatomic, strong) NSDate *sessionStartTime;
@property (nonatomic, assign) BOOL sessionActive;

/* HIME Context */
@property (nonatomic, assign) HimeContext *ctx;

@end

@implementation HIMETypingPracticeWindowController

static HIMETypingPracticeWindowController *sharedInstance = nil;

+ (instancetype)sharedController {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedInstance = [[self alloc] init];
    });
    return sharedInstance;
}

- (instancetype)init {
    NSRect frame = NSMakeRect(0, 0, 500, 450);
    NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
    NSWindow *window = [[NSWindow alloc] initWithContentRect:frame
                                                   styleMask:style
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
    window.title = @"Typing Practice / 打字練習";

    self = [super initWithWindow:window];
    if (self) {
        /* Initialize HIME */
        hime_init(NULL);
        self.ctx = hime_context_new();

        /* Default settings */
        self.category = HimeMacPracticeCategoryEnglish;
        self.difficulty = HimeMacPracticeDifficultyEasy;

        [self setupUI];
        [self loadRandomPracticeText];
    }
    return self;
}

- (void)dealloc {
    if (self.ctx) {
        hime_typing_end_session(self.ctx);
        hime_context_free(self.ctx);
    }
}

- (void)setupUI {
    NSView *contentView = self.window.contentView;
    CGFloat y = 400;
    CGFloat margin = 20;
    CGFloat width = 460;

    /* Title */
    NSTextField *titleLabel = [NSTextField labelWithString:@"Typing Practice / 打字練習"];
    titleLabel.font = [NSFont boldSystemFontOfSize:20];
    titleLabel.frame = NSMakeRect(margin, y, width, 30);
    [contentView addSubview:titleLabel];
    y -= 50;

    /* Category */
    NSTextField *categoryLabel = [NSTextField labelWithString:@"Category / 類別:"];
    categoryLabel.frame = NSMakeRect(margin, y, 150, 20);
    [contentView addSubview:categoryLabel];
    y -= 30;

    self.categoryPopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(margin, y, width, 25)];
    [self.categoryPopup
        addItemsWithTitles:@[ @"English", @"注音 (Zhuyin)", @"拼音 (Pinyin)", @"倉頡 (Cangjie)", @"Mixed / 混合" ]];
    [self.categoryPopup setTarget:self];
    [self.categoryPopup setAction:@selector(categoryChanged:)];
    [contentView addSubview:self.categoryPopup];
    y -= 35;

    /* Difficulty */
    NSTextField *difficultyLabel = [NSTextField labelWithString:@"Difficulty / 難度:"];
    difficultyLabel.frame = NSMakeRect(margin, y, 150, 20);
    [contentView addSubview:difficultyLabel];
    y -= 30;

    self.difficultyPopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(margin, y, width, 25)];
    [self.difficultyPopup addItemsWithTitles:@[ @"Easy / 簡單", @"Medium / 中等", @"Hard / 困難" ]];
    [self.difficultyPopup setTarget:self];
    [self.difficultyPopup setAction:@selector(difficultyChanged:)];
    [contentView addSubview:self.difficultyPopup];
    y -= 45;

    /* Practice text label */
    NSTextField *practiceLabel = [NSTextField labelWithString:@"Type this / 請輸入:"];
    practiceLabel.frame = NSMakeRect(margin, y, 150, 20);
    [contentView addSubview:practiceLabel];
    y -= 70;

    /* Practice text display */
    self.practiceTextLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(margin, y, width, 60)];
    self.practiceTextLabel.editable = NO;
    self.practiceTextLabel.selectable = NO;
    self.practiceTextLabel.bordered = YES;
    self.practiceTextLabel.font = [NSFont systemFontOfSize:20];
    self.practiceTextLabel.alignment = NSTextAlignmentCenter;
    self.practiceTextLabel.backgroundColor = [NSColor controlBackgroundColor];
    [contentView addSubview:self.practiceTextLabel];
    y -= 25;

    /* Hint label */
    self.hintLabel = [NSTextField labelWithString:@""];
    self.hintLabel.frame = NSMakeRect(margin, y, width, 20);
    self.hintLabel.alignment = NSTextAlignmentCenter;
    self.hintLabel.textColor = [NSColor secondaryLabelColor];
    [contentView addSubview:self.hintLabel];
    y -= 30;

    /* Progress indicator */
    self.progressIndicator = [[NSProgressIndicator alloc] initWithFrame:NSMakeRect(margin, y, width, 10)];
    self.progressIndicator.indeterminate = NO;
    self.progressIndicator.minValue = 0;
    self.progressIndicator.maxValue = 100;
    [contentView addSubview:self.progressIndicator];
    y -= 40;

    /* Input field */
    self.inputField = [[NSTextField alloc] initWithFrame:NSMakeRect(margin, y, width, 30)];
    self.inputField.placeholderString = @"Start typing here... / 在這裡輸入...";
    self.inputField.font = [NSFont systemFontOfSize:18];
    self.inputField.delegate = self;
    [contentView addSubview:self.inputField];
    y -= 55;

    /* Stats label */
    self.statsLabel = [NSTextField labelWithString:@"Progress: 0/0 | Accuracy: 100%"];
    self.statsLabel.frame = NSMakeRect(margin, y, width, 40);
    self.statsLabel.alignment = NSTextAlignmentCenter;
    self.statsLabel.font = [NSFont monospacedSystemFontOfSize:12 weight:NSFontWeightRegular];
    [contentView addSubview:self.statsLabel];
    y -= 50;

    /* Buttons */
    self.newTextButton = [[NSButton alloc] initWithFrame:NSMakeRect(margin, y, 220, 35)];
    self.newTextButton.title = @"New Text / 換題";
    self.newTextButton.bezelStyle = NSBezelStyleRounded;
    [self.newTextButton setTarget:self];
    [self.newTextButton setAction:@selector(newTextClicked:)];
    [contentView addSubview:self.newTextButton];

    self.resetButton = [[NSButton alloc] initWithFrame:NSMakeRect(260, y, 220, 35)];
    self.resetButton.title = @"Reset / 重置";
    self.resetButton.bezelStyle = NSBezelStyleRounded;
    [self.resetButton setTarget:self];
    [self.resetButton setAction:@selector(resetClicked:)];
    [contentView addSubview:self.resetButton];
}

#pragma mark - Practice Text Loading

/* Built-in practice texts */
static NSString *PRACTICE_TEXTS[] = {
    /* English */
    @"The quick brown fox jumps over the lazy dog.",
    @"Pack my box with five dozen liquor jugs.",
    @"How vexingly quick daft zebras jump!",

    /* Traditional Chinese */
    @"你好嗎？",
    @"謝謝你。",
    @"今天天氣很好。",

    /* Simplified Chinese */
    @"你好吗？",
    @"谢谢你。",

    /* Mixed */
    @"Hello 你好 World 世界",
    nil};

static NSString *PRACTICE_HINTS[] = {@"",
                                     @"",
                                     @"",
                                     @"ni3 hao3 ma",
                                     @"xie4 xie4 ni3",
                                     @"jin1 tian1 tian1 qi4 hen3 hao3",
                                     @"ni hao ma",
                                     @"xie xie ni",
                                     @"",
                                     nil};

- (void)loadRandomPracticeText {
    NSInteger startIdx = 0;
    NSInteger endIdx = 9;

    switch (self.category) {
        case HimeMacPracticeCategoryEnglish:
            startIdx = 0;
            endIdx = 3;
            break;
        case HimeMacPracticeCategoryZhuyin:
        case HimeMacPracticeCategoryCangjie:
            startIdx = 3;
            endIdx = 6;
            break;
        case HimeMacPracticeCategoryPinyin:
            startIdx = 6;
            endIdx = 8;
            break;
        case HimeMacPracticeCategoryMixed:
            startIdx = 8;
            endIdx = 9;
            break;
    }

    NSInteger idx = startIdx + (arc4random() % (endIdx - startIdx));
    if (idx >= 9)
        idx = 0;

    self.currentPracticeText = PRACTICE_TEXTS[idx];
    self.currentHint = PRACTICE_HINTS[idx];
    self.totalCharacters = self.currentPracticeText.length;

    [self startNewSession];
}

#pragma mark - Session Control

- (void)startNewSession {
    if (!self.ctx)
        return;

    hime_typing_start_session(self.ctx, [self.currentPracticeText UTF8String]);

    self.currentPosition = 0;
    self.correctCount = 0;
    self.incorrectCount = 0;
    self.sessionStartTime = [NSDate date];
    self.sessionActive = YES;

    self.inputField.stringValue = @"";
    self.progressIndicator.doubleValue = 0;

    [self updatePracticeTextDisplay];
    [self updateStats];
}

- (void)resetSession {
    if (!self.ctx || !self.sessionActive)
        return;

    hime_typing_reset_session(self.ctx);

    self.currentPosition = 0;
    self.correctCount = 0;
    self.incorrectCount = 0;
    self.sessionStartTime = [NSDate date];

    self.inputField.stringValue = @"";
    self.progressIndicator.doubleValue = 0;

    [self updatePracticeTextDisplay];
    [self updateStats];
}

#pragma mark - Text Processing

- (void)controlTextDidChange:(NSNotification *)notification {
    if (!self.sessionActive || !self.ctx)
        return;

    NSString *text = self.inputField.stringValue;
    if (text.length == 0)
        return;

    /* Get the last character entered */
    NSString *lastChar = [text substringFromIndex:text.length - 1];

    /* Submit to HIME */
    int result = hime_typing_submit_char(self.ctx, [lastChar UTF8String]);
    hime_typing_record_keystroke(self.ctx);

    if (result > 0) {
        self.correctCount++;
    } else if (result == 0) {
        self.incorrectCount++;
    }

    self.currentPosition++;
    [self updatePracticeTextDisplay];
    [self updateStats];

    /* Check completion */
    HimeTypingStats stats;
    hime_typing_get_stats(self.ctx, &stats);
    if (stats.completed) {
        [self showCompletionAlert];
    }
}

- (void)updatePracticeTextDisplay {
    if (!self.currentPracticeText)
        return;

    NSMutableAttributedString *attrStr = [[NSMutableAttributedString alloc] initWithString:self.currentPracticeText];

    /* Highlight typed portion in green */
    if (self.currentPosition > 0 && self.currentPosition <= self.currentPracticeText.length) {
        [attrStr addAttribute:NSForegroundColorAttributeName
                        value:[NSColor systemGreenColor]
                        range:NSMakeRange(0, MIN(self.currentPosition, self.currentPracticeText.length))];
    }

    /* Highlight current character */
    if (self.currentPosition < self.currentPracticeText.length) {
        [attrStr addAttribute:NSBackgroundColorAttributeName
                        value:[NSColor systemYellowColor]
                        range:NSMakeRange(self.currentPosition, 1)];
    }

    self.practiceTextLabel.attributedStringValue = attrStr;

    if (self.currentHint.length > 0) {
        self.hintLabel.stringValue = [NSString stringWithFormat:@"Hint: %@", self.currentHint];
    } else {
        self.hintLabel.stringValue = @"";
    }

    /* Update progress */
    if (self.totalCharacters > 0) {
        self.progressIndicator.doubleValue = (double) self.currentPosition / self.totalCharacters * 100;
    }
}

- (void)updateStats {
    HimeTypingStats stats;
    if (hime_typing_get_stats(self.ctx, &stats) != 0) {
        self.statsLabel.stringValue = @"";
        return;
    }

    self.statsLabel.stringValue = [NSString stringWithFormat:@"Progress: %d/%d | Correct: %d | Incorrect: %d\n"
                                                             @"Accuracy: %.1f%% | Speed: %.1f chars/min",
                                                             stats.typed_characters,
                                                             stats.total_characters,
                                                             stats.correct_characters,
                                                             stats.incorrect_characters,
                                                             stats.accuracy,
                                                             stats.chars_per_minute];

    _correctCharacters = stats.correct_characters;
    _incorrectCharacters = stats.incorrect_characters;
    _accuracy = stats.accuracy;
    _charactersPerMinute = stats.chars_per_minute;
}

- (void)showCompletionAlert {
    HimeTypingStats stats;
    hime_typing_get_stats(self.ctx, &stats);

    NSString *message = [NSString stringWithFormat:@"Accuracy: %.1f%%\nSpeed: %.1f chars/min\nTime: %.1f seconds",
                                                   stats.accuracy,
                                                   stats.chars_per_minute,
                                                   (double) stats.elapsed_time_ms / 1000.0];

    NSAlert *alert = [[NSAlert alloc] init];
    alert.messageText = @"Completed! / 完成!";
    alert.informativeText = message;
    [alert addButtonWithTitle:@"New Text / 換題"];
    [alert addButtonWithTitle:@"OK"];

    [alert beginSheetModalForWindow:self.window
                  completionHandler:^(NSModalResponse returnCode) {
                      if (returnCode == NSAlertFirstButtonReturn) {
                          [self loadRandomPracticeText];
                      }
                  }];
}

#pragma mark - Actions

- (void)categoryChanged:(id)sender {
    self.category = (HimeMacPracticeCategory) self.categoryPopup.indexOfSelectedItem;
    [self loadRandomPracticeText];
}

- (void)difficultyChanged:(id)sender {
    self.difficulty = (HimeMacPracticeDifficulty) (self.difficultyPopup.indexOfSelectedItem + 1);
    [self loadRandomPracticeText];
}

- (void)newTextClicked:(id)sender {
    [self loadRandomPracticeText];
}

- (void)resetClicked:(id)sender {
    [self resetSession];
}

- (void)showWindow:(id)sender {
    [self.window center];
    [super showWindow:sender];
    [self.window makeKeyAndOrderFront:sender];
}

@end
