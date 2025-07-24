#include <assert.h> 
#include <errno.h> 
#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/epoll.h> 
#include <sys/eventfd.h> 

#include "epoll_utils.h"

static int shutdown_fd = -1; 

int http_epoll_create_instance(void)
{
    int epoll_fd = epoll_create(1); 
    if (epoll_fd == -1)
    {
        perror("epoll_create"); 
        return -1; 
    }
    return epoll_fd; 
}

void http_epoll_close(int epoll_fd)
{
    if (epoll_fd == -1)
        return; 
    close(epoll_fd); 
}

int http_epoll_add_fd(int epoll_fd, Http_epoll_item_type_t type, int fd, uint32_t events)
{
    struct epoll_event ev; 
    ev.events = events; 
    Http_epoll_item_t* item = malloc(sizeof(Http_epoll_item_t)); 
    if (!item)
    {
        perror("malloc"); 
        return -1; 
    }

    item->type = type; 
    item->fd   = fd; 
    
    ev.data.ptr = item; 
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        perror("epoll_ctl"); 
        free(item); 
        return -1; 
    }
    
    return 0; 
}

int http_epoll_add_timer(int epoll_fd, Http_timer_t* timer, uint32_t events)
{
    assert(epoll_fd != -1); 
    assert(timer != NULL); 
    int timer_fd = timer->fd;  
    struct epoll_event ev; 
    ev.events = events; 
    Http_epoll_item_t* item = malloc(sizeof(Http_epoll_item_t)); 
    if (!item)
    {
        perror("malloc"); 
        return -1; 
    }
     
    item->type = HTTP_ITEM_TIMER, 
    item->timer  = timer; 

    ev.data.ptr = item; 
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, timer_fd, &ev) == -1)
    {
        perror("epoll_ctl"); 
        free(item); 
        return -1; 
    }

    return 0; 
}

int http_epoll_add_con(int epoll_fd, Http_connection_t* con, uint32_t events)
{
    assert(epoll_fd != -1); 
    assert(con != NULL); 
    int client_fd = con->client_fd; 
    struct epoll_event ev; 
    ev.events = events; 
    Http_epoll_item_t* item = malloc(sizeof(Http_epoll_item_t)); 
    if (!item)
    {
        perror("malloc"); 
        return -1; 
    }
     
    item->type = HTTP_ITEM_CLIENT, 
    item->con  = con; 

    ev.data.ptr = item; 
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
    {
        perror("epoll_ctl"); 
        free(item); 
        return -1; 
    }

    return 0; 
}

int http_epoll_mod_con(int epoll_fd, Http_epoll_item_t* con_item, uint32_t events)
{
    int fd = con_item->con->client_fd; 
    struct epoll_event ev; 
    ev.events = events; 
    ev.data.ptr = con_item; 
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev) == -1)
    {
        perror("epoll_ctl"); 
        return -1; 
    }

    return 0; 
}

void http_epoll_del_con(int epoll_fd, Http_connection_t* con)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, con->client_fd, NULL); 
}

typedef enum {
    HANDLE_SHUTDOWN,  
    HANDLE_CONTINUE, 
    HANDLE_ERROR, 
} Handle_result; 

static Handle_result handle_item_event(Http_server_context_t* ctx, Http_epoll_item_t* item, uint32_t events); 
static void handle_client(Http_server_context_t* ctx, Http_epoll_item_t* con_item, uint32_t events); 
static void close_client(int epoll_fd, Http_epoll_item_t* con_item, Http_timer_t* timer); 

