/*	$NetBSD: vmparam.h,v 1.1 2001/09/05 04:53:41 matt Exp $	*/

/*
 * Copyright (c) 1988 The Regents of the University of California.
 * All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 */

#ifndef	_ARM32_VMPARAM_H_
#define	_ARM32_VMPARAM_H_

/* for pt_entry_t definition */
#include <machine/pte.h>

#define	USRTEXT		VM_MIN_ADDRESS
#define	USRSTACK	VM_MAXUSER_ADDRESS

/*
 * Note that MAXTSIZ mustn't be greater than 32M. Otherwise you'd have
 * to change the compiler to not generate bl instructions
 */
#define	MAXTSIZ		(16*1024*1024)		/* max text size */
#ifndef	DFLDSIZ
#define	DFLDSIZ		(128*1024*1024)		/* initial data size limit */
#endif
#ifndef	MAXDSIZ
#define	MAXDSIZ		(512*1024*1024)		/* max data size */
#endif
#ifndef	DFLSSIZ
#define	DFLSSIZ		(2*1024*1024)		/* initial stack size limit */
#endif
#ifndef	MAXSSIZ
#define	MAXSSIZ		(8*1024*1024)		/* max stack size */
#endif

/*
 * Size of shared memory map
 */
#ifndef SHMMAXPGS
#define SHMMAXPGS       1024
#endif

/*
 * The time for a process to be blocked before being very swappable.
 * This is a number of seconds which the system takes as being a non-trivial
 * amount of real time.  You probably shouldn't change this;
 * it is used in subtle ways (fractions and multiples of it are, that is, like
 * half of a `long time'', almost a long time, etc.)
 * It is related to human patience and other factors which don't really
 * change over time.
 */
#define	MAXSLP		20

/*
 * Address space constants
 */

/*
 * The line between user space and kernel space
 * Mappings >= KERNEL_SPACE_START are constant across all processes
 */
#define	KERNEL_SPACE_START	0xa0000000

/* total number of page table entries to map 4GB * size of each entry*/
#define	PAGE_TABLE_SPACE	((1 << (32 - PGSHIFT)) * sizeof(pt_entry_t))

/* Address where the page tables are mapped */
#define	PAGE_TABLE_SPACE_START	(KERNEL_SPACE_START - PAGE_TABLE_SPACE)

/* Various constants used by the MD code*/
#define	KERNEL_BASE		0xa0000000
#define	KERNEL_TEXT_BASE	(KERNEL_BASE + 0x00100000)
#define	ALT_PAGE_TBLS_BASE	(KERNEL_BASE + 0x00c00000)
#define	KERNEL_VM_BASE		(KERNEL_BASE + 0x01000000)
/*
 * The Kernel VM Size varies depending on the machine depending on how
 * much space is needed (and where) for other mappings.
 * In some cases the chosen value may not be the maximum in order that
 * we don't waste memory with kernel pages tables as we can't currently
 * grow the kernel page tables after booting.
 * You only need to increase these values if you find that the number of
 * buffers is being limited due to lack of VA space.
 */
/*
 * The range 0xf1000000 - 0xfcffffff is available for kernel VM space
 * Footbridge registers and I/O mappings occupy 0xfd000000 - 0xffffffff
 */
#define KERNEL_VM_SIZE		0x06000000
#define	PROCESS_PAGE_TBLS_BASE	PAGE_TABLE_SPACE_START

/*
 * Override the default pager_map size, there's not enough KVA.
 */
#define PAGER_MAP_SIZE		(4 * 1024 * 1024)

/*
 * Mach derived constants
 */

#define	VM_MIN_ADDRESS		((vm_offset_t)0x00001000)
#define	VM_MAXUSER_ADDRESS	((vm_offset_t)(PAGE_TABLE_SPACE_START - UPAGES * NBPG))
#define	VM_MAX_ADDRESS		((vm_offset_t)(PAGE_TABLE_SPACE_START + (KERNEL_SPACE_START >> PGSHIFT) * sizeof(pt_entry_t)))

#define	VM_MIN_KERNEL_ADDRESS	((vm_offset_t)KERNEL_TEXT_BASE)
#define	VM_MAXKERN_ADDRESS	((vm_offset_t)(KERNEL_VM_BASE + KERNEL_VM_SIZE))
#define	VM_MAX_KERNEL_ADDRESS	((vm_offset_t)0xffffffff)

/*
 * Size of User Raw I/O map
 */

#define USRIOSIZE       300

/* XXX max. amount of KVM to be used by buffers. */
#ifndef VM_MAX_KERNEL_BUF
#define VM_MAX_KERNEL_BUF \
	((VM_MAXKERN_ADDRESS - KERNEL_VM_BASE) * 4 / 10)
#endif

/* virtual sizes (bytes) for various kernel submaps */

#define VM_PHYS_SIZE		(USRIOSIZE*NBPG)

/*
 * max number of non-contig chunks of physical RAM you can have
 */

#define	VM_PHYSSEG_MAX		32

/*
 * when converting a physical address to a vm_page structure, we
 * want to use a binary search on the chunks of physical memory
 * to find our RAM
 */

#define	VM_PHYSSEG_STRAT	VM_PSTRAT_BSEARCH

/*
 * this indicates that we can't add RAM to the VM system after the
 * vm system is init'd.
 */

#define	VM_PHYSSEG_NOADD

/*
 * we support 2 free lists:
 *
 *	- DEFAULT for all systems
 *	- ISADMA for the ISA DMA range on Sharks only
 */

#define	VM_NFREELIST		2
#define	VM_FREELIST_DEFAULT	0
#define	VM_FREELIST_ISADMA	1

/*
 * define structure pmap_physseg: there is one of these structures
 * for each chunk of noncontig RAM you have.
 */

#define	__HAVE_PMAP_PHYSSEG

struct pmap_physseg {
	struct pv_head *pvhead;		/* pv_entry array */
	char *attrs;			/* attrs array */
};

#endif	/* _ARM32_VMPARAM_H_ */

/* End of vmparam.h */
