#ifndef __ERROR_H__
#define __ERROR_H__

#define HALT_AND_CONTINUE(fmt, ...) \
    do { \
        printf(fmt, ##__VA_ARGS__); \
        printf("\nPress Enter to continue..."); \
        while (getchar() != '\n'); \
    } while (0)


#endif /*__ERROR_H__*/