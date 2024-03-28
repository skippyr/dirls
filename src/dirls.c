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

static void formatEntryMode(char *buffer, struct stat *status)
{
    char characters[] = {'r', 'w', 'x'};
    int flags[] = {S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP, S_IXGRP, S_IROTH, S_IWOTH, S_IXOTH};
    switch (status->st_mode & S_IFMT)
    {
    case S_IFREG:
        *buffer = '-';
        break;
    case S_IFDIR:
        *buffer = 'd';
        break;
    case S_IFLNK:
        *buffer = 'l';
        break;
    case S_IFBLK:
        *buffer = 'b';
        break;
    case S_IFCHR:
        *buffer = 'c';
        break;
    case S_IFIFO:
        *buffer = 'f';
        break;
    case S_IFSOCK:
        *buffer = 's';
        break;
    }
    for (size_t index = 0; index < 9; index++)
    {
        buffer[index + 1] =
            status->st_mode & flags[index] ? characters[index < 3 ? index : (index - 3) % 3] : '-';
    }
    buffer[10] = 0;
}

static void formatEntrySize(char *buffer, struct stat *status)
{
    struct SIMultiplier multipliers[] = {{1073741824, 'G'}, {1048576, 'M'}, {1024, 'k'}};
    for (size_t index = 0; index < 3; ++index)
    {
        if (status->st_size >= multipliers[index].value)
        {
            float size = status->st_size / multipliers[index].value;
            sprintf(buffer, "%.1f%cB", size, multipliers[index].prefix);
            return;
        }
    }
    sprintf(buffer, "%ldB", status->st_size);
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
    char **entryNames = allocateHeapMemory(sizeof(char *) * totalEntries);
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
        *(entryNames + index++) = entryName;
    }
    closedir(directoryStream);
    qsort(entryNames, totalEntries, sizeof(char *), sortAlphabetically);
    size_t directoryPathSize = strlen(directoryPath) + 1;
    for (index = 0; index < totalEntries; ++index)
    {
        char *entryName = *(entryNames + index);
        size_t entryNameSize = strlen(entryName) + 1;
        char *entryPath = allocateHeapMemory(directoryPathSize + entryNameSize);
        sprintf(entryPath, "%s/%s", directoryPath, entryName);
        struct stat entryStatus;
        stat(entryPath, &entryStatus);
        deallocateHeapMemory(entryPath);
        char entrySize[9];
        char entryMode[11];
        formatEntrySize(entrySize, &entryStatus);
        formatEntryMode(entryMode, &entryStatus);
        printf("%5zu %-8s %-8s %-8s %s %s\n", index + 1,
               resolveCredentialByID(&g_userCredentialsTree, entryStatus.st_uid, 1)->name,
               resolveCredentialByID(&g_groupCredentialsTree, entryStatus.st_gid, 0)->name,
               entrySize, entryMode, entryName);
        deallocateHeapMemory(entryName);
    }
    deallocateHeapMemory(entryNames);
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
