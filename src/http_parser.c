#include <ctype.h> 
#include <string.h> 
#include <strings.h> 
#include <stdio.h> /* debug */ 

#include "http_parser.h"

static const Http_method_mapping_t http_methods_table[] = {
    { HTTP_METHOD_GET,     "GET"     },
    { HTTP_METHOD_POST,    "POST"    },
    { HTTP_METHOD_PUT,     "PUT"     },
    { HTTP_METHOD_DELETE,  "DELETE"  },
    { HTTP_METHOD_HEAD,    "HEAD"    },
    { HTTP_METHOD_OPTIONS, "OPTIONS" },
    { HTTP_METHOD_PATCH,   "PATCH"   },
}; 

static const size_t http_methods_table_size = sizeof(http_methods_table) / sizeof(http_methods_table[0]); 

/* a bit slow */ 
Http_method_t http_method_from_string(char* str)
{
    if (!str) 
        return HTTP_METHOD_UNKNOWN; 

    for (size_t i = 0; i < http_methods_table_size; i++)
    {
        if (!strcasecmp(http_methods_table[i].name, str)) /* http methods are case insensitive */ 
            return http_methods_table[i].method; 
    }

    return HTTP_METHOD_UNKNOWN; 
}

char* http_request_search_header(Http_request_t* request, const char* key)
{
    for (size_t i = 0; i < request->headers_count; i++)
    {
        if (!strcasecmp(request->headers[i].key, key))
            return request->headers[i].value; 
    }
    return NULL; 
}

static const char tchar_table[256] = {
    ['!'] = 1, ['#'] = 1, ['$'] = 1, ['%'] = 1, ['&'] = 1,
    ['\''] = 1, ['*'] = 1, ['+'] = 1, ['-'] = 1, ['.'] = 1,
    ['^'] = 1, ['_'] = 1, ['`'] = 1, ['|'] = 1, ['~'] = 1,
};

/* allowed path chars */ 
static const char pchar_table[256] = {
    ['!'] = 1, ['#'] = 1, ['$'] = 1, ['%'] = 1, ['&'] = 1,
    ['\''] = 1, ['*'] = 1, ['+'] = 1, ['-'] = 1, ['.'] = 1,
    ['^'] = 1, ['_'] = 1, ['`'] = 1, ['|'] = 1, ['~'] = 1,
    ['/'] = 1, [':'] = 1, ['@'] = 1, ['?'] = 1, ['='] = 1, 
    [','] = 1,
};

/* allowed field chars */ 
static const char fchar_table[256] = {
    [0x09] = 1, 
    [0x20 ... 0x7E] = 1, 
    [0x80 ... 0xFF] = 1, 
}; 

static int istchar(char c); 
static int ispchar(char c); 
static int isfchar(char c); 
/* returns offset if parsed correctly or -1 if malforemed */ 
static int parse_request_line(Http_request_t* request, char* raw, size_t raw_len);  
static int parse_request_headers(Http_request_t* request, char* raw, size_t raw_len, size_t offset); 

#define MIN_RAW_REQUEST_SIZE 14 /* sus if under 14 bytes */ 
#define HTTP_VERSION_PREFIX "HTTP/"

/* strict parser (•`╭╮´•) */  
int http_request_parse(Http_request_t* request, char* request_raw, size_t request_raw_len)
{
    if (!request || !request_raw || request_raw_len < MIN_RAW_REQUEST_SIZE)  
        return -1; 
    memset(request, 0, sizeof(Http_request_t)); 
    int offset; 

    /* parse first line */ 
    offset = parse_request_line(request, request_raw, request_raw_len); 
    if (offset == -1)
        return -1; 

    offset = parse_request_headers(request, request_raw, request_raw_len, offset); 
    if (offset == -1)
        return -1; 

    /* find body length */ 
    char* content_length_value = http_request_search_header(request, "Content-Length"); 
    if (content_length_value)
    {
        if (http_parse_sizet(content_length_value, &request->body_len) == -1)
            return -1; 
    }

    return offset; 
}

static int istchar(char c)
{
    return isalnum((int)c) || tchar_table[(unsigned char)c]; 
}
static int ispchar(char c)
{
    return isalnum((int)c) || pchar_table[(unsigned char)c]; 
}
static int isfchar(char c)
{
    return fchar_table[(unsigned char)c]; 
}

