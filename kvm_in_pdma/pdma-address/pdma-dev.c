
/*
 * pdma-dev.c
 * pcie device implementation
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/interrupt.h>

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/cdev.h>
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/ioctl.h>
#include <linux/list.h>

#include <asm/atomic.h>
#include <asm/uaccess.h>

#include "pdma-pcie.h"
#include "pdma-dev.h"
#include "pdma-ioctl.h"

static int major   =   PDMA_MAJOR;
static int minor   =   PDMA_MINOR;
static int max_dev =   PDMA_MAX_DEV;
static char *pool  =   PDMA_POOL;
static char *block =   PDMA_BLOCK;

module_param(major,   int,   S_IRUGO);
module_param(minor,   int,   S_IRUGO);
module_param(max_dev, int,   S_IRUGO);
module_param(pool,    charp, S_IRUGO);
module_param(block,   charp, S_IRUGO);

/*
 * Open and close
 */
int pdma_open(struct inode *inode, struct file *filp)
{
	struct pdma *ppdma; 

	ppdma = container_of(inode->i_cdev, struct pdma, cdev);
	filp->private_data = ppdma; 

	return 0;
}

int pdma_release(struct inode *inode, struct file *filp)
{
	return 0;
}

void pdma_read_fifo_check_before(struct pdma *ppdma)
{
    if (ppdma->rd_submit_cnt > 0 && ppdma->rd_submit_cnt < PDMA_FIFO_WM) {
        static int print_thres = PDMA_FIFO_WM;
        if (ppdma->rd_submit_cnt < print_thres) {
            printk(KERN_WARNING "pdma: warning(before), read fifo not enough address (%d)\n", 
                   ppdma->rd_submit_cnt);
            print_thres = ppdma->rd_submit_cnt;
        }
    } else if (ppdma->rd_submit_cnt == 0) {
        static int print_flag;
        if (print_flag == 0) {
            printk(KERN_ERR "pdma: warning(before), read fifo is empty\n");
            print_flag++;
        }
    }
}

void pdma_read_fifo_check_after(struct pdma *ppdma)
{
    if (ppdma->rd_submit_cnt > 0 && ppdma->rd_submit_cnt < PDMA_FIFO_WM) {
        static int print_thres = PDMA_FIFO_WM;
        if (ppdma->rd_submit_cnt < print_thres) {
            printk(KERN_WARNING "pdma: warning(after), read fifo not enough address (%d)\n", 
                   ppdma->rd_submit_cnt);
            print_thres = ppdma->rd_submit_cnt;
        }
    } else if (ppdma->rd_submit_cnt == 0) {
        static int print_flag;
        if (print_flag == 0) {
            printk(KERN_ERR "pdma: warning(after), read fifo is empty\n");
            print_flag++;
        }
    }
}

void pdma_try_to_submit_read_nolock(struct pdma *ppdma, int check_b)
{
    struct list_head *cur;
    struct pdma_buf *pentry;

    /* sync: check stop */
    if (ppdma->dma_enable == 0) {
        return;
    }

    if (check_b) {
        pdma_read_fifo_check_before(ppdma);
    }

    /* try submit */
    while (ppdma->rd_submit_cnt < PDMA_RD_FIFO_DEPTH && !list_empty(&ppdma->rd_free)) {
        cur = ppdma->rd_free.next;
        pentry = container_of(cur, struct pdma_buf, list);
        list_del_init(cur);
        pdma_stat_rd_free_dec((void *)ppdma);
        pdma_pcie_submit_read((void *)ppdma, pentry->phys_addr);
        list_add_tail(cur, &ppdma->rd_submit);
        pdma_stat_rd_submit_inc((void *)ppdma);
        ppdma->rd_submit_cnt++;
    }

    pdma_read_fifo_check_after(ppdma);
}

void pdma_try_to_submit_write_nolock(struct pdma *ppdma)
{
    struct list_head *cur;
    struct pdma_buf *pentry;

    /* sync: check stop */
    if (ppdma->dma_enable == 0) {
        return;
    }
    while (ppdma->wt_submit_cnt < PDMA_WT_FIFO_DEPTH && !list_empty(&ppdma->wt_pending)) {
        cur = ppdma->wt_pending.next;
        pentry = container_of(cur, struct pdma_buf, list);
        list_del_init(cur);
        pdma_stat_wt_pending_dec((void *)ppdma);
        pdma_pcie_submit_write((void *)ppdma, pentry->phys_addr);
        list_add_tail(cur, &ppdma->wt_submit);
        pdma_stat_wt_submit_inc((void *)ppdma);
        ppdma->wt_submit_cnt++;
       // printk(KERN_INFO"123456789\n");

    }
}

void pdma_try_to_submit_read(struct pdma *ppdma, int check_b)
{
    unsigned long flags;

    spin_lock_irqsave(&ppdma->lock, flags);
    pdma_try_to_submit_read_nolock(ppdma, check_b);
    spin_unlock_irqrestore(&ppdma->lock, flags);
}

void pdma_try_to_submit_write(struct pdma *ppdma)
{
    unsigned long flags;

    spin_lock_irqsave(&ppdma->lock, flags);
    pdma_try_to_submit_write_nolock(ppdma);
    spin_unlock_irqrestore(&ppdma->lock, flags);
}

/*
 * Data management: read and write
 */
