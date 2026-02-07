/*
 * HIME Linux GTK Typing Practice
 *
 * GTK-based typing practice interface for Linux.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>

#include <gtk/gtk.h>

#include "hime.h"

/* UI Widgets */
static GtkWidget *main_window;
static GtkWidget *category_combo;
static GtkWidget *difficulty_combo;
static GtkWidget *practice_text_label;
static GtkWidget *hint_label;
static GtkWidget *progress_bar;
static GtkWidget *input_entry;
static GtkWidget *stats_label;

/* State */
static char practice_text[512] = "";
static char hint_text[256] = "";
static int current_position = 0;
static int total_characters = 0;
static int correct_count = 0;
static int incorrect_count = 0;
static time_t start_time = 0;
static gboolean session_active = FALSE;

/* Built-in practice texts */
static const char *PRACTICE_TEXTS[] = {
    /* English - Easy */
    "The quick brown fox jumps over the lazy dog.",
    "Pack my box with five dozen liquor jugs.",
    "How vexingly quick daft zebras jump!",

    /* Traditional Chinese */
    "你好嗎？",
    "謝謝你。",
    "今天天氣很好。",

    /* Simplified Chinese */
    "你好吗？",
    "谢谢你。",

    /* Mixed */
    "Hello 你好 World 世界",
    NULL};

static const char *PRACTICE_HINTS[] = {
    "",
    "",
    "",
    "ni3 hao3 ma",
    "xie4 xie4 ni3",
    "jin1 tian1 tian1 qi4 hen3 hao3",
    "ni hao ma",
    "xie xie ni",
    "",
    NULL};

/* Count UTF-8 characters */
static int utf8_char_count (const char *str) {
    if (!str)
        return 0;
    int count = 0;
    while (*str) {
        if ((*str & 0xC0) != 0x80) {
            count++;
        }
        str++;
    }
    return count;
}

/* Get UTF-8 character length */
static int utf8_char_len (const char *str) {
    if (!str || !*str)
        return 0;
    unsigned char c = (unsigned char) *str;
    if ((c & 0x80) == 0)
        return 1;
    if ((c & 0xE0) == 0xC0)
        return 2;
    if ((c & 0xF0) == 0xE0)
        return 3;
    if ((c & 0xF8) == 0xF0)
        return 4;
    return 1;
}

/* Get byte offset for character position */
static int utf8_byte_offset (const char *str, int char_pos) {
    if (!str)
        return 0;
    int offset = 0;
    int pos = 0;
    while (*str && pos < char_pos) {
        int len = utf8_char_len (str);
        offset += len;
        str += len;
        pos++;
    }
    return offset;
}

/* Update display */
static void update_display (void) {
    /* Update practice text with highlighting */
    char markup[2048];
    int byte_offset = utf8_byte_offset (practice_text, current_position);

    if (current_position > 0 && byte_offset > 0) {
        char typed_part[512];
        strncpy (typed_part, practice_text, byte_offset);
        typed_part[byte_offset] = '\0';

        const char *remaining = practice_text + byte_offset;
        int remaining_len = utf8_char_len (remaining);
        char current_char[8] = "";
        if (remaining_len > 0 && *remaining) {
            strncpy (current_char, remaining, remaining_len);
            current_char[remaining_len] = '\0';
        }
        const char *rest = remaining + remaining_len;

        snprintf (markup, sizeof (markup),
                  "<span foreground='green'>%s</span><span background='yellow'>%s</span>%s",
                  typed_part, current_char, rest);
    } else if (total_characters > 0) {
        int first_len = utf8_char_len (practice_text);
        char first_char[8] = "";
        if (first_len > 0) {
            strncpy (first_char, practice_text, first_len);
            first_char[first_len] = '\0';
        }
        snprintf (markup, sizeof (markup),
                  "<span background='yellow'>%s</span>%s",
                  first_char, practice_text + first_len);
    } else {
        snprintf (markup, sizeof (markup), "%s", practice_text);
    }

    gtk_label_set_markup (GTK_LABEL (practice_text_label), markup);

    /* Update hint */
    if (strlen (hint_text) > 0) {
        char hint_markup[300];
        snprintf (hint_markup, sizeof (hint_markup), "Hint: %s", hint_text);
        gtk_label_set_text (GTK_LABEL (hint_label), hint_markup);
    } else {
        gtk_label_set_text (GTK_LABEL (hint_label), "");
    }

    /* Update progress */
    if (total_characters > 0) {
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress_bar),
                                       (double) current_position / total_characters);
    }
}

