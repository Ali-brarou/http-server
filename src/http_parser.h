#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <stddef.h> 

#include "utils.h"
#include "config.h"
#include "http_common.h"

#define HTTP_METHOD_LAST HTTP_METHOD_PATCH
typedef enum Http_method_e {
    HTTP_METHOD_UNKNOWN = 0,
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_OPTIONS,
    HTTP_METHOD_PATCH,
} Http_method_t;

typedef struct Http_method_mapping_s {
    Http_method_t method; 
    const char* name;  
} Http_method_mapping_t; 

Http_method_t http_method_from_string(char* str); 

typedef struct Http_request_s {
    Http_method_t method; 
    char* method_str; 
    char* path; 
    char version[HTTP_VERSION_SIZE]; 
    Http_header_t headers[HTTP_MAX_HEADERS]; 
    size_t headers_count; 
    char* body; 
    size_t body_len; 
} Http_request_t; 

char* http_request_search_header(Http_request_t* request, const char* key); /* return null if didn't find */ 
int http_request_parse(Http_request_t* request, char* request_raw, size_t request_raw_len); 

/* debug */ 
void http_request_print(Http_request_t* request); 

#endif
