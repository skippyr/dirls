#ifdef _WIN32
#include <Windows.h>
#else
#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#define IS_ORDINAL_DAY(a_ordinal) !((systemTime.wDay - a_ordinal) % 10)
#else
#define IS_ORDINAL_DAY(a_ordinal) !((date->tm_mday - a_ordinal) % 10)
#endif
#ifdef _WIN32
#define PARSE_MODE(a_attribute, a_character) (entry.dwFileAttributes & a_attribute ? a_character : '-')
#define PARSE_OPTION(a_option, a_action) \
    if (!wcscmp(arguments[index], L"--" L##a_option)) \
    { \
        a_action; \
        return 0; \
    }
#else
#define PARSE_MODE(a_permission, a_character) (status.st_mode & a_permission ? a_character : '-')
#define PARSE_OPTION(a_option, a_action) \
    if (!strcmp(arguments[index], "--" a_option)) \
    { \
        a_action; \
        return 0; \
    }
#endif
#define PROGRAM_NAME "dirls"
#define PROGRAM_VERSION "v1.0.0"
#define PROGRAM_LICENSE "Copyright (c) 2024, Sherman Rofeman. BSD-3-Clause License."
#ifdef _WIN32
#ifdef _WIN64
#define PROGRAM_ARCHITECTURE "x86_64"
#else
#define PROGRAM_ARCHITECTURE "x86"
#endif
#else
#ifdef __x86_64__
#define PROGRAM_ARCHITECTURE "x86_64"
#else
#define PROGRAM_ARCHITECTURE "x86"
#endif
#endif
#ifdef _WIN32
#define PROGRAM_PLATFORM "Windows"
#elif __APPLE__
#define PROGRAM_PLATFORM "Apple"
#else
#define PROGRAM_PLATFORM "Linux"
#endif

static int g_return = 0;

static void *allocateMemory(size_t size);
#ifdef _WIN32
static char *convertUTF16ToUTF8(LPWSTR utf16String, int utf16Size);
#endif
static void deallocateMemory(void *allocation);
#ifdef _WIN32
static void getOwnerAndDomain(LPWSTR directoryPath, size_t directoryPathSize, LPWSTR entry, char **owner,
                              char **domain);
static char *getFullPath(LPWSTR path);
static void getSize(DWORD attributes, ULARGE_INTEGER sizeParts, char *buffer);
#else
static void getSize(struct stat *status, unsigned long sizeInBytes, char *buffer);
#endif
#ifdef _WIN32
static void readDirectory(LPWSTR utf16Path);
#else
static void readDirectory(char *path);
static int sortAlphabetically(const void *stringI, const void *stringII);
#endif
static void throwError(char *format, ...);
static void writeError(char *format, ...);
static void writeHelp(void);

static void *allocateMemory(size_t size)
{
    char *allocation = malloc(size);
    if (allocation)
    {
        return allocation;
    }
    throwError("could not allocate enough memory.");
    return NULL;
}

#ifdef _WIN32
static char *convertUTF16ToUTF8(LPWSTR utf16String, int utf16Size)
{
    int utf8Size = WideCharToMultiByte(CP_UTF8, 0, utf16String, utf16Size, NULL, 0, NULL, NULL);
    char *utf8String = allocateMemory(utf8Size);
    WideCharToMultiByte(CP_UTF8, 0, utf16String, utf16Size, utf8String, utf8Size, NULL, NULL);
    return utf8String;
}
#endif

static void deallocateMemory(void *allocation)
{
    if (allocation)
    {
        free(allocation);
    }
}

#ifdef _WIN32
static void getOwnerAndDomain(LPWSTR directory, size_t directorySize, LPWSTR entry, char **owner, char **domain)
{
    size_t entrySize = wcslen(entry) + 1;
    LPWSTR fullPath = allocateMemory((directorySize + entrySize) * 2);
    memcpy(fullPath, directory, (directorySize - 1) * 2);
    *(fullPath + directorySize - 1) = '\\';
    memcpy(fullPath + directorySize, entry, entrySize * 2);
    DWORD securityDescriptorSize = 0;
    GetFileSecurityW(fullPath, OWNER_SECURITY_INFORMATION, NULL, 0, &securityDescriptorSize);
    if (!securityDescriptorSize)
    {
        *owner = NULL;
        *domain = NULL;
        deallocateMemory(fullPath);
        return;
    }
    PSECURITY_DESCRIPTOR securityDescriptor = allocateMemory(securityDescriptorSize);
    GetFileSecurityW(fullPath, OWNER_SECURITY_INFORMATION, securityDescriptor, securityDescriptorSize,
                     &securityDescriptorSize);
    deallocateMemory(fullPath);
    PSID sid;
    BOOL isOwnerDefault;
    GetSecurityDescriptorOwner(securityDescriptor, &sid, &isOwnerDefault);
    DWORD utf16OwnerSize = 0;
    DWORD utf16DomainSize = 0;
    SID_NAME_USE use;
    LookupAccountSidW(NULL, sid, NULL, &utf16OwnerSize, NULL, &utf16DomainSize, &use);
    LPWSTR utf16Owner = allocateMemory(utf16OwnerSize * 2);
    LPWSTR utf16Domain = allocateMemory(utf16DomainSize * 2);
    LookupAccountSidW(NULL, sid, utf16Owner, &utf16OwnerSize, utf16Domain, &utf16DomainSize, &use);
    deallocateMemory(securityDescriptor);
    *owner = convertUTF16ToUTF8(utf16Owner, -1);
    deallocateMemory(utf16Owner);
    *domain = convertUTF16ToUTF8(utf16Domain, -1);
    deallocateMemory(utf16Domain);
}

static char *getFullPath(LPWSTR path)
{
    DWORD utf16FullPathSize = GetFullPathNameW(path, 0, NULL, NULL);
    LPWSTR ut16FullPath = allocateMemory(utf16FullPathSize * 2);
    GetFullPathNameW(path, utf16FullPathSize, ut16FullPath, NULL);
    char *utf8FullPath = convertUTF16ToUTF8(ut16FullPath, -1);
    deallocateMemory(ut16FullPath);
    size_t utf8FullPathSize = strlen(utf8FullPath) + 1;
    if (utf8FullPathSize > 4 && utf8FullPath[utf8FullPathSize - 2] == '\\')
    {
        utf8FullPath[utf8FullPathSize - 2] = 0;
    }
    return utf8FullPath;
}

static void getSize(DWORD attributes, ULARGE_INTEGER sizeParts, char *buffer)
#else
static void getSize(struct stat *status, unsigned long sizeInBytes, char *buffer)
#endif
{
#ifdef _WIN32
    if (attributes & FILE_ATTRIBUTE_DIRECTORY)
#else
    if (S_ISDIR(status->st_mode))
#endif
    {
        *buffer = '-';
        buffer[1] = 0;
        return;
    }
    char prefixes[] = {'G', 'M', 'k'};
    float multipliers[] = {1e9, 1e6, 1e3};
    for (int index = 0; index < 3; ++index)
    {
        float size;
#ifdef _WIN32
        if ((size = sizeParts.QuadPart / multipliers[index]) >= 1)
#else
        if ((size = sizeInBytes / multipliers[index]) >= 1)
#endif
        {
            sprintf(buffer, "%.1f%cB", size, prefixes[index]);
            return;
        }
    }
#ifdef _WIN32
    sprintf(buffer, "%zuB", sizeParts.QuadPart);
#else
    sprintf(buffer, "%ldB", sizeInBytes);
#endif
}

#ifdef _WIN32
static void readDirectory(LPWSTR utf16Path)
#else
static void readDirectory(char *path)
#endif
{
#ifdef _WIN32
    size_t utf16PathSize = wcslen(utf16Path) + 1;
    LPWSTR glob = allocateMemory((utf16PathSize + 2) * 2);
    memcpy(glob, utf16Path, (utf16PathSize - 1) * 2);
    *(glob + utf16PathSize - 1) = '\\';
    *(glob + utf16PathSize) = '*';
    *(glob + utf16PathSize + 1) = 0;
    WIN32_FIND_DATAW entry;
    HANDLE directory = FindFirstFileW(glob, &entry);
    deallocateMemory(glob);
    if (directory == INVALID_HANDLE_VALUE)
    {
        DWORD attributes = GetFileAttributesW(utf16Path);
        char *utf8Path = convertUTF16ToUTF8(utf16Path, -1);
        writeError(attributes == INVALID_FILE_ATTRIBUTES   ? "can not find the entry \"%s\".\n"
                   : attributes & FILE_ATTRIBUTE_DIRECTORY ? "can not open directory \"%s\".\n"
                                                           : "the entry \"%s\" is not a directory.\n",
                   utf8Path);
        deallocateMemory(utf8Path);
        return;
    }
    FindNextFileW(directory, &entry);
    size_t totalOfEntries;
    printf("Index  %-16s  %-14s  %-20s  %-8s  %-9s  %-5s  Name\n", "Owner", "Domain", "Modified Date", "Size", "Type",
           "Mode");
    printf("-----  ----------------  --------------  --------------------  --------  ---------  -----  "
           "----------------------------\n");
    for (totalOfEntries = 0; FindNextFileW(directory, &entry); ++totalOfEntries)
    {
        char *owner;
        char *domain;
        getOwnerAndDomain(utf16Path, utf16PathSize, entry.cFileName, &owner, &domain);
        SYSTEMTIME systemTime;
        FileTimeToSystemTime(&entry.ftLastWriteTime, &systemTime);
        char size[9];
        getSize(entry.dwFileAttributes, (ULARGE_INTEGER){entry.nFileSizeLow, entry.nFileSizeHigh}, size);
        char *name = convertUTF16ToUTF8(entry.cFileName, -1);
        printf("%5zu  %-16s  %-14s  %02d%s %s %.4d %02dh%02dm  %-8s  %-9s  %c%c%c%c%c  %s\n", totalOfEntries + 1,
               owner ? owner : "-", domain ? domain : "-", systemTime.wDay,
               IS_ORDINAL_DAY(1)   ? "st"
               : IS_ORDINAL_DAY(2) ? "nd"
               : IS_ORDINAL_DAY(3) ? "rd"
                                   : "th",
               systemTime.wMonth == 1    ? "Jan"
               : systemTime.wMonth == 2  ? "Feb"
               : systemTime.wMonth == 3  ? "Mar"
               : systemTime.wMonth == 4  ? "Apr"
               : systemTime.wMonth == 5  ? "May"
               : systemTime.wMonth == 6  ? "Jun"
               : systemTime.wMonth == 7  ? "Jul"
               : systemTime.wMonth == 8  ? "Aug"
               : systemTime.wMonth == 9  ? "Sep"
               : systemTime.wMonth == 10 ? "Oct"
               : systemTime.wMonth == 11 ? "Nov"
                                         : "Dec",
               systemTime.wYear, systemTime.wHour, systemTime.wMinute, size,
               entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? "Directory" : "File",
               PARSE_MODE(FILE_ATTRIBUTE_HIDDEN, 'h'), PARSE_MODE(FILE_ATTRIBUTE_READONLY, 'r'),
               PARSE_MODE(FILE_ATTRIBUTE_TEMPORARY, 't'), PARSE_MODE(FILE_ATTRIBUTE_ARCHIVE, 'a'),
               PARSE_MODE(FILE_ATTRIBUTE_REPARSE_POINT, 'p'), name);
        deallocateMemory(owner);
        deallocateMemory(domain);
        deallocateMemory(name);
    }
    FindClose(directory);
    if (!totalOfEntries)
    {
        printf("%68s\n", "Directory Is Empty");
    }
    printf("-----------------------------------------------------------------------------------------------------------"
           "------------\n");
    char *fullPath = getFullPath(utf16Path);
    printf("Directory: \"%s\".\n", fullPath);
    deallocateMemory(fullPath);
    printf("    Total: %zu %s.\n", totalOfEntries, totalOfEntries == 1 ? "entry" : "entries");
#else
    DIR *directory = opendir(path);
    if (!directory)
    {
        struct stat status;
        writeError(stat(path, &status)        ? "can not find the entry \"%s\".\n"
                   : !S_ISDIR(status.st_mode) ? "the entry \"%s\" is not a directory.\n"
                                              : "can not open directory \"%s\".\n",
                   path);
        return;
    }
    size_t totalOfEntries = -2;
    while (readdir(directory))
    {
        ++totalOfEntries;
    }
    printf("Index  %-8s  %-8s  %-20s  %-8s  %-9s  %-15s  Name\n", "User", "Group", "Modified Date", "Size", "Type",
           "Mode");
    printf("-----  --------  --------  --------------------  --------  ---------  ---------------  "
           "----------------------------\n");
    if (!totalOfEntries)
    {
        printf("%64s\n", "Directory Is Empty");
        goto end;
    }
    char **entries = allocateMemory(totalOfEntries * sizeof(NULL));
    rewinddir(directory);
    struct dirent *entry;
    for (size_t index = 0; (entry = readdir(directory));)
    {
        if (*entry->d_name == '.' && (!entry->d_name[1] || (entry->d_name[1] == '.' && !entry->d_name[2])))
        {
            continue;
        }
        size_t size = strlen(entry->d_name) + 1;
        char *name = allocateMemory(size);
        memcpy(name, entry->d_name, size);
        *(entries + index) = name;
        ++index;
    }
    qsort(entries, totalOfEntries, sizeof(NULL), sortAlphabetically);
    size_t pathSize = strlen(path) + 1;
    for (size_t index = 0; index < totalOfEntries; ++index)
    {
        size_t nameSize = strlen(*(entries + index)) + 1;
        char *fullPath = allocateMemory(pathSize + nameSize);
        memcpy(fullPath, path, pathSize - 1);
        *(fullPath + pathSize - 1) = '/';
        memcpy(fullPath + pathSize, *(entries + index), nameSize);
        struct stat status;
        stat(fullPath, &status);
        deallocateMemory(fullPath);
        struct passwd *user = getpwuid(status.st_uid);
        struct group *group = getgrgid(status.st_gid);
        char modifiedDate[16];
        struct tm *date = localtime(&status.st_mtime);
        strftime(modifiedDate, sizeof(modifiedDate), "%b %Y %Hh%Mm", date);
        char size[9];
        getSize(&status, status.st_size, size);
        printf("%5zu  %-8s  %-8s  %02d%s %s  %-8s  %-9s  %c%c%c%c%c%c%c%c%c (%o)  %s\n", index + 1, user->pw_name,
               group->gr_name, date->tm_mday,
               IS_ORDINAL_DAY(1)   ? "st"
               : IS_ORDINAL_DAY(2) ? "nd"
               : IS_ORDINAL_DAY(3) ? "rd"
                                   : "th",
               modifiedDate, size,
               S_ISDIR(status.st_mode)    ? "Directory"
               : S_ISREG(status.st_mode)  ? "File"
               : S_ISLNK(status.st_mode)  ? "Symlink"
               : S_ISBLK(status.st_mode)  ? "Block"
               : S_ISCHR(status.st_mode)  ? "Character"
               : S_ISFIFO(status.st_mode) ? "Fifo"
               : S_ISSOCK(status.st_mode) ? "Socket"
                                          : "Unknown",
               PARSE_MODE(S_IRUSR, 'r'), PARSE_MODE(S_IWUSR, 'w'), PARSE_MODE(S_IXUSR, 'x'), PARSE_MODE(S_IRGRP, 'r'),
               PARSE_MODE(S_IWGRP, 'w'), PARSE_MODE(S_IXGRP, 'x'), PARSE_MODE(S_IROTH, 'r'), PARSE_MODE(S_IWOTH, 'w'),
               PARSE_MODE(S_IXOTH, 'x'),
               status.st_mode &
                   (S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH),
               *(entries + index));
        deallocateMemory(*(entries + index));
    }
    deallocateMemory(entries);
end:
    closedir(directory);
    printf("-----------------------------------------------------------------------------------------------------------"
           "--------\n");
    char *fullPath = realpath(path, NULL);
    printf("Directory: \"%s\".\n", fullPath);
    deallocateMemory(fullPath);
    printf("    Total: %zu %s.\n", totalOfEntries, totalOfEntries == 1 ? "entry" : "entries");
#endif
}

#ifndef _WIN32
static int sortAlphabetically(const void *stringI, const void *stringII)
{
    return strcmp(*(char **)stringI, *(char **)stringII);
}
#endif

static void throwError(char *format, ...)
{
    fprintf(stderr, "%s: ", PROGRAM_NAME);
    va_list arguments;
    va_start(arguments, format);
    vfprintf(stderr, format, arguments);
    va_end(arguments);
    exit(1);
}

static void writeError(char *format, ...)
{
    fprintf(stderr, "%s: ", PROGRAM_NAME);
    va_list arguments;
    va_start(arguments, format);
    vfprintf(stderr, format, arguments);
    va_end(arguments);
    g_return = 1;
}

static void writeHelp(void)
{
    printf("Syntax\n");
    printf("------\n");
    printf("    %s [OPTION | PATH]...\n\n", PROGRAM_NAME);
    printf("Description\n");
    printf("-----------\n");
    printf("    List the entries inside of directories given their <PATH>s as arguments. If no directory is given, it "
           "will consider\n");
    printf("    the current directory.\n\n");
    printf("    On %s, for each entry, it shows its:\n\n", PROGRAM_PLATFORM);
#ifdef _WIN32
    printf("        - Index.\n");
    printf("        - Owner and domain.\n");
    printf("        - Modified date.\n");
    printf("        - Size (it is not a directory).\n");
    printf("        - Type (directory or file).\n");
    printf("        - Mode (hidden (h), read-only (r), temporary (t), archive (a), reparse point (p)).\n");
    printf("        - Name.\n\n");
#else
    printf("        - Index.\n");
    printf("        - User and group.\n");
    printf("        - Modified date.\n");
    printf("        - Size (it is not a directory).\n");
    printf("        - Type (directory, file, symlink, block, character, fifo or socket).\n");
    printf("        - Its permissions mode (read (r), write (w) and execute (x)) for user, group and others and its "
           "octal value.\n");
    printf("        - Name.\n\n");
#endif
    printf("Options\n");
    printf("-------\n");
    printf("    These options can be used to retrieve information about the program.\n\n");
    printf("           --help: prints these help instructions.\n");
    printf("        --version: prints its version.\n");
    printf("        --license: prints its copyright notice.\n\n");
    printf("Source Code\n");
    printf("-----------\n");
    printf("    Its source code is available at: <https://github.com/skippyr/dirls>.\n\n");
    printf("Help\n");
    printf("----\n");
    printf("    If you need help related to this project, open a new issue in its issues pages\n");
    printf("    (<https://github.com/skippyr/dirls/issues>) or send me an e-mail "
           "(<mailto:skippyr.developer@gmail.com>) describing\n");
    printf("    what is going on.\n\n");
    printf("Contributing\n");
    printf("------------\n");
    printf("    This project is open to review and possibly accept contributions, specially fixes and suggestions. If "
           "you are\n");
    printf("    interested, send your contribution to its pull requests page "
           "(<https://github.com/skippyr/dirls/pulls>) or to my\n");
    printf("    e-mail (<mailto:skippyr.developer@gmail.com>).\n\n");
    printf("    By contributing to this project, you agree to license your work under the same license that the "
           "project uses.\n\n");
    printf("License\n");
    printf("-------\n");
    printf("    This project is licensed under the BSD-3-Clause License. Refer to the LICENSE file that comes in its "
           "source code\n");
    printf("    for license and copyright details.\n");
}

#ifdef _WIN32
int main(void)
#else
int main(int totalOfArguments, char **arguments)
#endif
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    int totalOfArguments;
    LPWSTR *arguments = CommandLineToArgvW(GetCommandLineW(), &totalOfArguments);
#endif
    if (totalOfArguments == 1)
    {
#ifdef _WIN32
        readDirectory(L".");
#else
        readDirectory(".");
#endif
        return g_return;
    }
    for (int index = 1; index < totalOfArguments; ++index)
    {
        PARSE_OPTION("help", writeHelp());
        PARSE_OPTION("version", printf("%s %s (compiled for %s %s)\n", PROGRAM_NAME, PROGRAM_VERSION, PROGRAM_PLATFORM,
                                       PROGRAM_ARCHITECTURE));
        PARSE_OPTION("license", puts(PROGRAM_LICENSE));
    }
    for (int index = 1; index < totalOfArguments; ++index)
    {
        readDirectory(arguments[index]);
    }
    return g_return;
}