/* Update statistics */
static void update_stats (void) {
    time_t now = time (NULL);
    double elapsed_sec = difftime (now, start_time);
    double elapsed_min = elapsed_sec / 60.0;

    double accuracy = 100.0;
    if (correct_count + incorrect_count > 0) {
        accuracy = (double) correct_count / (correct_count + incorrect_count) * 100.0;
    }

    double cpm = 0;
    if (elapsed_min > 0) {
        cpm = correct_count / elapsed_min;
    }

    char stats[256];
    snprintf (stats, sizeof (stats),
              "Progress: %d/%d | Correct: %d | Incorrect: %d\n"
              "Accuracy: %.1f%% | Speed: %.1f chars/min",
              current_position, total_characters,
              correct_count, incorrect_count, accuracy, cpm);
    gtk_label_set_text (GTK_LABEL (stats_label), stats);
}

/* Load random practice text */
static void load_random_text (void) {
    int category = gtk_combo_box_get_active (GTK_COMBO_BOX (category_combo));
    int start_idx = 0;
    int end_idx = 9;

    switch (category) {
    case 0: /* English */
        start_idx = 0;
        end_idx = 3;
        break;
    case 1: /* Zhuyin */
    case 3: /* Cangjie */
        start_idx = 3;
        end_idx = 6;
        break;
    case 2: /* Pinyin */
        start_idx = 6;
        end_idx = 8;
        break;
    case 4: /* Mixed */
        start_idx = 8;
        end_idx = 9;
        break;
    }

    int idx = start_idx + (rand () % (end_idx - start_idx));
    if (idx >= 9)
        idx = 0;

    strncpy (practice_text, PRACTICE_TEXTS[idx], sizeof (practice_text) - 1);
    strncpy (hint_text, PRACTICE_HINTS[idx], sizeof (hint_text) - 1);
    total_characters = utf8_char_count (practice_text);
}

/* Start new session */
static void start_new_session (void) {
    current_position = 0;
    correct_count = 0;
    incorrect_count = 0;
    start_time = time (NULL);
    session_active = TRUE;

    gtk_entry_set_text (GTK_ENTRY (input_entry), "");
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress_bar), 0);

    update_display ();
    update_stats ();
}

/* Reset session */
static void reset_session (void) {
    current_position = 0;
    correct_count = 0;
    incorrect_count = 0;
    start_time = time (NULL);

    gtk_entry_set_text (GTK_ENTRY (input_entry), "");
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress_bar), 0);

    update_display ();
    update_stats ();
}

/* Show completion dialog */
static void show_completion_dialog (void) {
    time_t now = time (NULL);
    double elapsed_sec = difftime (now, start_time);

    double accuracy = 100.0;
    if (correct_count + incorrect_count > 0) {
        accuracy = (double) correct_count / (correct_count + incorrect_count) * 100.0;
    }
    double cpm = correct_count / (elapsed_sec / 60.0);

    char message[256];
    snprintf (message, sizeof (message),
              "Accuracy: %.1f%%\nSpeed: %.1f chars/min\nTime: %.1f seconds",
              accuracy, cpm, elapsed_sec);

    GtkWidget *dialog = gtk_message_dialog_new (
        GTK_WINDOW (main_window),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_NONE,
        "Completed! / 完成!");
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", message);
    gtk_dialog_add_button (GTK_DIALOG (dialog), "New Text / 換題", GTK_RESPONSE_YES);
    gtk_dialog_add_button (GTK_DIALOG (dialog), "OK", GTK_RESPONSE_OK);

    int response = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);

    if (response == GTK_RESPONSE_YES) {
        load_random_text ();
        start_new_session ();
    }
}

/* Input changed callback */
static void on_input_changed (GtkEditable *editable, gpointer user_data) {
    if (!session_active)
        return;
    if (current_position >= total_characters)
        return;

    const char *text = gtk_entry_get_text (GTK_ENTRY (input_entry));
    int text_len = strlen (text);

    if (text_len == 0)
        return;

    /* Get last character */
    int last_char_offset = text_len - utf8_char_len (text + text_len - 1);
    if (last_char_offset < 0)
        last_char_offset = 0;
    const char *last_char = text + last_char_offset;
    int last_char_len = utf8_char_len (last_char);

    /* Get expected character */
    int expected_offset = utf8_byte_offset (practice_text, current_position);
    const char *expected = practice_text + expected_offset;
    int expected_len = utf8_char_len (expected);

    /* Compare */
    if (last_char_len == expected_len && memcmp (last_char, expected, expected_len) == 0) {
        correct_count++;
    } else {
        incorrect_count++;
    }

    current_position++;
    update_display ();
    update_stats ();

    /* Check completion */
    if (current_position >= total_characters) {
        session_active = FALSE;
        show_completion_dialog ();
    }
}

/* New text button callback */
static void on_new_text_clicked (GtkButton *button, gpointer user_data) {
    load_random_text ();
    start_new_session ();
}

/* Reset button callback */
static void on_reset_clicked (GtkButton *button, gpointer user_data) {
    reset_session ();
}

/* Category changed callback */
static void on_category_changed (GtkComboBox *combo, gpointer user_data) {
    load_random_text ();
    start_new_session ();
}

