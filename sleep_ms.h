#ifndef SLEEP_MS_H
#define SLEEP_MS_H
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
static int sleep_ms(unsigned int msec)
{
    struct timespec timeout0;
    struct timespec timeout1;
    struct timespec* tmp;
    struct timespec* t0 = &timeout0;
    struct timespec* t1 = &timeout1;

    t0->tv_sec = msec / 1000;
    t0->tv_nsec = (msec % 1000) * (1000 * 1000);

    while(nanosleep(t0, t1) == -1)
    {
        if(errno == EINTR)
        {
            tmp = t0;
            t0 = t1;
            t1 = tmp;
        }
        else
        {
            return -1;
        }
    }
    return 0;
}
#endif
