#include <stdio.h>
#include "cache.h"


int main(int argc, char **argv) 
{
    cache_t                *cache; 
    cache_param_t          param;
    
    param.cache_size = 10000;
    
    cache = cache_factory(ARC_CACHE, &param);
    
      



}
