/*
 * pdma-ioctl.h
 * pdma ioctl header
 */

#ifndef _PDMA_IOCTL_H_
#define _PDMA_IOCTL_H_

#include <linux/ioctl.h>

/*
 * Ioctl definitions
 */

/* Use 0x9B as magic number */
#define PDMA_IOC_MAGIC  0x9B

/* for PDMA_IOC_RW_REG */
struct pdma_rw_reg {
    unsigned int type;             /* 0 for read, non-zero for write */
    unsigned int addr;             /* access address, from 0 to 256, must be 4 aligned */
    unsigned int val;              /* read/write value */
};

/* for PDMA_IOC_INFO */
struct pdma_info {
    unsigned long rd_pool_sz;      /* read pool size */
    unsigned long wt_pool_sz;      /* write pool size */
    unsigned int  rd_block_sz;     /* read block size */
    unsigned int  wt_block_sz;     /* write block size */
};

/* for PDMA_IOC_STAT */
struct pdma_stat {
    unsigned int rd_free_cnt;      /* current read free count */
    unsigned int rd_submit_cnt;    /* current read submit count */
    unsigned int rd_pending_cnt;   /* current read pending count */
    unsigned int wt_free_cnt;      /* current write free count */
    unsigned int wt_submit_cnt;    /* current write submit count */
    unsigned int wt_pending_cnt;   /* current write pending count */

    unsigned int rd_fifo_min;      /* read fifo minimum usage percentage */
    unsigned int rd_fifo_avg;      /* read fifo average usage percentage */
    unsigned int rd_fifo_cur;      /* read fifo current usage percentage */
    unsigned int rd_pending_max;   /* unread data maximum percentage */
    unsigned int rd_pending_avg;   /* unread data average percentage */
    unsigned int rd_pending_cur;   /* unread data current percentage */
    unsigned int wt_fifo_max;      /* write fifo maximum usage percentage */
    unsigned int wt_fifo_avg;      /* write fifo average usage percentage */
    unsigned int wt_fifo_cur;      /* write fifo current usage percentage */
    unsigned int wt_pending_max;   /* unwrite data maximum percentage */
    unsigned int wt_pending_avg;   /* unwrite data average percentage */
    unsigned int wt_pending_cur;   /* unwrite data current percentage */
};

#define PDMA_IOC_RW_REG         _IOWR(PDMA_IOC_MAGIC, 0, struct pdma_rw_reg)
#define PDMA_IOC_INFO           _IOR(PDMA_IOC_MAGIC,  1, struct pdma_info)
#define PDMA_IOC_START_DMA      _IO(PDMA_IOC_MAGIC,   2)
#define PDMA_IOC_STOP_DMA       _IO(PDMA_IOC_MAGIC,   3)
#define PDMA_IOC_STAT           _IOR(PDMA_IOC_MAGIC,  4, struct pdma_stat)
#define PDMA_IOC_RW_REG_DEBUG   _IOWR(PDMA_IOC_MAGIC, 5, struct pdma_rw_reg)
#define PDMA_IOC_MAXNR          5

#endif
