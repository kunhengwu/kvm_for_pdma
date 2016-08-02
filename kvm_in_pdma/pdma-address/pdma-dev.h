/*
 * pdma-dev.h
 * pdma device header
 */

#include <linux/list.h>
#include <linux/cdev.h>
#include <linux/spinlock.h>

#ifndef _PDMA_DEV_H_
#define _PDMA_DEV_H_

#define PDMA_DEBUG

#define PDMA_MAJOR          0
#define PDMA_MINOR          0
#define PDMA_MAX_DEV        32
#define PDMA_POOL           "128m"
#define PDMA_BLOCK          "16k"
#define PDMA_DEV_NAME       "pdma"
#define PDMA_FIFO_WM        (PDMA_RD_FIFO_DEPTH/4)


struct pdma_buf {
    struct list_head list;
	void *virt_addr;
	dma_addr_t phys_addr;
};

struct pdma {
    int dma_enable;
    atomic_t rw_cnt;

    wait_queue_head_t rdque;
    wait_queue_head_t wtque;

    spinlock_t pcie_lock;
    spinlock_t lock;
    struct list_head rd_free;
    struct list_head rd_submit;
    struct list_head rd_pending;
    struct list_head wt_free;
    struct list_head wt_pending;
    struct list_head wt_submit;
    int rd_submit_cnt;
    int wt_submit_cnt;

    struct pdma_buf *rd_buf_a;
    struct pdma_buf *wt_buf_a;
    int rd_buf_nr;
    int wt_buf_nr;
    int rd_buf_size;
    int wt_buf_size;

    void *pcie_ctrl;
    void *pdev;

    struct cdev cdev;

#ifdef PDMA_DEBUG
    atomic_t   rd_free_atomic;
    atomic_t   rd_submit_atomic;
    atomic_t   rd_pending_atomic;
    atomic_t   wt_free_atomic;
    atomic_t   wt_pending_atomic;
    atomic_t   wt_submit_atomic;

    atomic64_t rd_fifo_total_atomic;
    atomic64_t rd_fifo_cnt_atomic;
    atomic_t   rd_fifo_min_atomic;
    atomic64_t rd_pending_total_atomic;
    atomic64_t rd_pending_cnt_atomic;
    atomic_t   rd_pending_max_atomic;
    atomic64_t wt_fifo_total_atomic;
    atomic64_t wt_fifo_cnt_atomic;
    atomic_t   wt_fifo_max_atomic;
    atomic64_t wt_pending_total_atomic;
    atomic64_t wt_pending_cnt_atomic;
    atomic_t   wt_pending_max_atomic;
#endif
};

extern int  pdma_dev_init(void *pdma);
extern void pdma_dev_exit(void *pdma);
extern void pdma_intr_notify(void *pdma);
extern void pdma_dev_list_init(struct pdma *ppdma);
extern void pdma_dev_init_for_pcie_error(void *pdma);
extern void pdma_dev_exit_for_pcie_error(void *pdma);
extern void *pdma_alloc_entity(void);
extern void pdma_free_entity(void *ppdma);
extern void *pdma_get_pcie_ctrl(void *pdma);
extern void pdma_set_pcie_ctrl(void *pdma, void *pcie_ctrl);
extern void *pdma_get_pcie_dev(void *pdma);
extern void pdma_set_pcie_dev(void *pdma, void *pdev);
extern void *pdma_get_dev(void *pdma);
extern void pdma_stat_init(void *pdma);
extern void pdma_stat_his_init(void *pdma);
extern void pdma_stat_rd_free_inc(void *pdma);
extern void pdma_stat_rd_submit_inc(void *pdma);
extern void pdma_stat_rd_pending_inc(void *pdma);
extern void pdma_stat_rd_free_dec(void *pdma);
extern void pdma_stat_rd_submit_dec(void *pdma);
extern void pdma_stat_rd_pending_dec(void *pdma);
extern void pdma_stat_wt_free_inc(void *pdma);
extern void pdma_stat_wt_submit_inc(void *pdma);
extern void pdma_stat_wt_pending_inc(void *pdma);
extern void pdma_stat_wt_free_dec(void *pdma);
extern void pdma_stat_wt_submit_dec(void *pdma);
extern void pdma_stat_wt_pending_dec(void *pdma);
extern spinlock_t *pdma_get_pcie_lock(void *pdma);
extern void pdma_user_intr_handler(void *pdma);

#endif
