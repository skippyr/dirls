#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>

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

struct EntryCache
{
    char *user;
    char *group;
    char *name;
    char *size;
    struct EntryCache *next;
    time_t modifiedEpoch;
    mode_t mode;
};

struct SIMultiplier
{
    float value;
    char prefix;
};

static struct EntryCache *allocateEntryCache(struct EntryCache **list, struct dirent *entry, struct stat *status);
static char *allocateEntryCacheSize(struct stat *status);
static struct Credential *resolveCredentialByID(struct Credential **tree, int isUserType, uid_t id);
static void deallocateCredentialsTree(struct Credential **tree);
static void deallocateEntryCache(struct EntryCache **list);

static void *allocateHeapMemory(size_t size);
static void deallocateHeapMemory(void *allocation);
static void throwError(char *format, ...);
static void writeError(char *format, ...);
