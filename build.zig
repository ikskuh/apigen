const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const flex_dep = b.dependency("flex", .{});
    const flex = flex_dep.artifact("flex");

    const test_step = b.step("test", "Runs the test suite");

    const gen_step = b.step("generate", "Generates sources and headers with flex and bison.");
    const bundle_step = b.step("bundle", "Bundles all required sources and headers into zig-out.");

    const unittest_step = b.step("unittest", "Runs the unit tests");

    const lexer_gen = b.addRunArtifact(flex);
    lexer_gen.addArgs(&.{
        "--hex", //  use hexadecimal numbers instead of octal in debug outputs
    });
    const lexer_c_source = lexer_gen.addPrefixedOutputFileArg("--outfile=", "lexer.yy.c");
    const lexer_h_source = lexer_gen.addPrefixedOutputFileArg("--header-file=", "lexer.yy.h");
    lexer_gen.addFileSourceArg(.{ .path = "src/parser/lexer.l" });

    const parser_gen = b.addSystemCommand(&.{
        "bison", // TODO: Make bison package so we can get rid of that system dependency!
        "--language=c", // specify the output programming language
        "--locations", // enable location support
        // "--file-prefix=PREFIX", // specify a PREFIX for output files
        "-Wcounterexamples",
    });
    const parser_c_source = parser_gen.addPrefixedOutputFileArg("--output=", "parser.yy.c");
    const parser_h_source = parser_gen.addPrefixedOutputFileArg("--header=", "parser.yy.h");
    parser_gen.addFileSourceArg(.{ .path = "src/parser/parser.y" });

    // the "flex" output depends on the header that is generated by
    // the "bison" step
    lexer_gen.step.dependOn(&parser_gen.step);

    // Also install the C sources and headers if requested:
    {
        gen_step.dependOn(&b.addInstallFile(lexer_c_source, "src/parser/lexer.yy.c").step);
        gen_step.dependOn(&b.addInstallFile(lexer_h_source, "include/lexer.yy.h").step);
        gen_step.dependOn(&b.addInstallFile(parser_c_source, "src/parser/parser.yy.c").step);
        gen_step.dependOn(&b.addInstallFile(parser_h_source, "include/parser.yy.h").step);
    }

    // If requested
    {
        bundle_step.dependOn(gen_step);
        for (apigen_sources) |src_file| {
            std.debug.assert(std.mem.startsWith(u8, src_file, "src/"));
            bundle_step.dependOn(&b.addInstallFile(.{ .path = src_file }, src_file).step);
        }

        bundle_step.dependOn(&b.addInstallFile(.{ .path = "include/apigen.h" }, "include/apigen.h").step); // public header
        bundle_step.dependOn(&b.addInstallFile(.{ .path = "src/apigen-internals.h" }, "include/apigen-internals.h").step); // public header
        bundle_step.dependOn(&b.addInstallFile(.{ .path = "src/parser/parser.h" }, "src/parser/parser.h").step); // internal header
    }

    const exe = b.addExecutable(.{
        .name = "apigen",
        .target = target,
        .optimize = optimize,
        .root_source_file = .{ .path = "src/debug-support.zig" },
    });

    exe.addIncludePath(BuildHelper.getPathDir(lexer_h_source));
    exe.addIncludePath(BuildHelper.getPathDir(parser_h_source));

    exe.linkLibC();
    exe.addIncludePath(.{ .path = "include" });
    exe.addCSourceFiles(&apigen_sources, &strict_cflags);

    // both require access to "parser.h":
    const local_include = [_][]const u8{ "-I", b.pathFromRoot("src") };
    exe.addCSourceFile(.{ .file = lexer_c_source, .flags = &lax_cflags ++ local_include });
    exe.addCSourceFile(.{ .file = parser_c_source, .flags = &lax_cflags ++ local_include });

    b.installArtifact(exe);

    {
        for (parser_test_files) |test_file| {
            const run = b.addRunArtifact(exe);
            run.addArg("--test-mode=parser");
            run.addFileSourceArg(.{ .path = test_file });
            run.addCheck(.{ .expect_term = .{ .Exited = 0 } });
            run.stdin = .{ .bytes = "" };
            run.has_side_effects = true;
            test_step.dependOn(&run.step);
        }

        for (analyzer_test_files) |test_file| {
            const run = b.addRunArtifact(exe);
            run.addArg("--test-mode=analyzer");
            run.addFileSourceArg(.{ .path = test_file });
            run.addCheck(.{ .expect_term = .{ .Exited = 0 } });
            run.stdin = .{ .bytes = "" };
            run.has_side_effects = true;
            test_step.dependOn(&run.step);
        }

        const BackendLang = enum { c, @"c++", rust, zig, go };

        const enabled_backends = [_]BackendLang{ .c, .zig };

        for (backend_test_files) |test_file| {
            for (enabled_backends) |backend| {
                const target_lang_ext = switch (backend) {
                    .c => ".c",
                    .@"c++" => ".cpp",
                    .rust => ".rs",
                    .zig => ".zig",
                    .go => ".go",
                };

                const basename = std.fs.path.basename(test_file);
                const ext_len = std.fs.path.extension(basename);
                const temp_obj_name = b.fmt("{s}-{s}", .{
                    basename[0 .. basename.len - ext_len.len],
                    @tagName(backend),
                });
                const generated_file_name = b.fmt("test-{s}{s}", .{ basename, target_lang_ext });

                const run = b.addRunArtifact(exe);
                run.addArg("--language");
                run.addArg(@tagName(backend));
                run.addArg("--output");
                const generated_source = run.addOutputFileArg(generated_file_name);
                run.addFileSourceArg(.{ .path = test_file });
                run.addCheck(.{ .expect_term = .{ .Exited = 0 } });
                run.addCheck(.{ .expect_stderr_exact = "" });

                switch (backend) {
                    .c => {
                        const obj_build = b.addObject(.{
                            .name = temp_obj_name,
                            .target = .{},
                            .optimize = .Debug,
                        });
                        obj_build.linkLibC();

                        obj_build.addCSourceFile(.{
                            .file = generated_source,
                            .flags = &.{},
                        });

                        test_step.dependOn(&obj_build.step);
                    },

                    .zig => {
                        const obj_build = b.addObject(.{
                            .name = temp_obj_name,
                            .root_source_file = generated_source,
                            .target = .{},
                            .optimize = .Debug,
                        });
                        obj_build.linkLibC();

                        test_step.dependOn(&obj_build.step);
                    },

                    .@"c++" => @panic("c++ backend has no tests yet!"),
                    .rust => @panic("rust backend has no tests yet!"),
                    .go => @panic("go backend has no tests yet!"),
                }
            }
        }

        {
            const test_runner = b.addExecutable(.{
                .name = "apidef-unit-test",
                .target = target,
                .optimize = .Debug, // always run tests in debug modes
                .root_source_file = .{ .path = "src/debug-support.zig" },
            });
            test_runner.linkLibC();
            test_runner.addIncludePath(.{ .path = "include" });
            test_runner.addIncludePath(.{ .path = "src" }); // unit tests may access internals
            test_runner.addCSourceFiles(
                &.{
                    "tests/unit/test-runner.c",
                    "tests/unit/arena.c",
                    "tests/unit/framework.c",
                    "tests/unit/io.c",

                    "src/base.c",
                    "src/memory.c",
                    "src/io.c",
                },
                &strict_cflags,
            );

            const unit_test_run = b.addRunArtifact(test_runner);
            unit_test_run.has_side_effects = true;

            test_step.dependOn(&unit_test_run.step);
            unittest_step.dependOn(&unit_test_run.step);
        }
    }
}

