/*	$NetBSD: netbsd32_ktrace.c,v 1.1 2004/01/15 14:34:39 mrg Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)kern_ktrace.c	8.5 (Berkeley) 5/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_ktrace.c,v 1.1 2004/01/15 14:34:39 mrg Exp $");

#include "opt_ktrace.h"

#ifdef KTRACE
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/ktrace.h>
#include <sys/malloc.h>

#include <compat/netbsd32/netbsd32.h>

/*
 * Output a 64 bit ktrace syscall record, adjusting the argsize to
 * match.  Needs arguments already converted to 64 bit format.  This
 * is virtually identical to ktrsyscall().
 */
void
ktrsyscall32(p, code, realcode, callp, args)
	struct proc *p;
	register_t code;
	register_t realcode;
	const struct sysent *callp;
	register_t args[];
{
	struct ktr_header kth;
	struct ktr_syscall *ktp;
	register64_t *argp;
	int argsize;
	size_t len;
	u_int i;

	if (callp == NULL)
		callp = p->p_emul->e_sysent;
	
	/* Adjust argsize for 64 bit records.  */
	argsize = callp[code].sy_argsize *
		  sizeof(register64_t) / sizeof(register32_t);
	len = sizeof(struct ktr_syscall) + argsize;

	p->p_traceflag |= KTRFAC_ACTIVE;
	ktrinitheader(&kth, p, KTR_SYSCALL);
	ktp = malloc(len, M_TEMP, M_WAITOK);
	ktp->ktr_code = realcode;
	ktp->ktr_argsize = argsize;
	argp = (register64_t *)((char *)ktp + sizeof(struct ktr_syscall));
	for (i = 0; i < (argsize / sizeof(*argp)); i++)
		*argp++ = args[i];
	kth.ktr_buf = (caddr_t)ktp;
	kth.ktr_len = len;
	(void) ktrwrite(p, &kth);
	free(ktp, M_TEMP);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}
#endif /* KTRACE */
