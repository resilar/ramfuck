#include "config.h"
#include "ramfuck.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int accept(const char **pin, const char *what)
{
    size_t i;
    for (i = 0; what[i] && what[i] == (*pin)[i]; i++);
    if (!what[i] && (!(*pin)[i] || (*pin)[i] == '=' || isspace((*pin)[i]))) {
        for (*pin += i; isspace(**pin); (*pin)++);
        i = **pin == '=';
        for (*pin += i; isspace(**pin); (*pin)++);
        return 1;
    }
    return 0;
}

static int eol(const char *p)
{
    return *p == '\0';
}

struct config *config_new()
{
    struct config *cfg;
    if ((cfg = malloc(sizeof(struct config)))) {
        cfg->block.size = 256;
        cfg->cli.base = 10;
        cfg->cli.quiet = 0;
        cfg->search.align = 0;
        cfg->search.prot = 6; /* MEM_READ | MEM_WRITE */
        cfg->search.progress = 1;
    }
    return cfg;
}

void config_delete(struct config *cfg)
{
    free(cfg);
}

int config_process_line(struct config *cfg, const char *in)
{
    if (eol(in)) {
        const int quiet = cfg->cli.quiet;
        if (quiet)
            cfg->cli.quiet = 0;
        config_process_line(cfg, "block.size");
        config_process_line(cfg, "cli.base");
        fprintf(stdout, "cli.quiet = %d\n", quiet);
        config_process_line(cfg, "search.align");
        config_process_line(cfg, "search.prot");
        config_process_line(cfg, "search.progress");
        if (quiet)
            cfg->cli.quiet = 1;
        return 1;
    }

    if (accept(&in, "block.size")) {
        if (!eol(in)) {
            char *end;
            long value = strtol(in, &end, 0);
            while (isspace(*end)) end++;
            if (*end || value <= 0) {
                errf("config: bad block.size value");
                return 0;
            }
            cfg->block.size = value;
            if (cfg->cli.quiet)
                return 1;
        }
        if (!cfg->cli.quiet)
            fputs("block.size = ", stdout);
        fprintf(stdout, "%lu", cfg->block.size);
    } else if (accept(&in, "cli.base")) {
        if (!eol(in)) {
            char *end;
            unsigned long value = strtoul(in, &end, 10);
            while (isspace(*end)) end++;
            if (*end || (value != 10 && value != 16)) {
                errf("config: bad cli.base value");
                return 0;
            }
            cfg->cli.base = value;
            if (cfg->cli.quiet)
                return 1;
        }
        if (!cfg->cli.quiet)
            fputs("cli.base = ", stdout);
        fprintf(stdout, "%u", cfg->cli.base);
    } else if (accept(&in, "cli.quiet")) {
        if (!eol(in)) {
            int quiet = accept(&in, "1");
            if (!quiet && accept(&in, "0") && eol(in)) {
                cfg->cli.quiet = 0;
            } else if (quiet && eol(in)) {
                cfg->cli.quiet = 1;
            } else {
                errf("config: bad cli.quiet value (expected 0 or 1)");
                return 0;
            }
            if (cfg->cli.quiet)
                return 1;
        }
        if (!cfg->cli.quiet)
            fputs("cli.quiet = ", stdout);
        fprintf(stdout, "%d", cfg->cli.quiet);
    } else if (accept(&in, "search.align")) {
        if (!eol(in)) {
            char *end;
            unsigned long value = strtoul(in, &end, 0);
            while (isspace(*end)) end++;
            if (*end) {
                errf("config: bad search.align value (expected integer)");
                return 0;
            }
            cfg->search.align = value;
            if (cfg->cli.quiet)
                return 1;
        }
        if (!cfg->cli.quiet)
            fputs("search.align = ", stdout);
        fprintf(stdout, "%lu", cfg->search.align);
    } else if (accept(&in, "search.prot")) {
        if (!eol(in)) {
            char *end;
            unsigned long value = strtoul(in, &end, 10);
            while (isspace(*end)) end++;
            if (*end || value > 7) {
                errf("config: bad search.prot value");
                return 0;
            }
            cfg->search.prot = value;
            if (cfg->cli.quiet)
                return 1;
        }
        if (!cfg->cli.quiet)
            fputs("search.prot = ", stdout);
        fprintf(stdout, "%u", cfg->search.prot);
    } else if (accept(&in, "search.progress")) {
        if (!eol(in)) {
            int progress = -1;
            if (accept(&in, "0")) {
                progress = 0;
            } else if (accept(&in, "1")) {
                progress = 1;
            } else if (accept(&in, "2")) {
                progress = 2;
            }
            if (progress < 0 || !eol(in)) {
                errf("config: bad cli.progress value (expected 0, 1 or 2)");
                return 0;
            }
            cfg->search.progress = progress;
            if (cfg->cli.quiet)
                return 1;
        }
        if (!cfg->cli.quiet)
            fputs("search.progress = ", stdout);
        fprintf(stdout, "%d", cfg->search.progress);
    } else {
        size_t i;
        for (i = 0; in[i] && in[i] != '=' && !isspace(in[i]); i++);
        errf("config: unknown config item '%.*s'", i, in);
        return 0;
    }

    fputc('\n', stdout);
    return 1;
}
