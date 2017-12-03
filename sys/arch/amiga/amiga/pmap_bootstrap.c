/*	$NetBSD: pmap_bootstrap.c,v 1.9.22.1 2017/12/03 11:35:48 jdolecek Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1991 Regents of the University of California.
 * All rights reserved.
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
 *	@(#)pmap.c	7.5 (Berkeley) 5/10/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pmap_bootstrap.c,v 1.9.22.1 2017/12/03 11:35:48 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>

#include <uvm/uvm.h>

#include <machine/pte.h>
#include <machine/cpu.h>
#include <machine/vmparam.h>

#include <m68k/cacheops.h>

#include <amiga/amiga/memlist.h>

extern paddr_t		avail_start;
extern paddr_t		avail_end;

extern paddr_t	msgbufpa;

u_long	noncontig_enable;

extern paddr_t z2mem_start;

extern vaddr_t reserve_dumppages(vaddr_t);

/*
 * All those kernel PT submaps that BSD is so fond of
 */
void	*CADDR1, *CADDR2;
char	*vmmap;

/*
 *	Bootstrap the system enough to run with virtual memory.
 *
 *	This is called after mapping has already been enabled
 *	and just syncs the pmap module with what has already been done.
 */
void
pmap_bootstrap(paddr_t firstaddr, paddr_t loadaddr)
{
	vaddr_t va;
	int i;
	struct boot_memseg *sp, *esp;
	paddr_t fromads, toads;

	fromads = firstaddr;
	toads = maxmem << PGSHIFT;

	/* XXX: allow for msgbuf */
	toads -= m68k_round_page(MSGBUFSIZE);
	msgbufpa = toads;
	/*
	 * first segment of memory is always the one loadbsd found
	 * for loading the kernel into.
	 */

	uvmexp.pagesize = NBPG;
	uvm_md_init();

	/*
	 * May want to check if first segment is Zorro-II?
	 */
	uvm_page_physload(atop(fromads), atop(toads),
	    atop(fromads), atop(toads), VM_FREELIST_DEFAULT);

	sp = memlist->m_seg;
	esp = sp + memlist->m_nseg;
	i = 1;
	for (; noncontig_enable && sp < esp; sp++) {
		if ((sp->ms_attrib & MEMF_FAST) == 0)
			continue;		/* skip if not FastMem */
		if (firstaddr >= sp->ms_start &&
		    firstaddr < sp->ms_start + sp->ms_size)
			continue;		/* skip kernel segment */
		if (sp->ms_size == 0)
			continue;		/* skip zero size segments */
		fromads = sp->ms_start;
		toads = sp->ms_start + sp->ms_size;
#ifdef DEBUG_A4000
		/*
		 * My A4000 doesn't seem to like Zorro II memory - this
		 * hack is to skip the motherboard memory and use the
		 * Zorro II memory.  Only for trying to debug the problem.
		 * Michael L. Hitch
		 */
		if (toads == 0x08000000)
			continue;	/* skip A4000 motherboard mem */
#endif
		/*
		 * Deal with Zorro II memory stolen for DMA bounce buffers.
		 * This needs to be handled better.
		 *
		 * XXX is: disabled. This is handled now in amiga_init.c
		 * by removing the stolen memory from the memlist.
		 *
		 * XXX is: enabled again, but check real size and position.
		 * We check z2mem_start is in this segment, and set its end
		 * to the z2mem_start.
		 *
		 */
		if ((fromads <= z2mem_start) && (toads > z2mem_start))
			toads = z2mem_start;

		uvm_page_physload(atop(fromads), atop(toads),
		    atop(fromads), atop(toads), (fromads & 0xff000000) ?
		    VM_FREELIST_DEFAULT : VM_FREELIST_ZORROII);
		physmem += (toads - fromads) / PAGE_SIZE;
		++i;
		if (noncontig_enable == 1)
			break;		/* Only two segments enabled */
	}

	mem_size = physmem << PGSHIFT;

	avail_start = firstaddr;
	avail_end   = toads;

	virtual_end = VM_MAX_KERNEL_ADDRESS;

	/*
	 * Allocate all the submaps we need
	 */
#define	SYSMAP(c, v, n)	\
	v = (c)va; va += ((n)*PAGE_SIZE);

	va = virtual_avail;

	SYSMAP(void *	,CADDR1	 ,1			)
	SYSMAP(void *	,CADDR2	 ,1			)
	SYSMAP(void *	,vmmap	 ,1			)
	SYSMAP(void *	,msgbufaddr ,btoc(MSGBUFSIZE)	)

	DCIS();

	virtual_avail = reserve_dumppages(va);
}
