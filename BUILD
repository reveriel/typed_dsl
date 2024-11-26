load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test", "cc_binary")

# Common C++ compiler flags
copts = [
    "-std=c++17",
]

cc_library(
    name = "dag",
    hdrs = ["dag.h"],
    srcs = ["dag.cpp"],
    deps = [],
    visibility = ["//visibility:public"],
    copts = copts,
)

cc_test(
    name = "dag_test",
    srcs = ["dag_test.cc"],
    deps = [
        ":dag",
        "@com_google_googletest//:gtest_main",
    ],
    copts = ["-g"],
)

cc_binary(
    name = "dag_benchmark",
    srcs = ["dag_benchmark.cc"],
    deps = [
        ":dag",
        "@google_benchmark//:benchmark",
    ],
)

load("@hedron_compile_commands//:refresh_compile_commands.bzl", "refresh_compile_commands")

refresh_compile_commands(
    name = "refresh_compile_commands",

    # Specify the targets of interest.
    # For example, specify a dict of targets and any flags required to build.
    targets = {
    },
    # No need to add flags already in .bazelrc. They're automatically picked up.
    # If you don't need flags, a list of targets is also okay, as is a single target string.
    # Wildcard patterns, like //... for everything, *are* allowed here, just like a build.
      # As are additional targets (+) and subtractions (-), like in bazel query https://docs.bazel.build/versions/main/query.html#expressions
    # And if you're working on a header-only library, specify a test or binary target that compiles it.
    exclude_headers = "all"
)
