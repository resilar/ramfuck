#include "symbol.h"

#include <stdlib.h>
#include <string.h>

struct symbol *symbol_new(char *name, struct value *value)
{
    struct symbol *sym;
    if ((sym = malloc(sizeof(struct symbol)))) {
        if ((sym->name = malloc(strlen(name) + 1))) {
            strcpy(sym->name, name);
            value_copy(&sym->value, value);
        } else {
            free(sym);
            sym = NULL;
        }
    }
    return sym;
}

void symbol_delete(struct symbol *sym)
{
    if (sym) {
        if (sym->name) free(sym->name);
        free(sym);
    }
}

struct symbol_table *symbol_table_new()
{
    struct symbol_table *symtab;

    if ((symtab = malloc(sizeof(struct symbol_table)))) {
        symtab->size = 0;
        symtab->allocated = 16;
        symtab->symbols = malloc(symtab->allocated * sizeof(struct symbol *));
        if (!symtab->symbols) {
            symbol_table_delete(symtab);
            symtab = NULL;
        }
    }

    return symtab;
}

void symbol_table_delete(struct symbol_table *symtab)
{
    if (symtab->symbols) {
        int i;
        for (i = 0; i < symtab->size; i++)
            symbol_delete(symtab->symbols[i]);
        free(symtab->symbols);
    }
    free(symtab);
}

struct symbol *symbol_table_lookup(struct symbol_table *symtab,
                                   const char *name)
{
    int i;
    for (i = 0; i < symtab->size; i++) {
        if (!strcmp(symtab->symbols[i]->name, name))
            return symtab->symbols[i];
    }
    return NULL;
}

struct symbol *symbol_table_nlookup(struct symbol_table *symtab,
                                    const char *name, size_t len)
{
    int i, j;
    for (i = 0; i < symtab->size; i++) {
        for (j = 0; j < len; j++) {
            if (symtab->symbols[i]->name[j] != name[j])
                break;
        }
        if (j == len && !symtab->symbols[i]->name[j])
            return symtab->symbols[i];
    }
    return NULL;
}

int symbol_table_insert(struct symbol_table *symtab, struct symbol *sym)
{
    if (symbol_table_lookup(symtab, sym->name) != NULL)
        return 0;

    if (symtab->size >= symtab->allocated) {
        struct symbol **new;
        new = realloc(symtab->symbols,
                symtab->allocated * 2 * sizeof(struct symbol *));
        if (!new) return 0;
        symtab->symbols = new;
        symtab->allocated *= 2;
    }

    symtab->symbols[symtab->size++] = sym;
    return 1;
}
