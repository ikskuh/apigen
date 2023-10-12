#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include "apigen.h"
#include "apigen-internals.h"
#include "unittest.h"

#define C_BLK "\x1B[0;30m"
#define C_RED "\x1B[0;31m"
#define C_GRN "\x1B[0;32m"
#define C_YEL "\x1B[0;33m"
#define C_BLU "\x1B[0;34m"
#define C_MAG "\x1B[0;35m"
#define C_CYN "\x1B[0;36m"
#define C_WHT "\x1B[0;37m"
#define C_DIM "\x1B[0;2m"
#define C_OFF "\x1B[0m"

static size_t allocation_count = 0;
static void * test_malloc(size_t size)
{
    void * ptr = malloc(size);
    if(ptr != NULL) {
        allocation_count += 1;
    }
    return ptr;
}

static void test_free(void *ptr)
{
    if(ptr) {
        APIGEN_ASSERT(allocation_count > 0);
        allocation_count -= 1;
    }
    free(ptr);
}

static size_t registered_tests_count = 0;
static struct apigen_UnitTest * registered_tests;

void apigen_unittest_add(struct apigen_UnitTest * test)
{
    APIGEN_NOT_NULL(test);
    test->next = registered_tests;
    registered_tests = test;

    registered_tests_count += 1;
}


static jmp_buf apigen_unittest_returnpoint;

enum {
    SETJMP_INIT = 0,
    SETJMP_PANIC = 1,
};

static uint32_t expected_problems = 0;

static bool problem_expected(enum apigen_ExpectFlags flag)
{
    return !!(expected_problems & flag);
}

extern void apigen_unittest_expect(enum apigen_ExpectFlags flag)
{
    expected_problems |= flag;
}

void APIGEN_NORETURN APIGEN_WEAK apigen_panic(char const *msg)
{
    if(!problem_expected(UNITTEST_PANIC)) {
        (void)fprintf(stderr, C_RED "PANIC:" C_OFF " %s", msg);

        apigen_dump_stack_trace();
    }
    longjmp(apigen_unittest_returnpoint, SETJMP_PANIC);    
}

static bool invoke_test(struct apigen_UnitTest * test)
{
    apigen_memory_alloc_backend = test_malloc;
    apigen_memory_free_backend = test_free;
    allocation_count = 0;
    expected_problems = 0;

    fprintf(stderr, "\r" C_DIM "[%-30s]" C_OFF " " C_YEL "RUNNING...  " C_OFF, test->name);

    bool ok = false;

    int error = setjmp(apigen_unittest_returnpoint);
    switch(error) {
        case SETJMP_INIT:
            // initial result
            test->runner();

            
            if(problem_expected(UNITTEST_LEAK)) {
                if(allocation_count == 0) {
                    fprintf(stderr, C_RED "EXPECTED MEMORY LEAK MISSING!" C_OFF);
                }
                else {
                    ok = true;
                }
            }
            else {
                if(allocation_count > 0) {
                    fprintf(stderr, C_RED "MEMORY LEAK DETECTED!" C_OFF);
                }
                else {
                    ok = true;
                }
            }

            
            break;
        case SETJMP_PANIC:
            ok = problem_expected(UNITTEST_PANIC);
            break;
    }


    if(ok) {
        fprintf(stderr, "\r[" C_DIM "%-30s" C_OFF "] " C_GRN "OK        " C_OFF "\n", test->name);
    }
    else {
        fprintf(stderr, "\r[" C_DIM "%-30s" C_OFF "] " C_RED "FAILED    " C_OFF "\n", test->name);
    }
    
    apigen_memory_alloc_backend = NULL;
    apigen_memory_free_backend = NULL;

    return ok;
}

int main(int argc, char ** argv)
{
    (void)argc;
    (void)argv;

    apigen_enable_debug_diagnostics();

    size_t ok_count = 0;

    struct apigen_UnitTest * test = registered_tests;
    while(test) {
        if(invoke_test(test)) {
            ok_count += 1;
        }
        test =test->next;
    }
    
    fprintf(stderr, "%zu of %zu tests successful\n", ok_count, registered_tests_count);

    return (ok_count == registered_tests_count) ? EXIT_SUCCESS : EXIT_FAILURE;
}
