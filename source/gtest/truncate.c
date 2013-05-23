#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Usage: %s <db_file> <size>\n", argv[0]);
        printf("e.g., use '%s <db_file> 4096' to shrink db file to 4096 bytes.\n", argv[0]);
        printf("      use '%s <db_file> 0'    to recover db file to 64 Mbytes.\n", argv[0]);
        exit(-1);
    }

    char *file   = argv[1];
    off_t length = atoi(argv[2]);
    if (length <= 0) length = 64 * 1024 * 1024;

    int ret = truncate(file, length);
    if (ret == 0)
        printf("truncate file: %s to %d bytes successfully.\n", file, (int)length);
    else
        printf("truncate file: %s failure.\n", file);

    return ret;
}
