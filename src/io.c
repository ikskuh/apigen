#define _POSIX_C_SOURCE 200809L

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

static size_t apigen_io_readFile(void * context, char * string, size_t length)
{
    FILE * const file = context;

    size_t offset = 0;
    while (offset < length) {
        size_t read = fread(string + offset, 1, length - offset, file);
        if (read == 0)
            break;
        offset += read;
    }
    return offset;
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

static size_t apigen_io_readStdIn(void * context, char * string, size_t length)
{
    (void)context;
    return apigen_io_readFile(stdin, string, length);
}

static void apigen_io_writeNull(void * context, char const * string, size_t length)
{
    (void)context;
    (void)string;
    (void)length;
}

static size_t apigen_io_readNull(void * context, char * string, size_t length)
{
    (void)context;
    (void)string;
    (void)length;
    return 0;
}

struct apigen_Stream apigen_io_from_stream(FILE * file)
{
    APIGEN_NOT_NULL(file);
    return (struct apigen_Stream){
        .context = file,
        .write   = apigen_io_writeFile,
    };
}

void apigen_io_close(struct apigen_Stream * stream)
{
    APIGEN_NOT_NULL(stream);
    if(stream->close != NULL) {
        stream->close(stream->context);
    }
    memset(stream, 0xAA, sizeof *stream);
}

struct apigen_Stream const apigen_io_stdout = {.write = apigen_io_writeStdOut};
struct apigen_Stream const apigen_io_stderr = {.write = apigen_io_writeStdErr};
struct apigen_Stream const apigen_io_stdin  = {.read  = apigen_io_readStdIn};
struct apigen_Stream const apigen_io_null   = {.write = apigen_io_writeNull, .read = apigen_io_readNull};

void apigen_io_write(struct apigen_Stream stream, char const * data, size_t length)
{
    if(stream.write == NULL) apigen_panic("apigen_io_write() on read-only stream called.");
    stream.write(stream.context, data, length);
}

size_t apigen_io_read(struct apigen_Stream stream, char * data, size_t length)
{
    if(stream.read == NULL) apigen_panic("apigen_io_read() on write-only stream called.");
    return stream.read(stream.context, data, length);
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

#if defined(__WIN32__)
#error "implement win32 file i/o"
#else
// posix api will work the same everywhere:

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef intptr_t fd_t;

static bool posix_dir_openFile(void * context, enum apigen_FileMode mode, char const * file_name, struct apigen_Stream * out_stream);
static bool posix_dir_openDir(void * context, char const * file_name, struct apigen_Directory * out_dir);
static void posix_fd_write(void * context, char const * data, size_t length);
static void posix_fd_close(void * context);

static void posix_fd_write(void * context, char const * data, size_t length)
{
    fd_t const file_fd = (fd_t)context;

    size_t offset = 0;
    while(offset < length) {
        ssize_t len = write(file_fd, data + offset, length - offset);
        if(len == -1) {
            perror("failed to write");
            return;
        }
        if(len == 0) {
            return; // End of file?!
        }
        offset += len;
    }
}

static size_t posix_fd_read(void * context, char * data, size_t length)
{
    fd_t const file_fd = (fd_t)context;

    size_t offset = 0;
    while(offset < length) {
        ssize_t len = read(file_fd, data + offset, length - offset);
        if(len == -1) {
            perror("failed to write");
            return 0;
        }
        if(len == 0) {
            break;
        }
        offset += len;
    }
    return offset;
}

static bool posix_dir_openFile(void * context, enum apigen_FileMode mode, char const * file_name, struct apigen_Stream * out_dir)
{
    fd_t const dir_fd = (fd_t)context;
    
    uint32_t flags = O_CLOEXEC;
    if(mode == APIGEN_IO_OUTPUT) {
        flags |= O_RDWR;  // open read-write
        flags |= O_TRUNC; // truncate to zero
    }
    else {
        flags |= O_RDONLY;
    }

    fd_t const file_fd = openat(dir_fd, file_name, flags, 0);
    if(file_fd == -1) {
        perror("failed to open file");
        return false;
    }
    *out_dir = (struct apigen_Stream) {
        .context = (void*)file_fd,
        .write = (mode == APIGEN_IO_OUTPUT) ? posix_fd_write : NULL,
        .read  = (mode == APIGEN_IO_INPUT)  ? posix_fd_read : NULL,
        .close = posix_fd_close,
    };
    return true;
}

static bool posix_dir_openDir(void * context, char const * file_name, struct apigen_Directory * out_dir)
{
    fd_t const dir_fd = (fd_t)context;
    
    fd_t const subdir_fd = openat(dir_fd, file_name, O_DIRECTORY | O_CLOEXEC, 0);
    if(subdir_fd == -1) {
        perror("failed to open file");
        return false;
    }
    *out_dir = (struct apigen_Directory) {
        .context = (void*)subdir_fd,
        .openFile = posix_dir_openFile,
        .openDir = posix_dir_openDir,
        .close = posix_fd_close,
    };
    return true;
}

static void posix_fd_close(void * context)
{
    fd_t const fd = (fd_t)context;
    close(fd);
}


struct apigen_Directory apigen_io_cwd(void)
{
    return (struct apigen_Directory) {
        .context = (void*)AT_FDCWD,
        .openFile = posix_dir_openFile,
        .openDir = posix_dir_openDir,
        .close = NULL, // CWD isn't closable
    };
}

#endif

bool apigen_io_open_file_read(struct apigen_Directory parent, char const * path, struct apigen_Stream * out_stream)
{
    if(parent.openFile == NULL) {
        return false;
    }
    return parent.openFile(parent.context, APIGEN_IO_INPUT, path, out_stream);
}

bool apigen_io_open_file_write(struct apigen_Directory parent, char const * path, struct apigen_Stream * out_stream)
{
    if(parent.openFile == NULL) {
        return false;
    }
    return parent.openFile(parent.context, APIGEN_IO_OUTPUT, path, out_stream);
}

bool apigen_io_open_dir(struct apigen_Directory parent, char const * path, struct apigen_Directory * out_dir)
{
    if(parent.openDir == NULL) {
        return false;
    }
    return parent.openDir(parent.context, path, out_dir);
}

void apigen_io_close_dir(struct apigen_Directory * dir)
{
    APIGEN_NOT_NULL(dir);
    if(dir->close != NULL) {
        dir->close(dir->context);
    }
    memset(dir, 0xAA, sizeof *dir);
}


#if defined(__WIN32__)
#error "implement win32 file i/o"
#else
// posix api will work the same everywhere:

#include <libgen.h>

char * apigen_io_dirname(char const * path)
{
    APIGEN_NOT_NULL(path);

    size_t len = strlen(path);
    if(len < 3) 
        len = 3; // enough space for ".."

    char * inner_path = apigen_alloc(len + 1);
    memcpy(inner_path, path, len + 1);

    char const * dir = dirname(inner_path);
    APIGEN_NOT_NULL(dir);

    if(apigen_streq(dir, ".")) {
        apigen_free(inner_path);
        return NULL;
    }
    else if(apigen_streq(dir, "..")) {
        strcpy(inner_path, "..");
        return inner_path;
    }
    else {
        // printf("inner path: '%s'\n", inner_path);
        // printf("       dir: '%s'\n", dir);
        APIGEN_ASSERT(dir == inner_path); // If this ever happens, adjust implement memmove shifting of the contents.
    }

    return inner_path;
}

#endif
