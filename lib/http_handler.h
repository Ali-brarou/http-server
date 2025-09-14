#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

/* forward declaration */ 
typedef struct Http_request_s Http_request_t; 
typedef struct Http_response_s Http_response_t; 

typedef enum Http_hander_result_e {
    HTTP_HANDLER_ERR,   
    HTTP_HANDLER_OK,  
} Http_handler_result_t; 

/* if this returns error, server will cut connection immediately */ 
typedef Http_handler_result_t (*Http_handler)(Http_request_t* req, Http_response_t* resp); 

#endif
