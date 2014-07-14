#include <stdlib.h>

#include "cache.h"
#include "arc.h"


static void none_destory(cache_t **cache)
{
    free(*cache);
    *cache = NULL;
}

static cache_ops_t none_ops = {
   .cache_destroy = none_destory, 
};

static cache_t *none_init()
{
    cache_t *cache = malloc(sizeof(cache_t));
    if (cache == NULL)
        return NULL;    
    cache->ops = &none_ops;
    return cache;
}

cache_t *cache_factory(cache_type_t type, cache_param_t *param)
{
    switch (type) {
        case NONE_CACHE:
            return none_init();
        case ARC_CACHE:
            return arc_cache_init(param);
        default:
            return NULL;
    }
}
