/*	$NetBSD: cpu.h,v 1.7 2009/01/02 22:03:00 pooka Exp $	*/

/*
 * Copyright (c) 2007 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by Google Summer of Code.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SYS_RUMP_CPU_H_
#define _SYS_RUMP_CPU_H_

#include <sys/cpu_data.h>
#include <machine/pcb.h>

struct cpu_info {
	struct cpu_data ci_data;
	cpuid_t ci_cpuid;

/*
 * XXX: horrible workaround for vax lock.h.
 * I eventually want to nuke rump include/machine, so don't waste
 * energy fighting with this.
 */
#ifdef __vax__
	int ci_ipimsgs;
#define IPI_SEND_CNCHAR 0
#define IPI_DDB 0
#endif /* __vax__ */
};

/* more dirty rotten vax kludges */
#ifdef __vax__
static __inline void cpu_handle_ipi(void) {}
#endif /* __vax__ */

extern struct cpu_info rump_cpu;
#define curcpu() (&rump_cpu)
#define cpu_number() 0 /* XXX: good enuf? */

struct lwp *rump_get_curlwp(void); /* XXX */
#define curlwp rump_get_curlwp()

#endif /* _SYS_RUMP_CPU_H_ */
