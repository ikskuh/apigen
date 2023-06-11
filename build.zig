const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "apidef",
        .target = target,
        .optimize = optimize,
    });

    exe.linkLibC();
    exe.addIncludePath("include");
    exe.addCSourceFiles(
        &.{
            "src/apigen.c",
            "src/io.c",
            "src/gen/c_cpp.c",
            "src/gen/rust.c",
            "src/gen/zig.c",
        },
        &.{
            "-std=c11",
            "-Werror",
            "-Wall",
            "-Wextra",
            "-Wunused-parameter",
        },
    );

    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);

    run_cmd.step.dependOn(b.getInstallStep());

    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);
}
