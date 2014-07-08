#ifndef _ARC_HASH_H
#define _ARC_HASH_H

typedef struct arc_hash {
    uint64_t ht_size;
    uint64_t ht_mask;
    arc_buf_hdr_t **ht_table; 
} arc_hash_t;

arc_hash_t *arc_hash_init(uint32_t size);
void arc_hash_fnit(arc_hash_t **arc_hash_table);

arc_buf_hdr_t *arc_hash_find(arc_hash_t *arc_hash_table, uint32_t disk_id, uint32_t block_id);
arc_buf_hdr_t *arc_hash_insert(arc_hash_t *arc_hash_table, arc_buf_hdr_t *buf);
void arc_hash_remove(arc_hash_t *arc_hash_table, arc_buf_hdr_t *buf);

#endif
