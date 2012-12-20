#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define MAX_NAL_BUF_SIZE        (1500 * 100)

int TCP_OR_UDP = 0; /* 0: TCP; 1: UDP */

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

void show_usage(char *argv);
void show_usage(char *argv)
{
    fprintf(stderr, "usage: %s [OPTIONS] inputfile\n", argv);
    fprintf(stderr, "options:\n");
    fprintf(stderr, "        -h host       remote host\n");
    fprintf(stderr, "        -t|u          send via tcp(default) or udp\n");
    fprintf(stderr, "        -p port       dest_port\n");

    exit(1);
}

int main(int argc, char **argv)
{
    char *thisarg;
    int sock_fd;
    FILE *h264file;
    struct sockaddr_in server_s;
    uint16_t port;
    char *myname = argv[0];
    char remote_host[32] = "";

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
            TCP_OR_UDP = 1;
            break;
        case 'p':
            if (--argc <= 0)
                show_usage(myname);
            argv++;
            port = atoi(*argv);
            break;
        case 'h':
            if (--argc <= 0)
                show_usage(myname);
            argv++;
            strncpy(remote_host, *argv, sizeof(remote_host));
            break;
        default:
            show_usage(myname);
        }
        argc--;
        argv++;
    }
    if (argc < 1)
        show_usage(myname);

    h264file = fopen(*argv, "r");
    if (!h264file) {
        perror("fopen");
        exit(1);
    }

    // init_socket();


    fclose(h264file);
    return 0;
}
