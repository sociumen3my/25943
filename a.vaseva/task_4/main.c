#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE 1024


static void sanitize_string(char *str) {
    char *src = str;
    char *dst = str;

    while (*src) {
        unsigned char c = (unsigned char)*src;

        if (c == 0x1B) {
            if (src[1] == '[') {
                src += 3;
                continue;
            } else {
                src++;
                continue;
            }
        }

        if (isprint(c) || c == '\n' || c == '\t') {
            *dst++ = c;
        }

        src++;
    }
    *dst = '\0';
}
struct node {
    char *str;
    struct node *next;
};

int main(void) {
    struct node *head = NULL;
    struct node *tail = NULL;
    char buf[MAX_LINE];

    while (fgets(buf, sizeof(buf), stdin) != NULL) {
        if (buf[0] == '.')
            break;

	sanitize_string(buf);

        size_t len = strlen(buf);

        char *copy = malloc(len + 1);
        if (copy == NULL) {
            fprintf(stderr, "not enough memory\n");
            struct node *p = head;
	            while (p != NULL) {
                struct node *tmp = p;
                p = p->next;
                free(tmp->str);
                free(tmp);
            }
            return 1;
        }
        memcpy(copy, buf, len + 1);

        struct node *n = malloc(sizeof(struct node));
        if (n == NULL) {
            fprintf(stderr, "not enough memory\n");
            free(copy);
            struct node *p = head;
            while (p != NULL) {
                struct node *tmp = p;
                p = p->next;
                free(tmp->str);
                free(tmp);
            }
            return 1;
        }
        n->str  = copy;
        n->next = NULL;

        if (head == NULL) {
            head = tail = n;
        } else {
            tail->next = n;
            tail = n;
        }
    }

    for (struct node *p = head; p != NULL; p = p->next) {
        printf("%s", p->str);
    }
    fflush(stdout);
    struct node *p = head;
    while (p != NULL) {
        struct node *tmp = p;
        p = p->next;
        free(tmp->str);
        free(tmp);
    }

    return 0;
}
