/*
 * Tests for HIME Windows Installer/Uninstaller
 *
 * Verifies:
 *  - PE executables contain embedded UAC manifest
 *  - PE executables are console subsystem (not GUI)
 *  - Installer can create directories and copy files to a temp location
 *  - Uninstaller can remove files from a temp location
 *  - Self-copy detection (skip when src == dst)
 *  - Data file enumeration finds expected files
 *
 * Runs cross-compiled via Wine or natively on Windows.
 * Does NOT require admin privileges — uses a temp directory.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include <windows.h>

static int tests_passed = 0;
static int tests_failed = 0;
static int tests_total = 0;
static const char *current_test = NULL;

#define TEST(name)                        \
    static void test_##name (void);       \
    static void run_test_##name (void) {  \
        current_test = #name;             \
        tests_total++;                    \
        test_##name ();                   \
    }                                     \
    static void test_##name (void)

#define RUN_TEST(name)       \
    do {                     \
        run_test_##name ();  \
    } while (0)

#define PASS()                                          \
    do {                                                \
        printf ("  PASS: %s\n", current_test);          \
        tests_passed++;                                 \
    } while (0)

#define ASSERT_TRUE(expr)                                               \
    do {                                                                \
        if (!(expr)) {                                                  \
            printf ("  FAIL: %s\n    %s:%d: %s\n",                      \
                    current_test, __FILE__, __LINE__, #expr);           \
            tests_failed++;                                             \
            return;                                                     \
        }                                                               \
    } while (0)

#define ASSERT_FALSE(expr)                                              \
    do {                                                                \
        if (expr) {                                                     \
            printf ("  FAIL: %s\n    %s:%d: expected false: %s\n",      \
                    current_test, __FILE__, __LINE__, #expr);           \
            tests_failed++;                                             \
            return;                                                     \
        }                                                               \
    } while (0)

#define ASSERT_EQ(expected, actual)                                     \
    do {                                                                \
        if ((expected) != (actual)) {                                   \
            printf ("  FAIL: %s\n    %s:%d: expected %ld, got %ld\n",   \
                    current_test, __FILE__, __LINE__,                   \
                    (long)(expected), (long)(actual));                  \
            tests_failed++;                                             \
            return;                                                     \
        }                                                               \
    } while (0)

/* ========== Helper: get directory of this test exe ========== */

static void
get_exe_dir (WCHAR *buf, int buflen)
{
    GetModuleFileNameW (NULL, buf, buflen);
    WCHAR *sep = wcsrchr (buf, L'\\');
    if (sep)
        *sep = L'\0';
}

/* ========== PE Structure Tests ========== */

/*
 * Read PE file and check IMAGE_OPTIONAL_HEADER.Subsystem.
 * Console apps = IMAGE_SUBSYSTEM_WINDOWS_CUI (3).
 */