ssize_t pdma_read(struct file *filp, char __user *buf, size_t count,
                  loff_t *f_pos)
{
	struct pdma *ppdma = filp->private_data; 
    unsigned long flags;
    int ret = count;

    atomic_inc(&ppdma->rw_cnt);

    /* sync: check stop */
    if (ppdma->dma_enable == 0) {
        ret = -EIO;
        goto exit;
    }

    /* check read legality */
    if (count != ppdma->rd_buf_size) {
        static int print_flag;

        if (print_flag == 0) {
            printk(KERN_ERR "pdma: read size must be equal to %d\n", ppdma->rd_buf_size);
            print_flag++;
        }
        ret = -EINVAL;
        goto exit;
    }

    /* try read */
    spin_lock_irqsave(&ppdma->lock, flags);
try_read:
    if (!list_empty(&ppdma->rd_pending)) {
        struct pdma_buf *pentry;
        struct list_head *cur;

        cur = ppdma->rd_pending.next;
        pentry = container_of(cur, struct pdma_buf, list);
        list_del_init(cur);
        pdma_stat_rd_pending_dec((void *)ppdma);
        spin_unlock_irqrestore(&ppdma->lock, flags);
        if (copy_to_user(buf, pentry->virt_addr, count)) {
            spin_lock_irqsave(&ppdma->lock, flags);
            list_add(cur, &ppdma->rd_pending);
            pdma_stat_rd_pending_inc((void *)ppdma);
            spin_unlock_irqrestore(&ppdma->lock, flags);
            ret = -EFAULT;
            goto exit;
        }
        /*
        int i; 
        for(i=0;i<104;i++)
        {
           printk(KERN_INFO"read_kern_data:%016lx\n",((long long *)pentry->virt_addr)[i]);
        }
        */
        //printk(KERN_INFO"read virt_DATA addr:%x ,%x\n",pentry->phys_addr,pentry->virt_addr);
        
        spin_lock_irqsave(&ppdma->lock, flags);
        list_add_tail(cur, &ppdma->rd_free);
        pdma_stat_rd_free_inc((void *)ppdma);
        spin_unlock_irqrestore(&ppdma->lock, flags);

    } else {
        DEFINE_WAIT(wait);

        spin_unlock_irqrestore(&ppdma->lock, flags);
        if (filp->f_flags & O_NONBLOCK) {
            ret = -EAGAIN;
            goto exit;
        }
        prepare_to_wait(&ppdma->rdque, &wait, TASK_INTERRUPTIBLE);
        if (list_empty(&ppdma->rd_pending)) {
            schedule();
        }
        finish_wait(&ppdma->rdque, &wait);
        if (signal_pending(current)) {
            ret = -ERESTARTSYS;
            goto exit;
        }
        spin_lock_irqsave(&ppdma->lock, flags);

        /* sync: check stop */
        if (ppdma->dma_enable == 0) {
            spin_unlock_irqrestore(&ppdma->lock, flags);
            ret = -EIO;
            goto exit;
        }

        goto try_read;
    }

    /* submit read */
    pdma_try_to_submit_read(ppdma, 1);

exit:
    atomic_dec(&ppdma->rw_cnt);

    return ret;
}
/*
static int my_pdma_pcie_submit_write(void *pdma, u64 addr)
{
        printk(KERN_INFO"my_pdma_pcie_submit_write begin \n");
	pdma_pcie_submit_write(pdma, addr);
        printk(KERN_INFO"my_pdma_pcie_submit_write end \n");	
	return 3;
}*/

ssize_t pdma_write(struct file *filp, const char __user *buf, size_t count,
                   loff_t *f_pos)
{
	struct pdma *ppdma = filp->private_data; 
    unsigned long flags;
    int ret = count;
    

    atomic_inc(&ppdma->rw_cnt);

    /* sync: check stop */
    if (ppdma->dma_enable == 0) {
        ret = -EIO;
        goto exit;
    }

    /* check write legality */
    if (count != ppdma->wt_buf_size) {
        static int print_flag;

        if (print_flag == 0) {
            printk(KERN_ERR "pdma: write size must be equal to %d\n", ppdma->wt_buf_size);
            print_flag++;
        }
        ret = -EINVAL;
        goto exit;
    }

    /* try write */
    spin_lock_irqsave(&ppdma->lock, flags);
try_write:
    if (!list_empty(&ppdma->wt_free)) {
        struct pdma_buf *pentry;
        struct list_head *cur;

        cur = ppdma->wt_free.next;
        pentry = container_of(cur, struct pdma_buf, list);
        list_del_init(cur);
        pdma_stat_wt_free_dec((void *)ppdma);
        spin_unlock_irqrestore(&ppdma->lock, flags);
        if (copy_from_user(pentry->virt_addr, buf, count)) {
            spin_lock_irqsave(&ppdma->lock, flags);
            list_add(cur, &ppdma->wt_free);
            pdma_stat_wt_free_inc((void *)ppdma);
            spin_unlock_irqrestore(&ppdma->lock, flags);
            ret = -EFAULT;
            goto exit;
        }
        int ii,result;
        /*
        for(ii=0;ii<16384/8;ii++)
        {
           printk(KERN_INFO"kern_in:%08x\n",((int *)pentry->virt_addr)[ii*2]);
        }
        */

    
        //printk(KERN_INFO"virt_DATA addr:%016lx ,%016lx\n",pentry->phys_addr,pentry->virt_addr);
       
        spin_lock_irqsave(&ppdma->lock, flags);
        /* sync: check stop */
        if (ppdma->dma_enable == 0) {
            spin_unlock_irqrestore(&ppdma->lock, flags);
            ret = -EIO;
            goto exit;
        }
        //printk(KERN_INFO"ppdma->wt_submit_cnt:%d\n",ppdma->wt_submit_cnt);
        if (ppdma->wt_submit_cnt < PDMA_WT_FIFO_DEPTH) {

            pdma_pcie_submit_write((void *)ppdma, pentry->phys_addr);
            /*
            for(ii=0;ii<16384/8;ii++)
            {
             printk(KERN_INFO"kern_out:%016lx\n",((long long  *)pentry->virt_addr)[ii]);
            }
            */
            list_add_tail(cur, &ppdma->wt_submit);
            pdma_stat_wt_submit_inc((void *)ppdma);
            ppdma->wt_submit_cnt++;
        //printk(KERN_INFO"123ppdma->wt_submit_cnt:%d\n",ppdma->wt_submit_cnt);
        } else {
            list_add_tail(cur, &ppdma->wt_pending);
            pdma_stat_wt_pending_inc((void *)ppdma);
        }
        spin_unlock_irqrestore(&ppdma->lock, flags);

    } else {
        DEFINE_WAIT(wait);

        spin_unlock_irqrestore(&ppdma->lock, flags);
        if (filp->f_flags & O_NONBLOCK) {
            ret = -EAGAIN;
            goto exit;
        }
        prepare_to_wait(&ppdma->wtque, &wait, TASK_INTERRUPTIBLE);
        if (list_empty(&ppdma->wt_free)) {
            schedule();
        }
        finish_wait(&ppdma->wtque, &wait);
        if (signal_pending(current)) {
            ret = -ERESTARTSYS;
            goto exit;
        }
        spin_lock_irqsave(&ppdma->lock, flags);

        /* sync: check stop */
        if (ppdma->dma_enable == 0) {
            spin_unlock_irqrestore(&ppdma->lock, flags);
            ret = -EIO;
            goto exit;
        }

        goto try_write;
    }

exit:
    atomic_dec(&ppdma->rw_cnt);

    return ret;
}

