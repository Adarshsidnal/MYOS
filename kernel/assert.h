#include "defs.h"

#define GDN_ASSERT(condition, message) \
    do                                 \
    {                                  \
        if (!(condition))              \
            panic(message);            \
    } while (0)
