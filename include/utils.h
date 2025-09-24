#ifndef UTILS_H
#define UTILS_H

#include <stddef.h> 

/* make a socket nonblocking returns -1 in case of an error */ 
int http_socket_set_nonblocking(int sockfd);

int http_parse_sizet(const char* str, size_t* out); 

#endif
