/*	$NetBSD: ptrace.h,v 1.12 2015/09/25 16:05:17 christos Exp $	*/

/*
 * Copyright (c) 1993 Christopher G. Demetriou
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
 *      This product includes software developed by Christopher G. Demetriou.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH3_PTRACE_H_
#define _SH3_PTRACE_H_

/*
 * sh3-dependent ptrace definitions
 */

#define PT_STEP		(PT_FIRSTMACH + 0)

/* old struct reg (now struct __reg40) that was missing r_gbr */
#define	PT___GETREGS40	(PT_FIRSTMACH + 1)
#define	PT___SETREGS40	(PT_FIRSTMACH + 2)

#define	PT_GETREGS	(PT_FIRSTMACH + 3)
#define	PT_SETREGS	(PT_FIRSTMACH + 4)

#if 0 /* XXX: not yet, but reserve the numbers "leaked" to readelf(1). */
#define	PT_GETFPREGS	(PT_FIRSTMACH + 5)
#define	PT_SETFPREGS	(PT_FIRSTMACH + 6)
#endif

#define PT_MACHDEP_STRINGS \
	"PT_STEP", \
	"PT___GETREGS40", \
	"PT___SETREGS40", \
	"PT_GETREGS", \
	"PT_SETREGS",

#include <machine/reg.h>
#define PTRACE_REG_PC(r)	r->r_spc
#define PTRACE_REG_SET_PC(r, v)	r->r_spc = (v)
#define PTRACE_REG_SP(r)	r->r_r15
#define PTRACE_REG_INTV(r)	r->r_r0

#define PTRACE_BREAKPOINT	((const uint8_t[]) { 0xc3, 0xc3 })
#define PTRACE_BREAKPOINT_SIZE	2

#ifdef _KERNEL
#ifdef _KERNEL_OPT
#include "opt_compat_netbsd.h"
#endif

#ifdef COMPAT_40

#define __HAVE_PTRACE_MACHDEP

#define	PTRACE_MACHDEP_REQUEST_CASES			\
	case PT___GETREGS40:	/* FALLTHROUGH */	\
	case PT___SETREGS40:

#endif /* COMPAT_40 */

#ifdef __HAVE_PTRACE_MACHDEP
int ptrace_machdep_dorequest(struct lwp *, struct lwp *, int, void *, int);
#endif

#endif /* _KERNEL */
#endif /* !_SH3_PTRACE_H_ */
