#include <grp.h>
#include <pwd.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define PROGRAM_NAME "dirls"
#define PROGRAM_VERSION "v2.0.0"

enum CredentialType
{
    CredentialType_User,
    CredentialType_Group
};

struct Credential
{
    char *name;
    unsigned int id;
    struct Credential *lowerCredential;
    struct Credential *higherCredential;
}; /* x86_64 Size: 32B Alignment: 8B
    * x86    Size: 16B Alignment: 4B */

static struct Credential *getCredentialByID(struct Credential **tree, int type, unsigned int id);
static void deallocateCredentialTree(struct Credential *tree);

static void *allocateHeapMemory(size_t size);
static void deallocateHeapMemory(void *allocation);
static void throwError(char *format, ...);
static void writeError(char *format, ...);
