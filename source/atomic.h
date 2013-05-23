#ifndef __ATOMIC_H__
#define __ATOMIC_H__
#ifdef cplusplus
extern "C"
{
#endif

#include "common.h"

/******************************************************************************************************/

#if ((!ATOMIC_NOT_BUILTIN) && (GCC_VERSION >= 40100))

/* gcc built-in functions for atomic memory access */

/* get var's value */
#define ATOMIC_GET(var) var

/* set var to value, no return value */
#define ATOMIC_SET(var, value) (void)__sync_lock_test_and_set(&var, (value))

/* set var to value, and return var's previous value */
#define ATOMIC_SWAP(var, value)  __sync_lock_test_and_set(&var, (value))

/* if current value of var equal comp, then it will be set to value.
   the return value is always the var's previous value */
#define ATOMIC_CAS(var, comp, value) __sync_val_compare_and_swap(&var, (comp), (value))

/* set var to 0(or NULL), no return value */
#define ATOMIC_CLEAR(var) (void)__sync_lock_release(&var)

/* maths/bitop of var by value, and return the var's new value */
#define ATOMIC_ADD(var, value)   __sync_add_and_fetch(&var, (value))
#define ATOMIC_SUB(var, value)   __sync_sub_and_fetch(&var, (value))
#define ATOMIC_OR(var, value)    __sync_or_and_fetch(&var, (value))
#define ATOMIC_AND(var, value)   __sync_and_and_fetch(&var, (value))
#define ATOMIC_XOR(var, value)   __sync_xor_and_fetch(&var, (value))


#else /* GCC_VERSION */


#include <pthread.h>

extern pthread_mutex_t g_atomic_mutex;

#define ATOMIC_USE_LOCK

/* get var's value */
#define ATOMIC_GET(var)                             \
({                                                  \
    typeof(var) ret;                                \
    pthread_mutex_lock(&g_atomic_mutex);            \
    ret = var;                                      \
    pthread_mutex_unlock(&g_atomic_mutex);          \
    ret;                                            \
})

/* set var to value, no return value */
#define ATOMIC_SET(var, value)                      \
({                                                  \
    pthread_mutex_lock(&g_atomic_mutex);            \
    var = (value);                                  \
    pthread_mutex_unlock(&g_atomic_mutex);          \
    0;                                              \
})

/* set var to value, and return var's previous value */
#define ATOMIC_SWAP(var, value)                     \
({                                                  \
    typeof(var) ret;                                \
    pthread_mutex_lock(&g_atomic_mutex);            \
    ret = var;                                      \
    var = (value);                                  \
    pthread_mutex_unlock(&g_atomic_mutex);          \
    ret;                                            \
})

/* if current value of var equal comp, then it will be set to value.
   the return value is always the var's previous value */
#define ATOMIC_CAS(var, comp, value)                \
({                                                  \
    typeof(var) ret;                                \
    pthread_mutex_lock(&g_atomic_mutex);            \
    ret = var;                                      \
    if (var == (comp))                              \
    {                                               \
        var = (value);                              \
    }                                               \
    pthread_mutex_unlock(&g_atomic_mutex);          \
    ret;                                            \
})

/* set var to 0(or NULL), no return value */
#define ATOMIC_CLEAR(var)                           \
({                                                  \
    pthread_mutex_lock(&g_atomic_mutex);            \
    var = 0;                                        \
    pthread_mutex_unlock(&g_atomic_mutex);          \
    0;                                              \
})

/* maths/bitop of var by value, and return the var's new value */
#define ATOMIC_ADD(var, value)                      \
({                                                  \
    typeof(var) ret;                                \
    pthread_mutex_lock(&g_atomic_mutex);            \
    var += (value);                                 \
    ret = var;                                      \
    pthread_mutex_unlock(&g_atomic_mutex);          \
    ret;                                            \
})

#define ATOMIC_SUB(var, value)                      \
({                                                  \
    typeof(var) ret;                                \
    pthread_mutex_lock(&g_atomic_mutex);            \
    var -= (value);                                 \
    ret = var;                                      \
    pthread_mutex_unlock(&g_atomic_mutex);          \
    ret;                                            \
})

#define ATOMIC_OR(var, value)                       \
({                                                  \
    typeof(var) ret;                                \
    pthread_mutex_lock(&g_atomic_mutex);            \
    var |= (value);                                 \
    ret = var;                                      \
    pthread_mutex_unlock(&g_atomic_mutex);          \
    ret;                                            \
})

#define ATOMIC_AND(var, value)                      \
({                                                  \
    typeof(var) ret;                                \
    pthread_mutex_lock(&g_atomic_mutex);            \
    var &= (value);                                 \
    ret = var;                                      \
    pthread_mutex_unlock(&g_atomic_mutex);          \
    ret;                                            \
})

#define ATOMIC_XOR(var, value)                      \
({                                                  \
    typeof(var) ret;                                \
    pthread_mutex_lock(&g_atomic_mutex);            \
    var ^= (value);                                 \
    ret = var;                                      \
    pthread_mutex_unlock(&g_atomic_mutex);          \
    ret;                                            \
})

#endif /* GCC_VERSION */


/* set some bits on(1) of var, and return the var's new value */
#define ATOMIC_BIT_ON(var, mask)     ATOMIC_OR(var, (mask))

/* set some bits off(0) of var, and return the var's new value */
#define ATOMIC_BIT_OFF(var, mask)    ATOMIC_AND(var, ~(mask))

/* exchange some bits(1->0/0->1) of var, and return the var's new value */
#define ATOMIC_BIT_XCHG(var, mask)   ATOMIC_XOR(var, (mask))


/******************************************************************************************************/

#ifdef cplusplus
}
#endif
#endif /* #ifndef __ATOMIC_H__ */
