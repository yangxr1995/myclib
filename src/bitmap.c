#include <stdlib.h>

#include "bitmap.h"


void bitmap_init(bitmap_t *bmp, uint32_t size) 
{
    bmp->size = (size + 31) / 32;
    bmp->bitmap = (uint32_t *)calloc(bmp->size, sizeof(uint32_t)); 
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
    free(bmp->bitmap);
}

int main()
{
	return 0;
}
#if 0
int main()
{
	bitmap_t bmp;
	int sum;

	sum = 4;
	bitmap_init(&bmp, sum);
	bitmap_set(&bmp, 0);
	bitmap_set(&bmp, 1);

	for (int i = 0; i < sum; i++) {
		printf("Bit at %d : %d\n", i, bitmap_get(&bmp, i));
	}
	bitmap_free(&bmp);
	return 0;
}
#endif




