#include <stdlib.h>

#include "bitmap.h"


void bitmap_alloc(bitmap_t *bmp, uint32_t size) 
{
    bmp->size = (size + 31) / 32;
    bmp->bitmap = (uint32_t *)calloc(bmp->size, sizeof(uint32_t)); 
}

void bitmap_init(bitmap_t *bmp)
{
	bmp->size = 0;
	bmp->bitmap = NULL;
}

void bitmap_set(bitmap_t *bmp, uint32_t pos) 
{
    uint32_t index = pos / 32;
    uint32_t offset = pos % 32;
    bmp->bitmap[index] |= (1 << offset);
}

int bitmap_get(bitmap_t *bmp, uint32_t pos) 
{
    uint32_t index = pos / 32;
    uint32_t offset = pos % 32;
    return (bmp->bitmap[index] & (1 << offset)) != 0;
}

void bitmap_clear(bitmap_t *bmp, uint32_t pos) 
{
    uint32_t index = pos / 32;
    uint32_t offset = pos % 32;
    bmp->bitmap[index] &= ~(1 << offset);
}

void bitmap_free(bitmap_t *bmp) 
{
	if (bmp->bitmap) {
		free(bmp->bitmap);
		bmp->bitmap = NULL;
	}
}

