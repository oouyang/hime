/*
 * Copyright (C) 2020 The HIME team, Taiwan
 * Copyright (C) 2011 Edward Der-Hua Liu, Hsin-Chu, Taiwan
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation version 2.1
 * of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <signal.h>
#include <string.h>

#include "hime.h"

#include "hime-protocol.h"
#include "im-srv.h"
#include "lang.h"
#include "win-gtab.h"
#include "win-kbm.h"
#include "win-pho.h"
#include "win0.h"
#include "win1.h"

/* Forward declarations - grouped for maintainability */
void load_tsin_db (void);
void load_tsin_conf (void);
void load_settings (void);
void load_tab_pho_file (void);
void disp_hide_tsin_status_row (void);
void update_win_kbm_inited (void);
void change_tsin_line_color (void);
void change_win0_style (void);
void change_win_gtab_style (void);
void destroy_inmd_menu (void);
void load_gtab_list (gboolean);
void change_win1_font (void);
void set_wselkey (char *s);
gboolean init_in_method (int in_no);
void toggle_im_enabled (void);
void change_tsin_font_size (void);
void change_gtab_font_size (void);
void change_pho_font_size (void);
void change_win_sym_font_size (void);
void change_module_font_size (void);
void toggle_gb_output (void);
void execute_message (char *message);
void disp_win_kbm_capslock_init (void);
void reload_tsin_db (void);
void load_phrase (void);
void exec_setup_scripts (void);
void free_pho_mem (void);
void free_tsin (void);
void free_all_IC (void);
void free_gtab (void);
void free_phrase (void);
void sim_output (void);
void trad_output (void);

#if TRAY_ENABLED
void update_item_active_all (void);
void disp_tray_icon (void);
void destroy_tray (void);
void init_tray (void);
void init_tray_double (void);
#if TRAY_UNITY
void init_tray_appindicator (void);
#endif
#endif

Window root;

int input_window_width, input_window_height;
int win_x, win_y;  // actual win x/y

// display width/height in pixels
// display_width and display_height are global variable shared across files
int display_width, display_height;

gboolean key_press_shift;

Window xim_xwin;

extern unich_t *fullchar[];
gboolean win_kbm_inited;

char *half_char_to_full_char (KeySym xkey) {
    if (xkey < ' ' || xkey > 127)
        return NULL;
    return _ (fullchar[xkey - ' ']);
}

static void start_inmd_window (void) {
    GtkWidget *win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_realize (win);
    xim_xwin = GDK_WINDOW_XWINDOW (gtk_widget_get_window (win));
    dbg ("xim_xwin %x\n", xim_xwin);
}

#if USE_XIM
char *lc;

static XIMStyle Styles[] = {
#if 1
    XIMPreeditCallbacks | XIMStatusCallbacks,  // OnTheSpot
    XIMPreeditCallbacks | XIMStatusArea,       // OnTheSpot
    XIMPreeditCallbacks | XIMStatusNothing,    // OnTheSpot
#endif
    XIMPreeditPosition | XIMStatusArea,     // OverTheSpot
    XIMPreeditPosition | XIMStatusNothing,  // OverTheSpot
    XIMPreeditPosition | XIMStatusNone,     // OverTheSpot
#if 1
    XIMPreeditArea | XIMStatusArea,     // OffTheSpot
    XIMPreeditArea | XIMStatusNothing,  // OffTheSpot
    XIMPreeditArea | XIMStatusNone,     // OffTheSpot
#endif
    XIMPreeditNothing | XIMStatusNothing,  // Root
    XIMPreeditNothing | XIMStatusNone,     // Root
};
static XIMStyles im_styles;

#if 1
static XIMTriggerKey trigger_keys[] = {
    {XK_space, ControlMask, ControlMask},
    {XK_space, ShiftMask, ShiftMask},
    {XK_space, Mod1Mask, Mod1Mask},  // Alt
    {XK_space, Mod4Mask, Mod4Mask},  // Windows
};
#endif

/* Supported Encodings */
static XIMEncoding chEncodings[] = {
    "COMPOUND_TEXT",
    0};
static XIMEncodings encodings;

int xim_ForwardEventHandler (IMForwardEventStruct *call_data);

XIMS current_ims;
extern void toggle_im_enabled ();

