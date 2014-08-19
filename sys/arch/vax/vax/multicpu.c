/*	$NetBSD: multicpu.c,v 1.32.12.1 2014/08/20 00:03:27 tls Exp $	*/

/*
 * Copyright (c) 2000 Ludd, University of Lule}, Sweden. All rights reserved.
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
 *      This product includes software developed at Ludd, University of
 *      Lule}, Sweden and its contributors.
 * 4. The name of the author may not be used to endorse or promote products
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

/*
 * CPU-type independent code to spin up other VAX CPU's.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: multicpu.c,v 1.32.12.1 2014/08/20 00:03:27 tls Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/xcall.h>
#include <sys/ipi.h>

#include <uvm/uvm_extern.h>

#include <vax/vax/gencons.h>

#include "ioconf.h"

const struct cpu_mp_dep *mp_dep_call;

struct cpuq {
	SIMPLEQ_ENTRY(cpuq) cq_q;
	struct cpu_info *cq_ci;
	device_t cq_dev;
};

SIMPLEQ_HEAD(, cpuq) cpuq = SIMPLEQ_HEAD_INITIALIZER(cpuq);

extern long avail_start, avail_end;
struct cpu_info_qh cpus = SIMPLEQ_HEAD_INITIALIZER(cpus);

void
cpu_boot_secondary_processors(void)
{
	struct cpuq *q;

	while ((q = SIMPLEQ_FIRST(&cpuq))) {
		SIMPLEQ_REMOVE_HEAD(&cpuq, cq_q);
		(*mp_dep_call->cpu_startslave)(q->cq_ci);
		free(q, M_TEMP);
	}
}

/*
 * Allocate a cpu_info struct and fill it in then prepare for getting
 * started by cpu_boot_secondary_processors().
 */
void
cpu_slavesetup(device_t self, int slotid)
{
	struct cpu_info *ci;
	struct cpuq *cq;
	struct vm_page *pg;
	vaddr_t istackbase;

	KASSERT(device_private(self) == NULL);

	ci = malloc(sizeof(*ci), M_DEVBUF, M_ZERO|M_NOWAIT);
	if (ci == NULL)
		panic("cpu_slavesetup1");

	self->dv_private = ci;
	ci->ci_dev = self;
	ci->ci_slotid = slotid;
	ci->ci_cpuid = device_unit(self);

	/* Allocate an interrupt stack */
	pg = uvm_pagealloc(NULL, 0, NULL, 0);
	if (pg == NULL)
		panic("cpu_slavesetup2");

	istackbase = VM_PAGE_TO_PHYS(pg) | KERNBASE;
	kvtopte(istackbase)->pg_v = 0; /* istack safety belt */

	/* Populate the PCB and the cpu_info struct */
	ci->ci_istack = istackbase + PAGE_SIZE;
	SIMPLEQ_INSERT_TAIL(&cpus, ci, ci_next);

	cq = malloc(sizeof(*cq), M_TEMP, M_NOWAIT|M_ZERO);
	if (cq == NULL)
		panic("cpu_slavesetup3");

	cq->cq_ci = ci;
	cq->cq_dev = ci->ci_dev;
	SIMPLEQ_INSERT_TAIL(&cpuq, cq, cq_q);

	mi_cpu_attach(ci);	/* let the MI parts know about the new cpu */
}

/*
 * Send an IPI of type type to the CPU with logical device number cpu.
 */
void
cpu_send_ipi(int cpu, int type)
{
	struct cpu_info *ci;
	int i;

	if (cpu >= 0) {
		ci = device_lookup_private(&cpu_cd, cpu);
		bbssi(type, &ci->ci_ipimsgs);
		(*mp_dep_call->cpu_send_ipi)(ci);
		return;
	}

	for (i = 0; i < cpu_cd.cd_ndevs; i++) {
		ci = device_lookup_private(&cpu_cd, i);
		if (ci == NULL)
			continue;
		switch (cpu) {
		case IPI_DEST_MASTER:
			if (ci->ci_flags & CI_MASTERCPU) {
				bbssi(type, &ci->ci_ipimsgs);
				(*mp_dep_call->cpu_send_ipi)(ci);
			}
			break;
		case IPI_DEST_ALL:
			if (i == cpu_number())
				continue;	/* No IPI to myself */
			bbssi(type, &ci->ci_ipimsgs);
			(*mp_dep_call->cpu_send_ipi)(ci);
			break;
		}
	}
}

void
cpu_handle_ipi(void)
{
	struct cpu_info * const ci = curcpu();
	int bitno;
	int s;

	s = splhigh();

	while ((bitno = ffs(ci->ci_ipimsgs))) {
		bitno -= 1; /* ffs() starts from 1 */
		bbcci(bitno, &ci->ci_ipimsgs);
		switch (bitno) {
		case IPI_START_CNTX:
#ifdef DIAGNOSTIC
			if (CPU_IS_PRIMARY(ci) == 0)
				panic("cpu_handle_ipi");
#endif
			gencnstarttx();
			break;
		case IPI_SEND_CNCHAR:
#ifdef DIAGNOSTIC
			if (CPU_IS_PRIMARY(ci) == 0)
				panic("cpu_handle_ipi2");
#endif
			(*mp_dep_call->cpu_cnintr)();
			break;
		case IPI_RUNNING:
			break;
		case IPI_TBIA:
			mtpr(0, PR_TBIA);
			break;
		case IPI_DDB:
			Debugger();
			break;
		case IPI_XCALL:
			xc_ipi_handler();
			break;
		case IPI_GENERIC:
			ipi_cpu_handler();
			break;
		default:
			panic("cpu_handle_ipi: bad bit %x", bitno);
		}
	}
	splx(s);
}

/*
 * MD support for xcall(9) interface.
 */

void
xc_send_ipi(struct cpu_info *ci)
{
	KASSERT(kpreempt_disabled());
	KASSERT(curcpu() != ci);

	if (ci) {
		/* Unicast: remote CPU. */
		cpu_send_ipi(ci->ci_cpuid, IPI_XCALL);
	} else {
		/* Broadcast: all, but local CPU (caller will handle it). */
		cpu_send_ipi(IPI_DEST_ALL, IPI_XCALL);
	}
}

void
cpu_ipi(struct cpu_info *ci)
{
	KASSERT(kpreempt_disabled());
	KASSERT(curcpu() != ci);

	if (ci) {
		/* Unicast: remote CPU. */
		cpu_send_ipi(ci->ci_cpuid, IPI_GENERIC);
	} else {
		/* Broadcast: all, but local CPU (caller will handle it). */
		cpu_send_ipi(IPI_DEST_ALL, IPI_GENERIC);
	}
}
