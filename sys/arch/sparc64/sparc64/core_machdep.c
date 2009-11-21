/*	$NetBSD: core_machdep.c,v 1.3 2009/11/21 04:16:52 rmind Exp $ */

/*
 * Copyright (c) 1996-2002 Eduardo Horvath.  All rights reserved.
 * Copyright (c) 1996
 *	The President and Fellows of Harvard College. All rights reserved.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *	This product includes software developed by Harvard University.
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
 *	This product includes software developed by Harvard University.
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
 *
 *	@(#)vm_machdep.c	8.2 (Berkeley) 9/23/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: core_machdep.c,v 1.3 2009/11/21 04:16:52 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/core.h>
#include <sys/buf.h>
#include <sys/exec.h>
#include <sys/vnode.h>

#include <sys/exec_aout.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/trap.h>
#include <machine/bus.h>

/*
 * cpu_coredump is called to write a core dump header.
 */
int
cpu_coredump(struct lwp *l, void *iocookie, struct core *chdr)
{
	int error;
	struct md_coredump md_core;
	struct coreseg cseg;

	if (iocookie == NULL) {
		CORE_SETMAGIC(*chdr, COREMAGIC, MID_MACHINE, 0);
		chdr->c_hdrsize = ALIGN(sizeof(*chdr));
		chdr->c_seghdrsize = ALIGN(sizeof(cseg));
		chdr->c_cpusize = sizeof(md_core);
		chdr->c_nseg++;
		return 0;
	}

	/* Copy important fields over. */
	md_core.md_tf.tf_tstate = l->l_md.md_tf->tf_tstate;
	md_core.md_tf.tf_pc = l->l_md.md_tf->tf_pc;
	md_core.md_tf.tf_npc = l->l_md.md_tf->tf_npc;
	md_core.md_tf.tf_y = l->l_md.md_tf->tf_y;
	md_core.md_tf.tf_tt = l->l_md.md_tf->tf_tt;
	md_core.md_tf.tf_pil = l->l_md.md_tf->tf_pil;
	md_core.md_tf.tf_oldpil = l->l_md.md_tf->tf_oldpil;

	md_core.md_tf.tf_global[0] = l->l_md.md_tf->tf_global[0];
	md_core.md_tf.tf_global[1] = l->l_md.md_tf->tf_global[1];
	md_core.md_tf.tf_global[2] = l->l_md.md_tf->tf_global[2];
	md_core.md_tf.tf_global[3] = l->l_md.md_tf->tf_global[3];
	md_core.md_tf.tf_global[4] = l->l_md.md_tf->tf_global[4];
	md_core.md_tf.tf_global[5] = l->l_md.md_tf->tf_global[5];
	md_core.md_tf.tf_global[6] = l->l_md.md_tf->tf_global[6];
	md_core.md_tf.tf_global[7] = l->l_md.md_tf->tf_global[7];

	md_core.md_tf.tf_out[0] = l->l_md.md_tf->tf_out[0];
	md_core.md_tf.tf_out[1] = l->l_md.md_tf->tf_out[1];
	md_core.md_tf.tf_out[2] = l->l_md.md_tf->tf_out[2];
	md_core.md_tf.tf_out[3] = l->l_md.md_tf->tf_out[3];
	md_core.md_tf.tf_out[4] = l->l_md.md_tf->tf_out[4];
	md_core.md_tf.tf_out[5] = l->l_md.md_tf->tf_out[5];
	md_core.md_tf.tf_out[6] = l->l_md.md_tf->tf_out[6];
	md_core.md_tf.tf_out[7] = l->l_md.md_tf->tf_out[7];

#ifdef DEBUG
	md_core.md_tf.tf_local[0] = l->l_md.md_tf->tf_local[0];
	md_core.md_tf.tf_local[1] = l->l_md.md_tf->tf_local[1];
	md_core.md_tf.tf_local[2] = l->l_md.md_tf->tf_local[2];
	md_core.md_tf.tf_local[3] = l->l_md.md_tf->tf_local[3];
	md_core.md_tf.tf_local[4] = l->l_md.md_tf->tf_local[4];
	md_core.md_tf.tf_local[5] = l->l_md.md_tf->tf_local[5];
	md_core.md_tf.tf_local[6] = l->l_md.md_tf->tf_local[6];
	md_core.md_tf.tf_local[7] = l->l_md.md_tf->tf_local[7];

	md_core.md_tf.tf_in[0] = l->l_md.md_tf->tf_in[0];
	md_core.md_tf.tf_in[1] = l->l_md.md_tf->tf_in[1];
	md_core.md_tf.tf_in[2] = l->l_md.md_tf->tf_in[2];
	md_core.md_tf.tf_in[3] = l->l_md.md_tf->tf_in[3];
	md_core.md_tf.tf_in[4] = l->l_md.md_tf->tf_in[4];
	md_core.md_tf.tf_in[5] = l->l_md.md_tf->tf_in[5];
	md_core.md_tf.tf_in[6] = l->l_md.md_tf->tf_in[6];
	md_core.md_tf.tf_in[7] = l->l_md.md_tf->tf_in[7];
#endif
	if (l->l_md.md_fpstate) {
		fpusave_lwp(l, true);
		md_core.md_fpstate = *l->l_md.md_fpstate;
	} else
		memset(&md_core.md_fpstate, 0,
		      sizeof(md_core.md_fpstate));

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
