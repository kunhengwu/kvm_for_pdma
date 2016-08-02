/*
 * pdma-user-intr.c
 * user interrupt handler
 */

/*
 * pcie user read interface
 *
 * @pdma:  pdma device instance
 * @addr:  pcie bar address, starting from word0 of user region
 * @pval:  the read out value will be stored to *pval
 * 
 * return 0 for success, others for failure
 */
extern int pdma_pcie_user_read(void *pdma, unsigned int addr, unsigned int *pval);

/*
 * pcie user write interface
 *
 * @pdma:  pdma device instance
 * @addr:  pcie bar address, starting from word0 of user region
 * @val:   the value that will be written to @addr
 * 
 * return 0 for success, others for failure
 */
extern int pdma_pcie_user_write(void *pdma, unsigned int addr, unsigned int val);


/* 
 * user interrupt handler
 * @pdma:  pdma device instance 
 */
void pdma_user_intr_handler(void *pdma)
{
    /* write here for user interrupt handler */
    ;
}
