
#ifndef __TOOLS_H__
#define __TOOLS_H__

#include <stdio.h>
#include <stdlib.h>

#define PRESS_ENTER \
    do \
    { \
        fprintf(stdout, "Press enter to continue...\n"); \
        getchar(); \
        CLEAR_SCREEN; \
    } while(0)
#ifdef DEBUG
    #define debug(format, ...) \
    fprintf(stderr, "File: %s, Function: %s, Line: %d\n" format \
        , __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
    #define debug_block(some_code) \
        do \
        { \
            printf("------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n"); \
            debug(""); \
            some_code \
            printf("------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n"); \
            PRESS_ENTER; \
        } while (0);
        
#else
    #define debug(format, ...)
    #define debug_block(some_code) 
#endif
#define CLEAR_SCREEN \
    system("clear")
/*haults the program when any error occurs. on enter press it continues*/
#define ERROR_PRESS_ENTER(format, ...) \
    do \
    { \
        fprintf(stderr, "Error: " format "\n", ##__VA_ARGS__); \
        PRESS_ENTER; \
    } while(0)
#define CALLOC(size, type) \
    (type*)calloc(size, sizeof(type))
#define MALLOC(num_byte, type) \
    (type*)custom_malloc(num_byte)

extern void* custom_calloc(size_t num_el, size_t size_el);
extern void* custom_malloc(size_t num_byte);


#endif/*__TOOLS_H__*/