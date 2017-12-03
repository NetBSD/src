/*	$NetBSD: intr.h,v 1.27.14.1 2017/12/03 11:36:41 jdolecek Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#ifndef	_SGIMIPS_INTR_H_
#define	_SGIMIPS_INTR_H_

#include <mips/intr.h>

#ifdef _KERNEL
#ifdef __INTR_PRIVATE

#include <sys/queue.h>
#include <sys/types.h>
#include <sys/evcnt.h>

#define NINTR	32

struct sgimips_intrhand {
	LIST_ENTRY(sgimips_intrhand) ih_q;
	int	(*ih_fun) (void *);
	void	 *ih_arg;
	struct	sgimips_intr *ih_intrhead;
	struct	sgimips_intrhand *ih_next;
	int	ih_pending;
};

struct sgimips_intr {
	LIST_HEAD(,sgimips_intrhand) intr_q;
	struct	evcnt ih_evcnt;
	unsigned long intr_ipl;
};

extern struct sgimips_intrhand intrtab[];

#endif /* __INTR_PRIVATE */

#ifndef _LOCORE
void *cpu_intr_establish(int, int, int (*)(void *), void *);
void mips1_fpu_intr(vaddr_t, uint32_t, uint32_t);
#endif
#endif /* !_KERNEL */

#endif	/* !_SGIMIPS_INTR_H_ */
