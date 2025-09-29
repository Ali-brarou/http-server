#ifdef HTTP_USE_MEMMEM
#define _GNU_SOURCE
#endif 
#include <assert.h> 
#include <errno.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/epoll.h> 
#include <netinet/in.h>
#include <unistd.h> 

#include <loom/connection.h>

static Http_connection_t* http_connection_create(int client_fd, Http_timer_t* timer, Http_config_t* cfg); 
static int buffer_process(Http_connection_t* con); 
static void socket_drain(Http_connection_t* con); 
static void write_error_response(Http_connection_t* con, int status_code); 

/* null if can't allocate memory */ 
static Http_connection_t* http_connection_create(int client_fd, Http_timer_t* timer, Http_config_t* cfg)
{
    Http_connection_t* con = malloc(sizeof(Http_connection_t)); 
    if (!con)
    {
        perror("malloc"); 
        return NULL; 
    }
    memset(con, 0, sizeof(Http_connection_t)); 
    con->client_fd = client_fd; 
    con->router = cfg->router; 

    if (http_timer_add_timeout(timer, con, HTTP_CLIENT_TIMEOUT) == -1)
    {
        free(con); 
        return NULL; 
    }

    /* http_circ_init(&con->response_buff, con->response_mem, sizeof(con->response_mem)); */

    return con; 
}

void http_connection_clean(Http_server_context_t* ctx, Http_connection_t* con)
{
    if (con->timeout_index != -1)
        http_timer_invalid_timeout(ctx->timer, con->timeout_index); 
    http_epoll_del_con(ctx->epoll_fd, con); 
    close(con->client_fd); 
    free(con); 

    ctx->active_clients--; 
}

void http_connection_accept(Http_server_context_t* ctx)
{
    assert(ctx != NULL); 
    assert(ctx->epoll_fd != -1 && ctx->listen_fd != -1); 
    struct sockaddr_in client_addr; 
    socklen_t socklen = sizeof(client_addr); 
    int client_fd = accept(ctx->listen_fd, (struct sockaddr *)&client_addr, &socklen) ; 
    if (client_fd == -1)
    {
        perror("accept"); 
        return;  
    }

    if (http_socket_set_nonblocking(client_fd) == -1) 
    {
        close(client_fd); 
        return; 
    }

    Http_connection_t* con = http_connection_create(client_fd, ctx->timer, ctx->cfg); 
    if (!con)
    {
        close(client_fd); 
        return; 
    }

    if (http_epoll_add_con(ctx->epoll_fd, con, EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLHUP) == -1) 
    { free(con); 
        close(client_fd); 
        return; 
    }

    ctx->active_clients++; 
}

static void write_error_response(Http_connection_t* con, int status_code)
{
    HTTP_SET_SHOULD_CLOSE(con->flags); 

    Http_response_t response; 
    http_response_make_error(&response, status_code); 
    /* try sending the response if the response buffer is full aaaa idk */ 
    int used = http_response_raw(&response, con->response + con->response_len, 
                                HTTP_RESPONSE_SIZE - con->response_len); 
    if (used == -1)
        return; /* dont send the error */  

    con->response_len += used; 
    HTTP_SET_WRITING(con->flags); 
}

static void socket_drain(Http_connection_t* con)
{
    int client_fd = con->client_fd; 
    for (;;)  /* drain the buffer :3 */ 
    {
        ssize_t n = read(client_fd, con->buff + con->buff_len, HTTP_REQUEST_SIZE - con->buff_len); 
        if (n == 0)
        {
            break; 
        }
        else if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break; 
            perror("read"); 
            break; 
        }
        con->buff_len += n; 
    }
}

#define HTTP_HEADER_DELIMITER       "\r\n\r\n"
#define HTTP_HEADER_DELIMITER_LEN   4

