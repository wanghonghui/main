#ifndef _ARC_STATE_H
#define _ARC_STATE_H

#include <sys/queue.h>

#include "arc_buf.h"

TAILQ_HEAD(arc_state_list, arc_buf_hdr);

typedef struct arc_state {
    uint64_t                arcs_size;
    struct arc_state_list   arcs_list;
} arc_state_t; 

int arc_state_change(arc_state_t *new_state, struct arc_buf_hdr *buf, int32_t delta); 
#endif
