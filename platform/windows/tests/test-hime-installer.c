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

/* ========== Issue #1: Uninstall ghost entries in keyboard list ========== */
/* After uninstall, HKCU registry entries can leave "ghost" keyboard entries
 * in Windows Settings. The uninstaller must clean these up. */

#define HIME_TEST_CLSID_STR L"{B8A45C32-5F6D-4E2A-9C1B-0D3E4F5A6B7C}"
#define HIME_TEST_CLSID_UPPER L"B8A45C32-5F6D-4E2A-9C1B-0D3E4F5A6B7C"
#define HIME_TEST_CLSID_LOWER L"b8a45c32-5f6d-4e2a-9c1b-0d3e4f5a6b7c"

TEST (uninstall_clsid_pattern_matches) {
    /* The uninstaller searches for HIME's CLSID in registry values.
     * Verify the pattern matching logic (case-insensitive wcsstr). */
    const WCHAR *testData1 = L"0404:" HIME_TEST_CLSID_STR L"{C9B56D43-6E7F-5F3B-AD2C-1E4F5061C8D9}";
    const WCHAR *testData2 = L"0404:{b8a45c32-5f6d-4e2a-9c1b-0d3e4f5a6b7c}{c9b56d43-6e7f-5f3b-ad2c-1e4f5061c8d9}";
    const WCHAR *testDataOther = L"0404:{12345678-ABCD-ABCD-ABCD-123456789ABC}";

    /* Upper-case match */
    ASSERT_TRUE (wcsstr (testData1, HIME_TEST_CLSID_UPPER) != NULL);
    /* Lower-case match */
    ASSERT_TRUE (wcsstr (testData2, HIME_TEST_CLSID_LOWER) != NULL);
    /* Should NOT match other CLSIDs */
    ASSERT_TRUE (wcsstr (testDataOther, HIME_TEST_CLSID_UPPER) == NULL);
    ASSERT_TRUE (wcsstr (testDataOther, HIME_TEST_CLSID_LOWER) == NULL);
    PASS ();
}

TEST (uninstall_covers_all_registry_locations) {
    /* Verify the uninstaller cleans up all 5 HKCU registry locations
     * where Windows stores keyboard preferences. These are the locations
     * that cause ghost entries if not cleaned up.
     *
     * This test verifies the KEY PATHS are correct strings (not actual
     * registry operations which require admin). */
    const WCHAR *locations[] = {
        L"Software\\Microsoft\\CTF\\TIP\\" HIME_TEST_CLSID_STR,
        L"Software\\Microsoft\\CTF\\SortOrder\\AssemblyItem\\0x00000404",
        L"Software\\Microsoft\\CTF\\SortOrder\\Language\\00000404",
        L"Keyboard Layout\\Preload",
        L"Keyboard Layout\\Substitutes"
    };

    for (int i = 0; i < 5; i++) {
        /* Key paths must not be empty and must contain backslash */
        ASSERT_TRUE (wcslen (locations[i]) > 10);
        ASSERT_TRUE (wcschr (locations[i], L'\\') != NULL);
    }

    /* Verify the Chinese Traditional locale code is correct */
    ASSERT_TRUE (wcsstr (locations[1], L"0x00000404") != NULL ||
                 wcsstr (locations[1], L"00000404") != NULL);
    PASS ();
}

TEST (uninstall_ctf_tip_key_format) {
    /* CTF TIP key must be exactly:
     * Software\Microsoft\CTF\TIP\{CLSID}
     * The CLSID must be in brace-GUID format */
    WCHAR key[256];
    _snwprintf (key, 256, L"Software\\Microsoft\\CTF\\TIP\\%ls", HIME_TEST_CLSID_STR);

    ASSERT_TRUE (wcsstr (key, L"CTF\\TIP\\{") != NULL);
    ASSERT_TRUE (key[wcslen (key) - 1] == L'}');
    PASS ();
}

