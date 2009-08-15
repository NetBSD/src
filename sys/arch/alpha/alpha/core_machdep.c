/* $NetBSD: core_machdep.c,v 1.2 2009/08/15 23:44:57 matt Exp $ */

/*
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: core_machdep.c,v 1.2 2009/08/15 23:44:57 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/user.h>
#include <sys/core.h>
#include <sys/exec.h>

#include <sys/exec_aout.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/alpha.h>
#include <machine/pmap.h>
#include <machine/reg.h>

/*
 * Dump the machine specific header information at the start of a core dump.
 */
int
cpu_coredump(struct lwp *l, void *iocookie, struct core *chdr)
{
	int error;
	struct md_coredump cpustate;
	struct coreseg cseg;

	if (iocookie == NULL) {
		CORE_SETMAGIC(*chdr, COREMAGIC, MID_MACHINE, 0);
		chdr->c_hdrsize = ALIGN(sizeof(*chdr));
		chdr->c_seghdrsize = ALIGN(sizeof(cseg));
		chdr->c_cpusize = sizeof(cpustate);
		chdr->c_nseg++;
		return 0;
	}

	cpustate.md_tf = *l->l_md.md_tf;
	cpustate.md_tf.tf_regs[FRAME_SP] = alpha_pal_rdusp();	/* XXX */
	if (l->l_md.md_flags & MDP_FPUSED) {
		if (l->l_addr->u_pcb.pcb_fpcpu != NULL)
			fpusave_proc(l, 1);
		cpustate.md_fpstate = l->l_addr->u_pcb.pcb_fp;
	} else
		memset(&cpustate.md_fpstate, 0, sizeof(cpustate.md_fpstate));

	CORE_SETMAGIC(cseg, CORESEGMAGIC, MID_MACHINE, CORE_CPU);
	cseg.c_addr = 0;
	cseg.c_size = chdr->c_cpusize;

	error = coredump_write(iocookie, UIO_SYSSPACE, &cseg,
	    chdr->c_seghdrsize);
	if (error)
		return error;

	return coredump_write(iocookie, UIO_SYSSPACE, &cpustate,
	    sizeof(cpustate));
}
