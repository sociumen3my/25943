#include <sys/types.h>
#include <sys/stat.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MODE_STRING_SIZE 11
#define NAME_SIZE 32
#define DATE_SIZE 32
#define PATH_SIZE 1024

static void mode_string(mode_t mode, char *result)
{
    result[0] = S_ISDIR(mode) ? 'd' : S_ISREG(mode) ? '-' : '?';
    result[1] = (mode & S_IRUSR) != 0 ? 'r' : '-';
    result[2] = (mode & S_IWUSR) != 0 ? 'w' : '-';
    result[3] = (mode & S_IXUSR) != 0 ? 'x' : '-';
    result[4] = (mode & S_IRGRP) != 0 ? 'r' : '-';
    result[5] = (mode & S_IWGRP) != 0 ? 'w' : '-';
    result[6] = (mode & S_IXGRP) != 0 ? 'x' : '-';
    result[7] = (mode & S_IROTH) != 0 ? 'r' : '-';
    result[8] = (mode & S_IWOTH) != 0 ? 'w' : '-';
    result[9] = (mode & S_IXOTH) != 0 ? 'x' : '-';
    result[10] = '\0';
}

static void file_name(const char *path, char *result, size_t result_size)
{
    const char *start;
    const char *end;
    size_t length;

    end = path + strlen(path);
    while (end > path + 1 && end[-1] == '/')
    {
        end--;
    }

    start = end;
    while (start > path && start[-1] != '/')
    {
        start--;
    }

    length = (size_t)(end - start);
    if (length >= result_size)
    {
        length = result_size - 1;
    }

    memcpy(result, start, length);
    result[length] = '\0';
}

static int print_file(const char *path)
{
    struct stat info;
    struct passwd *owner;
    struct group *group;
    struct tm *modification_time;
    char mode[MODE_STRING_SIZE];
    char owner_name[NAME_SIZE];
    char group_name[NAME_SIZE];
    char size[NAME_SIZE];
    char date[DATE_SIZE];
    char name[PATH_SIZE];

    if (lstat(path, &info) == -1)
    {
        perror(path);
        return -1;
    }

    mode_string(info.st_mode, mode);

    owner = getpwuid(info.st_uid);
    if (owner != NULL)
    {
        snprintf(owner_name, sizeof owner_name, "%s", owner->pw_name);
    }
    else
    {
        snprintf(owner_name, sizeof owner_name, "%lu",
            (unsigned long)info.st_uid);
    }

    group = getgrgid(info.st_gid);
    if (group != NULL)
    {
        snprintf(group_name, sizeof group_name, "%s", group->gr_name);
    }
    else
    {
        snprintf(group_name, sizeof group_name, "%lu",
            (unsigned long)info.st_gid);
    }

    if (S_ISREG(info.st_mode))
    {
        snprintf(size, sizeof size, "%llu",
            (unsigned long long)info.st_size);
    }
    else
    {
        size[0] = '\0';
    }

    date[0] = '\0';
    modification_time = localtime(&info.st_mtime);
    if (modification_time == NULL ||
        strftime(date, sizeof date, "%Y-%m-%d %H:%M", modification_time) == 0)
    {
        snprintf(date, sizeof date, "?");
    }

    file_name(path, name, sizeof name);

    printf("%-10s %4lu %-12s %-12s %10s %-16s %s\n",
        mode, (unsigned long)info.st_nlink, owner_name, group_name,
        size, date, name);
    return 0;
}

int main(int argc, char **argv)
{
    int i;
    int failed;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s file ...\n", argv[0]);
        return EXIT_FAILURE;
    }

    failed = 0;
    for (i = 1; i < argc; i++)
    {
        if (print_file(argv[i]) == -1)
        {
            failed = 1;
        }
    }

    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
