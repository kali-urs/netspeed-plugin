#!/bin/bash
set -e

OUTPUT_DIR="output"
mkdir -p "$OUTPUT_DIR"

echo "==> 使用 Docker 编译 deepin 25 网速插件..."
docker build -t deepin-netspeed-builder -f Dockerfile.build .

echo "==> 提取编译产物..."
docker run --rm -v "$(pwd)/$OUTPUT_DIR:/out" deepin-netspeed-builder \
    cp /build/output/libnetspeed-monitor.so /out/

echo "==> 编译完成！文件位置: $OUTPUT_DIR/libnetspeed-monitor.so"
ls -lh "$OUTPUT_DIR/libnetspeed-monitor.so"
