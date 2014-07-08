#include <stdio.h>
#include <string.h>

#include "arc_buf.h"
#include "arc_hash.h"

/*see http://www.burtleburtle.net/bob/c/lookup3.c for more information*/

#define ROT(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

#define FINAL(a,b,c) \
{ \
  c ^= b; c -= ROT(b,14); \
  a ^= c; a -= ROT(c,11); \
  b ^= a; b -= ROT(a,25); \
  c ^= b; c -= ROT(b,16); \
  a ^= c; a -= ROT(c,4);  \
  b ^= a; b -= ROT(a,14); \
  c ^= b; c -= ROT(b,24); \
}

arc_hash_t *arc_hash_init(uint32_t size) 
{
    arc_hash_t *arc_hash_table;

    arc_hash_table = malloc(sizeof(arc_hash_t));
    ASSERT(arc_hash_table != NULL);

    arc_hash_table->ht_size = 1ULL << 12;

    while (arc_hash_table->ht_size < (uint64_t)size)
        arc_hash_table->ht_size <<= 1;
    
    arc_hash_table->ht_mask = arc_hash_table->ht_size - 1;

    while(1) {
        arc_hash_table->ht_table = malloc(arc_hash_table->ht_size * sizeof (void*));
        if (arc_hash_table->ht_table == NULL) {
            ASSERT(arc_hash_table->ht_size > (1ULL << 8));
            arc_hash_table->ht_size >>= 1;
            continue;
        } 
        memset(arc_hash_table->ht_table, 0, arc_hash_table->ht_size * sizeof (void*));
        break; 
    }
}

void arc_hash_fini(arc_hash_t **arc_hash_table) 
{
    free(*arc_hash_table->ht_table);
    free(*arc_hash_table);
    *arc_hash_table = NULL;
}


static uint64_t hash(uint32_t x, uint32_t y)
{
    uint32_t a,b,c;

    a = b = c = 0xdeadbeef + ((uint32_t)(2<<2)) ;

    b+=y; 
    a+=x;

    FINAL(a,b,c);

    return (uint64_t)(b<<32)+c;
}

arc_buf_hdr_t *arc_hash_find(arc_hash_t *arc_hash_table, uint32_t disk_id, uint32_t block_id)
{
    arc_buf_hdr_t *buf;
    uint64_t idx = hash(disk_id, block_id) & arc_hash_table->ht_mask;

    for (buf = arc_hash_table->ht_table[idx]; buf != NULL; buf = buf->b_hash_next) {
        if(buf->b_disk_id == disk_id && buf->b_block_id == block_id)
            return buf;
    }
    return NULL;
}

arc_buf_hdr_t *arc_hash_insert(arc_hash_t *arc_hash_table, arc_buf_hdr_t *buf)
{
    arc_buf_hdr_t *fbuf;
    uint64_t idx = hash(buf->b_disk_id, buf->b_block_id) & arc_hash_table->ht_mask;

    for (fbuf = arc_hash_table->ht_table[idx]; fbuf != NULL; fbuf = fbuf->b_hash_next) {
        if(fbuf->b_disk_id == buf->b_disk_id && fbuf->b_block_id == buf->b_block_id)
            return fbuf;
    }

    buf->b_hash_next = fbuf;
    arc_hash_table->ht_table[idx] = buf;
    return NULL;
}

void arc_hash_remove(arc_hash_t *arc_hash_table, arc_buf_hdr_t *buf)
{
    arc_buf_hdr_t *fbuf, **bufp;
    uint64_t idx = hash(buf->b_disk_id, buf->b_block_id) & arc_hash_table->ht_mask;

    bufp = &arc_hash_table->ht_table[idx];
    while ((fbuf = *bufp) != buf) { 
        if (fbuf == NULL)
            return;
        bufp = &fbuf->b_hash_next;
    }

    *bufp = buf->b_hash_next;
    buf->b_hash_next = NULL;
    return NULL;
}

