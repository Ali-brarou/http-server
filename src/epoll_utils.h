#ifndef EPOLL_UTILS_H
#define EPOLL_UTILS_H

#include <stdint.h> 

#include "server_context.h"
#include "config.h"
#include "connection.h"
#include "timer.h"

/* forward declaration */ 
typedef struct Http_connection_s Http_connection_t; 

typedef enum Http_epoll_item_type_e {
    HTTP_ITEM_SHUTDOWN, 
    HTTP_ITEM_LISTENER, 
    HTTP_ITEM_CLIENT, 
    HTTP_ITEM_TIMER,
} Http_epoll_item_type_t; 

/* a wrapper around epoll data */ 
/* this might be slow because of heap allocation */ 
/* maybe I will using a tag system */ 
typedef struct Http_epoll_item_s {
    Http_epoll_item_type_t type; 
    union {
        int fd; 
        Http_connection_t* con; 
        Http_timer_t* timer; 
    }; 
} Http_epoll_item_t; 

/* returns -1 in case of an error */ 
int  http_epoll_create_instance(void); 
void http_epoll_close(int epoll_fd); 
int  http_epoll_add_fd(int epoll_fd, Http_epoll_item_type_t type, int fd, uint32_t events);
int  http_epoll_add_timer(int epoll_fd, Http_timer_t* timer, uint32_t events); 
int  http_epoll_add_con(int epoll_fd, Http_connection_t* con, uint32_t events);  
/* modify the events of the client fd */ 
int  http_epoll_mod_con(int epoll_fd, Http_epoll_item_t* con_item, uint32_t events); 
void http_epoll_del_con(int epoll_fd, Http_connection_t* con); 
/* main server loop */ 
int  http_epoll_run_loop(Http_server_context_t* ctx); 


int  http_shutdown_setup(int epoll_fd); 
void http_trigger_shutdown(void); 
void http_shutdown_close(void); 

#endif
