#!/usr/bin/env bash

# This script builds GXDE-Display-Manager & its deployable install tarball with
# bazel.
#
# Qt6 devel packages are required and we ASSUME that you already satisified
# those requirement.
# See MODULE.bazel and bazel/qt/system_qt.bzl for more details.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

bazel build "$@" \
    //src/daemon:gxdm \
    //src/greeter:gxdm-greeter-qt6 \
    //src/helper:gxdm-helper \
    //src/helper:gxdm-helper-start-wayland \
    //src/helper:gxdm-helper-start-x11user \
    //:gxdm-dist

echo
echo "Build shall be complete. Here're the artifacts:"
echo "\tdaemon: bazel-bin/src/daemon/gxdm"
echo "\tgreeter: bazel-bin/src/greeter/gxdm-greeter-qt6"
echo "\thelpers: bazel-bin/src/helper/gxdm-helper{,-start-wayland,-start-x11user}"
echo "\ttarball: bazel-bin/gxdm-dist.tar  (extract at / on the target)"
