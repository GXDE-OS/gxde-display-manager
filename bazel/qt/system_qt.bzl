"""Repository rule that exposes system Qt6 to Bazel
"""

_BUILD = """\
load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "qt6",
    hdrs = glob(["include/**"], allow_empty = True),
    includes = [
        "include",
        "include/QtCore",
        "include/QtGui",
        "include/QtNetwork",
        "include/QtDBus",
        "include/QtQml",
        "include/QtQuick",
        "mkspecs",
    ],
    linkopts = [
        "-L{libdir}",
        "-lQt6Core",
        "-lQt6Gui",
        "-lQt6Network",
        "-lQt6DBus",
        "-lQt6Qml",
        "-lQt6Quick",
    ],
)
"""

def _impl(rctx):
    rctx.symlink(rctx.attr.include_dir, "include")
    rctx.symlink(rctx.attr.mkspecs_dir, "mkspecs")
    rctx.file("WORKSPACE", "")
    rctx.file("BUILD.bazel", _BUILD.format(libdir = rctx.attr.lib_dir))

system_qt_repo = repository_rule(
    implementation = _impl,
    local = True,
    attrs = {
        "include_dir": attr.string(mandatory = True),
        "mkspecs_dir": attr.string(mandatory = True),
        "lib_dir": attr.string(mandatory = True),
    },
)
