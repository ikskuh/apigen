const std = @import("std");

const lax_cflags = [_][]const u8{"-std=c11"};

const strict_cflags = lax_cflags ++ [_][]const u8{
    "-Werror",
    "-Wall",
    "-Wextra",
    "-Wunused-parameter",
    "-Wreturn-type",
    "-Wimplicit-fallthrough",
    "-Wmissing-prototypes",
    "-Wshadow",
};

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const test_step = b.step("test", "Runs the test suite");

    const lexer_gen = b.addSystemCommand(&.{
        "flex",
        "--hex", //  use hexadecimal numbers instead of octal in debug outputs
        "--8bit", // generate 8-bit scanner
        "--batch", // generate batch scanner
        "--yylineno", //  track line count in yylineno
        "--prefix=apigen_parser_", // custom prefix instead of yy
        "--reentrant", // generate a reentrant C scanner
        "--nounistd", //  do not include <unistd.h>
        "--bison-locations", // include yylloc support.
        "--bison-bridge", // scanner for bison pure parser.
    });
    const lexer_c_source = lexer_gen.addPrefixedOutputFileArg("--outfile=", "lexer.yy.c");

    lexer_gen.addArg("--header-file=zig-cache/lexer.yy.h"); // HACK UNTIL INCLUDES ARE RESOLVED
    // const lexer_h_source = lexer_gen.addPrefixedOutputFileArg("--header-file=", "lexer.yy.h");
    // _ = lexer_h_source;

    lexer_gen.addFileSourceArg(.{ .path = "src/parser/lexer.l" });

    const parser_gen = b.addSystemCommand(&.{
        "bison",
        "--language=c", // specify the output programming language
        "--locations", // enable location support
        // "--file-prefix=PREFIX", // specify a PREFIX for output files
        "-Wcounterexamples",
    });
    const parser_c_source = parser_gen.addPrefixedOutputFileArg("--output=", "parser.yy.c");

    parser_gen.addArg("--header=zig-cache/parser.yy.h"); // HACK UNTIL INCLUDES ARE RESOLVED
    // const lexer_h_source = lexer_gen.addPrefixedOutputFileArg("--header-file=", "parser.yy.h");
    // _ = lexer_h_source;

    parser_gen.addFileSourceArg(.{ .path = "src/parser/parser.y" });

    // the "flex" output depends on the header that is generated by
    // the "bison" step
    lexer_gen.step.dependOn(&parser_gen.step);

    const exe = b.addExecutable(.{
        .name = "apigen",
        .target = target,
        .optimize = optimize,
    });

    exe.addIncludePath("zig-cache"); // HACK UNTIL INCLUDES ARE RESOLVED
    exe.addIncludePath("src"); // HACK UNTIL INCLUDES ARE RESOLVED

    exe.linkLibC();
    exe.addIncludePath("include");
    exe.addCSourceFiles(
        &.{
            "src/apigen.c",
            "src/io.c",
            "src/memory.c",
            "src/base.c",
            "src/diag.c",
            "src/type-pool.c",
            "src/analyzer.c",
            "src/parser/parser.c",
            "src/gen/c_cpp.c",
            "src/gen/rust.c",
            "src/gen/zig.c",
            "src/gen/go.c",
        },
        &strict_cflags,
    );
    exe.addCSourceFileSource(.{ .source = lexer_c_source, .args = &lax_cflags });
    exe.addCSourceFileSource(.{ .source = parser_c_source, .args = &lax_cflags });

    b.installArtifact(exe);

    {
        for (parser_test_files) |test_file| {
            const run = b.addRunArtifact(exe);
            run.addArg("--test-mode=parser");
            run.addFileSourceArg(.{ .path = test_file });
            run.addCheck(.{ .expect_term = .{ .Exited = 0 } });
            run.stdin = "";
            test_step.dependOn(&run.step);
        }

        for (analyzer_test_files) |test_file| {
            const run = b.addRunArtifact(exe);
            run.addArg("--test-mode=analyzer");
            run.addFileSourceArg(.{ .path = test_file });
            run.addCheck(.{ .expect_term = .{ .Exited = 0 } });
            run.stdin = "";
            test_step.dependOn(&run.step);
        }

        {
            const test_runner = b.addExecutable(.{
                .name = "apidef-arena-test",
                .target = target,
                .optimize = .Debug, // always run tests in debug modes
            });
            test_runner.linkLibC();
            test_runner.addIncludePath("src"); // HACK UNTIL INCLUDES ARE RESOLVED
            test_runner.addIncludePath("include");
            test_runner.addCSourceFiles(
                &.{ "tests/arena.c", "src/base.c", "src/memory.c" },
                &strict_cflags,
            );

            test_step.dependOn(&b.addRunArtifact(test_runner).step);
        }
    }
}

const general_examples = [_][]const u8{
    // "examples/apigen.api",
    // "examples/pax.api",
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

    // extra
    "tests/parser/paxfuncs.api",
} ++ general_examples;

const analyzer_test_files = [_][]const u8{
    // good examples
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

    // bad examples
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
} ++ general_examples;
