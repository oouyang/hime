/*
 * HIME Android Settings Activity
 *
 * Main activity for HIME configuration and setup.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

package org.hime.android;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.provider.Settings;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

/**
 * HimeSettingsActivity provides setup instructions and configuration options.
 */
public class HimeSettingsActivity extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        /* Create UI programmatically */
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(48, 48, 48, 48);
        layout.setBackgroundColor(0xFF303030);

        /* Title */
        TextView title = new TextView(this);
        title.setText(R.string.setup_title);
        title.setTextSize(24);
        title.setTextColor(0xFFFFFFFF);
        title.setPadding(0, 0, 0, 32);
        layout.addView(title);

        /* Setup steps */
        addStep(layout, getString(R.string.setup_step1));
        addStep(layout, getString(R.string.setup_step2));
        addStep(layout, getString(R.string.setup_step3));
        addStep(layout, getString(R.string.setup_step4));
        addStep(layout, getString(R.string.setup_step5));

        /* Open settings button */
        Button settingsButton = new Button(this);
        settingsButton.setText(R.string.setup_open_settings);
        settingsButton.setOnClickListener(v -> openInputMethodSettings());
        LinearLayout.LayoutParams btnParams = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT,
            LinearLayout.LayoutParams.WRAP_CONTENT
        );
        btnParams.topMargin = 48;
        layout.addView(settingsButton, btnParams);

        /* Switch IME button */
        Button switchButton = new Button(this);
        switchButton.setText("切換輸入法");
        switchButton.setOnClickListener(v -> showInputMethodPicker());
        LinearLayout.LayoutParams switchParams = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT,
            LinearLayout.LayoutParams.WRAP_CONTENT
        );
        switchParams.topMargin = 16;
        layout.addView(switchButton, switchParams);

        /* About section */
        TextView aboutTitle = new TextView(this);
        aboutTitle.setText(R.string.settings_about);
        aboutTitle.setTextSize(20);
        aboutTitle.setTextColor(0xFFFFFFFF);
        aboutTitle.setPadding(0, 64, 0, 16);
        layout.addView(aboutTitle);

        TextView version = new TextView(this);
        version.setText(String.format(getString(R.string.about_version), BuildConfig.VERSION_NAME));
        version.setTextColor(0xFFAAAAAA);
        layout.addView(version);

        TextView description = new TextView(this);
        description.setText(R.string.about_description);
        description.setTextColor(0xFFAAAAAA);
        description.setPadding(0, 16, 0, 0);
        layout.addView(description);

        TextView license = new TextView(this);
        license.setText(R.string.about_license);
        license.setTextColor(0xFFAAAAAA);
        license.setPadding(0, 16, 0, 0);
        layout.addView(license);

        setContentView(layout);
    }

    private void addStep(LinearLayout layout, String text) {
        TextView step = new TextView(this);
        step.setText(text);
        step.setTextSize(16);
        step.setTextColor(0xFFDDDDDD);
        step.setPadding(0, 8, 0, 8);
        layout.addView(step);
    }

    private void openInputMethodSettings() {
        Intent intent = new Intent(Settings.ACTION_INPUT_METHOD_SETTINGS);
        startActivity(intent);
    }

    private void showInputMethodPicker() {
        InputMethodManager imm = (InputMethodManager) getSystemService(INPUT_METHOD_SERVICE);
        if (imm != null) {
            imm.showInputMethodPicker();
        }
    }
}
