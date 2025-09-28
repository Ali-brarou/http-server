#include <assert.h> 
#include <stdlib.h>
#include <string.h>

#include <loom/router.h>

static void route_add(Http_router_t* router, Http_route_t* route); 

int http_route_register(Http_router_t* router,
                        Http_method_t method,
                        const char* path,
                        Http_handler_t handler)
{
    if (!router || !path)
        return -1; 

    Http_route_t* route = malloc(sizeof(Http_route_t)); 
    if (!route)
        return -1; 
    
    memset(route, 0, sizeof(Http_route_t)); 
    route->method = method; 
    route->path = strdup(path); 
    if (!route->path)
    {
        free(route); 
        return -1; 
    }
    route->handler = handler; 

    route_add(router, route); 

    return 0; 
}

Http_handler_t http_router_find(Http_router_t* router, Http_method_t method, const char* path)
{
    for (Http_route_t* route = router->routes; route != NULL; route = route->next)
    {
        if (route->method == method && !strcmp(route->path, path))
            return route->handler; 
    }
    return NULL; 
}

int http_router_init(Http_router_t* router)
{
    if (!router)
        return -1; 
    memset(router, 0, sizeof(Http_router_t)); 
    return 1; 
}

void http_router_clean(Http_router_t* router)
{
    Http_route_t *route, *route_next; 
    for (route = router->routes; route != NULL; route = route_next)
    {
        route_next = route->next;  
        free(route->path); 
        free(route); 
    }
}

static void route_add(Http_router_t* router, Http_route_t* route)
{
    route->next = router->routes; 
    router->routes = route; 
    router->route_count++; 
}
