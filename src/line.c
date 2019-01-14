#include "line.h"
#include "ramfuck.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#if 1
struct fgets_reader {
    struct linereader ops;
    char *buf;
    size_t len;
    size_t capacity;
};

static char *fgets_reader_get_line(struct linereader *reader, FILE *in)
{
    struct fgets_reader *liner = (struct fgets_reader *)reader;

    /* Put previous line */
    if (liner->len) {
        memset(liner->buf, '\0', liner->len);
        liner->len = 0;
    }

    /* Get next line */
    while (fgets(&liner->buf[liner->len], liner->capacity - liner->len, in)) {
        liner->len += strlen(&liner->buf[liner->len]);
        if (liner->buf[liner->len-1] == '\n') {
            liner->buf[--liner->len] = '\0';
            return liner->buf;
        } else {
            char *new = realloc(liner->buf, (liner->capacity *= 2));
            if (!new) {
                errf("line: out-of-memory");
                break;
            }
            liner->buf = new;
        }
    }
    return NULL;
}

static int fgets_reader_add_history(struct linereader *reader, const char *line)
{
    /* Do nothing */
    return 1;
}

static struct fgets_reader *fgets_reader_get()
{
    const static struct fgets_reader reader_c_init = {
        { fgets_reader_get_line, fgets_reader_add_history },
        NULL, 0, BUFSIZ
    };
    struct fgets_reader *liner = malloc(sizeof(struct fgets_reader));

    if (liner) {
        memcpy(liner, &reader_c_init, sizeof(struct fgets_reader));
        liner->buf = calloc(liner->capacity, sizeof(char));
    }
    if (!liner || !liner->buf) {
        errf("line: out-of-memory for fgets_reader");
        return NULL;
    }
    return liner;
}

static void fgets_reader_put(struct fgets_reader *reader)
{
    free(reader->buf);
    free(reader);
}

struct linereader *linereader_get()
{
    return (struct linereader *)fgets_reader_get();
}

void linereader_put(struct linereader *reader)
{
    fgets_reader_put((struct fgets_reader *)reader);
}
#else
/* readline/linenoise or something? */
#endif
