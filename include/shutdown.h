#ifndef SHUTDOWN_H
#define SHUTDOWN_H

#include "server_context.h"
#include "epoll_utils.h"

/* returns shutdown fd if success or -1 if error */ 
int  http_shutdown_setup(int epoll_fd);
void http_trigger_shutdown(Http_server_context_t* ctx);
void http_shutdown_close(int shutdown_fd);

#endif