int pdma_dma_stop(struct pdma *ppdma)
{
    unsigned long flags;
    int count = 0, ret = 0;

    /* sync: disable dma */
    spin_lock_irqsave(&ppdma->lock, flags);
    ppdma->dma_enable = 0;
    spin_unlock_irqrestore(&ppdma->lock, flags);

    /* reset */
    ret = pdma_pcie_dma_reset((void *)ppdma);
    if (ret != 0) {
        return ret;
    }

re_wakeup:
    wake_up_interruptible(&ppdma->rdque);
    wake_up_interruptible(&ppdma->wtque);    
    if (atomic_read(&ppdma->rw_cnt) != 0) {
        udelay(1);
        count++;
        /* max wait 1s */
        if (count > 1000000) {
            printk(KERN_ERR "pdma: dma stop failed\n");
            return -EAGAIN;
        }
        goto re_wakeup;
    }

    return 0;
}

void pdma_dma_start(struct pdma *ppdma)
{
    unsigned long flags;

    if (ppdma->dma_enable != 0) {
        pdma_dma_stop(ppdma);
    }

    pdma_pcie_dma_reset((void *)ppdma);

    spin_lock_irqsave(&ppdma->lock, flags);
    pdma_dev_list_init(ppdma);
    ppdma->dma_enable = 1;
    spin_unlock_irqrestore(&ppdma->lock, flags);

    pdma_try_to_submit_read(ppdma, 0);
}


