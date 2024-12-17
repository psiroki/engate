#!/bin/bash

# platforms.txt should have lines in the following format:
# <build-suffix> <vpx-target> /path/to/dir/containing/miyommini/cross/compile/docker/makefile
# e.g.
# armhf armv7-linux-gcc /path/to/dir/containing/armhf/cross/compile/docker/makefile
# (so the parent directory of the Makefile, not the path to the Makefile itself)

set -eu

trapERR() {
    ss=$? bc="$BASH_COMMAND" ln="$BASH_LINENO"
    echo ">> '$bc' has failed on line $ln, status is $ss (in $(pwd)) <<" >&2
    exit $ss
}

trap trapERR ERR    

SCRIPT_DIR="$(dirname $0)"
PROJECT_ROOT="$(realpath "$SCRIPT_DIR/..")"
if [ "$#" -gt 0 ]; then
  ONLY_TARGET="$1"
else
  ONLY_TARGET=
fi

processLine() {
  line="$1"
  CURRENT_LINE="$line"
  TAG="$(echo $line | cut -d ' ' -f 1)"
  SUFFIX="-$TAG"
  VPX_TARGET="$(echo $line | cut -d ' ' -f 2)"
  TCD="$(realpath $(echo $line | cut -d ' ' -f 3-))"
  cd "$PROJECT_ROOT"
  mkdir -p build$SUFFIX
  BUILD_DIR="$PROJECT_ROOT/build$SUFFIX"
  if [ -z "$ONLY_TARGET" ] || [ "$ONLY_TARGET" = "$SUFFIX" ]; then
    cd "$TCD"
    make "WORKSPACE_DIR=$PROJECT_ROOT" "DOCKER_CMD=/bin/bash /workspace/platforms/internal_build_helper.sh build$SUFFIX $TAG $VPX_TARGET" "INTERACTIVE=0" shell
  fi
}

grep -v '^#' "$SCRIPT_DIR/platforms.txt" | while read -r line && [ -n "$line" ]; do
  processLine "$line"
done
