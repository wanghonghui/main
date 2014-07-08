#include "cache.h"
#include "arc.h"

typedef struct arc_state_t {



};

typedef struct arc_cache_t {
    cache_t         arc_cache;

    arc_state_t     arc_mru;
    arc_state_t     arc_mru_ghost;
    arc_state_t     arc_mfu;
    arc_state_t     arc_mfu_ghost;

    uint64_t        arc_p;
    uint64_t        arc_c;
    
};