#ifdef PDMA_DEBUG
void pdma_get_stat(struct pdma *ppdma, struct pdma_stat *pstat)
{
    unsigned long total, cnt;
    unsigned int cur, avg, min, max;

    /* current counter */
    pstat->rd_free_cnt = atomic_read(&ppdma->rd_free_atomic);
    pstat->rd_submit_cnt = atomic_read(&ppdma->rd_submit_atomic);
    pstat->rd_pending_cnt = atomic_read(&ppdma->rd_pending_atomic);
    pstat->wt_free_cnt = atomic_read(&ppdma->wt_free_atomic);
    pstat->wt_submit_cnt = atomic_read(&ppdma->wt_submit_atomic);
    pstat->wt_pending_cnt = atomic_read(&ppdma->wt_pending_atomic);

    /* read fifo submit stat */
    total = atomic64_read(&ppdma->rd_fifo_total_atomic);
    cnt = atomic64_read(&ppdma->rd_fifo_cnt_atomic);
    min = atomic_read(&ppdma->rd_fifo_min_atomic);
    cur = atomic_read(&ppdma->rd_submit_atomic);
    if (cnt == 0) {
        avg = cur;
    } else {
        avg = (unsigned int)(total/cnt);
    }

    pstat->rd_fifo_min = (min*100)/PDMA_RD_FIFO_DEPTH;
    if (pstat->rd_fifo_min == 0 && min != 0) {
        pstat->rd_fifo_min = 1;
    }
    pstat->rd_fifo_avg = (avg*100)/PDMA_RD_FIFO_DEPTH;
    if (pstat->rd_fifo_avg == 0 && avg != 0) {
        pstat->rd_fifo_avg = 1;
    }
    pstat->rd_fifo_cur = (cur*100)/PDMA_RD_FIFO_DEPTH;
    if (pstat->rd_fifo_cur == 0 && cur != 0) {
        pstat->rd_fifo_cur = 1;
    }

    /* read pending stat */
    total = atomic64_read(&ppdma->rd_pending_total_atomic);
    cnt = atomic64_read(&ppdma->rd_pending_cnt_atomic);
    max = atomic_read(&ppdma->rd_pending_max_atomic);
    cur = atomic_read(&ppdma->rd_pending_atomic);
    if (cnt == 0) {
        avg = cur;
    } else {
        avg = (unsigned int)(total/cnt);
    }

    pstat->rd_pending_max = (max*100)/ppdma->rd_buf_nr;
    if (pstat->rd_pending_max == 0 && max != 0) {
        pstat->rd_pending_max = 1;
    }
    pstat->rd_pending_avg = (avg*100)/ppdma->rd_buf_nr;
    if (pstat->rd_pending_avg == 0 && avg != 0) {
        pstat->rd_pending_avg = 1;
    }
    pstat->rd_pending_cur = (cur*100)/ppdma->rd_buf_nr;
    if (pstat->rd_pending_cur == 0 && cur != 0) {
        pstat->rd_pending_cur = 1;
    }

    /* write fifo submit stat */
    total = atomic64_read(&ppdma->wt_fifo_total_atomic);
    cnt = atomic64_read(&ppdma->wt_fifo_cnt_atomic);
    max = atomic_read(&ppdma->wt_fifo_max_atomic);
    cur = atomic_read(&ppdma->wt_submit_atomic);
    if (cnt == 0) {
        avg = cur;
    } else {
        avg = (unsigned int)(total/cnt);
    }

    pstat->wt_fifo_max = (max*100)/PDMA_WT_FIFO_DEPTH;
    if (pstat->wt_fifo_max == 0 && max != 0) {
        pstat->wt_fifo_max = 1;
    }
    pstat->wt_fifo_avg = (avg*100)/PDMA_WT_FIFO_DEPTH;
    if (pstat->wt_fifo_avg == 0 && avg != 0) {
        pstat->wt_fifo_avg = 1;
    }
    pstat->wt_fifo_cur = (cur*100)/PDMA_WT_FIFO_DEPTH;
    if (pstat->wt_fifo_cur == 0 && cur != 0) {
        pstat->wt_fifo_cur = 1;
    }

    /* write pending stat */
    total = atomic64_read(&ppdma->wt_pending_total_atomic);
    cnt = atomic64_read(&ppdma->wt_pending_cnt_atomic);
    max = atomic_read(&ppdma->wt_pending_max_atomic);
    cur = atomic_read(&ppdma->wt_pending_atomic);
    if (cnt == 0) {
        avg = cur;
    } else {
        avg = (unsigned int)(total/cnt);
    }

    pstat->wt_pending_max = (max*100)/ppdma->wt_buf_nr;
    if (pstat->wt_pending_max == 0 && max != 0) {
        pstat->wt_pending_max = 1;
    }
    pstat->wt_pending_avg = (avg*100)/ppdma->wt_buf_nr;
    if (pstat->wt_pending_avg == 0 && avg != 0) {
        pstat->wt_pending_avg = 1;
    }
    pstat->wt_pending_cur = (cur*100)/ppdma->wt_buf_nr;
    if (pstat->wt_pending_cur == 0 && cur != 0) {
        pstat->wt_pending_cur = 1;
    }
}
#else
void pdma_get_stat(struct pdma *ppdma, struct pdma_stat *pstat)
{
    memset(pstat, 0, sizeof(struct pdma_stat));
}
#endif

/*
 * The ioctl() implementation
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long pdma_ioctl(struct file *filp, unsigned int cmd, 
                unsigned long arg)
#else
int pdma_ioctl(struct inode *inode, struct file *filp,
               unsigned int cmd, unsigned long arg)
#endif
{
	struct pdma *ppdma = filp->private_data; 
	int ret = 0;
    
	/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	 */
	if (_IOC_TYPE(cmd) != PDMA_IOC_MAGIC) { 
        return -ENOTTY;
    }
	if (_IOC_NR(cmd) > PDMA_IOC_MAXNR) {
        return -ENOTTY;
    }

	switch(cmd) {

    /* read/write register */
    case PDMA_IOC_RW_REG: {
        struct pdma_rw_reg ctrl;        
        if (copy_from_user(&ctrl, (void *)arg, sizeof(struct pdma_rw_reg))) {
            ret = -EFAULT;
            break;
        }

        if (ctrl.type == 0) {
            /* read */
            ret = pdma_pcie_user_read((void *)ppdma, ctrl.addr, &ctrl.val);
            if (ret) {
                break;
            }
            if (copy_to_user((void *)arg, &ctrl, sizeof(struct pdma_rw_reg))) {
                ret = -EFAULT;
                break;
            }
        } else {
            /* write */
            ret = pdma_pcie_user_write((void *)ppdma, ctrl.addr, ctrl.val);
            if (ret) {
                break;
            }
        }

		break;
    }

    /* pdma info */
    case PDMA_IOC_INFO: {
        struct pdma_info info;

        info.rd_pool_sz = (unsigned long)ppdma->rd_buf_nr * ppdma->rd_buf_size;
        info.wt_pool_sz = (unsigned long)ppdma->wt_buf_nr * ppdma->wt_buf_size;
        info.rd_block_sz = ppdma->rd_buf_size;
        info.wt_block_sz = ppdma->wt_buf_size;
        if (copy_to_user((void *)arg, &info, sizeof(struct pdma_info))) {
            ret = -EFAULT;
        }
		break;
    }

    /* start dma */
    case PDMA_IOC_START_DMA:
        pdma_dma_start(ppdma);
		break;

    /* stop dma */
    case PDMA_IOC_STOP_DMA:
        ret = pdma_dma_stop(ppdma);
		break;

    /* pdma stat */
    case PDMA_IOC_STAT: {
        struct pdma_stat stat;

        pdma_get_stat(ppdma, &stat);
        if (copy_to_user((void *)arg, &stat, sizeof(struct pdma_stat))) {
            ret = -EFAULT;
        }
		break;
    }

    /* read/write register for debug */
    case PDMA_IOC_RW_REG_DEBUG: {
        struct pdma_rw_reg ctrl;        
        if (copy_from_user(&ctrl, (void *)arg, sizeof(struct pdma_rw_reg))) {
            ret = -EFAULT;
            break;
        }

        if (ctrl.type == 0) {
            /* read */
            ret = pdma_pcie_reg_read((void *)ppdma, ctrl.addr, &ctrl.val);
            if (ret) {
                break;
            }
            if (copy_to_user((void *)arg, &ctrl, sizeof(struct pdma_rw_reg))) {
                ret = -EFAULT;
                break;
            }
        } else {
            /* write */
            ret = pdma_pcie_reg_write((void *)ppdma, ctrl.addr, ctrl.val);
            if (ret) {
                break;
            }
        }

		break;
    }

    default:  /* redundant, as cmd was checked against MAXNR */
		return -ENOTTY;
	}

	return ret;
}

