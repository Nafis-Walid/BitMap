#include "bit-map.h"
#include "error.h"

#define U16_NUM_DIGITS 5U
#define U16_MAX UINT16_MAX
#define U32_MAX UINT32_MAX
#define BIT_SIZE_OF(a) (sizeof(a) * sizeof(u8) * 8U)
#define CHAR_NULL '\0'
#define CHAR_COMMA ','
#define NUMERICS "0123456789"
#define LEFT_SHIFT(a, b) ((a) << (b))
#define RIGHT_SHIFT(a, b) ((a) >> (b))
#define DIV_BY_32(a) RIGHT_SHIFT(a, 5U)
#define MULT_BY_32(a) LEFT_SHIFT(a, 5U)
#define MOD_32(a) ((a) & 0x1FU)
#define MASK(val) LEFT_SHIFT(1U, MOD_32((val) - 1U))
#define BLOCK_INDEX(val) DIV_BY_32((val) - 1U)
#define BIT_INDEX(val) (MOD_32((val) - 1U) + 1U)
#define OUT_RANGE(start, val, end) ((val) < (start) || ((val) > (end)))
#define MAX(a, b) (a) > (b) ? (a) : (b)
#define MIN(a, b) (a) < (b) ? (a) : (b)
#define RIGHT_MOST_BIT(a) ((a) & 1U)
#define LEFT_MOST_BIT(a) ((a) & 0x80000000U)
#define PRINT_NEW_LINE printf("\n")

static bool bitmap_check(struct bitmap *bm);
static u16 str_to_nat16(char* str);
static range_t find_range(char* str);
static u8 count_ones(u32 n);
static u32 range_mask(u32 start, u32 end);
static bool is_valid_range(range_t range);
static void first_update(struct bitmap* bm, u16 min_index_hint, u16 max_index_hint);
static void last_update(struct bitmap* bm, u16 min_index_hint, u16 max_index_hint);
static void bitmap_reset_padding(struct bitmap* bm);
static void bitmap_add_range(struct bitmap* bm, range_t range);

struct bitmap* bitmap_create(u16 capacity)
{
    struct bitmap *bm = NULL;
    u16 buf_len = 0;
    
    if(capacity == 0)
        return NULL;

    buf_len = BLOCK_INDEX(capacity) + 1;
    bm = (struct bitmap*)calloc(sizeof(struct bitmap) + buf_len * sizeof(u32), 1);

    if (bm == NULL)
        return NULL;

    bm->bm_self = bm;
    bm->max_value = capacity;
    bm->first_value = bm->last_value = bm->numbers = 0;
    bm->buf_len = buf_len;
    
    return bm;
}

static bool bitmap_check(struct bitmap *bm)
{
    struct bitmap* clone_bm = NULL;
    u16 i = 0;

    if(bm == NULL || bm != bm->bm_self || bm->buf_len == 0)
        return false;

    clone_bm = bitmap_clone(bm);
    first_update(clone_bm, 0, clone_bm->buf_len - 1);
    last_update(clone_bm, 0, clone_bm->buf_len - 1);
    bm->numbers = 0;
    bitmap_reset_padding(clone_bm);
    
    for(i = 0; i < clone_bm->buf_len; i++)
        bm->numbers += count_ones(clone_bm->buf[i]);

    if(clone_bm->first_value != bm->first_value || clone_bm->last_value != bm->last_value || clone_bm->numbers != bm->numbers)
    {
        bitmap_destroy(clone_bm);
        return false;
    }
        
    bitmap_destroy(clone_bm);

    return true;
}

void bitmap_destroy(struct bitmap *bm)
{
    if (bm != NULL && bm == bm->bm_self)
        free(bm);
    else
        HALT_AND_CONTINUE("bitmap not freed!");

    return;
}

struct bitmap* bitmap_clone(struct bitmap *bm)
{
    struct bitmap *clone = NULL;
    
    if(bm == NULL || bm != bm->bm_self)
    {
        HALT_AND_CONTINUE("Couldn't clone bitmap");
        return NULL;
    }
       
    clone = bitmap_create(bm->max_value);

    if (clone != NULL)
    {
        memcpy(clone->buf, bm->buf, bm->buf_len * sizeof(u32));
        clone->first_value = bm->first_value;
        clone->last_value = bm->last_value;
        clone->numbers = bm->numbers;
        clone->buf_len = bm->buf_len;
    }
    
    return clone;
}

