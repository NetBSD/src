/*	$NetBSD: reg.h,v 1.19 2014/01/04 00:10:02 dsl Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 * from: Utah Hdr: reg.h 1.1 90/07/09
 *
 *	@(#)reg.h	7.2 (Berkeley) 11/3/90
 */

#ifndef _M68K_REG_H_
#define _M68K_REG_H_

/*
 * Register set accessible via /proc/$pid/reg and ptrace()
 */
struct reg {
	int	r_regs[16];	/* D0-D7/A0-A7 */
	int	r_sr;
	int	r_pc;
};

struct fpreg {
	int	r_regs[8*3];	/* FP0-FP7 */
	int	r_fpcr;
	int	r_fpsr;
	int	r_fpiar;
};

/* XXXX this is historical (but it can't be deprecated quite yet) */

/*
 * Location of the users' stored
 * registers relative to D0.
 * Usage is u.u_ar0[XX].
 */
#define	D0	(0)
#define	D1	(1)
#define	D2	(2)
#define	D3	(3)
#define	D4	(4)
#define	D5	(5)
#define	D6	(6)
#define	D7	(7)
#define	A0	(8)
#define	A1	(9)
#define	A2	(10)
#define	A3	(11)
#define	A4	(12)
#define	A5	(13)
#define	A6	(14)
#define	A7	(15)

#define	FP	A6
#define	SP	A7
#define	PS	(16)
#define	PC	(17)

#ifdef _KERNEL
/*
 * Due to a mental lapse somewhere down the line, wait returns its values
 * in strange registers.  Kludge it up here so we don't have to in the
 * machine-independent code.
 */
#define	R0	D1
#define	R1	A0

struct lwp;
int	process_read_regs(struct lwp *, struct reg *);
int	process_read_fpregs(struct lwp *, struct fpreg *, size_t *);

#endif

#endif /* !_M68K_REG_H_ */
