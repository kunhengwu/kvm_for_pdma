#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include "../pdma-ioctl.h"

void usage()
{
    printf("pdma-info device\n");
    printf("device: device name\n");
}

int main(int argc, char *argv[])
{
    char *fl = argv[1];
    int fd, ret;
    struct pdma_info info;

    if (argc == 1 || !strcmp("-h", argv[1])) {
        usage();
        return 0;
    }

    fd = open(fl, O_RDONLY);
    if (fd == -1) {
        printf("open failed for pdma device %s \n", fl);
        return -1;
    }

    ret = ioctl(fd, PDMA_IOC_INFO, (unsigned long)&info);
    if (ret == -1) {
        printf("ioctl failed\n");
        close(fd);
        return -1;
    }

    printf("rd_pool_sz  = 0x%lx\n", info.rd_pool_sz);
    printf("wt_pool_sz  = 0x%lx\n", info.wt_pool_sz);
    printf("rd_block_sz = 0x%x\n",  info.rd_block_sz);
    printf("wt_block_sz = 0x%x\n",  info.wt_block_sz);

    close(fd);
    return 0;
}
