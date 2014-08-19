/* $NetBSD: cpu_arm.c,v 1.2.10.2 2014/08/20 00:03:27 tls Exp $ */

/*-
 * Copyright (c) 2011 Reinoud Zandijk <reinoud@netbsd.org>
 * Copyright (c) 2007, 2013 Jared D. McNeill <jmcneill@invisible.ca>
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
 * Note that this machdep.c uses the `dummy' mcontext_t defined for usermode.
 * This is basicly a blob of PAGE_SIZE big. We might want to switch over to
 * non-generic mcontext_t's one day, but will this break non-NetBSD hosts?
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu_arm.c,v 1.2.10.2 2014/08/20 00:03:27 tls Exp $");

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/exec.h>
#include <sys/buf.h>
#include <sys/boot_flag.h>
#include <sys/ucontext.h>
#include <sys/utsname.h>
#include <machine/pcb.h>
#include <machine/psl.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_page.h>

#include <dev/mm.h>
#include <machine/machdep.h>
#include <machine/thunk.h>

#include "opt_exec.h"

/* from sys/arch/arm/include/frame.h : KEEP IN SYNC */
struct sigframe_siginfo {
	siginfo_t	sf_si;		/* actual saved siginfo */
	ucontext_t	sf_uc;		/* actual saved ucontext */
};

void
sendsig_siginfo(const ksiginfo_t *ksi, const sigset_t *mask)
{
	panic("sendsig_siginfo not implemented");
}

void
setregs(struct lwp *l, struct exec_package *pack, vaddr_t stack)
{
	panic("sendsig_siginfo not implemented");
}

void
md_syscall_get_syscallnumber(ucontext_t *ucp, uint32_t *code)
{
	panic("md_syscall_get_syscallnumber not implemented");
}

int
md_syscall_getargs(lwp_t *l, ucontext_t *ucp, int nargs, int argsize,
	register_t *args)
{
	panic("md_syscall_getargs not implemented");
	return 0;
}

void
md_syscall_set_returnargs(lwp_t *l, ucontext_t *ucp,
	int error, register_t *rval)
{
	panic("md_syscall_set_returnargs not implemented");
}

register_t
md_get_pc(ucontext_t *ucp)
{
	unsigned int *reg = (unsigned int *)&ucp->uc_mcontext;
	return reg[15];
}

register_t
md_get_sp(ucontext_t *ucp)
{
	unsigned int *reg = (unsigned int *)&ucp->uc_mcontext;
	return reg[13];
}

int
md_syscall_check_opcode(ucontext_t *ucp)
{
	panic("md_syscall_check_opcode not implemented");
	return 0;
}

void
md_syscall_get_opcode(ucontext_t *ucp, uint32_t *opcode)
{
	panic("md_syscall_get_opcode not implemented");
}

void
md_syscall_inc_pc(ucontext_t *ucp, uint32_t opcode)
{
	panic("md_syscall_inc_pc not implemented");
}

void
md_syscall_dec_pc(ucontext_t *ucp, uint32_t opcode)
{
	panic("md_syscall_dec_pc not implemented");
}