int MyTriggerNotifyHandler (IMTriggerNotifyStruct *call_data) {
    //    dbg("MyTriggerNotifyHandler %d %x\n", call_data->key_index, call_data->event_mask);

    if (call_data->flag == 0) { /* on key */
                                //        db(g("trigger %d\n", call_data->key_index);
        if ((call_data->key_index == 0 && hime_im_toggle_keys == Control_Space) ||
            (call_data->key_index == 3 && hime_im_toggle_keys == Shift_Space) ||
            (call_data->key_index == 6 && hime_im_toggle_keys == Alt_Space) ||
            (call_data->key_index == 9 && hime_im_toggle_keys == Windows_Space)) {
            toggle_im_enabled ();
        }
        return True;
    } else {
        /* never happens */
        return False;
    }
}

#if 0
void switch_IC_index(int index);
#endif
void CreateIC (IMChangeICStruct *call_data);
void DeleteIC (CARD16 icid);
void SetIC (IMChangeICStruct *call_data);
void GetIC (IMChangeICStruct *call_data);
int xim_hime_FocusIn (IMChangeFocusStruct *call_data);
int xim_hime_FocusOut (IMChangeFocusStruct *call_data);

#define MAX_CONNECT 20000

/* Debug macro for XIM protocol - set to 1 to enable verbose XIM logging */
#define XIM_DEBUG 0
#if XIM_DEBUG
#define XIM_DBG(fmt, ...) dbg (fmt, ##__VA_ARGS__)
#else
#define XIM_DBG(fmt, ...) \
    do {                  \
    } while (0)
#endif

int hime_ProtoHandler (XIMS ims, IMProtocol *call_data) {
    current_ims = ims;

    switch (call_data->major_code) {
    case XIM_OPEN: {
        IMOpenStruct *pimopen = (IMOpenStruct *) call_data;
        if (pimopen->connect_id > MAX_CONNECT - 1)
            return True;
        XIM_DBG ("XIM_OPEN lang=%s connect_id=%d\n", pimopen->lang.name, pimopen->connect_id);
        return True;
    }

    case XIM_CLOSE:
        XIM_DBG ("XIM_CLOSE\n");
        return True;

    case XIM_CREATE_IC:
        XIM_DBG ("XIM_CREATE_IC\n");
        CreateIC ((IMChangeICStruct *) call_data);
        return True;

    case XIM_DESTROY_IC: {
        IMChangeICStruct *pimcha = (IMChangeICStruct *) call_data;
        XIM_DBG ("XIM_DESTROY_IC icid=%d\n", pimcha->icid);
        DeleteIC (pimcha->icid);
        return True;
    }

    case XIM_SET_IC_VALUES:
        XIM_DBG ("XIM_SET_IC_VALUES\n");
        SetIC ((IMChangeICStruct *) call_data);
        return True;

    case XIM_GET_IC_VALUES:
        XIM_DBG ("XIM_GET_IC_VALUES\n");
        GetIC ((IMChangeICStruct *) call_data);
        return True;

    case XIM_FORWARD_EVENT:
        XIM_DBG ("XIM_FORWARD_EVENT\n");
        return xim_ForwardEventHandler ((IMForwardEventStruct *) call_data);

    case XIM_SET_IC_FOCUS:
        XIM_DBG ("XIM_SET_IC_FOCUS\n");
        return xim_hime_FocusIn ((IMChangeFocusStruct *) call_data);

    case XIM_UNSET_IC_FOCUS:
        XIM_DBG ("XIM_UNSET_IC_FOCUS\n");
        return xim_hime_FocusOut ((IMChangeFocusStruct *) call_data);

    case XIM_RESET_IC:
        XIM_DBG ("XIM_RESET_IC\n");
        return True;

    case XIM_TRIGGER_NOTIFY:
        XIM_DBG ("XIM_TRIGGER_NOTIFY\n");
        MyTriggerNotifyHandler ((IMTriggerNotifyStruct *) call_data);
        return True;

    case XIM_PREEDIT_START_REPLY:
        XIM_DBG ("XIM_PREEDIT_START_REPLY\n");
        return True;

    case XIM_PREEDIT_CARET_REPLY:
        XIM_DBG ("XIM_PREEDIT_CARET_REPLY\n");
        return True;

    case XIM_STR_CONVERSION_REPLY:
        XIM_DBG ("XIM_STR_CONVERSION_REPLY\n");
        return True;

    default:
        dbg ("Unknown XIM major code: %d\n", call_data->major_code);
        break;
    }

    return True;
}

