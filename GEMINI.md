# GEMINI.md

Project-specific guidance for AI agents working in the GXDE Display Manager
(`gxde-display-manager`) repository. Read this together with `README.md`,
`INSTALL.md`, and the files under `docs/`.

## 1. Project Overview

GXDE Display Manager (`GXDM`) is a fork of SDDM maintained for GXDE OS. It is a
complete display-manager stack, not just a greeter:

| Component | Binary / target | Source | Purpose |
| --- | --- | --- | --- |
| Daemon | `gxdm` | `src/daemon` | Owns seats, displays, sessions and power management; starts greeters; integrates with logind/elogind; exposes D-Bus APIs. |
| New greeter | `gxdm-greeter-neo` | `src/greeter-neo` | Default QML/Qt Quick greeter with login transitions and multi-screen support. |
| Classic greeter | `gxdm-greeter-classic` | `src/greeter-classic` | GXDE/DTK2Widget greeter inherited from DDE `session-ui`. |
| Classic lock screen | `gxdm-lock-classic` | `src/greeter-classic/lock` | In-session DTK/QWidget lock screen; reuses GXDM's PAM authenticator and is the stable autostart/D-Bus entry point. |
| QML lock screen | `gxdm-lock-neo` | `src/lock-neo` | Qt Quick lock screen backed by `src/lock-common`; selected by `[General] LockScreenStyle`. |
| Generic greeter | `gxdm-greeter-qt6` | `src/greeter` | SDDM-style Qt Quick greeter used for themes and compatibility. |
| Auth helper | `gxdm-helper` | `src/helper` | PAM authentication and session setup over a local socket. |
| Display helpers | `gxdm-helper-start-wayland`, `gxdm-helper-start-x11user` | `src/helper` | Start compositors/X servers and prepare the user session. |

Key facts:

- **Languages:** C++ (C++20 under CMake; `greeter-neo` uses C++17; Bazel
  defaults to C++17 with per-target C++20 overrides), QML, CMake, Bazel, shell.
- **UI stack:** Qt 6, Qt Quick/QML, DTK2Widget/DTK6, LayerShellQt,
  GSettings-Qt6.
- **Platform:** Linux with systemd/logind or elogind; FreeBSD-specific PAM and
  build branches also exist.
- **Upstream heritage:** the C++ namespace is `SDDM`, the CMake project is
  still named `SDDM`, and several files/identifiers retain `sddm` names. Do not
  rename these casually; upstream compatibility and existing configuration
  depend on them.
- **License:** GXDE code is GPL-3.0-or-later. Upstream SDDM files may be
  GPL-2.0-or-later. QML assets are MIT; images are CC-BY-3.0. Preserve all
  existing copyright/license headers and add a GPL-3.0-or-later header to new
  files.

## 2. Repository Map

