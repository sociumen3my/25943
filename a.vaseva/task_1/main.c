#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <string.h>
#include <errno.h>

#ifndef RLIMIT_NPROC
#define RLIMIT_NPROC 6
#endif

void do_i(void) {
    printf("real uid=%d, euid=%d, real gid=%d, egid=%d\n",
           getuid(), geteuid(), getgid(), getegid());
}

void do_s(void) {
    if (setpgid(0, 0) == -1) {
        perror("setpgid");
    } else {
        printf("group leader (PGID=%d)\n", getpgrp());
    }
}

void do_p(void) {
    printf("PID=%d, PPID=%d, PGID=%d\n",
           getpid(), getppid(), getpgrp());
}

void do_u(void) {
    long max_procs = sysconf(_SC_CHILD_MAX);
    if (max_procs == -1) {
        perror("sysconf(_SC_CHILD_MAX)");
    } else {
        printf("ulimit = %ld\n", max_procs);
    }
}

void do_U(const char *arg) {
    long val = strtol(arg, NULL, 10);
    struct rlimit rl;

    if (getrlimit(RLIMIT_NPROC, &rl) == 0) {
        rl.rlim_cur = val;
        if (setrlimit(RLIMIT_NPROC, &rl) == -1) {
            perror("setrlimit");
        } else {
            printf("ulimit changed to %ld\n", val);
        }
    } else {
        printf("error\n");
    }
}

void do_c(void) {
    struct rlimit rl;
    if (getrlimit(RLIMIT_CORE, &rl) == 0) {
        printf("core = ");
        if (rl.rlim_cur == RLIM_INFINITY)
            printf("unlimited\n");
        else
            printf("%llu bytes\n", (unsigned long long)rl.rlim_cur);
    }
}

void do_C(const char *arg) {
    long val = strtol(arg, NULL, 10);
    struct rlimit rl;
    if (getrlimit(RLIMIT_CORE, &rl) == 0) {
        rl.rlim_cur = val;
        if (setrlimit(RLIMIT_CORE, &rl) == -1) {
            perror("setrlimit");
        } else {
            printf("core changed to %ld\n", val);
        }
    }
}

void do_d(void) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("[d] cwd = %s\n", cwd);
    } else {
        perror("getcwd");
    }
}

void do_v(void) {
    extern char **environ;
    printf("environment variables:\n");
    for (char **env = environ; *env != NULL; env++) {
        printf("  %s\n", *env);
    }
}

void do_V(const char *arg) {
    if (putenv((char *)arg) != 0) {
        perror("putenv");
    } else {
        printf("environment updated: %s\n", arg);
    }
}

typedef struct {
    char opt;
    char *arg;
} Option;

int main(int argc, char *argv[]) {
    int opt;
    Option opts[100];
    int opt_count = 0;

    while ((opt = getopt(argc, argv, "ispucdC:V:")) != -1) {
        switch (opt) {
            case 'i': case 's': case 'p': case 'u':
            case 'c': case 'd': case 'v':
                opts[opt_count].opt = opt;
                opts[opt_count].arg = NULL;
                opt_count++;
                break;
            case 'U': case 'C': case 'V':
                opts[opt_count].opt = opt;
                opts[opt_count].arg = optarg;
                opt_count++;
                break;
            case '?':
                fprintf(stderr, "Unknown option: -%c\n", optopt);
                return 1;
        }
    }

    for (int i = opt_count - 1; i >= 0; i--) {
        switch (opts[i].opt) {
            case 'i': do_i(); break;
            case 's': do_s(); break;
            case 'p': do_p(); break;
            case 'u': do_u(); break;
            case 'U': do_U(opts[i].arg); break;
            case 'c': do_c(); break;
            case 'C': do_C(opts[i].arg); break;
            case 'd': do_d(); break;
            case 'v': do_v(); break;
            case 'V': do_V(opts[i].arg); break;
        }
    }

    return 0;
}
