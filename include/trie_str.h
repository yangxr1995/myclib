#pragma once

#include <alloca.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>

#include "arr.h"
#include "assert.h"
#include "common.h"
#include "assert.h"

typedef struct trie_node_s trie_node_t;

struct trie_node_s {
    char *key;
    void *value;
    arr_t *child;  // trie_node_t *
};

void trie_print(trie_node_t *root);

inline static trie_node_t *
trie_new()
{
    trie_node_t *root;

    root = NEW(root);
    root->key = NULL;
    root->value = NULL;
    root->child = arr_new(2, sizeof(trie_node_t *));

    return root;
}
static void
trie_destory(trie_node_t *root)
{
    trie_node_t **ppos, *pos;

    arr_for_each(root->child, ppos) {
        pos = *ppos;
        FREE((*ppos)->key);
        trie_destory(pos);
    }
    arr_destroy(root->child);
    FREE(root);
}

static void domain_arr_free(arr_t *arr)
{
    char **pp;
    arr_for_each(arr, pp) {
        FREE(*pp);
    }
    arr_destroy(arr);
}

static arr_t *domain_arr_new(const char *domain)
{
    arr_t *arr;
    char *buf = NULL;

    assert(domain);
    buf = strdup(domain);
    arr = arr_new(4, sizeof(char *));
    char *p1, *p2, **pp;

    for (p1 = buf; (p2 = strchr(p1, '.')) != NULL; p1 = p2 + 1) {
        *p2 = '\0';
        pp = (char **)arr_push(arr);
        *pp = strdup(p1);
    }
    pp = (char **)arr_push(arr);
    *pp = strdup(p1);

    FREE(buf);

    return arr;
}

void *trie_insert_arr(trie_node_t *root, void *value, arr_t *arr);

inline static void *
trie_insert_domain(trie_node_t *tr, void *val, const char *domain)
{
    arr_t *arr;
    void *old;

    arr = domain_arr_new(domain);
    old = trie_insert_arr(tr, val, arr);
    domain_arr_free(arr);
    return old;
}

void *_trie_remove(trie_node_t *root, arr_t *domain);

inline static void *
trie_remove_domain(trie_node_t *root, const char *domain)
{
    assert(root);
    va_list ap;
    void *ret;
    arr_t *arr;

    arr = domain_arr_new(domain);
    ret = _trie_remove(root, arr);
    domain_arr_free(arr);

    return ret;
}

void *_trie_get(trie_node_t *root, arr_t *domain, void *);

inline static void *
trie_get_domain(trie_node_t *root, const char *domain)
{
    assert(root != NULL);
    arr_t *arr;
    void *ret;
    arr = domain_arr_new(domain);
    ret = _trie_get(root, arr, NULL);
    domain_arr_free(arr);

    return ret;
}

void trie_map(trie_node_t *root, void (*call)(void **pval, void *cl), void *cl);

