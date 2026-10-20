#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_LINES 1000
#define MAX_LINE_LENGTH 4096

typedef struct Line
{
    off_t offset;
    size_t length;
} Line;

static int build_table(int fd, Line *table)
{
    char symbol;
    off_t start;
    off_t position;
    ssize_t result;
    int lines;

    start = 0;
    lines = 0;

    while ((result = read(fd, &symbol, 1)) == 1)
    {
        if (symbol == '\n')
        {
            position = lseek(fd, 0L, SEEK_CUR);
            table[lines].offset = start;
            table[lines].length = (size_t)(position - start);
            start = position;
            lines++;

            if (lines == MAX_LINES)
            {
                break;
            }
        }
    }

    if (result == -1)
    {
        perror("read");
        return -1;
    }

    position = lseek(fd, 0L, SEEK_CUR);
    if (position > start && lines < MAX_LINES)
    {
        table[lines].offset = start;
        table[lines].length = (size_t)(position - start);
        lines++;
    }

    return lines;
}

static void print_table(const Line *table, int line_count)
{
    int i;

    printf("Table of lines:\n");
    printf("  line   offset   length\n");
    for (i = 0; i < line_count; i++)
    {
        printf("  %4d   %7ld   %7zu\n", i + 1, (long)table[i].offset, table[i].length);
    }
}

static int print_line(int fd, const Line *line)
{
    char text[MAX_LINE_LENGTH];
    ssize_t result;

    lseek(fd, line->offset, SEEK_SET);
    result = read(fd, text, line->length);
    if (result != (ssize_t)line->length)
    {
        perror("read");
        return -1;
    }

    text[result] = '\0';
    printf("%s", text);
    if (result == 0 || text[result - 1] != '\n')
    {
        putchar('\n');
    }

    return 0;
}

int main(int argc, char **argv)
{
    Line table[MAX_LINES];
    int line_count;
    int fd;
    int number;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return EXIT_FAILURE;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd == -1)
    {
        perror("open");
        return EXIT_FAILURE;
    }

    line_count = build_table(fd, table);
    if (line_count == -1)
    {
        close(fd);
        return EXIT_FAILURE;
    }

    print_table(table, line_count);

    for (;;)
    {
        printf("Line number (0 to quit): ");
        fflush(stdout);

        if (scanf("%d", &number) != 1 || number == 0)
        {
            break;
        }

        if (number < 1 || number > line_count)
        {
            printf("File has %d lines\n", line_count);
            continue;
        }

        if (table[number - 1].length >= MAX_LINE_LENGTH)
        {
            puts("Line is too long");
            continue;
        }

        if (print_line(fd, &table[number - 1]) == -1)
        {
            break;
        }
    }

    if (close(fd) == -1)
    {
        perror("close");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
