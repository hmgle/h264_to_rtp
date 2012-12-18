#include "llist.h"

linklist create_null_list_link(void)
{
    linklist llist = (linklist)malloc(sizeof(struct node));
    if (llist != NULL)
        llist->next = NULL;
    else
        fprintf(stderr, "out of space!\n");

    return llist;
} /* linklist create_null_list_link(void) */

int is_nulllist_link(linklist llist)
{
    return llist->next == NULL;
} /* int is_nulllist_link(linklist llist) */

linklist insert_link(linklist llist, const char *ipaddr)
{
    pnode q = (pnode)malloc(sizeof(struct node));
    if (q == NULL) {
        fprintf(stderr, "out of space!\n");
    } else {
        strcpy(q->node_info.ipaddr, ipaddr);
        q->next = llist->next;
        llist->next = q;
    }
    return llist;
} /* linklist insert_link(linklist llist, const char *ipaddr) */

linklist delete_node(linklist llist, const char *ipaddr)
{
    if (llist->next == NULL)
        return llist;

    pnode q = llist->next;
    pnode p = llist;
    do {
        if (!strcmp(q->node_info.ipaddr, ipaddr)) {
            p->next = q->next;
            free(q);
            q = NULL;
            break;
        }
        p = q;
        q = q->next;
    } while (q != NULL);

    return llist;
} /* linklist delete_node(linklist llist, const char *ipaddr) */


pnode delete_this_node(linklist llist, pnode this_pnode)
{
    if (llist->next == NULL)
        return llist;

    pnode q = llist->next;
    pnode p = llist;

    do {
        if (q == this_pnode) {
            p->next = q->next;
            free(q);
            q = NULL;
            break;
        }
        p = q;
        q = q->next;
    } while (q != NULL);

    return p;
} /* linklist delete_this_node(linklist llist, pnode this_pnode) */

/*
 * return 0 if no find; else return 1.
 */
pnode search_node(linklist llist, const char *ipaddr)
{
    pnode q = llist->next;

    if (is_nulllist_link(llist))
        return NULL;

    do {
        if (!strcmp(q->node_info.ipaddr, ipaddr))
            return q;

        q = q->next;
    } while (q != NULL);

    return NULL;
} /* int search_node(linklist llist, const char *ipaddr) */

int num_node(linklist llist)
{
    int n = 0;
    while (llist->next) {
        n++;
        llist = llist->next;
    }
    return n;
} /* int num_node(linklist llist) */

/*
 * return 0: fail
 * return 1: succeed
 */
int insert_nodulp_node(linklist llist, const char *ipaddr)
{
    int ret = 0;
    if (search_node(llist, ipaddr) == NULL) {
        insert_link(llist, ipaddr);
        ret = 1;
    }

    return ret;
} /* linklist insert_nodulp_node(linklist llist, const char *ipaddr) */

/*
 * return 0: empty linklist
 */
int free_linklist(linklist llist)
{
    if (llist->next == NULL)
        return 0;

    pnode q = llist->next->next; 
    do {
        free(llist->next);
        llist->next = q;
        q = q->next;
    } while (q != NULL);
    return 1;
} /* int free_linklist(linklist llist) */

