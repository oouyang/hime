/*
 * HIME Keyboard View Implementation
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#import "HIMEKeyboardView.h"
#import "HIMEKeyboardViewController.h"

/* Bopomofo keyboard layout - Standard Zhuyin */
static NSString *const kZhuyinRow1[] = {@"ㄅ", @"ㄉ", @"ˇ", @"ˋ", @"ㄓ", @"ˊ", @"˙", @"ㄚ", @"ㄞ", @"ㄢ"};
static NSString *const kZhuyinRow2[] = {@"ㄆ", @"ㄊ", @"ㄍ", @"ㄐ", @"ㄔ", @"ㄗ", @"ㄧ", @"ㄛ", @"ㄟ", @"ㄣ"};
static NSString *const kZhuyinRow3[] = {@"ㄇ", @"ㄋ", @"ㄎ", @"ㄑ", @"ㄕ", @"ㄘ", @"ㄨ", @"ㄜ", @"ㄠ", @"ㄤ"};
static NSString *const kZhuyinRow4[] = {@"ㄈ", @"ㄌ", @"ㄏ", @"ㄒ", @"ㄖ", @"ㄙ", @"ㄩ", @"ㄝ", @"ㄡ", @"ㄥ"};

/* Key mappings (Zhuyin symbol to keyboard character) */
static NSDictionary *kZhuyinToKey = nil;

@interface HIMEKeyboardView ()

/* Candidate bar */
@property (nonatomic, strong) UIScrollView *candidateScrollView;
@property (nonatomic, strong) UIStackView *candidateStackView;
@property (nonatomic, strong) UILabel *preeditLabel;
@property (nonatomic, strong) UILabel *modeLabel;

/* Keyboard rows */
@property (nonatomic, strong) NSArray<UIStackView *> *keyboardRows;

/* Page buttons */
@property (nonatomic, strong) UIButton *pageUpButton;
@property (nonatomic, strong) UIButton *pageDownButton;

@end

@implementation HIMEKeyboardView

+ (void)initialize {
    if (self == [HIMEKeyboardView class]) {
        /* Map Zhuyin symbols to keyboard characters */
        kZhuyinToKey = @{
            @"ㄅ" : @"1",
            @"ㄆ" : @"q",
            @"ㄇ" : @"a",
            @"ㄈ" : @"z",
            @"ㄉ" : @"2",
            @"ㄊ" : @"w",
            @"ㄋ" : @"s",
            @"ㄌ" : @"x",
            @"ˇ" : @"3",
            @"ㄍ" : @"e",
            @"ㄎ" : @"d",
            @"ㄏ" : @"c",
            @"ˋ" : @"4",
            @"ㄐ" : @"r",
            @"ㄑ" : @"f",
            @"ㄒ" : @"v",
            @"ㄓ" : @"5",
            @"ㄔ" : @"t",
            @"ㄕ" : @"g",
            @"ㄖ" : @"b",
            @"ˊ" : @"6",
            @"ㄗ" : @"y",
            @"ㄘ" : @"h",
            @"ㄙ" : @"n",
            @"˙" : @"7",
            @"ㄧ" : @"u",
            @"ㄨ" : @"j",
            @"ㄩ" : @"m",
            @"ㄚ" : @"8",
            @"ㄛ" : @"i",
            @"ㄜ" : @"k",
            @"ㄝ" : @",",
            @"ㄞ" : @"9",
            @"ㄟ" : @"o",
            @"ㄠ" : @"l",
            @"ㄡ" : @".",
            @"ㄢ" : @"0",
            @"ㄣ" : @"p",
            @"ㄤ" : @";",
            @"ㄥ" : @"/",
            @"ㄦ" : @"-",
        };
    }
}

- (instancetype)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        self.backgroundColor = [UIColor colorWithRed:0.82 green:0.84 blue:0.86 alpha:1.0];
        [self setupUI];
    }
    return self;
}

#pragma mark - UI Setup

- (void)setupUI {
    [self setupCandidateBar];
    [self setupKeyboard];
}

