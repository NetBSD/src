/*	$NetBSD: svr4_machdep.h,v 1.2 1995/01/09 01:05:24 christos Exp $	 */

/*
 * Copyright (c) 1994 Christos Zoulas
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
 * 3. The name of the author may not be used to endorse or promote products
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

#ifndef	_I386_SVR4_MACHDEP_H_
#define	_I386_SVR4_MACHDEP_H_

#include <compat/svr4/svr4_types.h>

/*
 * Machine dependent portions [X86]
 */

#define SVR4_X86_GS	0
#define SVR4_X86_FS	1
#define SVR4_X86_ES	2
#define SVR4_X86_DS	3
#define SVR4_X86_EDI	4
#define SVR4_X86_ESI	5
#define SVR4_X86_EBP	6
#define SVR4_X86_ESP	7
#define SVR4_X86_EBX	8
#define SVR4_X86_EDX	9
#define SVR4_X86_ECX	10
#define SVR4_X86_EAX	11
#define SVR4_X86_TRAPNO	12
#define SVR4_X86_ERR	13
#define SVR4_X86_EIP	14
#define SVR4_X86_CS	15
#define SVR4_X86_EFL	16
#define SVR4_X86_UESP	17
#define SVR4_X86_SS	18
#define SVR4_X86_MAXREG	19


typedef int svr4_greg_t;
typedef svr4_greg_t svr4_gregset_t[SVR4_X86_MAXREG];

typedef struct {
    union {

	struct fpchip_state {
            int		state[27];  /* x87 state */
            int		status;     /* exception status */
        } fp_chip_state;

        struct fp_emul_space {
            char	fp_emul[246];
            char	fp_epad[2];
        } fp_emul_state;

        int f_fpregs[62];	/* union of the above */

    } fp_reg_set;

    long    f_weiregs[33]; 	/* weitek */

} svr4_fregset_t;

struct svr4_ucontext;
void svr4_getcontext __P((struct proc *, struct svr4_ucontext *,
			  int, int));
int svr4_setcontext __P((struct proc *p, struct svr4_ucontext *));
void svr4_sendsig __P((sig_t, int, int, u_long));

#endif /* !_I386_SVR4_MACHDEP_H_ */
