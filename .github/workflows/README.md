# HIME GitHub Actions CI/CD

Automated build, test, and release workflows for all platforms.

## Workflows

### `build.yml` — Continuous Integration

Triggers on every push and pull request to `master`.

Builds all three platforms in parallel:

| Job | Runner | What it does |
|-----|--------|-------------|
| `linux` | `ubuntu-latest` | autoreconf + configure + make, runs 98 shared tests |
| `windows` | `ubuntu-latest` | Cross-compiles with MinGW-w64, uploads DLLs as artifacts |
| `macos` | `macos-latest` | Native build with CMake + Input Method Kit, uploads HIME.app |

Build artifacts are available for download from the Actions tab for 90 days.

### `release.yml` — Release Publishing

Triggers when a version tag is pushed.

Builds all platforms, packages them, and creates a GitHub Release with:

| Platform | Artifact | Contents |
|----------|----------|----------|
| Linux | `hime-VERSION-linux-x86_64.tar.gz` | Installed files in /usr tree |
| Windows | `hime-VERSION-windows-x86_64.zip` | DLLs, data, register/unregister scripts |
| macOS | `hime-VERSION-macos.dmg` | HIME.app (ad-hoc code signed) |

## Creating a Release

```bash
# Tag the release
git tag v0.10.1
git push origin v0.10.1
```

The workflow will:
1. Build all three platforms in parallel
2. Run tests (Linux)
3. Package platform-specific archives
4. Create a GitHub Release with release notes auto-generated from commits
5. Attach all platform archives to the release

## macOS Build Details

The macOS build runs on GitHub's `macos-latest` runner which includes:
- Xcode (with Input Method Kit framework)
- CMake
- Code signing tools

### Code Signing

The release workflow uses **ad-hoc signing** (`codesign --sign -`), which is
sufficient for local installation but will trigger Gatekeeper warnings on
other users' machines.

For proper distribution, set up Apple Developer ID signing:

1. Enroll in the [Apple Developer Program](https://developer.apple.com/programs/) ($99/year)
2. Create a Developer ID Application certificate
3. Export the certificate as a .p12 file
4. Add these GitHub repository secrets:
   - `APPLE_CERTIFICATE` — base64-encoded .p12 file
   - `APPLE_CERTIFICATE_PASSWORD` — .p12 password
   - `APPLE_TEAM_ID` — your Apple Team ID
5. Update the release workflow to import the certificate and sign:

```yaml
- name: Import certificate
  env:
    CERTIFICATE: ${{ secrets.APPLE_CERTIFICATE }}
    PASSWORD: ${{ secrets.APPLE_CERTIFICATE_PASSWORD }}
  run: |
    echo "$CERTIFICATE" | base64 --decode > cert.p12
    security create-keychain -p "" build.keychain
    security import cert.p12 -k build.keychain -P "$PASSWORD" -T /usr/bin/codesign
    security set-key-partition-list -S apple-tool:,apple: -s -k "" build.keychain

- name: Code sign
  run: |
    codesign --force --deep \
      --sign "Developer ID Application: Your Name (${{ secrets.APPLE_TEAM_ID }})" \
      platform/macos/build/HIME.app
```

For notarization (required for macOS 10.15+), add a step after signing:

```yaml
- name: Notarize
  env:
    APPLE_ID: ${{ secrets.APPLE_ID }}
    APPLE_PASSWORD: ${{ secrets.APPLE_APP_PASSWORD }}
    TEAM_ID: ${{ secrets.APPLE_TEAM_ID }}
  run: |
    xcrun notarytool submit platform/macos/build/hime-*.dmg \
      --apple-id "$APPLE_ID" \
      --password "$APPLE_PASSWORD" \
      --team-id "$TEAM_ID" \
      --wait
    xcrun stapler staple platform/macos/build/hime-*.dmg
```

## Windows Build Details

Windows binaries are cross-compiled from Linux using MinGW-w64. No Windows
runner is needed. The zip package includes:

```
hime-windows/
├── bin/
│   ├── hime-core.dll
│   ├── hime-tsf.dll
│   ├── test-hime-core.exe
│   └── data/
│       ├── pho.tab2
│       └── *.kbm
├── register.bat
├── unregister.bat
└── README.txt
```

## Troubleshooting

- **macOS build fails**: Check that `platform/macos/CMakeLists.txt` doesn't reference
  Linux-only paths. The macOS runner has a different SDK path.
- **Windows cross-compile fails**: Ensure `mingw-w64` is installed. The
  toolchain file `mingw-w64-x86_64.cmake` must be present.
- **Tests fail**: Check the shared test output in the Actions log. All 98
  tests must pass before release artifacts are created.
- **Release not created**: Ensure the tag matches the pattern `v*` (e.g.,
  `v0.10.1`). The `release.yml` workflow only triggers on tags.
