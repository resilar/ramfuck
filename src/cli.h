#ifndef CLI_H_INCLUDED
#define CLI_H_INCLUDED

#include "ramfuck.h"

/* Execute a single line (no semicolons or hashtags allowed) */
int cli_execute_line(struct ramfuck *ctx, const char *in);

/* Execute line(s) */
int cli_execute(struct ramfuck *ctx, char *in);

/* Execute formatted line */
int cli_execute_format(struct ramfuck *ctx, const char *format, ...);

/* Run main loop */
int cli_main_loop(struct ramfuck *ctx);

#endif
