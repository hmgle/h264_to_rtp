#ifndef _H264TORTP_H
#define _H264TORTP_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "llist.h"

#define NAL_MAX     4000000
#define H264        96
#define G711        8

typedef struct rtp_header {
    /* little-endian */
    /* byte 0 */
    uint8_t csrc_len:       4;  /* bit: 0~3 */
    uint8_t extension:      1;  /* bit: 4 */
    uint8_t padding:        1;  /* bit: 5*/
    uint8_t version:        2;  /* bit: 6~7 */
    /* byte 1 */
    uint8_t payload_type:   7;  /* bit: 0~6 */
    uint8_t marker:         1;  /* bit: 7 */
    /* bytes 2, 3 */
    uint16_t seq_no;            
    /* bytes 4-7 */
    uint32_t timestamp;        
    /* bytes 8-11 */
    uint32_t ssrc;
} __attribute__ ((packed)) rtp_header_t; /* 12 bytes */

typedef struct nalu_header {
    /* byte 0 */
	uint8_t type:   5;  /* bit: 0~4 */
    uint8_t nri:    2;  /* bit: 5~6 */
	uint8_t f:      1;  /* bit: 7 */
} __attribute__ ((packed)) nalu_header_t; /* 1 bytes */

typedef struct nalu {
    int startcodeprefix_len;
    unsigned len;             /* Length of the NAL unit (Excluding the start code, which does not belong to the NALU) */
    unsigned max_size;        /* Nal Unit Buffer size */
    int forbidden_bit;        /* should be always FALSE */
    int nal_reference_idc;    /* NALU_PRIORITY_xxxx */
    int nal_unit_type;        /* NALU_TYPE_xxxx */
    char *buf;                /* contains the first byte followed by the EBSP */
    unsigned short lost_packets;  /* true, if packet loss is detected */
} nalu_t;


typedef struct fu_indicator {
    /* byte 0 */
    uint8_t type:   5;
	uint8_t nri:    2; 
	uint8_t f:      1;    
} __attribute__ ((packed)) fu_indicator_t; /* 1 bytes */

typedef struct fu_header {
    /* byte 0 */
    uint8_t type:   5;
	uint8_t r:      1;
	uint8_t e:      1;
	uint8_t s:      1;    
} __attribute__ ((packed)) fu_header_t; /* 1 bytes */

typedef struct rtp_package {
    rtp_header_t rtp_package_header;
    uint8_t *rtp_load;
} rtp_t;

struct func_para {
    uint8_t *send_buf;
    size_t len_sendbuf;
    linklist iplist;
};

int h264naltortp_send(int framerate, uint8_t *pstStream, int nalu_len, void (*deal_func)(void *p), void *deal_func_para);
void add_client_to_list(linklist client_ip_list, char *ipaddr);

#endif
