#ifndef _ARC_BUF_H
#define _ARC_BUF_H

#include <sys/queue.h>

#include "cache.h"
#include "arc_state.h"

typedef struct arc_buf_hdr arc_buf_hdr_t;
typedef struct arc_state arc_state_t;

typedef struct arc_buf {
    cache_buf_t         b_buf;
    arc_buf_hdr_t       *b_hdr;            
} arc_buf_t;

struct arc_buf_hdr {
    uint32_t        b_disk;
    uint32_t        b_block;

    uint32_t        b_flags;
    uint32_t        b_refcnt;
    uint64_t        b_access;
    uint32_t        b_buf_size;

    arc_state_t     *b_state; 
    arc_buf_t       *b_arc_buf;
    arc_buf_hdr_t   *b_hash_next;

    TAILQ_ENTRY(arc_buf_hdr) b_node; 
};

#endif
