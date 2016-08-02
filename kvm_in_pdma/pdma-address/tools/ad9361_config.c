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
    FILE *fp, *fp_w, *fp_r;
    int i = 0,j=0;
    char buf[150];
    struct pdma_rw_reg ctrl;
    fp = fopen("write_reg","r");

    if(fp == NULL){
       printf("open write reg fail\n");
   }

   fp_w = fopen("write_reg_cmp","w");

    if(fp_w == NULL){
       printf("open write reg cmp fail\n");
   }

   fp_r = fopen("read_reg_cmp","w");

    if(fp_r == NULL){
       printf("open read reg fail\n");
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

    ctrl.type = 1;
 
    printf("%s",buf);
    sscanf(buf,"%x",&ctrl.val);
    printf("0x%08x\n",ctrl.val);

    i = ((ctrl.val & 0x0fff0000)>>16);
    j = ((ctrl.val & 0x0000ff00)>>8);
    fprintf(fp_w,"%03x %02x\n",i,j);
                                          
   
    //break;
    ret = ioctl(fd, PDMA_IOC_RW_REG, (unsigned long)&ctrl);
    if (ret == -1) {
        printf("ioctl failed\n");
        close(fd);
        return -1;
    }
    usleep(5000);
  /*
    ctrl.val = (ctrl.val & 0x0fffffff);
    printf("%08x\n",ctrl.val);
    i = ((ctrl.val & 0x0fff0000)>>16);
    ret = ioctl(fd, PDMA_IOC_RW_REG, (unsigned long)&ctrl);    
    usleep(8000);
  
    ctrl.type = 0;

    ret = ioctl(fd, PDMA_IOC_RW_REG, (unsigned long)&ctrl);    
    
    fprintf(fp_r,"%03x %02x\n",i,ctrl.val);
  
    usleep(5000);
  */
   }
    if (ctrl.type == 0) {
        printf("user_reg[0x%x]=0x%x\n", ctrl.addr, ctrl.val);
    }
    fclose(fp_w);
    fclose(fp_r);
    fclose(fp);
    close(fd); 
    return 0;
}
