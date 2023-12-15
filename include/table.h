#ifndef __TABLE_H
#define __TABLE_H

typedef struct table_s *table_t;

extern table_t table_new(int hint, int (*cmp)(const void *x, const void *y), unsigned int (*hash)(const void *key));
extern void table_free(table_t *table);
extern void *table_put(table_t table, const void *key, void *value);
extern void *table_get(table_t table, const void *key);
extern int table_length(table_t table);
extern void *table_remove(table_t table, const char *key);
extern void table_map(table_t table, void (*apply)(const char *key, void **value, void *cl), void *cl);
extern void **table_to_array(table_t table, void *end);


#endif
