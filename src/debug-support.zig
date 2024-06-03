//!
//! This file is not necessary to use apigen, but is compiled in when building with Zig
//! so additional crash diagnostic capabilities are unlocked.
//!
//!
//! "Inspired" by:
//! https://gist.github.com/andrewrk/752a1f4db1a3327ee21f684cc87026e7
//!

const std = @import("std");

export fn apigen_enable_debug_diagnostics() void {
    std.debug.maybeEnableSegfaultHandler();
}

export fn apigen_dump_stack_trace() void {
    std.debug.dumpCurrentStackTrace(@returnAddress());
}

/// Communicate to zig that main() will be provided elsewhere.
pub const _start = {};
