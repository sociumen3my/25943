#include <sys/types.h>
#include <sys/resource.h>
#include <errno.h>
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

static int options(int argc, char **argv)
{
    int option = getopt(argc, argv, ":ispuU:cC:dvV:");
    char *argument = optarg;
    struct rlimit rlimit;
    char path[PATH_MAX + 1];
    char **environment;
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
        errno = 0;
        value = ulimit(UL_GETFSIZE);
        if (value == -1 && errno != 0)
        {
            perror("ulimit");
            return -1;
        }
        printf("ulimit=%ld\n", value);
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

    return options(argc, argv) == -1 ? EXIT_FAILURE : EXIT_SUCCESS;
}