const apigen_sources = [_][]const u8{
    "src/apigen.c",
    "src/args.c",
    "src/io.c",
    "src/memory.c",
    "src/base.c",
    "src/diag.c",
    "src/type-pool.c",
    "src/test-runner.c",
    "src/analyzer.c",
    "src/parser/parser.c",
    "src/gen/c_cpp.c",
    "src/gen/rust.c",
    "src/gen/zig.c",
    "src/gen/go.c",
};

const general_examples = [_][]const u8{
    "examples/apigen.api",
    "examples/pax.api",
};

const parser_test_files = [_][]const u8{
    "tests/parser/primitives.api",
    "tests/parser/constexpr.api",
    "tests/parser/arrays.api",
    "tests/parser/pointers.api",
    "tests/parser/vars-and-consts.api",
    "tests/parser/opaques.api",

    // compounds:
    "tests/parser/functions.api",
    "tests/parser/enums.api",
    "tests/parser/structs.api",
    "tests/parser/unions.api",

    // doc comments:
    "tests/parser/functiondocs.api",
    "tests/parser/enumdocs.api",
    "tests/parser/structdocs.api",
    "tests/parser/uniondocs.api",
    "tests/parser/empty-doc.api",

    // extra
    "tests/parser/paxfuncs.api",
} ++ general_examples ++ analyzer_positive_files;

const analyzer_positive_files = [_][]const u8{
    "tests/analyzer/ok/empty.api",
    "tests/analyzer/ok/nested-types.api",
    "tests/analyzer/ok/forward-ref.api",
    "tests/analyzer/ok/arrays.api",
    "tests/analyzer/ok/pointers.api",
    "tests/analyzer/ok/backward-ref.api",
    "tests/analyzer/ok/structs.api",
    "tests/analyzer/ok/enums.api",
    "tests/analyzer/ok/var-decl.api",
    "tests/analyzer/ok/const-decl.api",
    "tests/analyzer/ok/early-ref-unique.api",
    "tests/analyzer/ok/unions.api",
    "tests/analyzer/ok/interning.api",
    "tests/analyzer/ok/func.api",
    "tests/analyzer/ok/builtin.api",
    "tests/analyzer/ok/constexpr-strings.api",
    "tests/analyzer/ok/constexpr-decl.api",
    "tests/analyzer/ok/enum-negative-nums.api",
    "tests/analyzer/ok/include.api",
    "tests/analyzer/ok/nested-include.api",
    "tests/analyzer/ok/empty-include.api",
    "tests/analyzer/ok/fn-with-alias-type.api",
};

