#ifndef ROUNTER_H
#define ROUTNER_H

#include "http_parser.h"
#include "http_handler.h"

typedef struct Http_route_s {
    Http_method_t method; 
    const char* path; 
} Http_route_t; 

typedef struct Http_router_s {
    
} Http_router_t; 

int Http_router_add(Http_router_t* router, Http_method_t method, const char* path); 

#endif
