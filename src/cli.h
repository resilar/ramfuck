#ifndef CLI_H_INCLUDED
#define CLI_H_INCLUDED

#include "ramfuck.h"

int cli_execute_line(struct ramfuck *ctx, const char *in);

int cli_execute_format(struct ramfuck *ctx, const char *format, ...);

int cli_main_loop(struct ramfuck *ctx);

#endif
