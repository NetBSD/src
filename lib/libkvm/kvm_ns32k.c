/*	$NetBSD: kvm_ns32k.c,v 1.16 2003/05/16 10:24:55 wiz Exp $	*/

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

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)kvm_hp300.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: kvm_ns32k.c,v 1.16 2003/05/16 10:24:55 wiz Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

/*
 * ns32k machine dependent routines for kvm.  Hopefully, the forthcoming
 * vm code will one day obsolete this module.
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/stat.h>

#include <sys/core.h>
#include <sys/exec_aout.h>
#include <sys/kcore.h>

#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <nlist.h>
#include <kvm.h>

#include <uvm/uvm_extern.h>

#include <db.h>

#include "kvm_private.h"

#include <machine/pmap.h>
#include <machine/pte.h>
#include <machine/kcore.h>
#include <machine/vmparam.h>

#ifndef btop
#define	btop(x)		(((unsigned)(x)) >> PGSHIFT)	/* XXX */
#define	ptob(x)		((caddr_t)((x) << PGSHIFT))	/* XXX */
#endif

#define KREAD(kd, addr, p)\
	(kvm_read(kd, addr, (char *)(p), sizeof(*(p))) != sizeof(*(p)))

void
_kvm_freevtop(kd)
	kvm_t *kd;
{
	if (kd->vmst != 0)
		free(kd->vmst);
}

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
	u_long offset;
	u_long pte_pa;
	u_long pde_pa;
	pd_entry_t pde;
	pt_entry_t pte;

	if (ISALIVE(kd)) {
		_kvm_err(kd, 0, "vatop called in live kernel!");
		return(0);
	}

	offset = va & PGOFSET;
	cpu_kh = kd->cpu_data;

        /*
         * If we are initializing (kernel page table descriptor pointer
	 * not yet set) * then return pa == va to avoid infinite recursion.
         */
        if (cpu_kh->ptd == 0) {
                *pa = va;
                return (NBPG - offset);
        }

	pde_pa = _kvm_pa2off(kd, cpu_kh->ptd + sizeof(pt_entry_t) * pdei(va));
	if (pread(kd->pmfd, &pde, sizeof(pde), (off_t)pde_pa) != sizeof(pde)) {
		_kvm_syserr(kd, kd->program, "could not read PDE");
		goto invalid;
	}
	if ((pde & PG_V) == 0)
		goto invalid;

	pte_pa = _kvm_pa2off(kd, (pde & PG_FRAME) + sizeof(pt_entry_t) * ptei(va));
	if (pread(kd->pmfd, &pte, sizeof(pte), (off_t)pte_pa) != sizeof(pte)) {
		_kvm_syserr(kd, kd->program, "could not read PTE");
		goto invalid;
	}

	*pa = (pte & PG_FRAME) + offset;
	return (NBPG - offset);

invalid:
	_kvm_err(kd, 0, "invalid address (%lx)", va);
	return (0);
}

/*
 * Translate a physical address to a file-offset in the crash dump.
 */
off_t
_kvm_pa2off(kd, pa)
	kvm_t	*kd;
	u_long	pa;
{
	return(kd->dump_off + pa);
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
