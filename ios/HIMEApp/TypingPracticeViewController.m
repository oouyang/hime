/*
 * HIME iOS Typing Practice View Controller Implementation
 *
 * Provides a typing practice interface for practicing input methods.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#import "TypingPracticeViewController.h"
#import "../../Shared/include/hime-core.h"

@interface TypingPracticeViewController () <UITextFieldDelegate>

/* UI Components */
@property (nonatomic, strong) UILabel *titleLabel;
@property (nonatomic, strong) UISegmentedControl *categorySelector;
@property (nonatomic, strong) UISegmentedControl *difficultySelector;
@property (nonatomic, strong) UILabel *practiceTextLabel;
@property (nonatomic, strong) UILabel *hintLabel;
@property (nonatomic, strong) UITextField *inputField;
@property (nonatomic, strong) UILabel *statsLabel;
@property (nonatomic, strong) UIProgressView *progressView;
@property (nonatomic, strong) UIButton *newTextButton;
@property (nonatomic, strong) UIButton *resetButton;

/* State */
@property (nonatomic, strong) NSString *currentPracticeText;
@property (nonatomic, strong) NSString *currentHint;
@property (nonatomic, assign) NSInteger currentPosition;
@property (nonatomic, assign) NSInteger totalCharacters;
@property (nonatomic, assign) NSInteger correctCount;
@property (nonatomic, assign) NSInteger incorrectCount;
@property (nonatomic, strong) NSDate *sessionStartTime;
@property (nonatomic, assign) BOOL sessionActive;

/* HIME Context */
@property (nonatomic, assign) HimeContext *ctx;

@end

@implementation TypingPracticeViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    self.view.backgroundColor = [UIColor systemBackgroundColor];
    self.title = @"Typing Practice";

    /* Initialize HIME */
    NSString *dataPath = [[NSBundle mainBundle] pathForResource:@"data" ofType:nil];
    if (dataPath) {
        hime_init([dataPath UTF8String]);
    } else {
        hime_init(NULL);
    }
    self.ctx = hime_context_new();

    /* Default settings */
    self.category = HimePracticeCategoryEnglish;
    self.difficulty = HimePracticeDifficultyEasy;

    [self setupUI];
    [self loadRandomPracticeText];
}

- (void)dealloc {
    if (self.ctx) {
        hime_typing_end_session(self.ctx);
        hime_context_free(self.ctx);
    }
}

