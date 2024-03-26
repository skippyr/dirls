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

enum CredentialType
{
    CredentialType_User,
    CredentialType_Group
};

struct Credential
{
    struct Credential *lowerCredential;
    struct Credential *higherCredential;
    uid_t id;
    char name[];
};

static struct Credential *resolveCredentialByID(struct Credential **tree, uid_t id, int isUserType);
static void deallocateCredentialsTree(struct Credential *tree);

static void *allocateHeapMemory(size_t size);
static void deallocateHeapMemory(void *allocation);
static void throwError(char *format, ...);
static void writeError(char *format, ...);
