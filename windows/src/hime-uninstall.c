/*
 * HIME Uninstaller for Windows
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 *
 * Unregisters the TSF DLL, removes installed files, and cleans up registry.
 * Requires administrator privileges (enforced via embedded UAC manifest).
 */

#include <stdio.h>

#include <windows.h>

#define HIME_VERSION "0.10.1"
#define INSTALL_DIR L"C:\\Program Files\\HIME"
#define UNINSTALL_REG_KEY \
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\HIME"

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
delete_directory_contents (const WCHAR *dir) {
    WCHAR pattern[MAX_PATH];
    WIN32_FIND_DATAW fd;

    _snwprintf (pattern, MAX_PATH, L"%ls\\*", dir);

    HANDLE hFind = FindFirstFileW (pattern, &fd);
    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do {
        if (wcscmp (fd.cFileName, L".") == 0 || wcscmp (fd.cFileName, L"..") == 0)
            continue;

        WCHAR path[MAX_PATH];
        _snwprintf (path, MAX_PATH, L"%ls\\%ls", dir, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            delete_directory_contents (path);
            RemoveDirectoryW (path);
        } else {
            if (!DeleteFileW (path)) {
                wprintf (L"  Warning: could not delete %ls (error %lu)\n",
                         path, GetLastError ());
            } else {
                wprintf (L"  Deleted %ls\n", path);
            }
        }
    } while (FindNextFileW (hFind, &fd));

    FindClose (hFind);
}

int wmain (int argc, WCHAR *argv[]) {
    (void) argc;
    (void) argv;

    wprintf (L"\n");
    wprintf (L"HIME Input Method Editor - Uninstaller v" HIME_VERSION "\n");
    wprintf (L"=============================================\n\n");

    /* Check if HIME is installed */
    WCHAR tsf_path[MAX_PATH];
    _snwprintf (tsf_path, MAX_PATH, L"%ls\\hime-tsf.dll", INSTALL_DIR);
    if (GetFileAttributesW (tsf_path) == INVALID_FILE_ATTRIBUTES) {
        wprintf (L"HIME does not appear to be installed.\n");
        wprintf (L"Press any key to exit...\n");
        goto wait_exit;
    }

    /* Confirm */
    wprintf (L"This will remove HIME Input Method Editor from your system.\n");
    wprintf (L"Continue? (Y/N): ");
    fflush (stdout);

    WCHAR response[16];
    if (!fgetws (response, 16, stdin))
        return 1;
    if (response[0] != L'Y' && response[0] != L'y') {
        wprintf (L"Uninstall cancelled.\n");
        return 0;
    }

    wprintf (L"\n");

    /* Unregister TSF DLL */
    wprintf (L"Unregistering HIME Text Service...\n");
    WCHAR unreg_args[MAX_PATH + 16];
    _snwprintf (unreg_args, MAX_PATH + 16, L"/u /s \"%ls\"", tsf_path);
    run_regsvr32 (unreg_args);

    /* Delete all files except the uninstaller itself */
    wprintf (L"\nRemoving files:\n");

    WCHAR self_path[MAX_PATH];
    GetModuleFileNameW (NULL, self_path, MAX_PATH);

    /* Delete data directory contents */
    WCHAR data_dir[MAX_PATH];
    _snwprintf (data_dir, MAX_PATH, L"%ls\\data", INSTALL_DIR);
    delete_directory_contents (data_dir);
    RemoveDirectoryW (data_dir);

    /* Delete files in install dir (except self) */
    WCHAR pattern[MAX_PATH];
    WIN32_FIND_DATAW fd;
    _snwprintf (pattern, MAX_PATH, L"%ls\\*", INSTALL_DIR);

    HANDLE hFind = FindFirstFileW (pattern, &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;

            WCHAR file_path[MAX_PATH];
            _snwprintf (file_path, MAX_PATH, L"%ls\\%ls", INSTALL_DIR,
                        fd.cFileName);

            /* Skip self — handle below */
            if (_wcsicmp (file_path, self_path) == 0)
                continue;

            if (!DeleteFileW (file_path)) {
                wprintf (L"  Warning: could not delete %ls (error %lu)\n",
                         file_path, GetLastError ());
            } else {
                wprintf (L"  Deleted %ls\n", file_path);
            }
        } while (FindNextFileW (hFind, &fd));

        FindClose (hFind);
    }

    /* Schedule self-deletion on reboot */
    if (!MoveFileExW (self_path, NULL, MOVEFILE_DELAY_UNTIL_REBOOT)) {
        wprintf (L"  Warning: could not schedule self-deletion (error %lu)\n",
                 GetLastError ());
    } else {
        wprintf (L"  %ls will be removed on next reboot\n", self_path);
    }

    /* Schedule install directory removal on reboot */
    MoveFileExW (INSTALL_DIR, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);

    /* Try to remove install directory now (will succeed if empty) */
    RemoveDirectoryW (INSTALL_DIR);

    /* Remove Add/Remove Programs registry entry */
    wprintf (L"\nRemoving Add/Remove Programs entry...\n");
    LONG rc = RegDeleteKeyW (HKEY_LOCAL_MACHINE, UNINSTALL_REG_KEY);
    if (rc != ERROR_SUCCESS && rc != ERROR_FILE_NOT_FOUND) {
        wprintf (L"  Warning: could not remove registry key (error %ld)\n",
                 rc);
    }

    wprintf (L"\nUninstall complete!\n");
    wprintf (
        L"Please sign out and sign back in to complete the removal.\n\n");
    wprintf (L"Press any key to exit...\n");

wait_exit:;
    HANDLE hStdin = GetStdHandle (STD_INPUT_HANDLE);
    FlushConsoleInputBuffer (hStdin);
    INPUT_RECORD ir;
    DWORD read;
    while (ReadConsoleInputW (hStdin, &ir, 1, &read)) {
        if (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown)
            break;
    }

    return 0;
}
