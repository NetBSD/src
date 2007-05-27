/*	$NetBSD: cpu.h,v 1.6.66.1 2007/05/27 12:26:51 ad Exp $	*/

/*-
 * Copyright (c) 2002 Ben Harris
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

#if defined(_KERNEL) && defined(_KERNEL_OPT) && !defined(_LOCORE)
#include "opt_multiprocessor.h"

#ifdef MULTIPROCESSOR
void	cpu_boot_secondary_processors(void);

#define MP_CPU_INFO_MEMBERS						\
	cpuid_t ci_cpuid;						\
	struct proc *ci_curproc;					\
	struct pcb *ci_curpcb;

#define CPU_MAXNUM 8

extern struct cpu_info *cpu_info[];

#define CPU_IS_PRIMARY(ci)	((ci)->ci_cpuid == 0)
extern cpuid_t cpu_number(void);
#define curcpu()		(cpu_info[cpu_number()])

extern cpuid_t cpu_next(cpuid_t);

#define CPU_INFO_ITERATOR cpuid_t
#define CPU_INFO_FOREACH(cii, ci)					\
	cii = 0, ci = cpu_info[0];					\
	cii < CPU_MAXNUM;						\
	cii = cpu_next(cii), ci = cpu_info[cii]

#endif /* MULTIPROCESSOR */
#endif /* _KERNEL && _KERNEL_OPT && !_LOCORE */

#include <arm/cpu.h>
