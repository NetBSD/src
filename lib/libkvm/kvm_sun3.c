/*	$NetBSD: kvm_sun3.c,v 1.5 1997/03/21 18:44:25 gwr Exp $	*/

/*-
 * Copyright (c) 1992, 1993
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
static char sccsid[] = "@(#)kvm_sparc.c	8.1 (Berkeley) 6/4/93";
#else
static char *rcsid = "$NetBSD: kvm_sun3.c,v 1.5 1997/03/21 18:44:25 gwr Exp $";
#endif
#endif /* LIBC_SCCS and not lint */

/*
 * Sun3 machine dependent routines for kvm.
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

#include "kvm_private.h"
#include "kvm_m68k.h"

int   _kvm_sun3_initvtop __P((kvm_t *));
void  _kvm_sun3_freevtop __P((kvm_t *));
int	  _kvm_sun3_kvatop   __P((kvm_t *, u_long, u_long *));
off_t _kvm_sun3_pa2off   __P((kvm_t *, u_long));

struct kvm_ops _kvm_ops_sun3 = {
	_kvm_sun3_initvtop,
	_kvm_sun3_freevtop,
	_kvm_sun3_kvatop,
	_kvm_sun3_pa2off };

/*
 * XXX: I don't like this, but until all arch/.../include files
 * are exported into some user-accessable place, there is no
 * convenient alternative to copying these definitions here.
 */

/* sun3/include/param.h */
#define PGSHIFT 	13
#define NBPG		8192
#define PGOFSET 	(NBPG-1)
#define SEGSHIFT	17
#define KERNBASE	0x0E000000

/* sun3/include/pte.h */
#define NKSEG 256	/* kernel segmap entries */
#define NPAGSEG 16	/* pages per segment */
#define PG_VALID	0x80000000
#define PG_FRAME	0x0007FFFF
#define PG_PA(pte) ((pte & PG_FRAME) <<PGSHIFT)

#define VA_SEGNUM(x)	((u_int)(x) >> SEGSHIFT)
#define VA_PTE_NUM_MASK (0xF << PGSHIFT)
#define VA_PTE_NUM(va) ((va & VA_PTE_NUM_MASK) >> PGSHIFT)

/* sun3/include/kcore.h */
typedef struct cpu_kcore_hdr {
	phys_ram_seg_t	ram_segs[4];
	u_char ksegmap[NKSEG];
} cpu_kcore_hdr_t;

/* Finally, our local stuff... */
struct vmstate {
	/* Page Map Entry Group (PMEG) */
	int   pmeg[NKSEG][NPAGSEG];
};

/*
 * Prepare for translation of kernel virtual addresses into offsets
 * into crash dump files. We use the MMU specific goop written at the
 * beginning of a crash dump by dumpsys()
 * Note: sun3 MMU specific!
 */
int
_kvm_sun3_initvtop(kd)
	kvm_t *kd;
{
	register char *p;

	p = kd->cpu_data;
	p += (NBPG - sizeof(kcore_seg_t));
	kd->vmst = (struct vmstate *)p;

	return (0);
}

void
_kvm_sun3_freevtop(kd)
	kvm_t *kd;
{
	/* This was set by pointer arithmetic, not allocation. */
	kd->vmst = (void*)0;
}

/*
 * Translate a kernel virtual address to a physical address using the
 * mapping information in kd->vm.  Returns the result in pa, and returns
 * the number of bytes that are contiguously available from this 
 * physical address.  This routine is used only for crashdumps.
 */
int
_kvm_sun3_kvatop(kd, va, pap)
	kvm_t *kd;
	u_long va;
	u_long *pap;
{
	register cpu_kcore_hdr_t *ckh;
	u_int segnum, sme, ptenum;
	int pte, offset;
	u_long pa;

	if (ISALIVE(kd)) {
		_kvm_err(kd, 0, "vatop called in live kernel!");
		return((off_t)0);
	}
	ckh = kd->cpu_data;

	if (va < KERNBASE) {
		_kvm_err(kd, 0, "not a kernel address");
		return((off_t)0);
	}

	/*
	 * Get the segmap entry (sme) from the kernel segmap.
	 * Note: only have segmap entries from KERNBASE to end.
	 */
	segnum = VA_SEGNUM(va - KERNBASE);
	ptenum = VA_PTE_NUM(va);
	offset = va & PGOFSET;

	/* The segmap entry selects a PMEG. */
	sme = ckh->ksegmap[segnum];
	pte = kd->vmst->pmeg[sme][ptenum];

	if ((pte & PG_VALID) == 0) {
		_kvm_err(kd, 0, "page not valid (VA=0x%x)", va);
		return (0);
	}
	pa = PG_PA(pte) + offset;

	*pap = pa;
	return (NBPG - offset);
}

/*
 * Translate a physical address to a file-offset in the crash-dump.
 */
off_t
_kvm_sun3_pa2off(kd, pa)
	kvm_t	*kd;
	u_long	pa;
{
	return(kd->dump_off + pa);
}

