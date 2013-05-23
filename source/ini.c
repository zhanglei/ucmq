#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "internal.h"
#include "uthash.h"
#include "ini.h"

#define TOKEN_SIZE      1024
#define MAX_TOKENS      1024

typedef struct ini_kv
{
    UT_hash_handle  hh;
    uint16_t        value_count;
    uint16_t        values_size;
    union
    {
        char*       value;
        char**      values;
    };
    union
    {
        uint32_t    line;
        uint32_t*   lines;
    };
    char            key[0];
}ini_kv_t;

struct ini
{
    ini_kv_t*    hash_table;
};

/*********************************************************************************************
Function Name:  add_key_value
Description  :  
Inputs       :  ini_t* ini                 :   
                const char* key            :
                const char* value          :
                int key_len                :
                int value_len              :
                uint32_t line              :

Outputs      :  return value               :
ErrorCodes   :
History      :
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
zhanggp                2011-06-21                    create
**********************************************************************************************/
static inline int add_key_value(ini_t* ini, 
                                const char* key, 
                                const char* value, 
                                int key_len, 
                                int value_len, 
                                uint32_t line);

/*********************************************************************************************
Function Name:  find_key
Description  :  
Inputs       :  ini_t* ini              :   
                const char* key            :
                int idx                    :
                int* count                 :
Outputs      :  return value               :
                int* count                 :
ErrorCodes   :
History      :
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
zhanggp                2011-06-21                    create
**********************************************************************************************/
static inline char* find_key(const ini_t* ini, const char* key, int idx, int* count);

/*********************************************************************************************
Function Name:  trim_str
Description  :  
Inputs       :  char* ptr                  :   
                int trim_end               :
Outputs      :  return value               :
ErrorCodes   :
History      :
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
zhanggp                2011-06-21                    create
**********************************************************************************************/
static inline char* trim_str(char* ptr, int trim_end);

/*********************************************************************************************
Function Name:  find_token
Description  :  
Inputs       :  const ini_t* ini        :   
                char* start                :
                int skip                   :
                int* cur_len               :
                int start_offset           :
                const int max_size         :
                int* tokens                :
                uint32_t line              :
Outputs      :  return value               :
                int* tokens                :
ErrorCodes   :
History      :
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
zhanggp                2011-06-21                    create
**********************************************************************************************/
static char* find_token(const ini_t* ini, 
                        char* start, 
                        int skip, 
                        int* cur_len, 
                        int start_offset, 
                        const int max_size, 
                        int* tokens, 
                        uint32_t line);

/*********************************************************************************************
Function Name:  replace_in_line
Description  :  
Inputs       :  char* str                  :   
                char* buf                  :
                int buf_size               :
                const ini_t* ini        :
                int can_free               :
                uint32_t line              :
Outputs      :  return value               :
ErrorCodes   :
History      :
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
zhanggp                2011-06-21                    create
**********************************************************************************************/
static inline char* replace_in_line(char* str, 
                                    char* buf, 
                                    int buf_size, 
                                    const ini_t* ini, 
                                    int can_free, 
                                    uint32_t line);

/*********************************************************************************************
Function Name:  replace_var
Description  :  
Inputs       :  const ini_t* ini        :   
                char* buf                  :
                int buf_size               :
Outputs      :  return value               :
ErrorCodes   :
History      :
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
zhanggp                2011-06-21                    create
**********************************************************************************************/
static inline int replace_var(const ini_t* ini, char* buf, int buf_size);

/********************************************************************************************/