struct file_operations pdma_fops = {
	.owner          =  THIS_MODULE,
	.read           =  pdma_read,
	.write          =  pdma_write,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
    .unlocked_ioctl =  pdma_ioctl,
#else
	.ioctl          =  pdma_ioctl,
#endif
	.open           =  pdma_open,
	.release        =  pdma_release,
};

void pdma_intr_notify(void *pdma)
{
	struct pdma *ppdma = (struct pdma *)pdma;
    struct list_head *cur;
    unsigned long flags;
    u16 rd_cnt, wt_cnt, i;
    //static int count=0;

    spin_lock_irqsave(&ppdma->lock, flags);

    /* sync: check stop */
    if (ppdma->dma_enable == 0) {
        spin_unlock_irqrestore(&ppdma->lock, flags);
        goto exit;
    }

    pdma_pcie_get_rw_cnt(pdma, &rd_cnt, &wt_cnt);
    /*++count;
    if(count>=490000)
    {
      printk(KERN_INFO"count:%d\n",count);
    }*/
    //printk(KERN_INFO"wt_cnt:%d rd_cnt:%d\n",wt_cnt,rd_cnt);
    if (rd_cnt != 0) {        
        for (i = 0; i < rd_cnt; i++) {
            cur = ppdma->rd_submit.next;
            list_del_init(cur);
            pdma_stat_rd_submit_dec((void *)ppdma);
            list_add_tail(cur, &ppdma->rd_pending);
            pdma_stat_rd_pending_inc((void *)ppdma);
        }
        //printk(KERN_INFO"read_rd_cnt:%d\n",rd_cnt);
        ppdma->rd_submit_cnt -= rd_cnt;
        pdma_try_to_submit_read_nolock(ppdma, 1);
    }
    
    if (wt_cnt != 0) {
        for (i = 0; i < wt_cnt; i++) {
            cur = ppdma->wt_submit.next;
            list_del_init(cur);
            pdma_stat_wt_submit_dec((void *)ppdma);
            list_add_tail(cur, &ppdma->wt_free);
            pdma_stat_wt_free_inc((void *)ppdma);
        }   
        //printk(KERN_INFO"wt_cnt:%d\n",wt_cnt);
        //printk(KERN_INFO"rd_cnt:%d\n",rd_cnt);
        //printk(KERN_INFO"wt_submit_cnt:%d\n",ppdma->wt_submit_cnt);
        ppdma->wt_submit_cnt -= wt_cnt;
        pdma_try_to_submit_write_nolock(ppdma);
    }
    spin_unlock_irqrestore(&ppdma->lock, flags);

    /* wakeup read and write */
    if (rd_cnt != 0) {
        wake_up_interruptible(&ppdma->rdque);
    }
    if (wt_cnt != 0) {
        wake_up_interruptible(&ppdma->wtque);
    }

exit:
    pdma_user_intr_handler(pdma);
}

/*
 * rely on pool and block
 */
int pdma_input_check_and_parse(struct pdma *ppdma)
{
    char *str;
    long pool_sz;
    int block_sz;

    /* parse and check input */
    str = block;
    block_sz = memparse(str, &str);
    if (block_sz == 0 || block_sz % 4096 != 0 || block_sz > (1024*1024)) {
        printk(KERN_ERR "pdma: illegal input, block=%s\n", block);
        return -EINVAL;
    }

    str = pool;
    pool_sz = memparse(str, &str);
    if (pool_sz == 0 || pool_sz % block_sz != 0) {
        printk(KERN_ERR "pdma: illegal input, pool=%s\n", pool);
        return -EINVAL;
    }

    /* init pool info */
    ppdma->rd_buf_nr = pool_sz / block_sz;
    ppdma->rd_buf_size = block_sz;
    ppdma->wt_buf_nr = pool_sz / block_sz;
    ppdma->wt_buf_size = block_sz;

    return 0;
}

void pdma_dev_free_pool(struct pdma *ppdma)
{
    struct pdma_buf *buf;
    void *dev;
    int i;

    dev = pdma_get_dev(ppdma);

    /* free read pool */
    if (ppdma->rd_buf_a) {
        for (i = 0; i < ppdma->rd_buf_nr; i++) {
            buf = &ppdma->rd_buf_a[i];
            if (buf->virt_addr) {
                dma_free_coherent(dev, 
                                  ppdma->rd_buf_size, 
                                  buf->virt_addr,
                                  buf->phys_addr);
            }
        }
        vfree(ppdma->rd_buf_a);
    }

    /* free write pool */
    if (ppdma->wt_buf_a) {
        for (i = 0; i < ppdma->wt_buf_nr; i++) {
            buf = &ppdma->wt_buf_a[i];
            if (buf->virt_addr) {
                dma_free_coherent(dev, 
                                  ppdma->wt_buf_size, 
                                  buf->virt_addr,
                                  buf->phys_addr);
            }
        }
        vfree(ppdma->wt_buf_a);
    }
}

