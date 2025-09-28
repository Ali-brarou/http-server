#ifndef SERVER_H
#define SERVER_H

#include "router.h"
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

/* generic static file handler */
Http_handler_result_t http_handler_static_file(Http_request_t* req,
                                               Http_response_t* resp,
                                               const char* file_path,
                                               Http_content_type_t content_type);

#define DEFINE_STATIC_HANDLER(name, path, content_type_enum)                \
Http_handler_result_t name(Http_request_t* req, Http_response_t* resp)      \
{                                                                           \
    return http_handler_static_file(req, resp, path, content_type_enum);    \
}

#endif
