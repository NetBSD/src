/*	$NetBSD: pmap_bootstrap.c,v 1.5 2009/12/06 00:33:59 tsutsui Exp $	*/
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>

#include <uvm/uvm.h>

#include <machine/pte.h>
#include <machine/cpu.h>
#include <machine/vmparam.h>

#include <m68k/cacheops.h>

struct memseg	boot_segs[NMEM_SEGS];
struct memseg	usable_segs[NMEM_SEGS];

extern paddr_t	avail_start;
extern paddr_t	avail_end;
#if defined(M68040) || defined(M68060)
extern u_int	protostfree;
#endif

extern paddr_t	msgbufpa;

/*
 * All those kernel PT submaps that BSD is so fond of
 */
void 		*CADDR1, *CADDR2;
char		*vmmap;

/*
 *	Bootstrap the system enough to run with virtual memory.
 *
 *	This is called after mapping has already been enabled
 *	and just syncs the pmap module with what has already been done.
 */
void
pmap_bootstrap(vaddr_t vstart, paddr_t sysseg_pa)
{
	vaddr_t	va;
	int	i;

	/*
	 * Announce page-size to the VM-system
	 */
	uvmexp.pagesize = NBPG;
	uvm_setpagesize();

	/*
	 * Setup physical address ranges
	 */
	for (i = 0; usable_segs[i + 1].start; i++)
		;
	/* XXX: allow for msgbuf */
	usable_segs[i].end -= m68k_round_page(MSGBUFSIZE);
	msgbufpa = usable_segs[i].end;

	/*
	 * Count physical memory
	 */
	mem_size = 0;
	for (i = 0; i < NMEM_SEGS; i++) {
		if (boot_segs[i].start == boot_segs[i].end)
			break;
		mem_size += boot_segs[i].end - boot_segs[i].start;
	}

	/*
	 * Announce available memory to the VM-system
	 */
	for (i = 0; usable_segs[i].start; i++)
		uvm_page_physload(atop(usable_segs[i].start),
				 atop(usable_segs[i].end),
				 atop(usable_segs[i].start),
				 atop(usable_segs[i].end),
				 usable_segs[i].free_list);

	avail_start = usable_segs[0].start;
	avail_end   = usable_segs[i - 1].end;

	virtual_avail = vstart;
	virtual_end   = VM_MAX_KERNEL_ADDRESS;

	/*
	 * Initialize protection array.
	 * XXX don't use a switch statement, it might produce an
	 * absolute "jmp" table.
	 */
	{
		u_int *kp;

		kp = (u_int *)&protection_codes;
		kp[VM_PROT_NONE|VM_PROT_NONE|VM_PROT_NONE] = 0;
		kp[VM_PROT_READ|VM_PROT_NONE|VM_PROT_NONE] = PG_RO;
		kp[VM_PROT_READ|VM_PROT_NONE|VM_PROT_EXECUTE] = PG_RO;
		kp[VM_PROT_NONE|VM_PROT_NONE|VM_PROT_EXECUTE] = PG_RO;
		kp[VM_PROT_NONE|VM_PROT_WRITE|VM_PROT_NONE] = PG_RW;
		kp[VM_PROT_NONE|VM_PROT_WRITE|VM_PROT_EXECUTE] = PG_RW;
		kp[VM_PROT_READ|VM_PROT_WRITE|VM_PROT_NONE] = PG_RW;
		kp[VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE] = PG_RW;
	}

	/*
	 * Kernel page/segment table allocated in locore,
	 * just initialize pointers.
	 */
	pmap_kernel()->pm_stpa = (st_entry_t *)sysseg_pa;
	pmap_kernel()->pm_stab = Sysseg;
	pmap_kernel()->pm_ptab = Sysmap;
#if defined(M68040) || defined(M68060)
	if (mmutype == MMU_68040) {
		pmap_kernel()->pm_stfree = protostfree;
	}
#endif

	simple_lock_init(&pmap_kernel()->pm_lock);
	pmap_kernel()->pm_count = 1;

	/*
	 * Allocate all the submaps we need
	 */
#define	SYSMAP(c, v, n)	\
	v = (c)va; va += ((n)*PAGE_SIZE);

	va = virtual_avail;

	SYSMAP(void *   ,CADDR1	,1			)
	SYSMAP(void *   ,CADDR2	,1			)
	SYSMAP(void *   ,vmmap	,1			)
	SYSMAP(void *	,msgbufaddr ,btoc(MSGBUFSIZE)	)

	DCIS();

	virtual_avail = reserve_dumppages(va);
}
