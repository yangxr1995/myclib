#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "memchk.h"

typedef struct entry_s{
	struct entry_s *next;
	const void *key;
	void *value;
} entry_t;

typedef struct table_s {
	int size;
	int length;
	int (*cmp)(const void *x, const void *y);
	unsigned int (*hash)(const char *key);
	entry_t **bucket;
} *table_t;

table_t table_new(int hint, int (*cmp)(const void *x, const void *y), unsigned int (*hash)(const char *key))
{
	unsigned int primes[] = {
		13,33,63,123,253,513,1023,2027
	};
	unsigned int i;

	assert(cmp != NULL);
	assert(hash != NULL);

	for (i = 0; i < sizeof(primes)/sizeof(*primes) - 1; i++) {
		if (hint < primes[i])
			break;
	}

	table_t tb;

	tb = (table_t) malloc(sizeof(*tb));
	tb->size = primes[i];
	tb->cmp = cmp;
	tb->hash = hash;
	tb->length = 0;
	tb->bucket = (entry_t **)malloc(tb->size * sizeof(entry_t *));
	memset(tb->bucket, 0x0, tb->size * sizeof(entry_t *));

	printf("New table, size : %d\n ", tb->size);

	return tb;
}

void table_free(table_t *ptb)
{
	entry_t *entry, *tmp;
	unsigned int i;
	table_t tb;

	tb = *ptb;

	printf("Free table, size : %d, length : %d\n ", tb->size, tb->length);

	for (i = 0; i < tb->size; i++) {
		for (entry = tb->bucket[i]; entry; entry = tmp) {
			tmp = entry->next;
			free(entry);
		}
	}
	free(tb->bucket);
	free(tb);
	*ptb = NULL;
}

#define entry_find(_key) do{	\
	idx = tb->hash(_key)%tb->size;	\
	for (entry = tb->bucket[idx]; entry; entry = entry->next) {	\
		if (tb->cmp(entry->key, _key) == 0) {	\
			break;	\
		}	\
	}	\
} while(0)

void *table_put(table_t tb, const void *key, void *value)
{
	unsigned int idx;
	entry_t *entry;

	entry_find(key);
	if (entry) {
		entry->value = value;
	}
	else {
		entry = (entry_t *)malloc(sizeof(entry_t));	
		entry->key = key;
		entry->value = value;
		entry->next = tb->bucket[idx];
		tb->bucket[idx] = entry;
		tb->length++;
	}

	printf("Put entry into table, size : %d, idx : %d, length : %d\n ", tb->size, idx, tb->length);

	return entry->value;
}
void *table_get(table_t tb, const void *key)
{
	unsigned int idx;
	entry_t *entry;

	entry_find(key);

	printf("Get entry from table, idx : %d, length : %d\n ", idx, tb->length);

	if (entry) {
		return entry->value;
	}
	else {
		return NULL;
	}
}
int table_length(table_t tb)
{
	return tb->length;
}
void *table_remove(table_t tb, const char *key)
{
	unsigned int idx;
	entry_t *entry, *prev;
	void *ret;

	idx = tb->hash(key)%tb->size;

	printf("Remove entry from table, idx : %d, length : %d\n ", idx, tb->length);

	entry = tb->bucket[idx];
	if (tb->cmp(key, entry->key) == 0) {
		ret = entry->value;
		tb->bucket[idx] = entry->next;
		tb->length--;
		free(entry);
		return ret;
	}

	for (prev = tb->bucket[idx], entry = prev->next; entry; prev = entry, entry = entry->next) {
		if (tb->cmp(key, entry->key) == 0) {
			ret = entry->value;
			prev->next = entry->next;
			tb->length--;
			free(entry);
			return ret;
		}
	}

	return NULL;
}
void table_map(table_t tb, void (*apply)(const char *key, void **value, void *cl), void *cl)
{
	entry_t *entry;
	unsigned int i;


	for (i = 0; i < tb->size; i++) {
		for (entry = tb->bucket[i]; entry; entry = entry->next) {
			apply(entry->key, &entry->value, cl);
		}
	}
	printf("Map entry into table, size : %d, length : %d\n ", tb->size, tb->length);
}
void **table_to_array(table_t tb, void *end)
{
	void **arr;
	unsigned int idx, j, nb;
	entry_t *entry;

	unsigned int count = 0;

	printf("Get array from table, length : %d\n ", tb->length);

	nb = tb->length * 2 + 1;
	arr = (void **)malloc(sizeof(void *) * nb);
	arr[nb - 1] = end;
	for (idx = 0, j = 0; idx < tb->size; idx++) {
		for (entry = tb->bucket[idx]; entry; entry = entry->next) {
			count++;
			arr[j] = (void *)entry->key;
			arr[j + 1] = entry->value;
			j += 2;
		}
	}

	return arr;
}
