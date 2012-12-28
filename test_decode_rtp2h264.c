#include <stdio.h>
#include <stdlib.h>
#if _WIN32_
#include <windows.h>
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#define MessageBox(fmt, ...) do {;} while(0)
#define closesocket(fmt)	close(fmt)
#endif
#include "h264tortp.h"

#define DEFAULT_PORT    1234
#define RTP_DATA_BUFCNT 28000

typedef struct rtp_data {
    uint8_t *buf;
    int len;
} rtp_data_t;

rtp_data_t RTP_DATA_BUF[RTP_DATA_BUFCNT];

void decode_rtp2h264(uint8_t *rtp_buf, int len, FILE *savefp);

void decode_rtp2h264(uint8_t *rtp_buf, int len, FILE *savefp)
{
    nalu_header_t *nalu_header;
    fu_header_t   *fu_header;
    uint8_t h264_nal_header;

    /*
     * 1. 根据荷载头type判断是单一包还是分拆的包
     */
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

int main(int argc, char **argv)
{
    FILE *save_file_fd;
    uint16_t port;
    int socket_s = -1;
    struct sockaddr_in si_me;
    int ret;
    uint8_t buf[1500];

    if (argc < 2) {
        fprintf(stderr, "usage: %s save_filename [recv_port]\n", 
                argv[0]);
        exit(0);
    }

    if (argc > 2)
        port = atoi(argv[2]);
    else 
        port = DEFAULT_PORT;

    save_file_fd = fopen(argv[1], "wb");
    if (!save_file_fd) {
        perror("fopen");
        exit(1);
    }

    int i;
    for (i = 0; i < RTP_DATA_BUFCNT; i++) {
        RTP_DATA_BUF[i].buf = malloc(1500);
        RTP_DATA_BUF[i].len = 0;
    }

    /*
     * init socket
     */
#if _WIN32_
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(1, 1), &wsadata) == SOCKET_ERROR) {
        fprintf(stderr, "WSAStartup fail");
        exit(1);
    }
#endif

    /*
     * 初始化 socket
     */
    socket_s = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_s < 0) {
        MessageBox(NULL, TEXT("socket fail"), TEXT("title"), MB_OK);
        exit(1);
    }

        unsigned int value = 1;
        setsockopt(socket_s, SOL_SOCKET, SO_REUSEADDR, 
                   &value, sizeof(value));  /* 端口复用 */

        memset((char *)&si_me, 0, sizeof(si_me));
        si_me.sin_family = AF_INET;
        si_me.sin_port = htons(port);
        si_me.sin_addr.s_addr = htonl(INADDR_ANY);

        ret = bind(socket_s, (const struct sockaddr *)&si_me, sizeof(si_me));

#if 1
        while (1) {
            ret = recv(socket_s, 
                       buf,
                       sizeof(buf),
                       0);
            if (ret < 0) {
                fprintf(stderr, "recv fail\n");
                continue;
            }

            decode_rtp2h264(buf, ret, save_file_fd);
        } /* wile (1) */
#else
        i = 0;
        while (1) {
            ret = recv(socket_s, 
                       RTP_DATA_BUF[i].buf,
                       1500,
                       0);
            if (ret < 0) {
                fprintf(stderr, "recv fail\n");
                continue;
            }
            RTP_DATA_BUF[i].len = ret;
            i++;
            // if (i >= RTP_DATA_BUFCNT)
            if (i >= RTP_DATA_BUFCNT / 2)
                break;
        } /* wile (1) */
        for (i = 0; i < RTP_DATA_BUFCNT / 2; i++) {
            decode_rtp2h264(RTP_DATA_BUF[i].buf, 
                            RTP_DATA_BUF[i].len,
                            save_file_fd);
        }
#endif

    fclose(save_file_fd);
    if (socket_s > 0)
        closesocket(socket_s);

    return 0;
}
