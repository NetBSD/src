/*	$NetBSD: mcontext.h,v 1.1.2.2 2002/06/21 06:55:18 gmcgarry Exp $	 */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg,
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

#ifndef	_SPARC_MCONTEXT_H_
#define	_SPARC_MCONTEXT_H_

/*
 * Machine dependent mcontext_t for the SPARC architecture.
 */

#define _REG_PSR	0
#define _REG_PC		1
#define _REG_nPC	2
#define _REG_Y		3
#define _REG_G1		4
#define _REG_G2		5
#define _REG_G3		6
#define _REG_G4		7
#define _REG_G5		8
#define _REG_G6		9
#define _REG_G7		10
#define _REG_O0		11
#define _REG_O1		12
#define _REG_O2		13
#define _REG_O3		14
#define _REG_O4		15
#define _REG_O5		16
#define _REG_O6		17
#define _REG_O7		18
#define _NGREG		19

#define _REG_SP		_REG_O6
#define _REG_PS		_REG_PSR

#define __SPARC_MAXWIN	31

typedef int __greg_t;
typedef __greg_t __gregset_t[_NGREG];

/* Layout of a register window. */
typedef struct {
	__greg_t	__rw_local[8];		/* %l0-%l7 */
	__greg_t	__rw_in[8];		/* %i0-%i7 */
} __rwindow_t;

/* Description of available register windows. */
typedef struct {
	int		__wbcnt;
	int		*__spbuf[__SPARC_MAXWIN];
	__rwindow_t	__wbuf[__SPARC_MAXWIN];
} __gwindow_t;


typedef struct {
	union {
		u_int	__fp_ri[32];
		double	__fp_rd[16];
	} __fpu_regs;
	void		*__fp_q;
	unsigned	__fp_fsr;
	u_char		__fp_nqel;
	u_char		__fp_nqsize;
	u_char		__fp_busy;
} __fregset_t;

/* `Extra Register State'(?) */
typedef struct {
	u_int		__id;
#define __XRS_ID	(('x' << 24) | ('r' << 16) | ('s' << 8))
	void		*__ptr;
} __xrs_t;


typedef struct __mcontext {
	__gregset_t	__greg;
	__gwindow_t	*__gwin;
	__fregset_t	__freg;
	__xrs_t		__xrs;
	long		__pad[19];
} mcontext_t;

#define _UC_MACHINE_SP(uc)	((uc)->uc_mcontext.__greg[_REG_SP])

#endif /* !_SPARC_MCONTEXT_H_ */