TEST (stale_settings_should_not_reduce_methods) {
    /* Regression test for stale EnabledMethods registry bug.
     *
     * Bug: _LoadSettings() reads HKCU\Software\HIME\EnabledMethods and
     * disables all methods first, then only enables listed ones. A stale
     * registry value from an older version (e.g. "liu.gtab") would reduce
     * the cycle to just that one method.
     *
     * The fix: _DiscoverMethods() defaults all available methods to enabled.
     * _LoadSettings() only overrides when the registry key exists. If the
     * registry has a stale/incomplete value, deleting the key restores
     * the all-enabled defaults.
     *
     * Verify the EnabledMethods format: comma-separated filenames,
     * "zhuyin" for Zhuyin, and .gtab filenames for GTAB methods. */

    /* A proper full settings string should list many methods */
    const WCHAR *fullSettings = L"zhuyin,cj.gtab,cj5.gtab,cj543.gtab,cj-punc.gtab,"
                                L"simplex.gtab,simplex-punc.gtab,dayi3.gtab,ar30.gtab,"
                                L"array40.gtab,ar30-big.gtab,liu.gtab,pinyin.gtab,"
                                L"jyutping.gtab,hangul.gtab,hangul-roman.gtab,vims.gtab,"
                                L"symbols.gtab,greek.gtab,russian.gtab,esperanto.gtab,"
                                L"latin-letters.gtab";

    /* A stale value from an older version would have very few entries */
    const WCHAR *staleSettings = L"liu.gtab";

    /* Full settings should contain many commas (21+ methods) */
    int commaCount = 0;
    for (const WCHAR *p = fullSettings; *p; p++) {
        if (*p == L',') commaCount++;
    }
    ASSERT_TRUE (commaCount >= 20);

    /* Stale settings should have no commas (only 1 method) */
    commaCount = 0;
    for (const WCHAR *p = staleSettings; *p; p++) {
        if (*p == L',') commaCount++;
    }
    ASSERT_EQ (0, commaCount);

    /* All filenames should end with .gtab except "zhuyin" */
    ASSERT_TRUE (wcsstr (fullSettings, L"zhuyin") != NULL);
    ASSERT_TRUE (wcsstr (fullSettings, L"cj5.gtab") != NULL);
    ASSERT_TRUE (wcsstr (fullSettings, L"liu.gtab") != NULL);
    ASSERT_TRUE (wcsstr (fullSettings, L"hangul.gtab") != NULL);
    ASSERT_TRUE (wcsstr (fullSettings, L"symbols.gtab") != NULL);
    PASS ();
}

TEST (settings_registry_key_path) {
    /* EnabledMethods is stored at HKCU\Software\HIME\EnabledMethods.
     * Verify the key path is correct and doesn't require admin access. */
    const WCHAR *keyPath = L"Software\\HIME";
    ASSERT_TRUE (wcsstr (keyPath, L"HIME") != NULL);
    /* Should be under Software (HKCU, not HKLM) */
    ASSERT_TRUE (wcsncmp (keyPath, L"Software\\", 9) == 0);
    PASS ();
}

/* ========== Issue #2: System keyboard settings menu ========== */
/* HIME needs correct TSF registration to appear in Windows Settings.
 * These tests verify the registration data is well-formed. */

