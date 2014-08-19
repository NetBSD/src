/*	$NetBSD: core_machdep.c,v 1.8.12.1 2014/08/20 00:03:20 tls Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
 * All rights reserved.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: core_machdep.c,v 1.8.12.1 2014/08/20 00:03:20 tls Exp $");

#ifdef _KERNEL_OPT
#include "opt_altivec.h"
#include "opt_ppcarch.h"
#endif

#include <sys/param.h>
#include <sys/core.h>
#include <sys/exec.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/vnode.h>

#include <sys/exec_aout.h>

#include <uvm/uvm_extern.h>

#if defined(ALTIVEC) || defined(PPC_HAVE_SPE)
#include <powerpc/altivec.h>
#endif
#include <machine/fpu.h>
#include <machine/pcb.h>

/*
 * Write the machine-dependent part of a core dump.
 */
int
cpu_coredump(struct lwp *l, struct coredump_iostate *iocookie,
    struct core *chdr)
{
	struct coreseg cseg;
	struct md_coredump md_core;
	struct pcb * const pcb = lwp_getpcb(l);
	int error;

	if (iocookie == NULL) {
		CORE_SETMAGIC(*chdr, COREMAGIC, MID_POWERPC, 0);
		chdr->c_hdrsize = ALIGN(sizeof *chdr);
		chdr->c_seghdrsize = ALIGN(sizeof cseg);
		chdr->c_cpusize = sizeof md_core;
		chdr->c_nseg++;
		return 0;
	}

	md_core.frame = *l->l_md.md_utf;
	pcu_save_all(l);
	if (fpu_used_p(l)) {
		md_core.fpstate = pcb->pcb_fpu;
	} else
		memset(&md_core.fpstate, 0, sizeof(md_core.fpstate));

#if defined(ALTIVEC) || defined(PPC_HAVE_SPE)
	if (vec_used_p(l)) {
		md_core.vstate = pcb->pcb_vr;
	} else
#endif
		memset(&md_core.vstate, 0, sizeof(md_core.vstate));

	CORE_SETMAGIC(cseg, CORESEGMAGIC, MID_MACHINE, CORE_CPU);
	cseg.c_addr = 0;
	cseg.c_size = chdr->c_cpusize;

	error = coredump_write(iocookie, UIO_SYSSPACE, &cseg,
		    chdr->c_seghdrsize);
	if (error)
		return error;

	return coredump_write(iocookie, UIO_SYSSPACE, &md_core,
	    sizeof(md_core));
}
