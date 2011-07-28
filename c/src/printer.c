#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "api.h"
#include "printer.h"
#include "smile_decode.h"

#define INDENT printf("%-*s", indent, " ");

// Don't pretty print by default
static int pretty_print = 0;
static int indent = 0;

inline void print_start_object()
{
    if (pretty_print) {
        INDENT
    }
    putchar('{');
    if (pretty_print) {
        putchar('\n');
        indent += 2;
    }
}

inline void print_end_object()
{
    if (pretty_print) {
        indent -= 2;
        INDENT
    }
    putchar('}');
    if (pretty_print) {
        putchar('\n');
    }
}

inline void print_start_array()
{
    putchar('[');
    if (pretty_print) {
        putchar('\n');
        indent += 2;
    }
}

inline void print_end_array()
{
    putchar(']');
    if (pretty_print) {
        putchar('\n');
        indent += 2;
    }
}

inline void print_start_key()
{
    if (pretty_print) {
        INDENT
    }
}

inline void print_end_key()
{
    putchar(':');
    if (pretty_print) {
       putchar(' ');
    }
}

inline void print_start_value()
{
}

inline void print_end_value()
{
    putchar(',');
    if (pretty_print) {
        putchar('\n');
    }
}

inline void print_null_value()
{
    printf("null");
}

inline void print_false_value()
{
    printf("false");
}

inline void print_true_value()
{
    printf("true");
}

inline void print_number_value(long number)
{
    printf("%lu", number);
}

void print_string(const u8* ch, int start, int length)
{
    putchar('"');
    const u8* ip = ch + start;
    while (length--) {
        putchar(*ip++);
    }
    putchar('"');
}

struct content_handler printer = {
    .name = "PRINTER",
    .start_object = print_start_object,
    .end_object = print_end_object,
    .start_array = print_start_array,
    .end_array = print_end_array,
    .start_key = print_start_key,
    .end_key = print_end_key,
    .start_value = print_start_value,
    .end_value = print_end_value,
    .null_value = print_null_value,
    .false_value = print_false_value,
    .true_value = print_true_value,
    .number_value = print_number_value,
    .characters = print_string
};

// Decodes raw smile
int main(int argc, char *argv[])
{
    int fd, rc = 0;
    struct stat mystat;
    const char* fname = argv[1];
    size_t nbytes;
    ssize_t bytes_read;
    u8 buf[BUFFER_SIZE];
    u8 header[4];
    int pretty = 0;
    static const struct option opts[] =
    {
        { "pretty", no_argument, NULL, 'p' },
        { NULL,     no_argument, NULL, '\0'}
    };

    int c;
    while ((c = getopt_long (argc, argv, "p", opts, NULL)) != -1) {
        switch (c) {
            case 'p':
                pretty_print = 1;
        }
    }

    nbytes = sizeof(buf);

    rc = stat(fname, &mystat);
    fd = open(fname, O_RDONLY);

    if (fd == -1) {
        fprintf (stderr, "%s: not a regular file.\n", fname);
        exit(1);
    }


    bytes_read = read(fd, header, 4);
    if (bytes_read != 4) {
          fprintf(stderr, "%s: unable to read header: \n", fname);
          perror ("");
          goto exit;
    } else {
        if (!smile_decode_header(header).valid) {
            fprintf(stderr, "%s: bad header: %s\n", fname, header);
            goto exit;
        } else {
            dprintf("Got header: %s", header);
        }
    }

    while (1) {
        // Read block by block
        bytes_read = read(fd, buf, nbytes);
        if (bytes_read == -1) {
          fprintf(stderr, "%s: read error: \n", fname);
          perror ("");
          goto exit;
        } else if (bytes_read == 0) {
            break;
        }

        smile_decode(buf, nbytes, &printer);
   }

exit:
    close(fd);
    dprintf("Done!");
    exit(0);
}
