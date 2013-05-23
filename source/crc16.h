#ifndef __CRC16_H__
#define __CRC16_H__
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include "common.h"

/*********************************************************************************************
Function Name:  crc16_append
Description  :  Append data to crc_16
Inputs       :  const void* data            : the buffer of handle
                int32_t len                 : the len of the buffer
Outputs      :  int                         : 0 for ok
ErrorCodes   :
History      :
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
yuancy                 2012-10-22                    create
**********************************************************************************************/
extern bool crc16_append(uint16_t* crc16, const void* data, int32_t len);

#ifdef __cplusplus
}
#endif
#endif /* #ifndef __CRC16_H__ */