static void open_xim (void) {
    XIMTriggerKeys triggerKeys;

    im_styles.supported_styles = Styles;
    im_styles.count_styles = sizeof (Styles) / sizeof (Styles[0]);

    triggerKeys.count_keys = sizeof (trigger_keys) / sizeof (trigger_keys[0]);
    triggerKeys.keylist = trigger_keys;

    encodings.count_encodings = sizeof (chEncodings) / sizeof (XIMEncoding) - 1;
    encodings.supported_encodings = chEncodings;

    char *xim_name = get_hime_xim_name ();

    XIMS xims = IMOpenIM (dpy,
                          IMServerWindow, xim_xwin,  // input window
                          IMModifiers, "Xi18n",      // X11R6 protocol
                          IMServerName, xim_name,    // XIM server name
                          IMLocale, lc,
                          IMServerTransport, "X/",    // Comm. protocol
                          IMInputStyles, &im_styles,  // faked styles
                          IMEncodingList, &encodings,
                          IMProtocolHandler, hime_ProtoHandler,
                          IMFilterEventMask, KeyPressMask | KeyReleaseMask,
                          IMOnKeysList, &triggerKeys,
                          NULL);

    if (xims == NULL) {
        p_err ("IMOpenIM '%s' failed. Maybe another XIM server is running.\n",
               xim_name);
    }
}

#endif  // if USE_XIM

static int get_in_method_by_filename (const char *filename) {
    int i, in_method = 0;
    gboolean found = FALSE;
    for (i = 0; i < inmdN; i++) {
        if (strcmp (filename, inmd[i].filename))
            continue;
        found = TRUE;
        in_method = i;
        break;
    }
    if (!found)
        in_method = default_input_method;
    return in_method;
}

/* Client state backup for reload - avoids VLA with dynamic allocation */
typedef struct {
    char *filename;
    gboolean im_enabled;
} ClientStateBackup;

static void reload_data (void) {
    dbg ("reload_data\n");

    /* Save current CS state */
    char *current_filename = NULL;
    gboolean current_im_enabled = FALSE;
    if (current_CS) {
        current_im_enabled = current_CS->b_im_enabled;
        current_filename = g_strdup (inmd[current_CS->in_method].filename);
    }

    /* Save all client states with dynamic allocation */
    ClientStateBackup *backups = NULL;
    if (hime_clientsN > 0) {
        backups = g_new0 (ClientStateBackup, hime_clientsN);
        for (int c = 0; c < hime_clientsN; c++) {
            if (!hime_clients[c].cs)
                continue;
            ClientState *cs = hime_clients[c].cs;
            backups[c].im_enabled = cs->b_im_enabled;
            backups[c].filename = g_strdup (inmd[cs->in_method].filename);
        }
    }

    /* Reload configuration */
    free_omni_config ();
    load_settings ();
    if (current_method_type () == method_type_TSIN)
        set_wselkey (pho_selkey);

    change_win0_style ();
    change_win1_font ();
    change_win_gtab_style ();
    load_tab_pho_file ();
    update_win_kbm_inited ();
    destroy_inmd_menu ();
    load_gtab_list (TRUE);

#if TRAY_ENABLED
    update_item_active_all ();
#endif

    /* Restore client states */
    if (backups) {
        for (int c = 0; c < hime_clientsN; c++) {
            if (!hime_clients[c].cs || !backups[c].filename)
                continue;
            hime_clients[c].cs->b_im_enabled = TRUE;
            hime_clients[c].cs->in_method = get_in_method_by_filename (backups[c].filename);
            init_in_method (hime_clients[c].cs->in_method);
            if (!backups[c].im_enabled)
                toggle_im_enabled ();
            hime_clients[c].cs->b_im_enabled = backups[c].im_enabled;
            g_free (backups[c].filename);
        }
        g_free (backups);
    }

    /* Restore current CS state */
    if (current_filename) {
        current_CS->b_im_enabled = TRUE;
        init_in_method (get_in_method_by_filename (current_filename));
        if (!current_im_enabled)
            toggle_im_enabled ();
        current_CS->b_im_enabled = current_im_enabled;
        g_free (current_filename);
    }
}

extern gboolean win_kbm_on;

