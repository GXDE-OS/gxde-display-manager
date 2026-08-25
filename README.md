![HEADER](./docs/img/header.png)

![CONTRIBUTORS](https://img.shields.io/github/contributors/GXDE-OS/gxde-display-manager.svg?style=plastic)
![ISSUES](https://img.shields.io/github/issues/GXDE-OS/gxde-display-manager.svg?style=plastic)
![LICENSE](https://img.shields.io/badge/license-GPLv3-blue)
![](https://img.shields.io/badge/made_with-love-red)


<!-- PROJECT LOGO -->
<br />
<div align="center">
  <!--
  <a href="https://github.com/othneildrew/Best-README-Template">
    <img src="images/logo.png" alt="Logo" width="80" height="80">
  </a>
  -->

  <h3 align="center">GXDE Display Manager</h3>

  <p align="center">
    An out-of-box display manager, built specifically for GXDE OS.
    <br />
    <a href="https://gitee.com/GXDE-OS/gxde-display-manager/wikis"><strong>Explore WIKI »</strong></a>
    <br />
    <br />
    <a href="https://github.com/GXDE-OS/gxde-display-manager/releases">Explore previous releases</a>
    &middot;
    <a href="https://gitee.com/GXDE-OS/gxde-display-manager/issues">Fire up an issue</a>
    &middot;
    <a href="https://gitee.com/GXDE-OS/gxde-display-manager/issues">Request features</a>
  </p>
</div>


<!-- ABOUT THE PROJECT -->
## About the Project
GXDE Display Manager (`GXDM`) is a fork of `SDDM` and is build specifically for GXDE OS. It aims to bring an out-of-box experience of logining and greeter.

Currently, `GXDM` uses a Wayland greeter by default. It prefers `gxde-wlcom` and falls back to `LabWC`, `Sway` or `Weston` when it is unavailable. The greeter itself is from Deepin `session-ui` from the DDE 15 era, and it is fully adapted to Wayland and Qt6.



### Toolchains
#### Build Dependencies
* REQUIRED
  * Bazel/Bzlmod或CMake 3.16+
  * PAM, X11, XCB, XCursor, XFixes, XInput, XKB, XRandR, XTest
  * A compiler that is compactiable with C20 standard.
  * **Qt6**: Core, DBus, Gui, Widgets, Network, Qml, Quick, Svg, Xml, WaylandClient
  * **DTK**: DTK2Widget-Qt6, DTK6 Core, DTK6 DLog
  * **GSettings**: gsettings-qt6 development files
  * **Wayland lock screen**: Qt WaylandClient and LayerShellQt development files
* Optional
  * **Debian packaging**: Debhelper, dpkg-buildpackage

***NOTE**: You may refer to [`debian/control`](./debian/control) for full Debian dependencies.*

#### Runtime
* PAM
* logind
* D-Bus
* AccountsService
* XKB, Xcursor
* Wayland/X11 environment
* DTK and Qt6 libraries mentioned above
* gsettings-qt6
* Wayland locking uses `ext-session-lock-v1` when the compositor supports it;
  LayerShellQt/`zwlr_layer_shell_v1` remains the compatibility fallback


<!-- GETTING STARTED -->
## Getting Started
### Prerequisite
Please install all the depencies above before getting started.

GXDM supports both Bazel and CMake. You should ensure Bazel is installed if you really want to build it with Bazel. The guide for installing Bazel could be found [here](https://bazel.build/install).

### Building From Source (Bazel)
```bash
$ bazel build --spawn_strategy=local //:gxdm-dist
```

The generated deployment archive is located at `bazel-bin/gxdm-dist.tar`.

### Building From Source (CMake)
```bash
$ cmake -S . -B build-cmake -DBUILD_WITH_QT6=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
$ cmake --build build-cmake --parallel
```


### Building a Debian Package
You won't have to worry about CMake things for it is handled through`debian/rules`.

```bash
$ chmod a+x ./build-deb
$ ./build-deb -d
```

The `.deb` package generated could be found on the parent of project root directory.

If you need a cleanup after you're done, please run:
```bash
./build-deb -c
```

Please note that this also clears the artifacts (those `.deb` files) generated.


<!-- USAGE EXAMPLES -->
## Usage
GXDM's `.deb` package should allow you to choose your preferred display manager after GXDM is being installed.

GXDM's usage is almost the same as SDDM, and here are some handy commands:

### Manually Enable & Start
```bash
$ sudo systemctl stop <其他正在使用的display manager服务名>.service
$ sudo systemctl disable <其他正在使用的display manager服务名>.service
$ sudo systemctl enable gxdm.service
$ sudo systemctl start gxdm.service
```

### Manually Stop & Disable
```bash
$ sudo systemctl stop gxdm.service
$ sudo systemctl disable gxdm.service
$ sudo systemctl enable <其他欲使用的display manager服务名>.service
$ sudo systemctl start <其他欲使用的display manager服务名>.service
```

### Manually Restart
```bash
$ sudo systemctl restart gxdm.service
```

### Check Status
```bash
$ systemctl status gxdm.service --no-pager
$ sudo journalctl -u gxdm.service -b --no-pager
```

### D-Bus API
GXDM exposes lock-shortcut and global greeter appearance controls on the session bus. Applications should use this public endpoint:

| Item | Value |
| --- | --- |
| Bus | Session bus |
| Service | `top.gxde.DisplayManager` |
| Object path | `/top/gxde/DisplayManager` |
| Interface | `top.gxde.DisplayManager` |

| Method | Description |
| --- | --- |
| `TryEnrollLkScr(bool enabled) -> bool` | Registers or withdraws the Super+L GXDM lock shortcut. An existing Super+L binding is never stolen. |
| `LkScrStat() -> bool` | Returns whether GXDM successfully owns the Super+L shortcut. |
| `Show()` | Shows the GXDM lock screen. |
| `SetCursor(string theme) -> bool` | Sets the global greeter cursor theme. The theme must already be installed. |
| `SetWallpaperGXDEDefault() -> bool` | Restores the current GXDE default as the global greeter wallpaper. |
| `SetWallpaperDDELockDefault() -> bool` | Uses the bundled DDE lock default background as the global greeter wallpaper. |
| `SetWallpaper(string path) -> bool` | Sets a custom global greeter wallpaper from a local path or `file://` URL. |
| `SetLockWallpaperOverride(string path) -> bool` | Sets the current user's lock-screen wallpaper override from a local path or `file://` URL. |
| `ClearLockWallpaperOverride() -> bool` | Clears the current user's lock-screen wallpaper override. |
| `LockWallpaperOverride() -> string` | Returns the current user's persisted lock-screen wallpaper override. |
| `SetGreeterDisplayServer(string displayServer) -> bool` | Persists the greeter display server. Valid values are `wayland`, `x11`, and `x11-user`. |
| `GreeterDisplayServer() -> string` | Returns the persisted greeter display server. |

**Note:** the greeter uses one global wallpaper. The lock screen uses this priority order: the current user's `LockWallpaperOverride`, the current user's Deepin `GreeterBackground`, GSettings desktop backgrounds, then the global greeter wallpaper/default. Every wallpaper setting refreshes the active greeter or lock screen immediately after it is persisted. Custom wallpapers and overrides must be recognized images no larger than 128 MiB. The global greeter wallpaper is copied under `~gxdm`. The user lock override is copied twice: `~/.local/share/gxdm/lock-wallpaper-override` for the in-session locker and `~gxdm/lock-wallpaper-override-<uid>` for the greeter (which can still read it when the user's home directory is not traversable, e.g. mode `0700`); neither depends on the original image remaining available. `SetGreeterDisplayServer` takes effect the next time the greeter display is created.

Examples:

```bash
# Register Super+L
busctl --user call top.gxde.DisplayManager \
  /top/gxde/DisplayManager top.gxde.DisplayManager \
  TryEnrollLkScr b true

# Query the Super+L state
busctl --user call top.gxde.DisplayManager \
  /top/gxde/DisplayManager top.gxde.DisplayManager \
  LkScrStat

# Set the greeter cursor theme
busctl --user call top.gxde.DisplayManager \
  /top/gxde/DisplayManager top.gxde.DisplayManager \
  SetCursor s gxde

# Set a custom greeter wallpaper
busctl --user call top.gxde.DisplayManager \
  /top/gxde/DisplayManager top.gxde.DisplayManager \
  SetWallpaper s /absolute/path/to/wallpaper.jpg

# Restore the GXDE default greeter wallpaper
busctl --user call top.gxde.DisplayManager \
  /top/gxde/DisplayManager top.gxde.DisplayManager \
  SetWallpaperGXDEDefault

# Override the lock-screen wallpaper
busctl --user call top.gxde.DisplayManager \
  /top/gxde/DisplayManager top.gxde.DisplayManager \
  SetLockWallpaperOverride s /absolute/path/to/lock-wallpaper.jpg

# Clear the lock-screen wallpaper override
busctl --user call top.gxde.DisplayManager \
  /top/gxde/DisplayManager top.gxde.DisplayManager \
  ClearLockWallpaperOverride

# Use the X11 greeter display server
busctl --user call top.gxde.DisplayManager \
  /top/gxde/DisplayManager top.gxde.DisplayManager \
  SetGreeterDisplayServer s x11

# Query the greeter display server
busctl --user call top.gxde.DisplayManager \
  /top/gxde/DisplayManager top.gxde.DisplayManager \
  GreeterDisplayServer
```

The GXDM daemon also owns the same service name on the system bus and exposes the internal `top.gxde.DisplayManager.System` interface. It persists global appearance values in `~gxdm/state.conf` and writes the greeter display server to `/etc/gxdm.conf`; global custom wallpapers are accepted as Unix file descriptors rather than caller-supplied paths. The session-bus interface stores user lock-screen overrides in the user's data directory and additionally forwards them to the system-bus interface so the greeter can read a copy from `~gxdm`. The system-bus interface is an implementation detail and should not be called directly by regular applications.


<!-- ROADMAP -->
## Roadmap
- [x] Add DEB packaging.
- [x] Port DDE Greeter.
- [x] Add support for Wayland greeter.
- [x] Update support for CMake.
- [x] Add support for on-screen keyboard.
- [ ] Add Archlinux packaging.

Please refer to [opened issues](https://gitee.com/GXDE-OS/gxde-display-manager/issues) to see those feature requested.



<!-- CONTRIBUTING -->
## Contributing
We are grateful to all contributors. If you have any suggestions for making GXDM better or feature requests, feel free to open a new Issue. You can also fork this repo and submit a pull request.

### Add Packing for Other Distros
If you want to add packaging for other distros, please make sure that your package installs `./data/configs/OMG.conf` to `/etc/gxdm/`.

### Top Contributors
***NOTE**: GXDM is based on SDDM, and the contribution information of SDDM could be found [here](https://github.com/sddm/sddm/graphs/contributors?all=1).*

<a href="https://github.com/GXDE-OS/gxde-display-manager/graphs/contributors?all=1">
  <img src="https://contrib.rocks/image?repo=GXDE-OS/gxde-display-manager" alt="contrib.rocks image" />
</a>


<!-- LICENSE -->
## License
This project is licensed under [GNU GENERAL PUBLIC LICENSE Version 3](./LICENSE).

This project contains source code from GXDE `session-ui`, which could be located under `src/greeter-classic` and upstream [GXDE-OS/gxde-session-ui](https://gitee.com/GXDE-OS/gxde-session-ui). They are licensed under [GNU GENERAL PUBLIC LICENSE Version 3](https://gitee.com/GXDE-OS/gxde-session-ui/blob/master/LICENSE?svcp_stk=1_ea4aETwwgfjPoBNPiiCDEDSV5zkBHs2UhWwbnE9dOUMgTQpn3GWWsP7g-d5P7vdOc8pUy6n3gqUzBwYWRSpsMn5glSx_AwPrOegiEQB6-z-bFaJX84-951qO7y66p6b5okTkZXtJkAnfclQjppYXX55voA3ESpiytcTqZIN1qUEqZKeRHjnERUf83GXnqoXUeXTWmfPGOafIokUE6EkrfA%3D%3D) unless specified in file header.

This project contains source code from [CharOfString/samoyed-greeter](https://gitee.com/CharOfString/samoyed-greeter). They are licensed under [GNU GENERAL PUBLIC LICENSE Version 3](https://gitee.com/CharOfString/samoyed-greeter/blob/main/LICENSE) unless specified in file header.

This project uses [Maven Pro](https://github.com/googlefonts/mavenproFont) as lockscreen font, which is licensed under [SIL Open Font License 1.1](https://github.com/googlefonts/mavenproFont/blob/main/OFL.txt).


<!-- ACKNOWLEDGMENTS -->
## Acknowledgements
The development of GXDM draws heavily on the reference and inspiration from the following projects: 
* **SDDM**: https://github.com/sddm/sddm
* **LightDM**: https://github.com/ubuntu/lightdm
* **GXDE Session UI**: https://gitee.com/GXDE-OS/gxde-session-ui
* **Samoyed Greeter**: https://gitee.com/CharOfString/samoyed-greeter
* **BEST-Readme-Template**: https://github.com/othneildrew/Best-README-Template
* **SilentSDDM**: https://github.com/uiriansan/SilentSDDM
