/*	$NetBSD: mcontext.h,v 1.1.2.4 2001/12/02 10:35:13 scw Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein.
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

#ifndef _M68K_MCONTEXT_H_
#define _M68K_MCONTEXT_H_

#ifdef _KERNEL
#include <machine/frame.h>
#endif

/*
 * General register state
 */
#define	_NGREG		18
typedef int		__greg_t;
typedef	__greg_t	__gregset_t[_NGREG];

#define	_REG_D0	0
#define	_REG_D1	1
#define	_REG_D2	2
#define	_REG_D3	3
#define	_REG_D4	4
#define	_REG_D5	5
#define	_REG_D6	6
#define	_REG_D7	7
#define	_REG_A0	8
#define	_REG_A1	9
#define	_REG_A2	10
#define	_REG_A3	11
#define	_REG_A4	12
#define	_REG_A5	13
#define	_REG_A6	14
#define	_REG_A7	15
#define	_REG_PC	16
#define	_REG_PS	17

typedef struct {
	int	__fp_pcr;
	int	__fp_psr;
	int	__fp_piaddr;
	int	__fp_fpregs[8*3];
} __fpregset_t;

typedef struct {
	__gregset_t	__gregs;	/* General Register set */
	__fpregset_t	__fpregs;	/* Floating Point Register set */
	union {
		long	__mc_state[202];	/* Only need 308 bytes... */
#ifdef _KERNEL
		struct {
			/* Rest of the frame. */
			unsigned int	format;
			unsigned int	vector;
			union F_u	exframe;
			/* Rest of the FPU frame. */
			union FPF_u1	fpf_u1;
			union FPF_u2	fpf_u2;
		} mc_frame;
#endif
	}		__mc_pad;
} mcontext_t;

/* Note: no additional padding is to be performed in ucontext_t. */

/* Machine-specific uc_flags value */
#define _UC_M68K_UC_USER 0x40000000

#endif	/* !_M68K_MCONTEXT_H_ */
