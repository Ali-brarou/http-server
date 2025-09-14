#include <assert.h> 
#include <stdio.h> 
#include <sys/epoll.h> 
#include <sys/eventfd.h> 
#include <unistd.h> 

#include "shutdown.h"

int http_shutdown_setup(int epoll_fd)
{
    int shutdown_fd = eventfd(0, EFD_NONBLOCK);
    if (shutdown_fd == -1)
    {
        perror("eventfd");
        return -1;
    }

    if (http_epoll_add_fd(epoll_fd, HTTP_ITEM_SHUTDOWN,shutdown_fd, EPOLLIN) == -1)
    {
        close(shutdown_fd); 
        return -1; 
    }

    return shutdown_fd; 
}

void http_trigger_shutdown(Http_server_context_t* ctx)
{
    assert(ctx != NULL); 
    assert(ctx->shutdown_fd != -1); 
    uint64_t u = 1;
    write(ctx->shutdown_fd, &u, sizeof u);
}

void http_shutdown_close(int shutdown_fd)
{
    assert(shutdown_fd != -1); 
    close(shutdown_fd);
}
