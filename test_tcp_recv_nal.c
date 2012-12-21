#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include "debug_log.h"
#include "h264tortp.h"

#define DEBUG_PRINT         1
#define debug_print(fmt, ...) \
    do { if (DEBUG_PRINT) fprintf(stderr, "-------%s: %d: %s():---" fmt "----\n", \
            __FILE__, __LINE__, __func__, ##__VA_ARGS__);} while (0)


#define SO_MAXCONN              10
#define DEFAULT_PORT            1893

FILE *OUTPUT_FD;
static int USE_TCP = 1; /* 1: TCP; 0: UDP */
static int sock_opt = 1;
static int TCP_NODELAY_FLAG = 0;

int bind_server(int server_s, const char *server_ip, uint16_t server_port);
int bind_server(int server_s, const char *server_ip, uint16_t server_port)
{
    struct sockaddr_in server_sockaddr;

    memset(&server_sockaddr, 0, sizeof server_sockaddr);
    server_sockaddr.sin_family = AF_INET;
    if (server_ip)
        inet_aton(server_ip, &server_sockaddr.sin_addr);
    else
        server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    server_sockaddr.sin_port = htons(server_port);

    return bind(server_s, (struct sockaddr *)&server_sockaddr,
                sizeof(server_sockaddr));
}

static int create_server_socket(const char *server_ip, uint16_t server_port);
static int create_server_socket(const char *server_ip, uint16_t server_port)
{
    int server_s;
    int ret;

    if (USE_TCP)
        server_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    else     /* use udp */
        server_s = socket(AF_INET, SOCK_DGRAM, 0);

    if (server_s == -1) {
        debug_log("unable to create socket");
        exit(1);
    }

    /* reuse socket addr */
    if ((setsockopt(server_s, SOL_SOCKET, SO_REUSEADDR, (void *) &sock_opt,
                    sizeof(sock_opt))) == -1) {
        debug_log("setsockopt");
        exit(1);
    }

    if (USE_TCP && TCP_NODELAY_FLAG) {
        ret = setsockopt(server_s, IPPROTO_TCP, TCP_NODELAY, 
                         (void *)&sock_opt, sizeof(sock_opt));
        if (ret == -1) {
            debug_log("setsockopt");
            exit(1);
        }
    }

    /* internet family-specific code encapsulated in bind_server()  */
    if (bind_server(server_s, server_ip, server_port) == -1) {
        debug_log("unable to bind");
    }

    /* listen: large number just in case your kernel is nicely tweaked */
    if (listen(server_s, SO_MAXCONN) == -1) {
        debug_log("unable to listen");
    }
    return server_s;
}

void show_usage(char *argv);
void show_usage(char *argv)
{
    fprintf(stderr, "usage: %s [OPTIONS] outfile\n", argv);
    fprintf(stderr, "options:\n");
    fprintf(stderr, "        -t|u          send via tcp(default) or udp\n");
    fprintf(stderr, "        -p port       dest_port\n");
    fprintf(stderr, "        -q            setsocktcp tcp_nodelay\n");

    exit(1);
}

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

void *recv_fun(void *p);
void *recv_fun(void *p)
{
    int ret;
    uint8_t recv_buf[2000];
    int sock_fd;

    sock_fd = *(int *)p;

    debug_print("in recv_fun");
    while (1) {
        ret = recv(sock_fd, recv_buf, sizeof(recv_buf), 0);
        if (ret < 0) {
            debug_log("recv fail");
        } else if (!ret) {
            debug_print("client have closed");
            break;
        }
        decode_rtp2h264(recv_buf, ret, OUTPUT_FD);
        // fprintf(stderr, "len is %d\n", ret);
    }

    return NULL;
}

int main(int argc, char **argv)
{
    char *thisarg;
    int sock_fd;
    uint16_t port = DEFAULT_PORT;
    char *myname = argv[0];
    int new_fd;
    struct sockaddr_in remote_addr;
    socklen_t remote_addrlen = sizeof(struct sockaddr_in);
    int ret;
    pthread_t recv_pid;

    open_debug_log("_test_tcp_recv_nal.log");
    argc--;
    argv++;
    /* parse any options */
    while (argc >= 1 && **argv == '-') {
        thisarg = *argv;
        thisarg++;
        switch (*thisarg) {
        case 't':
            break;
        case 'u':
            USE_TCP = 0;
            break;
        case 'p':
            if (--argc <= 0)
                show_usage(myname);
            argv++;
            port = atoi(*argv);
            break;
        case 'q':
            TCP_NODELAY_FLAG = 1;
            break;
        default:
            show_usage(myname);
        }
        argc--;
        argv++;
    }
    if (argc < 1)
        show_usage(myname);

    OUTPUT_FD = fopen(*argv, "wb");
    if (!OUTPUT_FD) {
        perror("fopen");
        exit(1);
    }

    sock_fd = create_server_socket(NULL, port);

    while (1) {
        if (USE_TCP) {
            new_fd = accept(sock_fd, (struct sockaddr *)&remote_addr,
                            &remote_addrlen);
            if (new_fd < 0) {
                debug_log("accept fail");
                exit(1);
            }

            pthread_create(&recv_pid, NULL, recv_fun, &new_fd);
        } else {

        }
    }
    pthread_join(recv_pid, NULL);

    close(sock_fd);
    fclose(OUTPUT_FD);
    close_debug_log();
    return 0;
}
