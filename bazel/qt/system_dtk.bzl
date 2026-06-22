"""Repository rule for DTK2Widget-Qt6
"""

_BUILD = """\
load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

# Headers-only; consumers must also depend on //bazel/qt:qt6 (DTK pulls in Qt).
cc_library(
    name = "dtk",
    hdrs = glob(
        ["dwidget/**", "dcore/**", "dlog/**"],
        allow_empty = True,
    ),
    includes = ["dwidget", "dcore", "dlog"],
    linkopts = [
        "-L{libdir}",
        "-ldtk2widget",
        "-ldtk6core",
        "-ldtk6log",
    ],
)
"""

def _impl(rctx):
    rctx.symlink(rctx.attr.dwidget_dir, "dwidget")
    rctx.symlink(rctx.attr.dcore_dir, "dcore")
    rctx.symlink(rctx.attr.dlog_dir, "dlog")
    rctx.file("WORKSPACE", "")
    rctx.file("BUILD.bazel", _BUILD.format(libdir = rctx.attr.lib_dir))

system_dtk_repo = repository_rule(
    implementation = _impl,
    local = True,
    attrs = {
        "dwidget_dir": attr.string(mandatory = True),
        "dcore_dir": attr.string(mandatory = True),
        "dlog_dir": attr.string(mandatory = True),
        "lib_dir": attr.string(mandatory = True),
    },
)