static Handle_result handle_item_event(Http_server_context_t* ctx, Http_epoll_item_t* item, uint32_t events)
{
    switch (item->type)
    {
        case HTTP_ITEM_SHUTDOWN: /* shutdown is triggered */ 
        {
            int shutdown_fd = item->fd; 
            uint64_t u;  
            read(shutdown_fd, &u, sizeof u); 
            return HANDLE_SHUTDOWN; 
        }
        case HTTP_ITEM_LISTENER: /* it's a new connection */  
        {
            http_connection_accept(ctx); 
            return HANDLE_CONTINUE; 
        } 
        case HTTP_ITEM_CLIENT: /* handle a client */  
        {
            handle_client(ctx, item, events); 
            return HANDLE_CONTINUE; 
        }
        case HTTP_ITEM_TIMER: 
        {
            int timer_fd = item->timer->fd; 
            uint64_t u; 
            read(timer_fd, &u, sizeof u); 
            Http_timer_event_t event; 
            if (http_timer_pop_recent(item->timer, &event) == -1)
            {
                return HANDLE_ERROR; 
            }
            
            if (event.flag == HTTP_TIMER_EVENT_VALID)
            {
                event.con->timeout_index = -1; 
                http_connection_clean(event.con, ctx->epoll_fd, ctx->timer); 
            }

            return HANDLE_CONTINUE; 
        }
        default: 
            fprintf(stderr, "Unexpected epoll type\n"); 
            return HANDLE_ERROR; 
    }
}

static void handle_client(Http_server_context_t* ctx, Http_epoll_item_t* con_item, uint32_t events)
{
    Http_connection_t* con = con_item->con; 

    if (events & EPOLLIN)
    {
        http_connection_read(con); 
        http_connection_update_events(ctx->epoll_fd, con_item); 
        if (HTTP_IS_CLOSING(con->flags))
        {
            close_client(ctx->epoll_fd, con_item, ctx->timer); 
            return; /* no need to check if client has closed connection */ 
        }
    }

    if (events & EPOLLOUT)
    {
        http_connection_write(con); 
        http_connection_update_events(ctx->epoll_fd, con_item); 
        if (HTTP_IS_CLOSING(con->flags))
        {
            close_client(ctx->epoll_fd, con_item, ctx->timer); 
            return; /* no need to check if client has closed connection */ 
        }
    }
    /* client has closed connection or connection is dead */ 
    if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) 
    {
        close_client(ctx->epoll_fd, con_item, ctx->timer); 
    }
}

static void close_client(int epoll_fd, Http_epoll_item_t* con_item, Http_timer_t* timer)
{
    assert(con_item->type == HTTP_ITEM_CLIENT); 

    http_connection_clean(con_item->con, epoll_fd, timer); 
    free(con_item); 
}

int http_epoll_run_loop(Http_server_context_t* ctx)
{
    struct epoll_event *events = calloc(ctx->cfg->max_events, sizeof(struct epoll_event)); 
    if (!events)
    {
        perror("calloc"); 
        return -1; 
    }

    for (;;)
    {
        int nfds = epoll_wait(ctx->epoll_fd, events, ctx->cfg->max_events, -1); 
        if (nfds < 0)
        {
            if (errno == EINTR) 
                continue; 
            perror("epoll_wait"); 
            break; 
        }
        for (int i = 0; i < nfds; i++)
        {
            Http_epoll_item_t* item = events[i].data.ptr; 
            assert(item != NULL); 

            Handle_result result = handle_item_event(ctx, item, events[i].events); 
            switch (result)
            {
                case HANDLE_SHUTDOWN:
                    goto shutdown; 

                case HANDLE_ERROR:
                    fprintf(stderr, "Error handling event\n");
                    goto shutdown; 

                case HANDLE_CONTINUE:
                default:
                    break;
            }
        }
    }
shutdown: 
    free(events); 
    return 0;
}


int http_shutdown_setup(int epoll_fd)
{
    shutdown_fd = eventfd(0, EFD_NONBLOCK); 
    if (shutdown_fd == -1)
    {
        perror("eventfd"); 
        return -1; 
    }
    return http_epoll_add_fd(epoll_fd, HTTP_ITEM_SHUTDOWN,shutdown_fd, EPOLLIN); 
}

void http_trigger_shutdown(void)
{
    if (shutdown_fd == -1)
        return; 
    uint64_t u = 1; 
    write(shutdown_fd, &u, sizeof u); 
}

void http_shutdown_close(void)
{
    if (shutdown_fd == -1)
        return; 
    close(shutdown_fd); 
    shutdown_fd = -1; 
}
