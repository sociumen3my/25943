#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    pid_t child;
    int status;

    child = fork();
    if (child == -1)
    {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (child == 0)
    {
        execlp("cat", "cat", "data.txt", (char *)NULL);
        perror("execlp");
        _exit(127);
    }

    printf("Parent: child %ld is running cat\n", (long)child);

    if (waitpid(child, &status, 0) == -1)
    {
        perror("waitpid");
        return EXIT_FAILURE;
    }

    printf("Parent: child finished\n");
    return EXIT_SUCCESS;
}