static void change_font_size (void) {
    load_settings ();
    change_tsin_font_size ();
    change_gtab_font_size ();
    change_pho_font_size ();
    change_win_sym_font_size ();
    change_win0_style ();
    change_win_gtab_style ();
    update_win_kbm_inited ();
    change_win1_font ();
    //  change_win_pho_style();
    change_module_font_size ();
}

static int xerror_handler (Display *d, XErrorEvent *eve) {
    return 0;
}

Atom hime_atom;
extern int hime_show_win_kbm;

void cb_trad_sim_toggle (void) {
    toggle_gb_output ();
#if TRAY_ENABLED
    disp_tray_icon ();
#endif
}

void kbm_open_close (GtkButton *checkmenuitem, gboolean b_show) {
    hime_show_win_kbm = b_show;

    if (hime_show_win_kbm) {
        show_win_kbm ();
        disp_win_kbm_capslock_init ();
    } else {
        hide_win_kbm ();
    }
}

void kbm_toggle (void) {
    win_kbm_inited = 1;
    kbm_open_close (NULL, !hime_show_win_kbm);
}

void do_exit (void);

/* Message handler functions for dispatch table */
static void msg_change_font_size (void) {
    change_font_size ();
}

static void msg_gb_output_toggle (void) {
    cb_trad_sim_toggle ();
#if TRAY_ENABLED
    update_item_active_all ();
#endif
}

static void msg_sim_output (void) {
    sim_output ();
#if TRAY_ENABLED
    disp_tray_icon ();
    update_item_active_all ();
#endif
}

static void msg_trad_output (void) {
    trad_output ();
#if TRAY_ENABLED
    disp_tray_icon ();
    update_item_active_all ();
#endif
}

static void msg_kbm_toggle (void) {
    kbm_toggle ();
}

#if TRAY_ENABLED
static void msg_update_tray (void) {
    disp_tray_icon ();
}
#endif

static void msg_reload_tsin (void) {
    reload_tsin_db ();
}

static void msg_exit (void) {
    do_exit ();
}

/* Message dispatch table for O(n) lookup - more maintainable than if-else chain */
typedef struct {
    const char *message;
    void (*handler) (void);
} MessageHandler;

static const MessageHandler message_handlers[] = {
    {CHANGE_FONT_SIZE, msg_change_font_size},
    {GB_OUTPUT_TOGGLE, msg_gb_output_toggle},
    {SIM_OUTPUT_TOGGLE, msg_sim_output},
    {TRAD_OUTPUT_TOGGLE, msg_trad_output},
    {KBM_TOGGLE, msg_kbm_toggle},
#if TRAY_ENABLED
    {UPDATE_TRAY, msg_update_tray},
#endif
    {RELOAD_TSIN_DB, msg_reload_tsin},
    {HIME_EXIT_MESSAGE, msg_exit},
    {NULL, NULL} /* sentinel */
};

void message_cb (char *message) {
    /* Check dispatch table first */
    for (const MessageHandler *h = message_handlers; h->message != NULL; h++) {
        if (strcmp (message, h->message) == 0) {
            h->handler ();
            return;
        }
    }

    /* Handle special case: embedded hime_message */
    if (strstr (message, "#hime_message")) {
        execute_message (message);
        return;
    }

    /* Default: reload data */
    reload_data ();
}

static GdkFilterReturn my_gdk_filter (GdkXEvent *xevent,
                                      GdkEvent *event,
                                      gpointer data) {
    XEvent *xeve = (XEvent *) xevent;
#if 0
   dbg("a zzz %d\n", xeve->type);
#endif

    // only very old WM will enter this
    if (xeve->type == FocusIn || xeve->type == FocusOut) {
#if 0
     dbg("focus %s\n", xeve->type == FocusIn ? "in":"out");
#endif
        return GDK_FILTER_REMOVE;
    }

#if USE_XIM
    if (XFilterEvent (xeve, None) == True)
        return GDK_FILTER_REMOVE;
#endif

    return GDK_FILTER_CONTINUE;
}

static void init_atom_property (void) {
    hime_atom = get_hime_atom (dpy);
    XSetSelectionOwner (dpy, hime_atom, xim_xwin, CurrentTime);
}

void do_exit (void) {
    dbg ("----------------- do_exit ----------------\n");

    free_pho_mem ();
    free_tsin ();
#if USE_XIM
    free_all_IC ();
#endif
    free_gtab ();
    free_phrase ();

    destroy_win0 ();
    destroy_win1 ();
    destroy_win_pho ();
    destroy_win_gtab ();

#if TRAY_ENABLED
    destroy_tray ();
#endif

    free_omni_config ();
    gtk_main_quit ();
}