const analyzer_negative_files = [_][]const u8{
    "tests/analyzer/fail/duplicate-symbol-uniq-triv.api",
    "tests/analyzer/fail/duplicate-symbol-triv-uniq.api",
    "tests/analyzer/fail/constexpr-value-mismatch.api",
    "tests/analyzer/fail/enum-out-of-range-unsigned.api",
    "tests/analyzer/fail/enum-empty-backing.api",
    "tests/analyzer/fail/enum-duplicate-value.api",
    "tests/analyzer/fail/enum-out-of-range-signed.api",
    "tests/analyzer/fail/constexpr-value-range.api",
    "tests/analyzer/fail/duplicate-symbol-two-unique.api",
    "tests/analyzer/fail/enum-duplicate-item.api",
    "tests/analyzer/fail/struct-invalid-field-type.api",
    "tests/analyzer/fail/struct-empty.api",
    "tests/analyzer/fail/duplicate-parameter.api",
    "tests/analyzer/fail/struct-duplicate-field.api",
    "tests/analyzer/fail/enum-str-item.api",
    "tests/analyzer/fail/enum-backing-non-int.api",
    "tests/analyzer/fail/enum-empty.api",
    "tests/analyzer/fail/enum-backing-unspec-int.api",
    "tests/analyzer/fail/duplicate-symbol-two-trivial.api",
    "tests/analyzer/fail/nested-struct-bad.api",
    "tests/analyzer/fail/union-empty.api",
    "tests/analyzer/fail/constexpr-type-unsupported.api",
};

const lax_cflags = [_][]const u8{"-std=c11"};

const strict_cflags = lax_cflags ++ [_][]const u8{
    "-pedantic",
    "-Werror",
    "-Wall",
    "-Wextra",
    "-Wunused-parameter",
    "-Wreturn-type",
    "-Wimplicit-fallthrough",
    "-Wmissing-prototypes",
    "-Wshadow",
    "-Wmissing-variable-declarations", // this is most likely a bug, otherwise it can be explicitly solved locally
    "-Wextra-semi-stmt", // this is just clean code
    "-Wformat-extra-args", // definitly a bug when it happens
    "-Wunreachable-code", // definitly a bug when it happens

    // "-Weverything", // Enable when changing larger pieces of the code base and triage

    // we don't care for those warnings:
    "-Wno-declaration-after-statement", // writing C11 code
    "-Wno-c++98-compat", // writing C11 code
    "-Wno-padded", // This is a purely informational warning and provides no benefit for us
    "-Wno-unsafe-buffer-usage", // We're writing C, not C++!
    "-Wno-switch-enum", // stupidly, it isn't muted when a "default:" is preset
};

const analyzer_test_files = analyzer_positive_files ++ analyzer_negative_files ++ general_examples;

const backend_test_files = analyzer_positive_files ++ general_examples;

const BuildHelper = struct {
    pub fn getPathDir(path: std.Build.LazyPath) std.Build.LazyPath {
        const ComputeStep = struct {
            step: std.Build.Step,
            input: std.build.LazyPath,
            output: std.build.GeneratedFile,

            fn make(step: *std.Build.Step, progress: *std.Progress.Node) !void {
                _ = progress;

                const self = @fieldParentPtr(@This(), "step", step);

                const realpath = self.input.getPath2(step.owner, step);

                self.output.path = resolve(realpath);
            }

            fn resolve(realpath: []const u8) []const u8 {
                return std.fs.path.dirname(realpath) orelse if (std.fs.path.isAbsolute(realpath)) "/" else ".";
            }
        };

        switch (path) {
            .cwd_relative => |value| return std.Build.LazyPath{ .cwd_relative = ComputeStep.resolve(value) },
            .path => |value| return std.Build.LazyPath{ .path = ComputeStep.resolve(value) },

            .generated => |ptr| {
                const child = ptr.step.owner.allocator.create(ComputeStep) catch @panic("oom");
                child.* = ComputeStep{
                    .input = path,
                    .output = .{ .step = &child.step },
                    .step = std.Build.Step.init(.{
                        .id = .custom,
                        .name = "dirname",
                        .owner = ptr.step.owner,
                        .makeFn = ComputeStep.make,
                        .first_ret_addr = @returnAddress(),
                    }),
                };
                path.addStepDependencies(&child.step);
                return std.Build.LazyPath{ .generated = &child.output };
            },
        }
    }
};
