#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <signal.h>

#define MAX_LINES 1024

typedef struct {
    off_t  off[MAX_LINES];
    size_t len[MAX_LINES];
    int    count;
} LineTable;

static char  *g_data = NULL;    
static size_t g_size = 0;       
static LineTable g_table;       

static int build_table(const char *data, size_t size, LineTable *t)
{
    size_t start = 0;
    t->count = 0;

    for (size_t i = 0; i < size; i++) {
        if (data[i] == '\n') {
            if (t->count >= MAX_LINES) {
                fprintf(stderr, "Too many lines (max %d)\n", MAX_LINES);
                return -1;
            }
            t->off[t->count] = (off_t)start;
            t->len[t->count] = i - start;
            t->count++;
            start = i + 1;
        }
    }

    if (start < size) {
        if (t->count >= MAX_LINES) {
            fprintf(stderr, "Too many lines (max %d)\n", MAX_LINES);
            return -1;
        }
        t->off[t->count] = (off_t)start;
        t->len[t->count] = size - start;
        t->count++;
    }

    return 0;
}

static void print_table(const LineTable *t)
{
    printf("Line table:\n");
    printf(" idx |  offset  | length\n");
    printf("-----+----------+--------\n");
    for (int i = 0; i < t->count; i++) {
        printf("%3d  | %7ld  | %4zu\n",
               i + 1, (long)t->off[i], t->len[i]);
    }
    fflush(stdout);
}

static void print_line(const LineTable *t, int n)
{
    if (n < 1 || n > t->count) {
        printf("No such line: %d (file has %d lines)\n", n, t->count);
        fflush(stdout);
        return;
    }

    size_t off = (size_t)t->off[n - 1];
    size_t len = t->len[n - 1];

    printf("Line %d: ", n);
    if (len > 0)
        fwrite(g_data + off, 1, len, stdout);
    printf("\n");
    fflush(stdout);
}

static void on_alarm(int sig)
{
    (void)sig;
    if (g_data != NULL && g_size > 0)
        write(STDOUT_FILENO, g_data, g_size);
    write(STDOUT_FILENO, "\n", 1);
    _exit(0);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        return 1;
    }

    if (st.st_size == 0) {
        fprintf(stderr, "File is empty\n");
        close(fd);
        return 1;
    }

    g_size = (size_t)st.st_size;
    g_data = mmap(NULL, g_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (g_data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    close(fd);

    if (build_table(g_data, g_size, &g_table) != 0) {
        munmap(g_data, g_size);
        return 1;
    }

    print_table(&g_table);

    struct sigaction sa;
    sa.sa_handler = on_alarm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    printf("\nEnter line number (5 sec): ");
    fflush(stdout);

    alarm(5);

    char inbuf[64];
    int num;

    if (fgets(inbuf, sizeof(inbuf), stdin) == NULL) {
        alarm(0);
        munmap(g_data, g_size);
        return 0;
    }

    alarm(0);   

    if (sscanf(inbuf, "%d", &num) != 1) {
        printf("Bad input\n");
    } else if (num == 0) {
        munmap(g_data, g_size);
        return 0;
    } else {
        print_line(&g_table, num);
    }

    for (;;) {
        printf("\nEnter line number (0 to quit): ");
        fflush(stdout);

        if (fgets(inbuf, sizeof(inbuf), stdin) == NULL)
            break;

        if (sscanf(inbuf, "%d", &num) != 1) {
            printf("Bad input, try again.\n");
            continue;
        }

        if (num == 0)
            break;

        print_line(&g_table, num);
    }

    munmap(g_data, g_size);
    return 0;
}