int pdma_dev_alloc_pool(struct pdma *ppdma)
{
    struct pdma_buf *buf;
    void *dev;
    int i;

    dev = pdma_get_dev(ppdma);

    /* alloc read pool */
    ppdma->rd_buf_a = vmalloc(ppdma->rd_buf_nr * sizeof(struct pdma_buf));
    if (!ppdma->rd_buf_a) {
        printk(KERN_ERR "pdma: alloc control for read pool failed\n");
        pdma_dev_free_pool(ppdma);
        return -ENOMEM;        
    }
    memset(ppdma->rd_buf_a, 0, ppdma->rd_buf_nr * sizeof(struct pdma_buf));
    for (i = 0; i < ppdma->rd_buf_nr; i++) {
        buf = &ppdma->rd_buf_a[i];
        buf->virt_addr = dma_alloc_coherent(dev, 
                                            ppdma->rd_buf_size, 
                                            &buf->phys_addr, 
                                            GFP_KERNEL);
        if (!buf->virt_addr) {
            printk(KERN_ERR "pdma: alloc read pool failed\n");
            pdma_dev_free_pool(ppdma);
            return -ENOMEM;
        }
    }

    /* alloc write pool */
    ppdma->wt_buf_a = vmalloc(ppdma->wt_buf_nr * sizeof(struct pdma_buf));
    if (!ppdma->wt_buf_a) {
        printk(KERN_ERR "pdma: alloc control for write pool failed\n");
        pdma_dev_free_pool(ppdma);
        return -ENOMEM;        
    }
    memset(ppdma->wt_buf_a, 0, ppdma->wt_buf_nr * sizeof(struct pdma_buf));
    for (i = 0; i < ppdma->wt_buf_nr; i++) {
        buf = &ppdma->wt_buf_a[i];
        buf->virt_addr = dma_alloc_coherent(dev, 
                                            ppdma->wt_buf_size, 
                                            &buf->phys_addr, 
                                            GFP_KERNEL);
        if (!buf->virt_addr) {
            printk(KERN_ERR "pdma: alloc write pool failed\n");
            pdma_dev_free_pool(ppdma);
            return -ENOMEM;
        }
    }

    return 0;
}

void pdma_dev_list_init(struct pdma *ppdma)
{
    struct pdma_buf *buf;
    int i;

    /* init list */
    INIT_LIST_HEAD(&ppdma->rd_free);
    INIT_LIST_HEAD(&ppdma->rd_submit);
    INIT_LIST_HEAD(&ppdma->rd_pending);
    INIT_LIST_HEAD(&ppdma->wt_free);
    INIT_LIST_HEAD(&ppdma->wt_submit);
    INIT_LIST_HEAD(&ppdma->wt_pending);
    ppdma->rd_submit_cnt = 0;
    ppdma->wt_submit_cnt = 0;
    pdma_stat_init((void *)ppdma);
    pdma_stat_his_init((void *)ppdma);

    /* read pool */
    for (i = 0; i < ppdma->rd_buf_nr; i++) {
        buf = &ppdma->rd_buf_a[i];
        INIT_LIST_HEAD(&buf->list);
        list_add_tail(&buf->list, &ppdma->rd_free);
        pdma_stat_rd_free_inc((void *)ppdma);
    }

    /* write pool */
    for (i = 0; i < ppdma->wt_buf_nr; i++) {
        buf = &ppdma->wt_buf_a[i];
        INIT_LIST_HEAD(&buf->list);
        list_add_tail(&buf->list, &ppdma->wt_free);
        pdma_stat_wt_free_inc((void *)ppdma);
    }
}

void pdma_dev_ctrl_init(struct pdma *ppdma)
{
    ppdma->dma_enable = 0;

    atomic_set(&ppdma->rw_cnt, 0);
    init_waitqueue_head(&ppdma->rdque);
    init_waitqueue_head(&ppdma->wtque);

    spin_lock_init(&ppdma->pcie_lock);
    spin_lock_init(&ppdma->lock);

    pdma_dev_list_init(ppdma);
}

int pdma_dev_register(struct pdma *ppdma)
{
	int ret;
	dev_t dev;
    static int minor_idx = 0;

    if (minor_idx >= max_dev) {
		printk(KERN_ERR "pdma: device number exceeded the max number "
                        "which driver can support\n");
        return -ENOSPC;
    }

    dev = MKDEV(major, (minor + minor_idx));

	cdev_init(&ppdma->cdev, &pdma_fops);
	ppdma->cdev.owner = THIS_MODULE;
	ppdma->cdev.ops = &pdma_fops;
	ret = cdev_add(&ppdma->cdev, dev, 1);
	if (ret) {
		printk(KERN_ERR "pdma: adding pdma device failed, with major=%d minor=%d\n", 
               major, minor + minor_idx);
        return ret;
    }

    minor_idx++;

    return 0;
}

void pdma_dev_unregister(struct pdma *ppdma)
{
	cdev_del(&ppdma->cdev);
}


