/*	$NetBSD: vmparam.h,v 1.5.2.4 2002/06/23 17:33:56 jdolecek Exp $	*/

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

#ifdef _KERNEL

#include <arm/arm32/vmparam.h>

/*
 * Address space constants
 */

/*
 * The line between user space and kernel space
 * Mappings >= KERNEL_BASE are constant across all processes
 */
#define	KERNEL_BASE		0xf0000000

/* Various constants used by the MD code*/
#define	KERNEL_TEXT_BASE	(KERNEL_BASE + 0x00000000)
#define	APTE_BASE		(KERNEL_BASE + 0x00c00000)
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
 * The range 0xf1000000 - 0xf5ffffff is available for kernel VM space
 * Fixed mappings exist from 0xf6000000 - 0xffffffff
 */
#define	KERNEL_VM_SIZE		0x05000000

/*
 * Override the default pager_map size, there's not enough KVA.
 */
#define PAGER_MAP_SIZE		(4 * 1024 * 1024)

/*
 * Size of User Raw I/O map
 */

#define USRIOSIZE       300

/* XXX max. amount of KVM to be used by buffers. */
#ifndef VM_MAX_KERNEL_BUF
#define VM_MAX_KERNEL_BUF \
	((KERNEL_VM_SIZE) * 4 / 10)
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
 *	- RPCDMA for the DMA range on RiscPC's + Kinetic cards
 */

#define	VM_NFREELIST		2
#define	VM_FREELIST_DEFAULT	0
#define	VM_FREELIST_ISADMA	1
#define VM_FREELIST_RPCDMA	1

#endif /* _KERNEL */

#endif	/* _ARM32_VMPARAM_H_ */
