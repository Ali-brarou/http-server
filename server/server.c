#include <assert.h> 
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/epoll.h>

#include <loom/server.h>

static int http_server_setup(Http_config_t* cfg); 
static void http_server_close(int server_fd); 

int http_server_start(Http_server_context_t* ctx, Http_config_t* config)
{
    if (!config)
    {
        fprintf(stderr, "Error: no config was provided\n") ;
        return -1; 
    }
    if (!config->router)
    {
        fprintf(stderr, "Error: No router provided in config\n"); 
        return -1; 
    }

    ctx->cfg = config; 
    ctx->active_clients = 0; 

    ctx->listen_fd = http_server_setup(config);
    if (ctx->listen_fd == -1)
    {
        fprintf(stderr, "Error: failed getting listening socket\n");
        return -1;
    }

    ctx->epoll_fd = http_epoll_create_instance();
    if (ctx->epoll_fd == -1)
    {
        fprintf(stderr, "Error: failed creating epoll instance\n");
        return -1;
    }

    ctx->shutdown_fd = http_shutdown_setup(ctx->epoll_fd); 
    if (ctx->shutdown_fd == -1)
    {
        fprintf(stderr, "Error: failed setting up shutdown event\n");
        return -1;
    }
    
    ctx->timer = http_timer_create(); 
    if (!ctx->timer)
    {
        fprintf(stderr, "error :failed creating timer\n"); 
        return -1; 
    }

    if (http_epoll_add_timer(ctx->epoll_fd, ctx->timer, EPOLLET | EPOLLIN) == -1)
    {
        fprintf(stderr, "error :failed adding timer to epoll\n"); 
        return -1; 
    }

    printf("server listening on %s:%d\n", config->host, config->port);

    if (http_epoll_add_fd(ctx->epoll_fd, HTTP_ITEM_LISTENER, ctx->listen_fd, EPOLLIN) == -1)
    {
        return -1;
    }
    return 0; 
}

void http_server_run(Http_server_context_t* ctx)
{
    /* main loop */
    http_epoll_run_loop(ctx); 
}

int http_server_setup(Http_config_t* cfg)
{
    if (!cfg)
        return -1; 
    struct addrinfo hints, *res, *p; 
    int status; 
    int listen_fd = -1; 
    int yes = 1; 
    char port_str[NI_MAXSERV]; 

    memset(&hints, 0, sizeof hints); 

    hints.ai_family   = AF_UNSPEC;  /* handle both ivp4 and ivp6 */ 
    hints.ai_socktype = SOCK_STREAM;/* HTTP is built on top of the TCP duh */  
    hints.ai_flags    = AI_PASSIVE; /* fill the ip */  

    snprintf(port_str, sizeof port_str, "%d", cfg->port); 

    if ((status = getaddrinfo(cfg->host, port_str, &hints, &res)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status)); 
        return -1;  
    }

    for (p = res; p != NULL; p = p->ai_next)
    {
        listen_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol); 
        if (listen_fd ==  -1)
        {
            perror("socket"); 
            goto fail; 
        }

        if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1)
        {
            perror("setsockopt"); 
            goto fail; 
        }

        if (bind(listen_fd, p->ai_addr, p->ai_addrlen) == -1)
        {
            perror("bind"); 
            goto fail; 
        }

        break; 
fail: 
        if (listen_fd != -1)
            close(listen_fd); 
        listen_fd = -1; 
    }

    freeaddrinfo(res); 

    if (listen_fd == -1)
        return -1; 

    if (http_socket_set_nonblocking(listen_fd) == -1)
        return -1; 

    if (listen(listen_fd, cfg->backlog) == -1)
    {
        perror("listen"); 
        close(listen_fd); 
        return -1; 
    }

    return listen_fd;
}

static void http_server_close(int server_fd)
{
    assert(server_fd != -1); 

    close(server_fd); 
}

void http_server_clean(Http_server_context_t* ctx)
{
    /* clean up */
    http_epoll_close(ctx->epoll_fd);
    http_timer_clean(ctx->timer); 
    http_server_close(ctx->listen_fd);
    http_shutdown_close(ctx->shutdown_fd);
}


Http_handler_result_t http_handler_static_file(Http_request_t* req,
                                               Http_response_t* resp,
                                               const char* file_path,
                                               Http_content_type_t content_type)
{
    (void) req; 

    FILE* file = fopen(file_path, "rb"); 
    if (!file)
    {
        http_response_make_error(resp, HTTP_NOT_FOUND); 
        return HTTP_HANDLER_OK; 
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file); 
        http_response_make_error(resp, HTTP_INTERNAL_SERVER_ERROR); 
        return HTTP_HANDLER_OK; 
    }

    long file_size = ftell(file); 
    if (file_size < 0)
    {
        fclose(file); 
        http_response_make_error(resp, HTTP_NOT_FOUND); 
        return HTTP_HANDLER_OK; 
    }

    if (file_size == 0)
    {
        fclose(file); 
        http_response_make_error(resp, HTTP_NOT_FOUND); 
        return HTTP_HANDLER_OK; 
    }

    rewind(file); 

    char* buffer = malloc(file_size); 
    if (!buffer)
    {   
        fclose(file); 
        http_response_make_error(resp, HTTP_INTERNAL_SERVER_ERROR); 
        return HTTP_HANDLER_OK; 
    }

    size_t read_bytes = fread(buffer, 1, (size_t)file_size, file); 
    fclose(file); 
    if (read_bytes != (size_t)file_size)
    {
        free(buffer); 
        http_response_make_error(resp, HTTP_INTERNAL_SERVER_ERROR); 
        return HTTP_HANDLER_OK; 
    }

    resp->status_code = HTTP_OK; 
    resp->content_type = content_type; 
    resp->body = buffer; 
    resp->body_len = (size_t)file_size; 
    resp->body_mem = HTTP_MEM_OWNED; /* free memory */ 
    resp->connection_close = 0; 

    return HTTP_HANDLER_OK; 
}
