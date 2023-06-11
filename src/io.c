
#include "apigen.h"

#include <stdio.h>

void apigen_io_writeFile(apigen_Stream file, char const *string, size_t length)
{
    size_t offset = 0;
    while (offset < length)
    {
        size_t written = fwrite(string + offset, 1, length - offset, file);
        if (written == 0)
            apigen_panic("io failed");
        offset += written;
    }
}

void apigen_io_writeStdOut(apigen_Stream null_stream, char const *string, size_t length)
{
    (void)null_stream;
    apigen_io_writeFile((apigen_Stream)stdout, string, length);
}

void apigen_io_writeStdErr(apigen_Stream null_stream, char const *string, size_t length)
{
    (void)null_stream;
    apigen_io_writeFile((apigen_Stream)stderr, string, length);
}