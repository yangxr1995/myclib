#include <stdio.h>

typedef struct hash_point_s{
	struct hash_point_s *next;
	void *key;
	void *value;
} hash_point_t;

typedef struct table_s {
	int size;
	int length;
	int (*cmp)(const void *x, const void *y);
	unsigned int (*hash)(const char *key);
	hash_point_t **bucket;
} *table_t;

table_t table_new(int hint, int (*cmp)(const void *x, const void *y), unsigned int (*hash)(const char *key))
{
	return NULL;
}

void table_free(table_t *table)
{

}
void *table_put(table_t table, const void *key, void *value)
{

}
void *table_get(table_t table, const void *key)
{
	return NULL;
}
int table_length(table_t table)
{
	return 0;
}
void *table_remove(table_t table, const char *key)
{
	return NULL;
}
void table_map(table_t table, void (*apply)(const char *key, void **value, void *cl), void *cl)
{

}
void **table_to_array(table_t table, void *end)
{
	return NULL;
}
