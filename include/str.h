#ifndef STR_H

#define STR_H

#include <stdlib.h>
#include <string.h>

typedef struct str_s str_t;
struct str_s {
	int len;
	char *data;
};

#define str_new(str) { sizeof(str) - 1, (char *	)str }
#define str_null { 0, NULL }
#define str_set_text(str, text) \
	(str)->len = sizeof(text) - 1; (str)->data = text
#define str_set_null(str) \
	(str)->len = 0; (str)->data = NULL

#define const_str_len(text) (sizeof(text) - sizeof((text)[0]))

#define str_set_ptr(str, ptr) \
	(str)->len = strlen(ptr); (str)->data = (char *)ptr
#define str_set_comm(str, _data, _len) \
	(str)->len = _len; (str)->data = (char *)_data

extern int str_find(str_t *str, int i, int j, str_t *astr);

inline static void str_dup(str_t *str, char *ptr, int len)
{
	str->data = malloc(len);
	memcpy(str->data, ptr, len);
	str->len = len;
}

#define str_adup(str) ({ \
		str_t _ret; \
		_ret.data = alloca(str->len); \
		_ret.len = str->len; \
		memcpy(_ret.data, str->data, _ret.len); \
		_ret; \
		})

inline static int str_cmp(str_t *s1, str_t *s2)
{
	if (s1->len > s2->len) {
		return 1;	
	}
	else if (s1->len < s2->len) {
		return -1;	
	}
	else {
		return strncmp(s1->data, s2->data, s1->len);
	}
}
#endif /* end of include guard: STR_H */