ini_t* ini_load(const char* file)
{
    char line[4 * TOKEN_SIZE + 1];
    char sec[2 * TOKEN_SIZE + 2];
    char val[TOKEN_SIZE + 1];
    char* key                       = sec;
    char* where                     = NULL;
    FILE* fp_ini                    = NULL;
    char* cur_key                   = NULL;
    char* cur_val                   = NULL;
    ini_t* ini                   = NULL;
    uint32_t line_num               = 0;

    assert(file);

    ini = calloc(1,sizeof(ini_t));
    if(ini == NULL)
    {
        set_errno(MQ_ENOMEM);
        return NULL;
    }

    if((fp_ini = fopen(file, "r")) == NULL)
    {
        free(ini);
        return NULL;
    }

    while(fgets(line, sizeof(line), fp_ini) != NULL)
    {
        ++line_num;
        where = trim_str(line, 0);
        switch(*where)
        {
            case ';':
            case '#':
            case '\0':
                continue;
            case '[':
                if(sscanf(where, "[%"PRINT_MACRO(TOKEN_SIZE)"[^]]", sec) == 1)
                {
                    key = &sec[strlen(sec)];
                    *key = ':';
                    *(++key) = '\0';
                    continue;
                }
            default:
                val[0] = '\0';
    
                /* sscanf return 1 maybe means has a valid key with a empty value */
                if(sscanf(where, "%"PRINT_MACRO(TOKEN_SIZE)"[^=] = %"PRINT_MACRO(TOKEN_SIZE)"[^\r\n]", 
                          key, val) > 0)
                {
                    cur_key = trim_str(sec, 1);
                    cur_val = trim_str(val, 1);
    
                    if(add_key_value(ini, cur_key, cur_val, strlen(cur_key), strlen(cur_val), line_num) != 0)
                    {
                        ini_clear(ini);
                        fclose(fp_ini);
                        return NULL;
                    }
                }
                break;
        }    
    }

    fclose(fp_ini);

    if(replace_var(ini, line, sizeof(line)) != 0)
    {
        ini_clear(ini);
        return NULL;
    }

    return ini;
}

const char* ini_find(const ini_t* ini, const char* section, const char* key, int idx)
{
    int count                   = 0;
    char mykey[2 * TOKEN_SIZE + 1];
    const char* cur_key         = mykey;

    assert(ini);
    assert(section);
    assert(key);
    assert(idx >= 0);

    if(*section != '\0')
    {
        snprintf(mykey, sizeof(mykey), "%s:%s", section, key);
        mykey[sizeof(mykey) - 1] = '\0';
    }
    else
    {
        cur_key = key;
    }

    return find_key(ini, cur_key, idx, &count);
}

int32_t ini_count(const ini_t* ini, const char* section, const char* key)
{
    int count               = 0;
    char mykey[2 * TOKEN_SIZE + 1];
    const char* cur_key     = mykey;

    assert(ini);
    assert(section);
    assert(key);

    if(*section != '\0')
    {
        snprintf(mykey, sizeof(mykey), "%s:%s",section,key);
        mykey[sizeof(mykey) - 1] = '\0';
    }
    else
    {
        cur_key = key;
    }

    find_key(ini, cur_key, 0, &count);

    return count;
}

int ini_clear(ini_t* ini)
{
    ini_kv_t* cur = NULL;
    ini_kv_t* tmp = NULL;
    uint32_t i       = 0;

    assert(ini);

    HASH_ITER(hh, ini->hash_table, cur, tmp)
    {
        HASH_DEL(ini->hash_table, cur);

        switch(cur->values_size)
        {
	        case 0:
	            break;
	        case 1:
	            free(cur->value);
	            break;
	        default:
	            for(i = ((cur->values_size & 0x01) != 0) ? 0 : 1; i < cur->value_count; i++)
	            {
	                free(cur->values[i]);
	            }
	            free(cur->lines);
	            free(cur->values);
	            break;
        }

        free(cur);
    }

    free(ini);

    return 0;
}

/********************************************************************************************/

