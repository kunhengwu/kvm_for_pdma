#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include "../pdma-ioctl.h"

void usage()
{
    printf("pdma-stat device\n");
    printf("device: device name\n");
}

int main(int argc, char *argv[])
{
    char *fl = argv[1];
    int fd, ret;
    struct pdma_stat stat;

    if (argc == 1 || !strcmp("-h", argv[1])) {
        usage();
        return 0;
    }

    fd = open(fl, O_RDONLY);
    if (fd == -1) {
        printf("open failed for pdma device %s \n", fl);
        return -1;
    }

    ret = ioctl(fd, PDMA_IOC_STAT, (unsigned long)&stat);
    if (ret == -1) {
        printf("ioctl failed\n");
        close(fd);
        return -1;
    }

    printf("rd_free_cnt    =  %d\n",   stat.rd_free_cnt);
    printf("rd_submit_cnt  =  %d\n",   stat.rd_submit_cnt);
    printf("rd_pending_cnt =  %d\n",   stat.rd_pending_cnt);
    printf("wt_free_cnt    =  %d\n",   stat.wt_free_cnt);
    printf("wt_submit_cnt  =  %d\n",   stat.wt_submit_cnt);
    printf("wt_pending_cnt =  %d\n",   stat.wt_pending_cnt);
    printf("rd_fifo_min    =  %d%%\n", stat.rd_fifo_min);
    printf("rd_fifo_avg    =  %d%%\n", stat.rd_fifo_avg);
    printf("rd_fifo_cur    =  %d%%\n", stat.rd_fifo_cur);
    printf("rd_pending_max =  %d%%\n", stat.rd_pending_max);
    printf("rd_pending_avg =  %d%%\n", stat.rd_pending_avg);
    printf("rd_pending_cur =  %d%%\n", stat.rd_pending_cur);
    printf("wt_fifo_max    =  %d%%\n", stat.wt_fifo_max);
    printf("wt_fifo_avg    =  %d%%\n", stat.wt_fifo_avg);
    printf("wt_fifo_cur    =  %d%%\n", stat.wt_fifo_cur);
    printf("wt_pending_max =  %d%%\n", stat.wt_pending_max);
    printf("wt_pending_avg =  %d%%\n", stat.wt_pending_avg);
    printf("wt_pending_cur =  %d%%\n", stat.wt_pending_cur);

    close(fd);
    return 0;
}
