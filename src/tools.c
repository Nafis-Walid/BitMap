#include "tools.h"
#include <stdlib.h>

void* custom_calloc(size_t num_el, size_t size_el)
{
    void* ptr = NULL;

    ptr = calloc(num_el, size_el);
    if(!ptr)
    {
        ERROR_PRESS_ENTER("Memory Couldn't be allocated!\n");
        return NULL;
    }

    return ptr;
}

void* custom_malloc(size_t num_byte)
{
    void* ptr = NULL;

    ptr = malloc(num_byte);
    if(!ptr)
    {
        ERROR_PRESS_ENTER("Memory Couldn't be allocated!\n");
        return NULL;
    }

    return ptr;
}