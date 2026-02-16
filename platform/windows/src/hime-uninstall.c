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

static BOOL
is_running_as_admin (void) {
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

static void
wait_for_keypress (void) {
    HANDLE hStdin = GetStdHandle (STD_INPUT_HANDLE);
    FlushConsoleInputBuffer (hStdin);
    INPUT_RECORD ir;
    DWORD read;
    while (ReadConsoleInputW (hStdin, &ir, 1, &read)) {
        if (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown)
            break;
    }
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
#ifdef NDEBUG
                if (!wcsstr (fd.cFileName, L".old-"))
#endif
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

    if (!is_running_as_admin ()) {
        wprintf (L"ERROR: Administrator privileges are required.\n\n");
        wprintf (L"Please right-click hime-uninstall.exe and select\n");
        wprintf (L"\"Run as administrator\", then try again.\n\n");
        wprintf (L"Press any key to exit...\n");
        wait_for_keypress ();
        return 1;
    }

    /* Check if HIME is installed */
    WCHAR tsf_path[MAX_PATH];
    _snwprintf (tsf_path, MAX_PATH, L"%ls\\hime-tsf.dll", INSTALL_DIR);
    if (GetFileAttributesW (tsf_path) == INVALID_FILE_ATTRIBUTES) {
        wprintf (L"HIME does not appear to be installed.\n");
        wprintf (L"Press any key to exit...\n");
        wait_for_keypress ();
        return 1;
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

    /* Unregister TSF DLL (calls DllUnregisterServer which removes
     * the language profile and TSF registration) */
    wprintf (L"Unregistering HIME Text Service...\n");
    WCHAR unreg_args[MAX_PATH + 16];
    _snwprintf (unreg_args, MAX_PATH + 16, L"/u /s \"%ls\"", tsf_path);
    run_regsvr32 (unreg_args);

    /* Clean up stale keyboard layout entries from user registry.
     * When a user adds HIME via Settings → Language → Add keyboard,
     * Windows stores the preference in multiple HKCU locations.
     * regsvr32 /u only removes the HKLM registration — the HKCU entries
     * become "無法使用的輸入法" ghost items.
     *
     * HIME CLSID:   {B8A45C32-5F6D-4E2A-9C1B-0D3E4F5A6B7C}
     * Profile GUID:  {C9B56D43-6E7F-5F3B-AD2C-1E4F5061C8D9}
     */
    wprintf (L"Cleaning up user keyboard settings...\n");

#define HIME_CLSID_STR L"{B8A45C32-5F6D-4E2A-9C1B-0D3E4F5A6B7C}"
#define HIME_CLSID_UPPER L"B8A45C32-5F6D-4E2A-9C1B-0D3E4F5A6B7C"
#define HIME_CLSID_LOWER L"b8a45c32-5f6d-4e2a-9c1b-0d3e4f5a6b7c"

    {
        HKEY hKey;

        /* 1. Remove HKCU\Software\Microsoft\CTF\TIP\{CLSID} — the TIP registration */
        if (RegDeleteTreeW (HKEY_CURRENT_USER,
                            L"Software\\Microsoft\\CTF\\TIP\\" HIME_CLSID_STR) == ERROR_SUCCESS) {
            wprintf (L"  Removed CTF TIP entry\n");
        }

        /* 2. Remove assembly items under CTF\SortOrder\AssemblyItem\0x00000404
         *    (scan subkeys for our CLSID) */
        if (RegOpenKeyExW (HKEY_CURRENT_USER,
                           L"Software\\Microsoft\\CTF\\SortOrder\\AssemblyItem\\0x00000404",
                           0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            WCHAR subKeyName[256];
            DWORD idx = 0;
            DWORD nameLen;
            while (1) {
                nameLen = 256;
                LONG rc = RegEnumKeyExW (hKey, idx, subKeyName, &nameLen,
                                         NULL, NULL, NULL, NULL);
                if (rc != ERROR_SUCCESS)
                    break;

                HKEY hSub;
                if (RegOpenKeyExW (hKey, subKeyName, 0, KEY_READ, &hSub) == ERROR_SUCCESS) {
                    WCHAR clsid[128] = {0};
                    DWORD clsidSize = sizeof (clsid);
                    DWORD type;
                    if (RegQueryValueExW (hSub, L"CLSID", NULL, &type,
                                          (BYTE *) clsid, &clsidSize) == ERROR_SUCCESS) {
                        if (_wcsicmp (clsid, HIME_CLSID_STR) == 0) {
                            RegCloseKey (hSub);
                            RegDeleteTreeW (hKey, subKeyName);
                            wprintf (L"  Removed CTF assembly item: %ls\n", subKeyName);
                            continue;
                        }
                    }
                    RegCloseKey (hSub);
                }
                idx++;
            }
            RegCloseKey (hKey);
        }

        /* 3. Remove from CTF\SortOrder\Language\00000404
         *    (values referencing our CLSID or profile GUID) */
        if (RegOpenKeyExW (HKEY_CURRENT_USER,
                           L"Software\\Microsoft\\CTF\\SortOrder\\Language\\00000404",
                           0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            WCHAR valueName[64];
            DWORD idx = 0;
            DWORD nameLen;
            while (1) {
                nameLen = 64;
                WCHAR data[256] = {0};
                DWORD dataSize = sizeof (data);
                DWORD type;
                LONG rc = RegEnumValueW (hKey, idx, valueName, &nameLen,
                                         NULL, &type, (BYTE *) data, &dataSize);
                if (rc != ERROR_SUCCESS)
                    break;

                if (wcsstr (data, HIME_CLSID_UPPER) ||
                    wcsstr (data, HIME_CLSID_LOWER)) {
                    RegDeleteValueW (hKey, valueName);
                    wprintf (L"  Removed CTF language entry: %ls\n", valueName);
                    continue;
                }
                idx++;
            }
            RegCloseKey (hKey);
        }

        /* 4. Remove from Keyboard Layout\Preload
         *    (values like "0404:{B8A45C32-...}{C9B56D43-...}") */
        if (RegOpenKeyExW (HKEY_CURRENT_USER,
                           L"Keyboard Layout\\Preload",
                           0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            WCHAR valueName[32];
            DWORD idx = 0;
            DWORD nameLen;
            while (1) {
                nameLen = 32;
                WCHAR data[256] = {0};
                DWORD dataSize = sizeof (data);
                DWORD type;
                LONG rc = RegEnumValueW (hKey, idx, valueName, &nameLen,
                                         NULL, &type, (BYTE *) data, &dataSize);
                if (rc != ERROR_SUCCESS)
                    break;

                if (wcsstr (data, HIME_CLSID_UPPER) ||
                    wcsstr (data, HIME_CLSID_LOWER)) {
                    RegDeleteValueW (hKey, valueName);
                    wprintf (L"  Removed Preload entry: %ls\n", valueName);
                    continue;
                }
                idx++;
            }
            RegCloseKey (hKey);
        }

        /* 5. Remove from Keyboard Layout\Substitutes (if any) */
        if (RegOpenKeyExW (HKEY_CURRENT_USER,
                           L"Keyboard Layout\\Substitutes",
                           0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            WCHAR valueName[64];
            DWORD idx = 0;
            DWORD nameLen;
            while (1) {
                nameLen = 64;
                WCHAR data[256] = {0};
                DWORD dataSize = sizeof (data);
                DWORD type;
                LONG rc = RegEnumValueW (hKey, idx, valueName, &nameLen,
                                         NULL, &type, (BYTE *) data, &dataSize);
                if (rc != ERROR_SUCCESS)
                    break;

                if (wcsstr (valueName, HIME_CLSID_UPPER) ||
                    wcsstr (valueName, HIME_CLSID_LOWER) ||
                    wcsstr (data, HIME_CLSID_UPPER) ||
                    wcsstr (data, HIME_CLSID_LOWER)) {
                    RegDeleteValueW (hKey, valueName);
                    wprintf (L"  Removed Substitutes entry: %ls\n", valueName);
                    continue;
                }
                idx++;
            }
            RegCloseKey (hKey);
        }
    }

#undef HIME_CLSID_STR
#undef HIME_CLSID_UPPER
#undef HIME_CLSID_LOWER

    /* Delete all files except the uninstaller itself */
    wprintf (L"\nRemoving files:\n");

    WCHAR self_path[MAX_PATH];
    GetModuleFileNameW (NULL, self_path, MAX_PATH);

    /* Delete data and icons directory contents */
    WCHAR data_dir[MAX_PATH];
    _snwprintf (data_dir, MAX_PATH, L"%ls\\data", INSTALL_DIR);
    delete_directory_contents (data_dir);
    RemoveDirectoryW (data_dir);

    WCHAR icons_dir[MAX_PATH];
    _snwprintf (icons_dir, MAX_PATH, L"%ls\\icons", INSTALL_DIR);
    delete_directory_contents (icons_dir);
    RemoveDirectoryW (icons_dir);

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
#ifdef NDEBUG
                /* .old files are scheduled for reboot deletion — skip warning */
                if (!wcsstr (fd.cFileName, L".old-"))
#endif
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
    wait_for_keypress ();

    return 0;
}
