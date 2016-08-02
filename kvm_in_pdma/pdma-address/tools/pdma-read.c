#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include "pdma-lib.h"
#include "../pdma-ioctl.h"

extern void file_printall(void *buf, int len , FILE *fb);

void usage()
{
    printf("pdma-read device [-cnt value] [-dma value] [-d value] [-pt value] [-inc value]\n");
    printf("device: device name, /dev/pdma\n");
    printf("-cnt:  total read count\n");
    printf("-dma:  1: send dma-start command before read; 0: don't send\n");
    printf("-d:    delay between two read system call, measured by us\n");
    printf("-pt:   start pattern to be checked\n");
    printf("-inc:  pattern increase to be checked\n");
    printf("example: ./pdma-read /dev/pdma -cnt 1000000\n");
}

int main(int argc, char *argv[])
{
    struct pdma_info info;
    char *fl = argv[1];
    int fd, ret=0, check_pt=0;
    unsigned int dma_start=0, delay=0, pattern=0, inc=1;
    unsigned long long count=(unsigned long long)(-1);
    unsigned long long total_read_num=0, total_read_size=0;
    unsigned long long tm_beg, tm_end, tm_total;
    char *pval, *buf,*buf_high,*buf_low;
    int i;

    /* parse argument */
    if (argc == 1 || !strcmp("-h", argv[1])) {
        usage();
        return 0;
    }

    pval = arg_get_next(argc, argv, "-cnt");
    if (pval) {
        count = memparse(pval, &pval);
    }
    pval = arg_get_next(argc, argv, "-dma");
    if (pval) {
        dma_start = memparse(pval, &pval);
    }
    pval = arg_get_next(argc, argv, "-d");
    if (pval) {
        delay = memparse(pval, &pval);
    }
    pval = arg_get_next(argc, argv, "-pt");
    if (pval) {
        pattern = memparse(pval, &pval);
        check_pt = 1;
    }
    pval = arg_get_next(argc, argv, "-inc");
    if (pval) {
        inc = memparse(pval, &pval);
    }

    /* open */
    fd = open(fl, O_RDWR);
    if (fd == -1) {
        printf("open failed for pdma device %s \n", fl);
        return -1;
    }

    /* get info */
    ret = ioctl(fd, PDMA_IOC_INFO, (unsigned long)&info);
    if (ret == -1) {
        printf("get info failed\n");
        close(fd);
        return -1;
    }

    /* malloc buf */
    buf = (char *)malloc(info.rd_block_sz);
     buf_high = (char *)malloc(info.rd_block_sz);
     buf_low = (char *)malloc(info.rd_block_sz); 
    if (!buf) {
        printf("alloc buffer failed\n");
        close(fd);
        return -1;
    }

    /* start dma */
    if (dma_start != 0) {
        ret = ioctl(fd, PDMA_IOC_START_DMA);
        if (ret == -1) {
            printf("start dma failed\n");
            free(buf);
            close(fd);
            return -1;
        }
    }
    int ct = 0;
    /* start read */
    sys_tm_get_us(&tm_beg);
       FILE *fb1,*fb2,*fb;
       fb = fopen("out","w");
       if(fb == NULL)
            printf("open fial\n");
       fb1 = fopen("out_high","w");
       if(fb1 == NULL)
            printf("open fial\n");
       fb2 = fopen("out_low","w");
       if(fb1 == NULL)
            printf("open fial\n");

    while (count--) {
        ret = read(fd, buf, info.rd_block_sz);
        printf("this block read from %d\n",ret);

        if (ret != info.rd_block_sz) {
            printf("read failed\n");
            ret = -1;
            break;
        }
       for(i=0;i<info.rd_block_sz/8;i++){
               ((int *)buf_high)[i] = ((int *)buf)[2*i+1];
         }
       for(i=0;i<info.rd_block_sz/8;i++){
               ((int *)buf_low)[i] = ((int *)buf)[2*i];
         }

       if(count<=500)
	{
       file_printall(buf,info.rd_block_sz,fb);
       hex_print(buf,64);
	printf("%d\n",count);
       file_print(buf_high,info.rd_block_sz/2,fb1);
       file_print(buf_low,info.rd_block_sz/2,fb2);

	}
        if (check_pt) {
            int i;
            unsigned int start_pt = pattern + inc*(info.rd_block_sz/4)*total_read_num;
            unsigned int *val32 = (unsigned int *)buf;

            for (i = 0; i < info.rd_block_sz/4; i++) {
                if (val32[i] != (start_pt + i*inc)) {
                    printf("check pattern failed, start=0x%x inc=0x%x addr=0x%x\n", 
                           start_pt, inc, i*4);
                    hex_print(val32, info.rd_block_sz);
                    count = 0;
                    ret = -1;
                    break;
                }
            }
        }

        if (delay != 0) {
            sys_tm_wait_us(delay);
        }

        total_read_num++;
    }
    sys_tm_get_us(&tm_end); 
    tm_total = tm_end - tm_beg;
    if (tm_total == 0) tm_total = 1;
    total_read_size = total_read_num * info.rd_block_sz;

    printf("total read %lldKB, elapsed %lldus\n", total_read_size/1024, tm_total);
    printf("read performance is %lldMB/s\n", total_read_size/tm_total);
    fclose(fb1);
    fclose(fb2);
    free(buf_low);
    free(buf_high);
    free(buf);
    close(fd);
    return ret;
}
