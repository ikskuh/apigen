# APIGEN - API Definition Language

## Features

Supports the following types

- `void` (only available as a return type, means the function returns nothing)
- `bool` => `stdbool.h` (system equivalent to `_Bool` from C)
- `u8` => `uint8_t`
- `u16` => `uint16_t`
- `u32` => `uint32_t`
- `u64` => `uint64_t`
- `usize` => `uintptr_t`
- `i8` => `int8_t`
- `i16` => `int16_t`
- `i32` => `int32_t`
- `i64` => `int64_t`
- `isize` => `intptr_t`
- `*T` => `*T` (non-nullable pointer to one)
- `?*T` => `*T` (nullable pointer to one)
- `[*]T` => `*T` (non-nullable pointer-to-many)
- `?[*]T` => `*T` (nullable pointer-to-many)
- `[*:X]T` => `*T` (non-nullable pointer-to-many terminated by X)
- `?[*:X]T` => `*T` (nullable pointer-to-many terminated by X)
- `[N]T` => `T[N]` (array of N elements of T, must be wrapped as a struct in C)
- structs
- unions
- enums
- opaque
- `c_int`
- `c_uint`
- `c_short`
- `c_ushort`
- `c_char`
- `c_uchar`
- `c_ichar`
- `c_long`
- `c_ulong`
- `c_longlong`
- `c_ulonglong`

Can declare API functions, variables, constants and types