- (void)setupUI {
    UIScrollView *scrollView = [[UIScrollView alloc] init];
    scrollView.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:scrollView];

    UIView *contentView = [[UIView alloc] init];
    contentView.translatesAutoresizingMaskIntoConstraints = NO;
    [scrollView addSubview:contentView];

    /* Title */
    self.titleLabel = [[UILabel alloc] init];
    self.titleLabel.translatesAutoresizingMaskIntoConstraints = NO;
    self.titleLabel.text = @"Typing Practice / 打字練習";
    self.titleLabel.font = [UIFont boldSystemFontOfSize:24];
    self.titleLabel.textAlignment = NSTextAlignmentCenter;
    [contentView addSubview:self.titleLabel];

    /* Category selector */
    UILabel *categoryLabel = [[UILabel alloc] init];
    categoryLabel.translatesAutoresizingMaskIntoConstraints = NO;
    categoryLabel.text = @"Category / 類別:";
    categoryLabel.font = [UIFont systemFontOfSize:14];
    categoryLabel.textColor = [UIColor secondaryLabelColor];
    [contentView addSubview:categoryLabel];

    self.categorySelector =
        [[UISegmentedControl alloc] initWithItems:@[ @"English", @"注音", @"拼音", @"倉頡", @"Mixed" ]];
    self.categorySelector.translatesAutoresizingMaskIntoConstraints = NO;
    self.categorySelector.selectedSegmentIndex = 0;
    [self.categorySelector addTarget:self
                              action:@selector(categoryChanged)
                    forControlEvents:UIControlEventValueChanged];
    [contentView addSubview:self.categorySelector];

    /* Difficulty selector */
    UILabel *difficultyLabel = [[UILabel alloc] init];
    difficultyLabel.translatesAutoresizingMaskIntoConstraints = NO;
    difficultyLabel.text = @"Difficulty / 難度:";
    difficultyLabel.font = [UIFont systemFontOfSize:14];
    difficultyLabel.textColor = [UIColor secondaryLabelColor];
    [contentView addSubview:difficultyLabel];

    self.difficultySelector =
        [[UISegmentedControl alloc] initWithItems:@[ @"Easy / 簡單", @"Medium / 中等", @"Hard / 困難" ]];
    self.difficultySelector.translatesAutoresizingMaskIntoConstraints = NO;
    self.difficultySelector.selectedSegmentIndex = 0;
    [self.difficultySelector addTarget:self
                                action:@selector(difficultyChanged)
                      forControlEvents:UIControlEventValueChanged];
    [contentView addSubview:self.difficultySelector];

    /* Practice text display */
    UILabel *practiceLabel = [[UILabel alloc] init];
    practiceLabel.translatesAutoresizingMaskIntoConstraints = NO;
    practiceLabel.text = @"Type this / 請輸入:";
    practiceLabel.font = [UIFont systemFontOfSize:14];
    practiceLabel.textColor = [UIColor secondaryLabelColor];
    [contentView addSubview:practiceLabel];

    self.practiceTextLabel = [[UILabel alloc] init];
    self.practiceTextLabel.translatesAutoresizingMaskIntoConstraints = NO;
    self.practiceTextLabel.font = [UIFont systemFontOfSize:24];
    self.practiceTextLabel.textAlignment = NSTextAlignmentCenter;
    self.practiceTextLabel.numberOfLines = 0;
    self.practiceTextLabel.backgroundColor = [UIColor secondarySystemBackgroundColor];
    self.practiceTextLabel.layer.cornerRadius = 10;
    self.practiceTextLabel.layer.masksToBounds = YES;
    [contentView addSubview:self.practiceTextLabel];

    /* Hint label */
    self.hintLabel = [[UILabel alloc] init];
    self.hintLabel.translatesAutoresizingMaskIntoConstraints = NO;
    self.hintLabel.font = [UIFont systemFontOfSize:14];
    self.hintLabel.textColor = [UIColor tertiaryLabelColor];
    self.hintLabel.textAlignment = NSTextAlignmentCenter;
    [contentView addSubview:self.hintLabel];

    /* Progress bar */
    self.progressView = [[UIProgressView alloc] initWithProgressViewStyle:UIProgressViewStyleDefault];
    self.progressView.translatesAutoresizingMaskIntoConstraints = NO;
    self.progressView.progressTintColor = [UIColor systemGreenColor];
    [contentView addSubview:self.progressView];

    /* Input field */
    self.inputField = [[UITextField alloc] init];
    self.inputField.translatesAutoresizingMaskIntoConstraints = NO;
    self.inputField.borderStyle = UITextBorderStyleRoundedRect;
    self.inputField.placeholder = @"Start typing here... / 在這裡輸入...";
    self.inputField.font = [UIFont systemFontOfSize:20];
    self.inputField.autocorrectionType = UITextAutocorrectionTypeNo;
    self.inputField.autocapitalizationType = UITextAutocapitalizationTypeNone;
    self.inputField.delegate = self;
    [self.inputField addTarget:self
                        action:@selector(textFieldDidChange:)
              forControlEvents:UIControlEventEditingChanged];
    [contentView addSubview:self.inputField];

    /* Stats label */
    self.statsLabel = [[UILabel alloc] init];
    self.statsLabel.translatesAutoresizingMaskIntoConstraints = NO;
    self.statsLabel.font = [UIFont monospacedSystemFontOfSize:14 weight:UIFontWeightRegular];
    self.statsLabel.textColor = [UIColor secondaryLabelColor];
    self.statsLabel.textAlignment = NSTextAlignmentCenter;
    self.statsLabel.numberOfLines = 0;
    [contentView addSubview:self.statsLabel];

    /* Buttons */
    UIStackView *buttonStack = [[UIStackView alloc] init];
    buttonStack.translatesAutoresizingMaskIntoConstraints = NO;
    buttonStack.axis = UILayoutConstraintAxisHorizontal;
    buttonStack.spacing = 16;
    buttonStack.distribution = UIStackViewDistributionFillEqually;
    [contentView addSubview:buttonStack];

    self.newTextButton = [UIButton buttonWithType:UIButtonTypeSystem];
    [self.newTextButton setTitle:@"New Text / 換題" forState:UIControlStateNormal];
    self.newTextButton.titleLabel.font = [UIFont boldSystemFontOfSize:16];
    self.newTextButton.backgroundColor = [UIColor systemBlueColor];
    [self.newTextButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    self.newTextButton.layer.cornerRadius = 8;
    [self.newTextButton addTarget:self
                           action:@selector(loadRandomPracticeText)
                 forControlEvents:UIControlEventTouchUpInside];
    [buttonStack addArrangedSubview:self.newTextButton];

    self.resetButton = [UIButton buttonWithType:UIButtonTypeSystem];
    [self.resetButton setTitle:@"Reset / 重置" forState:UIControlStateNormal];
    self.resetButton.titleLabel.font = [UIFont boldSystemFontOfSize:16];
    self.resetButton.backgroundColor = [UIColor systemOrangeColor];
    [self.resetButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    self.resetButton.layer.cornerRadius = 8;
    [self.resetButton addTarget:self action:@selector(resetSession) forControlEvents:UIControlEventTouchUpInside];
    [buttonStack addArrangedSubview:self.resetButton];

    /* Layout constraints */
    [NSLayoutConstraint activateConstraints:@[
        [scrollView.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor],
        [scrollView.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],
        [scrollView.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor],
        [scrollView.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor],

        [contentView.topAnchor constraintEqualToAnchor:scrollView.topAnchor],
        [contentView.leadingAnchor constraintEqualToAnchor:scrollView.leadingAnchor],
        [contentView.trailingAnchor constraintEqualToAnchor:scrollView.trailingAnchor],
        [contentView.bottomAnchor constraintEqualToAnchor:scrollView.bottomAnchor],
        [contentView.widthAnchor constraintEqualToAnchor:scrollView.widthAnchor],

        [self.titleLabel.topAnchor constraintEqualToAnchor:contentView.topAnchor constant:16],
        [self.titleLabel.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor constant:16],
        [self.titleLabel.trailingAnchor constraintEqualToAnchor:contentView.trailingAnchor constant:-16],

        [categoryLabel.topAnchor constraintEqualToAnchor:self.titleLabel.bottomAnchor constant:20],
        [categoryLabel.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor constant:16],

        [self.categorySelector.topAnchor constraintEqualToAnchor:categoryLabel.bottomAnchor constant:8],
        [self.categorySelector.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor constant:16],
        [self.categorySelector.trailingAnchor constraintEqualToAnchor:contentView.trailingAnchor constant:-16],

        [difficultyLabel.topAnchor constraintEqualToAnchor:self.categorySelector.bottomAnchor constant:16],
        [difficultyLabel.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor constant:16],

        [self.difficultySelector.topAnchor constraintEqualToAnchor:difficultyLabel.bottomAnchor constant:8],
        [self.difficultySelector.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor constant:16],
        [self.difficultySelector.trailingAnchor constraintEqualToAnchor:contentView.trailingAnchor constant:-16],

        [practiceLabel.topAnchor constraintEqualToAnchor:self.difficultySelector.bottomAnchor constant:24],
        [practiceLabel.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor constant:16],

        [self.practiceTextLabel.topAnchor constraintEqualToAnchor:practiceLabel.bottomAnchor constant:8],
        [self.practiceTextLabel.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor constant:16],
        [self.practiceTextLabel.trailingAnchor constraintEqualToAnchor:contentView.trailingAnchor constant:-16],
        [self.practiceTextLabel.heightAnchor constraintGreaterThanOrEqualToConstant:80],

        [self.hintLabel.topAnchor constraintEqualToAnchor:self.practiceTextLabel.bottomAnchor constant:8],
        [self.hintLabel.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor constant:16],
        [self.hintLabel.trailingAnchor constraintEqualToAnchor:contentView.trailingAnchor constant:-16],

        [self.progressView.topAnchor constraintEqualToAnchor:self.hintLabel.bottomAnchor constant:16],
        [self.progressView.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor constant:16],
        [self.progressView.trailingAnchor constraintEqualToAnchor:contentView.trailingAnchor constant:-16],

        [self.inputField.topAnchor constraintEqualToAnchor:self.progressView.bottomAnchor constant:16],
        [self.inputField.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor constant:16],
        [self.inputField.trailingAnchor constraintEqualToAnchor:contentView.trailingAnchor constant:-16],
        [self.inputField.heightAnchor constraintEqualToConstant:50],

        [self.statsLabel.topAnchor constraintEqualToAnchor:self.inputField.bottomAnchor constant:16],
        [self.statsLabel.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor constant:16],
        [self.statsLabel.trailingAnchor constraintEqualToAnchor:contentView.trailingAnchor constant:-16],

        [buttonStack.topAnchor constraintEqualToAnchor:self.statsLabel.bottomAnchor constant:24],
        [buttonStack.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor constant:16],
        [buttonStack.trailingAnchor constraintEqualToAnchor:contentView.trailingAnchor constant:-16],
        [buttonStack.heightAnchor constraintEqualToConstant:50],
        [buttonStack.bottomAnchor constraintEqualToAnchor:contentView.bottomAnchor constant:-24]
    ]];
}

#pragma mark - Practice Text Loading

- (void)loadRandomPracticeText {
    HimePracticeText text;
    int ret = hime_typing_get_random_text((HimePracticeCategory) self.category, &text);

    if (ret == 0) {
        self.currentPracticeText = [NSString stringWithUTF8String:text.text];
        self.currentHint = [NSString stringWithUTF8String:text.hint];
        self.totalCharacters = text.char_count;
    } else {
        /* Fallback text */
        self.currentPracticeText = @"The quick brown fox.";
        self.currentHint = @"";
        self.totalCharacters = 20;
    }

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

    self.inputField.text = @"";
    self.progressView.progress = 0;

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

    self.inputField.text = @"";
    self.progressView.progress = 0;

    [self updatePracticeTextDisplay];
    [self updateStats];
}

- (void)endSession {
    if (!self.ctx)
        return;

    hime_typing_end_session(self.ctx);
    self.sessionActive = NO;
}

#pragma mark - Text Processing

- (void)textFieldDidChange:(UITextField *)textField {
    if (!self.sessionActive || !self.ctx)
        return;

    NSString *text = textField.text;
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
                        value:[UIColor systemGreenColor]
                        range:NSMakeRange(0, MIN(self.currentPosition, self.currentPracticeText.length))];
    }

    /* Highlight current character */
    if (self.currentPosition < self.currentPracticeText.length) {
        [attrStr addAttribute:NSBackgroundColorAttributeName
                        value:[UIColor systemYellowColor]
                        range:NSMakeRange(self.currentPosition, 1)];
    }

    self.practiceTextLabel.attributedText = attrStr;
    self.hintLabel.text = self.currentHint.length > 0 ? [NSString stringWithFormat:@"Hint: %@", self.currentHint] : @"";

    /* Update progress */
    if (self.totalCharacters > 0) {
        self.progressView.progress = (float) self.currentPosition / self.totalCharacters;
    }
}

