#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include "pdma-lib.h"
#include "../pdma-ioctl.h"


int main(int argc, char *argv[])
{
    struct pdma_info info;
    char *fl = argv[1];
    int fd, ret=0, check_pt=0;
    unsigned int dma_start=0, delay=0, pattern=0, inc=1;
    unsigned long long count=(unsigned long long)(-1);
    unsigned long long total_read_num=0, total_read_size=0;
    unsigned long long tm_beg, tm_end, tm_total;

       FILE *fb;
       fb = fopen("out","r");
       if(fb == NULL)
            printf("open fial\n");



  
    return ret;
}
