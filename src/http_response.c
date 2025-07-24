#include <assert.h> 
#include <stdio.h> 
#include <stdlib.h> 

#include "http_response.h"

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
    {
        return "Unknown Status";
    }
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
    resp->content_type = HTTP_CONTENT_NONE; 
    resp->connection_close = 1; 
}

int http_response_raw(const Http_response_t* resp, char* buffer, size_t buffer_len)
{
    assert(resp != NULL); 
    assert(buffer != NULL); 
    size_t used = 0; 
    int written; 
    /* first line */ 
    written = snprintf(buffer, buffer_len, "HTTP/1.1 %d %s\r\n", 
            resp->status_code, http_status_reason_phrase(resp->status_code)); 
    if (written < 0 || (size_t)written >= buffer_len) 
        return -1; 
    used += (size_t )written; 

    /* headers */  

    const char* connection_value = resp->connection_close ? "close" : "keep-alive";
    written = snprintf(buffer + used, buffer_len - used, "Connection: %s\r\n", connection_value);
    if (written < 0 || (size_t)written >= buffer_len - used) return -1;
    used += (size_t)written;

    for (size_t i = 0; i < resp->headers_count; i++)
    {
        written = snprintf(buffer + used, buffer_len - used, "%s: %s\r\n",
                resp->headers[i].key, resp->headers[i].value); 
        if (written < 0 || (size_t)written >= buffer_len - used) return -1;
        used += (size_t)written;
    }

    const char* content_type_value = http_content_type_value(resp->content_type); 
    if (content_type_value)
    {
        written = snprintf(buffer + used, buffer_len - used, "Content-Type: %s\r\n", content_type_value);
        if (written < 0 || (size_t)written >= buffer_len - used) return -1;
        used += (size_t)written;
    }

    written = snprintf(buffer + used, buffer_len - used, "Content-Length: %ld\r\n", resp->body_len);
    if (written < 0 || (size_t)written >= buffer_len - used) return -1;
    used += (size_t)written;

    /* delimitier */ 
    if (buffer_len - used < 2) return -1;
    memcpy(buffer + used, "\r\n", 2);
    used += 2;

    /* body :3 */ 
    if (buffer_len - used < resp->body_len)
        return -1; 
    if (resp->body_len != 0)
    {
        memcpy(buffer + used, resp->body, resp->body_len); 
        used += resp->body_len; 
    }

    return used; 
}

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