/* Create main window */
static void create_window (void) {
    main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (main_window), "HIME Typing Practice / 打字練習");
    gtk_window_set_default_size (GTK_WINDOW (main_window), 500, 450);
    gtk_container_set_border_width (GTK_CONTAINER (main_window), 20);
    g_signal_connect (main_window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add (GTK_CONTAINER (main_window), vbox);

    /* Title */
    GtkWidget *title = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (title), "<b><big>Typing Practice / 打字練習</big></b>");
    gtk_box_pack_start (GTK_BOX (vbox), title, FALSE, FALSE, 0);

    /* Category */
    GtkWidget *category_label = gtk_label_new ("Category / 類別:");
    gtk_widget_set_halign (category_label, GTK_ALIGN_START);
    gtk_box_pack_start (GTK_BOX (vbox), category_label, FALSE, FALSE, 0);

    category_combo = gtk_combo_box_text_new ();
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (category_combo), "English");
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (category_combo), "注音 (Zhuyin)");
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (category_combo), "拼音 (Pinyin)");
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (category_combo), "倉頡 (Cangjie)");
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (category_combo), "Mixed / 混合");
    gtk_combo_box_set_active (GTK_COMBO_BOX (category_combo), 0);
    g_signal_connect (category_combo, "changed", G_CALLBACK (on_category_changed), NULL);
    gtk_box_pack_start (GTK_BOX (vbox), category_combo, FALSE, FALSE, 0);

    /* Difficulty */
    GtkWidget *difficulty_label = gtk_label_new ("Difficulty / 難度:");
    gtk_widget_set_halign (difficulty_label, GTK_ALIGN_START);
    gtk_box_pack_start (GTK_BOX (vbox), difficulty_label, FALSE, FALSE, 0);

    difficulty_combo = gtk_combo_box_text_new ();
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (difficulty_combo), "Easy / 簡單");
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (difficulty_combo), "Medium / 中等");
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (difficulty_combo), "Hard / 困難");
    gtk_combo_box_set_active (GTK_COMBO_BOX (difficulty_combo), 0);
    gtk_box_pack_start (GTK_BOX (vbox), difficulty_combo, FALSE, FALSE, 0);

    /* Practice text label */
    GtkWidget *practice_label = gtk_label_new ("Type this / 請輸入:");
    gtk_widget_set_halign (practice_label, GTK_ALIGN_START);
    gtk_box_pack_start (GTK_BOX (vbox), practice_label, FALSE, FALSE, 5);

    /* Practice text display */
    practice_text_label = gtk_label_new ("");
    gtk_label_set_line_wrap (GTK_LABEL (practice_text_label), TRUE);
    gtk_widget_set_size_request (practice_text_label, -1, 60);

    GtkWidget *practice_frame = gtk_frame_new (NULL);
    gtk_container_add (GTK_CONTAINER (practice_frame), practice_text_label);
    gtk_box_pack_start (GTK_BOX (vbox), practice_frame, FALSE, FALSE, 0);

    /* Hint label */
    hint_label = gtk_label_new ("");
    gtk_box_pack_start (GTK_BOX (vbox), hint_label, FALSE, FALSE, 0);

    /* Progress bar */
    progress_bar = gtk_progress_bar_new ();
    gtk_box_pack_start (GTK_BOX (vbox), progress_bar, FALSE, FALSE, 5);

    /* Input entry */
    input_entry = gtk_entry_new ();
    gtk_entry_set_placeholder_text (GTK_ENTRY (input_entry), "Start typing here... / 在這裡輸入...");
    g_signal_connect (input_entry, "changed", G_CALLBACK (on_input_changed), NULL);
    gtk_box_pack_start (GTK_BOX (vbox), input_entry, FALSE, FALSE, 5);

    /* Stats label */
    stats_label = gtk_label_new ("Progress: 0/0 | Accuracy: 100%");
    gtk_box_pack_start (GTK_BOX (vbox), stats_label, FALSE, FALSE, 5);

    /* Button box */
    GtkWidget *button_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_set_homogeneous (GTK_BOX (button_box), TRUE);
    gtk_box_pack_start (GTK_BOX (vbox), button_box, FALSE, FALSE, 10);

    GtkWidget *new_text_button = gtk_button_new_with_label ("New Text / 換題");
    g_signal_connect (new_text_button, "clicked", G_CALLBACK (on_new_text_clicked), NULL);
    gtk_box_pack_start (GTK_BOX (button_box), new_text_button, TRUE, TRUE, 0);

    GtkWidget *reset_button = gtk_button_new_with_label ("Reset / 重置");
    g_signal_connect (reset_button, "clicked", G_CALLBACK (on_reset_clicked), NULL);
    gtk_box_pack_start (GTK_BOX (button_box), reset_button, TRUE, TRUE, 0);

    gtk_widget_show_all (main_window);
}

int main (int argc, char *argv[]) {
    srand (time (NULL));

    gtk_init (&argc, &argv);

    create_window ();
    load_random_text ();
    start_new_session ();

    gtk_main ();

    return 0;
}