| Path | Contents |
| --- | --- |
| `src/daemon` | `DaemonApp`, `DisplayManager`, `Seat`, `Display`, `Greeter`, `DisplayServer` (X11/Wayland), `PowerManager`, socket server, logind types. |
| `src/auth` | Shared authentication protocol: `Auth`, `AuthRequest`, `AuthPrompt`, `AuthMessages`. Used by the daemon, helper and lock screen. |
| `src/common` | Config parsing (`ConfigReader`, `Configuration`), session/theme metadata, `Constants.h.in`, `SafeDataStream`, `XAuth`, `VirtualTerminal`, greeter appearance/cursor/scale, OS defaults. |
| `src/greeter` | Generic SDDM-derived Qt Quick greeter (`gxdm-greeter-qt6`). |
| `src/greeter-classic` | DTK2Widget greeter and in-session lock screen. `lock/` contains the locker; `vendor/` contains imported DDE code. |
| `src/lock-common` | QML-facing lock backend (`LockBackend`, `UserListModel`), style resolution (`LockStyleResolver`) and the shared `gxdm-lock-backend`/`gxdm-lock-common` static libraries. |
| `src/lock-neo` | QML lock screen (`gxdm-lock-neo`): `src/main.cc`, `qml/LockWindow.qml`, `qml/LockScreen.qml`, and lock-specific components. |
| `src/greeter-neo` | New QML greeter: `qml/MainWindow.qml`, C++ backend, session list, screen manager, login transition, and gated tests. |
| `src/helper` | PAM helper, Wayland/X11 user-session helpers, `backend/PamBackend.cpp`. |
| `components` | Reusable QML components installed as `GxdmComponents` (`2.0/` and `common/`). |
| `data` | Themes, flags, faces, session scripts, translations, D-Bus interface XML, D-Bus policy/service files, `configs/OMG.conf`. |
| `services` | systemd unit, sysusers, tmpfiles and PAM configuration files. |
| `test` | QtTest/CTest suite for configuration, sessions, auth transitions, greeter settings, scaling, virtual keyboard, multi-screen, and Wayland cleanup. |
| `debian`, `arch` | Distribution packaging. |
| `docs` | Architecture, theming, and login-transition notes. |
| `build/` | Local, git-ignored CMake artifact directory. Never edit or commit it. |

`README.original.md` and `docs/THEMING.md` are inherited from upstream SDDM and
may use `sddm-*` binary names. Prefer the actual GXDM target names in
`README.md` and the CMake files when writing instructions or code.

## 3. Runtime Architecture

### Process model

1. `gxdm` starts as the display-manager daemon (normally via
   `gxdm.service`).
2. It creates a seat and display, starts an X11 or Wayland display server, then
   launches a greeter with a local socket.
3. The greeter authenticates through the daemon, which delegates to
   `gxdm-helper` over a socket. `gxdm-helper` performs PAM authentication and
   prepares the session.
4. After successful authentication, the daemon starts the selected session
   (X11 or Wayland) and manages the VT/seat lifecycle.
5. Power actions, session selection, user lists and greeter appearance are
   exposed through D-Bus and greeter models.

### Display servers and greeter selection

- `SetGreeterDisplayServer` accepts `wayland`, `x11`, or `x11-user`.
- The Wayland greeter prefers `gxde-wlcom`, then falls back to `LabWC`, `Sway`
  or `Weston`.
- `[General] EnableNeoGreeter` defaults to `true`. When the neo binary is
  missing, the daemon falls back to the classic greeter.
- Qt Virtual Keyboard requires both its QML module and input-context plugin.
- Lock screen frontends: `gxdm-lock-classic` remains the autostart/D-Bus
  entry point. When `[General] LockScreenStyle` (or OMG.conf's
  `SwitchLkScrStyle`) resolves to `modern` (the default), it execs `gxdm-lock-neo` with the
  same arguments. A missing QML binary falls back to the classic locker.