static inline int add_key_value(ini_t* ini, 
                                const char* key, 
                                const char* value, 
                                int key_len, 
                                int value_len, 
                                uint32_t line)
{
    char* p         = NULL;
    char** pp       = NULL;
    uint32_t l      = 0;
    uint32_t* ll    = NULL;
    ini_kv_t* kv = NULL;

    HASH_FIND_STR(ini->hash_table, key, kv);
    if(kv == NULL)
    {
        kv = malloc(sizeof(ini_kv_t) + key_len + value_len + 2);
        if(kv == NULL)
        {
            set_errno(MQ_ENOMEM);
            return MQ_ENOMEM;
        }
        
        memset(kv, 0x00, sizeof(ini_kv_t));
        memcpy(kv->key, key, key_len);
        kv->key[key_len] = '\0';
        kv->value = &kv->key[key_len + 1];
        memcpy(kv->value, value, value_len);
        kv->value[value_len] = '\0';
        kv->line = line;

        kv->values_size = 0;
        kv->value_count = 1;

        HASH_ADD_STR(ini->hash_table, key, kv);
    }
    else
    {
        if(kv->values_size == 0)
        {
            p = kv->value;
            kv->values_size = 8;
            kv->values = malloc(sizeof(char*) * kv->values_size);
            if(kv->values == NULL)
            {
                kv->values_size = 0;
                set_errno(MQ_ENOMEM);
                return MQ_ENOMEM;
            }
            kv->values[0] = p;

            l = kv->line;
            kv->lines = malloc(sizeof(uint32_t) * kv->values_size);
            if(kv->lines == NULL)
            {
                set_errno(MQ_ENOMEM);
                return MQ_ENOMEM;
            }
            kv->lines[0] = l;
        }

        if(kv->values_size <= kv->value_count)
        {
            pp = kv->values;
            kv->values = malloc(sizeof(char*) * kv->values_size * 2);
            if(kv->values == NULL)
            {
                set_errno(MQ_ENOMEM);
                return MQ_ENOMEM;
            }
            
            memcpy(kv->values, pp, sizeof(char*) * kv->values_size);
            free(pp);

            ll = kv->lines;
            kv->lines = malloc(sizeof(uint32_t) * kv->values_size * 2);
            if(kv->lines == NULL)
            {
                set_errno(MQ_ENOMEM);
                return MQ_ENOMEM;
            }
            memcpy(kv->lines, ll, sizeof(uint32_t) * kv->values_size);
            free(ll);

            kv->values_size <<= 1;
        }

        kv->values[kv->value_count] = malloc(value_len + 1);
        if(kv->values[kv->value_count] == NULL)
        {
            set_errno(MQ_ENOMEM);
            return MQ_ENOMEM;
        }
        memcpy(kv->values[kv->value_count], value, value_len);
        kv->values[kv->value_count][value_len] = '\0';
        kv->lines[kv->value_count] = line;
        kv->value_count += 1;
    }

    return 0;
}

static inline char* find_key(const ini_t* ini, const char* key, int idx, int* count)
{
    ini_kv_t* kv = NULL;

    HASH_FIND_STR(ini->hash_table, key, kv);
    if(kv == NULL)
    {
        *count = 0;
        return NULL;
    }
    *count = kv->value_count;
    if(idx >= kv->value_count)
    {
        return NULL;
    }

    return(kv->values_size <= 1) ? kv->value : kv->values[idx];
}

static inline char* trim_str(char* ptr, int trim_end)
{
    char* start = NULL;
    char* end   = NULL;

    if(ptr[0] == '\0')
    {
        return ptr;
    }
    
    start = ptr;
    end = &ptr[strlen(ptr) - 1];
    for(; isspace(*start); start++)
    {
    }
    if(trim_end == 1)
    {
        for(; isspace(*end); end--)
        {
        }
        *(end + 1) = '\0';
    }

    return start;
}

