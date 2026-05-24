# deepin 25 任务栏实时网速插件

[![Build](https://github.com/kali-urs/netspeed-plugin/actions/workflows/build.yml/badge.svg)](https://github.com/kali-urs/netspeed-plugin/actions/workflows/build.yml)

适用于 **deepin 25 / UOS 25** 的任务栏实时网速监控插件，在任务栏上直接显示上行/下行实时网速。

## 效果

```
↑ 1.2  MB/s
↓ 100  KB/s
```

- ↑↓ 箭头和数字分离，宽度固定，绝不抖动
- 支持任务栏上/下/左/右四种位置自动适配布局
- 右键菜单可调整字体大小（放大/缩小）

## 下载安装

从 [Releases 页面](https://github.com/kali-urs/netspeed-plugin/releases) 下载最新 `netspeed-plugin-v*.tar.gz`，解压后：

```bash
sudo cp libnetspeed-monitor.so /usr/lib/dde-dock/plugins/
sudo chmod 644 /usr/lib/dde-dock/plugins/libnetspeed-monitor.so
killall dde-shell
```

或在 **控制中心 → 个性化 → 桌面 → 插件区域** 中启用。

## 源码编译

```bash
# 安装依赖
sudo apt install build-essential cmake qt6-base-dev \
    libdtk6core-dev libdtk6gui-dev libdtk6widget-dev \
    dde-tray-loader-dev

# 编译安装
mkdir build && cd build
cmake .. -DOPT_ENABLE_QT6=ON -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo cp libnetspeed-monitor.so /usr/lib/dde-dock/plugins/
sudo chmod 644 /usr/lib/dde-dock/plugins/libnetspeed-monitor.so
killall dde-shell
```

### Docker 交叉编译（macOS/Linux 通用）

```bash
./docker-build.sh
# 编译产物在 output/libnetspeed-monitor.so
```

## 右键菜单

| 功能 | 说明 |
|------|------|
| 放大 | 字体缩放 +10%（最大 200%） |
| 缩小 | 字体缩放 -10%（最小 50%） |
| 刷新 | 重置网速统计计数 |

## 技术原理

- 每秒读取 `/proc/net/dev` 获取各网口收发字节数
- 过滤 `lo` 环回接口，累加实际网口数据
- 计算每秒差值得到实时网速
- 通过 `dde-tray-loader` 的 `PluginsItemInterface` 加载到任务栏

## 项目结构

```
├── .github/workflows/build.yml   # GitHub Actions 自动构建
├── CMakeLists.txt                # CMake 构建配置（Qt6）
├── Dockerfile.build              # Docker 交叉编译
├── docker-build.sh               # Docker 构建脚本
├── netspeed.json                 # 插件元数据
├── netspeedplugin.h/cpp          # 插件入口
├── mainwidget.h/cpp              # 任务栏显示控件
├── type.h                        # 数据结构
├── build.sh                      # deepin 本地编译脚本
└── README.md
```

## 兼容性

| 版本 | 状态 |
|------|------|
| deepin 25 / UOS 25 | ✅ 支持 |
| deepin V23 Release | ⚠️ 需 Qt5 编译 `-DOPT_ENABLE_QT6=OFF` |
| deepin V20 | ⚠️ 需 Qt5 编译，改用 `dde-dock-dev` 依赖 |

## License

GPL-2.0
