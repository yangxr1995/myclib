#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "memchk.h"
#include "table.h"

typedef struct {
	const char *name;
	int chinese;
	int math;
	int english;
	int average;
} stu_t;

static int stu_cmp(const void *x, const void *y);
static unsigned int stu_hash(const char *key);
static void calculate_average(const char *key, void **value, void *cl);

int main()
{
	stu_t s1 = {
		.name = "aaa",
		.chinese = 89,
		.math = 77,
		.english = 90,
		.average = 0
	};
	stu_t s2 = {
		.name = "bbb",
		.chinese = 99,
		.math = 78,
		.english = 80,
		.average = 0
	};
	stu_t s3 = {
		.name = "ccc",
		.chinese = 80,
		.math = 97,
		.english = 80, 
		.average = 0
	};
	stu_t s4 = {
		.name = "ddd",
		.chinese = 99,
		.math = 89,
		.english = 79, 
		.average = 0
	};

	table_t score_table;

	score_table = table_new(50, &stu_cmp, &stu_hash);

	table_put(score_table, s1.name, &s1);
	table_put(score_table, s2.name, &s2);
	table_put(score_table, s3.name, &s3);
	table_put(score_table, s4.name, &s4);
	
	table_map(score_table, &calculate_average, NULL);

	void **arr, **stu_ptr;
	int count = 0;
	arr = table_to_array(score_table, NULL);

	for (stu_ptr = arr; *stu_ptr; stu_ptr += 2) {
		printf("name : %s, average : %d\n", 
				(const char *)(stu_ptr[0]), ((stu_t *)stu_ptr[1])->average);
		count += 2;
		assert(count < 20);	
	}

	table_free(&score_table);
	free(arr);
	mem_leak();

	return 0;
}

static int stu_cmp(const void *x, const void *y)
{
	return strcmp(x, y);
}

static unsigned int stu_hash(const char *key)
{
	unsigned int sum;
	for (sum = 0; *key; key++) {
		sum += *key;
	}	
	return sum;
}

static void calculate_average(const char *key, void **value, void *cl)
{
	stu_t *stu;

	stu = (stu_t *)*value;
	stu->average = (stu->chinese + stu->english + stu->math)/3;

	return ;
}
