#pragma once 

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

struct apigen_UnitTest {
    char const * name;
    char const * file;
    int line;
    void (*runner)(void);
    struct apigen_UnitTest * next;
};

enum apigen_ExpectFlags {
    UNITTEST_PANIC = (1U<<0),
    UNITTEST_LEAK = (1U<<1),
};

extern void apigen_unittest_add(struct apigen_UnitTest * test);

extern void apigen_unittest_expect(enum apigen_ExpectFlags flag);

#define UNITTEST_CONCAT_INNER(_A, _B) _A##_B
#define UNITTEST_CONCAT(_A, _B) UNITTEST_CONCAT_INNER(_A, _B)
#define UNITTEST_SYMBOL(_Identifier) UNITTEST_CONCAT(_Identifier, __LINE__)

#define UNITTEST(_Name)                                                                 \
    static void UNITTEST_SYMBOL(apigen_test_runner_ut)(void);                           \
    static struct apigen_UnitTest UNITTEST_SYMBOL(apigen_test_ut) APIGEN_UNUSED = {     \
        .name = (_Name),                                                                \
        .file = __FILE__,                                                               \
        .line = __LINE__,                                                               \
        .runner = UNITTEST_SYMBOL(apigen_test_runner_ut),                               \
    };                                                                                  \
    static void APIGEN_CONSTRUCTOR UNITTEST_SYMBOL(apigen_test_registrar)(void) {       \
        apigen_unittest_add(&UNITTEST_SYMBOL(apigen_test_ut));                          \
    }                                                                                   \
    static void UNITTEST_SYMBOL(apigen_test_runner_ut)(void)
