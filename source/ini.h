#ifndef __INI_H__
#define __INI_H__
#ifdef __cplusplus
extern "C"
{
#endif

#include "common.h"

typedef struct ini ini_t;

/*********************************************************************************************/
/*********************************************************************************************
Function Name:  ini_load
Description  :  load ini file and return ini
Inputs       :  const char* file        :   file path
Outputs      :  return value            :   the pointer of the ini
ErrorCodes   :
History      :
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
zhanggp                2011-06-21                    create
**********************************************************************************************/
extern ini_t* ini_load(const char* file);

/*********************************************************************************************
Function Name:  ini_find
Description  :  find name from ini
Inputs       :  ini_t* ini              : The ini pointer
                char* section                   : The section string
                char* key                       : The key string
                int idx                       : The position of key(start from 0)
Outputs      :  return value                    : The find value string
ErrorCodes   :
History      :
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
zhanggp                2011-06-21                    create
**********************************************************************************************/
extern const char* ini_find(const ini_t* ini,
                               const char* section,
                               const char* key,
                               int idx);

/*********************************************************************************************
Function Name:  ini_count
Description  :  get value count of same section and key
Inputs       :  ini_t* ini              :   The ini pointer
                char* section                   :   The section string
                char* key                       :   The key string
Outputs      :  return value                    :   The count of the key in the section
ErrorCodes   :
History      :
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
zhanggp                2011-06-21                    create
*********************************************************************************************/
extern int32_t ini_count(const ini_t* ini,
                            const char* section,
                            const char* key);

/*********************************************************************************************
Function Name:  ini_clear
Description  :  clear ini
Inputs       :  ini_t* ini              :   The ini pointer
Outputs      :
ErrorCodes   :
History      :
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
zhanggp                2011-06-21                    create
**********************************************************************************************/
extern int ini_clear(ini_t* ini);

/*********************************************************************************************/

#ifdef __cplusplus
}
#endif
#endif /* #ifndef __INI_H__ */
