#include <stdio.h>
#include <sys/queue.h>

#include "arc_state.h"
#include "arc_buf.h"

int arc_state_change(arc_state_t *new_state, arc_buf_hdr_t *buf, int32_t delta)
{
    arc_state_t *old_state = buf->b_state;
    if (old_state) { 
        old_state->arcs_size -= (buf->b_buf_size - delta); 
        TAILQ_REMOVE(&old_state->arcs_list, buf, b_node);
        buf->b_state = NULL;
    }

    if (new_state) { 
        new_state->arcs_size += buf->b_buf_size; 
        TAILQ_INSERT_HEAD(&new_state->arcs_list, buf, b_node);
        buf->b_state = new_state;
    }

    return;
} 
