#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024

#define ESC 0x1B

enum
{
    STATE_TEXT,
    STATE_ESC,
    STATE_CSI,
    STATE_OSC
};

typedef struct LineNode
{
    char *text;
    struct LineNode *next;
} LineNode;

static void trim_newline(char *text)
{
    size_t length;

    length = strlen(text);

    if (length > 0 && text[length - 1] == '\n')
    {
        text[length - 1] = '\0';
    }
}

static char *strip_terminal_sequences(char *text)
{
    char *src;
    char *dst;
    int state;

    src = text;
    dst = text;
    state = STATE_TEXT;

    while (*src != '\0')
    {
        unsigned char c;

        c = (unsigned char)*src;

        switch (state)
        {
        case STATE_TEXT:
            if (c == ESC)
            {
                state = STATE_ESC;
            }
            else if (c < 0x20 && c != '\t')
            {
            }
            else
            {
                *dst++ = (char)c;
            }
            src++;
            break;

        case STATE_ESC:
            src++;
            if (c == '[')
            {
                state = STATE_CSI;
            }
            else if (c == ']')
            {
                state = STATE_OSC;
            }
            else if (c >= 0x20 && c <= 0x7E)
            {
                src++;
                state = STATE_TEXT;
            }
            else
            {
                state = STATE_TEXT;
            }
            break;

        case STATE_CSI:
            src++;
            if (c == '\0' || (c >= 0x40 && c <= 0x7E))
            {
                state = STATE_TEXT;
            }
            break;

        case STATE_OSC:
            if (c == '\007')
            {
                src++;
                state = STATE_TEXT;
            }
            else if (c == ESC)
            {
                src++;
                if (*src == '\\')
                {
                    src++;
                }
                state = STATE_TEXT;
            }
            else
            {
                src++;
            }
            break;
        }
    }

    *dst = '\0';
    return text;
}

static int append_line(LineNode **head, LineNode **tail, const char *text)
{
    LineNode *new_node;
    size_t length;

    new_node = malloc(sizeof(*new_node));
    if (new_node == NULL)
    {
        perror("malloc new_node");
        return -1;
    }

    length = strlen(text);
    new_node->text = malloc(length + 1);
    if (new_node->text == NULL)
    {
        perror("malloc text");
        free(new_node);
        return -1;
    }

    memcpy(new_node->text, text, length + 1);
    new_node->next = NULL;

    if (*tail == NULL)
    {
        *head = new_node;
        *tail = new_node;
    }
    else
    {
        (*tail)->next = new_node;
        *tail = new_node;
    }

    return 0;
}

static void print_lines(const LineNode *head)
{
    const LineNode *current;

    current = head;
    while (current != NULL)
    {
        puts(current->text);
        current = current->next;
    }
}

static void free_lines(LineNode *head)
{
    LineNode *current;
    LineNode *next;

    current = head;
    while (current != NULL)
    {
        next = current->next;
        free(current->text);
        free(current);
        current = next;
    }
}

int main(void)
{
    char buffer[MAX_LINE_LENGTH];
    LineNode *head;
    LineNode *tail;

    head = NULL;
    tail = NULL;

    while (fgets(buffer, sizeof(buffer), stdin) != NULL)
    {
        if (buffer[0] == '.')
        {
            break;
        }

        trim_newline(buffer);
        strip_terminal_sequences(buffer);

        if (append_line(&head, &tail, buffer) == -1)
        {
            free_lines(head);
            return EXIT_FAILURE;
        }
    }

    if (ferror(stdin))
    {
        perror("fgets");
        free_lines(head);
        return EXIT_FAILURE;
    }

    print_lines(head);
    free_lines(head);

    return EXIT_SUCCESS;
}