static char* find_token(const ini_t* ini, 
                        char* start, 
                        int skip, 
                        int* cur_len, 
                        int start_offset, 
                        const int max_size, 
                        int* tokens, 
                        uint32_t line)
{
    UNUSED(line);

    char* p              = start + skip;
    int count            = -1;
    const char* find_str = NULL;
    int old_len;
    int new_len;
    int move_len;

    if(--(*tokens) <= 0)
    {
        set_errno(MQ_ELOOP);
#ifdef DEBUG
        fprintf(stderr, "find too many tokens : %d at line : %d \n", MAX_TOKENS, line);
#endif
        return NULL;
    }

    while(*p != '\0')
    {
        switch(*p)
        {
	        case '$':
	            if(*(p + 1) == '(')
	            {
	                p = find_token(ini, p, 2, cur_len, start_offset +(p - start), 
                                   max_size, tokens, line);
	                if(p == NULL)
	                {
	                    return NULL;
	                }
	                else
	                {
	                    continue;
	                }
	            }
	            break;
	        case ')':
	            if(skip != 0)
	            {
	                *p = '\0';
	                find_str = find_key(ini, start + skip, 0, &count);
	                if((count != 1) || (find_str == NULL))
	                {
	                    set_errno(MQ_ENOKEY);
	#ifdef DEBUG
	                    fprintf(stderr, "can't find the variable : %s at line %d \n", 
                                start + skip, line);
	#endif
	                    return NULL;
	                }
	
	                old_len = (p - start) + 1;
	                new_len = strlen(find_str);
	                move_len = *cur_len - old_len - start_offset;
	
	                *cur_len += (new_len - old_len);
	                if(*cur_len >= max_size)
	                {
	                    set_errno(MQ_EOVERFLOW);
	#ifdef DEBUG
	                    fprintf(stderr, "the buffer size reach the max : %d at line : %d \n", 
                                max_size, line);
	#endif
	                    return NULL;
	                }
	
	                memmove(start + new_len, p + 1, move_len);
	                memcpy(start, find_str, new_len);
	                start[new_len + move_len] = '\0';
	                return start;
	            }
	            break;
	        default:
	            break;
        }
        p++;
    }

    return p;
}

static inline char* replace_in_line(char* str, 
                                    char* buf, 
                                    int buf_size, 
                                    const ini_t* ini, 
                                    int can_free, 
                                    uint32_t line)
{
    UNUSED(line);

    int old_len = strlen(str);
    int cur_len = old_len;
    int offset;
    char* p = strstr(str, "$(");

    if(p != NULL)
    {
        memcpy(buf, str, cur_len);
        buf[cur_len] = '\0';
        offset = p - str;

        int tokens = MAX_TOKENS;

        if(find_token(ini, buf + offset, 0, &cur_len, 
                      offset, buf_size, &tokens, line) == NULL)
        {
            return NULL;
        }

        if(cur_len <= old_len)
        {
            memcpy(str, buf, cur_len);
            str[cur_len] = '\0';
        }
        else
        {
            if(can_free == 1)
            {
                free(str);
            }
            return strdup(buf);
        }
    }

    return str;
}

static inline int replace_var(const ini_t* ini, char* buf, int buf_size)
{
    ini_kv_t* cur            = NULL;
    ini_kv_t* tmp            = NULL;
    uint16_t i                  = 0;
    char* p                     = NULL;

    HASH_ITER(hh, ini->hash_table, cur, tmp)
    {
        if(cur->value_count == 1)
        {
            p = replace_in_line(cur->value, buf, buf_size, ini, 0, cur->line);
            if((p != NULL) && (p != cur->value))
            {
                cur->values_size |= 0x01;
                cur->value = p;
            }
            else if(p == NULL)
            {
                return MQ_ENOKEY;
            }
        }
        else
        {
            for(i = 0; i < cur->value_count; i++)
            {
                p = replace_in_line(cur->values[i], buf, buf_size, ini,
                                    (i == 0) ? 0 : 1, cur->lines[i]);
                if(p == NULL)
                {
                    return MQ_ENOKEY;
                }
                else if(p != cur->values[i])
                {
                    if(i == 0)
                    {
                        cur->values_size |= 0x01;
                    }
                    cur->values[i] = p;
                }
            }
        }
    }

    return 0;
}
