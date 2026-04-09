/*	$NetBSD: dumpsys.c,v 1.1 2026/04/09 12:49:35 thorpej Exp $	*/

/*      
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990, 1993
 *      The Regents of the University of California.  All rights reserved.
 *       
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:     
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *                      
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.         
 *                      
 *      from: Utah Hdr: machdep.c 1.74 92/12/20
 *      from: @(#)machdep.c     8.10 (Berkeley) 4/20/94
 */     

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dumpsys.c,v 1.1 2026/04/09 12:49:35 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/core.h>
#include <sys/kcore.h>

#include <machine/cpu.h>
#include <machine/kcore.h>
#include <machine/reg.h>
#include <machine/pcb.h>

#include <m68k/seglist.h>

#include <uvm/uvm_extern.h>

/*
 * Compute the size of the machine-dependent crash dump header.
 * Returns size in disk blocks.
 */
#define	CHDRSIZE  (ALIGN(sizeof(kcore_seg_t)) + ALIGN(sizeof(cpu_kcore_hdr_t)))
#define	MDHDRSIZE roundup(CHDRSIZE, dbtob(1))

static int
cpu_dumpsize(void)
{
	int sz = btodb(MDHDRSIZE);

#ifdef __HAVE_M68K_PMAP_DUMPSYS
	sz += pmap_dumpsize();
#endif

	return sz;
}

/*
 * Calculate size of RAM (in pages) to be dumped.
 */
static u_long
cpu_dump_mempagecnt(void)
{
	u_long i, n;
	size_t size;

	for (n = 0, i = 0; i < VM_PHYSSEG_MAX; i++) {
		size = phys_seg_list[i].ps_end - phys_seg_list[i].ps_start;
		if (size == 0) {
			continue;
		}
		n += atop(size);
	}
	return n;
}

/*
 * Initialize machine-dependent portion of the crash dump header.
 */
static void
cpu_init_kcore_hdr(cpu_kcore_hdr_t *chdr)
{
	phys_ram_seg_t *ram_segs = pmap_init_kcore_hdr(chdr);
	size_t size;
	int i, j;

	for (i = 0, j = 0; i < VM_PHYSSEG_MAX; i++) {
		size = phys_seg_list[i].ps_end - phys_seg_list[i].ps_start;
		if (size == 0) {
			continue;
		}
		ram_segs[j].start = phys_seg_list[i].ps_start;
		ram_segs[j].size = size;
		j++;
	}
}

/*
 * Called by dumpsys() to dump the crash dump header.
 */
static int
cpu_dump(int (*dump)(dev_t, daddr_t, void *, size_t), daddr_t *blknop)
{
	int buf[MDHDRSIZE / sizeof(int)];
	cpu_kcore_hdr_t *chdr;
	kcore_seg_t *kseg;
	int error;

	kseg = (kcore_seg_t *)buf;
	chdr = (cpu_kcore_hdr_t *)&buf[ALIGN(sizeof(kcore_seg_t)) /
	    sizeof(int)];

	/* Create the segment header. */
	CORE_SETMAGIC(*kseg, KCORE_MAGIC, MID_MACHINE, CORE_CPU);
	kseg->c_size = MDHDRSIZE - ALIGN(sizeof(kcore_seg_t));

	cpu_init_kcore_hdr(chdr);

	error = (*dump)(dumpdev, *blknop, (void *)buf, sizeof(buf));
	*blknop += btodb(sizeof(buf));

	return error;
}

/*
 * These variables are needed by /sbin/savecore
 */
uint32_t dumpmag = 0x8fca0101;	/* magic number */
int	dumpsize = 0;		/* pages */
long	dumplo = 0;		/* blocks */

/*
 * This is called by main to set dumplo and dumpsize.
 * Dumps always skip the first PAGE_SIZE of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end to
 * reduce the chance that swapping trashes it.
 */
