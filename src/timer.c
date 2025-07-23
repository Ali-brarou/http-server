#include <sys/timerfd.h> 
#include <unistd.h> 

#include "timer.h"

int http_timer_create()
{
    return timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK); 
}

void http_timer_close(int timer_fd)
{
    close(timer_fd); 
}
