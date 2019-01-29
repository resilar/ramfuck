#include "hits.h"
#include "ramfuck.h"

#include <memory.h>
#include <stdlib.h>

struct hits *hits_new()
{
    struct hits *hits;
    if ((hits = malloc(sizeof(struct hits)))) {
        hits->size = 0;
        hits->capacity = 256;
        hits->addr_type = U32;
        hits->value_type = S32;
        if (!(hits->items = malloc(sizeof(struct hit) * hits->capacity))) {
            free(hits);
            hits = NULL;
        }
    }
    return hits;
}

void hits_delete(struct hits *hits)
{
    free(hits->items);
    free(hits);
}

int hits_add(struct hits *hits, addr_t addr, enum value_type type,
             union value_data *data)
{
    struct hit *hit;
    size_t size = value_type_sizeof((type & PTR) ? hits->addr_type : type);

    if (hits->size == hits->capacity) {
        struct hit *new;
        new = realloc(hits->items, sizeof(struct hit) * 2*hits->capacity);
        if (!new) {
            errf("hits: out-of-memory for larger hits container");
            return 0;
        }
        hits->capacity *= 2;
        hits->items = new;
    }

    hit = &hits->items[hits->size++];
    hit->addr = addr;
    hit->type = type;
    memcpy(&hit->prev, data, size);

    return 1;
}
