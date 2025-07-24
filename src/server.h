#ifndef SERVER_H
#define SERVER_H

#include "server_context.h"
#include "config.h"
#include "utils.h"
#include "epoll_utils.h"
#include "timer.h"


void http_server_init(void); 
/* get the server listening socket fd */ 
int  http_server_setup(Http_config_t* config);
/* int  http_server_start(Http_config_t* config); */ 
int  http_server_run(Http_config_t* config); 
void http_server_close(int server_fd); 
void http_server_clean(void); 

#endif
