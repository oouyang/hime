/*
 * HIME Installer for Windows
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 *
 * Installs HIME IME to C:\Program Files\HIME\ and registers the TSF DLL.
 * Handles upgrades by unregistering the old DLL before overwriting.
 * Requires administrator privileges (enforced via embedded UAC manifest).
 */

#include <stdio.h>

#include <windows.h>

#define HIME_VERSION "0.10.1"
#define INSTALL_DIR L"C:\\Program Files\\HIME"
#define UNINSTALL_REG_KEY \
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\HIME"

static BOOL
is_running_as_admin (void)
{
    BOOL is_admin = FALSE;
    SID_IDENTIFIER_AUTHORITY authority = {SECURITY_NT_AUTHORITY};
    PSID admin_group = NULL;

    if (AllocateAndInitializeSid (&authority, 2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0, &admin_group)) {
        CheckTokenMembership (NULL, admin_group, &is_admin);
        FreeSid (admin_group);
    }
    return is_admin;
}

static BOOL
copy_file_checked (const WCHAR *src, const WCHAR *dst) {
    if (!CopyFileW (src, dst, FALSE)) {
        wprintf (L"  FAILED to copy %ls -> %ls (error %lu)\n", src, dst,
                 GetLastError ());
        return FALSE;
    }
    wprintf (L"  %ls\n", dst);
    return TRUE;
}

static void
run_regsvr32 (const WCHAR *args) {
    WCHAR cmd[MAX_PATH + 64];
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    _snwprintf (cmd, sizeof (cmd) / sizeof (cmd[0]), L"regsvr32 %ls", args);

    ZeroMemory (&si, sizeof (si));
    si.cb = sizeof (si);
    ZeroMemory (&pi, sizeof (pi));

    if (CreateProcessW (NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        WaitForSingleObject (pi.hProcess, INFINITE);
        CloseHandle (pi.hProcess);
        CloseHandle (pi.hThread);
    }
}

static void
write_uninstall_registry (void) {
    HKEY hKey;
    DWORD disp;
    LONG rc = RegCreateKeyExW (HKEY_LOCAL_MACHINE, UNINSTALL_REG_KEY, 0, NULL,
                               0, KEY_WRITE, NULL, &hKey, &disp);
    if (rc != ERROR_SUCCESS) {
        wprintf (
            L"Warning: could not create Add/Remove Programs entry "
            L"(error %ld)\n",
            rc);
        return;
    }

    WCHAR uninstall_str[MAX_PATH + 32];
    _snwprintf (uninstall_str, sizeof (uninstall_str) / sizeof (uninstall_str[0]),
                L"\"%ls\\hime-uninstall.exe\"", INSTALL_DIR);

#define SET_SZ(name, val)                                        \
    RegSetValueExW (hKey, name, 0, REG_SZ, (const BYTE *) (val), \
                    (DWORD) ((wcslen (val) + 1) * sizeof (WCHAR)))

    SET_SZ (L"DisplayName", L"HIME Input Method Editor");
    SET_SZ (L"DisplayVersion", L"" HIME_VERSION);
    SET_SZ (L"Publisher", L"HIME Team");
    SET_SZ (L"UninstallString", uninstall_str);
    SET_SZ (L"InstallLocation", INSTALL_DIR);

#undef SET_SZ

    DWORD one = 1;
    RegSetValueExW (hKey, L"NoModify", 0, REG_DWORD, (const BYTE *) &one,
                    sizeof (one));
    RegSetValueExW (hKey, L"NoRepair", 0, REG_DWORD, (const BYTE *) &one,
                    sizeof (one));

    RegCloseKey (hKey);
}

static BOOL
copy_data_files (const WCHAR *src_data_dir, const WCHAR *dst_data_dir) {
    WCHAR pattern[MAX_PATH];
    WIN32_FIND_DATAW fd;
    BOOL ok = TRUE;

    _snwprintf (pattern, MAX_PATH, L"%ls\\*", src_data_dir);

    HANDLE hFind = FindFirstFileW (pattern, &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        wprintf (L"  Warning: no data files found in %ls\n", src_data_dir);
        return FALSE;
    }

    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        WCHAR src_path[MAX_PATH], dst_path[MAX_PATH];
        _snwprintf (src_path, MAX_PATH, L"%ls\\%ls", src_data_dir,
                    fd.cFileName);
        _snwprintf (dst_path, MAX_PATH, L"%ls\\%ls", dst_data_dir,
                    fd.cFileName);

        if (!copy_file_checked (src_path, dst_path))
            ok = FALSE;
    } while (FindNextFileW (hFind, &fd));

    FindClose (hFind);
    return ok;
}

static void
wait_for_keypress (void)
{
    HANDLE hStdin = GetStdHandle (STD_INPUT_HANDLE);
    FlushConsoleInputBuffer (hStdin);
    INPUT_RECORD ir;
    DWORD read;
    while (ReadConsoleInputW (hStdin, &ir, 1, &read)) {
        if (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown)
            break;
    }
}

