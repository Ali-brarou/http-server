#ifndef CIRC_BUFF_H
#define CIRC_BUFF_H

#include <stddef.h> 
#include <string.h> 

typedef struct Http_circ_buff_s {
    char* buff; 
    size_t size;  /* must be power of 2 */ 
    size_t head; 
    size_t tail; 
} Http_circ_buff_t; 

#define HTTP_CIRC_LEN(c) \
    (((c)->head - (c)->tail) & ((c)->size - 1))

#define HTTP_CIRC_SPACE(c) \
    ((c)->size - HTTP_CIRC_LEN(c) - 1)

#define HTTP_CIRC_EMPTY(c) \
    ((c)->head == (c)->tail)

#define HTTP_CIRC_FULL(c) \
    (HTTP_CIRC_LEN(c) == ((c)->size - 1))

void http_circ_init(Http_circ_buff_t* c, char* buff, size_t size); 
int http_circ_write(Http_circ_buff_t* c, char* data, size_t len); 
/* int http_circ_read(Http_circ_buff_t* c, char* data, size_t len); */
/* int http_circ_peek(Http_circ_buff_t* c, char* data, size_t len); */

#endif
