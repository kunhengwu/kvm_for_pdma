/*
 * pdma-lib.h
 * pdma lib header
 */

#ifndef _PDMA_LIB_H_
#define _PDMA_LIB_H_

extern unsigned long long memparse(const char *ptr, char **retptr);
extern char *arg_get_next(int argc, char *argv[], char *name);
extern void sys_tm_get_us(unsigned long long *p_ts);
extern void sys_tm_wait_us(unsigned int us);
extern void hex_print(void *buf, int len);


#endif