int wmain (int argc, WCHAR *argv[]) {
    (void) argc;
    (void) argv;

    wprintf (L"\n");
    wprintf (L"HIME Input Method Editor - Installer v" HIME_VERSION "\n");
    wprintf (L"==========================================\n\n");

    if (!is_running_as_admin ()) {
        wprintf (L"ERROR: Administrator privileges are required.\n\n");
        wprintf (L"Please right-click hime-install.exe and select\n");
        wprintf (L"\"Run as administrator\", then try again.\n\n");
        wprintf (L"Press any key to exit...\n");
        wait_for_keypress ();
        return 1;
    }

    /* Determine source directory (where this exe is running from) */
    WCHAR exe_path[MAX_PATH];
    GetModuleFileNameW (NULL, exe_path, MAX_PATH);
    WCHAR *last_sep = wcsrchr (exe_path, L'\\');
    if (last_sep)
        *last_sep = L'\0';
    WCHAR src_dir[MAX_PATH];
    wcscpy (src_dir, exe_path);

    /* Check for upgrade: unregister old DLL if present */
    WCHAR old_tsf[MAX_PATH];
    _snwprintf (old_tsf, MAX_PATH, L"%ls\\hime-tsf.dll", INSTALL_DIR);
    if (GetFileAttributesW (old_tsf) != INVALID_FILE_ATTRIBUTES) {
        wprintf (L"Existing installation detected. Upgrading...\n");
        wprintf (L"Unregistering old TSF DLL...\n");
        WCHAR unreg_args[MAX_PATH + 16];
        _snwprintf (unreg_args, MAX_PATH + 16, L"/u /s \"%ls\"", old_tsf);
        run_regsvr32 (unreg_args);
    }

    /* Create install directories */
    wprintf (L"Creating install directory...\n");
    CreateDirectoryW (INSTALL_DIR, NULL);
    WCHAR data_dir[MAX_PATH];
    _snwprintf (data_dir, MAX_PATH, L"%ls\\data", INSTALL_DIR);
    CreateDirectoryW (data_dir, NULL);

    /* Copy DLLs */
    wprintf (L"\nCopying files:\n");
    WCHAR src_path[MAX_PATH], dst_path[MAX_PATH];
    BOOL ok = TRUE;

    const WCHAR *dlls[] = {L"hime-core.dll", L"hime-tsf.dll"};
    for (int i = 0; i < 2; i++) {
        _snwprintf (src_path, MAX_PATH, L"%ls\\%ls", src_dir, dlls[i]);
        _snwprintf (dst_path, MAX_PATH, L"%ls\\%ls", INSTALL_DIR, dlls[i]);
        if (!copy_file_checked (src_path, dst_path))
            ok = FALSE;
    }

    /* Copy installer and uninstaller */
    const WCHAR *exes[] = {L"hime-install.exe", L"hime-uninstall.exe"};
    for (int i = 0; i < 2; i++) {
        _snwprintf (src_path, MAX_PATH, L"%ls\\%ls", src_dir, exes[i]);
        _snwprintf (dst_path, MAX_PATH, L"%ls\\%ls", INSTALL_DIR, exes[i]);
        /* Skip if source and destination are the same file */
        if (_wcsicmp (src_path, dst_path) != 0) {
            if (!copy_file_checked (src_path, dst_path))
                ok = FALSE;
        }
    }

    /* Copy data files */
    WCHAR src_data_dir[MAX_PATH];
    _snwprintf (src_data_dir, MAX_PATH, L"%ls\\data", src_dir);
    if (!copy_data_files (src_data_dir, data_dir))
        ok = FALSE;

    if (!ok) {
        wprintf (L"\nWarning: some files failed to copy.\n");
    }

    /* Register TSF DLL */
    wprintf (L"\nRegistering HIME Text Service...\n");
    WCHAR reg_args[MAX_PATH + 16];
    _snwprintf (reg_args, MAX_PATH + 16, L"/s \"%ls\\hime-tsf.dll\"",
                INSTALL_DIR);
    run_regsvr32 (reg_args);

    /* Write Add/Remove Programs registry entry */
    wprintf (L"Writing Add/Remove Programs entry...\n");
    write_uninstall_registry ();

    wprintf (L"\n");
    wprintf (L"Installation complete!\n\n");
    wprintf (L"Next steps:\n");
    wprintf (L"  1. Open Settings > Time & Language > Language & Region\n");
    wprintf (L"  2. Under your language, click '...' > Language options\n");
    wprintf (L"  3. Add a keyboard and select 'HIME Input Method'\n");
    wprintf (L"  4. Use Win+Space to switch input methods\n\n");
    wprintf (L"Press any key to exit...\n");
    wait_for_keypress ();

    return ok ? 0 : 1;
}
