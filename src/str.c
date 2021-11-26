#include <string.h>

#include "str.h"
#include "assert.h"

int 
str_find(str_t *str, int i, int j, str_t *astr)
{
	assert(str && str->data && str->len > 0);
	assert(astr && astr->data && astr->len > 0);
	int t;
	if (i > j) {
		t = j;	
		j = i;
		i = t;
	}
	assert(i < j);	
	int rc;
	for (; i < j; i++) {
		rc = memcmp(str->data + i, astr->data, astr->len);
		if (rc < 0) {
			return -1;	
		}
		if (rc == 0) {
			return i;
		}
	}
	return -1;
}
