#include<stdlib.h>
#include<stdio.h>

int main(int argc, char **argv){
    int i;
    int count = 52;
    char *fname = "dataByte.txt";
    
    if(access(fname, 0) == 0) remove(fname);

    FILE *fp = fopen(fname, "w");
    if(!fp) printf("open dataByte.txt fail\n");
    
    if(argc == 2) count = atoi(argv[1]);
    
    for(i = 1; i <= count; i++)
        fprintf(fp, "%d ", i%256);
    
    fclose(fp);

    return 0;
}
