#ifndef ROUTER_H
#define ROUTER_H

#include "http_parser.h"
#include "http_handler.h"

/* TODO: implement a faster data structure */
typedef struct Http_route_s {
    Http_method_t method; 
    char* path; 
    Http_handler_t handler;
    void* data; /* user data */  
    struct Http_route_s* next; 
} Http_route_t; 

typedef struct Http_router_s {
    Http_route_t* routes;  
    size_t route_count; 
} Http_router_t; 

int http_route_register(Http_router_t* router, 
                        Http_method_t method, 
                        const char* path, 
                        Http_handler_t handler); 
/* returns NULL if no routing exists */
Http_handler_t http_router_find(Http_router_t* router, Http_method_t method, const char* path);
int http_router_init(Http_router_t* router); 
void http_router_clean(Http_router_t* router); 


#endif
