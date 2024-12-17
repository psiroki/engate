#!/bin/bash

set -e

if [ "$#" -ne 3 ]; then
  echo internal_build_helper.sh '<build directory> <tag> <libvpx target>' 1>&2
  exit
fi

trapERR() {
    ss=$? bc="$BASH_COMMAND" ln="$BASH_LINENO"
    echo ">> '$bc' has failed on line $ln, status is $ss <<" >&2
    echo ">> $CURRENT_LINE" >&2
    exit $ss
}

echo Building in $1
echo Running cmake with $2
echo LibVPX target is $3

PS1=anything_but_empty_string
. /etc/bash.bashrc

PROJECT_ROOT="$(realpath "$(dirname "$0")"/..)"
cd $PROJECT_ROOT/libwebm
mkdir -p $1
cd $1
cmake ..
make

cd $PROJECT_ROOT/libopus
mkdir -p $1
cd $1
cmake ..
make

cd $PROJECT_ROOT/libyuv
mkdir -p $1
cd $1
cmake ..
make

cd $PROJECT_ROOT/libvpx
mkdir -p $1
cd $1
../configure --enable-static --disable-shared --disable-examples --disable-tools --disable-unit-tests --target=$3
make

mkdir -p $PROJECT_ROOT/$1
cd $PROJECT_ROOT/$1

SUFFIX="$(echo $1 | sed 's/^build//')"
cmake $PROJECT_ROOT "-DPLATFORM_SUFFIX=$SUFFIX" "-DEG_ARCH=$2" -DCMAKE_BUILD_TYPE=Release
make
