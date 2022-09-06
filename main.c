#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#include "FragDecoder.h"

int8_t flash_write(uint32_t addr, uint8_t *buf, uint32_t len);
int8_t flash_read(uint32_t addr, uint8_t *buf, uint32_t len);

#define FRAG_SIZE               (232)

uint8_t flash_buf[FRAG_MAX_NB * FRAG_MAX_SIZE];
int skip_arr[100];
int skip_num;

FragDecoderStatus_t decoder_status;
int32_t decoder_process_status;

/* output similar to Zephyr LOG_HEXDUMP */
void log_hex(uint8_t *buf, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (i % 8 == 0) {
            printf(" ");
        }
        if (i != 0 && i % 64 == 0) {
            printf("\n ");
        }
        printf("%02x ", buf[i]);
    }
    printf("\n");
}

int8_t flash_write(uint32_t addr, uint8_t *buf, uint32_t len)
{
    static bool frag_written[FRAG_MAX_NB];
    int frag = addr / FRAG_SIZE;

	printf("Writing %u bytes to addr 0x%x:\n", len, addr);

    log_hex(buf, len);

    /* simulate behavior of actual flash, allowing only single write operations */
    if (!frag_written[frag]) {
        memcpy(flash_buf + addr, buf, len);
        frag_written[frag] = true;
    }
}

int8_t flash_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    memcpy(buf, flash_buf + addr, len);
}

bool skip(int nb)
{
    for (int i = 0; i < skip_num; i++) {
        if (skip_arr[i] == nb) {
            return true;
        }
    }

    return false;
}

int main(int argc, char *argv[])
{
    int ret, len, nb;
    uint8_t buf[FRAG_SIZE];

    FragDecoderCallbacks_t decoder_callbacks = { 0 };

    if (argc < 4) {
        printf("command-line arguments: <input.bin> <output.bin> <nb_frag> <skipped_frags>\n");
        return -1;
    }

    int nb_frag = atoi(argv[3]);

    for (int i = 0; i < argc - 4 && i < sizeof(skip_arr); i++) {
        skip_arr[i] = atoi(argv[i + 4]);
        skip_num++;
    }

    FragDecoderInit(nb_frag, FRAG_SIZE, &decoder_callbacks);
    decoder_process_status = FRAG_SESSION_ONGOING;

    memset(flash_buf, 0xFF, sizeof(flash_buf));

    /*
     * Assign callbacks after initialization to prevent the FragDecoder
     * from writing byte-wise 0xFF to the entire flash. Instead, erase
     * flash properly with own implementation.
     */
    decoder_callbacks.FragDecoderWrite = flash_write;
    decoder_callbacks.FragDecoderRead = flash_read;

    FILE *infile = fopen(argv[1], "rb");
    FILE *outfile = fopen(argv[2], "wb");

    nb = 0;
    do {
        nb++;
        len = fread(buf, 1, sizeof(buf), infile);
        if (!skip(nb)) {
            if (decoder_process_status == FRAG_SESSION_ONGOING) {
                decoder_process_status = FragDecoderProcess(nb, buf);
                decoder_status = FragDecoderGetStatus();
                printf("frag %d, process status: %d\n", nb, decoder_process_status);
            }
            else if (decoder_process_status >= 0) {
                printf("All fragments written to flash\n");
            }
        }
        else {
            printf("skipped frag %d\n", nb);
        }
    } while (len > 0);

    fwrite(flash_buf, 1, sizeof(flash_buf), outfile);

    fclose(infile);
    fclose(outfile);
}

