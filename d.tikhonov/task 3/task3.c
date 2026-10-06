#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

int main() {
    
    printf("UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());

    FILE *file = fopen("main.txt", "r");
    if (file == NULL) {
        perror("fopen failed");
    } else {
        printf("File opened successfully\n");
        fclose(file);
    }

    if (setuid(geteuid()) == -1) {
        perror("setuid failed");
        exit(EXIT_FAILURE);
    }
    printf("setuid(geteuid()) called successfully\n");
    printf("UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());

    file = fopen("main.txt", "r");
    if (file == NULL) {
        perror("fopen failed");
    } else {
        printf("File opened successfully\n");
        fclose(file);
    }

    return 0;
}