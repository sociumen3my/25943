#include <stdio.h>
#include <time.h>

int main(void){
    time_t now = time(NULL);
    struct tm *my_tm = localtime(&now);
    time_t pst_seconds = now - 8 * 3600;
    struct tm *pst_tm = gmtime(&pst_seconds);
    printf("%04d-%02d-%02d %02d:%02d:%02d\n",
           pst_tm->tm_year + 1900, pst_tm->tm_mon + 1, pst_tm->tm_mday,
           pst_tm->tm_hour, pst_tm->tm_min, pst_tm->tm_sec);
    return 0;
}
