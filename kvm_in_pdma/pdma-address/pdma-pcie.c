
/*
 * pdma-pcie.c
 * pcie driver implementation
 */

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/ioport.h>
#ifdef CONFIG_PCIEAER
#include <linux/aer.h>
#endif
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/sched.h>

#include "pdma-pcie.h"
#include "pdma-dev.h"

static const struct pci_device_id pdma_pci_device_id[] = {
	{0x55AA, 0x6024, PCI_ANY_ID, PCI_ANY_ID},
	{0,}
};
MODULE_DEVICE_TABLE(pci, pdma_pci_device_id);

#define DMA_BIT_MASK(n)	(((n) == 64) ? ~0ULL : ((1ULL<<(n))-1))

/* pdma interrupt handler */
irqreturn_t pdma_intr_handler(int irq, void *dev_id)
{
	pdma_intr_notify(dev_id);
	return IRQ_HANDLED;
}

struct pdma_irq_entry {
	const char *name;
	irq_handler_t handler;
};

static struct pdma_irq_entry pdma_irq_entries[PDMA_IRQ_MAX_REQ_NR] = {
	{"pdma", pdma_intr_handler},
};

static int pdma_pci_install_irq(struct pci_dev *pdev)
{
	void *ppdma;
	struct pdma_pcie *pcie_ctrl;
	int ret, i, j;

#ifndef CONFIG_PCI_MSI
	printk(KERN_ERR "to use pdma, please enable CONFIG_PCI_MSI first\n");
	return -ENODEV;
#endif

	ppdma = pci_get_drvdata(pdev);
	pcie_ctrl = pdma_get_pcie_ctrl(ppdma);
	pcie_ctrl->irq_req_nr = PDMA_IRQ_MAX_REQ_NR;
	pcie_ctrl->irq_used_nr = PDMA_IRQ_USED_NR;

	/* enable msi interrupts */
	if (pci_find_capability(pdev, PCI_CAP_ID_MSI)) {
		ret = pci_enable_msi(pdev);
		if (ret != 0) {
			printk(KERN_ERR "pdma: pci enable msi failed\n");
			return -ENODEV;
		}
		pcie_ctrl->irq = pdev->irq;
	} else {
		printk(KERN_ERR "pdma: msi capabiltiy not enabled\n");
		return -ENODEV;
	}

	/* register pdma irqs*/
	for ( i = 0; i < pcie_ctrl->irq_used_nr; i++ ) {
		ret = request_irq(pcie_ctrl->irq+i, 
                          pdma_irq_entries[i].handler, 0, 
                          pdma_irq_entries[i].name, ppdma);
		if (ret != 0) {
			printk(KERN_ERR "pdma: request %s irq failed\n", 
			       pdma_irq_entries[i].name);
			for (j = 0; j < i; j++) {
				free_irq(pcie_ctrl->irq+j, ppdma);
			}
			pci_disable_msi(pdev);
			return ret;
		}
	}

	return 0;
}

static void pdma_pci_unstall_irq(struct pci_dev *pdev)
{
	void *ppdma;
	struct pdma_pcie *pcie_ctrl;
	int i;

#ifndef CONFIG_PCI_MSI
	return;
#endif

	ppdma = pci_get_drvdata(pdev);
	pcie_ctrl = pdma_get_pcie_ctrl(ppdma);

	for ( i = 0; i < pcie_ctrl->irq_used_nr; i++ ) {
		free_irq(pcie_ctrl->irq+i, ppdma);
	}
	pci_disable_msi(pdev);
}

