#include "line.h"
#include "ramfuck.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

struct linereader {
    FILE *(*put)(struct linereader *reader);
    char *(*get_line)(struct linereader *reader, const char *prompt);
    void (*free_line)(struct linereader *reader, char *line);
    int (*add_history)(struct linereader *reader, const char *line);
};

#if 1
struct fgets_reader {
    struct linereader ops;
    FILE *in;
    char *buf;
    size_t len;
    size_t capacity;
};

static FILE *fgets_reader_put(struct linereader *reader)
{
    struct fgets_reader *this = (struct fgets_reader *)reader;
    FILE *in = this->in;
    free(this->buf);
    free(this);
    return in;
}

static char *fgets_reader_get_line(struct linereader *reader,
                                   const char *prompt)
{
    struct fgets_reader *this = (struct fgets_reader *)reader;

    /* Prompt */
    if (prompt) {
        fprintf(stdout, "%s", prompt);
        fflush(stdout);
    }

    /* Clear previous line */
    if (this->len) {
        errf("line: get_line() called twice without free_line() in between");
        memset(this->buf, '\0', this->len);
        this->len = 0;
    }

    /* Fail if input stream is already closed */
    if (this->in == NULL)
        return NULL;

    /* Get next line */
    while (fgets(&this->buf[this->len], this->capacity - this->len, this->in)) {
        this->len += strlen(&this->buf[this->len]);
        if (this->buf[this->len-1] == '\n') {
            this->buf[--this->len] = '\0';
            return this->buf;
        } else {
            char *new = realloc(this->buf, (this->capacity *= 2));
            if (!new) {
                errf("line: out-of-memory");
                return NULL;
            }
            this->buf = new;
        }
    }
    fclose(this->in);
    this->in = NULL;
    return NULL;
}

static void fgets_reader_free_line(struct linereader *reader, char *line)
{
    struct fgets_reader *this = (struct fgets_reader *)reader;
    if (line == this->buf) {
        memset(this->buf, '\0', this->len);
        this->len = 0;
    } else {
        errf("line: fgets_reader_free_line() received invalid line pointer");
        abort();
    }
}

static int fgets_reader_add_history(struct linereader *reader, const char *line)
{
    /* Do nothing */
    return 0;
}


static struct fgets_reader *fgets_reader_get(FILE *in)
{
    const static struct fgets_reader reader_c_init = {
        {
            fgets_reader_put,
            fgets_reader_get_line,
            fgets_reader_free_line,
            fgets_reader_add_history
        },
        NULL, NULL, 0, BUFSIZ
    };
    struct fgets_reader *this = malloc(sizeof(struct fgets_reader));

    if (this) {
        memcpy(this, &reader_c_init, sizeof(struct fgets_reader));
        this->in = in;
        this->buf = calloc(this->capacity, sizeof(char));
        if (!this->buf) {
            errf("line: out-of-memory for fgets_reader buffer of BUFSIZ bytes");
            free(this);
            this = NULL;
        }
    } else {
        errf("line: out-of-memory for fgets_reader");
    }
    return this;
}

struct linereader *linereader_get(FILE *in)
{
    return (struct linereader *)fgets_reader_get(in);
}

FILE *linereader_put(struct linereader *reader)
{
    return reader->put(reader);
}

void linereader_close(struct linereader *reader)
{
    FILE *in = reader->put(reader);
    if (in != NULL)
        fclose(in);
}

char *linereader_get_line(struct linereader *reader, const char *prompt)
{
    return reader->get_line(reader, prompt);
}

void linereader_free_line(struct linereader *reader, char *line)
{
    if (reader->free_line)
        reader->free_line(reader, line);
}

int linereader_add_history(struct linereader *reader, const char *line)
{
    return reader->add_history ? reader->add_history(reader, line) : -1;
}

#else
/* readline/linenoise or something? */
#endif
