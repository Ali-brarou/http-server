#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include "config.h"
#include "connection.h"

#define HTTP_MAX_STATUS_CODE 599
typedef enum Http_status_code_e {
    HTTP_CONTINUE = 100,
    HTTP_SWITCHING_PROTOCOLS = 101,

    HTTP_OK = 200,
    HTTP_CREATED = 201,
    HTTP_ACCEPTED = 202,
    HTTP_NO_CONTENT = 204,

    HTTP_MOVED_PERMANENTLY = 301,
    HTTP_FOUND = 302,
    HTTP_NOT_MODIFIED = 304,

    HTTP_BAD_REQUEST = 400,
    HTTP_UNAUTHORIZED = 401,
    HTTP_FORBIDDEN = 403,
    HTTP_NOT_FOUND = 404,
    HTTP_METHOD_NOT_ALLOWED = 405,
    HTTP_CONFLICT = 409,
    HTTP_GONE = 410,
    HTTP_PAYLOAD_TOO_LARGE = 413,
    HTTP_UNSUPPORTED_MEDIA_TYPE = 415,

    HTTP_INTERNAL_SERVER_ERROR = 500,
    HTTP_NOT_IMPLEMENTED = 501,
    HTTP_BAD_GATEWAY = 502,
    HTTP_SERVICE_UNAVAILABLE = 503,
    HTTP_GATEWAY_TIMEOUT = 504
}  Http_status_code_t; 

const char* http_reason_from_status(int status_code); 

#define HTTP_CONTENT_TYPE_LAST HTTP_CONTENT_TEXT_HTML 
typedef enum Http_content_type_e {
    HTTP_CONTENT_NONE = 0, 
    HTTP_CONTENT_TEXT_PLAIN, 
    HTTP_CONTENT_TEXT_HTML, 
} Http_content_type_t;  

const char* http_content_type_value(Http_content_type_t content_type); 

typedef enum Http_memory_flag_e {
    HTTP_MEM_STATIC, 
    HTTP_MEM_OWNED, 
} Http_memory_flag_t; 

typedef struct {
    char* key;
    char* value;
    Http_memory_flag_t key_mem;
    Http_memory_flag_t value_mem;
} Http_response_header_t;

typedef struct Http_response_s {
    int status_code; 
    Http_content_type_t content_type; 
    Http_response_header_t headers[HTTP_MAX_HEADERS]; 
    size_t headers_count; 
    char* body; 
    size_t body_len; 
    Http_memory_flag_t body_mem; 
    int connection_close; 
} Http_response_t; 

void http_response_make_error(Http_response_t* resp, int status_code); 
/* returns -1 if an error or the used size if everything is ok */ 
int  http_response_raw(const Http_response_t* resp, char* buffer, size_t buffer_len); 
/* response won't be free if the handler returned error */ 
void http_response_free(Http_response_t* resp); 

#endif
