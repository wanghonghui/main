#ifndef _CACHE_H
#define _CACHE_H

typedef struct cache        cache_t;
typedef struct cache_buf    cache_buf_t;
typedef struct cache_ops    cache_ops_t;  
typedef struct cache_param  cache_param_t;
typedef enum   cache_type   cache_type_t;  

typedef void data_evict_func_t(void *data);

typedef void *cache_get_func_t(cache_t *cache, void *key, uint32_t key_len);
typedef void cache_put_func_t(cache_t *cache, cache_buf_t *buf);
typedef cache_buf_t *cache_write_func_t(cache_t *cache, void *key, uint32_t key_len, 
                                 void *data, uint32_t data_len, buf_evict_func_t *evict_func);
typedef void cache_desc_func_t(cache_t *cache, cache_buf_t *buf, void *priv);
typedef void cache_grow_func_t(cache_t *cache, uint32_t size);
typedef void cache_shrink_func_t(cache_t *cache, uint32_t size);
typedef void cache_destroy_func_t(cache_t **cache);

struct cache {
    cache_opt_t *ops;  
};

struct cache_buf {
    void *b_data;
    void *b_data_len;
};

struct cache_ops {
    cache_get_func_t        cache_get;
    cache_put_func_t        cache_put;
    cache_write_func_t      cache_write;
    cache_desc_func_t       cache_desc;
    cache_grow_func_t       cache_grow;
    cache_shrink_func_t     cache_shrink;
    cache_destroy_func_t    cache_destroy;
};

struct cache_param {
    uint32_t cache_size;
    void *priv;
};

enum cache_type {
    NONE_CACHE,
    ARC_CACHE
};

cache_t *cache_factory(cache_type_t type);

#endif 
