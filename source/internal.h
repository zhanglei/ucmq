#ifndef __INTERNAL_H__
#define __INTERNAL_H__
#ifdef __cplusplus
extern "C"
{
#endif

#include <errno.h>
#include <assert.h>
#include <stdbool.h>

#include "mq_errno.h"
#include "common.h"

/******************************************************************************************************/

/* set new error number
   return the prev error number */
extern int32_t set_errno(int32_t error_number);

/******************************************************************************************************/

/* use to avoid unused variable warning */
#define UNUSED(x)        do{/* null */}while((uintptr_t)&x == 0x01);

/* print macro value */
#define PRINT_FUNC(x)    #x

#define PRINT_MACRO(x)   PRINT_FUNC(x)

/* get array count */
#define ARR_SIZE(x)      (sizeof(x)/sizeof(x[0]))

/******************************************************************************************************/

#ifdef __cplusplus
}
#endif
#endif /* #ifndef __INTERNAL_H__ */ 
