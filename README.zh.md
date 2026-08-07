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
    开箱即用的登录管理器，专为GXDE OS构建
    <br />
    <a href="https://gitee.com/GXDE-OS/gxde-display-manager/wikis"><strong>查看WIKI »</strong></a>
    <br />
    <br />
    <a href="https://github.com/GXDE-OS/gxde-display-manager/releases">查看往期版本</a>
    &middot;
    <a href="https://gitee.com/GXDE-OS/gxde-display-manager/issues">提出Issue</a>
    &middot;
    <a href="https://gitee.com/GXDE-OS/gxde-display-manager/issues">请求功能</a>
  </p>
</div>


<!-- ABOUT THE PROJECT -->
## 关于本项目
GXDE Display Manager（简称`GXDM`）基于`SDDM`开发，专为GXDE OS打造，旨在提供开箱即用的登录管理与欢迎界面体验。

目前，`GXDM`默认采用Wayland环境，并内置了对`gxde-wlcom`、`LabWC`和`Weston`三款合成器的支持。欢迎界面本身源自DDE 15时代的Deepin `session-ui`组件，现已完成全面改造，兼容Qt6与Wayland协议栈。



### 工具链
#### 构建时
* 必要
  * Bazel/Bzlmod或CMake 3.16+
  * PAM, X11, XCB, XCursor, XFixes, XInput, XKB, XRandR, XTest
  * 一个兼容C++ 20的编译器
  * **Qt6**: Core, DBus, Gui, Widgets, Network, Qml, Quick, Svg, Xml
  * **DTK**: DTK2Widget-Qt6, DTK6 Core, DTK6 DLog
* 可选
  * **Debian包构建**: Debhelper, dpkg-buildpackage

***注**: 完整的Debian打包依赖以[`debian/control`](./debian/control)文件为准。*

#### 运行时
* PAM
* logind
* D-Bus
* AccountsService
* XKB, Xcursor
* Wayland/X11环境
* 上述Qt6与DTK库


<!-- GETTING STARTED -->
## 开始上手
### 前置条件
在开始前，请安装所有上述依赖包。

本程序同时支持Bazel和CMake构建。使用Bazel前请确保其在系统上可用；安装方法参见[安装Bazel](https://bazel.build/install)。

### 从源码构建（Bazel）
```bash
$ bazel build --spawn_strategy=local //:gxdm-dist
```

生成的部署归档位于`bazel-bin/gxdm-dist.tar`。

### 从源码构建（CMake）
```bash
$ cmake -S . -B build-cmake -DBUILD_WITH_QT6=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
$ cmake --build build-cmake --parallel
```


### 从源码构建（自动DEB打包）
该流程通过`debian/rules`调用CMake完成配置、测试、构建和安装。

```bash
$ chmod a+x ./build-deb
$ ./build-deb -d
```

生成的`.deb`包位于项目根目录的上级目录。

构建完成后如需清理请执行：
```bash
./build-deb -c
```

请注意这也将同时清理掉产出的`.deb`文件。


<!-- USAGE EXAMPLES -->
## 使用方式
GXDM的`.deb`包在安装时有一个供用户选择当前显示管理器的界面，可以利用该界面更新显示管理器的偏好。

GXDM的使用方式与SDDM一致，以下列出一些常用指令。

### 手动启用GXDM
```bash
$ sudo systemctl stop <其他正在使用的display manager服务名>.service
$ sudo systemctl disable <其他正在使用的display manager服务名>.service
$ sudo systemctl enable gxdm.service
$ sudo systemctl start gxdm.service
```

### 手动停用GXDM
```bash
$ sudo systemctl stop gxdm.service
$ sudo systemctl disable gxdm.service
$ sudo systemctl enable <其他欲使用的display manager服务名>.service
$ sudo systemctl start <其他欲使用的display manager服务名>.service
```

### 手动重启GXDM
```bash
$ sudo systemctl restart gxdm.service
```

### 验证运行状态
```bash
$ systemctl status gxdm.service --no-pager
$ sudo journalctl -u gxdm.service -b --no-pager
```


<!-- ROADMAP -->
## 里程碑
- [x] 新增DEB打包
- [x] 移植DDE Greeter
- [x] 新增Waylanbd greeter支持
- [x] 新增CMake支持
- [ ] 新增Archlinux打包
- [ ] 新增屏幕键盘支持

请参阅[开启的Issues](https://gitee.com/GXDE-OS/gxde-display-manager/issues)以获取当前所有请求的新功能。



<!-- CONTRIBUTING -->
## 贡献
我们感激每一位贡献者，如果您有让GXDM更好的建议或者想要请求功能，欢迎您提出新Issue。您也可以fork本仓库并创建拉取请求。

### GXDM的贡献者们
***注**: GXDM基于SDDM，原版SDDM的贡献者信息可以在[这里](https://github.com/sddm/sddm/graphs/contributors?all=1)找到*

<a href="https://github.com/GXDE-OS/gxde-display-manager/graphs/contributors?all=1">
  <img src="https://contrib.rocks/image?repo=GXDE-OS/gxde-display-manager" alt="contrib.rocks image" />
</a>


<!-- LICENSE -->
## 许可证
本项目以[GNU GENERAL PUBLIC LICENSE Version 3](./LICENSE)许可发行。

本项目含有来自GXDE `session-ui`的源码，可以在`src/greeter-neo`找到，原版上游位于[GXDE-OS/gxde-session-ui](https://gitee.com/GXDE-OS/gxde-session-ui)，以[GNU GENERAL PUBLIC LICENSE Version 3](https://gitee.com/GXDE-OS/gxde-session-ui/blob/master/LICENSE?svcp_stk=1_ea4aETwwgfjPoBNPiiCDEDSV5zkBHs2UhWwbnE9dOUMgTQpn3GWWsP7g-d5P7vdOc8pUy6n3gqUzBwYWRSpsMn5glSx_AwPrOegiEQB6-z-bFaJX84-951qO7y66p6b5okTkZXtJkAnfclQjppYXX55voA3ESpiytcTqZIN1qUEqZKeRHjnERUf83GXnqoXUeXTWmfPGOafIokUE6EkrfA%3D%3D)获得许可，除非在文件头部另行说明。

本项目含有[CharOfString/samoyed-greeter](https://gitee.com/CharOfString/samoyed-greeter)的部分源码，以[GNU GENERAL PUBLIC LICENSE Version 3](https://gitee.com/CharOfString/samoyed-greeter/blob/main/LICENSE)获得许可，除非在文件头部另行说明。

本项目使用[Maven Pro](https://github.com/googlefonts/mavenproFont)作为锁屏字体，其以[SIL Open Font License 1.1](https://github.com/googlefonts/mavenproFont/blob/main/OFL.txt)获得许可。


<!-- ACKNOWLEDGMENTS -->
## 鸣谢
GXDM的开发离不开以下项目提供的参考：
* **SDDM**: https://github.com/sddm/sddm
* **LightDM**: https://github.com/ubuntu/lightdm
* **GXDE Session UI**: https://gitee.com/GXDE-OS/gxde-session-ui
* **Samoyed Greeter**: https://gitee.com/CharOfString/samoyed-greeter
* **BEST-Readme-Template**: https://github.com/othneildrew/Best-README-Template
