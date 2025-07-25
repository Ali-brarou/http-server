#ifndef SERVER_CONTEXT_H
#define SERVER_CONTEXT_H

#include "timer.h"
#include "config.h"

typedef struct Http_server_context_s {
    int listen_fd; 
    int epoll_fd; 
    int shutdown_fd; 
    Http_timer_t* timer; 
    Http_config_t* cfg; 
    size_t active_clients; /* keep track of clients number */ 
} Http_server_context_t; 

#endif
