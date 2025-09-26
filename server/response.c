#include <assert.h> 
#include <stdio.h> 
#include <stdlib.h> 

#include <http_response.h> 

static const char *http_status_reasons_table[HTTP_MAX_STATUS_CODE + 1] = {
    [100] = "Continue",
    [101] = "Switching Protocols",
    [200] = "OK",
    [201] = "Created",
    [202] = "Accepted",
    [204] = "No Content",
    [301] = "Moved Permanently",
    [302] = "Found",
    [304] = "Not Modified",
    [400] = "Bad Request",
    [401] = "Unauthorized",
    [403] = "Forbidden",
    [404] = "Not Found",
    [405] = "Method Not Allowed",
    [409] = "Conflict",
    [410] = "Gone",
    [413] = "Payload Too Large",
    [415] = "Unsupported Media Type",
    [500] = "Internal Server Error",
    [501] = "Not Implemented",
    [502] = "Bad Gateway",
    [503] = "Service Unavailable",
    [504] = "Gateway Timeout"
};

const char *http_status_reason_phrase(int code) 
{
    if (code < 100 || code > HTTP_MAX_STATUS_CODE || http_status_reasons_table[code] == NULL) 
        return "Unknown Status";

    return http_status_reasons_table[code];
}

static const char* content_type_table[HTTP_CONTENT_TYPE_LAST + 1] = {
    [HTTP_CONTENT_NONE]         = NULL, 
    [HTTP_CONTENT_TEXT_PLAIN]   = "text/plain", 
    [HTTP_CONTENT_TEXT_HTML]    = "text/html", 
};

const char* http_content_type_value(Http_content_type_t content_type)
{
    if (content_type < 0 || content_type > HTTP_CONTENT_TYPE_LAST)
        return "application/octet-stream"; 

    return content_type_table[content_type]; 
}

void http_response_make_error(Http_response_t* resp, int status_code)
{
    assert(resp != NULL); 
    memset(resp, 0, sizeof(Http_response_t)); 

    resp->status_code = status_code; 

    resp->content_type = HTTP_CONTENT_TEXT_PLAIN; 
    const char* status_reason = http_status_reason_phrase(resp->status_code); 
    resp->body = (char*)status_reason;  /* please don't kill me */ 
    resp->body_len = strlen(status_reason); 
    resp->body_mem = HTTP_MEM_STATIC; 

    resp->connection_close = 1; 
}

#define RAW_WRITE(fmt, ...) \
    do { \
        n = snprintf(buffer + written, buffer_len - written, fmt, __VA_ARGS__); \
        if (n < 0 || (size_t)n >= buffer_len) \
            return -1; \
        written += n; \
    } while(0)

int http_response_raw(const Http_response_t* resp, char* buffer, size_t buffer_len)
{
    assert(resp != NULL); 
    assert(buffer != NULL); 
    int written = 0; 
    int n; 
    /* first line */ 
    RAW_WRITE("HTTP/1.1 %d %s\r\n", 
            resp->status_code, 
            http_status_reason_phrase(resp->status_code)); 

    /* headers */  
    const char* conn_val = resp->connection_close ? "close" : "keep-alive";
    RAW_WRITE("Connection: %s\r\n", conn_val); 

    for (size_t i = 0; i < resp->headers_count; i++)
    {
        RAW_WRITE("%s: %s\r\n", 
            resp->headers[i].key, 
            resp->headers[i].value); 
    }

    const char* content_type_value = http_content_type_value(resp->content_type); 
    if (content_type_value)
    {
        RAW_WRITE("Content-Type: %s\r\n", content_type_value); 
    }

    RAW_WRITE("Content-Length: %zu\r\n", resp->body_len); 

    /* delimitier */ 
    if (buffer_len - written < 2) return -1;
    memcpy(buffer + written , "\r\n", 2);
    written += 2;

    /* body :3 */ 
    if (buffer_len - written < resp->body_len)
        return -1; 
    
    memcpy(buffer + written , resp->body, resp->body_len); 
    written += resp->body_len; 

    return written; 
}

#undef RAW_WRITE

/* helper function */ 
#define CIRC_WRITE(fmt, ...)\
    do { \
    n = snprintf(tmp, sizeof(tmp), fmt, __VA_ARGS__); \
    if (n < 0 || n >= (int)sizeof(tmp)) \
        return -1; \
    if (http_circ_write(resp_buff, tmp, n) == -1) \
        return -1; \
    written += n; \
    } while(0)

int http_response_raw_circ(const Http_response_t* resp, Http_circ_buff_t* resp_buff)
{
    assert(resp); 
    assert(resp_buff); 

    char tmp[HTTP_MAX_HEADER_LINE]; 
    int written = 0; 
    int n; 

    /* first line */ 
    CIRC_WRITE("HTTP/1.1 %d %s\r\n", 
            resp->status_code, 
            http_status_reason_phrase(resp->status_code)); 

    /* headers */  
    const char* conn_val = resp->connection_close ? "close" : "keep-alive";
    CIRC_WRITE("Connection: %s\r\n", conn_val); 

    for (size_t i = 0; i < resp->headers_count; i++)
    {
        CIRC_WRITE("%s: %s\r\n", 
            resp->headers[i].key, 
            resp->headers[i].value); 
    }

    const char* content_type_value = http_content_type_value(resp->content_type); 
    if (content_type_value)
    {
        CIRC_WRITE("Content-Type: %s\r\n", content_type_value); 
    }

    CIRC_WRITE("Content-Length: %zu\r\n", resp->body_len); 

    /* delimitier */ 
    if (http_circ_write(resp_buff, "\r\n", 2) == -1) 
        return -1; 
    written += 2;

    /* body :3 */ 
    if (http_circ_write(resp_buff, resp->body, resp->body_len) == -1) 
        return -1; 
    written += resp->body_len; 

    return written; 
}

#undef CIRC_WRITE

void http_response_free(Http_response_t* resp)
{
    assert(resp != NULL); 
    if (resp->body_mem == HTTP_MEM_OWNED)
    {
        free(resp->body); 
    }
    for (size_t i = 0; i < resp->headers_count; i++)
    {
        if (resp->headers[i].key_mem == HTTP_MEM_OWNED)
            free(resp->headers[i].key); 

        if (resp->headers[i].value_mem == HTTP_MEM_OWNED)
            free(resp->headers[i].value); 
    }
}
