#include <assert.h> 
#include <stdlib.h> 
#include <stdio.h> 
#include <sys/timerfd.h> 
#include <unistd.h> 

#include "timer.h"
#include "epoll_utils.h"

#define ROOT 0
#define PARENT(i) (((i) - 1) >> 1)
#define LEFT(i) (((i) << 1) + 1)
#define RIGHT(i) (((i) << 1) + 2)

#define SWAP_EVENTS(events, i, j) do {\
    Http_timer_event_t tmp = (events)[i]; \
    (events)[i] = (events)[j]; \
    (events)[j] = tmp; \
    if ((events)[i].con) \
        (events)[i].con->timeout_index = i; \
    if ((events)[j].con) \
        (events)[j].con->timeout_index = j; \
} while(0)
#define CMP_EVENTS(events, i, j) ((events)[i].timeout < (events)[j].timeout)

static int insert(Http_timer_t* timer, Http_connection_t* connection, time_t real_timeout); 
static void heapify(Http_timer_t* timer, int i); 
static int timer_update(Http_timer_t* timer); 
static time_t timer_recent_time(Http_timer_t* timer); 

Http_timer_t* http_timer_create(void)
{ 
    Http_timer_t* timer = malloc(sizeof(Http_timer_t)); 
    if (!timer)
        return NULL; 
   
    timer->events_count = 0; 

    timer->events = malloc(HTTP_TIMER_MAX_EVENTS * sizeof(Http_timer_event_t)); 
    if (!timer->events)
    {
        free(timer); 
        return NULL; 
    }

    timer->fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK); 
    if (timer->fd == -1)
    {
        free(timer->events); 
        free(timer); 
        return NULL; 
    }

    return timer; 
}

void http_timer_clean(Http_timer_t* timer)
{
    close(timer->fd); 
    free(timer->events); 
    free(timer); 
}

int http_timer_add_timeout(Http_timer_t* timer, Http_connection_t* con, int timeout_sec)
{
    assert(timer != NULL); 
    assert(con != NULL); 
    time_t real_timeout = time(NULL) + timeout_sec; 
    if (insert(timer, con, real_timeout) == -1)
        return -1; 

    return timer_update(timer); 
}

void http_timer_invalid_timeout(Http_timer_t* timer, int event_index)
{
    assert(timer != NULL); 
    
    timer->events[event_index].con = NULL; 
    timer->events[event_index].flag = HTTP_TIMER_EVENT_INVALID; 
}

int http_timer_reset_timeout(Http_timer_t* timer, Http_connection_t* con, int timeout_sec)
{
    /* invalid the old one and create a new one */ 
    assert(timer != NULL); 
    assert(con != NULL); 
    http_timer_invalid_timeout(timer, con->timeout_index); 
    return http_timer_add_timeout(timer, con, timeout_sec); 
}

int http_timer_pop_recent(Http_timer_t* timer, Http_timer_event_t* event)
{
    assert(timer != NULL); 
    assert(timer->events_count > 0); 
        
    *event = timer->events[ROOT]; 
    
    if (timer->events_count == 1)
    {
        timer->events_count--; 
        return 0;  
    }

    timer->events[ROOT] = timer->events[timer->events_count - 1]; 
    if (timer->events[ROOT].con)
        timer->events[ROOT].con->timeout_index = ROOT; 
    timer->events_count--; 

    heapify(timer, ROOT); 

    return timer_update(timer); 
}

static int insert(Http_timer_t* timer, Http_connection_t* con, time_t real_timeout)
{
    if (timer->events_count >= HTTP_TIMER_MAX_EVENTS)  
        return -1; 

    int i = timer->events_count; 
    timer->events[i].con = con; 
    timer->events[i].flag = HTTP_TIMER_EVENT_VALID; 
    timer->events[i].timeout = real_timeout; 
    timer->events[i].con->timeout_index = i; 

    timer->events_count++; 

    while (i != ROOT && CMP_EVENTS(timer->events, i, PARENT(i)))
    {
        SWAP_EVENTS(timer->events, i, PARENT(i)); 
        i = PARENT(i); 
    }

    return 0; 
}

int timer_update(Http_timer_t* timer)
{
    assert(timer != NULL); 
    assert(timer->events_count > 0); 
    time_t recent_expiration = timer_recent_time(timer) ; 
    struct itimerspec ts = {0};
    ts.it_value.tv_sec = recent_expiration; 

    if (timerfd_settime(timer->fd, TFD_TIMER_ABSTIME, &ts, NULL) == -1)
    {
        perror("timerfd_settime"); 
        return -1; 
    }

    return 0; 
}

static time_t timer_recent_time(Http_timer_t* timer)
{
    assert(timer->events_count > 0); 
    return timer->events[0].timeout;
}

static void heapify(Http_timer_t* timer, int i)
{
    int smallest = i;  
    size_t l = LEFT(i); 
    size_t r = RIGHT(i); 

    if (l < timer->events_count && CMP_EVENTS(timer->events, l, smallest))
        smallest = l; 
    if (r < timer->events_count && CMP_EVENTS(timer->events, r, smallest))
        smallest = r; 

    if (smallest != i)
    {
        SWAP_EVENTS(timer->events, i, smallest); 
        heapify(timer, smallest); 
    }
}
