#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_LINES 1000
#define MAX_INPUT_LENGTH 100

typedef struct Line
{
    size_t offset;
    size_t length;
} Line;

static char *file_data;
static size_t file_size;

static void timeout(int signal_number)
{
    (void)signal_number;

    puts("\nTime is over. Complete file:");
    if (file_size != 0)
    {
        fwrite(file_data, 1, file_size, stdout);
    }
    exit(EXIT_SUCCESS);
}

static int build_table(Line *table)
{
    size_t start;
    size_t index;
    int lines;

    start = 0;
    lines = 0;

    for (index = 0; index < file_size && lines < MAX_LINES; index++)
    {
        if (file_data[index] == '\n')
        {
            table[lines].offset = start;
            table[lines].length = index - start + 1;
            start = index + 1;
            lines++;
        }
    }

    if (start < file_size && lines < MAX_LINES)
    {
        table[lines].offset = start;
        table[lines].length = file_size - start;
        lines++;
    }

    return lines;
}

static void print_line(const Line *line)
{
    fwrite(file_data + line->offset, 1, line->length, stdout);
    if (file_data[line->offset + line->length - 1] != '\n')
    {
        putchar('\n');
    }
}

int main(int argc, char **argv)
{
    Line table[MAX_LINES];
    struct stat info;
    char input[MAX_INPUT_LENGTH];
    char *end;
    int fd;
    int line_count;
    int number;
    int first_input;
    long value;

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

    if (fstat(fd, &info) == -1)
    {
        perror("fstat");
        close(fd);
        return EXIT_FAILURE;
    }

    file_size = (size_t)info.st_size;
    if (file_size != 0)
    {
        file_data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (file_data == MAP_FAILED)
        {
            perror("mmap");
            close(fd);
            return EXIT_FAILURE;
        }
    }
    close(fd);

    line_count = build_table(table);

    signal(SIGALRM, timeout);

    first_input = 1;
    for (;;)
    {
        if (first_input)
        {
            printf("Line number (5 seconds, 0 to quit): ");
        }
        else
        {
            printf("Line number (0 to quit): ");
        }
        fflush(stdout);

        if (first_input)
        {
            alarm(5);
        }

        if (fgets(input, sizeof input, stdin) == NULL)
        {
            break;
        }

        if (first_input)
        {
            alarm(0);
            first_input = 0;
        }

        value = strtol(input, &end, 10);
        while (*end == ' ' || *end == '\t')
        {
            end++;
        }

        if (end == input || (*end != '\n' && *end != '\0'))
        {
            puts("Invalid line number");
            continue;
        }

        if (value == 0)
        {
            break;
        }

        if (value < 1 || value > line_count)
        {
            printf("File has %d lines\n", line_count);
            continue;
        }

        number = (int)value;
        print_line(&table[number - 1]);
    }

    alarm(0);
    if (file_size != 0)
    {
        munmap(file_data, file_size);
    }

    return EXIT_SUCCESS;
}
