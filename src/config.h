#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

struct config {
    struct {
        /*
         * Default block size for various commands (e.g., hex).
         */
        unsigned long size;
    } block;

    struct {
        /*
         * Default base for displaying numbers in CLI.
         * 10 -> decimal
         * 16 -> hexadecimal
         */
        unsigned int base;

        /*
         * Quiet CLI mode.
         * 0 -> isatty(STDOUT_FILENO)
         * 1 -> !isatty(STDOUT_FILENO)
         */
        int quiet;
    } cli;

    struct {
        /*
         * Alignment to use when searching a value.
         * 0 -> sizeof(value)
         * n -> n bytes
         */
        unsigned long align;

        /*
         * Memory protections required to search a region.
         * 1 -> EXECUTE
         * 2 -> WRITE
         * 4 -> READ
         */
        unsigned int prot;

        /*
         * Print progress.
         * 0 -> Do not print progress
         * 1 -> Output scanned memory regions (unless cli.quiet == 1)
         * 2 -> Output scanned memory regions
         */
        int progress;
    } search;
};

/* Allocate a new config with default settings */
struct config *config_new();

/* Delete an existing config */
void config_delete(struct config *cfg);

/* Get or set a config item */
int config_process_line(struct config *cfg, const char *in);

#endif
