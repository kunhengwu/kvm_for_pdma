#include<math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>

#include "tool.h"
//#define DEBUG
//#define WFILE

void  interleaver(unsigned char *x, int f1, int f2, int K_bit, unsigned char * xinterleaved){
	int i, index;
	for (i = 0; i < K_bit; i++){
		index = (f1 * i + f2 * i * i) % K_bit;
		xinterleaved[i] = x[index];
#ifdef DEBUG
        printf( " %u(%x)----->%u(%x)  ", i, xinterleaved[i], index, x[index]);
#endif
	}
}

int getfindex(int K_byte){
	int index = 0;
	if (K_byte<=64)
		index = (K_byte-5);
	else if (K_byte <=128)
		index = 59 + ((K_byte-64)>>1);
	else if (K_byte <= 256)
		index = 91 + ((K_byte-128)>>2);
	else if (K_byte <= 768)
		index = 123 + ((K_byte-256)>>3);
	else {
		printf("encoder: Illegal codeword size %d!!!\n", K_byte);
		return (-1);
	}
	return index;
}

void encoder(unsigned char *x,  int K_bit, unsigned char *parity){
	//intial data
	int i = 0;
	unsigned char p0 = '0';
	unsigned char p1 = '0';
	unsigned char p2 = '0';
	unsigned char p3 = '0';
	unsigned char p4 = '0';
	unsigned char p = '0';  
	

	//encoder logical, get parity  and  tail bit
	for (i = 0; i < K_bit + 3; i ++){
		p3 = p2;
		p2 = p1;
		p1 = p0;
		p4 = (p2 ^ p3) & 1;
		if (i < K_bit )     
			p0 = (x[i] ^ p4) & 1;
		else{   //  tail bit
			x[i] = p4;
			p0 = (p4 ^ p4) & 1;      //p4 ^ p4
		}
		p = (p0 ^ p1 ^ p3) &1;
		parity[i] = p;
	}
}


int getfilesize(char *filename){
	int K_byte = 0;
	//input data
	FILE * input_fd = fopen(filename, "r");
	if (input_fd == NULL)
		printf("open %s fail\n", filename);

	/*get file size*/
	while(!feof(input_fd)){
        if(fgetc(input_fd) == ' ') K_byte++;
    }
 	
	fclose(input_fd);	
	printf(" %d byte\n", K_byte);
	return K_byte;
}

//read data and encoder
void  getoutput(char *filename, int k_byte, unsigned char *buf){	
	unsigned char *x;                     //input bit
    unsigned char *x_buf;
	unsigned char *xinterleaved;          //inter-leaved bit
	unsigned char *parity1, *parity2;
	unsigned char *x_byte;             
	int f1, f2, findex;
	int i, b;
    int K_byte = k_byte;   
	int K_bit = K_byte << 3;              //K is data block size
	unsigned char *y = buf;               //output  encoder data

	//malloc for x, y, x_interleave ,conside the tail bit
	x = (unsigned char *)malloc(K_bit + 3);           
	xinterleaved = (unsigned char *)malloc(K_bit + 3);
	parity1 = (unsigned char *)malloc(K_bit + 3);
	parity2 = (unsigned char *)malloc(K_bit + 3); 
	x_byte = (unsigned char *)malloc(K_byte);
    memset(x, 0, K_bit+3);
    memset(xinterleaved, 0, K_bit+3);
    memset(parity1, 0, K_bit+3);
    memset(parity2, 0, K_bit+3);
    memset(x_byte, 0, K_byte);

	//read data
	FILE * input_fd = fopen(filename, "r");
	//printf("input data:\n"); 
	x_buf = x_byte;
	for(i = 0; i < K_byte; i++){
        fscanf(input_fd, "%d ", x_buf);
		//printf("%d\t", *x_buf);
		x_buf++;
	}
    close(input_fd);
    //printf("\n\n");

    //transfer byte into 8bit, 1bit occupy 1byte
	//printf("input bit:\n"); 
    unsigned char *x_tmp = x;
    for(i = 0; i < K_byte; i++){
        for(b = 0; b < 8; b++){
            *x_tmp = (x_byte[i]&(1<<(7-b)))>>(7-b);
            //printf("%x", *x_tmp);
			x_tmp++;
        }
        //printf("\t");
    }
   // printf("\n");

#ifdef WFILE
    if(access("x.txt", 0) == 0) remove("x.txt");
    FILE *xx = fopen("x.txt", "w");
    for(i = 0; i < K_bit; i++){  
        if(i%8 == 0) fprintf(xx, "n=%d, ", i);
        fprintf(xx, "%x ", x[i]);
        if((i+1)%8 == 0) fprintf(xx, "\n");
	}
    fclose(xx);
#endif
	
#ifdef WFILE
    if(access("ii.txt", 0) == 0) remove("ii.txt");
    FILE *six = fopen("ii.txt", "w");
    //printf("input 16x:\n"); 
    for(i = 0; i < K_byte; i++){
        fprintf(six, "%02x\t", x_byte[i]);
    }
    fclose(six);
    printf("\n");
#endif

	//get findex
	findex = getfindex(K_byte); 
	f1 = f1f2[findex * 2];
	f2 = f1f2[findex * 2 + 1];
	//printf("f1: %d, f2: %d \n", f1, f2);

	//get x_interleaved data
	interleaver(x, f1, f2, K_bit, (unsigned char *)xinterleaved);

#ifdef WFILE
    if(access("xinter.txt", 0) == 0) remove("xinter.txt");
    FILE *xr = fopen("xinter.txt", "w");
    for(i = 0; i < K_bit; i++){  
        if(i%8 == 0) fprintf(xr, "n=%d, ", i);
        fprintf(xr, "%x ", xinterleaved[i]);
        if((i+1)%8 == 0) fprintf(xr, "\n");
	}
    fclose(xr);
#endif

	//encoder, get parity
	encoder(x,  K_bit, parity1);
#ifdef DEBUG
    printf("parity1 data:\n"); 
	for(i = 0; i < K_bit + 3; i++){
		printf("%x  ",  parity1[i]);
	}
	printf(" \n");
#endif

	encoder(xinterleaved, K_bit, parity2);
#ifdef DEBUG	
    printf("parity2 data:\n"); 
	for(i = 0; i < K_bit +3; i++){
		printf("%x  ",  parity2[i]);
	}	
	printf(" \n");
#endif

	//get output y
	unsigned char * y_buf = y;
	for( i = 0; i < K_bit; i ++){
		*y_buf++ = *x++;               
		*y_buf++ = *parity1++;   
		*y_buf++ = *parity2++;   
		 y_buf++;                        
	}
	//12 tail bit: (xp xp xp) RSC1  (xp xp xp) RSC2 
	*y_buf++ = *x++;            
	*y_buf++ = *parity1++;   
	*y_buf++ = *x++;            
	y_buf++;                        
	*y_buf++ = *parity1++;   
	*y_buf++ = *x++;          
	*y_buf++ = *parity1++;   
	 y_buf++;                       

	*y_buf++ = xinterleaved[K_bit];            
	*y_buf++ = *parity2++;                    
	*y_buf++ = xinterleaved[K_bit + 1];         
	 y_buf++;                                               
	*y_buf++ = *parity2++;              
	*y_buf++ = xinterleaved[K_bit + 2];       
	*y_buf++ = *parity2++;              

#ifdef DEBUG   
	printf("y data:\n"); 
	for(i = 0; i < (K_bit + 4) * 4; i++){
		printf("%x ", y[i]);
	}
	printf("\n");
#endif

}
