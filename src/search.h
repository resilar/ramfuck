#ifndef SEARCH_H_INCLUDED
#define SEARCH_H_INCLUDED

#include "ramfuck.h"
#include "mem.h"
#include "value.h"

/*
 * Search a value of type 'type' from a process specified by 'pid'.
 * Returns a hits structure representing the hits.
 */
void search(struct ramfuck *ctx, enum value_type type, const char *expression);

#endif
