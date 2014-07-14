#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>

#include "cache.h"
#include "arc.h"
#include "arc_buf.h"
#include "arc_state.h"
#include "arc_hash.h"

typedef struct arc_cache {
    cache_t             cache;

    arc_hash_t          *arc_hash;

    arc_state_t         arc_mru;
    arc_state_t         arc_mru_ghost;
    arc_state_t         arc_mfu;
    arc_state_t         arc_mfu_ghost;

    uint64_t            arc_size_max;
    uint64_t            arc_p;
    uint64_t            arc_c;
} arc_cache_t;

#define MIN(a, b)                   ((a) < (b) ? (a): (b))                   
#define MAX(a, b)                   ((a) > (b) ? (a): (b))                   

#define ARC_P_MIN_SHIFT             4
#define ARC_MINTIME                 62
#define ARC_CACHE_LEN_MAX           (1024 *1024)

#define ARC_IN_HASH_TABLE           (1 << 1)
#define ARC_BUF_IN_USE              (1 << 2)

#define HDR_IN_HASH_TABLE(hdr)      ((hdr)->b_flags & ARC_IN_HASH_TABLE)
#define HDR_BUF_IN_USE(hdr)         ((hdr)->b_flags & ARC_BUF_IN_USE)

static int32_t arc_evict(arc_hash_t *arc_hash, arc_state_t *to, arc_state_t *from, int32_t bytes)
{
    arc_buf_hdr_t        *buf, *fbuf;
    
    buf = TAILQ_LAST(&from->arcs_list, arc_state_list);
    while(1) {
        fbuf = TAILQ_PREV(buf, arc_state_list, b_node); 

        if (!HDR_BUF_IN_USE(buf)) {
            TAILQ_REMOVE(&from->arcs_list, buf, b_node);

            bytes -= buf->b_buf_size;
            from->arcs_size -= buf->b_buf_size;

            if (to == NULL) { 
                if (buf->b_arc_buf->b_buf.data_evict) {
                    buf->b_arc_buf->b_buf.data_evict(buf->b_arc_buf->b_buf.b_data, buf->b_arc_buf->b_buf.b_data_len);
                }
                arc_hash_remove(arc_hash, buf);
                free(buf->b_arc_buf->b_buf.b_data); 
                free(buf->b_arc_buf); 
                free(buf); 
            } else {
                buf->b_state = to;
                to->arcs_size += buf->b_buf_size;
                TAILQ_INSERT_HEAD(&to->arcs_list, buf, b_node);
            } 
        }

        if (bytes<= 0)
            return 0;

        if (fbuf == NULL)
            break;

        buf = fbuf; 
    }

    return bytes;  
}

static void arc_adapt(arc_cache_t *arc_cache, arc_buf_hdr_t *buf, int32_t bytes)
{
    uint32_t mult;
    uint64_t arc_p_min = arc_cache->arc_c >> ARC_P_MIN_SHIFT;

    if (buf->b_state == &arc_cache->arc_mru_ghost) {
        mult = ((arc_cache->arc_mru_ghost.arcs_size >= arc_cache->arc_mfu_ghost.arcs_size) ?
                        1 : (arc_cache->arc_mfu_ghost.arcs_size/arc_cache->arc_mru_ghost.arcs_size));
        mult = MIN(mult, 10); 

        arc_cache->arc_p = MIN(arc_cache->arc_c - arc_p_min, arc_cache->arc_p + bytes * mult);
    }
    
    if (buf->b_state == &arc_cache->arc_mfu_ghost) {
        uint64_t delta;

        mult = ((arc_cache->arc_mfu_ghost.arcs_size >= arc_cache->arc_mru_ghost.arcs_size) ?
                1 : (arc_cache->arc_mru_ghost.arcs_size/arc_cache->arc_mfu_ghost.arcs_size));
        mult = MIN(mult, 10);

        delta = MIN(bytes * mult, arc_cache->arc_p);
        arc_cache->arc_p = MAX(arc_p_min, arc_cache->arc_p - delta);
    } 

}

static uint64_t now_usec()
{
    struct timeval      t;

    gettimeofday(&t, NULL);

    return t.tv_sec * 1000 + t.tv_usec;
}

