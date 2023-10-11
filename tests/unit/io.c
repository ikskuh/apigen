#include "apigen.h"
#include "unittest.h"

#define CTX "I/O: "

UNITTEST(CTX "get cwd")
{
    struct apigen_Directory dir = apigen_io_cwd();

    (void)dir;
}


UNITTEST(CTX "open self")
{
    struct apigen_Directory cwd = apigen_io_cwd();

    struct apigen_Directory dir;
    APIGEN_ASSERT( apigen_io_open_dir(cwd, ".", &dir) );
    apigen_io_close_dir(&dir);
}

