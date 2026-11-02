#!/usr/bin/env bash
# Print the directory that contains Eigen's headers on the current runner.
case "$RUNNER_OS" in
  Linux) echo /usr/include/eigen3 ;;
  macOS) echo "$(brew --prefix eigen)/include/eigen3" ;;
  Windows) cygpath -m /mingw64/include/eigen3 ;;
esac