bool bitmap_add_value(struct bitmap *bm, u16 value)
{
    u16 index = 0;
    u32 mask = 0;

    if (bitmap_check(bm) == false)
    {
        HALT_AND_CONTINUE("The bitmap does not exist or, not a valid bitmap");
        return false;
    }
        

    if(OUT_RANGE(1, value, bm->max_value))
    {
        HALT_AND_CONTINUE("The value: %d is out of range\n", value);
        return false;
    }
    
    index = BLOCK_INDEX(value);
    mask = MASK(value);

    if (bm->buf[index] & mask)
        return true;

    bm->buf[index] |= mask;
    bm->numbers++;
    bm->first_value = bm->first_value == 0 ? value : MIN(value, bm->first_value);
    bm->last_value = MAX(value, bm->last_value);

    return true;
}

static void bitmap_add_range(struct bitmap* bm, range_t range)
{
    u16 start_index = 0;
    u16 end_index = 0;
    u32 mask = 0;
    u16 split = 0;

    if(range.start < 1 || range.end > bm->max_value)
        return;
    
    if(range.start == range.end)
    {
        bitmap_add_value(bm, range.start);
        return;
    }

    start_index = BLOCK_INDEX(range.start);
    end_index = BLOCK_INDEX(range.end);

    if(start_index == end_index)
    {
        mask = range_mask(BIT_INDEX(range.start), BIT_INDEX(range.end));
        bm->buf[start_index] |= mask;
        bm->numbers += (range.end - range.start + 1);
        bm->first_value = bm->first_value == 0 ? range.start : MIN(range.start, bm->first_value);
        bm->last_value = MAX(range.end, bm->last_value);
        return;
    }

    split = MULT_BY_32(start_index + 1);
    bitmap_add_range(bm, (range_t){range.start, split});
    bitmap_add_range(bm, (range_t){split + 1, range.end});

    return;
}

bool bitmap_del_value(struct bitmap *bm, u16 value_to_delete)
{
    u16 index = 0;
    u32 mask = 0;

    if (bitmap_check(bm) == false || OUT_RANGE(1, value_to_delete, bm->max_value))
        return false;

    index = BLOCK_INDEX(value_to_delete);
    mask = MASK(value_to_delete);

    if ((bm->buf[index] & mask) == 0)/*already deleted*/
        return true;

    bm->buf[index] &= ~mask;
    bm->numbers--;

    if(value_to_delete > bm->first_value && value_to_delete < bm->last_value)/*value_to_delete number is in between first and last exclusive*/
        return true;
    
    if(value_to_delete == bm->first_value && value_to_delete == bm->last_value)/*last value is value_to_delete*/
    {
        bm->first_value = bm->last_value = 0;
        return true;
    }

    if(value_to_delete == bm->first_value)/*update first*/
    {
        first_update(bm, BLOCK_INDEX(bm->first_value), BLOCK_INDEX(bm->last_value));
        return true;
    }

    if(value_to_delete == bm->last_value)/*update last*/
    {
        last_update(bm, BLOCK_INDEX(bm->first_value), BLOCK_INDEX(bm->last_value));
        return true;
    }
    
    return false;
}

/*to reset the padding bits of a bitmap*/
static void bitmap_reset_padding(struct bitmap* bm)
{
    u32 mask = 0;
    u16 i = 0;

    i = MOD_32(bm->max_value);
    mask = i == 0 ? 0 : range_mask(i + 1, BIT_SIZE_OF(u32));
    bm->buf[bm->buf_len - 1] &= ~mask;

    return;
}

bool bitmap_not(struct bitmap *bm)
{
    u16 i = 0;

    if (bitmap_check(bm) == false)
        return false;

    for (i = 0; i < bm->buf_len; i++)
        bm->buf[i] = ~bm->buf[i];

    bitmap_reset_padding(bm);
    bm->numbers = bm->max_value - bm->numbers;

    if(bm->numbers == 0)
    {
        bm->first_value = bm->last_value = 0;
        return true;
    }

    first_update(bm, 0, bm->buf_len - 1);
    last_update(bm, 0, bm->buf_len - 1);

    return true;
}

bool bitmap_or(struct bitmap *bm_store, struct bitmap *bm)
{
    u16 i = 0;
    u16 start_block = 0;
    u16 end_block = 0;

    if (bitmap_check(bm_store) == false || bitmap_check(bm) == false)
        return false;
    
    start_block = MIN(BLOCK_INDEX(bm_store->first_value), BLOCK_INDEX(bm->first_value));
    end_block = MIN(BLOCK_INDEX(bm_store->max_value), BLOCK_INDEX(bm->last_value));
    bm_store->numbers = 0;

    for (i = start_block; i <= end_block; i++)
    {
        bm_store->buf[i] |= bm->buf[i];
        bm_store->numbers += count_ones(bm_store->buf[i]);
    }

    bitmap_reset_padding(bm_store);
    end_block = bm_store->buf_len;

    while(i < end_block)
        bm_store->numbers += count_ones(bm_store->buf[i++]);        
    
    first_update(bm_store, 0, BLOCK_INDEX(bm_store->first_value));
    last_update(bm_store, BLOCK_INDEX(bm_store->last_value), bm_store->buf_len - 1);

    return true;
}

