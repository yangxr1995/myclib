#ifndef __TRIE_H__
#define __TRIE_H__

#include "arr.h"
#include "assert.h"
#include <arpa/inet.h>

typedef struct trie_node_s trie_node_t;

struct trie_node_s {
    unsigned int key;
    void *value;
    arr_t *child;  // trie_node_t *
};

trie_node_t *trie_new();
void trie_free(trie_node_t *root);
void *trie_insert(trie_node_t *root, void *value, int key_nb, ...);
void *trie_del(trie_node_t *root, int key_nb, ...);
void trie_print(trie_node_t *root);
void *trie_get(trie_node_t *root, int key_nb, ...);
void *trie_remove(trie_node_t *root, int key_nb, ...);
void *trie_get_max_match(trie_node_t *root, int key_nb, ...);

inline static void *
trie_get_max_match_host(trie_node_t *tr, unsigned int addr)
{
    unsigned char *p = (unsigned char *)&addr;
    return trie_get_max_match(tr, 4, p[0], p[1], p[2], p[3]);
}

inline static  void *
trie_insert_net(trie_node_t *tr, void *val, 
        unsigned int addr, unsigned int mask)
{
    unsigned char *p = (unsigned char *)&addr;
    unsigned int nb = mask / 8;

    if (nb == 2)
        return trie_insert(tr, val, 2, p[0], p[1]);
    else if (nb == 3)
        return trie_insert(tr, val, 3, p[0], p[1], p[2]);
    else if (nb == 4)
        return trie_insert(tr, val, 4, p[0], p[1], p[2], p[3]);
    else if (nb == 1)
        return trie_insert(tr, val, 1, p[0]);
    else
        assert(nb != 0);

    return NULL;
}

inline static  void *
trie_remove_net(trie_node_t *tr, unsigned int addr, unsigned int mask)
{
    unsigned char *p = (unsigned char *)&addr;
    unsigned int nb = mask / 8;

    if (nb == 2)
        return trie_remove(tr, 2, p[0], p[1]);
    else if (nb == 3)
        return trie_remove(tr, 3, p[0], p[1], p[2]);
    else if (nb == 4)
        return trie_remove(tr, 4, p[0], p[1], p[2], p[3]);
    else if (nb == 1)
        return trie_remove(tr, 1, p[0]);
    else
        assert(nb != 0);

    return NULL;
}

inline static void * 
trie_insert_net_h(trie_node_t *tr, void *val, 
        const char *ip, unsigned int mask)
{
    return trie_insert_net(tr, val, inet_addr(ip), mask);
}


#endif
