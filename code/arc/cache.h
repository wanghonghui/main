#ifndef _CACHE_H
#define _CACHE_H

#include <stdint.h>

typedef struct cache        cache_t;
typedef struct cache_buf    cache_buf_t;
typedef struct cache_ops    cache_ops_t;  
typedef struct cache_param  cache_param_t;
typedef enum   cache_type   cache_type_t;  

struct cache {
    cache_ops_t *ops;  
};

struct cache_buf {
    uint32_t            b_data_len;
    void                *b_data;
    void (*data_evict) (void *data, uint32_t len);
};

struct cache_ops {
    cache_buf_t *(*cache_get) (cache_t *cache, void *key, uint32_t key_len, uint32_t cache_len);
    void (*cache_put) (cache_t *cache, cache_buf_t *buf);
    void (*cache_desc) (cache_t *cache, cache_buf_t *buf, void *priv);
    void (*cache_grow) (cache_t *cache, uint32_t size);
    void (*cache_shrink) (cache_t *cache, uint32_t size);
    void (*cache_destroy) (cache_t **cache);
};

struct cache_param {
    uint32_t cache_size;
    void *priv;
};

enum cache_type {
    NONE_CACHE,
    ARC_CACHE
};

cache_t *cache_factory(cache_type_t type, cache_param_t *param);

#endif 
