/*
 * pdma-pcie.h
 * pdma pcie header
 */

#ifndef _PDMA_PCIE_H_
#define _PDMA_PCIE_H_

#define PDMA_PCI_DRV_NAME                   "pdma"
#define PDMA_IRQ_MAX_REQ_NR                 1
#define PDMA_IRQ_USED_NR                    1
#define PDMA_RD_FIFO_DEPTH                  128
#define PDMA_WT_FIFO_DEPTH                  128

#define PDMA_REG_RD_FIFO                    0
#define PDMA_REG_WT_FIFO                    0x08
#define PDMA_REG_RW_CNT                     0x10
#define PDMA_REG_CONF_SIZE                  0x14
#define PDMA_REG_DMA_RESET                  0x18
#define PDMA_REG_DMA_PAUSE                  0x1C
#define PDMA_REG_DMA_STAT                   0x20
#define PDMA_REG_USER_REGION                256

struct pdma_pcie {
	void   *reg_base;
	int    len;
	void   *reg_base2;
	int    len2;
	int    irq;
	int    irq_req_nr;
	int    irq_used_nr;
};

extern void *pdma_get_dev_from_pdev(void *pdev);
extern int  pdma_probe_devices(void);
extern void pdma_disconnect_devices(void);
extern void pdma_pcie_submit_read(void *pdma, u64 addr);
extern void pdma_pcie_submit_write(void *pdma, u64 addr);
extern void pdma_pcie_get_rw_cnt(void *pdma, u16 *rd_cnt, u16 *wt_cnt);
extern int  pdma_pcie_dma_reset(void *pdma);
extern int  pdma_pcie_user_read(void *pdma, unsigned int addr, unsigned int *pval);
extern int  pdma_pcie_user_write(void *pdma, unsigned int addr, unsigned int val);
extern int  pdma_pcie_config_rw_size(void *pdma, unsigned int size);
extern void pdma_pcie_get_dma_stat(void *pdma, unsigned int *pval);
extern int  pdma_pcie_reg_read(void *pdma, unsigned int addr, unsigned int *pval);
extern int  pdma_pcie_reg_write(void *pdma, unsigned int addr, unsigned int val);

#endif