int pdma_dev_init(void *pdma)
{
	struct pdma *ppdma;
    int ret;

	ppdma = (struct pdma *)pdma;

    /* input check and parse */
    ret = pdma_input_check_and_parse(ppdma);
    if (ret) {
        return ret;
    }

    /* alloc pool */
    ret = pdma_dev_alloc_pool(ppdma);
    if (ret) {
        return ret;
    }

    /* init control */
    pdma_dev_ctrl_init(ppdma);

    /* config register */
    ret = pdma_pcie_config_rw_size(pdma, ppdma->rd_buf_size);
    if (ret) {
        pdma_dev_free_pool(ppdma);
        return ret;
    }

    /* 
     * CAREME: start dma here 
     * if don't start dma here, please set dma_enable to 1
     */
    //pdma_dma_start(ppdma);

    /* register device */
    ret = pdma_dev_register(ppdma);
    if (ret) {
        pdma_dev_free_pool(ppdma);
        return ret;
    }

    return 0;
}

void pdma_dev_exit(void *pdma)
{
	struct pdma *ppdma;

	ppdma = (struct pdma *)pdma;

    pdma_dev_unregister(ppdma);
    pdma_dma_stop(ppdma); //CAREME
    pdma_dev_free_pool(ppdma);

    return;
}

void pdma_dev_init_for_pcie_error(void *pdma)
{
	struct pdma *ppdma;

	ppdma = (struct pdma *)pdma;

    /* reinit */
    pdma_dev_ctrl_init(ppdma);

    /* config register */
    pdma_pcie_config_rw_size(pdma, ppdma->rd_buf_size);

    /* start dma */
    pdma_dma_start(ppdma);
}

void pdma_dev_exit_for_pcie_error(void *pdma)
{
    pdma_dma_stop(pdma); //CAREME
}

/* 
 * rely on major, minor, max_dev
 */
int pdma_register_region(void)
{
	int ret;
	dev_t dev = 0;

	if (major) {
		dev = MKDEV(major, minor);
		ret = register_chrdev_region(dev, max_dev, PDMA_DEV_NAME);
	} else {
		ret = alloc_chrdev_region(&dev, minor, max_dev, PDMA_DEV_NAME);
		major = MAJOR(dev);
	}
	if (ret < 0) {
		printk(KERN_WARNING "pdma: can't get major %d\n", major);
        return ret;
	}

    return 0;
}

void pdma_unregister_region(void)
{

    dev_t dev = MKDEV(major, minor);

    unregister_chrdev_region(dev, max_dev);
}

static int __init pdma_init(void)
{
    int ret;

    ret = pdma_register_region();
    if (ret) {
        return ret;
    }

	ret = pdma_probe_devices();
    if (ret) {
        pdma_unregister_region();
        return ret;
    }

    return 0;
}

static void __exit pdma_cleanup(void)
{
	pdma_disconnect_devices();
    pdma_unregister_region();
}

/* 
 * object interfaces 
 */
void *pdma_alloc_entity(void)
{
    struct pdma *ppdma;

    ppdma = kzalloc(sizeof(struct pdma), GFP_KERNEL);
    return (void *)ppdma;
}

void pdma_free_entity(void *ppdma)
{
    kfree(ppdma);
}

void *pdma_get_pcie_ctrl(void *pdma)
{
	struct pdma *ppdma;

	ppdma = (struct pdma *)pdma;
	return ppdma->pcie_ctrl;
}

void pdma_set_pcie_ctrl(void *pdma, void *pcie_ctrl)
{
	struct pdma *ppdma;

	ppdma = (struct pdma *)pdma;
	ppdma->pcie_ctrl = pcie_ctrl;
}

void *pdma_get_pcie_dev(void *pdma)
{
	struct pdma *ppdma;

	ppdma = (struct pdma *)pdma;
	return ppdma->pdev;
}

void pdma_set_pcie_dev(void *pdma, void *pdev)
{
	struct pdma *ppdma;

	ppdma = (struct pdma *)pdma;
	ppdma->pdev = pdev;
}

void *pdma_get_dev(void *pdma)
{
	void *pdev;

	pdev = pdma_get_pcie_dev(pdma);
	return pdma_get_dev_from_pdev(pdev);
}

spinlock_t *pdma_get_pcie_lock(void *pdma)
{
	struct pdma *ppdma;

	ppdma = (struct pdma *)pdma;
    return &ppdma->pcie_lock;
}

/* 
 * statistic interfaces 
 */
#ifdef PDMA_DEBUG
void pdma_stat_init(void *pdma)
{
	struct pdma *ppdma = (struct pdma *)pdma;

    atomic_set(&ppdma->rd_free_atomic, 0);
    atomic_set(&ppdma->rd_submit_atomic, 0);
    atomic_set(&ppdma->rd_pending_atomic, 0);
    atomic_set(&ppdma->wt_free_atomic, 0);
    atomic_set(&ppdma->wt_submit_atomic, 0);
    atomic_set(&ppdma->wt_pending_atomic, 0);
}

void pdma_stat_his_init(void *pdma)
{
	struct pdma *ppdma = (struct pdma *)pdma;

    atomic64_set(&ppdma->rd_fifo_total_atomic, 0);
    atomic64_set(&ppdma->rd_fifo_cnt_atomic, 0);
    atomic_set(&ppdma->rd_fifo_min_atomic, 0); //CAREME
    atomic64_set(&ppdma->rd_pending_total_atomic, 0);
    atomic64_set(&ppdma->rd_pending_cnt_atomic, 0);
    atomic_set(&ppdma->rd_pending_max_atomic, 0);
    atomic64_set(&ppdma->wt_fifo_total_atomic, 0);
    atomic64_set(&ppdma->wt_fifo_cnt_atomic, 0);
    atomic_set(&ppdma->wt_fifo_max_atomic, 0);
    atomic64_set(&ppdma->wt_pending_total_atomic, 0);
    atomic64_set(&ppdma->wt_pending_cnt_atomic, 0);
    atomic_set(&ppdma->wt_pending_max_atomic, 0);
}