static int parse_request_line(Http_request_t* req, char* raw, size_t raw_len)
{
    size_t offset = 0, prev_offset;  
    
    /* parse METHOD */ 
    req->method_str = &raw[offset]; 
    while (offset < raw_len && raw[offset] != ' ')
    {
        if (!istchar(raw[offset]))
            return -1; 
        offset++; 
    }

    if (offset >= raw_len || offset == 0) /* malformed method */ 
        return -1; 
    raw[offset] = '\0'; 
    offset++; 
    req->method = http_method_from_string(req->method_str); 

    /* parse PATH */ 
    prev_offset = offset; 
    req->path = &raw[offset]; 
    while (offset < raw_len && raw[offset] != ' ')
    {
        if (!ispchar(raw[offset]))
            return -1; 
        offset++; 
    }
    if (offset >= raw_len || offset == prev_offset)
        return -1; 
    raw[offset] = '\0'; 
    offset++; 

    /* parse vesrion */ 
    /* this parser will support only version 1.0 and 1.1 so it must check minor version */ 
    if (offset + strlen(HTTP_VERSION_PREFIX) + 3 > raw_len) 
        return -1; 
    if (strncmp(&raw[offset], HTTP_VERSION_PREFIX, strlen(HTTP_VERSION_PREFIX)))
        return -1; 

    offset += strlen(HTTP_VERSION_PREFIX); 
    if (!isdigit((int)raw[offset]))
        return -1; 
    req->version[0] = raw[offset]; 
    offset++; 

    if (raw[offset] != '.')
        return -1; 
    req->version[1] = '.'; 
    offset++; 

    if (!isdigit((int)raw[offset]))
        return -1; 
    req->version[2] = raw[offset]; 
    req->version[3] = '\0'; 
    offset++; 

    /* CRLF */ 
    if (offset + 2 > raw_len || raw[offset] != '\r' || raw[offset+1] != '\n')
        return -1; 
    offset += 2; 

    return offset; 
}



static int parse_request_headers(Http_request_t* req, char* raw, size_t raw_len, size_t offset)
{
    size_t prev_offset, end_offset; 
    while (offset + 1 < raw_len)
    {
        if (raw[offset] == '\r' && raw[offset+1] == '\n')
        {
            offset += 2; 
            return offset; 
        }
        
        /* parsing name */ 
        prev_offset = offset; 
        req->headers[req->headers_count].key = &raw[offset]; 
        while (offset < raw_len && raw[offset] != ':')
        {
            if (!istchar(raw[offset]))
                return -1; 
            offset++; 
        }

        if (offset >= raw_len || raw[offset] != ':')
            return -1 ;

        if (offset == prev_offset) 
            return -1; 

        raw[offset] = '\0'; 
        offset++; 
        /* parsing value */ 
        while (offset < raw_len && (raw[offset] == ' ' || raw[offset] == '\t'))
        {
            offset++; 
        }

        if (offset >= raw_len || raw[offset] == ' ' || raw[offset] == '\t')
            return -1; 

        req->headers[req->headers_count].value = &raw[offset]; 
        prev_offset = offset; 
        while (offset < raw_len && raw[offset] != '\r')
        {
            if (!isfchar(raw[offset]))
                return -1; 
            offset++; 
        }
        if (offset + 1 >= raw_len || raw[offset] != '\r' || raw[offset+1] != '\n')
            return -1; 
        end_offset = offset; 
        while (end_offset > prev_offset  && 
                (raw[end_offset-1] == ' ' || raw[end_offset-1] == '\t'))
        {
            end_offset--;   
        }
        raw[end_offset] = '\0'; 
        offset += 2; 
        req->headers_count++; 
        if (req->headers_count >= HTTP_MAX_HEADERS)
        {
            /* too many headers */  
            if (offset + 1 >= raw_len || raw[offset] != '\r' || raw[offset+1] != '\n') 
            {
                return -1;
            }
            offset += 2; 
            return offset; 
        }
    }
    return -1; 
}


/* debug */ 

void http_request_print(Http_request_t* request)
{
    if (!request)
    {
        printf("Null request\n"); 
        return; 
    }
    
    printf("Http method : %s\n", request->method_str); 
    printf("Path : %s\n", request->path); 
    printf("version : %3s\n", request->version); 
    for (size_t i = 0; i < request->headers_count; i++)
    {
        printf("Header: %s -> %s\n", request->headers[i].key,request->headers[i].value); 
    }
}
