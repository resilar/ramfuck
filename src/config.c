#include "config.h"
#include "ramfuck.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static int accept(const char **pin, const char *what)
{
    size_t i;
    for (i = 0; what[i] && what[i] == (*pin)[i]; i++);
    if (!what[i] && (!(*pin)[i] || (*pin)[i] == '=' || isspace((*pin)[i]))) {
        for (*pin += i; **pin && isspace(**pin); (*pin)++);
        i = **pin == '=';
        for (*pin += i; **pin && isspace(**pin); (*pin)++);
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
        cfg->search.align = 0;
        cfg->search.prot = 6; /* MEM_READ | MEM_WRITE */
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
        config_process_line(cfg, "search.align");
        config_process_line(cfg, "search.prot");
        return 1;
    }

    if (accept(&in, "search.align")) {
        if (!eol(in)) {
            char *end;
            unsigned long value = strtoul(in, &end, 10);
            if (*end || value != (unsigned int)value) {
                errf("config: bad search.align value");
                return 0;
            }
            cfg->search.align = value;
        }
        infof("search.align = %u", cfg->search.align);
        return 1;
    } else if (accept(&in, "search.prot")) {
        if (!eol(in)) {
            char *end;
            unsigned long value = strtoul(in, &end, 10);
            if (*end || value != (unsigned int)value || value > 7) {
                errf("config: bad search.prot value");
                return 0;
            }
            cfg->search.prot = value;
        }
        infof("search.prot = %u", cfg->search.prot);
        return 1;
    } else {
        size_t i;
        for (i = 0; in[i] && in[i] != '=' && !isspace(in[i]); i++);
        errf("config: unknown config item '%.*s'", i, in);
    }

    return 0;
}
