## CMake installation instructions

GXDM requires CMake 3.16 or newer, a C++20 compiler, Qt 6 and DTK2Widget-Qt6.
The complete package dependency list is maintained in `debian/control`.

Configure and build from the project root:

```sh
cmake -S . -B build-cmake \
    -DBUILD_WITH_QT6=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build-cmake --parallel
```

The default build includes the daemon, the Qt 6 QML greeter, the GXDE greeter,
the GXDE lock screen and all helper executables. Stage the complete install tree
before installing it onto the host:

```sh
DESTDIR="$PWD/build-cmake/stage" cmake --install build-cmake
```

After inspecting `build-cmake/stage`, install it with:

```sh
sudo cmake --install build-cmake
```

Useful project options include:

- `BUILD_MAN_PAGES=ON` builds the manual pages (requires `rst2man`).
- `ENABLE_JOURNALD=OFF` disables journald logging.
- `BUILD_NEO_GREETER=OFF` excludes the GXDE greeter and lock screen.
- `BUILD_WITH_QT6=OFF -DBUILD_NEO_GREETER=OFF` enables the legacy Qt 5 build.
- `INSTALL_PAM_CONFIGURATION=OFF` skips installation of PAM configuration.

Run `cmake -S . -B build-cmake -LAH` to list all cached options. CMake installs
files directly and does not produce a Debian package; use `./build-deb -d` for
the package-managed installation path.
