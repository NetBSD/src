/*	$NetBSD: kvm_alpha.c,v 1.4 1996/10/01 19:04:02 cgd Exp $	*/

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

#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/kcore.h>
#include <machine/kcore.h>
#include <unistd.h>
#include <nlist.h>
#include <kvm.h>

#include <vm/vm.h>
#include <vm/vm_param.h>

#include <limits.h>
#include <db.h>

#include "kvm_private.h"

void
_kvm_freevtop(kd)
	kvm_t *kd;
{

}

int
_kvm_initvtop(kd)
	kvm_t *kd;
{

	return (0);
}

int
_kvm_kvatop(kd, va, pa)
	kvm_t *kd;
	u_long va;
	u_long *pa;
{
	cpu_kcore_hdr_t *cpu_kh;
	int rv, page_off;

        if (ISALIVE(kd)) {
                _kvm_err(kd, 0, "vatop called in live kernel!");
                return(0);
        }

	cpu_kh = kd->cpu_data;
	page_off = va & (cpu_kh->page_size - 1);

	if (va >= ALPHA_K0SEG_BASE && va <= ALPHA_K0SEG_END) {
		/*
		 * Direct-mapped address.  Just convert it.
		 */
		*pa = ALPHA_K0SEG_TO_PHYS(va);
		rv = cpu_kh->page_size - page_off;
	} else if (va >= ALPHA_K1SEG_BASE && va <= ALPHA_K1SEG_END) {
		/*
		 * Real kernel virtual address.  Do the translation.
		 */
		/* XXX TRANSLATE IT! */
		goto barf; /* XXX */
	} else {
		/*
		 * The address is from space (not a KV address).  Return
		 * values that indicate that it can't be used.
		 */
barf: /* XXX */
		*pa = -1;
		rv = 0;
	}

/* printf("_kvm_kvatop va = 0x%lx, returning pa = 0x%lx, rv = 0x%lx\n", va, *pa, rv); */
	return (rv);
}

/*
 * Translate a physical address to a file-offset in the crash-dump.
 */
off_t   
_kvm_pa2off(kd, pa)
	kvm_t *kd;
	u_long pa;
{
	off_t off;
	cpu_kcore_hdr_t *cpu_kh;

	cpu_kh = kd->cpu_data;

	off = 0;
	pa -= cpu_kh->core_seg.start;

	return (kd->dump_off + off + pa);
}
