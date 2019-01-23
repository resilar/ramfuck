#include "symbol.h"

#include <stdlib.h>
#include <string.h>

struct symbol *symbol_new(const char *name,
                          enum value_type type, union value_data *pdata)
{
    struct symbol *sym;
    if ((sym = malloc(sizeof(struct symbol) + strlen(name) + 1))) {
        strcpy((char *)sym + sizeof(struct symbol), name);
        sym->type = type;
        sym->pdata = pdata;
    }
    return sym;
}

void symbol_delete(struct symbol *sym)
{
    if (sym) {
        free(sym);
    }
}

struct symbol_table *symbol_table_new(struct ramfuck *ctx)
{
    struct symbol_table *symtab;

    if ((symtab = malloc(sizeof(struct symbol_table)))) {
        struct symbol **symbols;
        symtab->ctx = ctx;
        symtab->size = 0;
        symtab->capacity = 16;
        if ((symbols = malloc(symtab->capacity * sizeof(struct symbol *)))) {
            symtab->symbols = &symbols[-1]; /* indexing from 1 */
        } else {
            symbol_table_delete(symtab);
            symtab = NULL;
        }
    }

    return symtab;
}

void symbol_table_delete(struct symbol_table *symtab)
{
    size_t i;
    for (i = 1; i <= symtab->size; i++)
        symbol_delete(symtab->symbols[i]);
    free(&symtab->symbols[1]);
    free(symtab);
}

size_t symbol_table_add(struct symbol_table *symtab, const char *name,
                        enum value_type type, union value_data *data)
{
    struct symbol *sym;
    if (symbol_table_lookup(symtab, name, strlen(name))) {
        errf("symbol: symtab already contains '%s'", name);
        return 0;
    }

    if (!(sym = symbol_new(name, type, data)))
        return 0;

    if (symtab->size >= symtab->capacity) {
        struct symbol **new;
        new = realloc(&symtab->symbols[1],
                      symtab->capacity * 2 * sizeof(struct symbol *));
        if (!new) return 0;
        symtab->symbols = &new[-1];
        symtab->capacity *= 2;
    }

    symtab->symbols[++symtab->size] = sym;
    return symtab->size;
}

size_t symbol_table_lookup(struct symbol_table *symtab,
                           const char *name, size_t len)
{
    size_t i, j;
    for (i = 1; i <= symtab->size; i++) {
        for (j = 0; j < len; j++) {
            if (symbol_name(symtab->symbols[i])[j] != name[j])
                break;
        }
        if (j == len && !symbol_name(symtab->symbols[i])[j])
            return i;
    }
    return 0;
}