- Runtime style switch: `SetLockScreenStyle` (session `top.gxde.DisplayManager`
  forwards to `top.gxde.DisplayManager.System`) persists the key as root and
  emits `LockScreenStyleChanged`. Each locker's `LockStyleManager` re-resolves
  the style and, once the screen is hidden, the locker releases its D-Bus names
  (and neo's instance lock) and execs the other binary with `--daemon`.
- `gxdm-lock-neo` uses `ext-session-lock-v1` on Wayland when available, then
  LayerShellQt; on X11 it uses full-screen windows and grabs the keyboard.

### D-Bus

- **Public/session bus:** service `top.gxde.DisplayManager`, object path
  `/top/gxde/DisplayManager`, interface `top.gxde.DisplayManager`. This is the
  API regular applications should use.
- **Internal/system bus:** the daemon owns the same service name and exposes
  `top.gxde.DisplayManager.System`. Treat this as an implementation detail; do
  not call it from regular applications.
- Interface definitions live in `data/interfaces/*.xml`; policy and service
  activation files live in `data/` (`top.gxde.DisplayManager.conf`,
  `top.gxde.DisplayManager.service.in`).
- Global appearance state is persisted in `~gxdm/state.conf`; the greeter
  display server is written to `/etc/gxdm.conf`. User lock-screen wallpaper
  overrides are stored under `~/.local/share/gxdm/` and copied to `~gxdm/` for
  the greeter.
- The session-bus API includes lock-shortcut enrollment (`TryEnrollLkScr`,
  `UnregisterLkScr`, `LkScrStat`), greeter/lock appearance (`SetCursor`,
  `SetWallpaper*`, `SetLockWallpaperOverride`,
  `ClearLockWallpaperOverride`), display-server selection, and
  `SetNeoGreeterEnabled`/`NeoGreeterEnabled`. See `README.md` for the full table
  and `busctl` examples.

### Configuration and runtime paths

- Main config: `/etc/gxdm.conf`
- Drop-in config: `/etc/gxdm.conf.d/`
- System config: `/usr/lib/gxdm/gxdm.conf.d/`
- GXDE/Oh-My-GXDE integration: `/etc/gxdm/OMG.conf` (required by third-party
  packaging)
- State: `/var/lib/gxdm/`
- Runtime: `/run/gxdm/` when systemd is present, otherwise
  `/var/run/gxdm/`
- Greeter home/user: `gxdm`
- Session scripts: `<data-dir>/gxdm/scripts/Xsession` and
  `<data-dir>/gxdm/scripts/wayland-session`

These values are generated from `src/common/Constants.h.in`; do not hardcode
paths in new code when a constant or `Configuration` accessor exists.

### Login transition (neo greeter)

`docs/LOGIN_TRANSITION.md` describes the post-authentication transition in
Chinese. Key points for maintainers:

- `Display::beginTransition()` chooses the transition mode and pauses the auth
  helper; `Display::completeTransition()` resumes session startup.
- `MainWindow.qml` handles `handlePrepareTransition()` and
  `handleLoginFailed()`.
- `LoginTransition` waits for frame submission; `Start()`, `FrameSwapped()`,
  `Complete()` and `Cancel()` manage its state.
- The greeter waits at most 300 ms for the final frame; the daemon waits at most
  800 ms for the whole transition. Timeouts must continue login rather than
  block authentication.
- Shared-X11 X11→X11 keeps the wallpaper; most other transitions fade to black.
- Do not restart a real display manager on a daily-use desktop to test this.
  Use a VM or dedicated test machine.

## 4. Building

### Prerequisites

Install the dependencies listed in `debian/control`. At minimum:

- A C++20-capable compiler and CMake (the top level says 3.16, but
  `src/greeter-neo` and `debian/control` require 3.21; treat 3.21 as the
  effective minimum).
- Qt 6: Core, DBus, Gui, Widgets, Network, Qml, Quick, Svg, Xml, WaylandClient,
  Test, QuickTest, LinguistTools.
- DTK2Widget-Qt6, DTK6 Core, DTK6 DLog.
- GSettings-Qt6, LayerShellQt, PAM, systemd, X11/XCB/XKB/Xrandr/Xcursor/Xfixes/
  XInput/XTest, wayland-protocols.
- For packaging: debhelper/dpkg-buildpackage or Arch `makepkg`.

### CMake (primary development build)

```sh
cmake -S . -B build-cmake \
    -DBUILD_WITH_QT6=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build-cmake --parallel
```

Stage and inspect an install before touching the host:

```sh
DESTDIR="$PWD/build-cmake/stage" cmake --install build-cmake
sudo cmake --install build-cmake   # only after inspecting the stage
```

Useful options:

| Option | Default | Effect |
| --- | --- | --- |
| `BUILD_WITH_QT6` | `ON` | Qt 6 build. Qt 5 is legacy and only allowed when both GXDE greeters are disabled. |
| `BUILD_NEO_GREETER` | `ON` | Build `gxdm-greeter-neo`. Requires Qt 6. |
| `BUILD_CLASSIC_GREETER` | `ON` | Build `gxdm-greeter-classic` and `gxdm-lock-classic`. Requires Qt 6. |
| `BUILD_NEO_LOCK` | `ON` | Build `gxdm-lock-neo` and the shared lock backend. Requires `BUILD_CLASSIC_GREETER=ON`. |
| `LOCK_BUILD_TESTS` | `OFF` | Build `LockBackend` and `LockStyle` CTest cases. |
| `BUILD_MAN_PAGES` | `OFF` | Build man pages (requires `rst2man`). |
| `ENABLE_JOURNALD` | `ON` | Enable journald logging when libsystemd is available. |
| `NO_SYSTEMD` | `OFF` | Disable systemd support. |
| `USE_ELOGIND` | `OFF` | Use elogind instead of logind. |
| `INSTALL_PAM_CONFIGURATION` | `ON` | Install platform-appropriate PAM files. |
| `GREETER_BUILD_TESTS` | `OFF` | Build the neo-greeter backend/QML tests (defined in `src/greeter-neo/CMakeLists.txt`). |

The root build adds `-Wall -Wextra` and the Qt porting guards
`QT_NO_CAST_FROM_ASCII`, `QT_NO_CAST_FROM_BYTEARRAY` and `QT_NO_FOREACH`.
`greeter-classic` removes those guards for legacy DTK code; `greeter-neo`
removes only the ASCII-cast guards. New code should use `QStringLiteral`,
`QLatin1String`, and range-based `for`.

`build/` is a local artifact directory ignored by `.gitignore` (`build/*`).
Use `build-cmake/` or another fresh directory for new work; never commit build
output.

### Bazel (deployment tarball)

Bazel is a secondary build used to produce a deployable archive. It uses
Bzlmod, Bazel 9.1.1 (`.bazelversion`), and **host** Qt 6/DTK repositories whose
paths are hardcoded for x86_64 in `MODULE.bazel`:

```sh
bazel build --spawn_strategy=local //:gxdm-dist
# output: bazel-bin/gxdm-dist.tar
```

`.bazelrc` already sets `--spawn_strategy=local` and a default of `-std=c++17`;
some targets (for example `src/greeter-classic`) explicitly add `-std=c++20`.
The Bazel path is host-specific, so use CMake for normal development and
testing. If you add a source file, update both the relevant `CMakeLists.txt`
and `BUILD.bazel` so the two build systems stay in sync.

### Packaging

**Debian/Ubuntu:**

```sh
chmod a+x ./build-deb
./build-deb -d    # install build deps from debian/control, then build
./build-deb -c    # clean build artifacts and generated .deb files
```

`debian/rules` configures CMake with Qt 6, both greeters, PAM installation, and
fixed `UID_MIN`/`UID_MAX`. The binary package is `gxdm`.

**Arch Linux:**

```sh
cd arch
makepkg -si
```

The `arch/PKGBUILD` builds from the current git checkout, installs helpers to
`/usr/lib/gxdm`, and installs `data/configs/OMG.conf` to `/etc/gxdm/`.
`./update-pkgver` reads the version from `debian/changelog` and updates
`arch/PKGBUILD`.

**Other distributions:** ensure `data/configs/OMG.conf` is installed to
`/etc/gxdm/`; this is an explicit packaging requirement.

## 5. Testing

Tests are QtTest executables registered with CTest. Build them with CMake, then
run:

```sh
cmake -S . -B build-cmake -DBUILD_WITH_QT6=ON -DBUILD_CLASSIC_GREETER=ON
cmake --build build-cmake --parallel
ctest --test-dir build-cmake --output-on-failure
```

Test inventory (`test/CMakeLists.txt`):

| CTest name | Binary | Notes |
| --- | --- | --- |
| `WaylandCleanup` | `WaylandCleanupTest` | Linux only; 90 s timeout; depends on `gxdm-helper-start-wayland`. |
| `Configuration` | `ConfigurationTest` | Config parser. |
| `QMLThemeConfig` | `QMLThemeConfigTest` | Runs with `-platform offscreen`; working directory `test/`. |
| `Session` | `SessionTest` | Session parsing/reuse. |
| `SessionReuse` | `SessionReuseTest` | Session reuse logic. |
| `SystemDefaults` | `SystemDefaultsTest` | Requires `BUILD_CLASSIC_GREETER`. |
| `GreeterAppearance` | `GreeterAppearanceTest` | Requires `BUILD_CLASSIC_GREETER`. |
| `GreeterScale` | `GreeterScaleTest` | Requires `BUILD_CLASSIC_GREETER`; links X11/Xrandr. |
| `VirtualKeyboard` | `VirtualKeyboardTest` | Requires `BUILD_CLASSIC_GREETER`; `-platform offscreen`. |
| `MultiScreenManager` | `MultiScreenManagerTest` | Requires `BUILD_CLASSIC_GREETER`; `-platform offscreen`. |
| `GreeterSettings` | `GreeterSettingsTest` | Config persistence and greeter selection. |
| `AuthTransition` | `AuthTransitionTest` | 20 s timeout; runs a private `gxdm-helper` fixture without PAM. |
| `LockBackend` | `LockBackendTest` | Requires `LOCK_BUILD_TESTS=ON`; lock facade and user model. |
| `LockStyle` | `LockStyleTest` | Requires `LOCK_BUILD_TESTS=ON`; lock style resolution and deferred handoff. |

Neo-greeter tests are built only with `-DGREETER_BUILD_TESTS=ON`:

- `greeter-backend`
- `session-list`
- `screen-manager`
- `greeter-qml` (Qt Quick Test, `-platform offscreen`)

Conventions:

- Use `QTEST_GUILESS_MAIN` for non-GUI tests and `QTEST_MAIN` when a GUI is
  needed.
- Declare test cases as private slots in a `QObject` subclass.
- Use `QTemporaryDir` for filesystem tests and clean up after yourself.
- GUI/QML tests must run offscreen; never require a real display server.
- Add new test executables to `test/CMakeLists.txt` with `add_test(...)`, and
  link only the Qt modules they need.
- For tests that exercise the daemon's socket protocol, follow
  `AuthTransitionTest.cpp`: a single binary can act as both test and helper
  fixture.

Manual greeter checks:

```sh
./build-cmake/src/greeter-neo/gxdm-greeter-neo --test-mode
./build-cmake/src/greeter/gxdm-greeter-qt6 --test-mode --theme /path/to/theme
```

`--test-mode` prevents real power/login actions. The generic greeter accepts
`--test-mode --theme <path>`; older upstream SDDM documentation may spell the
flag as `--test`, so check the binary's `--help` output in your build.

**Safety:** do not stop/restart `gxdm.service` on a daily-use desktop to test
changes. Use a VM, a spare TTY, or a dedicated test machine.

## 6. Development Conventions

### Code style

- `src/greeter-neo` uses Google style with 2-space indentation; its
  `.clang-format` is authoritative (`BasedOnStyle: Google`,
  `FixNamespaceComments: false`). It is the only checked-in `.clang-format` in
  the repository.
- Style is mixed elsewhere. Older upstream SDDM files use 4-space indentation
  with indented namespace bodies, while newer GXDE-authored files in
  `src/daemon` and `src/common` (for example `greeter_settings.cc` and
  `GreeterAppearance.cpp`) use 2-space Google style. Match the surrounding
  file rather than assuming a subtree-wide convention, and never run the neo
  `.clang-format` across the whole repository.
- The root CMake build uses C++20. `src/greeter-neo` uses C++17, and Bazel
  defaults to C++17 while some targets opt into C++20. Keep shared code
  compatible with every target that compiles it.
- Prefer `QStringLiteral`/`QLatin1String` and range-based `for` in code subject
  to the Qt porting guards. `greeter-classic` is the documented exception.

### Namespaces and naming

- Core daemon/common/auth code uses `namespace SDDM`.
- Newer GXDE-specific helpers use `namespace gxde` (for example
  `gxde::SaveGeneralSetting`, `gxde::GreeterExecutable`). Follow the local
  file's existing namespace rather than introducing a third convention.
- Executables use the `gxdm-` prefix. Some CMake targets use an internal name
  (for example `gxde-greeter`) with `OUTPUT_NAME gxdm-greeter-neo`; refer to
  the installed binary name in user-facing docs.
- Keep upstream `sddm` identifiers where they are part of the compatibility
  surface (config keys, D-Bus names, file names, existing class names).

### Configuration and filesystem access

- Read configuration through `ConfigReader`/`Configuration` and the constants
  generated from `Constants.h.in`; do not hardcode `/etc`, `/usr` or `/var`.
- When writing config values, preserve unrelated keys, sections and comments.
  `gxde::SaveGeneralSetting` is the reference implementation.
- Treat `~gxdm/state.conf`, `/etc/gxdm.conf`, and user lock-wallpaper overrides
  as persistent user state. Changes must be atomic and must not require the
  original wallpaper file to remain available.
- Keep the system-bus D-Bus interface internal. Public applications use the
  session-bus interface; system-bus wallpaper methods accept file descriptors
  rather than caller-supplied paths for security.

### Adding or changing features

When adding a feature, update every relevant layer:

1. **C++ source:** add files to the owning `CMakeLists.txt` and, if the target
   is built by Bazel, the corresponding `BUILD.bazel`.
2. **QML:** add files to `GREETER_QML_FILES` and set a `QT_RESOURCE_ALIAS` in
   `src/greeter-neo/CMakeLists.txt`; reusable components belong in
   `components/2.0/` or `components/common/` and are installed as
   `GxdmComponents`.
3. **D-Bus:** update `data/interfaces/*.xml`, register the adaptor in
   `src/daemon/CMakeLists.txt`, update policy files if access changes, and
   document public methods in `README.md`.
4. **Configuration:** add the entry to `src/common/Configuration.h` (or the
   relevant config class), provide a sensible default, and document it.
5. **PAM/system integration:** add files under `services/` and update
   `services/CMakeLists.txt`, including Debian/FreeBSD/generic branches.
6. **Tests:** add or extend a QtTest case in `test/` (or
   `src/greeter-neo/tests/` for neo-specific code).
7. **Docs:** update `README.md`, `INSTALL.md`, or `docs/` when behavior,
   packaging, or public APIs change.

### Versioning and commits

Versions live in several places and are intentionally not identical:

- `debian/changelog`: GXDE package version (currently `1.3.4-gxde1`).
- `CMakeLists.txt` and `MODULE.bazel`: upstream SDDM base version
  (`0.21.0`).
- `arch/PKGBUILD`: Arch `pkgver`/`pkgrel`, synced from `debian/changelog` by
  `./update-pkgver`.

Do not bump versions unless explicitly asked. Recent commit subjects use
Conventional Commit-style prefixes (`feat:`, `fix:`, `bump:`) with lowercase,
imperative summaries; follow that pattern. Do not stage or commit changes
unless the user asks.

### Generated and ignored files

Do not edit or commit:

- `build/`, `build-cmake/`, `bazel-*` and other build output
- generated `src/common/Constants.h` and `src/daemon/config.h`
- `*.moc`, `moc_*.cpp`, `qrc_*.cpp`, `ui_*.h`, `*.qm`, `compile_commands.json`
- packaged `.deb`/`.tar` artifacts

## 7. Quick Reference

| Task | Command |
| --- | --- |
| Configure CMake | `cmake -S . -B build-cmake -DBUILD_WITH_QT6=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr` |
| Build | `cmake --build build-cmake --parallel` |
| Run tests | `ctest --test-dir build-cmake --output-on-failure` |
| Stage install | `DESTDIR="$PWD/build-cmake/stage" cmake --install build-cmake` |
| Bazel tarball | `bazel build --spawn_strategy=local //:gxdm-dist` |
| Debian package | `./build-deb -d` |
| Clean Debian artifacts | `./build-deb -c` |
| Arch package | `cd arch && makepkg -si` |
| Sync Arch version | `./update-pkgver` |
| Greeter preview | `./build-cmake/src/greeter-neo/gxdm-greeter-neo --test-mode` |
| Lock screen preview | `./build-cmake/src/greeter-classic/gxdm-lock-neo --test-mode` |
| Service status | `systemctl status gxdm.service --no-pager` |
| Service logs | `sudo journalctl -u gxdm.service -b --no-pager` |
| Restart service | `sudo systemctl restart gxdm.service` (only on a test machine) |
| Query greeter display server | `busctl --user call top.gxde.DisplayManager /top/gxde/DisplayManager top.gxde.DisplayManager GreeterDisplayServer` |

## 8. Known Gotchas

- **Two build systems:** CMake is primary and testable; Bazel produces the
  deployment tarball and uses hardcoded host paths. A file added to only one
  build system will break the other.
- **C++ standard split:** CMake top level is C++20; `greeter-neo` is C++17;
  Bazel defaults to C++17 but some targets (for example `greeter-classic`) opt
  into C++20. Avoid C++20-only constructs in shared or neo code.
- **Formatting split:** `greeter-neo` is Google/2-space; older upstream files
  are SDDM/4-space, while newer GXDE-authored files in `src/daemon` and
  `src/common` may also be 2-space. Match the surrounding file and never run
  the neo `.clang-format` across the whole repository.
- **Qt guards:** `QT_NO_CAST_FROM_ASCII`, `QT_NO_CAST_FROM_BYTEARRAY` and
  `QT_NO_FOREACH` are active for most targets; `greeter-classic` opts out.
- **Classic-greeter tests:** `SystemDefaults`, `GreeterAppearance`,
  `GreeterScale`, `VirtualKeyboard` and `MultiScreenManager` are skipped unless
  `BUILD_CLASSIC_GREETER=ON`.
- **Neo tests:** they are not built by default; pass `-DGREETER_BUILD_TESTS=ON`.
- **Lock tests:** `LockBackend` and `LockStyle` are only built with
  `-DLOCK_BUILD_TESTS=ON`.
- **Lock screen style:** `gxdm-lock-classic` is still the autostart/D-Bus
  entry; it execs `gxdm-lock-neo` when `LockScreenStyle=modern`. New QML lock
  files must be added to `LOCK_NEO_QML_FILES` and given a
  `QT_RESOURCE_ALIAS` in `src/lock-neo/CMakeLists.txt`.
- **QML lock and Bazel:** `//src/lock-neo:gxdm-lock-neo` is part of
  `//:gxdm-dist`. The lock backend sources stay in `src/greeter-classic/lock`
  and are exposed as `//src/greeter-classic:lock_backend`; QML files must also
  be listed in `src/lock-neo/BUILD.bazel`.
- **Greeter names:** `gxdm-greeter-neo` is the default when enabled;
  `gxdm-greeter-classic` is the fallback; `gxdm-greeter-qt6` is the generic
  theme greeter. Do not conflate them in docs or D-Bus examples.
- **Upstream docs:** `README.original.md` and `docs/THEMING.md` may refer to
  `sddm-*`; verify against the actual GXDM binaries before copying commands.
- **PAM selection:** `services/CMakeLists.txt` chooses Debian, FreeBSD, or
  generic PAM files based on the host. Do not assume one file is universal.
- **Packaging requirement:** third-party packages must install
  `data/configs/OMG.conf` to `/etc/gxdm/`.
- **Runtime testing:** never restart the display manager on a production or
  daily-use desktop; use a VM or dedicated machine.
