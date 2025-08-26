#include "src/bit-map.h"
#define PRINT_NEW_LINE printf("\n")

int main()
{
    struct bitmap* bm1 = NULL;
    struct bitmap* bm2 = NULL;

    bm1 = bitmap_parse_str("20,25-75,150");
    bitmap_print(bm1);

    bm2 = bitmap_parse_str("1-40,50,60-80");
    bitmap_print(bm2);

    bitmap_del_value(bm2, 50);
    bitmap_print(bm2);

    bitmap_not(bm1);
    bitmap_print(bm1);

    bitmap_and(bm1, bm2);
    bitmap_print(bm1);

    bitmap_or(bm2, bm1);
    bitmap_print(bm2);

    bitmap_destroy(bm1);
    bitmap_destroy(bm2);

    PRINT_NEW_LINE;

    return 0;
}