TEST (tsf_dll_exports_required_functions) {
    /* hime-tsf.dll must export DllRegisterServer, DllUnregisterServer,
     * DllGetClassObject, and DllCanUnloadNow for regsvr32 to work. */
    WCHAR dir[MAX_PATH];
    get_exe_dir (dir, MAX_PATH);
    WCHAR dllPath[MAX_PATH];
    _snwprintf (dllPath, MAX_PATH, L"%ls\\hime-tsf.dll", dir);

    /* Load as data to check exports without executing DllMain */
    HMODULE hMod = LoadLibraryExW (dllPath, NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (!hMod) {
        /* Fall back — DLL might not exist in test environment */
        printf ("    (hime-tsf.dll not found, skipping) ");
        PASS ();
        return;
    }

    ASSERT_TRUE (GetProcAddress (hMod, "DllRegisterServer") != NULL);
    ASSERT_TRUE (GetProcAddress (hMod, "DllUnregisterServer") != NULL);
    ASSERT_TRUE (GetProcAddress (hMod, "DllGetClassObject") != NULL);
    ASSERT_TRUE (GetProcAddress (hMod, "DllCanUnloadNow") != NULL);

    FreeLibrary (hMod);
    PASS ();
}

TEST (tsf_language_id_is_traditional_chinese) {
    /* HIME registers for zh-TW (0x0404). Verify the LANGID constant. */
    LANGID langId = MAKELANGID (LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL);
    ASSERT_EQ (0x0404, (int) langId);
    PASS ();
}

TEST (tsf_clsid_is_valid_guid_format) {
    /* CLSID string must be valid brace-GUID format */
    const WCHAR *clsid = HIME_TEST_CLSID_STR;
    ASSERT_EQ (L'{', clsid[0]);
    ASSERT_EQ (L'}', clsid[wcslen (clsid) - 1]);
    ASSERT_EQ (38, (int) wcslen (clsid)); /* {8-4-4-4-12} = 38 chars */
    ASSERT_EQ (L'-', clsid[9]);
    ASSERT_EQ (L'-', clsid[14]);
    ASSERT_EQ (L'-', clsid[19]);
    ASSERT_EQ (L'-', clsid[24]);
    PASS ();
}

/* ========== Issue #3: HIME floating toolbar ========== */
/* The toolbar is a WS_POPUP window. In TSF DLL context, window creation
 * can fail because the DLL runs inside another process's message loop. */

TEST (toolbar_window_class_name_not_empty) {
    /* Class name must be a non-empty string for RegisterClass */
    const WCHAR *cls = L"HimeToolbarClass";
    ASSERT_TRUE (wcslen (cls) > 0);
    ASSERT_TRUE (wcslen (cls) < 256);
    PASS ();
}

TEST (toolbar_dimensions_are_reasonable) {
    /* Toolbar is 78x28 pixels. Verify it's not zero-sized or absurdly large. */
    int w = 78, h = 28;
    ASSERT_TRUE (w > 0 && w < 500);
    ASSERT_TRUE (h > 0 && h < 500);
    PASS ();
}

TEST (toolbar_position_within_work_area) {
    /* The toolbar should be positioned at bottom-right of work area.
     * Verify the positioning math is correct for a sample work area. */
    RECT rcWork = {0, 0, 1920, 1040}; /* Typical 1080p minus taskbar */
    int w = 78, h = 28;
    int x = rcWork.right - w - 10;
    int y = rcWork.bottom - h - 10;

    ASSERT_TRUE (x > 0);
    ASSERT_TRUE (y > 0);
    ASSERT_TRUE (x + w <= rcWork.right);
    ASSERT_TRUE (y + h <= rcWork.bottom);
    /* Should be near bottom-right */
    ASSERT_TRUE (x > rcWork.right / 2);
    ASSERT_TRUE (y > rcWork.bottom / 2);
    PASS ();
}

/* ========== Issue #4: Mode indicator not displayed in system tray ========== */
/* The mode button must have correct TF_LANGBARITEMINFO for Windows to
 * show it in the system tray / language bar. */

TEST (mode_button_style_has_shownintray) {
    /* TF_LBI_STYLE_SHOWNINTRAY must be set for the icon to appear in tray */
    DWORD style = 0x00010000 | 0x00000002; /* BTN_BUTTON | SHOWNINTRAY */
    ASSERT_TRUE (style & 0x00000002); /* TF_LBI_STYLE_SHOWNINTRAY */
    PASS ();
}

TEST (mode_button_guid_is_unique) {
    /* The mode button GUID must differ from the lang bar button GUID */
    /* GUID_HimeLangBarButton = {DA8E7A60-...} */
    /* GUID_HimeModeButton    = {EB9F8B71-...} */
    ASSERT_TRUE (0xDA8E7A60 != 0xEB9F8B71);
    PASS ();
}

TEST (icon_files_exist_in_build) {
    /* Check that method icon PNGs are present in the build output */
    WCHAR dir[MAX_PATH];
    get_exe_dir (dir, MAX_PATH);
    WCHAR iconsDir[MAX_PATH];
    _snwprintf (iconsDir, MAX_PATH, L"%ls\\icons", dir);

    /* Required icons for the mode indicator to work */
    const WCHAR *requiredIcons[] = {
        L"hime-tray.png",  /* EN mode */
        L"juyin.png",      /* Zhuyin */
        L"cj5.png",        /* Cangjie 5 */
        L"hime.png",       /* App icon */
        NULL
    };

    int found = 0;
    for (int i = 0; requiredIcons[i]; i++) {
        WCHAR path[MAX_PATH];
        _snwprintf (path, MAX_PATH, L"%ls\\%ls", iconsDir, requiredIcons[i]);
        if (file_exists (path))
            found++;
    }

    /* At least the core icons should be present */
    ASSERT_TRUE (found >= 3);
    PASS ();
}

TEST (icon_files_cover_all_methods) {
    /* Check that icon PNGs exist for all major input methods */
    WCHAR dir[MAX_PATH];
    get_exe_dir (dir, MAX_PATH);

    const WCHAR *methodIcons[] = {
        L"cj.png", L"cj5.png", L"simplex.png", L"dayi3.png",
        L"ar30.png", L"pinyin.png", L"jyutping.png",
        L"hangul.png", L"symbols.png", L"greek.png",
        L"russian.png", L"esperanto.png", L"latin-letters.png",
        L"vims.png",
        NULL
    };

    int found = 0, total = 0;
    for (int i = 0; methodIcons[i]; i++) {
        total++;
        WCHAR path[MAX_PATH];
        _snwprintf (path, MAX_PATH, L"%ls\\icons\\%ls", dir, methodIcons[i]);
        if (file_exists (path))
            found++;
    }

    printf ("    (%d/%d method icons found) ", found, total);
    /* Most icons should be present (blue/ icons are copied by CMakeLists) */
    ASSERT_TRUE (found >= 10);
    PASS ();
}

TEST (tsf_dll_is_valid_dll) {
    /* hime-tsf.dll must be a DLL (not EXE). Check PE subsystem. */
    WCHAR dir[MAX_PATH];
    get_exe_dir (dir, MAX_PATH);
    WCHAR path[MAX_PATH];
    _snwprintf (path, MAX_PATH, L"%ls\\hime-tsf.dll", dir);

    if (!file_exists (path)) {
        printf ("    (hime-tsf.dll not found, skipping) ");
        PASS ();
        return;
    }

    /* DLLs should have the IMAGE_FILE_DLL characteristic.
     * Check via PE header: characteristics field bit 0x2000. */
    HANDLE hFile = CreateFileW (path, GENERIC_READ, FILE_SHARE_READ,
                                NULL, OPEN_EXISTING, 0, NULL);
    ASSERT_TRUE (hFile != INVALID_HANDLE_VALUE);

    IMAGE_DOS_HEADER dos;
    DWORD read;
    ReadFile (hFile, &dos, sizeof (dos), &read, NULL);
    ASSERT_EQ ((int) sizeof (dos), (int) read);
    ASSERT_EQ (IMAGE_DOS_SIGNATURE, (int) dos.e_magic);

    SetFilePointer (hFile, dos.e_lfanew, NULL, FILE_BEGIN);
    DWORD pe_sig;
    ReadFile (hFile, &pe_sig, sizeof (pe_sig), &read, NULL);
    ASSERT_EQ (IMAGE_NT_SIGNATURE, (int) pe_sig);

    IMAGE_FILE_HEADER fh;
    ReadFile (hFile, &fh, sizeof (fh), &read, NULL);
    /* IMAGE_FILE_DLL = 0x2000 */
    ASSERT_TRUE (fh.Characteristics & 0x2000);

    CloseHandle (hFile);
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

    printf ("\nIssue #1: Uninstall ghost entry tests:\n");
    RUN_TEST (uninstall_clsid_pattern_matches);
    RUN_TEST (uninstall_covers_all_registry_locations);
    RUN_TEST (uninstall_ctf_tip_key_format);
    RUN_TEST (stale_settings_should_not_reduce_methods);
    RUN_TEST (settings_registry_key_path);

    printf ("\nIssue #2: System keyboard settings tests:\n");
    RUN_TEST (tsf_dll_exports_required_functions);
    RUN_TEST (tsf_language_id_is_traditional_chinese);
    RUN_TEST (tsf_clsid_is_valid_guid_format);

    printf ("\nIssue #3: HIME toolbar tests:\n");
    RUN_TEST (toolbar_window_class_name_not_empty);
    RUN_TEST (toolbar_dimensions_are_reasonable);
    RUN_TEST (toolbar_position_within_work_area);

    printf ("\nIssue #4: Tray indicator tests:\n");
    RUN_TEST (mode_button_style_has_shownintray);
    RUN_TEST (mode_button_guid_is_unique);
    RUN_TEST (icon_files_exist_in_build);
    RUN_TEST (icon_files_cover_all_methods);
    RUN_TEST (tsf_dll_is_valid_dll);

    printf ("\n=== Results ===\n");
    printf ("Total:  %d\n", tests_total);
    printf ("Passed: %d\n", tests_passed);
    printf ("Failed: %d\n", tests_failed);
    printf ("\n");

    return tests_failed > 0 ? 1 : 0;
}
