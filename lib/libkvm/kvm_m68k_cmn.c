/*	$NetBSD: kvm_m68k_cmn.c,v 1.1 1997/03/21 18:44:24 gwr Exp $	*/

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

#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)kvm_hp300.c	8.1 (Berkeley) 6/4/93";
#else
static char *rcsid = "$NetBSD: kvm_m68k_cmn.c,v 1.1 1997/03/21 18:44:24 gwr Exp $";
#endif
#endif /* LIBC_SCCS and not lint */

/*
 * Common m68k machine dependent routines for kvm.
 *
 * Note: This file has to build on ALL m68k machines,
 * so do NOT include any <machine/*.h> files here.
 */

#include <sys/types.h>
#include <sys/kcore.h>

#include <unistd.h>
#include <limits.h>
#include <nlist.h>
#include <kvm.h>
#include <db.h>

/* XXX: Avoid <machine/pte.h> etc. (see below) */
typedef u_int pt_entry_t;		/* page table entry */
typedef u_int st_entry_t;		/* segment table entry */

#include <m68k/cpu.h>
#include <m68k/kcore.h>

#include "kvm_private.h"
#include "kvm_m68k.h"

int   _kvm_cmn_initvtop __P((kvm_t *));
void  _kvm_cmn_freevtop __P((kvm_t *));
int	  _kvm_cmn_kvatop   __P((kvm_t *, u_long, u_long *));
off_t _kvm_cmn_pa2off   __P((kvm_t *, u_long));

struct kvm_ops _kvm_ops_cmn = {
	_kvm_cmn_initvtop,
	_kvm_cmn_freevtop,
	_kvm_cmn_kvatop,
	_kvm_cmn_pa2off };

static int vatop_030 __P((kvm_t *, st_entry_t *, ulong, ulong *));
static int vatop_040 __P((kvm_t *, st_entry_t *, ulong, ulong *));

/*
 * XXX: I don't like this, but until all arch/.../include files
 * are exported into some user-accessable place, there is no
 * convenient alternative to copying these definitions here.
 */

/* Things from param.h */
#define PGSHIFT	13
#define NBPG	(1<<13)
#define PGOFSET (NBPG-1)
#define	btop(x)		(((unsigned)(x)) >> PGSHIFT)

/* Things from pte.h */

/* All variants */
#define SG_V		 2
#define	PG_NV		0x00000000
#define PG_FRAME	0xffffe000

/* MC68030 with MMU TCR set for 8/11/13 (bits) */
#define SG3_SHIFT	24	/* a.k.a SEGSHIFT */
#define SG3_FRAME	0xffffe000
#define SG3_PMASK	0x00ffe000

/* MC68040 with MMU set for 8K page size. */
#define SG4_MASK1	0xfe000000
#define SG4_SHIFT1	25
#define SG4_MASK2	0x01fc0000
#define SG4_SHIFT2	18
#define SG4_MASK3	0x0003e000
#define SG4_SHIFT3	13
#define SG4_ADDR1	0xfffffe00
#define SG4_ADDR2	0xffffff80



#define KREAD(kd, addr, p)\
	(kvm_read(kd, addr, (char *)(p), sizeof(*(p))) != sizeof(*(p)))

void
_kvm_cmn_freevtop(kd)
	kvm_t *kd;
{
	if (kd->vmst != 0)
		free(kd->vmst);
}

int
_kvm_cmn_initvtop(kd)
	kvm_t *kd;
{

	return (0);
}

int
_kvm_cmn_kvatop(kd, va, pa)
	kvm_t *kd;
	u_long va;
	u_long *pa;
{
	register cpu_kcore_hdr_t *cpu_kh;
	int (*vtopf) __P((kvm_t *, st_entry_t *, ulong, ulong *));

	if (ISALIVE(kd)) {
		_kvm_err(kd, 0, "vatop called in live kernel!");
		return (0);
	}

	cpu_kh = kd->cpu_data;
	switch (cpu_kh->mmutype) {

	case MMU_68030:
		vtopf = vatop_030;
		break;

	case MMU_68040:
		vtopf = vatop_040;
		break;

	default:
		_kvm_err(kd, 0, "vatop unknown MMU type!");
		return (0);
	}

	return ((*vtopf)(kd, cpu_kh->sysseg_pa, va, pa));
}

/*
 * Translate a physical address to a file-offset in the crash-dump.
 */
off_t
_kvm_cmn_pa2off(kd, pa)
	kvm_t	*kd;
	u_long	pa;
{
	off_t		off;
	phys_ram_seg_t	*rsp;
	register cpu_kcore_hdr_t *cpu_kh;

	cpu_kh = kd->cpu_data;
	off = 0;
	for (rsp = cpu_kh->ram_segs; rsp->size; rsp++) {
		if (pa >= rsp->start && pa < rsp->start + rsp->size) {
			pa -= rsp->start;
			break;
		}
		off += rsp->size;
	}
	return(kd->dump_off + off + pa);
}

/*****************************************************************
 * Local stuff...
 */