static void sig_do_exit (int sig) {
    do_exit ();
}

static gboolean delayed_start_cb (gpointer data) {
#if TRAY_ENABLED
    if (hime_status_tray) {
        if (hime_tray_display == HIME_TRAY_DISPLAY_SINGLE)
            init_tray ();
        else if (hime_tray_display == HIME_TRAY_DISPLAY_DOUBLE)
            init_tray_double ();
#if TRAY_UNITY
        else if (hime_tray_display == HIME_TRAY_DISPLAY_APPINDICATOR)
            init_tray_appindicator ();
#endif
    }
#endif

    dbg ("after init_tray\n");

    return FALSE;
}

static void get_display_size (void) {
#if !GTK_CHECK_VERSION(3, 0, 0)
    display_width = gdk_screen_width ();
    display_height = gdk_screen_height ();
#else
    GdkRectangle work_area;
    gdk_monitor_get_workarea (get_primary_monitor (), &work_area);

    // TODO:
    // The workaround only fixes the wrong input window position bug when using multiple monitors with the same resolution.
    // The different resolution scenario should be handled well in the future.
    display_width = work_area.width * gdk_display_get_n_monitors (get_default_display ());
    display_height = work_area.height;
#endif
}

static void screen_size_changed (GdkScreen *screen, gpointer user_data) {
    get_display_size ();
}

int main (int argc, char **argv) {
    gtk_init (&argc, &argv);

    signal (SIGCHLD, SIG_IGN);
    signal (SIGPIPE, SIG_IGN);

    if (getenv ("HIME_DAEMON")) {
        daemon (1, 1);
#if FREEBSD
        setpgid (0, getpid ());
#else
        setpgrp ();
#endif
    }

    set_is_chs ();

    char *lc_ctype = getenv ("LC_CTYPE");
    char *lc_all = getenv ("LC_ALL");
    char *lang = getenv ("LANG");
    if (!lc_ctype && lang)
        lc_ctype = lang;

    if (lc_all)
        lc_ctype = lc_all;

    if (!lc_ctype)
        lc_ctype = "zh_TW.Big5";
    dbg ("hime get env LC_CTYPE=%s  LC_ALL=%s  LANG=%s\n", lc_ctype, lc_all, lang);

#if USE_XIM
    char *t = strchr (lc_ctype, '.');
    if (t) {
        int len = t - lc_ctype;
#if FREEBSD
        lc = strdup (lc_ctype);
        lc[len] = 0;
#else
        lc = g_strndup (lc_ctype, len);
#endif
    } else
        lc = lc_ctype;

    dbg ("hime XIM will use %s as the default encoding\n", lc_ctype);
#endif

    if (argc == 2 && (!strcmp (argv[1], "-v") || !strcmp (argv[1], "--version") || !strcmp (argv[1], "-h"))) {
#if GIT_HAVE
        p_err (" version %s (git %s)\n", HIME_VERSION, GIT_HASH);
#else
        p_err (" version %s\n", HIME_VERSION);
#endif
    }

    init_TableDir ();
    load_settings ();
    load_gtab_list (TRUE);

#if HIME_I18N_MESSAGE
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
#endif

    dbg ("after gtk_init\n");

    dpy = GDK_DISPLAY ();
    root = DefaultRootWindow (dpy);

    get_display_size ();

    // update display_width/display_height on screen size-changed events
    g_signal_connect (
        gdk_screen_get_default (), "size-changed",
        G_CALLBACK (screen_size_changed), NULL);

    dbg ("display width:%d height:%d\n", display_width, display_height);

    start_inmd_window ();

#if USE_XIM
    open_xim ();
#endif

    gdk_window_add_filter (NULL, my_gdk_filter, NULL);

    init_atom_property ();
    signal (SIGINT, sig_do_exit);
    signal (SIGHUP, sig_do_exit);
    // disable the io handler abort
    // void *olderr =
    XSetErrorHandler ((XErrorHandler) xerror_handler);

    init_hime_im_serv (xim_xwin);

    exec_setup_scripts ();

    g_timeout_add (200, delayed_start_cb, NULL);  // Old setting is 5000 here.

    dbg ("before gtk_main\n");

    disp_win_kbm_capslock_init ();

    gtk_main ();

    return 0;
}
