#include <stdio.h>
#include <strings.h>

typedef unsigned char uchar; 
static char ligne[20480+1];

#define MAX_TRACE_LINE (sizeof(ligne)/64)

char *asc_trace(char *buffer,int lg)
{
    int i=0, j=0, x=0, y=0, z=0, nb_bloc; 

    bzero(ligne,sizeof(ligne)); 
    
    nb_bloc = ((lg + 15) / 16) > (int)MAX_TRACE_LINE ? (int)MAX_TRACE_LINE : ((lg + 15) /16);

    for ( x=0 ; x < nb_bloc ; x++)
    {
        sprintf(ligne + j,"%6.6x  ",i); j +=8 ;


        for (z=0; z<4; z++)
        {
            i < lg ? sprintf(ligne + j,"%02X",(uchar)buffer[i]): 
                sprintf(ligne + j,"%2s","  "); 
            j += 2; i++;

            i < lg ? sprintf(ligne + j,"%02X",(uchar)buffer[i]): 
                sprintf(ligne + j,"%2s","  "); 
            j += 2; i++;

            i < lg ? sprintf(ligne + j,"%02X",(uchar)buffer[i]): 
                sprintf(ligne + j,"%2s","  "); 
            j += 2; i++;

            i < lg ? sprintf(ligne + j,"%02X ",(uchar)buffer[i]): 
                sprintf(ligne + j,"%3s","   "); 
            j += 3; i++;
        }
        i-=16;
        sprintf(ligne + j," |"); j += 2;
        for (z=0; z<16; z++)
        {
            y=(uchar)buffer[i];

            if ( 0x21<=y && y<=0x7E )
            {
                sprintf(ligne + j, "%c" , i <= lg ? y : '.'); j += 1;
                i++;
            }
            else
            {
                sprintf(ligne + j,"."); j += 1;
                i++;
            }
        }
        sprintf(ligne + j,"|\n"); j += 2;
    }
    return ligne;
}
