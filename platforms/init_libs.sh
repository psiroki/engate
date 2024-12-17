#!/bin/bash

set -e

cd "$(dirname "$0")"
cd ..

git clone --depth 1 https://github.com/xiph/opus.git libopus
git clone --depth 1 --branch stable https://chromium.googlesource.com/libyuv/libyuv
git clone --depth 1 --branch libwebm-1.0.0.31 https://chromium.googlesource.com/webm/libwebm
git clone --depth 1 --branch v1.15.0 https://chromium.googlesource.com/webm/libvpx

cd libopus
git apply ../libopus.diff

cd ../libyuv
git apply ../libyuv.diff

cd ../libwebm
git apply ../libwebm.diff

cd ../libvpx
git apply ../libvpx.diff
