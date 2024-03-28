#include "dirls.h"

static struct Credential *g_userCredentialsTree = NULL;
static struct Credential *g_groupCredentialsTree = NULL;

static int g_exitCode = 0;

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

static void readDirectory(char *directoryPath)
{
    DIR *directoryStream = opendir(directoryPath);
    if (!directoryStream)
    {
        struct stat directoryStatus;
        writeError(stat(directoryPath, &directoryStatus) ? "can not find the entry \"%s\"."
                   : S_ISDIR(directoryStatus.st_mode)    ? "can not open the directory \"%s\"."
                                                         : "the entry \"%s\" is not a directory.",
                   directoryPath);
        return;
    }
    size_t totalEntries = -2;
    while (readdir(directoryStream))
    {
        ++totalEntries;
    }
    if (!totalEntries)
    {
        closedir(directoryStream);
        return;
    }
    char **entries = allocateHeapMemory(sizeof(char *) * totalEntries);
    rewinddir(directoryStream);
    size_t index = 0;
    for (struct dirent *entry; (entry = readdir(directoryStream));)
    {
        if (*entry->d_name == '.' &&
            (!entry->d_name[1] || (entry->d_name[1] == '.' && !entry->d_name[2])))
        {
            continue;
        }
        size_t entryNameSize = strlen(entry->d_name) + 1;
        char *entryName = allocateHeapMemory(entryNameSize);
        memcpy(entryName, entry->d_name, entryNameSize);
        *(entries + index++) = entryName;
    }
    closedir(directoryStream);
    size_t directoryPathSize = strlen(directoryPath) + 1;
    for (index = 0; index < totalEntries; ++index)
    {
        char *entry = *(entries + index);
        size_t entrySize = strlen(entry) + 1;
        char *entryPath = allocateHeapMemory(directoryPathSize + entrySize);
        sprintf(entryPath, "%s/%s", directoryPath, entry);
        struct stat entryStatus;
        stat(entryPath, &entryStatus);
        deallocateHeapMemory(entryPath);
        printf("%5zu %-8s %-8s %s\n", index + 1,
               resolveCredentialByID(&g_userCredentialsTree, entryStatus.st_uid, 1)->name,
               resolveCredentialByID(&g_groupCredentialsTree, entryStatus.st_gid, 0)->name, entry);
        deallocateHeapMemory(entry);
    }
    deallocateHeapMemory(entries);
}

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

static int sortAlphabetically(const void *string0, const void *string1)
{
    return strcmp(*(char **)string0, *(char **)string1);
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

int main(int totalArguments, char **arguments)
{
    if (totalArguments == 1)
    {
        readDirectory(".");
        goto exit;
    }
    for (int index = 1; index < totalArguments; ++index)
    {
        readDirectory(arguments[index]);
    }
exit:
    deallocateCredentialsTree(g_userCredentialsTree);
    deallocateCredentialsTree(g_groupCredentialsTree);
    return g_exitCode;
}
