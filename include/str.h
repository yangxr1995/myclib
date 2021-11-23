#ifndef STR_H

#define STR_H

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

#endif /* end of include guard: STR_H */
