#include "search.h"

#include "ast.h"
#include "eval.h"
#include "mem.h"
#include "opt.h"
#include "parse.h"
#include "symbol.h"
#include "value.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct range {
    uintptr_t start;
    size_t size;
};

void search(struct ramfuck *ctx, enum value_type type, const char *expression)
{
    struct mem_io *mem;
    struct mem_region *mr;
    struct range *ranges, *new;
    size_t ranges_size, ranges_capacity;
    size_t size_max, range_idx;
    struct symbol_table *symtab;
    char *buf;
    struct value value;
    unsigned int align;
    uintptr_t addr, end;
    enum value_type addr_type;
    size_t value_sym;
    union value_data **ppdata;
    struct ast *ast, *opt;
    int errors;

    ranges_size = 0;
    if (!(ranges = calloc((ranges_capacity = 16), sizeof(struct range)))) {
        errf("search: out-of-memory for address ranges");
        return;
    }

    size_max = 0;
    mem = ctx->mem;
    for (mr = mem->region_first(mem); mr; mr = mem->region_next(mr)) {
        if ((mr->prot & MEM_READ) && (mr->prot & MEM_WRITE)) {
            if (ranges_size == ranges_capacity) {
                ranges_capacity *= 2;
                new = realloc(ranges, sizeof(struct range) * ranges_capacity);
                if (!new) {
                    errf("search: out-of-memory for address ranges");
                    free(ranges);
                    return;
                }
                ranges = new;
            }
            if (size_max < mr->size)
                size_max = mr->size;
            ranges[ranges_size].start = mr->start;
            ranges[ranges_size].size = mr->size;
            ranges_size++;
        }
    }
    if (!(new = realloc(ranges, sizeof(struct range) * ranges_size))) {
        errf("search: error truncating allocated address ranges");
        free(ranges);
        return;
    } else if (!(buf = malloc(size_max))) {
        errf("search: out-of-memory for memory region buffer");
        free(ranges);
        return;
    } else if (!(symtab = symbol_table_new(ctx))) {
        errf("search: error creating new symbol table");
        free(ranges);
        free(buf);
        return;
    }

    value.type = type;
    addr_type = (sizeof(uintptr_t) == 8) ? U64 : U32;
    symbol_table_add(symtab, "addr", addr_type, (void *)&addr);
    value_sym = symbol_table_add(symtab, "value", value.type, &value.data);
    ppdata = &symtab->symbols[value_sym]->pdata;
    if ((errors = parse_expression(expression, symtab, 0, &ast))) {
        errf("search: %d parse errors", errors);
        symbol_table_delete(symtab);
        free(ranges);
        free(buf);
        return;
    }
    if ((opt = ast_optimize(ast))) {
        ast_delete(ast);
        ast = opt;
    }

    if (!(align = value_sizeof(&value)))
        align = 1;
    for (range_idx = 0; range_idx < ranges_size; range_idx++) {
        addr = ranges[range_idx].start;
        end = addr + ranges[range_idx].size;
        if (!mem->read(mem, addr, buf, (size_t)(end - addr)))
            continue;
        *ppdata = (union value_data *)buf;

        printf("%p-%p\n", (void *)addr, (void *)end);
        while (addr < end) {
            if (ast_evaluate(ast, &value)) {
                if (value_is_nonzero(&value)) {
                    printf("%p hit\n", (void *)addr);
                }
            }

            *ppdata = (union value_data *)((char *)*ppdata + align);
            addr += align;
        }
    }

    ast_delete(ast);
    symbol_table_delete(symtab);
    free(ranges);
    free(buf);
}