static void arc_access(arc_cache_t *arc_cache, arc_buf_hdr_t *buf, int32_t delta)
{
    uint64_t            arc_c = arc_cache->arc_c;
    uint64_t            arc_p = arc_cache->arc_p;
    arc_state_t         *mru = &arc_cache->arc_mru;
    arc_state_t         *mru_ghost = &arc_cache->arc_mru_ghost;
    arc_state_t         *mfu = &arc_cache->arc_mfu;
    arc_state_t         *mfu_ghost = &arc_cache->arc_mfu_ghost;

#define L1_SIZE         (mru->arcs_size + mru_ghost->arcs_size)
#define L2_SIZE         (mfu->arcs_size + mfu_ghost->arcs_size)

    if (buf->b_state == NULL) {
        if (L1_SIZE + delta > arc_c) {
            if (mru_ghost->arcs_size > delta) { 
                arc_evict(arc_cache->arc_hash, NULL, mru_ghost, delta);
                arc_evict(arc_cache->arc_hash, mru_ghost, mru, delta); 
                if (mru->arcs_size + delta > arc_p)   
                    arc_evict(arc_cache->arc_hash, mru_ghost, mru, delta);
                else
                    if(mfu->arcs_size + delta > arc_c - arc_p)
                        arc_evict(arc_cache->arc_hash, mfu_ghost, mfu, delta);
            } else {
                arc_evict(arc_cache->arc_hash, NULL, mru, delta);
            }
        } else {
            if (L1_SIZE + L2_SIZE >= arc_c) { 
                if (L1_SIZE + L2_SIZE >= arc_c*2) { 
                    arc_evict(arc_cache->arc_hash, NULL, mfu_ghost, delta); 
                if (mru->arcs_size + delta > arc_p)   
                    arc_evict(arc_cache->arc_hash, mru_ghost, mru, delta);
                else
                    if(mfu->arcs_size + delta > arc_c - arc_p)
                        arc_evict(arc_cache->arc_hash, mfu_ghost, mfu, delta);
                }
            } 
        }
        buf->b_access = now_usec();
        arc_state_change(mru, buf, delta);
    } else if (buf->b_state == mru || buf->b_state == mfu) {
        buf->b_access = now_usec();
        if (buf->b_access + ARC_MINTIME > now_usec()) 
            arc_state_change(mfu, buf, delta);
        else 
            if(mfu->arcs_size + delta > arc_c - arc_p)
                arc_evict(arc_cache->arc_hash, mfu_ghost, mfu, delta);
    } else if (buf->b_state == mru_ghost) {
        buf->b_access = now_usec();
        arc_adapt(arc_cache, buf, delta);
        if (mru->arcs_size + delta > arc_p)   
            arc_evict(arc_cache->arc_hash, mru_ghost, mru, delta);
        else
            if(mfu->arcs_size + delta > arc_c - arc_p)
                arc_evict(arc_cache->arc_hash, mfu_ghost, mfu, delta);
        arc_state_change(mru, buf, delta);
    } else if (buf->b_state == mfu_ghost) {
        buf->b_access = now_usec();
        arc_adapt(arc_cache, buf, delta);
        if (mru->arcs_size + delta > arc_p)   
            arc_evict(arc_cache->arc_hash, mru_ghost, mru, delta);
        else
            if(mfu->arcs_size + delta > arc_c - arc_p)
                arc_evict(arc_cache->arc_hash, mfu_ghost, mfu, delta);
        arc_state_change(mfu, buf, delta);
    }

#undef L1_SIZE
#undef L2_SIZE
}


cache_buf_t *arc_cache_get(cache_t *cache, void *key, uint32_t key_len, uint32_t cache_len)
{
    arc_cache_t         *arc_cache;
    arc_buf_hdr_t       *buf;
    uint32_t            disk, block; 
        
    if (cache == NULL || key == NULL || key_len != sizeof(uint32_t)<<1)
        return NULL;

    if (cache_len > ARC_CACHE_LEN_MAX) 
        return NULL;

    disk = *(uint32_t *)key; 
    block = *(uint32_t *)(key + sizeof(uint32_t)); 
    arc_cache = (arc_cache_t *)cache;
    
    buf = arc_hash_find(arc_cache->arc_hash, disk, block);
    if (buf == NULL) {
        buf = malloc(sizeof(arc_buf_hdr_t)); 
        if (buf == NULL)
            return NULL;
        buf->b_disk = disk;
        buf->b_block = block;
        buf->b_flags = 0;
        buf->b_flags |= ARC_BUF_IN_USE; 
        buf->b_refcnt++;
        buf->b_buf_size = cache_len; 
        buf->b_state = NULL; 
        buf->b_hash_next = NULL;
        buf->b_arc_buf = malloc(sizeof(arc_buf_t)); 
        if (buf->b_arc_buf == NULL) {
            free(buf);
            return NULL;
        }
        buf->b_arc_buf->b_buf.b_data = malloc(sizeof(char) * cache_len); 
        if (buf->b_arc_buf->b_buf.b_data == NULL) {
            free(buf->b_arc_buf);
            free(buf);
            return NULL;
        }
        buf->b_arc_buf->b_buf.b_data_len = 0; 
        buf->b_arc_buf->b_buf.data_evict = NULL; 
        buf->b_arc_buf->b_hdr = buf; 
        assert(arc_hash_insert(arc_cache->arc_hash, buf) == NULL);
        buf->b_flags |= ARC_IN_HASH_TABLE;    
        arc_access(arc_cache, buf, cache_len);
        return (cache_buf_t *)buf->b_arc_buf;         
    } else {
            int32_t delta = 0;
            buf->b_flags |= ARC_BUF_IN_USE;
            buf->b_refcnt++;
            if (buf->b_buf_size < cache_len) {
                buf->b_arc_buf->b_buf.b_data = realloc(buf->b_arc_buf->b_buf.b_data, sizeof(char) * cache_len);
                if (buf->b_arc_buf->b_buf.b_data == NULL) 
                    buf->b_buf_size = 0;
                buf->b_buf_size = cache_len;
                delta = cache_len - buf->b_buf_size;
            }
            arc_access(arc_cache, buf, delta);
            return (cache_buf_t *)buf->b_arc_buf; 
    }
}

