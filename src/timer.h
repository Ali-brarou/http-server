#ifndef TIMER_H
#define TIMER_H

#include "config.h"

/* forward declaration */ 
typedef struct Http_connection_s Http_connection_t; 

/* I will implement a simple min heap for timer */ 

typedef enum Http_timer_event_flag_e {
    HTTP_TIMER_EVENT_INVALID    = 0, 
    HTTP_TIMER_EVENT_VALID      = 1, 
} Http_timer_event_flag_t; 

/* a timeout event */ 
/* the timer will responsible for tracking timeout_index in the connection */ 
typedef struct Http_timer_event_s
{
    time_t timeout; 
    Http_connection_t* con; 
    Http_timer_event_flag_t flag;  
} Http_timer_event_t; 

typedef struct Http_timer_s
{
    int fd; 
    Http_timer_event_t* events; 
    size_t events_count;  
} Http_timer_t; 

Http_timer_t*  http_timer_create(void); 
void http_timer_clean(Http_timer_t* timer); 

int  http_timer_add_timeout(Http_timer_t* timer, Http_connection_t* con, int timeout_sec); 
void http_timer_invalid_timeout(Http_timer_t* timer, int event_index); 
int  http_timer_reset_timeout(Http_timer_t* timer, Http_connection_t* con, int timeout_sec);
int  http_timer_pop_recent(Http_timer_t* timer, Http_timer_event_t* event); 

#endif
