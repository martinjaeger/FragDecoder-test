#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "FragDecoder.h"
#include <string.h>

int8_t flash_write(uint32_t addr, uint8_t *buf, uint32_t len);
int8_t flash_read(uint32_t addr, uint8_t *buf, uint32_t len);

#define FRAG_NB                 (1907)
#define FRAG_SIZE               (232)
#define FRAG_CR                 (FRAG_NB + 10)
#define FRAG_TOLERENCE          (10 + FRAG_NB * (FRAG_PER + 0.05))
#define FRAG_PER                (0.2)

uint8_t dec_buf[(FRAG_NB + FRAG_CR) * FRAG_SIZE + 1024*1024];
uint8_t dec_flash_buf[(FRAG_NB + FRAG_CR) * FRAG_SIZE + 1024*1024];

int8_t flash_write(uint32_t addr, uint8_t *buf, uint32_t len)
{
    memcpy(dec_flash_buf + addr, buf, len);
}

int8_t flash_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    memcpy(buf, dec_flash_buf + addr, len);
}

void putbuf(uint8_t *buf, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        printf("%02X ", buf[i]);
    }
    printf("\n");
}

FragDecoderStatus_t decoder_status;
//FragDecoderCallbacks_t decoder_callbacks;
int32_t decoder_process_status;

int main()
{
    int ret, len, nb;
    uint8_t buf[FRAG_SIZE];

    FragDecoderCallbacks_t decoder_callbacks = {
        .FragDecoderWrite = flash_write,
        .FragDecoderRead = flash_read,
    };

    FragDecoderInit(FRAG_NB, FRAG_SIZE, &decoder_callbacks);

    freopen(NULL, "rb", stdin);
    FILE *outfile = fopen("../../app_update_decoded_fragdecoder.bin", "wb");

    nb = 0;
    do {
        nb++;
        len = fread(buf, 1, sizeof(buf), stdin);
        if (nb % 10 != 0) {
            // skip each 10th fragment
            decoder_process_status = FragDecoderProcess(nb, buf);
            decoder_status = FragDecoderGetStatus();
            printf("frag %d, process status: %d\n", nb, decoder_process_status);
        }
    } while (len > 0);

    fwrite(dec_flash_buf, 1, sizeof(dec_flash_buf), outfile);

    fclose(stdin);
    fclose(outfile);
}