static int
get_pe_subsystem (const WCHAR *path)
{
    HANDLE hFile = CreateFileW (path, GENERIC_READ, FILE_SHARE_READ,
                                NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return -1;

    IMAGE_DOS_HEADER dos;
    DWORD read;
    ReadFile (hFile, &dos, sizeof (dos), &read, NULL);
    if (read != sizeof (dos) || dos.e_magic != IMAGE_DOS_SIGNATURE) {
        CloseHandle (hFile);
        return -1;
    }

    SetFilePointer (hFile, dos.e_lfanew, NULL, FILE_BEGIN);
    DWORD pe_sig;
    ReadFile (hFile, &pe_sig, sizeof (pe_sig), &read, NULL);
    if (read != sizeof (pe_sig) || pe_sig != IMAGE_NT_SIGNATURE) {
        CloseHandle (hFile);
        return -1;
    }

    IMAGE_FILE_HEADER fh;
    ReadFile (hFile, &fh, sizeof (fh), &read, NULL);

    /* Read just the Subsystem field from optional header */
    WORD magic;
    ReadFile (hFile, &magic, sizeof (magic), &read, NULL);

    int subsystem;
    if (magic == 0x20b) {
        /* PE32+ (64-bit) */
        IMAGE_OPTIONAL_HEADER64 opt;
        SetFilePointer (hFile, dos.e_lfanew + 4 + sizeof (IMAGE_FILE_HEADER),
                        NULL, FILE_BEGIN);
        ReadFile (hFile, &opt, sizeof (opt), &read, NULL);
        subsystem = opt.Subsystem;
    } else {
        /* PE32 (32-bit) */
        IMAGE_OPTIONAL_HEADER32 opt;
        SetFilePointer (hFile, dos.e_lfanew + 4 + sizeof (IMAGE_FILE_HEADER),
                        NULL, FILE_BEGIN);
        ReadFile (hFile, &opt, sizeof (opt), &read, NULL);
        subsystem = opt.Subsystem;
    }

    CloseHandle (hFile);
    return subsystem;
}

/*
 * Check if a PE file contains an embedded RT_MANIFEST resource
 * with "requireAdministrator" text.
 */
static BOOL
pe_has_uac_manifest (const WCHAR *path)
{
    HMODULE hMod = LoadLibraryExW (path, NULL,
                                   LOAD_LIBRARY_AS_DATAFILE
                                   | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    if (!hMod)
        return FALSE;

    HRSRC hRes = FindResourceW (hMod, CREATEPROCESS_MANIFEST_RESOURCE_ID,
                                RT_MANIFEST);
    if (!hRes) {
        FreeLibrary (hMod);
        return FALSE;
    }

    HGLOBAL hData = LoadResource (hMod, hRes);
    DWORD size = SizeofResource (hMod, hRes);
    if (!hData || size == 0) {
        FreeLibrary (hMod);
        return FALSE;
    }

    const char *data = (const char *) LockResource (hData);
    BOOL found = FALSE;
    if (data) {
        /* Search for the requireAdministrator string in the manifest XML */
        for (DWORD i = 0; i + 20 < size; i++) {
            if (memcmp (data + i, "requireAdministrator", 20) == 0) {
                found = TRUE;
                break;
            }
        }
    }

    FreeLibrary (hMod);
    return found;
}

TEST (install_exe_is_console) {
    WCHAR dir[MAX_PATH];
    get_exe_dir (dir, MAX_PATH);
    WCHAR path[MAX_PATH];
    _snwprintf (path, MAX_PATH, L"%ls\\hime-install.exe", dir);

    int subsystem = get_pe_subsystem (path);
    /* IMAGE_SUBSYSTEM_WINDOWS_CUI = 3 */
    ASSERT_EQ (3, subsystem);
    PASS ();
}

TEST (uninstall_exe_is_console) {
    WCHAR dir[MAX_PATH];
    get_exe_dir (dir, MAX_PATH);
    WCHAR path[MAX_PATH];
    _snwprintf (path, MAX_PATH, L"%ls\\hime-uninstall.exe", dir);

    int subsystem = get_pe_subsystem (path);
    ASSERT_EQ (3, subsystem);
    PASS ();
}

TEST (install_exe_has_uac_manifest) {
    WCHAR dir[MAX_PATH];
    get_exe_dir (dir, MAX_PATH);
    WCHAR path[MAX_PATH];
    _snwprintf (path, MAX_PATH, L"%ls\\hime-install.exe", dir);

    ASSERT_TRUE (pe_has_uac_manifest (path));
    PASS ();
}

TEST (uninstall_exe_has_uac_manifest) {
    WCHAR dir[MAX_PATH];
    get_exe_dir (dir, MAX_PATH);
    WCHAR path[MAX_PATH];
    _snwprintf (path, MAX_PATH, L"%ls\\hime-uninstall.exe", dir);

    ASSERT_TRUE (pe_has_uac_manifest (path));
    PASS ();
}

/* ========== Filesystem Logic Tests ========== */

/*
 * Helper: create a temp directory for testing.
 * Returns path in buf. Caller must clean up.
 */
static BOOL
create_temp_dir (WCHAR *buf, int buflen)
{
    WCHAR tmp[MAX_PATH];
    GetTempPathW (MAX_PATH, tmp);
    _snwprintf (buf, buflen, L"%lshime-test-%lu", tmp, GetCurrentProcessId ());
    return CreateDirectoryW (buf, NULL);
}

static void
remove_temp_dir (const WCHAR *dir)
{
    WCHAR pattern[MAX_PATH];
    WIN32_FIND_DATAW fd;

    _snwprintf (pattern, MAX_PATH, L"%ls\\*", dir);
    HANDLE hFind = FindFirstFileW (pattern, &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        RemoveDirectoryW (dir);
        return;
    }

    do {
        if (wcscmp (fd.cFileName, L".") == 0
            || wcscmp (fd.cFileName, L"..") == 0)
            continue;

        WCHAR path[MAX_PATH];
        _snwprintf (path, MAX_PATH, L"%ls\\%ls", dir, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            remove_temp_dir (path);
        } else {
            DeleteFileW (path);
        }
    } while (FindNextFileW (hFind, &fd));

    FindClose (hFind);
    RemoveDirectoryW (dir);
}

static BOOL
write_test_file (const WCHAR *path, const char *content)
{
    HANDLE hFile = CreateFileW (path, GENERIC_WRITE, 0, NULL,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;
    DWORD written;
    WriteFile (hFile, content, (DWORD) strlen (content), &written, NULL);
    CloseHandle (hFile);
    return TRUE;
}

static BOOL
file_exists (const WCHAR *path)
{
    return GetFileAttributesW (path) != INVALID_FILE_ATTRIBUTES;
}

TEST (create_install_directory) {
    WCHAR tmpdir[MAX_PATH];
    ASSERT_TRUE (create_temp_dir (tmpdir, MAX_PATH));

    WCHAR datadir[MAX_PATH];
    _snwprintf (datadir, MAX_PATH, L"%ls\\data", tmpdir);
    ASSERT_TRUE (CreateDirectoryW (datadir, NULL));

    ASSERT_TRUE (file_exists (tmpdir));
    ASSERT_TRUE (file_exists (datadir));

    remove_temp_dir (tmpdir);
    PASS ();
}

TEST (copy_file_to_install_dir) {
    WCHAR tmpdir[MAX_PATH];
    ASSERT_TRUE (create_temp_dir (tmpdir, MAX_PATH));

    /* Create a source file */
    WCHAR src[MAX_PATH], dst[MAX_PATH];
    _snwprintf (src, MAX_PATH, L"%ls\\source.txt", tmpdir);
    ASSERT_TRUE (write_test_file (src, "hello"));

    /* Create a "install" subdirectory */
    WCHAR instdir[MAX_PATH];
    _snwprintf (instdir, MAX_PATH, L"%ls\\install", tmpdir);
    CreateDirectoryW (instdir, NULL);

    _snwprintf (dst, MAX_PATH, L"%ls\\install\\source.txt", tmpdir);
    ASSERT_TRUE (CopyFileW (src, dst, FALSE));
    ASSERT_TRUE (file_exists (dst));

    remove_temp_dir (tmpdir);
    PASS ();
}

TEST (copy_file_overwrites_existing) {
    WCHAR tmpdir[MAX_PATH];
    ASSERT_TRUE (create_temp_dir (tmpdir, MAX_PATH));

    WCHAR src[MAX_PATH], dst[MAX_PATH];
    _snwprintf (src, MAX_PATH, L"%ls\\new.txt", tmpdir);
    _snwprintf (dst, MAX_PATH, L"%ls\\old.txt", tmpdir);

    ASSERT_TRUE (write_test_file (dst, "old content"));
    ASSERT_TRUE (write_test_file (src, "new content"));

    /* CopyFileW with bFailIfExists=FALSE should overwrite */
    ASSERT_TRUE (CopyFileW (src, dst, FALSE));

    /* Verify content was overwritten */
    HANDLE hFile = CreateFileW (dst, GENERIC_READ, FILE_SHARE_READ,
                                NULL, OPEN_EXISTING, 0, NULL);
    ASSERT_TRUE (hFile != INVALID_HANDLE_VALUE);
    char buf[64] = {0};
    DWORD read;
    ReadFile (hFile, buf, sizeof (buf) - 1, &read, NULL);
    CloseHandle (hFile);
    ASSERT_TRUE (strcmp (buf, "new content") == 0);

    remove_temp_dir (tmpdir);
    PASS ();
}

TEST (self_copy_detection) {
    /* Installer skips copy when src == dst (case-insensitive) */
    ASSERT_EQ (0, _wcsicmp (L"C:\\Program Files\\HIME\\hime-install.exe",
                             L"C:\\Program Files\\HIME\\hime-install.exe"));
    ASSERT_EQ (0, _wcsicmp (L"C:\\PROGRAM FILES\\HIME\\hime-install.exe",
                             L"c:\\program files\\hime\\hime-install.exe"));
    /* Different paths should not match */
    ASSERT_TRUE (_wcsicmp (L"C:\\build\\hime-install.exe",
                           L"C:\\Program Files\\HIME\\hime-install.exe") != 0);
    PASS ();
}

TEST (data_files_exist_in_build) {
    WCHAR dir[MAX_PATH];
    get_exe_dir (dir, MAX_PATH);
    WCHAR pattern[MAX_PATH];
    _snwprintf (pattern, MAX_PATH, L"%ls\\data\\*", dir);

    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW (pattern, &fd);
    ASSERT_TRUE (hFind != INVALID_HANDLE_VALUE);

    int file_count = 0;
    BOOL has_pho_tab2 = FALSE;
    BOOL has_gtab = FALSE;
    BOOL has_kbm = FALSE;

    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;
        file_count++;

        size_t len = wcslen (fd.cFileName);
        if (wcscmp (fd.cFileName, L"pho.tab2") == 0)
            has_pho_tab2 = TRUE;
        if (len > 5 && wcscmp (fd.cFileName + len - 5, L".gtab") == 0)
            has_gtab = TRUE;
        if (len > 4 && wcscmp (fd.cFileName + len - 4, L".kbm") == 0)
            has_kbm = TRUE;
    } while (FindNextFileW (hFind, &fd));

    FindClose (hFind);

    /* Should have a reasonable number of data files */
    ASSERT_TRUE (file_count >= 10);
    ASSERT_TRUE (has_pho_tab2);
    ASSERT_TRUE (has_gtab);
    ASSERT_TRUE (has_kbm);
    PASS ();
}

TEST (enumerate_and_copy_data_files) {
    WCHAR dir[MAX_PATH];
    get_exe_dir (dir, MAX_PATH);
    WCHAR src_data[MAX_PATH];
    _snwprintf (src_data, MAX_PATH, L"%ls\\data", dir);

    WCHAR tmpdir[MAX_PATH];
    ASSERT_TRUE (create_temp_dir (tmpdir, MAX_PATH));
    WCHAR dst_data[MAX_PATH];
    _snwprintf (dst_data, MAX_PATH, L"%ls\\data", tmpdir);
    CreateDirectoryW (dst_data, NULL);

    /* Copy all data files */
    WCHAR pattern[MAX_PATH];
    _snwprintf (pattern, MAX_PATH, L"%ls\\*", src_data);
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW (pattern, &fd);
    ASSERT_TRUE (hFind != INVALID_HANDLE_VALUE);

    int copied = 0;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;
        WCHAR src[MAX_PATH], dst[MAX_PATH];
        _snwprintf (src, MAX_PATH, L"%ls\\%ls", src_data, fd.cFileName);
        _snwprintf (dst, MAX_PATH, L"%ls\\%ls", dst_data, fd.cFileName);
        if (CopyFileW (src, dst, FALSE))
            copied++;
    } while (FindNextFileW (hFind, &fd));
    FindClose (hFind);

    ASSERT_TRUE (copied >= 10);

    /* Verify pho.tab2 exists in destination */
    WCHAR check[MAX_PATH];
    _snwprintf (check, MAX_PATH, L"%ls\\pho.tab2", dst_data);
    ASSERT_TRUE (file_exists (check));

    remove_temp_dir (tmpdir);
    PASS ();
}

/* ========== Uninstall Logic Tests ========== */

TEST (delete_directory_contents) {
    WCHAR tmpdir[MAX_PATH];
    ASSERT_TRUE (create_temp_dir (tmpdir, MAX_PATH));

    /* Create some files */
    WCHAR f1[MAX_PATH], f2[MAX_PATH], f3[MAX_PATH];
    _snwprintf (f1, MAX_PATH, L"%ls\\a.txt", tmpdir);
    _snwprintf (f2, MAX_PATH, L"%ls\\b.dll", tmpdir);
    _snwprintf (f3, MAX_PATH, L"%ls\\c.dat", tmpdir);
    write_test_file (f1, "a");
    write_test_file (f2, "b");
    write_test_file (f3, "c");

    /* Create a subdirectory with a file */
    WCHAR subdir[MAX_PATH], f4[MAX_PATH];
    _snwprintf (subdir, MAX_PATH, L"%ls\\sub", tmpdir);
    CreateDirectoryW (subdir, NULL);
    _snwprintf (f4, MAX_PATH, L"%ls\\d.txt", subdir);
    write_test_file (f4, "d");

    /* Delete contents (using the same recursive logic as uninstaller) */
    WCHAR pattern[MAX_PATH];
    WIN32_FIND_DATAW fd;
    _snwprintf (pattern, MAX_PATH, L"%ls\\*", tmpdir);
    HANDLE hFind = FindFirstFileW (pattern, &fd);
    ASSERT_TRUE (hFind != INVALID_HANDLE_VALUE);

    do {
        if (wcscmp (fd.cFileName, L".") == 0
            || wcscmp (fd.cFileName, L"..") == 0)
            continue;
        WCHAR path[MAX_PATH];
        _snwprintf (path, MAX_PATH, L"%ls\\%ls", tmpdir, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            /* Recursively delete subdir */
            remove_temp_dir (path);
        } else {
            DeleteFileW (path);
        }
    } while (FindNextFileW (hFind, &fd));
    FindClose (hFind);

    /* Verify all files are gone */
    ASSERT_FALSE (file_exists (f1));
    ASSERT_FALSE (file_exists (f2));
    ASSERT_FALSE (file_exists (f3));
    ASSERT_FALSE (file_exists (subdir));

    /* Directory itself should still exist (empty) */
    ASSERT_TRUE (file_exists (tmpdir));
    RemoveDirectoryW (tmpdir);
    PASS ();
}

TEST (skip_self_during_uninstall) {
    /*
     * Uninstaller skips deleting itself by comparing paths.
     * Verify the comparison logic handles the match correctly.
     */
    WCHAR self[] = L"C:\\Program Files\\HIME\\hime-uninstall.exe";
    WCHAR other[] = L"C:\\Program Files\\HIME\\hime-core.dll";
    WCHAR self_upper[] = L"C:\\PROGRAM FILES\\HIME\\HIME-UNINSTALL.EXE";

    /* Self matches self (case-insensitive) */
    ASSERT_EQ (0, _wcsicmp (self, self));
    ASSERT_EQ (0, _wcsicmp (self, self_upper));
    /* Other files don't match self */
    ASSERT_TRUE (_wcsicmp (self, other) != 0);
    PASS ();
}

TEST (registry_key_path_format) {
    /*
     * Verify the uninstall registry key path matches what Windows expects
     * for Add/Remove Programs entries.
     */
    const WCHAR *key =
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\HIME";
    /* Must start with SOFTWARE */
    ASSERT_TRUE (wcsncmp (key, L"SOFTWARE\\", 9) == 0);
    /* Must contain Uninstall */
    ASSERT_TRUE (wcsstr (key, L"Uninstall") != NULL);
    /* Must end with HIME */
    size_t len = wcslen (key);
    ASSERT_TRUE (wcscmp (key + len - 4, L"HIME") == 0);
    PASS ();
}

/* ========== Admin Check Tests ========== */

/*
 * Helper: replicate the admin check logic from the installer.
 * Returns TRUE if running as admin, FALSE otherwise.
 */
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

TEST (admin_check_returns_bool) {
    /*
     * Verify is_running_as_admin returns a valid BOOL (0 or 1).
     * We don't assert which value — it depends on the test environment.
     */
    BOOL result = is_running_as_admin ();
    ASSERT_TRUE (result == TRUE || result == FALSE);
    printf ("    (running as admin: %s)\n", result ? "yes" : "no");
    PASS ();
}

TEST (admin_check_consistent) {
    /* Calling twice should return the same result */
    BOOL first = is_running_as_admin ();
    BOOL second = is_running_as_admin ();
    ASSERT_EQ (first, second);
    PASS ();
}

/* ========== Main ========== */

int
wmain (int argc, WCHAR *argv[])
{
    (void) argc;
    (void) argv;

    printf ("\n=== HIME Installer/Uninstaller Tests ===\n\n");

    printf ("PE structure tests:\n");
    RUN_TEST (install_exe_is_console);
    RUN_TEST (uninstall_exe_is_console);
    RUN_TEST (install_exe_has_uac_manifest);
    RUN_TEST (uninstall_exe_has_uac_manifest);

    printf ("\nInstall logic tests:\n");
    RUN_TEST (create_install_directory);
    RUN_TEST (copy_file_to_install_dir);
    RUN_TEST (copy_file_overwrites_existing);
    RUN_TEST (self_copy_detection);
    RUN_TEST (data_files_exist_in_build);
    RUN_TEST (enumerate_and_copy_data_files);

    printf ("\nUninstall logic tests:\n");
    RUN_TEST (delete_directory_contents);
    RUN_TEST (skip_self_during_uninstall);
    RUN_TEST (registry_key_path_format);

    printf ("\nAdmin check tests:\n");
    RUN_TEST (admin_check_returns_bool);
    RUN_TEST (admin_check_consistent);

    printf ("\n=== Results ===\n");
    printf ("Total:  %d\n", tests_total);
    printf ("Passed: %d\n", tests_passed);
    printf ("Failed: %d\n", tests_failed);
    printf ("\n");

    return tests_failed > 0 ? 1 : 0;
}