- (void)setupCandidateBar {
    /* Container for candidate bar */
    UIView *candidateBar = [[UIView alloc] init];
    candidateBar.translatesAutoresizingMaskIntoConstraints = NO;
    candidateBar.backgroundColor = [UIColor whiteColor];
    [self addSubview:candidateBar];

    /* Preedit label */
    self.preeditLabel = [[UILabel alloc] init];
    self.preeditLabel.translatesAutoresizingMaskIntoConstraints = NO;
    self.preeditLabel.font = [UIFont systemFontOfSize:16];
    self.preeditLabel.textColor = [UIColor darkGrayColor];
    [candidateBar addSubview:self.preeditLabel];

    /* Mode indicator */
    self.modeLabel = [[UILabel alloc] init];
    self.modeLabel.translatesAutoresizingMaskIntoConstraints = NO;
    self.modeLabel.font = [UIFont systemFontOfSize:14];
    self.modeLabel.text = @"中";
    self.modeLabel.textColor = [UIColor systemBlueColor];
    [candidateBar addSubview:self.modeLabel];

    /* Candidate scroll view */
    self.candidateScrollView = [[UIScrollView alloc] init];
    self.candidateScrollView.translatesAutoresizingMaskIntoConstraints = NO;
    self.candidateScrollView.showsHorizontalScrollIndicator = NO;
    [candidateBar addSubview:self.candidateScrollView];

    /* Candidate stack view */
    self.candidateStackView = [[UIStackView alloc] init];
    self.candidateStackView.translatesAutoresizingMaskIntoConstraints = NO;
    self.candidateStackView.axis = UILayoutConstraintAxisHorizontal;
    self.candidateStackView.spacing = 8;
    self.candidateStackView.alignment = UIStackViewAlignmentCenter;
    [self.candidateScrollView addSubview:self.candidateStackView];

    /* Page buttons */
    self.pageUpButton = [UIButton buttonWithType:UIButtonTypeSystem];
    self.pageUpButton.translatesAutoresizingMaskIntoConstraints = NO;
    [self.pageUpButton setTitle:@"◀" forState:UIControlStateNormal];
    [self.pageUpButton addTarget:self action:@selector(pageUpTapped) forControlEvents:UIControlEventTouchUpInside];
    [candidateBar addSubview:self.pageUpButton];

    self.pageDownButton = [UIButton buttonWithType:UIButtonTypeSystem];
    self.pageDownButton.translatesAutoresizingMaskIntoConstraints = NO;
    [self.pageDownButton setTitle:@"▶" forState:UIControlStateNormal];
    [self.pageDownButton addTarget:self action:@selector(pageDownTapped) forControlEvents:UIControlEventTouchUpInside];
    [candidateBar addSubview:self.pageDownButton];

    /* Constraints */
    [NSLayoutConstraint activateConstraints:@[
        [candidateBar.topAnchor constraintEqualToAnchor:self.topAnchor],
        [candidateBar.leadingAnchor constraintEqualToAnchor:self.leadingAnchor],
        [candidateBar.trailingAnchor constraintEqualToAnchor:self.trailingAnchor],
        [candidateBar.heightAnchor constraintEqualToConstant:44],

        [self.modeLabel.leadingAnchor constraintEqualToAnchor:candidateBar.leadingAnchor constant:8],
        [self.modeLabel.centerYAnchor constraintEqualToAnchor:candidateBar.centerYAnchor],
        [self.modeLabel.widthAnchor constraintEqualToConstant:24],

        [self.preeditLabel.leadingAnchor constraintEqualToAnchor:self.modeLabel.trailingAnchor constant:8],
        [self.preeditLabel.centerYAnchor constraintEqualToAnchor:candidateBar.centerYAnchor],
        [self.preeditLabel.widthAnchor constraintEqualToConstant:60],

        [self.pageUpButton.leadingAnchor constraintEqualToAnchor:self.preeditLabel.trailingAnchor constant:4],
        [self.pageUpButton.centerYAnchor constraintEqualToAnchor:candidateBar.centerYAnchor],
        [self.pageUpButton.widthAnchor constraintEqualToConstant:30],

        [self.candidateScrollView.leadingAnchor constraintEqualToAnchor:self.pageUpButton.trailingAnchor constant:4],
        [self.candidateScrollView.topAnchor constraintEqualToAnchor:candidateBar.topAnchor],
        [self.candidateScrollView.bottomAnchor constraintEqualToAnchor:candidateBar.bottomAnchor],

        [self.pageDownButton.leadingAnchor constraintEqualToAnchor:self.candidateScrollView.trailingAnchor constant:4],
        [self.pageDownButton.trailingAnchor constraintEqualToAnchor:candidateBar.trailingAnchor constant:-8],
        [self.pageDownButton.centerYAnchor constraintEqualToAnchor:candidateBar.centerYAnchor],
        [self.pageDownButton.widthAnchor constraintEqualToConstant:30],

        [self.candidateStackView.leadingAnchor constraintEqualToAnchor:self.candidateScrollView.leadingAnchor],
        [self.candidateStackView.trailingAnchor constraintEqualToAnchor:self.candidateScrollView.trailingAnchor],
        [self.candidateStackView.topAnchor constraintEqualToAnchor:self.candidateScrollView.topAnchor],
        [self.candidateStackView.bottomAnchor constraintEqualToAnchor:self.candidateScrollView.bottomAnchor],
        [self.candidateStackView.heightAnchor constraintEqualToAnchor:self.candidateScrollView.heightAnchor],
    ]];
}