- (void)updateStats {
    HimeTypingStats stats;
    if (hime_typing_get_stats(self.ctx, &stats) != 0) {
        self.statsLabel.text = @"";
        return;
    }

    NSTimeInterval elapsed = [[NSDate date] timeIntervalSinceDate:self.sessionStartTime];

    self.statsLabel.text = [NSString stringWithFormat:@"Progress: %d/%d | Correct: %d | Incorrect: %d\n"
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

    UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Completed! / 完成!"
                                                                   message:message
                                                            preferredStyle:UIAlertControllerStyleAlert];

    [alert addAction:[UIAlertAction actionWithTitle:@"New Text / 換題"
                                              style:UIAlertActionStyleDefault
                                            handler:^(UIAlertAction *action) {
                                                [self loadRandomPracticeText];
                                            }]];

    [alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleCancel handler:nil]];

    [self presentViewController:alert animated:YES completion:nil];
}

#pragma mark - Settings Changes

- (void)categoryChanged {
    self.category = (HimePracticeCategory) self.categorySelector.selectedSegmentIndex;
    [self loadRandomPracticeText];
}

- (void)difficultyChanged {
    self.difficulty = (HimePracticeDifficulty) (self.difficultySelector.selectedSegmentIndex + 1);
    [self loadRandomPracticeText];
}

#pragma mark - UITextFieldDelegate

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
    [textField resignFirstResponder];
    return YES;
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self.view endEditing:YES];
}

@end
