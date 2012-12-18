#ifndef _LLIST_H
#define _LLIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 
 * 删除客户端节点的send失败次数阀值, 
 * 避免客户端浏览器非正常退出后还一直send();
 */
#ifndef MAX_SEND_FAIL_N
#define MAX_SEND_FAIL_N     29
#endif

typedef struct _client_info {
    char ipaddr[16];
    int socket_c;
} client_info;

struct node {
    client_info node_info;
    int send_fail_n;        /* 记录了send失败次数 */
    struct node *next;
};
typedef struct node *pnode;
typedef struct node *linklist;

linklist create_null_list_link(void);
int is_nulllist_link(linklist llist);
linklist insert_link(linklist llist, const char *ipaddr);
linklist delete_node(linklist llist, const char *ipaddr);

/*
 * 在llist删除this_pnode节点， 返回this_pnode前一个节点
 */
pnode delete_this_node(linklist llist, pnode this_pnode);

pnode search_node(linklist llist, const char *ipaddr);
int num_node(linklist llist);
int insert_nodulp_node(linklist llist, const char *ipaddr);
int free_linklist(linklist llist);

#endif /* ifndef _LLIST_H */
