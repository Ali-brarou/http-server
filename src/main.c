#include <stdio.h> 
#include <unistd.h> 
#include <getopt.h> 
#include <string.h> 
#include <stdlib.h> 
#include <signal.h> 
#include <sys/epoll.h> 

#include "server.h"

void print_help(char* program_name); 
void parse_arguments(int argc, char* argv[], Http_config_t* config); 
void sigint_handler(int sig); 
Http_handler_result_t handler(const Http_request_t* req, Http_response_t* resp); 

int main(int argc, char* argv[])
{
    signal(SIGINT, sigint_handler); /* will call http_trigger_shutdown() */ 

    Http_config_t config = HTTP_DEFAULT_CONFIG; 
    config.handler = &handler; 
    parse_arguments(argc, argv, &config); 

    return http_server_run(&config); 
}

void print_help(char* program_name)
{
    printf("Usage: %s [--host <host>] [--port <port>] [--help]\n", program_name);
    printf("Options:\n");
    printf("  -h, --help              Show this help message\n");
    printf("  -H, --host    <host>    Set the server host (default: %s)\n", HTTP_DEFAULT_HOST);
    printf("  -p, --port    <port>    Set the server port (default: %d)\n", HTTP_DEFAULT_PORT);
    printf("  -b, --backlog <port>    Set the server backlog (default: %d)\n", HTTP_DEFAULT_BACKLOG);
}

/* this is a simple http handler example */ 
#define BODY "<html><head><title>this shit works</title></head><body>Hello, browser!</body></html>"
Http_handler_result_t handler(const Http_request_t* req, Http_response_t* resp)
{
    (void)req; /* request is ignored in this simple example */ 
    resp->status_code = HTTP_OK; 
    resp->content_type = HTTP_CONTENT_TEXT_HTML; 
    resp->body = BODY;  
    resp->body_len = strlen(BODY); 
    resp->body_mem = HTTP_MEM_STATIC; /* tell the server to not free body buffer */ 
    resp->connection_close = 1;       /* close connection after response */  
    return HTTP_HANDLER_OK; 
}

void parse_arguments(int argc, char* argv[], Http_config_t* config)
{
    int opt;  

    static struct option long_options[] = 
    {
        {"help",    no_argument,        0, 'h'}, 
        {"host",    required_argument,  0, 'H'}, 
        {"port",    required_argument,  0, 'p'}, 
        {"backlog", required_argument,  0, 'b'}, 
        {0, 0, 0, 0}, 
    }; 

    while ((opt = getopt_long(argc, argv, "hH:p:b:", long_options, NULL)) != -1)
    {
        switch (opt) 
        {
            case 'h': 
                print_help(argv[0]); 
                exit(EXIT_SUCCESS); 
            case 'H': 
                strncpy(config->host, optarg, HTTP_MAX_HOST_LEN-1); 
                config->host[HTTP_MAX_HOST_LEN-1] = '\0';
                break; 
            case 'p': 
                config->port = atoi(optarg); 
                if (config->port <= 0 || config->port > 65535)  
                {
                    fprintf(stderr, "Error: %s is an invalid port number\n", optarg); 
                    exit(EXIT_FAILURE); 
                }
                break; 
            case 'b': 
                config->backlog = atoi(optarg); 
                if (config->backlog <= 0 || config->backlog > SOMAXCONN)  
                {
                    fprintf(stderr, "Error: %s is an invalid backlog number\n", optarg); 
                    exit(EXIT_FAILURE); 
                }
                break; 
            default: 
                print_help(argv[0]); 
                exit(EXIT_FAILURE); 
        }
    }
}

void sigint_handler(int sig)
{
    (void)sig; 
    http_trigger_shutdown(); 
}