static int pdma_pcie_setup(struct pci_dev *pdev)
{
	void *ppdma;
	struct pdma_pcie *pcie_ctrl;
	int ret=0;

	ppdma = pci_get_drvdata(pdev);
	pcie_ctrl = pdma_get_pcie_ctrl(ppdma);

	/* enable pcie device */
	ret = pci_enable_device(pdev);
	if (ret) {
		printk(KERN_ERR "pdma: enabe pcie card failed\n");
		goto exit;
	}

	/* request regions */
	ret = pci_request_regions(pdev, PDMA_PCI_DRV_NAME);
	if (ret) {
		printk(KERN_ERR "pdma: request pci regions failed\n");
		goto exit2;
	}

	/* set dma mask */
	if (pci_set_dma_mask(pdev, DMA_BIT_MASK(64)) != 0) {
		ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
		if (ret) {
			printk(KERN_ERR "pdma: set dma mask failed\n");
			goto exit3;
		}
	}

	/* map pci regions */
	pcie_ctrl->reg_base = pci_iomap(pdev, 0, 0);
	if (!pcie_ctrl->reg_base) {
		printk(KERN_ERR "pdma: pci map region failed\n");
		ret = -ENOMEM;
		goto exit3;
	}
	pcie_ctrl->len = pci_resource_len(pdev, 0);

	/* enable pci bus master and pcie error reporting*/
	pci_set_master(pdev);

#ifdef CONFIG_PCIEAER
	pci_enable_pcie_error_reporting(pdev);
#endif
	/* install irq */
	ret = pdma_pci_install_irq(pdev);
	if (ret) {
		printk(KERN_ERR "pdma: install irq failed\n");
		goto exit4;
	}	

	return 0;

exit4:
	pci_iounmap(pdev, pcie_ctrl->reg_base);
exit3:
	pci_release_regions(pdev);
exit2:
	pci_disable_device(pdev);
exit:
	return ret;

}

static void pdma_pcie_cleanup(struct pci_dev *pdev)
{
	void *ppdma;
	struct pdma_pcie *pcie_ctrl;

	ppdma = pci_get_drvdata(pdev);
	pcie_ctrl = pdma_get_pcie_ctrl(ppdma);

	pdma_pci_unstall_irq(pdev);
	pci_iounmap(pdev, pcie_ctrl->reg_base);
	pci_release_regions(pdev);
	pci_disable_device(pdev);
}

static int pdma_pci_probe(struct pci_dev *pdev, 
                          const struct pci_device_id *id)
{
	void *ppdma;
	struct pdma_pcie *pcie_ctrl;
	int ret=0;

	printk("pdma: pcie card probed vendor=0x%x device=0x%x\n", 
	       id->vendor, id->device);

	/* alloc pdma structure */
	ppdma = pdma_alloc_entity();
	if (!ppdma) {
		printk(KERN_ERR "pdma: alloc memory failed\n");
		ret = -ENOMEM;
		goto exit;
	}

	/* alloc pcie ctrl structure */
	pcie_ctrl = kzalloc(sizeof(struct pdma_pcie), GFP_KERNEL);
	if (!pcie_ctrl) {
		printk(KERN_ERR "pdma: alloc memory failed\n");
		ret = -ENOMEM;
		goto exit2;
	}

    /* set structure */
	pci_set_drvdata(pdev, ppdma);
	pdma_set_pcie_ctrl(ppdma, pcie_ctrl);
	pdma_set_pcie_dev(ppdma, pdev);

	/* pcie device setup */
	ret = pdma_pcie_setup(pdev);
	if (ret) {
		goto exit3;
	}

	/* pdma init */
    ret = pdma_dev_init(ppdma);
	if (ret) {
		goto exit4;
	}

    printk("pdma: pcie card init successfully\n");
	
	return 0;

exit4:
	pdma_pcie_cleanup(pdev);
exit3:
	kfree(pcie_ctrl);
exit2:
	pdma_free_entity(ppdma);
exit:
	return ret;
}

static void pdma_pci_remove(struct pci_dev *pdev)
{
	void *ppdma;
	struct pdma_pcie *pcie_ctrl;

	ppdma = pci_get_drvdata(pdev);
	pcie_ctrl = pdma_get_pcie_ctrl(ppdma);

	pdma_dev_exit(ppdma);
	pdma_pcie_cleanup(pdev);
	kfree(pcie_ctrl);
	pdma_free_entity(ppdma);

	printk("pdma: pcie card removed\n");
}

