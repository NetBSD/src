/* $NetBSD: kvm_mips.c,v 1.15 2000/06/29 06:34:24 mrg Exp $ */

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 * Modified for NetBSD/mips by Jason R. Thorpe, Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: kvm_mips.c,v 1.15 2000/06/29 06:34:24 mrg Exp $");
#endif /* LIBC_SCCS and not lint */

/*
 * MIPS machine dependent routines for kvm.
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
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

#include <mips/cpuregs.h>

void
_kvm_freevtop(kd)
	kvm_t *kd;
{

	/* Not actually used for anything right now, but safe. */
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
	int page_off;
	u_int pte;
	u_long pte_pa;

	if (ISALIVE(kd)) {
		_kvm_err(kd, 0, "vatop called in live kernel!");
		return((off_t)0);
	}

	cpu_kh = kd->cpu_data;
	page_off = va & PGOFSET;

	if (va < MIPS_KSEG0_START) {
		/*
		 * KUSEG (user virtual address space) - invalid.
		 */
		_kvm_err(kd, 0, "invalid kernel virtual address");
		goto lose;
	}

	if (va >= MIPS_KSEG0_START && va < MIPS_KSEG1_START) {
		/*
		 * Direct-mapped cached address: just convert it.
		 */
		*pa = MIPS_KSEG0_TO_PHYS(va);
		return (NBPG - page_off);
	}

	if (va >= MIPS_KSEG1_START && va < MIPS_KSEG2_START) {
		/*
		 * Direct-mapped uncached address: just convert it.
		 */
		*pa = MIPS_KSEG1_TO_PHYS(va);
		return (NBPG - page_off);
	}

	/*
	 * We now know that we're a KSEG2 (kernel virtually mapped)
	 * address.  Translate the address using the pmap's kernel
	 * page table.
	 */

	/*
	 * Step 1: Make sure the kernel page table has a translation
	 * for the address.
	 */
	if (va >= (MIPS_KSEG2_START + (cpu_kh->sysmapsize * NBPG))) {
		_kvm_err(kd, 0, "invalid KSEG2 address");
		goto lose;
	}

	/*
	 * Step 2: Locate and read the PTE.
	 */
	pte_pa = cpu_kh->sysmappa +
	    (((va - MIPS_KSEG2_START) >> PGSHIFT) * sizeof(u_int));
	if (pread(kd->pmfd, &pte, sizeof(pte), _kvm_pa2off(kd, pte_pa)) !=
	    sizeof(pte)) {
		_kvm_syserr(kd, 0, "could not read PTE");
		goto lose;
	}

	/*
	 * Step 3: Validate the PTE and return the physical address.
	 */
	if ((pte & cpu_kh->pg_v) == 0) {
		_kvm_err(kd, 0, "invalid translation (invalid PTE)");
		goto lose;
	}
	*pa = (((pte & cpu_kh->pg_frame) >> cpu_kh->pg_shift) << PGSHIFT) +
	    page_off;
	return (NBPG - page_off);

 lose:
	*pa = -1;
	return (0);
}

/*
 * Translate a physical address to a file-offset in the crash-dump.
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
	ramsegs = (phys_ram_seg_t *)((char *)cpu_kh + ALIGN(sizeof *cpu_kh));

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
