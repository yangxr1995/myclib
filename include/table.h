#ifndef __TABLE_H
#define __TABLE_H

/*
 * 表：键值对为元素的容器
 * 键和值可以为任何类型，
 * 容器本身可以用 哈希表 或 红黑树 组织 元素关系
 * 键是表的索引，所以通常 键固定，值固定，也就是
 * 若表中有键值对 aa:abc, 再加入新的键值对 aa:ddd,
 * 原来的值应该被覆盖，但也可以实现为 新的值记录
 * 在原来的值后面(栈逻辑).
 */

typedef struct table_s *table_t;

/*
 * @hint:提示本表可能需要容纳多少格键值对，用于
 * 		优化性能
 * @cmp:键比较接口，返回逻辑和 strcmp 一样
 * @hash:键哈希接口
 */
extern table_t table_new(int hint, int (*cmp)(const void *x, const void *y), unsigned int (*hash)(const char *key));
extern void table_free(table_t *table);
extern void *table_put(table_t table, const void *key, void *value);
extern void *table_get(table_t table, const void *key);
extern int table_length(table_t table);
extern void *table_remove(table_t table, const char *key);
/*
 * 对表中所有键值对使用 apply 方法
 * @apply:方法接口
 * @cl:apply参数
 */
extern void table_map(table_t table, void (*apply)(const char *key, void **value, void *cl), void *cl);
/*
 * 将表中键值对以数组方式返回
 * 键和值的指针紧密排列
 */
extern void **table_to_array(table_t table, void *end);


#endif
