#!/usr/bin/env bash
# Source this to make the shared libraries findable at run time.
#   source .github/scripts/runtime-env.sh <libcxxwrap-julia prefix>
# Every platform finds them differently; the Windows loader only looks at PATH.
case "$RUNNER_OS" in
  Linux) export LD_LIBRARY_PATH="${MUESLI_PREFIX}/lib:${GITHUB_WORKSPACE}/build/lib" ;;
  macOS) export DYLD_LIBRARY_PATH="${MUESLI_PREFIX}/lib:${GITHUB_WORKSPACE}/build/lib" ;;
  Windows)
    workspace=$(cygpath -u "$GITHUB_WORKSPACE")
    muesli=$(cygpath -u "$MUESLI_PREFIX")
    cxxwrap=$(cygpath -u "$1")
    export PATH="$muesli/bin:$workspace/build/lib:$cxxwrap/bin:$cxxwrap/lib:$PATH"
    ;;
esac
