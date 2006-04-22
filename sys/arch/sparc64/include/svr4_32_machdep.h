/*	$NetBSD: svr4_32_machdep.h,v 1.6.6.1 2006/04/22 11:38:02 simonb Exp $	 */

/*-
 * Copyright (c) 1994 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#ifndef	_SPARC_SVR4_32_MACHDEP_H_
#define	_SPARC_SVR4_32_MACHDEP_H_

#include <compat/svr4_32/svr4_32_types.h>
#include <machine/svr4_machdep.h>

/*
 * Machine dependent portions [SPARC]
 */

typedef u_int svr4_32_greg_t;

typedef struct {
	svr4_32_greg_t	rwin_lo[8];
	svr4_32_greg_t	rwin_in[8];
} svr4_32_rwindow_t;

typedef uint32_t svr4_32_gwindowp_t;
typedef struct {
	int		   cnt;
	netbsd32_intp	   sp[SVR4_SPARC_MAXWIN];
	svr4_32_rwindow_t  win[SVR4_SPARC_MAXWIN];
} svr4_32_gwindow_t;

typedef svr4_32_greg_t svr4_32_gregset_t[SVR4_SPARC_MAXREG];

typedef struct {
	union {
		u_int	 fp_ri[32];
		double	 fp_rd[16];
	} fpu_regs;
	netbsd32_voidp	 fp_q;
	unsigned	 fp_fsr;
	u_char		 fp_nqel;
	u_char		 fp_nqsize;
	u_char		 fp_busy;
} svr4_32_fregset_t;

typedef struct {
	u_int		 id;
	netbsd32_voidp	 ptr;
} svr4_32_xrs_t;

typedef struct svr4_32_mcontext {
	svr4_32_gregset_t	greg;
	svr4_32_gwindowp_t	gwin;
	svr4_32_fregset_t	freg;
	svr4_32_xrs_t		xrs;
	netbsd32_long		pad[19];
} svr4_32_mcontext_t;

#define	SVR4_32_UC_MACHINE_PAD	23	/* size of uc_pad */

struct svr4_32_ucontext;

#define svr4_32_syscall_intern syscall_intern

int svr4_32_trap(int, struct lwp *);

#endif /* !_SPARC_SVR4_32_MACHDEP_H_ */