static pci_ers_result_t pdma_pci_error_detected(struct pci_dev *pdev, 
                                                pci_channel_state_t state)
{
	void *ppdma;

	printk(KERN_ERR "pdma: pci error detected %d\n", state);

	ppdma = pci_get_drvdata(pdev);
	
	if (state == pci_channel_io_perm_failure) {
		/* pdma occurred permanent failure */
        printk(KERN_ERR "pdma: pci permanent failure\n");
		return PCI_ERS_RESULT_DISCONNECT;
	}

	/* pdma exit */
    pdma_dev_exit_for_pcie_error(ppdma);

	/* pdma pcie setup */
    pdma_pcie_cleanup(pdev);

	return PCI_ERS_RESULT_NEED_RESET;
}

static pci_ers_result_t pdma_pci_slot_reset(struct pci_dev *pdev)
{
	void *ppdma;
	int ret;

	printk(KERN_ERR "pdma: pci has been reset\n");

	ppdma = pci_get_drvdata(pdev);

	/* pdma pcie setup */
	ret = pdma_pcie_setup(pdev);
	if (ret != 0) {
		return PCI_ERS_RESULT_DISCONNECT;
	}

    /* reinit */
    pdma_dev_init_for_pcie_error(ppdma);

	return PCI_ERS_RESULT_RECOVERED;
}

static void pdma_pci_resume(struct pci_dev *pdev)
{
	void *ppdma;

	printk(KERN_ERR "pdma: pci resume, error recovered\n");

	ppdma = pci_get_drvdata(pdev);

	/* pdma up now */
    ;

	return;
}

void *pdma_get_dev_from_pdev(void *pdev)
{
	struct pci_dev *p;

	p = (struct pci_dev *)pdev;
	return &p->dev;
}

static struct pci_error_handlers pdma_pci_err_handler = {
	.error_detected = pdma_pci_error_detected,
	.slot_reset = pdma_pci_slot_reset,
	.resume = pdma_pci_resume,
};

static struct pci_driver pdma_pci_driver = {
	.name     = PDMA_PCI_DRV_NAME,
	.id_table = pdma_pci_device_id,
	.probe    = pdma_pci_probe,
	.remove   = pdma_pci_remove,
	.err_handler = &pdma_pci_err_handler,
};

int pdma_probe_devices(void)
{
	int ret;

	ret = pci_register_driver(&pdma_pci_driver);

	if (ret == 0) {
		printk("pdma: pcie driver registered successfully\n");
	} else {
		printk(KERN_ERR "pdma: pcie driver registered failed\n");
	}

	return ret;
}

void pdma_disconnect_devices(void)
{
	pci_unregister_driver(&pdma_pci_driver);
	printk("pdma: pcie card driver exited\n");
}

#define pdma_iowrite32(val, reg, plock, flags)        \
    spin_lock_irqsave(plock, flags);                  \
    iowrite32(val, reg);                              \
    spin_unlock_irqrestore(plock, flags);

#define pdma_ioread32(val, reg, plock, flags)         \
    spin_lock_irqsave(plock, flags);                  \
    val = ioread32(reg);                              \
    spin_unlock_irqrestore(plock, flags);

void pdma_pcie_submit_read(void *pdma, u64 addr)
{
    spinlock_t *plock = pdma_get_pcie_lock(pdma);
    struct pdma_pcie *pcie_ctrl;
    unsigned long flags;
	u32 *reg, val;

    pcie_ctrl = pdma_get_pcie_ctrl(pdma);
    reg = (u32 *)((u8 *)pcie_ctrl->reg_base + PDMA_REG_RD_FIFO);    
    addr = cpu_to_le64(addr);

    val = *((u32 *)&addr + 1);
    pdma_iowrite32(val, (void *)(reg + 1), plock, flags);
    val = *((u32 *)&addr);
    pdma_iowrite32(val, (void *)reg, plock, flags);
}

