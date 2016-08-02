#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include "pdma-lib.h"
#include "../pdma-ioctl.h"

unsigned long long ten(int n){
      int i;
      unsigned long long k =1;
      for(i=0;i<n;i++)
           k*=2;
}

void usage()
{
    printf("pdma-write device [-cnt value] [-dma value] [-d value] [-pt value] [-inc value]\n");
    printf("device: device name\n");
    printf("-cnt:  total write count\n");
    printf("-dma:  1: send dma-start command before write; 0: don't send\n");
    printf("-d:    delay between two write system call, measured by us\n");
    printf("-pt:   start pattern\n");
    printf("-inc:  pattern increase\n");
    printf("example: ./pdma-write /dev/pdma -cnt 1000000\n");
}

int main(int argc, char *argv[])
{
    struct pdma_info info;
    char *fl = argv[1];
    int fd, ret=0, check_pt=0;
    unsigned int dma_start=0, delay=0, pattern=0, inc=1;
    unsigned long long count=(unsigned long long)(30);
    unsigned long long total_write_num=0, total_write_size=0;
    unsigned long long tm_beg, tm_end, tm_total;
    static unsigned long long k=0; 
    char *pval, *buf,*tmp_buf,c;
    int i,j,t=0;
     FILE *fp;
     fp = fopen("in","r");
     if(fp==NULL)
          printf("open fail\n");


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
    buf = (char *)malloc(info.wt_block_sz);
    tmp_buf = buf;
   
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
    unsigned long long  base;

    //printf("%d %d\n",sizeof(unsigned long long ),sizeof(unsigned long));
    /* start write */
    sys_tm_get_us(&tm_beg);

    while (count--) 
    {

        if (check_pt) 
       {
            int i;
            unsigned int start_pt = pattern + inc*(info.wt_block_sz/4)*total_write_num;
            unsigned int *val32 = (unsigned int *)buf;

       }
           // for (i = 0; i < info.wt_block_sz/4; i++) {
       //         val32[i] = (start_pt + i*inc);
       //     }
    //    }
       j = 0;
       tmp_buf = buf;
          
            while(j<info.wt_block_sz/8)
           {
              *(unsigned long *)tmp_buf=++k;
              tmp_buf += 8 ;
              j=j+1;
           }
         
        ret = write(fd, buf, info.wt_block_sz);
        tmp_buf = buf;
        printf("block szie: %d\n",info.wt_block_sz);
        printf("count:%d\n",count);
        //for(j=0;j<info.wt_block_sz/8;j++)
        for(j=0;j<10;j++)
        {
           printf("%016lx\n",*((unsigned long *)tmp_buf));
           tmp_buf += 8;
        }
        printf("\n");
        if (ret != info.wt_block_sz) {
            printf("write failed\n");
            ret = -1;
            break;
        }

        if (delay != 0) {
            sys_tm_wait_us(delay);
        }

        total_write_num++;
    }
    sys_tm_get_us(&tm_end); 
    tm_total = tm_end - tm_beg;
    if (tm_total == 0) tm_total = 1;
    total_write_size = total_write_num * info.wt_block_sz;

    printf("block size %d\n",info.wt_block_sz);
    printf("total write %lldKB, elapsed %lldus\n", total_write_size/1024, tm_total);
    printf("write performance is %lldMB/s\n", total_write_size/tm_total);

    free(buf);
    close(fd);
    fclose(fp);
    return ret;
}
