#include <fcntl.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <limits.h> 
#include <stdint.h> 
#include <errno.h> 

#include <utils.h>

int http_socket_set_nonblocking(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1 || fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl");
        return -1;
    }
    return 0;
}


int http_parse_sizet(const char *str, size_t *out) {
    char *endptr;
    errno = 0;

    unsigned long long val = strtoull(str, &endptr, 10);

    if (errno == ERANGE) {
        return -1; /* overflow */  
    }

    if (endptr == str || *endptr != '\0') {
        return -1; 
    }

    if (val > (unsigned long long)SIZE_MAX) {
        return -1; 
    }

    *out = (size_t)val;
    return 0; 
}
