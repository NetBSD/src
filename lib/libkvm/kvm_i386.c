/*	$NetBSD: kvm_i386.c,v 1.20 2003/08/07 16:44:37 agc Exp $	*/

/*-
 * Copyright (c) 1989, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software developed by the Computer Systems
 * Engineering group at Lawrence Berkeley Laboratory under DARPA contract
 * BG 91-66 and contributed to Berkeley.
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
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)kvm_hp300.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: kvm_i386.c,v 1.20 2003/08/07 16:44:37 agc Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

/*
 * i386 machine dependent routines for kvm.
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/stat.h>
#include <sys/kcore.h>
#include <machine/kcore.h>
#include <stdlib.h>
#include <unistd.h>
#include <nlist.h>
#include <kvm.h>

#include <uvm/uvm_extern.h>

#include <limits.h>
#include <db.h>

#include "kvm_private.h"

#include <machine/pmap.h>
#include <machine/pte.h>
#include <machine/vmparam.h>

#ifndef btop
#define	btop(x)		(((unsigned)(x)) >> PGSHIFT)	/* XXX */
#define	ptob(x)		((caddr_t)((x) << PGSHIFT))	/* XXX */
#endif

void
_kvm_freevtop(kd)
	kvm_t *kd;
{

	/* Not actually used for anything right now, but safe. */
	if (kd->vmst != 0)
		free(kd->vmst);
}

/*ARGSUSED*/
int
_kvm_initvtop(kd)
	kvm_t *kd;
{

	return (0);
}

/*
 * Translate a kernel virtual address to a physical address.
 */
int
_kvm_kvatop(kd, va, pa)
	kvm_t *kd;
	u_long va;
	u_long *pa;
{
	cpu_kcore_hdr_t *cpu_kh;
	u_long page_off;
	pd_entry_t pde;
	pt_entry_t pte;
	u_long pde_pa, pte_pa;

	if (ISALIVE(kd)) {
		_kvm_err(kd, 0, "vatop called in live kernel!");
		return (0);
	}

	cpu_kh = kd->cpu_data;
	page_off = va & PGOFSET;

	/*
	 * Find and read the page directory entry.
	 */
	pde_pa = cpu_kh->ptdpaddr + (pdei(va) * sizeof(pd_entry_t));
	if (pread(kd->pmfd, (void *)&pde, sizeof(pde),
	    _kvm_pa2off(kd, pde_pa)) != sizeof(pde)) {
		_kvm_syserr(kd, 0, "could not read PDE");
		goto lose;
	}

	/*
	 * Find and read the page table entry.
	 */
	if ((pde & PG_V) == 0) {
		_kvm_err(kd, 0, "invalid translation (invalid PDE)");
		goto lose;
	}
	pte_pa = (pde & PG_FRAME) + (ptei(va) * sizeof(pt_entry_t));
	if (pread(kd->pmfd, (void *) &pte, sizeof(pte),
	    _kvm_pa2off(kd, pte_pa)) != sizeof(pte)) {
		_kvm_syserr(kd, 0, "could not read PTE");
		goto lose;
	}

	/*
	 * Validate the PTE and return the physical address.
	 */
	if ((pte & PG_V) == 0) {
		_kvm_err(kd, 0, "invalid translation (invalid PTE)");
		goto lose;
	}
	*pa = (pte & PG_FRAME) + page_off;
	return (int)(NBPG - page_off);

 lose:
	*pa = (u_long)~0L;
	return (0);
}

/*
 * Translate a physical address to a file-offset in the crash dump.
 */
off_t
_kvm_pa2off(kd, pa)
	kvm_t *kd;
	u_long pa;
{
	cpu_kcore_hdr_t *cpu_kh;
	phys_ram_seg_t *ramsegs;
	off_t off;
	int i;

	cpu_kh = kd->cpu_data;
	ramsegs = (void *)((char *)(void *)cpu_kh + ALIGN(sizeof *cpu_kh));

	off = 0;
	for (i = 0; i < cpu_kh->nmemsegs; i++) {
		if (pa >= ramsegs[i].start &&
		    (pa - ramsegs[i].start) < ramsegs[i].size) {
			off += (pa - ramsegs[i].start);
			break;
		}
		off += ramsegs[i].size;
	}

	return (kd->dump_off + off);
}

/*
 * Machine-dependent initialization for ALL open kvm descriptors,
 * not just those for a kernel crash dump.  Some architectures
 * have to deal with these NOT being constants!  (i.e. m68k)
 */
int
_kvm_mdopen(kd)
	kvm_t	*kd;
{

	kd->usrstack = USRSTACK;
	kd->min_uva = VM_MIN_ADDRESS;
	kd->max_uva = VM_MAXUSER_ADDRESS;

	return (0);
}
