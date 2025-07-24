#ifndef CONFIG_H
#define CONFIG_H

#include <string.h> 
#include <sys/socket.h>

#include "http_handler.h"

#define HTTP_MAX_HOST_LEN 64

typedef struct Http_config_s {
    int port;
    char host[HTTP_MAX_HOST_LEN];
    int backlog; 
    int max_events; 
    Http_handler handler; 
} Http_config_t;


/* not configurable (for now) */ 
#define HTTP_USE_MEMMEM       /* if defined use memmem function */ 
#define HTTP_REQUEST_SIZE                   8192
#define HTTP_RESPONSE_SIZE                  8192
#define HTTP_MAX_HEADERS                    128 
#define HTTP_CLIENT_TIMEOUT                 30
#define HTTP_TIMER_MAX_EVENTS               819200 

/* configurable */ 
#define HTTP_DEFAULT_PORT                   6969
#define HTTP_DEFAULT_HOST                   "0.0.0.0"
#define HTTP_DEFAULT_BACKLOG                SOMAXCONN
#define HTTP_DEFAULT_MAX_EVENTS             1024

/* a http handler should be provided */ 
#define HTTP_DEFAULT_CONFIG (Http_config_t){\
    HTTP_DEFAULT_PORT,          \
    HTTP_DEFAULT_HOST,          \
    HTTP_DEFAULT_BACKLOG,       \
    HTTP_DEFAULT_MAX_EVENTS,    \
    NULL,                       \
}

#endif
