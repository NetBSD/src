#include "sys/systm.h"
#include "sys/types.h"
#include "sys/reboot.h"
#include "sys/buf.h"

#include "../include/mon.h"





int cold;

void cpu_startup()
{


    cold = 0;
}


void consinit()
{
    extern void cninit();
    mon_printf("determining console:");
    cninit();
}

void cpu_reset()
{
    mon_reboot("");		/* or do i have to get the value out of
				 * the eeprom...bleh
				 */
}


int waittime = -1;

void boot(howto)
     int howto;
{
    if ((howto&RB_NOSYNC) == 0 && waittime < 0 && bfreelist[0].b_forw) {
	struct buf *bp;
	int iter, nbusy;
	extern struct pcb proc0;

	
	waittime = 0;
	(void) splnet();
	printf("syncing disks... ");
		/*
		 * Release inodes held by texts before update.
		 */
	if (panicstr == 0)
	    vnode_pager_umount(NULL);
	sync(&proc0, (void *) NULL, (int *) NULL);
	
	for (iter = 0; iter < 20; iter++) {
	    nbusy = 0;
	    for (bp = &buf[nbuf]; --bp >= buf; )
		if ((bp->b_flags & (B_BUSY|B_INVAL)) == B_BUSY)
		    nbusy++;
	    if (nbusy == 0)
		break;
	    printf("%d ", nbusy);
	    DELAY(40000 * iter);
	}
	if (nbusy)
	    printf("giving up\n");
	else
	    printf("done\n");
	DELAY(10000);			/* wait for printf to finish */
    }
    resettodr();
    splhigh();
    if (howto&RB_HALT) {
	printf("\n");
	printf("The operating system has halted.\n");
	mon_exit_to_mon();
    } else {
	if (howto & RB_DUMP) {
	    printf("dumping not supported yet :)\n");
	}
    }
    cpu_reset();
    for(;;) ;
    /*NOTREACHED*/
}
