#include "cache.h"
#include "arc.h"

static struct cache_ops_t none_ops = {
   .destroy = none_destory; 
}

static void none_destory(cache_t **cache)
{
    free(*cache);
    *cache = NULL;
}

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
            return arc_init(param);
        break:
            return NULL;
    }
}
