/*	$NetBSD: cpu.h,v 1.11.6.2 2007/08/05 13:57:26 ad Exp $	*/

/*-
 * Copyright (c) 2007 YAMAMOTO Takashi,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SYS_CPU_H_
#define _SYS_CPU_H_

#include <machine/cpu.h>

#include <sys/lwp.h>

struct cpu_info;

#ifndef cpu_idle
void cpu_idle(void);
#endif

#ifndef cpu_need_resched
void cpu_need_resched(struct cpu_info *, int);
#endif

/* flags for cpu_need_resched */
#define	RESCHED_IMMED	1

#ifndef cpu_did_resched
#define	cpu_did_resched()			\
do {						\
	curcpu()->ci_want_resched = 0;		\
} while (0)
#endif

#ifndef CPU_INFO_ITERATOR
#define	CPU_INFO_ITERATOR		int
#define	CPU_INFO_FOREACH(cii, ci)	\
    (void)cii, ci = curcpu(); ci != NULL; ci = NULL
#endif

lwp_t	*cpu_switchto(lwp_t *, lwp_t *);
struct	cpu_info *cpu_lookup(cpuid_t);
int	cpu_setonline(struct cpu_info *, bool);

extern kmutex_t cpu_lock;

static inline u_int
cpu_index(struct cpu_info *ci)
{
	return ci->ci_index;
}

#endif	/* !_SYS_CPU_H_ */
