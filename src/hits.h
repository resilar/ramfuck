#ifndef HITS_H_INCLUDED
#define HITS_H_INCLUDED

#include "value.h"

#include <stdint.h>

/*
 * Structure representing a hit (a found value) in the target process.
 */
struct hit {
    uintptr_t addr;
    enum value_type type;
    union value_data prev;
};

struct hits {
    struct hit *items;
    size_t size, capacity;
};

/*
 * (De)allocate a hits container.
 */
struct hits *hits_new();
void hits_delete(struct hits *hits);

int hits_add(struct hits *hits, uintptr_t addr, enum value_type type,
             union value_data *data);

#endif
