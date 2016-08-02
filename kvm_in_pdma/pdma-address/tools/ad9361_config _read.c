#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "pdma-lib.h"
#include "../pdma-ioctl.h"

void usage()
{
    printf("pdma-rw-reg device type addr [val]\n");
    printf("device: device name\n");
    printf("type:   0 for read, others for write\n");
    printf("addr:   address of register for user\n");
    printf("val:    value to be written to the specified address(valid for type!=0)\n");
    printf("example: ./pdma-rw-reg /dev/pdma 1 0x0 0x100\n");
}

int main(int argc, char *argv[])
{
    char *fl = argv[1];
    int fd, ret=0;
    FILE *fp;
    int i = 0;
    char buf[150];
    struct pdma_rw_reg ctrl;
    fp = fopen("write_reg","r");
    if(fp == NULL){
       printf("open write reg fail\n");
   }



    fd = open(fl, O_RDWR);
    if (fd == -1) {
        printf("open failed for pdma device %s \n", fl);
        return -1;
    }

 


    ctrl.type = 1;
    ctrl.addr = 0x0;
    ctrl.val = 0x023;
    while(fgets(buf,150,fp)!=NULL){
    printf("%s",buf);
    sscanf(buf,"%x",&ctrl.val);
    printf("0x%08x\n",ctrl.val);
    usleep(5000);
    //break;
    ret = ioctl(fd, PDMA_IOC_RW_REG, (unsigned long)&ctrl);
    if (ret == -1) {
        printf("ioctl failed\n");
        close(fd);
        return -1;
    }

   }
    if (ctrl.type == 0) {
        printf("user_reg[0x%x]=0x%x\n", ctrl.addr, ctrl.val);
    }
    fclose(fp);
    close(fd); 
    return 0;
}
