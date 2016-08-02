#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include "../pdma-ioctl.h"

void usage()
{
    printf("pdma-dma-start device\n");
    printf("device: device name\n");
}

int main(int argc, char *argv[])
{
    char *fl = argv[1];
    int fd, ret;

    if (argc == 1 || !strcmp("-h", argv[1])) {
        usage();
        return 0;
    }

    fd = open(fl, O_RDWR);
    if (fd == -1) {
        printf("open failed for pdma device %s \n", fl);
        return -1;
    }

    ret = ioctl(fd, PDMA_IOC_START_DMA);
    if (ret == -1) {
        printf("ioctl failed\n");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}
