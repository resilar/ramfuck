#ifndef LINE_H_INCLUDED
#define LINE_H_INCLUDED

#include <stdio.h>

struct linereader;

/* Get a line reader for the given input stream */
struct linereader *linereader_get(FILE *in);

/* Release line reader and return its input sream (if still valid) */
FILE *linereader_put(struct linereader *reader);

/* Release line reader and close its input sream (if still open) */
void linereader_close(struct linereader *reader);

/* Get line with prompt message */
char *linereader_get_line(struct linereader *reader, const char *prompt);

/* Free line returned by linereader_get_line(); */
void linereader_free_line(struct linereader *reader, char *line);

/* Add line to history and return 0 on success (not necessarily supported) */
int linereader_add_history(struct linereader *reader, const char *line);

#endif