void arc_cache_put(cache_t *cache, cache_buf_t *cache_buf)
{
    arc_cache_t         *arc_cache;
    arc_buf_t           *arc_buf;
    arc_buf_hdr_t       *buf;

    if (cache == NULL || cache_buf == NULL)
      return ;  

    arc_cache = (arc_cache_t *) cache; 
    arc_buf = (arc_buf_t *) cache_buf;
    buf = arc_buf->b_hdr;
   
    if (--buf->b_refcnt == 0)
       buf->b_flags &= ~ARC_BUF_IN_USE; 
}

static void do_free_list(struct arc_state_list *l)
{
    arc_buf_hdr_t       *buf;
    while(1) {
        buf = TAILQ_FIRST(l);
        if (buf == NULL)
            return;
        free(buf->b_arc_buf->b_buf.b_data); 
        free(buf->b_arc_buf); 
        free(buf); 
    }
}

static void arc_cache_destroy(cache_t **cache)
{
    if(cache == NULL || *cache == NULL)
       return; 
    arc_cache_t         *arc_cache = (arc_cache_t *)*cache;

    do_free_list(&arc_cache->arc_mru.arcs_list);
    do_free_list(&arc_cache->arc_mru_ghost.arcs_list);
    do_free_list(&arc_cache->arc_mfu.arcs_list);
    do_free_list(&arc_cache->arc_mfu_ghost.arcs_list);

    arc_hash_fini(arc_cache->arc_hash);
    free(arc_cache);
    *cache = NULL; 
}

static cache_ops_t arc_ops = {
    .cache_get          = arc_cache_get,
    .cache_put          = arc_cache_put,
    .cache_destroy      = arc_cache_destroy,
};

cache_t *arc_cache_init(cache_param_t *param)
{
    arc_cache_t         *arc_cache = NULL; 
    arc_hash_t          *arc_hash = NULL; 
    uint64_t            arc_size = 1ULL;

    if(param == NULL || param->cache_size < 1024*1024*512)
        return NULL;
    while (arc_size < (uint64_t)param->cache_size)
        arc_size <<= 1;

    while(1) {
        arc_cache = malloc(sizeof(arc_cache_t));
        if (arc_cache == NULL)
            break;
        arc_hash = arc_hash_init(param->cache_size>>1);
        if (arc_hash == NULL) 
            break;
        arc_cache->cache.ops = &arc_ops;
        arc_cache->arc_hash = arc_hash;
        arc_cache->arc_size_max = arc_size;
        arc_cache->arc_c = arc_size>>1;
        arc_cache->arc_mru.arcs_size = 0;
        arc_cache->arc_mru_ghost.arcs_size = 0;
        arc_cache->arc_mfu.arcs_size = 0;
        arc_cache->arc_mfu_ghost.arcs_size = 0;
        TAILQ_INIT(&arc_cache->arc_mru.arcs_list);   
        TAILQ_INIT(&arc_cache->arc_mru_ghost.arcs_list);   
        TAILQ_INIT(&arc_cache->arc_mfu.arcs_list);   
        TAILQ_INIT(&arc_cache->arc_mfu_ghost.arcs_list);   
        return (cache_t *)arc_cache; 
    }
    free(arc_cache);
    free(arc_hash);
    return NULL;
}

