# netspeed-plugin — 开发指南

## 项目概述
deepin 25 任务栏实时网速插件（上行/下行），基于 Qt6 + dde-tray-loader。

## 构建
```bash
# 本地构建 DEB
./build.sh

# 或直接 CMake
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DOPT_ENABLE_QT6=ON
make
sudo make install
```

## CI/CD
- GitHub Actions: `.github/workflows/build.yml`
- 基于 `linuxdeepin/deepin:25` 容器构建
- tag `v*` 触发自动构建并发布到 Releases
- 产物: `dde-dock-netspeed-plugin_<ver>_amd64.deb`

## 安装
```bash
sudo dpkg -i dde-dock-netspeed-plugin_*.deb
killall dde-shell   # 重启任务栏
```

## 关键文件

### 插件入口
- `netspeedplugin.h/cpp` — `PluginsItemInterface` 实现，插件生命周期
- `mainwidget.h/cpp` — 任务栏显示控件（↑↓ 双排固定宽度布局）
- `type.h` — `NetSpeedInfo` 结构体

### 元数据
- `netspeed.json` — `{"api": "1.0"}`，Qt 插件元数据
- `resources/gschema/` — GSettings schema，用于控制中心插件设置页
- `resources/icons/` — 控制中心图标

### 打包
- `debian/` — DEB 打包配置
- `debian/rules` — 使用 `dh $@ --parallel`，手动 postinst 编译 GSettings
- `debian/postinst` / `postrm` — 安装/卸载时编译 GSettings schema

## 技术要点

### deepin 25 插件加载机制
- dde-shell 扫描 `/usr/lib/dde-dock/plugins/*.so`
- 通过 DConfig (`org.deepin.ds.dock.tray`) 对插件分组
- 每组启动独立 `trayplugin-loader` 子进程
- 插件实现 `PluginsItemInterface`，IID 为 `com.deepin.dde.tray.Interface`

### 关键依赖
- `dde-tray-loader-dev` — 提供 `PluginsItemInterface` 和 `ModuleInterface_iid`
- `libglib2.0-bin` — 提供 `glib-compile-schemas`，用于 postinst 编译 GSettings
- `qt6-base-dev`, `libdtk6core-dev`, `libdtk6gui-dev`

### 布局
- 双行（TopToBottom），每行三列固定宽度（箭头 | 数字 | 单位）
- 箭头用 ↑/↓ Unicode 字符
- 数字右对齐，宽度固定 `999.9`，消除抖动

### 右键菜单
- 放大 (+10%) / 缩小 (-10%)，范围 50%~200%
- 缩放值通过 `QSettings` 持久化到 `~/.config/dde-dock-netspeed-plugin/settings.conf`
- **注意**: dde-tray-loader 的 `PluginProxyInterface::saveValue/getValue` 是空实现，不可用

## 常见问题
- 插件不显示: 检查 `/usr/lib/dde-dock/plugins/` 是否有 `libnetspeed-monitor.so`
- GSettings 未编译: 运行 `sudo glib-compile-schemas /usr/share/glib-2.0/schemas/`
- 安装后不生效: `killall dde-shell` 重启
