#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>

int main(int argc, char **argv){
	FILE *input = fopen("inputData.txt", "a+");
	int i = 0;
	int count;
	if(input == NULL){
		printf("open failed\n");
		return -1;
	}
	if(argc == 2){
		count = atoi(argv[1]);	
	} else{
		printf("Please input number\n");
		return 0;
	}
	for(i = 0; i < count; i++){
		fprintf(input, "%d ", i+1);
	}
	fclose(input);
	return 0;
}

















