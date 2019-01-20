/*
 * Symbol table for the expression evaluator.
 *
 * Currently symbol table is implemented as a dynamic array of symbols. RB-tree
 * would be nicer, but in ramfuck we have only few symbols so using fancy data
 * structures is not worth it.
 */

#ifndef SYMBOL_H_INCLUDED
#define SYMBOL_H_INCLUDED

#include "ramfuck.h"
#include "value.h"

#include <stddef.h>


struct symbol {
    char *name;
    struct value value;
};

struct symbol_table {
    struct ramfuck *ctx;
    size_t size, allocated;
    struct symbol **symbols;
};

/*
 * Allocate and fill a new symbol structure.
 *
 * Caller can set parameter 'value' to NULL and fill symbol->value later.
 * symbol->value receives a copy of value if its not NULL.
 */
struct symbol *symbol_new(char *name, struct value *value);
void symbol_delete(struct symbol *sym);

/*
 * Allocate a symbol table.
 */
struct symbol_table *symbol_table_new(struct ramfuck *ctx);
void symbol_table_delete(struct symbol_table *symtab);

/*
 * Symbol table lookup by ('\0'-terminated) name.
 */
struct symbol *symbol_table_lookup(struct symbol_table *symtab,
                                   const char *name);

/*
 * Symbol table lookup by name of length `len`.
 */
struct symbol *symbol_table_nlookup(struct symbol_table *symtab,
                                    const char *name, size_t len);

/*
 * Insert a symbol to a symbol table.
 *
 * Returns 1 if the symbol was inserted successfully. On error 0 is returned.
 * Function fails if a symbol with the same name exists in the table.
 */
int symbol_table_insert(struct symbol_table *symtab, struct symbol *sym);

/* yeah, there's no remove (not needed in ramfuck). */

#endif
