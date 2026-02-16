/*
 * HIME iOS Main View Controller Implementation
 *
 * Displays setup instructions for enabling the keyboard.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#import "ViewController.h"

@interface ViewController ()
@property (nonatomic, strong) UITextView *instructionsTextView;
@property (nonatomic, strong) UITextField *testTextField;
@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    self.view.backgroundColor = [UIColor systemBackgroundColor];

    [self setupUI];
}

- (void)setupUI {
    /* Title */
    UILabel *titleLabel = [[UILabel alloc] init];
    titleLabel.translatesAutoresizingMaskIntoConstraints = NO;
    titleLabel.text = @"HIME 輸入法";
    titleLabel.font = [UIFont boldSystemFontOfSize:28];
    titleLabel.textAlignment = NSTextAlignmentCenter;
    [self.view addSubview:titleLabel];

    /* Subtitle */
    UILabel *subtitleLabel = [[UILabel alloc] init];
    subtitleLabel.translatesAutoresizingMaskIntoConstraints = NO;
    subtitleLabel.text = @"Traditional Chinese Phonetic Input";
    subtitleLabel.font = [UIFont systemFontOfSize:16];
    subtitleLabel.textColor = [UIColor secondaryLabelColor];
    subtitleLabel.textAlignment = NSTextAlignmentCenter;
    [self.view addSubview:subtitleLabel];

    /* Instructions */
    self.instructionsTextView = [[UITextView alloc] init];
    self.instructionsTextView.translatesAutoresizingMaskIntoConstraints = NO;
    self.instructionsTextView.editable = NO;
    self.instructionsTextView.font = [UIFont systemFontOfSize:15];
    self.instructionsTextView.textColor = [UIColor labelColor];
    self.instructionsTextView.backgroundColor = [UIColor secondarySystemBackgroundColor];
    self.instructionsTextView.layer.cornerRadius = 10;
    self.instructionsTextView.textContainerInset = UIEdgeInsetsMake(16, 16, 16, 16);
    self.instructionsTextView.text = [self instructionsText];
    [self.view addSubview:self.instructionsTextView];

    /* Settings button */
    UIButton *settingsButton = [UIButton buttonWithType:UIButtonTypeSystem];
    settingsButton.translatesAutoresizingMaskIntoConstraints = NO;
    [settingsButton setTitle:@"Open Settings" forState:UIControlStateNormal];
    settingsButton.titleLabel.font = [UIFont boldSystemFontOfSize:18];
    settingsButton.backgroundColor = [UIColor systemBlueColor];
    [settingsButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    settingsButton.layer.cornerRadius = 10;
    [settingsButton addTarget:self action:@selector(openSettings) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:settingsButton];

    /* Test text field */
    UILabel *testLabel = [[UILabel alloc] init];
    testLabel.translatesAutoresizingMaskIntoConstraints = NO;
    testLabel.text = @"Test the keyboard:";
    testLabel.font = [UIFont systemFontOfSize:14];
    testLabel.textColor = [UIColor secondaryLabelColor];
    [self.view addSubview:testLabel];

    self.testTextField = [[UITextField alloc] init];
    self.testTextField.translatesAutoresizingMaskIntoConstraints = NO;
    self.testTextField.borderStyle = UITextBorderStyleRoundedRect;
    self.testTextField.placeholder = @"Type here to test HIME keyboard...";
    self.testTextField.font = [UIFont systemFontOfSize:18];
    [self.view addSubview:self.testTextField];

    /* Layout */
    [NSLayoutConstraint activateConstraints:@[
        [titleLabel.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor constant:20],
        [titleLabel.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor constant:20],
        [titleLabel.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor constant:-20],

        [subtitleLabel.topAnchor constraintEqualToAnchor:titleLabel.bottomAnchor constant:8],
        [subtitleLabel.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor constant:20],
        [subtitleLabel.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor constant:-20],

        [self.instructionsTextView.topAnchor constraintEqualToAnchor:subtitleLabel.bottomAnchor constant:20],
        [self.instructionsTextView.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor constant:20],
        [self.instructionsTextView.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor constant:-20],
        [self.instructionsTextView.heightAnchor constraintEqualToConstant:200],

        [settingsButton.topAnchor constraintEqualToAnchor:self.instructionsTextView.bottomAnchor constant:20],
        [settingsButton.centerXAnchor constraintEqualToAnchor:self.view.centerXAnchor],
        [settingsButton.widthAnchor constraintEqualToConstant:200],
        [settingsButton.heightAnchor constraintEqualToConstant:50],

        [testLabel.topAnchor constraintEqualToAnchor:settingsButton.bottomAnchor constant:30],
        [testLabel.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor constant:20],

        [self.testTextField.topAnchor constraintEqualToAnchor:testLabel.bottomAnchor constant:8],
        [self.testTextField.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor constant:20],
        [self.testTextField.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor constant:-20],
        [self.testTextField.heightAnchor constraintEqualToConstant:50],
    ]];
}

- (NSString *)instructionsText {
    return @"To enable HIME Keyboard:\n\n"
           @"1. Open Settings app\n"
           @"2. Go to General → Keyboard → Keyboards\n"
           @"3. Tap \"Add New Keyboard...\"\n"
           @"4. Select \"HIME\" from the list\n\n"
           @"To use HIME Keyboard:\n\n"
           @"1. Tap any text field\n"
           @"2. Tap the 🌐 globe icon on the keyboard\n"
           @"3. Select \"HIME\" keyboard\n"
           @"4. Type Bopomofo symbols and select characters";
}

- (void)openSettings {
    NSURL *settingsURL = [NSURL URLWithString:UIApplicationOpenSettingsURLString];
    if ([[UIApplication sharedApplication] canOpenURL:settingsURL]) {
        [[UIApplication sharedApplication] openURL:settingsURL options:@{} completionHandler:nil];
    }
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self.view endEditing:YES];
}

@end