bool bitmap_and(struct bitmap *bm_store, struct bitmap *bm)
{
    u16 i = 0;
    u16 start_block = 0;
    u16 end_block = 0;

    if (bitmap_check(bm_store) == false || bitmap_check(bm) == false)
        return false;

    start_block = MIN(BLOCK_INDEX(bm_store->first_value), BLOCK_INDEX(bm->first_value));
    end_block = MIN(BLOCK_INDEX(bm_store->max_value), BLOCK_INDEX(bm->max_value));
    bm_store->numbers = 0;

    for (i = start_block; i <= end_block; i++)
    {
        bm_store->buf[i] &= bm->buf[i];
        bm_store->numbers += count_ones(bm_store->buf[i]);
    }

    bitmap_reset_padding(bm_store);
    end_block = bm_store->buf_len;

    while(i < end_block)
        bm_store->buf[i++] = 0;

    first_update(bm_store, MAX(BLOCK_INDEX(bm_store->first_value), BLOCK_INDEX(bm->first_value)), bm_store->buf_len - 1);
    last_update(bm_store, 0, MIN(BLOCK_INDEX(bm_store->last_value), BLOCK_INDEX(bm->last_value)));
        
    return true;
}

void bitmap_print(struct bitmap *bm)
{
    u16 i = 0;
    u8 j = 0;
    bool first_print_flag = false;
    u16 bit_index = 0;
    u16 range_begin = 0;
    u16 range_end = 0;
    u16 end_index = 0;
    u32 temp_block = 0;

    if (!bitmap_check(bm))
    {
        printf("(nil)\n");
        return;
    }

    if(bm->numbers == 0 || bm->first_value == 0 || bm->last_value == 0)
    {
        printf("(empty bitmap)\n");
        return;
    }

    end_index = BLOCK_INDEX(bm->last_value);
    i = BLOCK_INDEX(bm->first_value);
    bit_index = MULT_BY_32(i) + 1;
    first_print_flag = true;

    for(; i <= end_index; i++)
    {
        
        temp_block = bm->buf[i];
        
        if(temp_block == 0)/*if the whole block is 0 filled, no neet to iterate every bits*/
        {
            bit_index += BIT_SIZE_OF(u32);

            if(range_begin && range_end)
            {
                if(range_begin == range_end)
                    printf((first_print_flag ? "%u" : ",%u"), range_begin);
                else
                    printf((first_print_flag ? "%u-%u" : ",%u-%u"), range_begin, range_end);
                
                first_print_flag = false;
                range_begin = range_end = 0;
            }

            continue;
        }

        if(temp_block == U32_MAX)/*if the whole block is 1 filled, no neet to iterate every bits*/
        {
            if(range_begin == 0)
                range_begin = bit_index;
            
            range_end = (bit_index += BIT_SIZE_OF(u32) - 1);
            bit_index++;
            continue;
        }

        j = 0;
        while(j++ < BIT_SIZE_OF(u32))
        {
            
            if(RIGHT_MOST_BIT(temp_block))
            {
                range_end = bit_index;

                if(range_begin == 0)
                    range_begin = range_end;
            }
            else
            {
                if(range_begin == 0 && range_end == 0)
                {
                    bit_index++;
                    temp_block = RIGHT_SHIFT(temp_block, 1);
                    continue;
                }

                if(range_begin == range_end)
                    printf((first_print_flag == true ? "%u" : ",%u"), range_begin);
                else
                    printf((first_print_flag == true ? "%u-%u" : ",%u-%u"), range_begin, range_end);

                first_print_flag = false;
                range_begin = range_end = 0;
            }

            bit_index++;
            temp_block = RIGHT_SHIFT(temp_block, 1);
        }
    }

    if(range_begin && range_end)
    {
        if(range_begin == range_end)
            printf((first_print_flag ? "%u" : ",%u"), range_begin);
        else
            printf((first_print_flag ? "%u-%u" : ",%u-%u"), range_begin, range_end);
        
        first_print_flag = false;
        range_begin = range_end = 0;
    }

    PRINT_NEW_LINE;

    return;
}

struct bitmap* bitmap_parse_str(char *str)
{
    struct bitmap* bm = NULL;
    char* tmp = NULL;
    char* str_wrk_cpy = NULL;
    char* remaining_str = NULL;
    range_t range = {0};
    range_t prev_range = {0};
    
    str_wrk_cpy = strdup(str);/*malloc allocated*/
    tmp = strrchr(str_wrk_cpy, CHAR_COMMA);
    range = find_range(tmp == NULL ? str_wrk_cpy : tmp + 1);