/* if -1 is returned then stop reading */ 
static int buffer_process(Http_connection_t* con)
{
    for (;;) /* process what's in the buffer */ 
    {
        /* reading headers state */ 
        switch (HTTP_GET_READ_STATE(con->flags))
        {
            case HTTP_READING_HEADERS: 
            {
                char* end = NULL; 
#ifdef HTTP_USE_MEMMEM
                end = memmem(con->buff, con->buff_len, 
                        HTTP_HEADER_DELIMITER, HTTP_HEADER_DELIMITER_LEN); 
#else 
                for (size_t i = 0; i + HTTP_HEADER_DELIMITER_LEN <= con->buff_len ; i++)
                {
                    if (!memcmp(con->buff + i, HTTP_HEADER_DELIMITER, HTTP_HEADER_DELIMITER_LEN))
                    {
                        end = con->buff + i; 
                        break; 
                    }
                }
#endif

                if (!end)
                {
                    if (con->buff_len >= HTTP_REQUEST_SIZE)
                        write_error_response(con, HTTP_PAYLOAD_TOO_LARGE); 
                    return -1; 
                }
                
                /* can parse the headers now */  
                con->header_len = end - con->buff + HTTP_HEADER_DELIMITER_LEN;  
                if (http_request_parse(&con->request, con->buff, con->header_len) == -1)
                {
                    write_error_response(con, HTTP_BAD_REQUEST); 
                    return -1; 
                }
                
                if (con->request.body_len == 0) 
                    HTTP_SET_READ_STATE(con->flags, HTTP_REQUEST_READY); 
                else 
                {
                    if (con->request.body_len >= HTTP_REQUEST_SIZE - con->header_len)
                    {
                        write_error_response(con, HTTP_PAYLOAD_TOO_LARGE); 
                        return -1; 
                    }
                    con->request.body = &con->buff[con->header_len]; 
                    con->body_len = con->request.body_len;  
                    HTTP_SET_READ_STATE(con->flags, HTTP_READING_BODY); 
                }
            }
            break; 
            case HTTP_READING_BODY: 
            /* succesfully recieved full body */  
            if (con->buff_len - con->header_len >= con->body_len) 
                HTTP_SET_READ_STATE(con->flags, HTTP_REQUEST_READY); 
            else 
                return -1; 
            break; 
            case HTTP_REQUEST_READY: 
            {
                /* make the handler create a response */ 
                Http_response_t response; 
                memset(&response, 0, sizeof response); 

                Http_handler_t handler = http_router_find(con->router, 
                                                        con->request.method,
                                                        con->request.path); 

                /* router didn't find a handler */ 
                if (!handler)
                {
                    write_error_response(con, HTTP_NOT_FOUND); 
                    return -1; 
                }

                if (handler(&con->request, &response) == HTTP_HANDLER_ERR)
                {
                    write_error_response(con, HTTP_INTERNAL_SERVER_ERROR); 
                    return -1; 
                }
                int used = http_response_raw(&response, con->response + con->response_len, HTTP_RESPONSE_SIZE - con->response_len); 
                http_response_free(&response); 
                if (used == -1)
                {
                    write_error_response(con, HTTP_PAYLOAD_TOO_LARGE); 
                    return -1; 
                }
                con->response_len += used; 
                HTTP_SET_WRITING(con->flags); 
                if (response.connection_close)
                {
                    HTTP_SET_SHOULD_CLOSE(con->flags); 
                    return -1; 
                }
                else
                {
                    /* reset buffer */  
                    size_t remains = con->buff_len - con->header_len - con->body_len; 
                    if (remains > 0)
                        memmove(con->buff, con->buff + con->body_len + con->header_len, remains); 
                    HTTP_SET_READ_STATE(con->flags, HTTP_READING_HEADERS); 
                    con->buff_len = remains; 
                    con->body_len = 0; 
                    }
            }
            break; 

        }
    }
    return 0; 
}

void http_connection_read(Http_connection_t* con)
{
    assert(con != NULL && con->client_fd != -1); 
    for (;;)
    {
        socket_drain(con); /* drain :3 */ 

        if (buffer_process(con) == -1)
            return; 
    }
}

void http_connection_write(Http_connection_t* con) 
{
    while (con->response_sent < con->response_len)   
    {
        /* MSG_NOSIGNAL to prevent SIGPIPE */
        ssize_t n = send(con->client_fd, 
                con->response + con->response_sent, 
                con->response_len - con->response_sent,
                MSG_NOSIGNAL); 
        if (n == -1) 
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break; 
            else 
            {
                perror("write"); 
                break; 
            }
        }

        con->response_sent += n; 
    }
    if (con->response_sent >= con->response_len)
    {
        con->response_len = 0;
        con->response_sent = 0; 
        HTTP_CLEAR_WRITING(con->flags); 
    }
}

void http_connection_update_events(int epoll_fd, Http_epoll_item_t* con_item)
{
    assert(con_item != NULL); 
    assert(con_item->type == HTTP_ITEM_CLIENT); 
    assert(con_item->con != NULL); 

    Http_connection_t* con = con_item->con; 
    /* if it's not writing and should close flags is set close the connection */ 
    if (!HTTP_IS_WRITING(con->flags) && HTTP_SHOULD_CLOSE(con->flags))
        HTTP_SET_CLOSING(con->flags); 

    /* there is nothing to do */ 
    if (HTTP_IS_CLOSING(con->flags)) 
        return; 

    uint32_t events = EPOLLET | EPOLLRDHUP | EPOLLHUP;  

    if (HTTP_IS_WRITING(con->flags)) /* if writing flag is set then add OUT event */ 
        events |= EPOLLOUT; 
    if (!HTTP_SHOULD_CLOSE(con->flags)) /* if should close is not set than add IN event */ 
        events |= EPOLLIN; 

    http_epoll_mod_con(epoll_fd, con_item, events); 
}
