#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include "pdma-ioctl.h"
#include "tool.h"
//#define  NODEV
#define DEV
// #define DEBUG
//#define WFILE

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

int main(int argc, char **argv){	
    int fd;
    int i, ret;
    int encodelen, K_byte, code_bit;
    unsigned char *buf, *outbuf, *readbuf;
    struct pdma_info info;
    struct pdma_rw_reg ctrl;
    int j, k, count;
    char *filename = "dataByte.txt";
    char *head = "readout";
    struct timeval timestart;
    struct timeval timend;
    struct timezone timez;

	/*combine filename*/
    if(argc != 2){
        printf("please input logc:\n");
        return 0;
    }
    char *tail = argv[1];
    char *logname = (char *)malloc(strlen(head) + strlen(tail) + 4);
    strcpy(logname, head);
    strcat(logname, tail);
    strcat(logname, ".txt");
    printf("filename: %s\n", logname);
	
     
#ifdef DEV
	fd = open("/dev/pdma", O_RDWR);
	if (fd == -1){
		printf("open failed for pdma device\n");
        return -1;
    }
    
    /*reset*/

ctrl.type = 1; //write
    ctrl.addr = 0;
    ctrl.val = 1; 
    ret = ioctl(fd, PDMA_IOC_RW_REG, (unsigned long)&ctrl);
    if(ret == -1){
        printf("pdma_ioc_rw_reg error\n");
        return -1;
    }
    
    ret = ioctl(fd, PDMA_IOC_INFO, (unsigned long)&info);
    if(ret == -1){
        printf("get info failed\n");
        close(fd);
        return -1;
    }
    
#endif

#ifdef NODEV
    info.wt_block_sz = 16384;
#endif

	//get file size
	K_byte = getfilesize(filename);
	//the code bit len that write into FPGA 
	code_bit = (K_byte << 3) + 4; 
	//after encode, the len is encodelen byte 
	encodelen = code_bit * 4;  
#ifdef DEBUG
	printf("\n encodelen : %d\n", encodelen);
    printf("code_len: %d\n", code_bit);
    printf("code_reg: %08x\n", code_bit<<19);
#endif
    
	//malloc buf	
	if(encodelen%info.wt_block_sz == 0)
		count = encodelen/info.wt_block_sz;
	else count = encodelen/info.wt_block_sz + 1;
    printf("info.wt_block_sz: count: %d\n", info.wt_block_sz, count);
	
    outbuf = (unsigned char *)malloc(encodelen);
	buf = (unsigned char *)malloc(count * info.wt_block_sz);
	readbuf = (unsigned char *)malloc(info.rd_block_sz);
    if(!buf){
        printf("alloc buf failed\n");
        close(fd);
        return -1;
    }


    memset(outbuf, 0, encodelen);
    memset(buf, 0, count * info.wt_block_sz);

    getoutput(filename, K_byte, outbuf);

#ifdef WFILE
    if(access("lte328.txt", 0) == 0) remove("lte228.txt");
    FILE *t1 = fopen("lte328.txt", "w");
    for(i = 0; i < encodelen; i++){  
        if(i%8 == 0) fprintf(t1, "n=%d, ", i);
        fprintf(t1, "%x", outbuf[i]);
        if((i+1)%8 == 0) fprintf(t1, "\n");
	}
    fclose(t1);
#endif

    for(i = 0; i < encodelen; i++){
        if(outbuf[i] == 1) buf[i] = 0x7f;
        else buf[i] = 0x80;
	}
    buf[encodelen-1] = 0x81;


	//swap every 8byte
 /*   for(i = 0; i < encodelen; i+=8){
        j = 7; 
        for(k = i; k < i+8; k++){
            buf[k] = outbuf[i+j];
            j--;
        }
    }
*/
#ifdef DEBUG    
	printf("outbuf data:\n");
	for(i = 0; i < encodelen; i++){
		printf("%x ", outbuf[i]);
	}
	printf("\n \n");

    printf("buf data: \n");
    for(i = 0; i < encodelen; i++){
        if(i%8 == 0) printf("n=%d, ", i);
		printf("%x ", buf[i]);
		if((i+1)%8 == 0) printf("\n");
    }
    printf("\n");
#endif

#ifdef WFILE
    if(access("writeout.txt", 0) == 0) remove("writeout.txt");
    FILE *tt = fopen("writeout.txt", "w");
    for(i = 0; i < count*info.wt_block_sz; i++){  
        if(i%8 == 0) fprintf(tt, "n=%d, ", i);
        fprintf(tt, "%x ", buf[i]);
        if((i+1)%8 == 0) fprintf(tt, "\n");
	}
    fclose(tt);
#endif

#ifdef DEV      
    gettimeofday(&timestart, &timez);
    ctrl.type = 1;
	ctrl.addr = 0;
    code_bit = code_bit << 19;   
    ctrl.val = code_bit; 
    //printf("code_bit : %08x\n", code_bit);
    ret = ioctl(fd, PDMA_IOC_RW_REG, (unsigned long)&ctrl);
    if(ret == -1){
        printf("pdma_ioc_rw_reg error\n");
        return -1;
    }
    
	/*wiret data*/
    i = 0;
    while(i < count){
		ret = write(fd, buf+i*info.wt_block_sz, info.wt_block_sz); 
		if(ret != info.wt_block_sz){
			printf("write failed\n");
			return -1;
		}
//		printf("%d write finish\n", i+1);
		i++;
	}
	
/*	//read reg
    ctrl.val = 0;
    ctrl.type = 0;
    ctrl.addr = 0;
    //while(!ctrl.val){
        ret = ioctl(fd, PDMA_IOC_RW_REG, (unsigned long)&ctrl);
	    printf("read len from reg: %d\n", ctrl.val);
        
        //if(ret < 0) continue;
    //}
    if (ret == -1) {
        printf("get info failed\n");
        close(fd);
        return -1;
    }
    //usleep(200);
*/	
	//read data
	ret = read(fd, readbuf, info.rd_block_sz);
		if (ret <= 0) {
			printf("read failed\n");
			ret = -1;
		}
	gettimeofday(&timend, &timez);
	long timeus = timend.tv_usec - timestart.tv_usec;
    FILE *time = fopen("timer.txt", "a+");
    fprintf(time, "%dbyte---use----%dus\n", K_byte, timeus);
    fclose(time);
			
	//file write
	if(access(logname, 0) == 0) remove(logname);
    FILE *fn = fopen(logname, "w");
	unsigned char *val32 = (unsigned char *)readbuf;
	for (i = 0; i < K_byte; i++) {   //print all  K_byte info.rd_block_sz
		if(i%8 == 0) fprintf(fn, "n= %d, ", i);
		fprintf(fn, "%d ", val32[i]);
		if((i+1)%8 == 0) fprintf(fn, "\n");
	}	
    fclose(fn);

#endif
    free(buf);
    close(fd);
    return 0;
}
