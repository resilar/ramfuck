#include "search.h"
#include "defines.h"

#include "ast.h"
#include "config.h"
#include "eval.h"
#include "hits.h"
#include "opt.h"
#include "parse.h"
#include "symbol.h"
#include "target.h"
#include "value.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct hits *search(struct ramfuck *ctx, enum value_type type,
                    const char *expression)
{
    struct target *target;
    struct region *mr, *regions, *new;
    size_t regions_size, regions_capacity;
    size_t region_size_max, region_idx, snprint_len_max;
    char *region_buf, *snprint_buf;
    struct parser parser;
    struct symbol_table *symtab;
    struct value value;
    unsigned int align;
    addr_t addr, end;
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
    if (!(regions = calloc(regions_capacity, sizeof(struct region)))) {
        errf("search: out-of-memory for address regions");
        return NULL;
    }

    addr_type = U32;
    region_size_max = snprint_len_max = 0;
    target = ctx->target;
    for (mr = target->region_first(target); mr; mr = target->region_next(mr)) {
#if ADDR_BITS == 64
        if (addr_type == U32 && (mr->start + mr->size-1) > UINT32_MAX)
            addr_type = U64;
#endif
        if ((mr->prot & ctx->config->search.prot) == ctx->config->search.prot) {
            size_t len;
            if (regions_size == regions_capacity) {
                size_t new_size;
                regions_capacity *= 2;
                new_size = sizeof(struct region) * regions_capacity;
                if (!(new = realloc(regions, new_size))) {
                    errf("search: out-of-memory for address regions");
                    goto fail;
                }
                regions = new;
            }

            region_copy(&regions[regions_size++], mr);

            if (region_size_max < mr->size)
                region_size_max = mr->size;

            if (snprint_len_max < (len = region_snprint(mr, NULL, 0)))
                snprint_len_max = len;
        }
    }
    if (!regions_size) {
        errf("search: no memory regions to scan");
        goto fail;
    }
    if (!(new = realloc(regions, sizeof(struct region) * regions_size))) {
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
#if ADDR_BITS == 64
        if (addr_type == U32) {
            uint32_t word = 0x43211234;
            uint32_t *data = (uint32_t *)&addr + (*(uint16_t *)&word == 0x4321);
            symbol_table_add(symtab, "addr", addr_type, (void *)data);
        } else {
            symbol_table_add(symtab, "addr", addr_type, (void *)&addr);
        }
#else
        symbol_table_add(symtab, "addr", addr_type, (void *)&addr);
#endif
        value.type = type;
        value_sym = symbol_table_add(symtab, "value", value.type, &value.data);
        ppdata = &symtab->symbols[value_sym]->pdata;
    } else {
        errf("search: error creating new symbol table");
        goto fail;
    }

    parser_init(&parser);
    parser.symtab = symtab;
    parser.addr_type = addr_type;
    parser.target = ctx->target;
    if (!(ast = parse_expression(&parser, expression))) {
        errf("search: %d parse errors", parser.errors);
        goto fail;
    }
    if ((opt = ast_optimize(ast))) {
        ast_delete(ast);
        ast = opt;
    }

    if ((hits = hits_new())) {
        hits->addr_type = addr_type;
        hits->value_type = value.type;
    } else {
        errf("search: error allocating hits container");
        goto fail;
    }

    if (!(align = ctx->config->search.align)) {
        if (!(align = value_sizeof(&value)))
            align = 1;
    }

    ramfuck_break(ctx);
    for (region_idx = 0; region_idx < regions_size; region_idx++) {
        const struct region *region = &regions[region_idx];
        if (!target->read(target, region->start, region_buf, region->size))
            continue;
        *ppdata = (union value_data *)region_buf;
        region_snprint(region, snprint_buf, snprint_len_max + 1);
        fprintf(stderr, "%s\n", snprint_buf);

        addr = region->start;
        end = addr + region->size;
        while (addr < end) {
            if (ast_evaluate(ast, &value) && value_is_nonzero(&value)) {
                if (!hits_add(hits, addr, type, *ppdata)) {
                    region_idx = regions_size;
                    break;
                }
            }
            *ppdata = (union value_data *)((char *)*ppdata + align);
            addr += align;
        }
    }
    ramfuck_continue(ctx);

    ret = hits;
    hits = NULL;

fail:
    if (ast) ast_delete(ast);
    if (symtab) symbol_table_delete(symtab);
    if (hits) hits_delete(hits);
    free(snprint_buf);
    free(region_buf);
    while (regions_size) region_destroy(&regions[--regions_size]);
    free(regions);
    return ret;
}

struct hits *filter(struct ramfuck *ctx, struct hits *hits,
                    const char *expression)
{
    struct target *target;
    struct symbol_table *symtab;
    struct parser parser;
    struct ast *ast, *opt;
    struct hits *filtered, *ret;
    struct value value, result;
    union value_data **ppdata;
    enum value_type addr_type, value_type;
    addr_t addr;
    size_t i;

    ast = NULL;
    symtab = NULL;
    filtered = NULL;

    ret = hits;
    addr_type = hits->addr_type;
    value_type = hits->value_type;
    if ((symtab = symbol_table_new(ctx))) {
        size_t prev_sym;
        symbol_table_add(symtab, "addr", addr_type, (void *)&addr);
        symbol_table_add(symtab, "value", value_type, &value.data);
        prev_sym = symbol_table_add(symtab, "prev", value_type, NULL);
        ppdata = &symtab->symbols[prev_sym]->pdata;
    } else {
        errf("filter: error creating new symbol table");
        goto fail;
    }

    parser_init(&parser);
    parser.symtab = symtab;
    parser.addr_type = addr_type;
    parser.target = ctx->target;

    if ((filtered = hits_new())) {
        filtered->addr_type = addr_type;
        filtered->value_type = value_type;
    } else {
        errf("filter: error allocating filtered hits container");
        goto fail;
    }
    if (!(ast = parse_expression(&parser, expression))) {
        errf("filter: %d parse errors", parser.errors);
        goto fail;
    }
    if ((opt = ast_optimize(ast))) {
        ast_delete(ast);
        ast = opt;
    }

    if (!ramfuck_break(ctx))
        goto fail;
    target = ctx->target;
    for (i = 0; i < hits->size; i++) {
        size_t size;
        addr = hits->items[i].addr;
        value.type = hits->items[i].type;
        size = value_type_sizeof((value.type & PTR) ? addr_type : value.type);
        if (!target->read(target, addr, &value.data, size))
            continue;

        *ppdata = &hits->items[i].prev;
        if (ast_evaluate(ast, &result) && value_is_nonzero(&result)) {
            if (!hits_add(filtered, addr, value_type, &value.data))
                break;
        }
    }
    ramfuck_continue(ctx);

    ret = filtered;
    filtered = NULL;

fail:
    if (filtered) hits_delete(filtered);
    if (ast) ast_delete(ast);
    if (symtab) symbol_table_delete(symtab);
    return ret;
}