- (void)setupKeyboard {
    /* Container for keyboard */
    UIStackView *keyboardContainer = [[UIStackView alloc] init];
    keyboardContainer.translatesAutoresizingMaskIntoConstraints = NO;
    keyboardContainer.axis = UILayoutConstraintAxisVertical;
    keyboardContainer.distribution = UIStackViewDistributionFillEqually;
    keyboardContainer.spacing = 6;
    [self addSubview:keyboardContainer];

    [NSLayoutConstraint activateConstraints:@[
        [keyboardContainer.topAnchor constraintEqualToAnchor:self.topAnchor constant:50],
        [keyboardContainer.leadingAnchor constraintEqualToAnchor:self.leadingAnchor constant:4],
        [keyboardContainer.trailingAnchor constraintEqualToAnchor:self.trailingAnchor constant:-4],
        [keyboardContainer.bottomAnchor constraintEqualToAnchor:self.bottomAnchor constant:-4],
    ]];

    /* Create Zhuyin rows */
    NSArray *rows = @[
        @[
            kZhuyinRow1[0],
            kZhuyinRow1[1],
            kZhuyinRow1[2],
            kZhuyinRow1[3],
            kZhuyinRow1[4],
            kZhuyinRow1[5],
            kZhuyinRow1[6],
            kZhuyinRow1[7],
            kZhuyinRow1[8],
            kZhuyinRow1[9]
        ],
        @[
            kZhuyinRow2[0],
            kZhuyinRow2[1],
            kZhuyinRow2[2],
            kZhuyinRow2[3],
            kZhuyinRow2[4],
            kZhuyinRow2[5],
            kZhuyinRow2[6],
            kZhuyinRow2[7],
            kZhuyinRow2[8],
            kZhuyinRow2[9]
        ],
        @[
            kZhuyinRow3[0],
            kZhuyinRow3[1],
            kZhuyinRow3[2],
            kZhuyinRow3[3],
            kZhuyinRow3[4],
            kZhuyinRow3[5],
            kZhuyinRow3[6],
            kZhuyinRow3[7],
            kZhuyinRow3[8],
            kZhuyinRow3[9]
        ],
        @[
            kZhuyinRow4[0],
            kZhuyinRow4[1],
            kZhuyinRow4[2],
            kZhuyinRow4[3],
            kZhuyinRow4[4],
            kZhuyinRow4[5],
            kZhuyinRow4[6],
            kZhuyinRow4[7],
            kZhuyinRow4[8],
            @"ㄦ"
        ],
        @[ @"中/英", @"🌐", @"␣", @"⌫", @"⏎" ],
    ];

    NSMutableArray *keyboardRows = [NSMutableArray array];
    for (NSArray *rowKeys in rows) {
        UIStackView *row = [self createKeyboardRow:rowKeys];
        [keyboardContainer addArrangedSubview:row];
        [keyboardRows addObject:row];
    }
    self.keyboardRows = keyboardRows;
}

