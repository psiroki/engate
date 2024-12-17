#!/bin/bash

set -e

cd "$(dirname "$0")"

mkdir -p compilation

cd compilation

git clone --depth 1 https://github.com/psiroki/x55toolchain.git arm64
git clone --depth 1 https://github.com/psiroki/rg35xxhf.git armhf

BASE="$(pwd)"

cd ..

cat << EOF > platforms.txt
arm64 arm64-linux-gcc $BASE/arm64
armhf armv7-linux-gcc $BASE/armhf
EOF

