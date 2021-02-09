#ifndef HITS_H_INCLUDED
#define HITS_H_INCLUDED

#include "defines.h"
#include "value.h"
#include <stdint.h>

/*
 * Structure representing a hit (a found value) in the target process.
 */
struct hit {
    addr_t addr;
    enum value_type type;
    union value_data prev;
};

struct hits {
    struct hit *items;
    umax_t size, capacity;
    enum value_type addr_type;
    enum value_type value_type;
};

/*
 * (De)allocate a hits container.
 */
struct hits *hits_new();
void hits_delete(struct hits *hits);

/*
 * Add a new hit.
 */
int hits_add(struct hits *hits, addr_t addr, enum value_type type,
             union value_data *data);
#endif
