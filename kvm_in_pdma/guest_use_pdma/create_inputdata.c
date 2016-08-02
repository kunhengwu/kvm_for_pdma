#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>

int main(int argc, char **argv){
	FILE *input = fopen("inputData.txt", "a+");
	int i = 0;
	if(input == NULL){
		printf("open failed\n");
		return -1;
	}
	for(i = 0; i < 64; i++){
		fprintf(input, "%d ", i+1);
	}
	fclose(input);
	return 0;
}

















