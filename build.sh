#!/bin/bash
set -e

echo "==> 安装构建依赖..."
sudo apt install -y build-essential cmake qt6-base-dev \
    libdtk6core-dev libdtk6gui-dev libdtk6widget-dev \
    dde-tray-loader-dev pkg-config debhelper

echo "==> 构建 DEB 包..."
dpkg-buildpackage -b -us -uc

echo "==> 完成！DEB 包在上级目录："
ls -lh ../*.deb