void pdma_stat_rd_free_inc(void *pdma)
{
	struct pdma *ppdma = (struct pdma *)pdma;

    atomic_inc(&ppdma->rd_free_atomic);
}
void pdma_stat_rd_submit_inc(void *pdma)
{
	struct pdma *ppdma = (struct pdma *)pdma;

    atomic_inc(&ppdma->rd_submit_atomic);
}
void pdma_stat_rd_pending_inc(void *pdma)
{
	struct pdma *ppdma = (struct pdma *)pdma;
    int val;

    atomic_inc(&ppdma->rd_pending_atomic);

    val = atomic_read(&ppdma->rd_pending_atomic);
    atomic64_add(val, &ppdma->rd_pending_total_atomic);
    atomic64_inc(&ppdma->rd_pending_cnt_atomic);
    if (val > atomic_read(&ppdma->rd_pending_max_atomic)) {
        atomic_set(&ppdma->rd_pending_max_atomic, val);
    }
}
void pdma_stat_rd_free_dec(void *pdma)
{
	struct pdma *ppdma = (struct pdma *)pdma;

    atomic_dec(&ppdma->rd_free_atomic);
}
void pdma_stat_rd_submit_dec(void *pdma)
{
	struct pdma *ppdma = (struct pdma *)pdma;
    int val;

    atomic_dec(&ppdma->rd_submit_atomic);

    val = atomic_read(&ppdma->rd_submit_atomic);
    atomic64_add(val, &ppdma->rd_fifo_total_atomic);
    atomic64_inc(&ppdma->rd_fifo_cnt_atomic);
    if (val < atomic_read(&ppdma->rd_fifo_min_atomic) || 
        atomic64_read(&ppdma->rd_fifo_cnt_atomic) == 1) {
        atomic_set(&ppdma->rd_fifo_min_atomic, val);
    }
}
void pdma_stat_rd_pending_dec(void *pdma)
{
	struct pdma *ppdma = (struct pdma *)pdma;

    atomic_dec(&ppdma->rd_pending_atomic);
}
void pdma_stat_wt_free_inc(void *pdma)
{
	struct pdma *ppdma = (struct pdma *)pdma;

    atomic_inc(&ppdma->wt_free_atomic);
}
void pdma_stat_wt_submit_inc(void *pdma)
{
	struct pdma *ppdma = (struct pdma *)pdma;
    int val;

    atomic_inc(&ppdma->wt_submit_atomic);

    val = atomic_read(&ppdma->wt_submit_atomic);
    atomic64_add(val, &ppdma->wt_fifo_total_atomic);
    atomic64_inc(&ppdma->wt_fifo_cnt_atomic);
    if (val > atomic_read(&ppdma->wt_fifo_max_atomic)) {
        atomic_set(&ppdma->wt_fifo_max_atomic, val);
    }
}
void pdma_stat_wt_pending_inc(void *pdma)
{
	struct pdma *ppdma = (struct pdma *)pdma;
    int val;

    atomic_inc(&ppdma->wt_pending_atomic);

    val = atomic_read(&ppdma->wt_pending_atomic);
    atomic64_add(val, &ppdma->wt_pending_total_atomic);
    atomic64_inc(&ppdma->wt_pending_cnt_atomic);
    if (val > atomic_read(&ppdma->wt_pending_max_atomic)) {
        atomic_set(&ppdma->wt_pending_max_atomic, val);
    }
}
void pdma_stat_wt_free_dec(void *pdma)
{
	struct pdma *ppdma = (struct pdma *)pdma;

    atomic_dec(&ppdma->wt_free_atomic);
}
void pdma_stat_wt_submit_dec(void *pdma)
{
	struct pdma *ppdma = (struct pdma *)pdma;

    atomic_dec(&ppdma->wt_submit_atomic);
}
void pdma_stat_wt_pending_dec(void *pdma)
{
	struct pdma *ppdma = (struct pdma *)pdma;

    atomic_dec(&ppdma->wt_pending_atomic);
}
#else
void pdma_stat_init(void *pdma) {}
void pdma_stat_his_init(void *pdma) {}
void pdma_stat_rd_free_inc(void *pdma) {}
void pdma_stat_rd_submit_inc(void *pdma) {}
void pdma_stat_rd_pending_inc(void *pdma) {}
void pdma_stat_rd_free_dec(void *pdma) {}
void pdma_stat_rd_submit_dec(void *pdma) {}
void pdma_stat_rd_pending_dec(void *pdma) {}
void pdma_stat_wt_free_inc(void *pdma) {}
void pdma_stat_wt_submit_inc(void *pdma) {}
void pdma_stat_wt_pending_inc(void *pdma) {}
void pdma_stat_wt_free_dec(void *pdma) {}
void pdma_stat_wt_submit_dec(void *pdma) {}
void pdma_stat_wt_pending_dec(void *pdma) {}
#endif


MODULE_AUTHOR("PDMA");
MODULE_DESCRIPTION("PCIe DMA");
MODULE_LICENSE("GPL");
module_init(pdma_init);
module_exit(pdma_cleanup);

