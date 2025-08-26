#ifndef __BIT_MAP_H__
#define __BIT_MAP_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

struct bitmap 
{
    struct bitmap *bm_self;
    u16 max_value;
    u16 first_value;
    u16 last_value;
    u16 numbers;
    u16 buf_len;
    u32 buf[0];
};
typedef struct
{
    u16 start;
    u16 end;
}range_t;

extern struct bitmap* bitmap_create(u16 capacity);
extern void bitmap_destroy(struct bitmap *bm);
extern struct bitmap* bitmap_clone(struct bitmap *bm);
extern bool bitmap_add_value(struct bitmap *bm, u16 value);
extern bool bitmap_del_value(struct bitmap *bm, u16 value);
extern bool bitmap_not(struct bitmap *bm);
extern bool bitmap_or(struct bitmap *bm_store, struct bitmap *bm);
extern bool bitmap_and(struct bitmap *bm_store, struct bitmap *bm);
extern void bitmap_print(struct bitmap *bm);
extern struct bitmap* bitmap_parse_str(char *str);

#endif/*__BIT_MAP_H__*/