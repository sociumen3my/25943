#include <time.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    setenv("TZ", "America/Los_Angeles", 1);
    tzset();
    time_t now;
    struct tm *sp;
    (void)time(&now);
    sp = localtime(&now);
    printf("%d:%d", sp->tm_hour, sp->tm_min);
}