void pdma_pcie_submit_write(void *pdma, u64 addr)
{   

    static int ii=0,jj=0;
    int *pa;
    spinlock_t *plock = pdma_get_pcie_lock(pdma);
    struct pdma_pcie *pcie_ctrl;
    unsigned long flags;
	u32 *reg, val;

    ii++;

    pcie_ctrl = pdma_get_pcie_ctrl(pdma);
    reg = (u32 *)((u8 *)pcie_ctrl->reg_base + PDMA_REG_WT_FIFO); 
    //printk(KERN_INFO"reg :%08x\n",reg);
   
    addr = cpu_to_le64(addr);


   
    val = *((u32 *)&addr + 1);
    pdma_iowrite32(val, (void *)(reg + 1), plock, flags);
    //printk(KERN_INFO"val high:%08x\n",val);

    val = *((u32 *)&addr);
    pdma_iowrite32(val, (void *)reg, plock, flags);
    //printk(KERN_INFO"val low:%08x\n",val);
    
    jj++;

   // printk(KERN_INFO"ii:%d\n",ii);
    if(ii!=jj)
    {
    	printk(KERN_INFO"jj:%d\n",jj);
	
    }
    

   
  /*  pa=addr;


    for(ii=0;ii<16384/8;ii++)
    {
        printk(KERN_INFO"data_out:%08x\n",pa[ii*2]);
    }

  */

}

void pdma_pcie_get_rw_cnt(void *pdma, u16 *rd_cnt, u16 *wt_cnt)
{
    spinlock_t *plock = pdma_get_pcie_lock(pdma);
    struct pdma_pcie *pcie_ctrl;
    unsigned long flags;
	u32 *reg, val;

    pcie_ctrl = pdma_get_pcie_ctrl(pdma);
    reg = (u32 *)((u8 *)pcie_ctrl->reg_base + PDMA_REG_RW_CNT);
    pdma_ioread32(val,(void *)reg, plock, flags);
    val = le32_to_cpu(val);

    *rd_cnt = (val & 0xFFFF);
    *wt_cnt = (val >> 16);
}

int pdma_pcie_dma_reset(void *pdma)
{
    spinlock_t *plock = pdma_get_pcie_lock(pdma);
    struct pdma_pcie *pcie_ctrl;
    unsigned long flags;
    u32 timeout = 500;
	u32 *reg, val, cnt=0;

    /* pause */
    pcie_ctrl = pdma_get_pcie_ctrl(pdma);
    reg = (u32 *)((u8 *)pcie_ctrl->reg_base + PDMA_REG_DMA_PAUSE);
    val = cpu_to_le32(0x03);
    pdma_iowrite32(val, (void *)reg, plock, flags);
    udelay(5);

    while(1) {
        pdma_ioread32(val,(void *)reg, plock, flags);
        val = le32_to_cpu(val);
        if ((val & 0xc) == 0xc) {
            break;
        }
        cnt++;
        if (cnt > timeout) {
            printk("pdma: dma pause timeout\n");
            return -EAGAIN;
        }
        set_current_state(TASK_UNINTERRUPTIBLE);
        schedule_timeout(1);
    };

    /* reset */
    reg = (u32 *)((u8 *)pcie_ctrl->reg_base + PDMA_REG_DMA_RESET);
    cnt = 0;
    timeout = 30;

    spin_lock_irqsave(plock, flags);
    val = cpu_to_le32(0x01);
    iowrite32(val, (void *)reg);
    udelay(3);
    val = ioread32((void *)reg);
    while (val != 0) {
        cnt++;
        if (cnt > timeout) {
            spin_unlock_irqrestore(plock, flags);
            printk("pdma: dma reset timeout\n");
            return -EAGAIN;
        }
        udelay(1);
        val = ioread32((void *)reg);
    }
    spin_unlock_irqrestore(plock, flags);

    return 0;
}

int pdma_pcie_user_read(void *pdma, unsigned int addr, unsigned int *pval)
{
    spinlock_t *plock = pdma_get_pcie_lock(pdma);
    struct pdma_pcie *pcie_ctrl;
    unsigned long flags;
	u32 *reg, val;

    pcie_ctrl = pdma_get_pcie_ctrl(pdma);
    if (addr % 4 != 0 ) {
        printk(KERN_ERR "pdma: addr=0x%x is not word(4Bytes) aligned\n", addr);
        return -EINVAL;
    }
    if (addr + PDMA_REG_USER_REGION >= pcie_ctrl->len ) {
        printk(KERN_ERR "pdma: addr=0x%x is out of range\n", addr);
        return -EINVAL;
    }

    reg = (u32 *)((u8 *)pcie_ctrl->reg_base + PDMA_REG_USER_REGION + addr);
    pdma_ioread32(val,(void *)reg, plock, flags);
    *pval = val;

    return 0;
}

