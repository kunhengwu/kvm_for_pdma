
#include <stdio.h>
#include <sys/time.h>

/*
 *  The followings are for parsing a string to a number 
 */
#define _U	0x01	/* upper */
#define _L	0x02	/* lower */
#define _D	0x04	/* digit */
#define _C	0x08	/* cntrl */
#define _P	0x10	/* punct */
#define _S	0x20	/* white space (space/lf/tab) */
#define _X	0x40	/* hex digit */
#define _SP	0x80	/* hard space (0x20) */
const unsigned char _ctype[] = {
_C,_C,_C,_C,_C,_C,_C,_C,				/* 0-7 */
_C,_C|_S,_C|_S,_C|_S,_C|_S,_C|_S,_C,_C,			/* 8-15 */
_C,_C,_C,_C,_C,_C,_C,_C,				/* 16-23 */
_C,_C,_C,_C,_C,_C,_C,_C,				/* 24-31 */
_S|_SP,_P,_P,_P,_P,_P,_P,_P,				/* 32-39 */
_P,_P,_P,_P,_P,_P,_P,_P,				/* 40-47 */
_D,_D,_D,_D,_D,_D,_D,_D,				/* 48-55 */
_D,_D,_P,_P,_P,_P,_P,_P,				/* 56-63 */
_P,_U|_X,_U|_X,_U|_X,_U|_X,_U|_X,_U|_X,_U,		/* 64-71 */
_U,_U,_U,_U,_U,_U,_U,_U,				/* 72-79 */
_U,_U,_U,_U,_U,_U,_U,_U,				/* 80-87 */
_U,_U,_U,_P,_P,_P,_P,_P,				/* 88-95 */
_P,_L|_X,_L|_X,_L|_X,_L|_X,_L|_X,_L|_X,_L,		/* 96-103 */
_L,_L,_L,_L,_L,_L,_L,_L,				/* 104-111 */
_L,_L,_L,_L,_L,_L,_L,_L,				/* 112-119 */
_L,_L,_L,_P,_P,_P,_P,_C,				/* 120-127 */
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,			/* 128-143 */
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,			/* 144-159 */
_S|_SP,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,	/* 160-175 */
_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,	/* 176-191 */
_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,	/* 192-207 */
_U,_U,_U,_U,_U,_U,_U,_P,_U,_U,_U,_U,_U,_U,_U,_L,	/* 208-223 */
_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,	/* 224-239 */
_L,_L,_L,_L,_L,_L,_L,_P,_L,_L,_L,_L,_L,_L,_L,_L};	/* 240-255 */
#define __ismask(x) (_ctype[(int)(unsigned char)(x)])
#define isxdigit(c)	((__ismask(c)&(_D|_X)) != 0)
#define isdigit(c)	((__ismask(c)&(_D)) != 0)
#define TOLOWER(x) ((x) | 0x20)

static unsigned int simple_guess_base(const char *cp)
{
    if (cp[0] == '0') {
        if (TOLOWER(cp[1]) == 'x' && isxdigit(cp[2]))
            return 16;
        else
            return 8;
    } else {
        return 10;
    }
}

/*
 * simple_strtoull - convert a string to an unsigned long long
 * @cp: The start of the string
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use
 */
unsigned long long simple_strtoull(const char *cp, char **endp, unsigned int base)
{
    unsigned long long result = 0;

    if (!base)
        base = simple_guess_base(cp);

    if (base == 16 && cp[0] == '0' && TOLOWER(cp[1]) == 'x')
        cp += 2;

    while (isxdigit(*cp)) {
        unsigned int value;

        value = isdigit(*cp) ? *cp - '0' : TOLOWER(*cp) - 'a' + 10;
        if (value >= base)
            break;
        result = result * base + value;
        cp++;
    }
    if (endp)
        *endp = (char *)cp;

    return result;
}

/*
 *    memparse - parse a string with mem suffixes into a number
 *    @ptr: Where parse begins
 *    @retptr: (output) Optional pointer to next char after parse completes
 *
 *    Parses a string into a number.  The number stored at @ptr is
 *    potentially suffixed with %K (for kilobytes, or 1024 bytes),
 *    %M (for megabytes, or 1048576 bytes), or %G (for gigabytes, or
 *    1073741824).  If the number is suffixed with K, M, or G, then
 *    the return value is the number multiplied by one kilobyte, one
 *    megabyte, or one gigabyte, respectively.
 */

unsigned long long memparse(const char *ptr, char **retptr)
{
    char *endptr;    /* local pointer to end of parsed string */

    unsigned long long ret = simple_strtoull(ptr, &endptr, 0);

    switch (*endptr) {
    case 'G':
    case 'g':
        ret <<= 10;
    case 'M':
    case 'm':
        ret <<= 10;
    case 'K':
    case 'k':
        ret <<= 10;
        endptr++;
    default:
        break;
    }

    if (retptr)
        *retptr = endptr;

    return ret;
}


char *arg_get_next(int argc, char *argv[], char *name)
{
    int i;

    /* if the name is the last, also return NULL */
    for (i = 1; i < argc - 1; i++) {
        if (!strcmp(name, argv[i])) {
            return argv[i+1];
        }
    }

    return 0;
}

void sys_tm_get_us(unsigned long long *p_ts)
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    *p_ts  = tv.tv_sec * 1000000ull;
    *p_ts += tv.tv_usec;
}
void sys_tm_wait_us(unsigned int us)
{
    unsigned long long start, end, delay;
    sys_tm_get_us(&start);
    do {
        sys_tm_get_us(&end);
        if (end < start) {
            break;
        }
        delay = end - start;
    } while(delay < us);
}

#define HEX_PRINT_ONE_LINE  4
void hex_print(void *buf, int len)
{
    unsigned int *v32 = (unsigned int *)buf;
    int i, j, idx;

    for (i = 0; i < len/HEX_PRINT_ONE_LINE; i++) {
        printf("0x%08x: ", HEX_PRINT_ONE_LINE * i);
        for (j = 0; j < HEX_PRINT_ONE_LINE/4; j++) {
            idx = i*HEX_PRINT_ONE_LINE/4 + j;
            printf("%08x ", v32[idx]);
        }
        printf("\n");
    }
}


void file_print(void *buf, int len , FILE *fb)
{
    unsigned int *v32 = (unsigned int *)buf;
    int i, j, idx;

    for (i = 0; i < len/HEX_PRINT_ONE_LINE; i++) {
        //printf("0x%08x: ", HEX_PRINT_ONE_LINE * i);
        for (j = 0; j < HEX_PRINT_ONE_LINE/4; j++) {
            idx = i*HEX_PRINT_ONE_LINE/4 + j;
            fprintf(fb,"%08x ", v32[idx]);
        }
        fprintf(fb,"\n");
    }
}

void file_printall(void *buf, int len , FILE *fb)
{
    unsigned int *v32 = (unsigned int *)buf;
    int i, j, idx;

    for (i = 0; i < len/16; i++) {
        //printf("0x%08x: ", HEX_PRINT_ONE_LINE * i);
        for (j = 0; j < 16/4; j++) {
            idx = i*16/4 + j;
            fprintf(fb,"%08x ", v32[idx]);
        }
        fprintf(fb,"\n");
    }
}


