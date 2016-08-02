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

     fread(buf,4,8192,fp);

     for(i = 0;i<16384/4;i++){
         
            fprintf(fp2,"%s",itoa(buf[4*i],str,2));
            fprintf(fp2,"%s",itoa(buf[4*i+1],str,2));
            fprintf(fp2,"%s",itoa(buf[4*i+2],str,2));
            fprintf(fp2,"%s",itoa(buf[4*i+3],str,2));

             printf("%08x\n",buf[4*i]);
          }



     fclose(fp);
     fclose(fp2);


}

//fprintf(fp2,"%s",sprintf(str,"%x",buf[4*i]));

//itoa(pid, &namebuf[length], 10);      // Unix版本：itoa()在头文件<stdlib.h>中不存在  
    sprintf(namebuf+length, "%d", pid);     // 使用sprintf将整数转换成字符串  

void   itoa   (   unsigned   long   val,   char   *buf,   unsigned   radix   )   
{   
                char   *p;                                 /*   pointer   to   traverse   string   */   
                char   *firstdig;                   /*   pointer   to   first   digit   */   
                char   temp;                             /*   temp   char   */   
                unsigned   digval;                 /*   value   of   digit   */   

                p   =   buf;   
                firstdig   =   p;                       /*   save   pointer   to   first   digit   */   

                do   {   
                        digval   =   (unsigned)   (val   %   radix);   
                        val   /=   radix;               /*   get   next   digit   */   

                        /*   convert   to   ascii   and   store   */   
                        if   (digval   >   9)   
                                *p++   =   (char   )   (digval   -   10   +   'a ');     /*   a   letter   */   
                        else   
                                *p++   =   (char   )   (digval   +   '0 ');               /*   a   digit   */   
                }   whio%xle   (val   >   0);   

                /*   We   now   have   the   digit   of   the   number   in   the   buffer,   but   in   reverse   
                      order.     Thus   we   reverse   them   now.   */   

                *p--   =   '\0 ';                         /*   terminate   string;   p   points   to   last   digit   */   

                do   {   
                        temp   =   *p;   
                        *p   =   *firstdig;   
                        *firstdig   =   temp;       /*   swap   *p   and   *firstdig   */   
                        --p;   
                        ++firstdig;                   /*   advance   to   next   two   digits   */   
                }   while   (firstdig   <   p);   /*   repeat   until   halfway   */   
}
