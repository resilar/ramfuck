#ifndef LINE_H_INCLUDED
#define LINE_H_INCLUDED

#include <stdio.h>

struct linereader {
    char *(*get_line)(struct linereader *reader, FILE *in);
    int (*add_history)(struct linereader *reader, const char *line);
};

struct linereader *linereader_get();

void linereader_put(struct linereader *reader);

#endif
