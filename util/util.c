
#include "util.h"

#define HEXDUMP_COL 16
void hexdump(const void *data, const int len, FILE *stream)
{
    char ascii_buf[HEXDUMP_COL + 1];
    const unsigned char *ptr = data;

    ascii_buf[HEXDUMP_COL] = '\0';

    int linecount = 0;
    int lineoffset;
    for (int i = 0; i < len; i++)
    {
        lineoffset = i % HEXDUMP_COL;

        /* Print offset if newline */
        if (lineoffset == 0)
            fprintf(stream, " %04x ", (unsigned int)i);

        /* Add space at every 4 bytes.. */
        if (lineoffset % 4 == 0)
            fprintf(stream, " ");

        fprintf(stream, " %02x", ptr[i]);
        if ((ptr[i] < 0x20) || (ptr[i] > 0x7e))
            ascii_buf[lineoffset] = '.';
        else
            ascii_buf[lineoffset] = ptr[i];

        /* Print ASCII if end of line */
        if (lineoffset == HEXDUMP_COL - 1)
        {
            fprintf(stream, "    %s\n", ascii_buf);
            linecount++;

            /* Print additional newline at every 5 lines */
            if (linecount != 0 && linecount % 5 == 0)
                fprintf(stream, "\n");
        }
    }

    for (int i = lineoffset + 1; i < HEXDUMP_COL; i++){
        lineoffset = i % HEXDUMP_COL;
        if (lineoffset % 4 == 0)
            fprintf(stream, " ");
        fprintf(stream, " ..");
    }

    ascii_buf[lineoffset + 1] = '\0';
    fprintf(stream, "    %s\n", ascii_buf);
}