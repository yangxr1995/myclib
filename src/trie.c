#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "assert.h"
#include "arr.h"
#include "trie.h"

//#include "memchk.h"

trie_node_t *
trie_new()
{
    trie_node_t *root;

    root = malloc(sizeof(*root));
    root->key = 0;
    root->value = NULL;
    root->child = arr_create(10, sizeof(trie_node_t *));

    return root;
}

void *
_trie_insert(trie_node_t *root, void *value, int key_nb, va_list *ap)
{
    unsigned int key;
    void *old = NULL;

    assert(key_nb > 0);

    key_nb--;
    key = va_arg(*ap, unsigned int);

    trie_node_t **pos, *next_step = NULL, **pnext_step;

    arr_for_each(root->child, pos) {
        if ((*pos)->key == key) {
            next_step = *pos;
            break;
        }
    }

    if (next_step == NULL) {
        next_step = trie_new();
        next_step->key = key;
        pnext_step = arr_push(root->child);
        *pnext_step = next_step;
    }

    void **pval;

    if (key_nb == 0) {
        old = next_step->value;
        next_step->value = value;
        goto ret;
    }

    return _trie_insert(next_step, value, key_nb, ap);

ret:
    return old;
}

void *
trie_insert(trie_node_t *root, void *value, int key_nb, ...)
{
    assert(root != NULL);
    assert(key_nb > 0);

    void *ret;
    va_list ap;

    va_start(ap, key_nb);

    ret = _trie_insert(root, value, key_nb, &ap);

    va_end(ap);

    return ret;
}

void
trie_delete(trie_node_t *root)
{
    trie_node_t **ppos, *pos;

    arr_for_each(root->child, ppos) {
        pos = *ppos;
        trie_delete(pos);
    }
    arr_destroy(root->child);
    free(root);
}

