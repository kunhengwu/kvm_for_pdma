#include <stdio.h>
#include <stdlib.h>

#include <string.h>


int main(int argc, char *argv[])
{

     FILE *fp,*fp2;
     fp = fopen("sinedat2","r");
     fp2 =fopen("sinedat","w");
     char str[64];
     int i;

     int *buf;
      
     buf= (int *)malloc(8192*sizeof(int));

     fread(buf,4,4096,fp);

     for(i = 0;i<4096/2;i++){
         
             fprintf(fp2,"%08x\n",buf[2*i]);
             fprintf(fp2,"%08x\n",buf[2*i+1]);

             printf("%08x\n",buf[2*i]);
          }



     fclose(fp);
     fclose(fp2);


}
