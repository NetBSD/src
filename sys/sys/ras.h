/*	$NetBSD: ras.h,v 1.11 2008/04/28 20:24:11 martin Exp $	*/

/*-
 * Copyright (c) 2002, 2004, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gregory McGarry.
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

#ifndef _SYS_RAS_H_
#define _SYS_RAS_H_

#include <sys/types.h>
#include <sys/queue.h>

struct ras {
	struct ras	*ras_next;
	void		*ras_startaddr;
	void		*ras_endaddr;
};

#define RAS_INSTALL		0
#define RAS_PURGE		1
#define RAS_PURGE_ALL		2

#ifdef _KERNEL

struct proc;

void	*ras_lookup(struct proc *, void *);
int	ras_fork(struct proc *, struct proc *);
int	ras_purgeall(void);

#else

#ifndef	RAS_DECL

#define	RAS_DECL(name)							\
extern void __CONCAT(name,_ras_start(void)), __CONCAT(name,_ras_end(void))

#endif	/* RAS_DECL */

/*
 * RAS_START and RAS_END contain implicit instruction reordering
 * barriers.  See __insn_barrier() in <sys/cdefs.h>.
 */
#define	RAS_START(name)							\
	__asm volatile(".globl " ___STRING(name) "_ras_start\n"	\
			 ___STRING(name) "_ras_start:" 			\
	    ::: "memory")

#define	RAS_END(name)							\
	__asm volatile(".globl " ___STRING(name) "_ras_end\n"		\
			 ___STRING(name) "_ras_end:"			\
	    ::: "memory")

#define	RAS_ADDR(name)	((void *)(uintptr_t) __CONCAT(name,_ras_start))
#define	RAS_SIZE(name)	((size_t)((uintptr_t) __CONCAT(name,_ras_end) -	\
				  (uintptr_t) __CONCAT(name,_ras_start)))

__BEGIN_DECLS
int rasctl(void *, size_t, int);
__END_DECLS

#endif /* _KERNEL */

#endif /* !_SYS_RAS_H_ */
