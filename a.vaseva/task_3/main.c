#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

#define FILENAME "data.txt"

static void print_ids(const char *label)
{
    uid_t ruid = getuid();
    uid_t euid = geteuid();
    printf("[%s] real uid = %d, effective uid = %d\n",
           label, (int)ruid, (int)euid);
    fflush(stdout);
}

static void try_open(const char *label)
{
    FILE *fp;

    printf("[%s] trying to open '%s'...\n", label, FILENAME);
    fflush(stdout);

    fp = fopen(FILENAME, "r");
    if (fp == NULL) {
        perror("fopen");
        return;
    }
    printf("[%s] fopen OK, closing.\n", label);
    fflush(stdout);

    if (fclose(fp) != 0) {
        perror("fclose");
    }
}

int main(void)
{
    print_ids("before setuid");

    try_open("before setuid");

    if (setuid(getuid()) == -1) {
        perror("setuid");
    } else {
        printf("setuid(getuid()) succeeded.\n");
    }
    fflush(stdout);

    print_ids("after setuid");
    try_open("after setuid");

    return 0;
}
