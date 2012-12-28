#include <stdio.h>
#include <stdlib.h>
#if !_WIN32_
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#else
#include <windows.h>
#endif
#include "h264tortp.h"

#define NAL_BUF_SIZE                (1500 * 150)

#define DEBUG_PRINT         0
#define debug_print(fmt, ...) \
    do { if (DEBUG_PRINT) fprintf(stderr, "-------%s: %d: %s():---" fmt "----\n", \
            __FILE__, __LINE__, __func__, ##__VA_ARGS__);} while (0)


uint8_t nal_buf[NAL_BUF_SIZE];

typedef struct rtp_data {
    uint8_t *buf;
    int len;
} rtp_data_t;

void decode_rtp2h264(uint8_t *rtp_buf, int len, FILE *savefp);

void decode_rtp2h264(uint8_t *rtp_buf, int len, FILE *savefp)
{
    nalu_header_t *nalu_header;
    fu_header_t   *fu_header;
    uint8_t h264_nal_header;

    nalu_header = (nalu_header_t *)&rtp_buf[12];
    if (nalu_header->type == 28) { /* FU-A */
        fu_header = (fu_header_t *)&rtp_buf[13];
        if (fu_header->e == 1) { /* end of fu-a */
            fwrite(&rtp_buf[14], 1, len - 14, savefp);
        } else if (fu_header->s == 1) { /* start of fu-a */
            // fprintf(savefp, "%c%c%c%c", 0, 0, 0, 1);
            fputc(0, savefp);
            fputc(0, savefp);
            fputc(0, savefp);
            fputc(1, savefp);
            h264_nal_header = (fu_header->type & 0x1f) 
                | (nalu_header->nri << 5)
                | (nalu_header->f << 7);
            fputc(h264_nal_header, savefp);
            fwrite(&rtp_buf[14], 1, len - 14, savefp);
        } else { /* middle of fu-a */
            fwrite(&rtp_buf[14], 1, len - 14, savefp);
        }
    } else { /* nalu */
        // fprintf(savefp, "%c%c%c%c", 0, 0, 0, 1);
        fputc(0, savefp);
        fputc(0, savefp);
        fputc(0, savefp);
        fputc(1, savefp);
        h264_nal_header = (nalu_header->type & 0x1f) 
            | (nalu_header->nri << 5)
            | (nalu_header->f << 7);
        fputc(h264_nal_header, savefp);
        fwrite(&rtp_buf[13], 1, len - 13, savefp);
    }

    fflush(savefp);
    return;
}

static int copy_nal_from_file(FILE *fp, uint8_t *buf, int *len);
static int copy_nal_from_file(FILE *fp, uint8_t *buf, int *len)
{
    char tmpbuf[4];     /* i have forgotten what this var mean */
    char tmpbuf2[1];    /* i have forgotten what this var mean */
    int flag = 0;       /* i have forgotten what this var mean */
    int ret;

#if 0
    ret = fread(tmpbuf, 4, 1, fp);
    if (!ret)
        return 0;
#endif

    *len = 0;

    do {
        ret = fread(tmpbuf2, 1, 1, fp);
        if (!ret) {
            return -1;
        }
        if (!flag && tmpbuf2[0] != 0x0) {
            buf[*len] = tmpbuf2[0];
            (*len)++;
            // debug_print("len is %d", *len);
        } else if (!flag && tmpbuf2[0] == 0x0) {
            flag = 1;
            tmpbuf[0] = tmpbuf2[0];
            debug_print("len is %d", *len);
        } else if (flag) {
            switch (flag) {
            case 1:
                if (tmpbuf2[0] == 0x0) {
                    flag++;
                    tmpbuf[1] = tmpbuf2[0];
                } else {
                    flag = 0;
                    buf[*len] = tmpbuf[0];
                    (*len)++;
                    buf[*len] = tmpbuf2[0];
                    (*len)++;
                }
                break;
            case 2:
                if (tmpbuf2[0] == 0x0) {
                    flag++;
                    tmpbuf[2] = tmpbuf2[0];
                } else if (tmpbuf2[0] == 0x1) {
                    flag = 0;
                    return *len;
                } else {
                    flag = 0;
                    buf[*len] = tmpbuf[0];
                    (*len)++;
                    buf[*len] = tmpbuf[1];
                    (*len)++;
                    buf[*len] = tmpbuf2[0];
                    (*len)++;
                }
                break;
            case 3:
                if (tmpbuf2[0] == 0x1) {
                    flag = 0;
                    return *len;
                } else {
                    flag = 0;
                    break;
                }
            }
        }

    } while (1);

    return *len;
} /* static int copy_nal_from_file(FILE *fp, char *buf, int *len) */

int main(int argc, char **argv)
{
    FILE *input_fp;
    FILE *output_fp;
    int ret;
    uint8_t buf[1500];

    if (argc < 3) {
        fprintf(stderr, "usage: %s inputfile outputfile\n", 
                argv[0]);
        exit(0);
    }

    input_fp = fopen(argv[1], "r");
    if (!input_fp) {
        perror("fopen");
        exit(1);
    }

    output_fp = fopen(argv[2], "wb");
    if (!output_fp) {
        perror("fopen");
        exit(1);
    }

    while (copy_nal_from_file(input_fp, nal_buf, &len) != -1) {

    }

    fclose(input_fp);
    fclose(output_fp);

    return 0;
}
