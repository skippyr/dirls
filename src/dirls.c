#include "dirls.h"

static int g_exitCode = 0;

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
