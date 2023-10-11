#include "apigen.h"
#include "unittest.h"

UNITTEST("Leak")
{
    apigen_unittest_expect(UNITTEST_LEAK);
    apigen_alloc(10);
}

UNITTEST("Panic")
{
    apigen_unittest_expect(UNITTEST_PANIC);
    apigen_panic("oh no");
}

UNITTEST("Assert")
{
    apigen_unittest_expect(UNITTEST_PANIC);
    APIGEN_ASSERT(10 == 20);
}
