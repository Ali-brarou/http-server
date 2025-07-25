#ifndef SERVER_H
#define SERVER_H

#include "server_context.h"
#include "config.h"
#include "utils.h"
#include "shutdown.h"
#include "epoll_utils.h"
#include "timer.h"


/* prepare the context return -1 if an error */ 
int  http_server_start(Http_server_context_t* ctx, Http_config_t* config); 
/* run the server loop */ 
void http_server_run(Http_server_context_t* ctx); 
/* clean context */ 
void http_server_clean(Http_server_context_t* ctx); 

#endif
