/*	$NetBSD: stubs.c,v 1.1.4.2 2001/03/12 13:28:22 bouyer Exp $	*/

/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Routines that are temporary or do not have a home yet.
 *
 * Created      : 17/09/94
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/msgbuf.h>
#include <uvm/uvm_extern.h>
#include <machine/cpu.h>
#include <machine/irqhandler.h>
#include <machine/bootconfig.h>
#include <machine/pcb.h>

extern dev_t dumpdev;
extern BootConfig bootconfig;

/* These queue functions are candiates for arm32/machdep.c */
struct queue {
	struct queue *q_next, *q_prev;
};

/*
 * insert an element into a queue
 */

void
_insque(v1, v2)
	void *v1;
	void *v2;
{
	struct queue *elem = v1, *head = v2;
	struct queue *next;

	next = head->q_next;
	elem->q_next = next;
	head->q_next = elem;
	elem->q_prev = head;
	next->q_prev = elem;
}

/*
 * remove an element from a queue
 */

void
_remque(v)
	void *v;
{
	struct queue *elem = v;
	struct queue *next, *prev;

	next = elem->q_next;
	prev = elem->q_prev;
	next->q_prev = prev;
	prev->q_next = next;
	elem->q_prev = 0;
}


/*
 * These variables are needed by /sbin/savecore
 */
u_long	dumpmag = 0x8fca0101;	/* magic number */
int 	dumpsize = 0;		/* pages */
long	dumplo = 0; 		/* blocks */

struct pcb dumppcb;

/*
 * This is called by main to set dumplo and dumpsize.
 * Dumps always skip the first CLBYTES of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end to
 * reduce the chance that swapping trashes it.
 */

void
cpu_dumpconf()
{
	int nblks;	/* size of dump area */
	int maj;

	if (dumpdev == NODEV)
		return;
	maj = major(dumpdev);
	if (maj < 0 || maj >= nblkdev)
		panic("dumpconf: bad dumpdev=0x%x", dumpdev);
	if (bdevsw[maj].d_psize == NULL)
		return;
	nblks = (*bdevsw[maj].d_psize)(dumpdev);
	if (nblks <= ctod(1))
		return;

	dumpsize = physmem;

	/* Always skip the first CLBYTES, in case there is a label there. */
	if (dumplo < ctod(1))
		dumplo = ctod(1);

	/* Put dump at end of partition, and make it fit. */
	if (dumpsize > dtoc(nblks - dumplo))
		dumpsize = dtoc(nblks - dumplo);
	if (dumplo < nblks - ctod(dumpsize))
		dumplo = nblks - ctod(dumpsize);
}

/* This should be moved to machdep.c */

extern pagehook_t page_hook0;

/*
 * Doadump comes here after turning off memory management and
 * getting on the dump stack, either when called above, or by
 * the auto-restart code.
 */

void
dumpsys()
{
	daddr_t blkno;
	int psize;
	int error;
	int addr;
	int block;
	int len;
	vm_offset_t dumpspace;

	/* Save registers. */
	savectx(&dumppcb);

	if (dumpdev == NODEV)
		return;
	if (dumpsize == 0) {
		cpu_dumpconf();
		if (dumpsize == 0)
			return;
	}
	if (dumplo <= 0) {
		printf("\ndump to dev %u,%u not possible\n", major(dumpdev),
		    minor(dumpdev));
		return;
	}
	printf("\ndumping to dev %u,%u offset %ld\n", major(dumpdev),
	    minor(dumpdev), dumplo);

	blkno = dumplo;
	dumpspace = page_hook0.va;

	psize = (*bdevsw[major(dumpdev)].d_psize)(dumpdev);
	printf("dump ");
	if (psize == -1) {
		printf("area unavailable\n");
		return;
	}

	error = 0;
	len = 0;

	for (block = 0; block < bootconfig.dramblocks && error == 0; ++block) {
		addr = bootconfig.dram[block].address;
		for (;addr < (bootconfig.dram[block].address
		    + (bootconfig.dram[block].pages * NBPG)); addr += NBPG) {
		    	if ((len % (1024*1024)) == 0)
		    		printf("%d ", len / (1024*1024));
	                pmap_map(dumpspace, addr, addr + NBPG, VM_PROT_READ);
			error = (*bdevsw[major(dumpdev)].d_dump)(dumpdev,
			    blkno, (caddr_t) dumpspace, NBPG);
			if (error) break;
			blkno += btodb(NBPG);
			len += NBPG;
		}
	}

	switch (error) {
	case ENXIO:
		printf("device bad\n");
		break;

	case EFAULT:
		printf("device not ready\n");
		break;

	case EINVAL:
		printf("area improper\n");
		break;

	case EIO:
		printf("i/o error\n");
		break;

	case EINTR:
		printf("aborted from console\n");
		break;

	default:
		printf("succeeded\n");
		break;
	}
	printf("\n\n");
	delay(1000000);
}

/* This is interrupt / SPL related */

int current_spl_level = _SPL_HIGH;
u_int spl_masks[_SPL_LEVELS];
u_int spl_smasks[_SPL_LEVELS];
int safepri = _SPL_0;

void
set_spl_masks()
{
	int loop;

	for (loop = 0; loop < _SPL_LEVELS; ++loop) {
		spl_masks[loop] = 0xffffffff;
		spl_smasks[loop] = 0;
	}

	spl_masks[_SPL_BIO]	   = imask[IPL_BIO];
	spl_masks[_SPL_NET]	   = imask[IPL_NET];
	spl_masks[_SPL_SOFTSERIAL] = imask[IPL_TTY];
	spl_masks[_SPL_TTY]	   = imask[IPL_TTY];
	spl_masks[_SPL_IMP]	   = imask[IPL_IMP];
	spl_masks[_SPL_AUDIO]	   = imask[IPL_AUDIO];
	spl_masks[_SPL_CLOCK]	   = imask[IPL_CLOCK];
	spl_masks[_SPL_HIGH]	   = imask[IPL_HIGH];
	spl_masks[_SPL_SERIAL]	   = imask[IPL_SERIAL];

	spl_smasks[_SPL_0] = 0xffffffff;
	for (loop = 0; loop < _SPL_SOFTSERIAL; ++loop)
		spl_smasks[loop] |= SOFTIRQ_BIT(SOFTIRQ_SERIAL);
	for (loop = 0; loop < _SPL_SOFTNET; ++loop)
		spl_smasks[loop] |= SOFTIRQ_BIT(SOFTIRQ_NET);
	for (loop = 0; loop < _SPL_SOFTCLOCK; ++loop)
		spl_smasks[loop] |= SOFTIRQ_BIT(SOFTIRQ_CLOCK);
}

#ifdef DIAGNOSTIC
void
dump_spl_masks()
{
	int loop;

	for (loop = 0; loop < _SPL_LEVELS; ++loop) {
		printf("spl_mask[%d]=%08x splsmask[%d]=%08x\n", loop,
		    spl_masks[loop], loop, spl_smasks[loop]);
	}
}
#endif

/* End of stubs.c */
