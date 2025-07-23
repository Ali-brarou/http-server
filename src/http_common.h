#ifndef HTTP_COMMON_H
#define HTTP_COMMON_H

#define HTTP_VERSION_SIZE 4

typedef struct Http_header_s {
    char* key;
    char* value;
} Http_header_t;

#endif
