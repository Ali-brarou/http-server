#ifndef CONNECTION_H
#define CONNECTION_H

#include "server_context.h"
#include "config.h"
#include "utils.h"
#include "http_parser.h"
#include "http_response.h"
#include "epoll_utils.h"
#include "timer.h"
#include "router.h"

/* forward declaration */ 
typedef struct Http_epoll_item_s Http_epoll_item_t; 

/* reading state */ 
#define HTTP_READING_HEADERS  0
#define HTTP_READING_BODY     1
#define HTTP_REQUEST_READY    2

/* flags */ 
/* if should close flag is set then stop recv data and compelete sending and the close */ 
#define HTTP_READ_STATE_MASK        0x03 /* 2 bits :3 */ 
#define HTTP_FLAG_WRITING           0x04
#define HTTP_FLAG_SHOULD_CLOSE      0x08 
#define HTTP_FLAG_CLOSING           0x10

#define HTTP_GET_READ_STATE(flags)      ((flags) & HTTP_READ_STATE_MASK)
#define HTTP_SET_READ_STATE(flags, state) \
    do { \
        (flags) = ((flags) & ~HTTP_READ_STATE_MASK) | ((state) & HTTP_READ_STATE_MASK); \
    } while (0) \

#define HTTP_SET_WRITING(flags)         ((flags) |= HTTP_FLAG_WRITING)
#define HTTP_IS_WRITING(flags)          ((flags) & HTTP_FLAG_WRITING)   
#define HTTP_CLEAR_WRITING(flags)       ((flags) &= ~HTTP_FLAG_WRITING)   

#define HTTP_SET_SHOULD_CLOSE(flags)    ((flags) |= HTTP_FLAG_SHOULD_CLOSE)
#define HTTP_SHOULD_CLOSE(flags)        ((flags) & HTTP_FLAG_SHOULD_CLOSE)

#define HTTP_SET_CLOSING(flags)         ((flags) |= HTTP_FLAG_CLOSING)
#define HTTP_IS_CLOSING(flags)          ((flags) & HTTP_FLAG_CLOSING)   

typedef struct Http_connection_s {
    int     client_fd; 
    int     timeout_index; /* keep track of where is timeout event in timer events array */

    Http_request_t request; 
    size_t header_len; 
    size_t body_len; 
    char    buff[HTTP_REQUEST_SIZE]; 
    size_t  buff_len; 

    char    response[HTTP_RESPONSE_SIZE]; 
    size_t  response_len;  
    size_t  response_sent; 

    uint8_t flags;  

    Http_router_t* router;  
} Http_connection_t; 

void http_connection_accept(Http_server_context_t* ctx); 
void http_connection_clean(Http_server_context_t* ctx, Http_connection_t* con); 

void http_connection_read(Http_connection_t* con); 
void http_connection_write(Http_connection_t* con); 
void http_connection_update_events(int epoll_fd, Http_epoll_item_t* con_item); 


#endif
