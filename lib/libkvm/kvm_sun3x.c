/*	$NetBSD: kvm_sun3x.c,v 1.1 1997/03/21 18:44:26 gwr Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)kvm_sparc.c	8.1 (Berkeley) 6/4/93";
#else
static char *rcsid = "$NetBSD: kvm_sun3x.c,v 1.1 1997/03/21 18:44:26 gwr Exp $";
#endif
#endif /* LIBC_SCCS and not lint */

/*
 * Sun3x machine dependent routines for kvm.
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

int   _kvm_sun3x_initvtop __P((kvm_t *));
void  _kvm_sun3x_freevtop __P((kvm_t *));
int	  _kvm_sun3x_kvatop   __P((kvm_t *, u_long, u_long *));
off_t _kvm_sun3x_pa2off   __P((kvm_t *, u_long));

struct kvm_ops _kvm_ops_sun3x = {
	_kvm_sun3x_initvtop,
	_kvm_sun3x_freevtop,
	_kvm_sun3x_kvatop,
	_kvm_sun3x_pa2off };

/*
 * XXX: I don't like this, but until all arch/.../include files
 * are exported into some user-accessable place, there is no
 * convenient alternative to copying these definitions here.
 */

/* sun3x/include/param.h */
#define PGSHIFT 	13
#define NBPG		8192
#define PGOFSET 	(NBPG-1)
#define KERNBASE	0xF8000000

/* sun3x/sun3x/pmap.c */
#define	KVAS_SIZE	(-KERNBASE)
#define	NKPTES		(KVAS_SIZE >> PGSHIFT)

#define PG_FRAME	0xffffff00
#define PG_VALID 	0x00000001

#define PG_PA(pte)	(pte & PG_FRAME)

/* sun3x/include/kcore.h */
#define NPHYS_RAM_SEGS 4
typedef struct {
	u_long	start;		/* Physical start address	*/
	u_long	size;		/* Size in bytes		*/
} cpu_ram_seg_t;
typedef struct cpu_kcore_hdr {
	u_long ckh_contig_end;
	u_long ckh_kernCbase;
	cpu_ram_seg_t	ram_segs[NPHYS_RAM_SEGS];
} cpu_kcore_hdr_t;

/* Finally, our local stuff... */

/*
 * Prepare for translation of kernel virtual addresses into offsets
 * into crash dump files.  Nothing to do here.
 */
int
_kvm_sun3x_initvtop(kd)
	kvm_t *kd;
{
	return (0);
}

void
_kvm_sun3x_freevtop(kd)
	kvm_t *kd;
{
}

/*
 * Translate a kernel virtual address to a physical address using the
 * mapping information in kd->vm.  Returns the result in pa, and returns
 * the number of bytes that are contiguously available from this 
 * physical address.  This routine is used only for crashdumps.
 */
int
_kvm_sun3x_kvatop(kd, va, pap)
	kvm_t *kd;
	u_long va;
	u_long *pap;
{
	register cpu_kcore_hdr_t *ckh;
	int idx, len, offset, pte;
	u_long pteva, pa;

	if (ISALIVE(kd)) {
		_kvm_err(kd, 0, "vatop called in live kernel!");
		return((off_t)0);
	}
	ckh = kd->cpu_data;

	if (va < KERNBASE) {
		_kvm_err(kd, 0, "not a kernel address");
		return(0);
	}

	/*
	 * If this VA is in the contiguous range, short-cut.
	 * Note that this ends our recursion when we call
	 * kvm_read to access the kernel page table, which
	 * is guaranteed to be in the contiguous range.
	 */
	if (va < ckh->ckh_contig_end) {
		len = va - ckh->ckh_contig_end;
		pa = va - KERNBASE;
		goto done;
	}

	/*
	 * The KVA is beyond the contiguous range, so we must
	 * read the PTE for this KVA from the page table.
	 */
	idx = ((va - KERNBASE) >> PGSHIFT);
	pteva = ckh->ckh_kernCbase + (idx * 4);
	if (kvm_read(kd, pteva, &pte, 4) != 4) {
		_kvm_err(kd, 0, "can not read PTE!");
		return (0);
	}
	if ((pte & PG_VALID) == 0) {
		_kvm_err(kd, 0, "page not valid (VA=0x%x)", va);
		return (0);
	}
	offset = va & PGOFSET;
	len = (NBPG - offset);
	pa = PG_PA(pte) + offset;

done:
	*pap = pa;
	return (len);
}

/*
 * Translate a physical address to a file-offset in the crash-dump.
 */
off_t
_kvm_sun3x_pa2off(kd, pa)
	kvm_t	*kd;
	u_long	pa;
{
	off_t		off;
	cpu_ram_seg_t	*rsp;
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

