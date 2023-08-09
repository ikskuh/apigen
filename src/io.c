
#include "apigen.h"

#include <stdio.h>
#include <string.h>

static void apigen_io_writeFile(void * context, char const * string, size_t length)
{
    FILE * const file = context;

    size_t offset = 0;
    while (offset < length) {
        size_t written = fwrite(string + offset, 1, length - offset, file);
        if (written == 0)
            apigen_panic("io failed");
        offset += written;
    }
}

static void apigen_io_writeStdOut(void * context, char const * string, size_t length)
{
    (void)context;
    apigen_io_writeFile(stdout, string, length);
}

static void apigen_io_writeStdErr(void * context, char const * string, size_t length)
{
    (void)context;
    apigen_io_writeFile(stderr, string, length);
}

static void apigen_io_writeNull(void * context, char const * string, size_t length)
{
    (void)context;
    (void)string;
    (void)length;
}

struct apigen_Stream apigen_io_file_stream(FILE * file)
{
    APIGEN_NOT_NULL(file);
    return (struct apigen_Stream){
        .context = file,
        .write   = apigen_io_writeFile,
    };
}

struct apigen_Stream const apigen_io_stdout = {.write = apigen_io_writeStdOut};
struct apigen_Stream const apigen_io_stderr = {.write = apigen_io_writeStdErr};
struct apigen_Stream const apigen_io_null   = {.write = apigen_io_writeNull};

void apigen_io_write(struct apigen_Stream stream, char const * data, size_t length)
{
    APIGEN_NOT_NULL(stream.write);
    stream.write(stream.context, data, length);
}

void apigen_io_print(struct apigen_Stream stream, char const * data)
{
    apigen_io_write(stream, data, strlen(data));
}

void apigen_io_printf(struct apigen_Stream stream, char const * format, ...)
{
    va_list list;
    va_start(list, format);
    apigen_io_vprintf(stream, format, list);
    va_end(list);
}

void apigen_io_vprintf(struct apigen_Stream stream, char const * format, va_list list)
{
    char fixed_buffer[1024];

    int const top_res = vsnprintf(fixed_buffer, sizeof fixed_buffer, format, list);
    if (top_res >= 0) {
        apigen_io_write(stream, fixed_buffer, (size_t)top_res);
        return;
    }

    int const dynamic_length = vsnprintf(NULL, 0, format, list);
    APIGEN_ASSERT(dynamic_length >= 0);

    char * const dynamic_buffer = apigen_alloc((size_t)dynamic_length);

    int const length = vsnprintf(dynamic_buffer, (size_t)dynamic_length, format, list);
    APIGEN_ASSERT(length == dynamic_length);

    apigen_io_write(stream, dynamic_buffer, (size_t)length);

    apigen_free(dynamic_buffer);
}
