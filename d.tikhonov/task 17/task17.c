#include <ctype.h>
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define LIMIT 40
#define ESCAPE 27
#define CONTROL_W 23
#define CONTROL_D 4

static void remove_chars(int count)
{
    while (count > 0)
    {
        fputs("\b \b", stdout);
        count--;
    }
}

static int escape_byte(unsigned char *key)
{
    struct pollfd input;
    int ready;

    input.fd = STDIN_FILENO;
    input.events = POLLIN;
    input.revents = 0;

    do
    {
        ready = poll(&input, 1, 100);
    } while (ready == -1 && errno == EINTR);

    return ready > 0 && read(STDIN_FILENO, key, 1) == 1;
}

static int skip_special_key(void)
{
    unsigned char part;

    if (!escape_byte(&part))
    {
        return -1;
    }

    if (part != '[' && part != 'O')
    {
        return part;
    }

    while (escape_byte(&part))
    {
        if (part >= 0x40 && part <= 0x7E)
        {
            break;
        }
    }

    return -1;
}

int main(void)
{
    struct termios saved;
    struct termios mode;
    char text[LIMIT];
    unsigned char key;
    ssize_t result;
    int used;
    int cut;
    int next;
    int failed;

    if (tcgetattr(STDIN_FILENO, &saved) == -1)
    {
        perror("tcgetattr");
        return EXIT_FAILURE;
    }

    mode = saved;
    mode.c_lflag &= ~(ICANON | ECHO | ECHONL | ISIG | IEXTEN);
    mode.c_iflag &= ~(IXON | IXOFF | ICRNL | INLCR | IGNCR | ISTRIP);
    mode.c_oflag &= ~OPOST;
    mode.c_cc[VMIN] = 1;
    mode.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &mode) == -1)
    {
        perror("tcsetattr");
        return EXIT_FAILURE;
    }

    setvbuf(stdout, NULL, _IONBF, 0);

    used = 0;
    failed = 0;

    for (;;)
    {
        result = read(STDIN_FILENO, &key, 1);

        if (result == -1 && errno == EINTR)
        {
            continue;
        }

        if (result <= 0)
        {
            failed = (result == -1);
            break;
        }

        if (key == ESCAPE)
        {
            next = skip_special_key();
            putchar('\a');

            if (next < 0)
            {
                continue;
            }

            key = (unsigned char)next;
        }

        if (key == saved.c_cc[VERASE])
        {
            if (used > 0)
            {
                remove_chars(1);
                used--;
            }
        }
        else if (key == saved.c_cc[VKILL])
        {
            remove_chars(used);
            used = 0;
        }
        else if (key == CONTROL_W)
        {
            cut = used;

            while (cut > 0 && text[cut - 1] == ' ')
            {
                cut--;
            }

            while (cut > 0 && text[cut - 1] != ' ')
            {
                cut--;
            }

            remove_chars(used - cut);
            used = cut;
        }
        else if (key == CONTROL_D)
        {
            if (used == 0)
            {
                break;
            }

            putchar('\a');
        }
        else if (!isprint(key))
        {
            putchar('\a');
        }
        else
        {
            if (used == LIMIT)
            {
                fputs("\r\n", stdout);
                used = 0;

                if (key == ' ')
                {
                    continue;
                }
            }

            text[used++] = (char)key;
            putchar(key);
        }
    }

    fputs("\r\n", stdout);

    if (tcsetattr(STDIN_FILENO, TCSANOW, &saved) == -1)
    {
        perror("tcsetattr");
        return EXIT_FAILURE;
    }

    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
