/*	$NetBSD: arm_machdep.c,v 1.1.6.1 2001/08/03 04:10:57 lukem Exp $	*/

/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "opt_compat_netbsd.h"
#include "opt_progmode.h"

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: arm_machdep.c,v 1.1.6.1 2001/08/03 04:10:57 lukem Exp $");

#include <sys/exec.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/user.h>

#include <machine/pcb.h>
#include <machine/vmparam.h>

/*
 * Clear registers on exec
 */

void
setregs(struct proc *p, struct exec_package *pack, u_long stack)
{
	struct trapframe *tf;

	tf = p->p_addr->u_pcb.pcb_tf;

	memset(tf, 0, sizeof(*tf));
	tf->tf_r0 = (u_int)PS_STRINGS;
#ifdef COMPAT_13
	tf->tf_r12 = stack;			/* needed by pre 1.4 crt0.c */
#endif
	tf->tf_usr_sp = stack;
	tf->tf_usr_lr = pack->ep_entry;
	tf->tf_svc_lr = 0x77777777;		/* Something we can see */
	tf->tf_pc = pack->ep_entry;
#ifdef PROG32
	tf->tf_spsr = PSR_USR32_MODE;
#endif

	p->p_addr->u_pcb.pcb_flags = 0;
}
