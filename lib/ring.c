#include <stdio.h>

#include "ring.h"

void ring_test()
{
    ring_t *r;
    int *pos, offset;

    r = ring_new(10, sizeof(int));

    for (int i = 0; i < 10; i++) 
        r = ring_push_h(r, &i);


    printf("init:\n");
    ring_for_each_idx(r, pos, offset) {
        printf("%d ", *pos);
    }
    printf("\n");

   while (1) {

        printf("del:\n");
        ring_for_each_idx(r, pos, offset) {
            if (*pos == 3)
                break;
        }

        printf("\n");

        printf("offset = %d\n", offset);

        ring_tighten(r, offset + 1);

        ring_for_each_idx(r, pos, offset) {
            printf("%d ", *pos);
        }
        printf("\n");

        for (int i = 0; i < 10; i++) 
            r = ring_push_h(r, &i);


        printf("add:\n");
        ring_for_each_idx(r, pos, offset) {
            printf("%d ", *pos);
        }
        printf("\n");


        getchar();
    }

}