- (UIStackView *)createKeyboardRow:(NSArray<NSString *> *)keys {
    UIStackView *row = [[UIStackView alloc] init];
    row.axis = UILayoutConstraintAxisHorizontal;
    row.distribution = UIStackViewDistributionFillEqually;
    row.spacing = 4;

    for (NSString *key in keys) {
        UIButton *button = [self createKeyButton:key];
        [row addArrangedSubview:button];
    }

    return row;
}

- (UIButton *)createKeyButton:(NSString *)title {
    UIButton *button = [UIButton buttonWithType:UIButtonTypeCustom];
    [button setTitle:title forState:UIControlStateNormal];
    [button setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];
    button.titleLabel.font = [UIFont systemFontOfSize:20];
    button.backgroundColor = [UIColor whiteColor];
    button.layer.cornerRadius = 5;
    button.layer.shadowColor = [UIColor grayColor].CGColor;
    button.layer.shadowOffset = CGSizeMake(0, 1);
    button.layer.shadowOpacity = 0.3;
    button.layer.shadowRadius = 1;

    /* Special key styling */
    if ([title isEqualToString:@"⌫"] || [title isEqualToString:@"⏎"] || [title isEqualToString:@"🌐"] ||
        [title isEqualToString:@"中/英"]) {
        button.backgroundColor = [UIColor colorWithRed:0.68 green:0.70 blue:0.73 alpha:1.0];
        button.titleLabel.font = [UIFont systemFontOfSize:16];
    }

    if ([title isEqualToString:@"␣"]) {
        button.titleLabel.font = [UIFont systemFontOfSize:14];
    }

    [button addTarget:self action:@selector(keyTapped:) forControlEvents:UIControlEventTouchUpInside];

    return button;
}

#pragma mark - Actions

- (void)keyTapped:(UIButton *)sender {
    NSString *key = sender.titleLabel.text;

    /* Convert Zhuyin symbol to keyboard character */
    NSString *mappedKey = kZhuyinToKey[key];
    if (mappedKey) {
        key = mappedKey;
    }

    [self.controller handleKeyPress:key];
}

- (void)pageUpTapped {
    [self.controller pageUp];
}

- (void)pageDownTapped {
    [self.controller pageDown];
}

#pragma mark - UI Updates

- (void)updateCandidates {
    /* Clear existing candidates */
    for (UIView *subview in self.candidateStackView.arrangedSubviews) {
        [subview removeFromSuperview];
    }

    /* Update preedit */
    self.preeditLabel.text = self.controller.engine.preeditString ?: @"";

    /* Add candidates */
    NSArray *candidates = [self.controller.engine candidatesForCurrentPage];
    for (NSInteger i = 0; i < candidates.count; i++) {
        UIButton *button = [UIButton buttonWithType:UIButtonTypeSystem];
        [button setTitle:candidates[i] forState:UIControlStateNormal];
        button.titleLabel.font = [UIFont systemFontOfSize:22];
        button.tag = i;
        [button addTarget:self action:@selector(candidateTapped:) forControlEvents:UIControlEventTouchUpInside];

        /* Add number label */
        NSString *labelText = [NSString stringWithFormat:@"%ld.%@", (long) (i + 1), candidates[i]];
        [button setTitle:labelText forState:UIControlStateNormal];

        [self.candidateStackView addArrangedSubview:button];
    }

    /* Update page buttons */
    self.pageUpButton.enabled = self.controller.engine.currentPage > 0;
    self.pageDownButton.enabled = self.controller.engine.hasCandidates &&
                                  (self.controller.engine.currentPage + 1) * self.controller.engine.candidatesPerPage <
                                      self.controller.engine.candidateCount;
}

- (void)updateModeIndicator {
    self.modeLabel.text = self.controller.engine.chineseMode ? @"中" : @"英";
}

- (void)candidateTapped:(UIButton *)sender {
    [self.controller selectCandidate:sender.tag];
}

@end
