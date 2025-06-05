#!/bin/bash

# === 1. 基础配置 ===
APP=NetDebugClient
BUILD_DIR=build
QT_PATH=/opt/Qt/6.9.1/gcc_64
APPDIR=AppDir
ICON_FILE=icon.png
DESKTOP_FILE=$APP.desktop

# === 2. 清理旧文件 ===
echo "🧹 清理旧文件..."
rm -rf "$APPDIR" "${APP}"*.AppImage squashfs-root/ linuxdeployqt.AppImage appimagetool-x86_64.AppImage

# 下载最新 AppImage 工具
wget -q --show-progress https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage -O linuxdeployqt.AppImage
wget -q --show-progress https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage
chmod +x *.AppImage

# === 3. 创建目录结构 ===
echo "📁 创建 AppDir 结构..."
mkdir -p "$APPDIR/usr/bin"
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"
mkdir -p "$APPDIR/usr/qml/MyApp"

# === 4. 拷贝可执行文件和资源 ===
echo "📦 拷贝可执行文件和资源..."
cp "$BUILD_DIR/$APP" "$APPDIR/usr/bin/"
cp -r qml web "$APPDIR/usr/"
cp "$ICON_FILE" "$APPDIR/usr/share/icons/hicolor/256x256/apps/netdebug.png"
cp "$ICON_FILE" "$APPDIR/netdebug.png"

# 拷贝 QML 并生成 qmldir 文件
cp qml/*.qml "$APPDIR/usr/qml/MyApp/"
cat >"$APPDIR/usr/qml/MyApp/qmldir" <<EOF
module MyApp
Main 1.0 Main.qml
EOF

# === 5. 创建 .desktop 文件 ===
echo "📝 生成 .desktop 文件..."
cat >"$APPDIR/usr/share/applications/$DESKTOP_FILE" <<EOF
[Desktop Entry]
Type=Application
Name=NetDebugClient
Exec=$APP
Icon=netdebug
Categories=Utility;
EOF

# === 6. 解包 linuxdeployqt 并使用 ===
echo "🛠 解包并运行 linuxdeployqt..."
./linuxdeployqt.AppImage --appimage-extract
./squashfs-root/AppRun "$APPDIR/usr/share/applications/$DESKTOP_FILE" \
    -qmake="$QT_PATH/bin/qmake" \
    -qmldir=qml \
    -bundle-non-qt-libs \
    -no-translations \
    -verbose=2 \
    -unsupported-allow-new-glibc

# === 7. 解包 appimagetool 并生成 AppImage ===
echo "📦 解包并运行 appimagetool..."
./appimagetool-x86_64.AppImage --appimage-extract
./squashfs-root/AppRun "$APPDIR"

# === 8. 清理中间文件 ===
echo "🧽 清理中间文件..."
rm -rf "$APPDIR" squashfs-root/ linuxdeployqt.AppImage appimagetool-x86_64.AppImage

# === 9. 完成提示 ===
echo "✅ 打包完成：$(ls ${APP}*.AppImage)"
