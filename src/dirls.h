#define _GNU_SOURCE
#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <time.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROGRAM_NAME "dirls"
#define PROGRAM_VERSION "v2.0.0"

struct Credential
{
    struct Credential *lowerCredential;
    struct Credential *higherCredential;
    uid_t id;
    char name[];
};

struct SIMultiplier
{
    float value;
    char prefix;
};

static void deallocateCredentialsTree(struct Credential *tree);
static void formatEntryMode(char *buffer, struct stat *status);
static void formatEntrySize(char *buffer, struct stat *status);
static void readDirectory(char *directoryPath);
static struct Credential *resolveCredentialByID(struct Credential **tree, uid_t id, int isUserType);
static int sortAlphabetically(const void *string0, const void *string1);

static void *allocateHeapMemory(size_t size);
static void deallocateHeapMemory(void *allocation);
static void throwError(char *format, ...);
static void writeError(char *format, ...);
