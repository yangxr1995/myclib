#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

//#include "memchk.h"

typedef struct entry_s{
	struct entry_s *next;
	const void *key;
	void *value;
} entry_t;

typedef struct table_s {
	int size;
	int length;
	unsigned int timestamp;
	int (*cmp)(const void *x, const void *y);
	unsigned int (*hash)(const void *key);
	entry_t **bucket;
} *table_t;

table_t table_new(int hint, int (*cmp)(const void *x, const void *y), unsigned int (*hash)(const void *key))
{
	table_t tb;
	unsigned int primes[] = {
		509, 1021, 2053, 4093, 8191, 16381, 32771, 65521, INT_MAX
	};
	unsigned int i;

	assert(cmp != NULL);
	assert(hash != NULL);
	assert(hint >= 0);

	for (i = 1; primes[i] < hint; i++)
		NULL;
	tb = (table_t) malloc( sizeof(*tb) + sizeof(tb->bucket[0]) * primes[i-1]);
	tb->size = primes[i-1];
	tb->cmp = cmp;
	tb->hash = hash;
	tb->length = 0;
	tb->timestamp = 0;
	tb->bucket = (entry_t **)(tb + 1);
	for (i = 0; i< tb->size; i++) {
		tb->bucket[i] = NULL;
	}

	return tb;
}

void table_free(table_t *ptb)
{
	entry_t *entry, *tmp;
	unsigned int i;
	table_t tb;

	assert(ptb && *ptb);
	tb = *ptb;
	printf("Free table, size : %d, length : %d\n ", tb->size, tb->length);
	for (i = 0; i < tb->size; i++) {
		for (entry = tb->bucket[i]; entry; entry = tmp) {
			tmp = entry->next;
			free(entry);
		}
	}
	free(tb);
	*ptb = NULL;
}

#define entry_find(_key) do{	\
	idx = (*tb->hash)(_key)%tb->size;	\
	for (entry = tb->bucket[idx]; entry; entry = entry->next) {	\
		if ((*tb->cmp)(entry->key, _key) == 0) { \
			break;	\
		}	\
	}	\
} while(0)

void *table_put(table_t tb, const void *key, void *value)
{
	unsigned int idx;
	entry_t *entry;
	void *prev;

	assert(tb);
	assert(key);
	entry_find(key);
	if (entry) {
		prev = entry->value;
	}
	else {
		entry = (entry_t *)malloc(sizeof(entry_t));	
		entry->key = key;
		entry->next = tb->bucket[idx];
		tb->bucket[idx] = entry;
		tb->length++;
		prev = NULL;
		entry->value = value;
	}
//	entry->value = value;
	tb->timestamp++;
	printf("Put entry into table, size : %d, idx : %d, length : %d\n ", tb->size, idx, tb->length);
	return prev;
}

void *table_get(table_t tb, const void *key)
{
	unsigned int idx;
	entry_t *entry;

	assert(tb);
	assert(key);
	entry_find(key);
	printf("Get entry from table, idx : %d, length : %d\n ", idx, tb->length);
	return entry ? entry->value : NULL;
}
int table_length(table_t tb)
{
	assert(tb);
	return tb->length;
}
void *table_remove(table_t tb, const char *key)
{
	unsigned int idx;
	entry_t *entry, *prev, **pp;
	void *ret;

	assert(tb);
	assert(key);

	tb->timestamp++;
	idx = (*tb->hash)(key)%tb->size;
	printf("Remove entry from table, idx : %d, length : %d\n ", idx, tb->length);
	entry = tb->bucket[idx];

	for (pp = tb->bucket + idx; *pp; pp = &(*pp)->next) {
		if ((*tb->cmp)((*pp)->key, key) == 0) {
			entry_t *p = *pp;
			void *value = p->value;
			*pp = p->next;
			free(p);
			tb->length--;
			return value;
		}
	}
	return NULL;
}

void table_map(table_t tb, void (*apply)(const char *key, void **value, void *cl), void *cl)
{
	entry_t *entry;
	unsigned int i, stamp;

	assert(tb);
	assert(apply);
	
	stamp = tb->timestamp;
	for (i = 0; i < tb->size; i++) {
		for (entry = tb->bucket[i]; entry; entry = entry->next) {
			apply(entry->key, &entry->value, cl);
			assert(tb->timestamp == stamp);
		}
	}
//	printf("Map entry into table, size : %d, length : %d\n ", tb->size, tb->length);
}
void **table_to_array(table_t tb, void *end)
{
	void **arr;
	unsigned int idx, j, nb;
	entry_t *entry;

	unsigned int count = 0;

	assert(tb);
//	printf("Get array from table, length : %d\n ", tb->length);

	nb = tb->length * 2 + 1;
	arr = (void **)malloc(sizeof(*arr) * (tb->length * 2 + 1));
	j = 0;
	for (idx = 0; idx < tb->size; idx++) {
		for (entry = tb->bucket[idx]; entry; entry = entry->next) {
			arr[j++] = (void *)entry->key;
			arr[j++] = entry->value;
		}
	}
	arr[j] = end;

	return arr;
}
