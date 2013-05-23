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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <errno.h>

#include "mq_errno.h"

const char* mq_strerror(int32_t error_number)
{
    return strerror(abs(error_number));
}

int32_t mq_errno(void)
{
    int32_t tmp = errno;
    return (tmp <= 0) ? tmp : -tmp;
}

int32_t set_errno(int32_t error_number)
{
    int32_t tmp = errno;
    errno = (error_number <= 0) ? error_number : -error_number;
    return tmp;
}

const char* mq_last_error(void)
{
    int32_t tmp = errno;
    return strerror((tmp >= 0) ? tmp : -tmp);
}

