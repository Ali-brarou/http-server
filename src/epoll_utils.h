#ifndef EPOLL_UTILS_H
#define EPOLL_UTILS_H

#include <stdint.h> 

#include "config.h"
#include "connection.h"

/* forward declaration */ 
typedef struct Http_connection_s Http_connection_t; 

typedef enum Http_epoll_item_type_e {
    HTTP_ITEM_SHUTDOWN, 
    HTTP_ITEM_LISTENER, 
    HTTP_ITEM_CLIENT, 
    HTTP_ITEM_TIMER, /* not implemented yet */  
} Http_epoll_item_type_t; 

/* a wrapper around epoll data */ 
/* this might be slow because of heap allocation */ 
/* maybe I will using a tag system */ 
typedef struct Http_epoll_item_s {
    Http_epoll_item_type_t type; 
    union {
        int fd; 
        Http_connection_t* con; 
    }; 
} Http_epoll_item_t; 

/* returns -1 in case of an error */ 
int  http_epoll_create_instance(void); 
void http_epoll_close(int epoll_fd); 
int  http_epoll_add_fd(int epoll_fd, Http_epoll_item_type_t type, int fd, uint32_t events);
int  http_epoll_add_con(int epoll_fd, Http_connection_t* con, uint32_t events);  
/* modify the events of the client fd */ 
int  http_epoll_mod_con(int epoll_fd, Http_epoll_item_t* con_item, uint32_t events); 
/* main server loop */ 
int  http_epoll_run_loop(int epoll_fd, Http_config_t* cfg); 


int  http_shutdown_setup(int epoll_fd); 
void http_trigger_shutdown(void); 
void http_shutdown_close(void); 

#endif
