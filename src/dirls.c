#include "dirls.h"

static struct Credential *g_userCredentialsTree = NULL;
static struct Credential *g_groupCredentialsTree = NULL;
static struct EntryCache *g_entryCacheList = NULL;

static int g_exitCode = 0;

static void allocateEntryCache(struct EntryCache **list, struct dirent *entry, struct stat *status)
{
    struct EntryCache *allocatedCache = allocateHeapMemory(sizeof(struct EntryCache));
    allocatedCache->user = resolveCredentialByID(&g_userCredentialsTree, 1, status->st_uid)->name;
    allocatedCache->group = resolveCredentialByID(&g_groupCredentialsTree, 0, status->st_gid)->name;
    allocatedCache->mode = status->st_mode;
    allocatedCache->modifiedEpoch = status->st_mtim.tv_sec;
    size_t nameSize = strlen(entry->d_name) + 1;
    allocatedCache->name = allocateHeapMemory(nameSize);
    memcpy(allocatedCache->name, entry->d_name, nameSize);
    allocatedCache->size = allocateEntryCacheSize(status);
    allocatedCache->next = NULL;
    if (!*list)
    {
        *list = allocatedCache;
        return;
    }
    struct EntryCache *listCache;
    for (listCache = *list; listCache->next; listCache = listCache->next)
    {
    }
    listCache->next = allocatedCache;
}

static char *allocateEntryCacheSize(struct stat *status)
{
    struct SIMultiplier multipliers[] = {{1073741824, 'G'}, {1048576, 'M'}, {1024, 'k'}};
    char format[9] = "";
    for (size_t index = 0; index < 3; ++index)
    {
        if (status->st_size >= multipliers[index].value)
        {
            float size = status->st_size / multipliers[index].value;
            sprintf(format, "%.1f%cB", size, multipliers[index].prefix);
            break;
        }
    }
    if (!*format)
    {
        sprintf(format, "%ldB", status->st_size);
    }
    size_t formatSize = strlen(format) + 1;
    char *size = allocateHeapMemory(formatSize);
    memcpy(size, format, formatSize);
    return size;
}

static struct Credential *resolveCredentialByID(struct Credential **tree, int isUserType, uid_t id)
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

static void deallocateCredentialsTree(struct Credential **tree)
{
    if (!*tree)
    {
        return;
    }
    if ((*tree)->lowerCredential)
    {
        deallocateCredentialsTree(&(*tree)->lowerCredential);
    }
    if ((*tree)->higherCredential)
    {
        deallocateCredentialsTree(&(*tree)->higherCredential);
    }
    deallocateHeapMemory(*tree);
    *tree = NULL;
}

static void deallocateEntryCache(struct EntryCache **list)
{
    if (!*list)
    {
        return;
    }
    struct EntryCache *nextCache;
    for (struct EntryCache *currentCache = *list; currentCache; currentCache = nextCache)
    {
        nextCache = currentCache->next;
        deallocateHeapMemory(currentCache->name);
        deallocateHeapMemory(currentCache->size);
        deallocateHeapMemory(currentCache);
    }
    *list = NULL;
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
