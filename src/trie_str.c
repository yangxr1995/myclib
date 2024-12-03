#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "assert.h"
#include "arr.h"
#include "common.h"
#include "logger.h"
#include "trie.h"

void 
_trie_print(trie_node_t *root, char *prefix, int nb)
{
    trie_node_t **pos;
    char str[256] = {0}, item[32];

    ++nb;

    sprintf(item, "%s ", root->key);
    if (prefix)
        strcat(str, prefix);
    strcat(str, item);

    if (root->value != NULL)
        printf("%s child[%d]\n", str, root->child->nelts);

    arr_for_each(root->child, pos) {
        _trie_print(*pos, str, nb);
    }
}

void 
trie_print(trie_node_t *root)
{
    trie_node_t **pos;
    int nb = 0;

    arr_for_each(root->child, pos) {
        _trie_print(*pos, NULL, nb);
        printf("\n--------\n");
    }
}

void *
_trie_get(trie_node_t *root, arr_t *domain, void *wildcard_value)
{
    char *key = NULL;
    void *ret = NULL;

    assert(domain->nelts > 0);
    key = *(char **)arr_pop(domain);

    trie_node_t **pos;

    arr_for_each(root->child, pos) {
        if (strcmp((*pos)->key, key) == 0) {
            if (domain->nelts == 0) {
                ret = (*pos)->value;
                goto _ret;
            }
            else {
                ret = _trie_get(*pos, domain, wildcard_value);
                if (ret == NULL && wildcard_value == NULL)
                    continue;
                goto _ret;
            }
        }
        else if (strcmp((*pos)->key, "*") == 0) {
            wildcard_value = (*pos)->value;
        }
    }

_ret:
    FREE(key);
    if (ret == NULL)
        ret = wildcard_value;

    return ret;
}

void *
_trie_remove(trie_node_t *root, arr_t *domain)
{
    trie_node_t **ppos, *pos, *del;
    const char *key;
    void *ret;

    key = *(char **)arr_pop(domain);

    /*log_debug("key[%s]", key);*/

    del = NULL;
    ret = NULL;
    arr_for_each(root->child, ppos) {
        pos = *ppos;
        if (pos->key != NULL && strcmp(pos->key, key) == 0) {
            del = pos;
            if (domain->nelts == 0) {
                ret = pos->value;
            }
            else {
                ret = _trie_remove(pos, domain);
            }
            break;
        }
        else {
            /*log_debug("pos->key[%s] != key[%s]",*/
            /*        pos->key == NULL ? "null" : pos->key, key);*/
        }
    }

    if (del) {

        if (domain->nelts == 0)
            del->value = NULL;

        if (del->child->nelts == 0 && del->value == NULL) {
            arr_del(root->child, &del, (int (*)(void *, void *))strcmp);
            arr_destroy(del->child);
            free(del);
        }

    }

    return ret;
}

/*
 * 失败: 返回非NULL，不修改原有值
 * 成功: 返回NULL
 */
void *
trie_insert_arr(trie_node_t *root, void *value, arr_t *arr)
{
    char *key;
    void *old = NULL;
    trie_node_t **pos, *next_step = NULL, **pnext_step;

    key = *(char **)arr_pop(arr);
    arr_for_each(root->child, pos) {
        if (strcmp((*pos)->key, key) == 0) {
            FREE(key);
            next_step = *pos;
            break;
        }
    }

    if (next_step == NULL) {
        next_step = trie_new();
        next_step->key = key;
        pnext_step = (trie_node_t **)arr_push(root->child);
        *pnext_step = next_step;
    }

    void **pval;

    if (arr->nelts == 0) {
        old = next_step->value;
        if (old == NULL)
            next_step->value = value;
        goto ret;
    }

    return trie_insert_arr(next_step, value, arr);

ret:
    return old;
}

void 
trie_map(trie_node_t *root, 
        void (*call)(void **pval, void *cl), void *cl)
{
    trie_node_t **pp, *p;

    arr_for_each(root->child, pp) {
        p = *pp;
        if (p->value)
            call(&p->value, cl);
        trie_map(p, call, cl);
    }
}

typedef struct {
    char *name;
    char *ip;
} domain_t;

void node_print(void **pval, void *cl)
{
    domain_t *d = *pval;
    printf("%s:%s\n", d->name, d->ip);
}

int trie_test()
{
    trie_node_t *root;

    domain_t domains[] = {
        {
            .name = "www.aaa.com",
            .ip = "192.168.3.1",
        },
        {
            .name = "www.bbb.com",
            .ip = "192.168.3.2",
        },
        {
            .name = "*.ccc.com",
            .ip = "192.168.3.3",
        },
        {
            .name = "www.ddd.com",
            .ip = "192.168.3.4",
        },
        {
            .name = "*.ddd.com",
            .ip = "192.168.3.5",
        },
        {
            .name = "aaa.www.ddd.com",
            .ip = "192.168.3.6",
        },
    };

    root = trie_new();

    domain_t *old;
    for (int i = 0; i < sizeof(domains)/ sizeof(*domains); ++i) {
        if ((old = trie_insert_domain(root, domains + i, domains[i].name)) != NULL) {
            printf("重复 domain[%s]\n", old->name);
        }
    }

    domain_t *d;

    d = (domain_t *)trie_get_domain(root, "www.ddd.com");
    if (d == NULL) {
        trie_destory(root);
        printf("not find \n");
        return 0;
    }
    printf("%s : %s\n", d->name, d->ip);
    /*trie_print(root);*/

    trie_map(root, node_print, NULL);

    trie_destory(root);

    return 0;
}

