#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/resource.h>
#include <dirent.h>
#include <errno.h>

#ifndef RLIMIT_NPROC
#define RLIMIT_NPROC 6
#endif
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ulimit.h>

extern char **environ;

static int number(char *text, long *value)
{
    char *end;

    errno = 0;
    *value = strtol(text, &end, 10);

    return *text != '\0' && *text != '-' && *end == '\0' && errno != ERANGE;
}

static void usage(const char *name)
{
    printf("Usage: %s [options]\n", name);
    printf("  -i             print uid, euid, gid, egid\n");
    printf("  -s             become the leader of a new process group\n");
    printf("  -p             print pid, ppid, pgrp\n");
    printf("  -u             print the max number of processes the user can still create\n");
    printf("  -U n           set the file size limit to n bytes\n");
    printf("  -c             print the core dump size limit\n");
    printf("  -C n           set the core dump size limit to n bytes\n");
    printf("  -d             print the current working directory\n");
    printf("  -v             print the environment\n");
    printf("  -V name=value  set an environment variable\n");
}

static int line_uid(const char *line, uid_t *uid)
{
    int value;

    if (sscanf(line, "Uid: %d", &value) == 1 ||
        sscanf(line, "uid: %d", &value) == 1 ||
        sscanf(line, "UID: %d", &value) == 1)
    {
        *uid = (uid_t)value;
        return 1;
    }

    return 0;
}

static int count_processes(const char *root, uid_t uid)
{
    DIR *dir;
    struct dirent *entry;
    char path[PATH_MAX + 1];
    char line[256];
    FILE *file;
    uid_t process_uid;
    int count;

    dir = opendir(root);
    if (dir == NULL)
    {
        perror("opendir");
        return -1;
    }

    count = 0;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] < '0' || entry->d_name[0] > '9')
        {
            continue;
        }

        snprintf(path, sizeof path, "%s/%s/status", root, entry->d_name);
        file = fopen(path, "r");
        if (file == NULL)
        {
            continue;
        }

        process_uid = (uid_t)-1;
        while (fgets(line, sizeof line, file) != NULL)
        {
            if (line_uid(line, &process_uid))
            {
                break;
            }
        }
        fclose(file);

        if (process_uid == uid)
        {
            count++;
        }
    }

    closedir(dir);
    return count;
}

static int options(int argc, char **argv)
{
    int option = getopt(argc, argv, ":ispuU:cC:dvV:");
    char *argument = optarg;
    struct rlimit rlimit;
    char path[PATH_MAX + 1];
    char **environment;
    int process_count;
    unsigned long long remaining;
    long value;

    if (option == -1)
    {
        if (optind < argc)
        {
            fprintf(stderr, "Unexpected argument: %s\n", argv[optind]);
        }
        return optind < argc ? -1 : 0;
    }

    if (option == '?' || option == ':')
    {
        fprintf(stderr, option == '?' ? "Unknown option: -%c\n"
            : "Option -%c needs an argument\n", optopt);
        return -1;
    }

    if (options(argc, argv) == -1)
    {
        return -1;
    }

    switch (option)
    {
    case 'i':
        printf("uid=%lu euid=%lu gid=%lu egid=%lu\n",
            (unsigned long)getuid(), (unsigned long)geteuid(),
            (unsigned long)getgid(), (unsigned long)getegid());
        break;

    case 's':
        if (setpgid(0, 0) == -1)
        {
            perror("setpgid");
            return -1;
        }
        break;

    case 'p':
        printf("pid=%ld ppid=%ld pgrp=%ld\n",
            (long)getpid(), (long)getppid(), (long)getpgrp());
        break;

    case 'u':
        if (getrlimit(RLIMIT_NPROC, &rlimit) == -1)
        {
            perror("getrlimit");
            return -1;
        }

        process_count = count_processes("/proc", getuid());
        if (process_count == -1)
        {
            return -1;
        }

        if (rlimit.rlim_cur == RLIM_INFINITY)
        {
            puts("max new processes=unlimited");
        }
        else
        {
            if ((unsigned long long)process_count >= (unsigned long long)rlimit.rlim_cur)
            {
                remaining = 0;
            }
            else
            {
                remaining = (unsigned long long)rlimit.rlim_cur -
                    (unsigned long long)process_count;
            }
            printf("max new processes=%llu (limit=%llu, current=%d)\n",
                remaining, (unsigned long long)rlimit.rlim_cur, process_count);
        }
        break;

    case 'U':
        if (!number(argument, &value))
        {
            fprintf(stderr, "Bad ulimit: %s\n", argument);
            return -1;
        }
        if (ulimit(UL_SETFSIZE, value) == -1)
        {
            perror("ulimit");
            return -1;
        }
        break;

    case 'c':
        if (getrlimit(RLIMIT_CORE, &rlimit) == -1)
        {
            perror("getrlimit");
            return -1;
        }
        if (rlimit.rlim_cur == RLIM_INFINITY)
        {
            puts("core=unlimited");
        }
        else
        {
            printf("core=%llu\n", (unsigned long long)rlimit.rlim_cur);
        }
        break;

    case 'C':
        if (!number(argument, &value))
        {
            fprintf(stderr, "Bad core size: %s\n", argument);
            return -1;
        }
        if (getrlimit(RLIMIT_CORE, &rlimit) == -1)
        {
            perror("getrlimit");
            return -1;
        }
        rlimit.rlim_cur = (rlim_t)value;
        if (setrlimit(RLIMIT_CORE, &rlimit) == -1)
        {
            perror("setrlimit");
            return -1;
        }
        break;

    case 'd':
        if (getcwd(path, sizeof path) == NULL)
        {
            perror("getcwd");
            return -1;
        }
        puts(path);
        break;

    case 'v':
        for (environment = environ; *environment != NULL; environment++)
        {
            puts(*environment);
        }
        break;

    case 'V':
        if (strchr(argument, '=') == NULL || argument[0] == '=')
        {
            fprintf(stderr, "Expected name=value: %s\n", argument);
            return -1;
        }
        if (putenv(argument) != 0)
        {
            perror("putenv");
            return -1;
        }
        break;
    }

    return 0;
}

int main(int argc, char **argv)
{
    opterr = 0;

    if (argc == 1)
    {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    return options(argc, argv) == -1 ? EXIT_FAILURE : EXIT_SUCCESS;
}
