/*	$NetBSD: svr4_machdep.c,v 1.15.12.1 2017/12/03 11:36:28 jdolecek Exp $ */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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

/* 
 * This does not implement COMPAT_SVR4 for MIPS. XXX: should be removed.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: svr4_machdep.c,v 1.15.12.1 2017/12/03 11:36:28 jdolecek Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/exec.h>
#include <sys/exec_elf.h> 
#include <sys/kernel.h>
#include <sys/mount.h> 
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/syscallargs.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
 
#include <uvm/uvm_extern.h>

#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_ucontext.h>
#include <compat/svr4/svr4_lwp.h>
#include <compat/svr4/svr4_syscallargs.h>
#include <compat/svr4/svr4_util.h>
#include <compat/svr4/svr4_exec.h>

#include <mips/psl.h>
#include <mips/reg.h>

#include <machine/vmparam.h>

/* 
 * Void function to get it building 
 * XXX This should be filled later 
 */
int
svr4_setmcontext(struct lwp *l, svr4_mcontext_t *mc, unsigned long flags)
{
	printf("Warning: svr4_setmcontext() called\n");
	return 0;
}

void * 
svr4_getmcontext(l, mc, flags)
	struct lwp *l;
	svr4_mcontext_t *mc;
	unsigned long *flags;
{    
	printf("Warning: svr4_getmcontext() called\n");
	return NULL;
}

void
svr4_md_init(void)
{

}

void
svr4_md_fini(void)
{

}
