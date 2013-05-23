/*
*  Copyright (c) 2013 UCWeb Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
* You may obtain a copy of the License at
*
*       http://www.gnu.org/licenses/gpl-2.0.html
*
* Email: osucmq@ucweb.com
* Author: ShaneYuan
*/

#ifndef __COMMON_H__
#define __COMMON_H__
#ifdef cplusplus
extern "C"
{
#endif

#include <stdint.h>

/******************************************************************************************************/

enum OPT
{
    INVALID = 0,         /* use for avoid/detect some errors */
    ON      = 1,         /* turn on some options */
    OFF     = 2,         /* turn off some options */
};

/******************************************************************************************************/

/* gcc version. for example : v4.1.2 is 40102, v3.4.6 is 30406 */
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

/******************************************************************************************************/

#ifdef cplusplus
}
#endif
#endif /* #ifndef __COMMON_H__ */

