#include "search.h"

#include "ast.h"
#include "eval.h"
#include "hits.h"
#include "mem.h"
#include "opt.h"
#include "parse.h"
#include "symbol.h"
#include "value.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct hits *search(struct ramfuck *ctx, enum value_type type,
                    const char *expression)
{
    struct mem_io *mem;
    struct mem_region *mr, *regions, *new;
    size_t regions_size, regions_capacity;
    size_t region_size_max, region_idx, snprint_len_max;
    char *region_buf, *snprint_buf;
    struct symbol_table *symtab;
    struct parser parser;
    struct value value;
    unsigned int align;
    uintptr_t addr, end;
    enum value_type addr_type;
    union value_data **ppdata;
    struct ast *ast, *opt;
    struct hits *hits, *ret;

    ast = NULL;
    symtab = NULL;
    hits = ret = NULL;
    region_buf = snprint_buf = NULL;

    regions_size = 0;
    regions_capacity = 16;
    if (!(regions = calloc(regions_capacity, sizeof(struct mem_region)))) {
        errf("search: out-of-memory for address regions");
        return NULL;
    }

    mem = ctx->mem;
    addr_type = U32;
    region_size_max = snprint_len_max = 0;
    for (mr = mem->region_first(mem); mr; mr = mem->region_next(mr)) {
        if (addr_type == U32 && (mr->start + mr->size-1) > UINT32_MAX)
            addr_type = U64;
        if ((mr->prot & MEM_READ) && (mr->prot & MEM_WRITE)) {
            size_t len;
            if (regions_size == regions_capacity) {
                size_t new_size;
                regions_capacity *= 2;
                new_size = sizeof(struct mem_region) * regions_capacity;
                if (!(new = realloc(regions, new_size))) {
                    errf("search: out-of-memory for address regions");
                    goto fail;
                }
                regions = new;
            }

            mem_region_copy(&regions[regions_size++], mr);

            if (region_size_max < mr->size)
                region_size_max = mr->size;

            if (snprint_len_max < (len = mem_region_snprint(mr, NULL, 0)))
                snprint_len_max = len;
        }
    }
    if (!(new = realloc(regions, sizeof(struct mem_region) * regions_size))) {
        errf("search: error truncating allocated address regions");
        goto fail;
    }
    regions = new;

    if (!(region_buf = malloc(region_size_max))) {
        errf("search: out-of-memory for memory region buffer");
        goto fail;
    }

    if (!(snprint_buf = malloc(snprint_len_max + 1))) {
        errf("search: out-of-memory for memory region text representation");
        goto fail;
    }

    if ((symtab = symbol_table_new(ctx))) {
        size_t value_sym;
        value.type = type;
        symbol_table_add(symtab, "addr", addr_type, (void *)&addr);
        value_sym = symbol_table_add(symtab, "value", value.type, &value.data);
        ppdata = &symtab->symbols[value_sym]->pdata;
    } else {
        errf("search: error creating new symbol table");
        goto fail;
    }

    parser_init(&parser);
    parser.symtab = symtab;
    if (!(ast = parse_expression(&parser, expression))) {
        errf("search: %d parse errors", parser.errors);
        goto fail;
    }
    if ((opt = ast_optimize(ast))) {
        ast_delete(ast);
        ast = opt;
    }

    if (!(hits = hits_new())) {
        errf("search: error allocating hits container");
        goto fail;
    }

    if (!(align = value_sizeof(&value)))
        align = 1;

    for (region_idx = 0; region_idx < regions_size; region_idx++) {
        const struct mem_region *region = &regions[region_idx];
        if (!mem->read(mem, region->start, region_buf, region->size))
            continue;
        *ppdata = (union value_data *)region_buf;
        mem_region_snprint(region, snprint_buf, snprint_len_max + 1);
        printf("%s\n", snprint_buf);

        addr = region->start;
        end = addr + region->size;
        while (addr < end) {
            if (ast_evaluate(ast, &value)) {
                if (value_is_nonzero(&value)) {
                    printf("%p hit\n", (void *)addr);
                    hits_add(hits, addr, type, *ppdata);
                }
            }
            *ppdata = (union value_data *)((char *)*ppdata + align);
            addr += align;
        }
    }

    ret = hits;
    hits = NULL;

fail:
    if (ast) ast_delete(ast);
    if (symtab) symbol_table_delete(symtab);
    if (hits) hits_delete(hits);
    free(snprint_buf);
    free(region_buf);
    while (regions_size) mem_region_destroy(&regions[--regions_size]);
    free(regions);
    return ret;
}