static int
vatop_030(kd, sta, va, pa)
	kvm_t *kd;
	st_entry_t *sta;
	u_long va;
	u_long *pa;
{
	register cpu_kcore_hdr_t *cpu_kh;
	register u_long addr;
	int p, ste, pte;
	int offset;

	offset = va & PGOFSET;
	cpu_kh = kd->cpu_data;

	/*
	 * If we are initializing (kernel segment table pointer not yet set)
	 * then return pa == va to avoid infinite recursion.
	 */
	if (cpu_kh->sysseg_pa == 0) {
		*pa = va + cpu_kh->kernel_pa;
		return (NBPG - offset);
	}

	addr = (u_long)&sta[va >> SG3_SHIFT];
	/*
	 * Can't use KREAD to read kernel segment table entries.
	 * Fortunately it is 1-to-1 mapped so we don't have to. 
	 */
	if (sta == cpu_kh->sysseg_pa) {
		if (lseek(kd->pmfd, _kvm_cmn_pa2off(kd, addr), 0) == -1 ||
			read(kd->pmfd, (char *)&ste, sizeof(ste)) < 0)
			goto invalid;
	} else if (KREAD(kd, addr, &ste))
		goto invalid;
	if ((ste & SG_V) == 0) {
		_kvm_err(kd, 0, "invalid segment (%x)", ste);
		return((off_t)0);
	}
	p = btop(va & SG3_PMASK);
	addr = (ste & SG3_FRAME) + (p * sizeof(pt_entry_t));

	/*
	 * Address from STE is a physical address so don't use kvm_read.
	 */
	if (lseek(kd->pmfd, _kvm_cmn_pa2off(kd, addr), 0) == -1 || 
	    read(kd->pmfd, (char *)&pte, sizeof(pte)) < 0)
		goto invalid;
	addr = pte & PG_FRAME;
	if (pte == PG_NV) {
		_kvm_err(kd, 0, "page not valid");
		return (0);
	}
	*pa = addr + offset;
	
	return (NBPG - offset);
invalid:
	_kvm_err(kd, 0, "invalid address (%x)", va);
	return (0);
}

static int
vatop_040(kd, sta, va, pa)
	kvm_t *kd;
	st_entry_t *sta;
	u_long va;
	u_long *pa;
{
	register cpu_kcore_hdr_t *cpu_kh;
	register u_long addr;
	st_entry_t *sta2;
	int p, ste, pte;
	int offset;

	offset = va & PGOFSET;
	cpu_kh = kd->cpu_data;
	/*
	 * If we are initializing (kernel segment table pointer not yet set)
	 * then return pa == va to avoid infinite recursion.
	 */
	if (cpu_kh->sysseg_pa == 0) {
		*pa = va + cpu_kh->kernel_pa;
		return (NBPG - offset);
	}

	addr = (u_long)&sta[va >> SG4_SHIFT1];
	/*
	 * Can't use KREAD to read kernel segment table entries.
	 * Fortunately it is 1-to-1 mapped so we don't have to. 
	 */
	if (sta == cpu_kh->sysseg_pa) {
		if (lseek(kd->pmfd, _kvm_cmn_pa2off(kd, addr), 0) == -1 ||
			read(kd->pmfd, (char *)&ste, sizeof(ste)) < 0)
			goto invalid;
	} else if (KREAD(kd, addr, &ste))
		goto invalid;
	if ((ste & SG_V) == 0) {
		_kvm_err(kd, 0, "invalid level 1 descriptor (%x)",
				 ste);
		return((off_t)0);
	}
	sta2 = (st_entry_t *)(ste & SG4_ADDR1);
	addr = (u_long)&sta2[(va & SG4_MASK2) >> SG4_SHIFT2];
	/*
	 * Address from level 1 STE is a physical address,
	 * so don't use kvm_read.
	 */
	if (lseek(kd->pmfd, _kvm_cmn_pa2off(kd, addr), 0) == -1 || 
		read(kd->pmfd, (char *)&ste, sizeof(ste)) < 0)
		goto invalid;
	if ((ste & SG_V) == 0) {
		_kvm_err(kd, 0, "invalid level 2 descriptor (%x)",
				 ste);
		return((off_t)0);
	}
	sta2 = (st_entry_t *)(ste & SG4_ADDR2);
	addr = (u_long)&sta2[(va & SG4_MASK3) >> SG4_SHIFT3];


	/*
	 * Address from STE is a physical address so don't use kvm_read.
	 */
	if (lseek(kd->pmfd, _kvm_cmn_pa2off(kd, addr), 0) == -1 || 
	    read(kd->pmfd, (char *)&pte, sizeof(pte)) < 0)
		goto invalid;
	addr = pte & PG_FRAME;
	if (pte == PG_NV) {
		_kvm_err(kd, 0, "page not valid");
		return (0);
	}
	*pa = addr + offset;
	
	return (NBPG - offset);
invalid:
	_kvm_err(kd, 0, "invalid address (%x)", va);
	return (0);
}
