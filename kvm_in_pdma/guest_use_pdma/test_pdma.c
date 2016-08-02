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

#define PDMA_DEV "/dev/pdma"

int main(int argc, char **argv){
	int fd;
	int ret, i;
	int code_bit, K_byte, encodelen;
	char *inputfile = "inputData.txt";
	char *outputfile = "outputData.txt";
	struct pdma_info info;
	struct pdma_rw_reg ctrl;
	int count = 0;
	unsigned char *outputBuf, *inputBuf, *readBuf;
	/*
	* init pdma
	*/
	fd = open(PDMA_DEV, O_RDWR);
	if(fd == -1){
		printf("open failed for pdma device\n");
		return -1;
	}
	ctrl.type = 1;
	ctrl.addr = 0;
	ctrl.val  = 1;
	ret = ioctl(fd, PDMA_IOC_RW_REG, (unsigned long)&ctrl);
	if(ret == -1){
		printf("pdma_ioc_rw_reg error\n");
		close(fd);
		return -1;
	}

	ret = ioctl(fd, PDMA_IOC_INFO, (unsigned long)&info);
	if(ret == -1){		
		printf("get info of pdma faild\n");
		close(fd);
		return -1;
	}

	printf("info.wt_block_sz is %d\n", info.wt_block_sz);

	K_byte = getfilesize(inputfile);
	code_bit = (K_byte << 3) + 4;
	encodelen = code_bit << 2;

	printf("K_byte is %d\n", K_byte);
	printf("code_bit is %d\n", code_bit);
	printf("encodelen is %d\n", encodelen);

	count = (encodelen + 1) / info.wt_block_sz + 1;
	outputBuf = (unsigned char *)malloc(encodelen);
	inputBuf = (unsigned char *)malloc(count * info.wt_block_sz);
	memset(outputBuf, 0, encodelen);
	memset(inputBuf, 0, count * info.wt_block_sz);
	getoutput(inputfile, K_byte, outputBuf);
	printf("count is %d\n", count);
	// for(i = 0; i < encodelen; i++){
	// 	if(i % 8 == 0)
	// 		printf("\n");
	// 	printf("%x ", outputBuf[i]);
	// }
	for(i = 0; i < encodelen; i++){
		if(outputBuf[i] == 1)
			inputBuf[i] = 0x7f;
		else 
			inputBuf[i] = 0x80;
	}
	inputBuf[encodelen - 1] = 0x81;

	//start write data into pdma_device;
	ctrl.type = 1;
	ctrl.addr = 0;
	code_bit = code_bit << 19;
	ctrl.val = code_bit;
	ret = ioctl(fd, PDMA_IOC_RW_REG, (unsigned long)&ctrl);
	if(ret == -1){
		printf("pdma_ioc_rw_reg error\n");
		return -1;
	}
	for (i = 0; i < count; ++i){
		ret = write(fd, inputBuf + i * info.wt_block_sz, info.wt_block_sz);
		if(ret != info.wt_block_sz){
			printf("write faild in %d\n", i);
			return -1;
		}
	}

	readBuf = (unsigned char *)malloc(info.rd_block_sz);
	// read data from pdma_dev
	
	ret = read(fd, readBuf, info.rd_block_sz);
	
	if(ret == -1){
		printf("read data from pdma_device failed\n");
		return -1;
	}
	printf("readBuf is \n");
	for(i = 0; i < K_byte; i++){
		if(i % 8 == 0)
			printf("\n");
		printf("%d ", readBuf[i]);
	}
	free(outputBuf);
	free(inputBuf);
	close(fd);
	return 0;
}


