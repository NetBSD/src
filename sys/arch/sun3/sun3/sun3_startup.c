#include <machine/cpufunc.h>
#include <machine/mon.h>
#include "vector.h"


unsigned int *old_vector_table;

static void initialize_vector_table()
{
    int i;

    old_vector_table = getvbr();
    for (i = 0; i < NVECTORS; i++) {
	if (vector_table[i] == COPY_ENTRY)
	    vector_table[i] = old_vector_table[i];
    }
    setvbr(vector_table);
}

void sun3_vm_init()
{

    
    /*
     * initialize vpage[2]
     * allocate msgbuf physical memory
     * possibly allocate virtual segment for temp_seg_addr
     * get/compute information about available physical memory
     * get/compute information about start/end of kernel virtual addresses
     */
       

}

void sun3_bootstrap()
{
    static char hello[] = "hello world";
    int i;

    /*
     * would do bzero of bss here but our bzero only works <64k stuff
     * so we've bailed and done it in locore right before this routine :)
     */

    for (i=0; i < 11; i++) {
	mon_putchar(hello[i]);
    }
    mon_printf("\nPROM Version: %d\n", romp->romvecVersion);

    initialize_vector_table();	/* point interrupts/exceptions to our table */

    sun3_vm_init();		/* handle kernel mapping problems, etc */

    pmap_bootstrap();		/*  */
    
    mon_exit_to_mon();
}