void
cpu_dumpconf(void)
{
	int nblks, dumpblks;	/* size of dump area */
	u_long npgs;

	if (dumpdev == NODEV) {
		goto bad;
	}
	nblks = bdev_size(dumpdev);
	if (nblks <= ctod(1)) {
		goto bad;
	}

	dumpblks = cpu_dumpsize();
	if (dumpblks < 0) {
		goto bad;
	}
	npgs = cpu_dump_mempagecnt();
	dumpblks += ctod(npgs);

	/* If dump won't fit (incl. room for possible label), punt. */
	if (dumpblks > (nblks - ctod(1))) {
		goto bad;
	}

	/* Put dump at end of partition */
	dumplo = nblks - dumpblks;

	/* dumpsize is in page units, and doesn't include headers. */
	dumpsize = npgs;
	return;

 bad:
	dumpsize = 0;
}

/*
 * Dump physical memory onto the dump device.  Called by cpu_reboot().
 */
void
dumpsys(void)
{
	const struct bdevsw *bdev;
	u_long memcl;
	u_long maddr;
	int psize, todo, npgs;
	daddr_t blkno;
	int (*dump)(dev_t, daddr_t, void *, size_t);
	int error;

	/* XXX Should save registers. */

	if (dumpdev == NODEV) {
		return;
	}
	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL || bdev->d_psize == NULL) {
		return;
	}

	/*
	 * For dumps during autoconfiguration,
	 * if dump device has already configured...
	 */
	if (dumpsize == 0) {
		cpu_dumpconf();
	}
	if (dumplo <= 0) {
		printf("\ndump to dev %u,%u not possible\n",
		    major(dumpdev), minor(dumpdev));
		return;
	}
	printf("\ndumping to dev %u,%u offset %ld\n",
	    major(dumpdev), minor(dumpdev), dumplo);

	psize = bdev_size(dumpdev);
	if (psize == -1) {
		printf("dump area unavailable\n");
		return;
	}

	/* XXX should purge all outstanding keystrokes. */

	dump = bdev->d_dump;
	blkno = dumplo;
	todo = dumpsize;	/* pages */

	if ((error = cpu_dump(dump, &blkno)) != 0) {
		goto err;
	}

#ifdef __HAVE_M68K_PMAP_DUMPSYS
	if ((error = pmap_dumpsys(dump, &blkno)) != 0) {
		goto err;
	}
#endif

	for (memcl = 0; memcl < VM_PHYSSEG_MAX; memcl++) {
		maddr = phys_seg_list[memcl].ps_start;
		npgs = m68k_btop(phys_seg_list[memcl].ps_end -
				 phys_seg_list[memcl].ps_start);

		while (npgs && todo) {
			/* Print pages left after every 16. */
			if (todo == dumpsize || (todo & 0xf) == 0) {
				printf_nostamp("\rpages %6d", todo);
			}

			pmap_kenter_pa((vaddr_t)vmmap, maddr, VM_PROT_READ, 0);
			pmap_update(pmap_kernel());

			error = (*dump)(dumpdev, blkno, vmmap, PAGE_SIZE);

			pmap_kremove((vaddr_t)vmmap, PAGE_SIZE);
			pmap_update(pmap_kernel());

			if (error) {
				goto err;
			}

			maddr += PAGE_SIZE;
			blkno += btodb(PAGE_SIZE);
			npgs--;
			todo--;
		}
	}

 err:
	printf_nostamp("\rdump ");
	switch (error) {
	case ENXIO:
		printf_nostamp("device bad\n");
		break;

	case EFAULT:
		printf_nostamp("device not ready\n");
		break;

	case EINVAL:
		printf_nostamp("area improper\n");
		break;

	case EIO:
		printf_nostamp("i/o error\n");
		break;

	case EINTR:
		printf_nostamp("aborted from console\n");
		break;

	case 0:
		printf_nostamp("succeeded\n");
		break;

	default:
		printf_nostamp("error %d\n", error);
		break;
	}
	printf_nostamp("\n\n");
	delay(5000);
}
