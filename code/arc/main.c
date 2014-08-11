#include <stdio.h>
#include "cache.h"


int main(int argc, char **argv) 
{
    uint32_t               key[2];
    cache_t                *cache; 
    cache_buf_t            *buf;  
    cache_param_t          param;
    
    key[0] = 1;
    key[1] = 2;

    param.cache_size = 1024*1024*1024;
    cache = cache_factory(ARC_CACHE, &param);
    buf = cache->ops->cache_get(cache, key, sizeof(key), 65536);     
    if (buf == NULL)
       printf("buf error\n"); 
    buf->b_data_len = 110;
    cache->ops->cache_put(cache, buf);     
    key[1] = 3;
    buf = cache->ops->cache_get(cache, key, sizeof(key), 65536);     
    if (buf == NULL)
       printf("buf error\n"); 
    cache->ops->cache_put(cache, buf);     
    buf = cache->ops->cache_get(cache, key, sizeof(key), 65536);     
    if (buf == NULL)
       printf("buf error\n"); 
    cache->ops->cache_put(cache, buf);     
    printf("data len:%d", buf->b_data_len);
    return 0;
}
