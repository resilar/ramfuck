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
    enum value_type type;
    union value_data *pdata;
};
#define symbol_name(sym) ((const char *)(sym) + sizeof(struct symbol))

struct symbol_table {
    struct ramfuck *ctx;
    size_t size, capacity;
    struct symbol **symbols;
};

/*
 * Allocate and fill a new symbol structure.
 */
struct symbol *symbol_new(const char *name,
                          enum value_type type, union value_data *pdata);

/*
 * Delete a symbol.
 */
void symbol_delete(struct symbol *sym);

/*
 * (De)allocate a symbol table.
 */
struct symbol_table *symbol_table_new(struct ramfuck *ctx);
void symbol_table_delete(struct symbol_table *symtab);

/*
 * Add symbol to a symbol table.
 */
size_t symbol_table_add(struct symbol_table *symtab, const char *name,
                        enum value_type type, union value_data *pdata);
size_t symbol_table_add_value(struct symbol_table *symtab, const char *name,
                              struct value *value);

/*
 * Symbol table lookup by name of length `len`.
 */
size_t symbol_table_lookup(struct symbol_table *symtab,
                           const char *name, size_t len);

#endif
