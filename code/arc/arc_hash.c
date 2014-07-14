#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
    if(arc_hash_table == NULL)
        return NULL;

    arc_hash_table->ht_size = 1ULL << 12;

    while (arc_hash_table->ht_size < (uint64_t)size)
        arc_hash_table->ht_size <<= 1;
    
    arc_hash_table->ht_mask = arc_hash_table->ht_size - 1;

    while(1) {
        arc_hash_table->ht_table = malloc(arc_hash_table->ht_size * sizeof (void*));
        if (arc_hash_table->ht_table == NULL) {
            assert(arc_hash_table->ht_size > (1ULL << 8));
            arc_hash_table->ht_size >>= 1;
            continue;
        } 
        memset(arc_hash_table->ht_table, 0, arc_hash_table->ht_size * sizeof (void*));
        break; 
    }
}

void arc_hash_fini(arc_hash_t **arc_hash_table) 
{
    free((*arc_hash_table)->ht_table);
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

    return (uint64_t)((uint64_t)b<<32)+c;
}

arc_buf_hdr_t *arc_hash_find(arc_hash_t *arc_hash_table, uint32_t disk, uint32_t block)
{
    arc_buf_hdr_t *buf;
    uint64_t idx = hash(disk, block) & arc_hash_table->ht_mask;

    for (buf = arc_hash_table->ht_table[idx]; buf != NULL; buf = buf->b_hash_next) {
        if(buf->b_disk == disk && buf->b_block == block)
            return buf;
    }
    return NULL;
}

arc_buf_hdr_t *arc_hash_insert(arc_hash_t *arc_hash_table, arc_buf_hdr_t *buf)
{
    arc_buf_hdr_t *fbuf;
    uint64_t idx = hash(buf->b_disk, buf->b_block) & arc_hash_table->ht_mask;

    for (fbuf = arc_hash_table->ht_table[idx]; fbuf != NULL; fbuf = fbuf->b_hash_next) {
        if(fbuf->b_disk == buf->b_disk && fbuf->b_block == buf->b_block)
            return fbuf;
    }

    buf->b_hash_next = fbuf;
    arc_hash_table->ht_table[idx] = buf;
    return NULL;
}

void arc_hash_remove(arc_hash_t *arc_hash_table, arc_buf_hdr_t *buf)
{
    arc_buf_hdr_t *fbuf, **bufp;
    uint64_t idx = hash(buf->b_disk, buf->b_block) & arc_hash_table->ht_mask;

    bufp = &arc_hash_table->ht_table[idx];
    while ((fbuf = *bufp) != buf) { 
        if (fbuf == NULL)
            return;
        bufp = &fbuf->b_hash_next;
    }

    *bufp = buf->b_hash_next;
    buf->b_hash_next = NULL;
}