void 
_trie_print(trie_node_t *root, char *prefix, int nb)
{
    trie_node_t **pos;
    char str[256] = {0}, item[32];

    ++nb;

    sprintf(item, "%d ", root->key);
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
_trie_get(trie_node_t *root, int key_nb, va_list *ap) 
{
    unsigned int key;

    assert(key_nb > 0);

    key = va_arg(*ap, unsigned int);
    --key_nb;

    trie_node_t **pos;

    arr_for_each(root->child, pos) {
        if ((*pos)->key == key) {
            if (key_nb == 0) {
                return (*pos)->value;
                /*assert((*pos)->values->nelts > 0);*/
                /*return ((void **)((*pos)->values->elts))[0];*/
            }
            else {
                return _trie_get(*pos, key_nb, ap);
            }
        }
    }

    return NULL;
}

void *
_trie_get_max_match(trie_node_t *root, int key_nb, va_list *ap) 
{
    unsigned int key;
    void *ret = NULL, *tmp;

    assert(key_nb > 0);

    key = va_arg(*ap, unsigned int);
    --key_nb;

    trie_node_t **ppos, *pos;

    arr_for_each(root->child, ppos) {
        pos = *ppos;
        if (pos->key == key) {

            if (pos->value != NULL) {
                ret = pos->value;
            }

            if (key_nb == 0) {
                return pos->value;
            }
            else {
                if ((tmp = _trie_get_max_match(pos, key_nb, ap)) != NULL) {
                    ret = tmp;
                }
            }

            break;
        }
    }

    return ret;
}

void *
trie_get_max_match(trie_node_t *root, int key_nb, ...)
{
    assert(root != NULL);
    assert(key_nb > 0);

    va_list ap;
    void  *ret;

    va_start(ap, key_nb);

    ret = _trie_get_max_match(root, key_nb, &ap);

    va_end(ap);

    return ret;
}

void *
trie_get(trie_node_t *root, int key_nb, ...)
{
    assert(root != NULL);
    assert(key_nb > 0);

    va_list ap;
    void  *ret;

    va_start(ap, key_nb);

    ret = _trie_get(root, key_nb, &ap);

    va_end(ap);

    return ret;
}

static int
cmp_addr(void *pa, void *pb)
{
    void *a = *(void **)pa;
    void *b = *(void **)pb;

    return a == b ? 0 : 1;
}

static void *
_trie_remove(trie_node_t *root, int key_nb, va_list *ap)
{
    trie_node_t **ppos, *pos, *del;
    unsigned int key;
    void *ret;

    --key_nb;
    key = va_arg(*ap, unsigned int);

    del = NULL;
    ret = NULL;
    arr_for_each(root->child, ppos) {
        pos = *ppos;
        if (pos->key == key) {
            del = pos;
            if (key_nb == 0)
                ret = pos->value;
            else
                ret = _trie_remove(pos, key_nb, ap);

            break;
        }
    }

    if (del) {

        if (key_nb == 0)
            del->value = NULL;

        if (del->child->nelts == 0 && del->value == NULL) {
            arr_del(root->child, &del, cmp_addr);
            arr_destroy(del->child);
            free(del);
        }

    }

    return ret;
}

void *
trie_remove(trie_node_t *root, int key_nb, ...)
{
    assert(root);
    assert(key_nb > 0);

    va_list ap;
    void *ret;
    
    va_start(ap, key_nb);

    ret = _trie_remove(root, key_nb, &ap);

    va_end(ap);

    return ret;
}





typedef struct stu_s {
  char *name;
  int age;
} stu_t;

int trie_test(int argc, char *argv[])
{
    trie_node_t *tr;
    stu_t *pos;
    stu_t stu[] = {
      {
        .name = "aaa",
        .age = 11,
      },
      {
        .name = "bbb",
        .age = 22,
      },
      {
        .name = "ccc",
        .age = 33,
      },
      {
          .name = "ddd",
          .age = 44,
      },
      {
          .name = "default",
          .age = 0,
      }
    };

    tr = trie_new();

    unsigned int addr;

    trie_insert_net_h(tr, stu, "192.168.3.33", 32);

    trie_insert_net_h(tr, stu + 1 , "192.168.3.34", 32);

    trie_insert_net_h(tr, stu + 2 , "192.168.4.1", 24);

    stu_t *pstu;
    pstu = trie_get_max_match_host(tr, inet_addr(argv[1]));
    if (pstu)
        printf("name %s age %d\n", pstu->name, pstu->age);
    else
        printf("can't find\n");

    /*if (trie_insert(tr, &stu[0], 4, 1, 2, 3, 4)) {*/
    /*    printf("冲突\n");*/
    /*}*/
    /**/
    /*if (trie_insert(tr, &stu[1], 4, 1, 2, 3, 4)) {*/
    /*    printf("冲突\n");*/
    /*}*/
    /*trie_insert(tr, &stu[1], 3, 1, 2, 3);*/
    /*trie_insert(tr, &stu[2], 4, 1, 3, 3, 5);*/
    /**/
    /*trie_print(tr);*/
    /**/
    /*pos = trie_get(tr , 4, 1, 2, 3, 4);*/
    /*printf("%s %d\n", pos->name, pos->age);*/
    /**/
    /*pos = trie_remove(tr, 4, 1, 2, 3, 4);*/
    /*if (pos)*/
    /*    printf("%s %d\n", pos->name, pos->age);*/
    /*pos = trie_remove(tr, 3, 1, 2, 3);*/
    /*if (pos)*/
    /*    printf("%s %d\n", pos->name, pos->age);*/
    /*pos = trie_remove(tr, 4, 1, 3, 3, 5);*/
    /*if (pos)*/
    /*    printf("%s %d\n", pos->name, pos->age);*/
    /**/
    /*printf("-----------\n");*/
    /*trie_print(tr);*/
    /**/
    /*trie_insert(tr, &stu[1], 3, 192, 168, 3);*/
    /*trie_insert(tr, &stu[0], 4, 192, 168, 3, 5);*/
    /*trie_insert(tr, &stu[2], 3, 192, 168, 4);*/
    /**/
    /*pos = trie_get_max_match(tr, 4, 192, 168, 3, 6); */
    /*if (pos)*/
    /*    printf("192.168.3.5 : %s %d\n", pos->name, pos->age);*/
    /**/
    /*pos = trie_get_max_match(tr, 4, 192, 168, 4, 5); */
    /*if (pos)*/
    /*    printf("192.168.4.5 : %s %d\n", pos->name, pos->age);*/
    /**/
    /*trie_delete(tr);*/
    /**/
    /*stu_t *pstu;*/
    /*pstu = (stu_t *)trie_get(tr , 4, 1, 2, 3, 4);*/
    /*if (pstu)*/
    /*    log_info("%s %d", pstu->name, pstu->age);*/
    /**/
    /*pstu = (stu_t *)trie_get(tr , 3, 1, 2, 3);*/
    /*if (pstu)*/
    /*    log_info("%s %d", pstu->name, pstu->age);*/
    /**/
    /*pstu = (stu_t *)trie_get(tr , 4, 1, 2, 3, 5);*/
    /*if (pstu)*/
    /*    log_info("%s %d", pstu->name, pstu->age);*/
    /**/
    /*pstu = (stu_t *)trie_get(tr , 4, 1, 3, 3, 5);*/
    /*if (pstu)*/
    /*    log_info("%s %d", pstu->name, pstu->age);*/
    /**/


    return 0;
}