int pdma_pcie_user_write(void *pdma, unsigned int addr, unsigned int val)
{
    spinlock_t *plock = pdma_get_pcie_lock(pdma);
    struct pdma_pcie *pcie_ctrl;
    unsigned long flags;
	u32 *reg;

    pcie_ctrl = pdma_get_pcie_ctrl(pdma);
    if (addr % 4 != 0 ) {
        printk(KERN_ERR "pdma: addr=0x%x is not word(4Bytes) aligned\n", addr);
        return -EINVAL;
    }
    if (addr + PDMA_REG_USER_REGION >= pcie_ctrl->len ) {
        printk(KERN_ERR "pdma: addr=0x%x is out of range\n", addr);
        return -EINVAL;
    }

    reg = (u32 *)((u8 *)pcie_ctrl->reg_base + PDMA_REG_USER_REGION + addr);
    pdma_iowrite32(val, (void *)reg, plock, flags);

    return 0;
}

int pdma_pcie_config_rw_size(void *pdma, unsigned int size)
{
    spinlock_t *plock = pdma_get_pcie_lock(pdma);
    struct pdma_pcie *pcie_ctrl;
    unsigned long flags;
	u32 val, *reg;

    pcie_ctrl = pdma_get_pcie_ctrl(pdma);
    if (size == 0 || size % 4096 != 0 || size > (1024*1024)) {
        printk(KERN_ERR "pdma: size illegal for read/write configuration\n");
        return -EINVAL;
    }

    reg = (u32 *)((u8 *)pcie_ctrl->reg_base + PDMA_REG_CONF_SIZE);
    val = size/4096 - 1;
    val = cpu_to_le32(val);
    pdma_iowrite32(val, (void *)reg, plock, flags);

    return 0;
}

#ifdef PDMA_DEBUG
void pdma_pcie_get_dma_stat(void *pdma, unsigned int *pval)
{
    spinlock_t *plock = pdma_get_pcie_lock(pdma);
    struct pdma_pcie *pcie_ctrl;
    unsigned long flags;
	u32 val, *reg;

    pcie_ctrl = pdma_get_pcie_ctrl(pdma);
    reg = (u32 *)((u8 *)pcie_ctrl->reg_base + PDMA_REG_DMA_STAT);
    pdma_ioread32(val,(void *)reg, plock, flags);
    *pval = le32_to_cpu(val);
}

int pdma_pcie_reg_read(void *pdma, unsigned int addr, unsigned int *pval)
{
    spinlock_t *plock = pdma_get_pcie_lock(pdma);
    struct pdma_pcie *pcie_ctrl;
    unsigned long flags;
	u32 *reg, val;

    pcie_ctrl = pdma_get_pcie_ctrl(pdma);
    reg = (u32 *)((u8 *)pcie_ctrl->reg_base + addr);
    pdma_ioread32(val,(void *)reg, plock, flags);
    *pval = val;

    return 0;
}

int pdma_pcie_reg_write(void *pdma, unsigned int addr, unsigned int val)
{
    spinlock_t *plock = pdma_get_pcie_lock(pdma);
    struct pdma_pcie *pcie_ctrl;
    unsigned long flags;
	u32 *reg;

    pcie_ctrl = pdma_get_pcie_ctrl(pdma);
    reg = (u32 *)((u8 *)pcie_ctrl->reg_base + addr);
    pdma_iowrite32(val, (void *)reg, plock, flags);

    return 0;
}

#else
void pdma_pcie_get_dma_stat(void *pdma, unsigned int *pval)
{
}
int pdma_pcie_reg_read(void *pdma, unsigned int addr, unsigned int *pval)
{
    return 0;
}
int pdma_pcie_reg_write(void *pdma, unsigned int addr, unsigned int val)
{
    return 0;
}
#endif

