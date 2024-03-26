#include "dirls.h"

static struct Credential *g_userCredentialsTree = NULL;
static struct Credential *g_groupCredentialsTree = NULL;

static int g_exitCode = 0;

static struct Credential *resolveCredentialByID(struct Credential **tree, uid_t id, int isUserType)
{
    struct Credential **node = tree;
    while (*node)
    {
        if ((*node)->id == id)
        {
            return *node;
        }
        else if (id < (*node)->id)
        {
            node = &(*node)->lowerCredential;
        }
        else if (id > (*node)->id)
        {
            node = &(*node)->higherCredential;
        }
    }
    char *name;
    if (isUserType)
    {
        struct passwd *user = getpwuid(id);
        if (!user)
        {
            return NULL;
        }
        name = user->pw_name;
    }
    else
    {
        struct group *group = getgrgid(id);
        if (!group)
        {
            return NULL;
        }
        name = group->gr_name;
    }
    size_t nameSize = strlen(name) + 1;
    *node = allocateHeapMemory(sizeof(struct Credential) + nameSize);
    (*node)->lowerCredential = NULL;
    (*node)->higherCredential = NULL;
    (*node)->id = id;
    memcpy((*node)->name, name, nameSize);
    return *node;
}

static void deallocateCredentialsTree(struct Credential *tree)
{
    if (!tree)
    {
        return;
    }
    if (tree->lowerCredential)
    {
        deallocateCredentialsTree(tree->lowerCredential);
    }
    if (tree->higherCredential)
    {
        deallocateCredentialsTree(tree->higherCredential);
    }
    deallocateHeapMemory(tree);
}

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
