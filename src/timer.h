#ifndef TIMER_H
#define TIMER_H

#include "config.h"

/* forward declaration */ 
typedef struct Http_connection_s Http_connection_t; 

typedef struct Http_timer_event_s
{
} Http_timer_event_t; 

typedef struct Http_timer_s
{
} Http_timer_t; 

int  http_timer_create();
void http_timer_close(int timer_fd); 

int  http_timer_add_timeout(Http_connection_t* con, int timeout_sec); 

#endif
