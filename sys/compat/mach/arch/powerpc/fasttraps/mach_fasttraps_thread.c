/*	$NetBSD: mach_fasttraps_thread.c,v 1.5 2003/01/30 19:14:20 manu Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mach_fasttraps_thread.c,v 1.5 2003/01/30 19:14:20 manu Exp $");

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/signal.h>
#include <sys/proc.h>
#include <sys/exec.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_exec.h>

#include <compat/mach/arch/powerpc/fasttraps/mach_fasttraps_syscall.h>
#include <compat/mach/arch/powerpc/fasttraps/mach_fasttraps_syscallargs.h>

int
mach_sys_cthread_set_self(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct mach_sys_cthread_set_self_args /* {
		syscallarg(mach_cproc_t) p;
	} */ *uap = v;

	l->l_private = (void *)SCARG(uap, p);
	return 0;
}

int
mach_sys_cthread_self(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{

	*retval = (register_t)l->l_private;
	return 0;
}
