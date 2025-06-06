
# NetDebugClient

**NetDebugClient** 是一个跨平台的桌面应用程序，提供网络调试和串口连接功能。它通过 WebView 渲染 xterm.js 实现终端交互，并通过与 ESP32 上的 `NetDebugLink` 模块配合，实现远程调试、串口通信、WiFi 配置及命令执行等功能。应用支持 Linux 和 Windows 平台，采用 Qt 6 构建，支持灵活的 QML 用户界面和终端控制。

本项目基于 [LibXR](https://github.com/Jiu-xiao/libxr) 跨平台嵌入式框架构建，提供独立的 AppImage 和 Windows EXE 安装包，支持自动构建和发布。

---

## 🔧 功能特点

- 基于 Qt 6 和 WebView 渲染真实终端界面
- 支持与 ESP32 模块进行串口桥接和通信
- 提供 WiFi 配置与远程命令（如 REBOOT、PING 等）执行功能
- 自动识别连接的 ESP32 串口设备
- 支持设备过滤和定时 UDP 广播
- GitHub Actions 自动化构建和发布 AppImage 及 Windows EXE 安装包

---

## 🚀 快速开始

### 安装预构建包

提供适用于 Linux 和 Windows 的预构建包。请根据您的操作系统选择对应的安装包：

- [下载 Linux AppImage 包](https://github.com/Jiu-xiao/NetDebugClient/releases)
- [下载 Windows EXE 包](https://github.com/Jiu-xiao/NetDebugClient/releases)

### 启动应用

#### Linux

下载并设置可执行权限：

```bash
chmod +x NetDebugClient-x86_64.AppImage
./NetDebugClient-x86_64.AppImage
```

#### Windows

解压后直接运行 `NetDebugClient.exe` 文件即可启动应用。

---

## 本地构建Linux AppImage

请按照以下步骤在本地构建 **NetDebugClient** 项目。构建过程将基于 Qt 6 和 CMake，适用于 Linux 和 Windows 平台。

### 1. 安装依赖（Ubuntu 20.04）

首先，确保你已经安装了 Python 3 和所有必要的构建依赖。执行以下命令：

```bash
# 安装 Python 和 pip
sudo apt update
sudo apt install -y python3.9 python3.9-venv python3.9-distutils curl
update-alternatives --install /usr/bin/python python /usr/bin/python3.9 1
curl -sS https://bootstrap.pypa.io/get-pip.py | python

# 安装构建依赖
sudo apt install -y   build-essential cmake git wget curl   libnss3 libasound2 libdbus-1-3 libxcomposite1 libxdamage1   libxrandr2 libxi6 libxkbfile1 libxtst6   libnss3-dev libasound2-dev libdbus-1-dev libxcomposite-dev   libxdamage-dev libxrandr-dev libxi-dev libxkbfile-dev   libgl1-mesa-dev libxkbcommon-dev libxkbcommon-x11-0   libxcb-cursor0 libxcb-keysyms1 libxcb-image0 libxcb-icccm4   libxcb-randr0 libxcb-xinerama0 libxcb-xfixes0 libxcb-render-util0   libx11-xcb1 libxcb-shape0 libodbc1 libpq5 libmysqlclient21   pipx sudo libfontconfig-dev libfreetype6-dev
```

### 2. 安装和配置 `aqtinstall`

下载并安装 `aqtinstall`，该工具用于获取 Qt 6.9.1：

```bash
# 克隆并安装 aqtinstall
git clone https://github.com/miurahr/aqtinstall && cd aqtinstall
git checkout b22c86daef2ceeab6635ee0851e089f7346ec286
pip3.9 install .
```

### 3. 安装和配置 Qt 6.9.1

安装 Qt 6.9.1，用于构建项目：

```bash
# 安装 Qt 6.9.1
aqt install-qt linux desktop 6.9.1 linux_gcc_64 --modules all --outputdir /opt/Qt
```

### 4. 获取 LibXR 库

在项目目录中获取 LibXR：

```bash
git clone https://github.com/Jiu-xiao/libxr.git
```

### 5. 构建项目

创建构建目录并编译项目：

```bash
mkdir build && cd build
cmake .. -DQT_FORCE_MIN_CMAKE_VERSION_FOR_USING_QT=3.16
make -j$(nproc)
```

### 6. 打包为 AppImage

执行打包脚本生成 AppImage 安装包：

```bash
chmod +x ./pack_appimage.sh
./pack_appimage.sh
```

---

## 本地构建Windows EXE

### 1. 设置 Python 和 Qt

安装 Python 和 `aqtinstall`，然后下载和安装 Qt 6.9.1：

```bash
# 安装 aqtinstall
pip install aqtinstall

# 安装 Qt 6.9.1（Windows）
cmd /c "aqt install-qt windows desktop 6.9.1 win64_msvc2022_64 --modules all --outputdir C:/Qt"
```

### 2. 获取 LibXR 库

```bash
git clone https://github.com/Jiu-xiao/libxr.git
```

### 3. 配置和构建项目

使用 CMake 配置项目并构建：

```bash
# 配置项目
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=C:/Qt/6.9.1/msvc2022_64 -DCMAKE_BUILD_TYPE=Release

# 构建项目
cmake --build build --config Release
```

### 4. 使用 windeployqt 部署

在构建目录（`build/Release`），运行 `windeployqt` 部署项目：

```bash
C:/Qt/6.9.1/msvc2022_64/bin/windeployqt.exe NetDebugClient.exe --qmldir ../../qml
```

### 5. 打包 Windows EXE

在工程根目录，执行打包脚本生成 EXE 安装包：

```bash
cmd /c "pack_win_exe.cmd"
```

打包后的文件将保存在 `build/dist` 中。

---

## 📁 目录结构

```bash
NetDebugClient/
├── build/                    # 构建输出目录
├── CMakeLists.txt            # CMake 构建配置文件
├── icon.png                  # 应用图标
├── libxr/                    # LibXR 库
├── LICENSE                   # 项目许可证
├── pack_appimage.sh          # AppImage 打包脚本
├── pack_win_exe.cmd          # Windows EXE 打包脚本
├── qml/                      # QML 文件夹
│   ├── Main.qml              # 主界面文件
│   ├── MainTerminalView.qml  # 终端视图界面
│   ├── SerialConfigPanel.qml # 串口配置面板
│   ├── StatusIndicators.qml  # 状态指示器
│   ├── TabButton.qml         # 标签按钮
│   └── TerminalBackendConnector.qml # 终端后端连接器
├── README.md                 # 项目 README 文件
├── User/                     # 用户代码文件夹
│   ├── app_main.hpp          # 主程序头文件
│   ├── DeviceManager.hpp     # 设备管理器头文件
│   ├── qt_main.cpp           # 主程序源文件
│   ├── QTTimebase.hpp        # 时间基准头文件
│   ├── TerminalBackend.cpp   # 终端后端实现文件
│   └── TerminalBackend.hpp   # 终端后端头文件
└── web/                      # WebView 资源
    ├── favicon.ico           # 网站图标
    ├── index.html            # HTML 文件
    └── xterm                 # xterm 相关文件夹
```

---

## 📄 License

MIT License © 2025 Jiu-xiao
