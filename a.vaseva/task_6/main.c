#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>

#define MAX_LINES 1024

typedef struct {
    off_t  off[MAX_LINES];
    size_t len[MAX_LINES];
    int    count;
} LineTable;

static int g_fd = -1;

static int build_table(int fd, LineTable *t)
{
    char c;
    off_t cur;
    off_t start = 0;
    ssize_t n;

    t->count = 0;

    while ((n = read(fd, &c, 1)) == 1) {
        cur = lseek(fd, 0L, SEEK_CUR);
        if (cur == (off_t)-1) {
            perror("lseek");
            return -1;
        }

        if (c == '\n') {
            if (t->count >= MAX_LINES) {
                fprintf(stderr, "Too many lines (max %d)\n", MAX_LINES);
                return -1;
            }
            t->off[t->count] = start;
            t->len[t->count] = (size_t)(cur - 1 - start);
            t->count++;
            start = cur;
        }
    }

    if (n < 0) {
        perror("read");
        return -1;
    }

    if (start < lseek(fd, 0L, SEEK_END)) {
        if (t->count >= MAX_LINES) {
            fprintf(stderr, "Too many lines (max %d)\n", MAX_LINES);
            return -1;
        }
        t->off[t->count] = start;
        t->len[t->count] = (size_t)(lseek(fd, 0L, SEEK_END) - start);
        t->count++;
    }

    return 0;
}

static void print_table(const LineTable *t)
{
    int i;
    printf("Line table:\n");
    printf(" idx |  offset  | length\n");
    printf("-----+----------+--------\n");
    for (i = 0; i < t->count; i++) {
        printf("%3d  | %7ld  | %4zu\n",
               i + 1, (long)t->off[i], t->len[i]);
    }
    fflush(stdout);
}

static void print_line(int fd, const LineTable *t, int n)
{
    char *buf;
    ssize_t r;
    off_t pos;

    if (n < 1 || n > t->count) {
        printf("No such line: %d (file has %d lines)\n", n, t->count);
        fflush(stdout);
        return;
    }

    buf = malloc(t->len[n - 1] + 2);
    if (buf == NULL) {
        perror("malloc");
        return;
    }

    pos = lseek(fd, t->off[n - 1], SEEK_SET);
    if (pos == (off_t)-1) {
        perror("lseek");
        free(buf);
        return;
    }

    r = read(fd, buf, t->len[n - 1]);
    if (r < 0) {
        perror("read");
        free(buf);
        return;
    }
    buf[r] = '\0';

    printf("Line %d: %s\n", n, buf);
    fflush(stdout);
    free(buf);
}

static void on_alarm(int sig)
{
    (void)sig;
    if (g_fd >= 0 && lseek(g_fd, 0, SEEK_SET) != (off_t)-1) {
        char buf[4096];
        ssize_t n;
        while ((n = read(g_fd, buf, sizeof(buf))) > 0)
            write(STDOUT_FILENO, buf, n);
        write(STDOUT_FILENO, "\n", 1);
    }
    _exit(0);
}

int main(int argc, char **argv)
{
    int fd;
    LineTable t;
    int num;
    char inbuf[64];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (build_table(fd, &t) != 0) {
        close(fd);
        return 1;
    }

    print_table(&t);

    g_fd = fd;

    struct sigaction sa;
    sa.sa_handler = on_alarm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    printf("\nEnter line number (5 sec): ");
    fflush(stdout);

    alarm(5);

    if (fgets(inbuf, sizeof(inbuf), stdin) == NULL) {
        alarm(0);
        close(fd);
        return 0;
    }

    alarm(0);

    if (sscanf(inbuf, "%d", &num) != 1) {
        printf("Bad input\n");
    } else if (num == 0) {
        close(fd);
        return 0;
    } else {
        print_line(fd, &t, num);
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

        print_line(fd, &t, num);
    }

    close(fd);
    return 0;
}
