#ifndef __BITMAP_H_
#define __BITMAP_H_

#include <stdio.h>
#include <stdint.h>

typedef struct bitmap_s bitmap_t;
struct bitmap_s{
    uint32_t *bitmap;
    uint32_t size;
};


void bitmap_alloc(bitmap_t *bmp, uint32_t size);
void bitmap_init(bitmap_t *bmp);
void bitmap_set(bitmap_t *bmp, uint32_t pos);
int bitmap_get(bitmap_t *bmp, uint32_t pos);
void bitmap_clear(bitmap_t *bmp, uint32_t pos);
void bitmap_free(bitmap_t *bmp);

#endif
