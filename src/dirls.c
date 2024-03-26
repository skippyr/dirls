#include "dirls.h"

static int g_exitCode = 0;

static void *allocateHeapMemory(size_t size)
{
    void *allocation = malloc(size);
    if (allocation)
    {
        return allocation;
    }
    throwError("can not allocate %zu bytes of memory on the heap.", size);
    return NULL;
}

static void deallocateHeapMemory(void *allocation)
{
    if (allocation)
    {
        free(allocation);
    }
}

static void throwError(char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    fprintf(stderr, "%s: ", PROGRAM_NAME);
    vfprintf(stderr, format, arguments);
    fprintf(stderr, "\n");
    va_end(arguments);
    exit(1);
}

static void writeError(char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    fprintf(stderr, "%s: ", PROGRAM_NAME);
    vfprintf(stderr, format, arguments);
    fprintf(stderr, "\n");
    va_end(arguments);
    g_exitCode = 1;
}

int main(void)
{
    return g_exitCode;
}