    if(is_valid_range(range) == false)
    {
        free(str_wrk_cpy);
        return NULL;
    }

    bm = bitmap_create(range.end);

    if(bm == NULL)
    {
        free(str_wrk_cpy);
        return NULL;
    }

    remaining_str = str_wrk_cpy;
    tmp = strchr(remaining_str, CHAR_COMMA);

    while(tmp != NULL)
    {
        *tmp = CHAR_NULL;
        range = find_range(remaining_str);

        if(is_valid_range(range) == false || range.start <= prev_range.end)
        {
            free(str_wrk_cpy);
            free(bm);
            return NULL;
        }

        bitmap_add_range(bm, range);
        remaining_str = tmp + 1;
        tmp = strchr(remaining_str, CHAR_COMMA);
        prev_range = range;
    }
    
    range = find_range(remaining_str);

    if(is_valid_range(range) == false || range.start <= prev_range.end)
    {
        free(bm);
        free(str_wrk_cpy);
        return NULL;
    }

    bitmap_add_range(bm, range);

    return bm;
}

static u8 count_ones(u32 n) 
{
    u8 count = 0;

    while(n)
    {
        n &= (n - 1);
        count++;
    }

    return count;
}

static range_t find_range(char* str)
{
    char* tmp = NULL;
    char* str_wrk_cpy = NULL;
    range_t range = {0};

    if(str == NULL)
        return range;
    
    str_wrk_cpy = strdup(str);

    if((tmp = strchr(str_wrk_cpy, '-')) == NULL || *str_wrk_cpy == '-' || str_wrk_cpy[strlen(str_wrk_cpy) - 1] == '-')
    {
        if((range.start = str_to_nat16(str_wrk_cpy)) == 0)
        {
            free(str_wrk_cpy);
            return (range_t){0};
        }
        
        free(str_wrk_cpy);
        range.end = range.start;
        return range;
    }
    
    *tmp = CHAR_NULL;
    tmp++;

    if((range.start = str_to_nat16(str_wrk_cpy)) == 0 || (range.end = str_to_nat16(tmp)) == 0 || range.end < range.start)
    {
        free(str_wrk_cpy);
        return (range_t){0};
    }
        
    free(str_wrk_cpy);

    return range;
}

/********************************************************************************************************************
 * Function Name:       str_to_nat16
 * Input:               a string
 * Output:              natural  number:    if valid number(only digits are allowed, no heading spaces)
 *                      0:                  if invalid
 * Description          converts a string to 16 bit natural number(1, 2, 3, ..., 65535)
 ********************************************************************************************************************/
static u16 str_to_nat16(char* str)
{
    u32 num = 0;
    u32 len = 0;

    len = strlen(str);

    if(str == NULL || *str == CHAR_NULL || len > U16_NUM_DIGITS || strspn(str, NUMERICS) != len)
        return 0;

    num = strtoul(str, NULL, 10);

    if(num > U16_MAX)
        return 0;

    return num;
}

static u32 range_mask(u32 start, u32 end)
{
    if ((start - 1U) >= BIT_SIZE_OF(u32) || (end - 1U) >= BIT_SIZE_OF(u32) || start > end)
        return 0;
    
    return (u32)LEFT_SHIFT((LEFT_SHIFT(1UL, (end - start + 1UL)) - 1UL), (start - 1));
}

static void first_update(struct bitmap* bm, u16 min_index_hint, u16 max_index_hint)
{
    u32 temp_block = 0;
    u16 i = 0;
    u16 j = 0;

    for(i = min_index_hint; i < max_index_hint && bm->buf[i] == 0; i++);
    temp_block = bm->buf[i];

    if(temp_block == 0)
    {
        bm->first_value = 0;
        return;
    }

    j = 1;

    while(RIGHT_MOST_BIT(temp_block) == 0)/*as temp block is non zero, at least once this operation will be true*/
    {
        temp_block = RIGHT_SHIFT(temp_block, 1);
        j++;
    }

    bm->first_value = MULT_BY_32(i) + j;
    
    return;
}

static void last_update(struct bitmap* bm, u16 min_index_hint, u16 max_index_hint)
{
    u32 temp_block = 0;
    u16 i = 0;
    u16 j = 0;

    for(i = max_index_hint; i > min_index_hint && bm->buf[i] == 0; i--);

    temp_block = bm->buf[i];
    if(temp_block == 0)
    {
        bm->last_value = 0;
        return;
    }
    j = 32;

    while(LEFT_MOST_BIT(temp_block) == 0)
    {
        temp_block = LEFT_SHIFT(temp_block, 1);
        j--;
    }

    bm->last_value = MULT_BY_32(i) + j;

    return;
}

static bool is_valid_range(range_t range)
{
    if(range.start == 0 || range.end == 0 || range.start > range.end)
        return false;

    return true;
}
