#!/bin/bash
set -e

BUILD_DIR="build"
PLUGIN_NAME="libnetspeed-monitor.so"
INSTALL_DIR="/usr/lib/dde-dock/plugins"

echo "==> 安装依赖 (deepin 25)"
sudo apt install -y build-essential cmake qt6-base-dev qt6-base-dev-tools \
    libdtk6core-dev libdtk6gui-dev libdtk6widget-dev \
    dde-tray-loader-dev

echo "==> 创建构建目录..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "==> 执行 cmake (Qt6)..."
cmake .. -DOPT_ENABLE_QT6=ON -DCMAKE_BUILD_TYPE=Release

echo "==> 编译..."
make -j$(nproc)

echo "==> 安装插件..."
sudo cp "$PLUGIN_NAME" "$INSTALL_DIR/"
sudo chmod 644 "$INSTALL_DIR/$PLUGIN_NAME"

echo "==> 重启任务栏..."
killall dde-shell 2>/dev/null || true

echo "==> 完成！请注销或重启后生效"
