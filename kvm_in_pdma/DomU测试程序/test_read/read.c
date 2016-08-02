#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include "pdma-ioctl.h"

unsigned long reverse(unsigned long x){
    unsigned long y;
    
    y = (x & 0x01) << 7;
    y |= (x & 0x02) << 5;
    y |= (x & 0x04) << 3;
    y |= (x & 0x08) << 1;
    y |= (x & 0x10) >> 1;
    y |= (x & 0x20) >> 3;
    y |= (x & 0x40) >> 5;
    y |= (x & 0x80) >> 7;
   
    return y;
}

int main(int argc, char *argv[])
{
    struct pdma_info info;
    char *pval, *buf;
    int fd, len, ret, i;
    char *fl = "/dev/fpdma";
    char *head = "readout";
    int ct = 0;
    
    if(argc != 2){
        printf("please input length!\n");
        return 0;
    }
    len = atoi(argv[1]);
    //printf("read len: %d\n", len);

    char *tail = argv[1];
    char *filename = (char *)malloc(strlen(head) + strlen(tail) + 4);
    strcpy(filename, head);
    strcat(filename, tail);
    strcat(filename, ".txt");
    printf("filename: %s\n", filename);

    /* open */
    fd = open(fl, O_RDWR);
    if (fd == -1) {
        printf("open failed for pdma device %s \n", fl);
        return -1;
    }


    /* malloc buf */
    buf = (char *)malloc(len);
    if (!buf) {
        printf("alloc buffer failed\n");
        close(fd);
        return -1;
    }

    /* start read */
    if(access(filename, 0) == 0) remove(filename);
    FILE *fn = fopen(filename, "w");

    ret = read(fd, buf, len);
    if (ret <= 0) {
        printf("read failed\n");
        ret = -1;
    }

    unsigned char *val32 = (unsigned char *)buf;
    for (i = 0; i < len; i++) {  //len
        if(i%8 == 0) fprintf(fn, "n= %d, ", i);
        fprintf(fn, "%d ", val32[i]);
        if((i+1)%8 == 0) fprintf(fn, "\n");
    }

    free(buf);
    close(fd);
    close(fn);
    return ret;
}
