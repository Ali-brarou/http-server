#include <circ_buff.h> 
#include <assert.h> 

#define IS_POW_OF_TWO(n) ((n) && !((n) & ((n)-1)))

void http_circ_init(Http_circ_buff_t* c, char* buff, size_t size)
{
    assert(IS_POW_OF_TWO(size)); 
    assert(c); 
    assert(buff); 

    c->buff = buff; 
    c->size = size; 
    c->head = 0; 
    c->tail = 0; 
}

int http_circ_write(Http_circ_buff_t* c, char* data, size_t len)
{
    assert(c); 
    assert(data); 
    assert(len); 
    
    const size_t space = HTTP_CIRC_SPACE(c); 
    if (len > space)
        return -1; 

    size_t head = c->head & (c->size - 1); 
    const size_t first_chunk = (len < c->size - head) ? len : (c->size - head); 
    const size_t second_chunk = len - first_chunk; 

    memcpy(&c->buff[head], &data[0], first_chunk); 
    memcpy(&c->buff[0], &data[first_chunk], second_chunk); 

    c->head += len; 

    return (int)len; 
}
