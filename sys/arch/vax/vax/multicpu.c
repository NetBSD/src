/*	$NetBSD: multicpu.c,v 1.16 2004/03/19 20:17:51 matt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: multicpu.c,v 1.16 2004/03/19 20:17:51 matt Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/../vax/gencons.h>

#include "ioconf.h"

struct cpu_mp_dep *mp_dep_call;

static	void slaverun(void);

struct cpuq {
	SIMPLEQ_ENTRY(cpuq) cq_q;
	struct cpu_info *cq_ci;
	struct device *cq_dev;
};

SIMPLEQ_HEAD(, cpuq) cpuq = SIMPLEQ_HEAD_INITIALIZER(cpuq);

extern long avail_start, avail_end, proc0paddr;
struct cpu_info_qh cpus = SIMPLEQ_HEAD_INITIALIZER(cpus);

void
cpu_boot_secondary_processors()
{
	struct cpuq *q;

	while ((q = SIMPLEQ_FIRST(&cpuq))) {
		SIMPLEQ_REMOVE_HEAD(&cpuq, cq_q);
		(*mp_dep_call->cpu_startslave)(q->cq_dev, q->cq_ci);
		free(q, M_TEMP);
	}
}

/*
 * Allocate an UAREA for this CPU, create an idle PCB, get a cpu_info 
 * struct and fill it in and prepare for getting started by 
 * cpu_boot_secondary_processors().
 */
void
cpu_slavesetup(struct device *dev)
{
	struct cpu_mp_softc *sc = (struct cpu_mp_softc *)dev;
	struct cpuq *cq;
	struct cpu_info *ci;
	struct pglist mlist;
	struct vm_page *pg;
	struct pcb *pcb;
	vaddr_t istackbase, scratch;
	int error;

	/* Get an UAREA */
	error = uvm_pglistalloc(USPACE, avail_start, avail_end, 0, 0,
	    &mlist, 1, 1);
	if (error)
		panic("cpu_slavesetup: error %d", error);
	pcb = (struct pcb *)(VM_PAGE_TO_PHYS(TAILQ_FIRST(&mlist)) | KERNBASE);

	/* Copy our own idle PCB */
	memcpy(pcb, (void *)proc0paddr, sizeof(struct user));
	kvtopte((u_int)pcb + REDZONEADDR)->pg_v = 0;

	/* Allocate an interrupt stack */
	pg = uvm_pagealloc(NULL, 0, NULL, 0);
	if (pg == NULL)
		panic("cpu_slavesetup2");
	istackbase = VM_PAGE_TO_PHYS(pg) | KERNBASE;
	kvtopte(istackbase)->pg_v = 0; /* istack safety belt */

	/* Get scratch pages for different use, see pmap.c for comment */
	pg = uvm_pagealloc(NULL, 0, NULL, 0);
	if (pg == NULL)
		panic("cpu_slavesetup3");
	scratch = VM_PAGE_TO_PHYS(pg) | KERNBASE;

	/* Populate the PCB and the cpu_info struct */
	ci = &sc->sc_ci;
	ci->ci_dev = dev;
	ci->ci_exit = scratch;
	ci->ci_pcb = (void *)((intptr_t)pcb & ~KERNBASE);
	ci->ci_istack = istackbase + PAGE_SIZE;
	SIMPLEQ_INSERT_TAIL(&cpus, ci, ci_next);
	pcb->KSP = (uintptr_t)pcb + USPACE; /* Idle kernel stack */
	pcb->SSP = (uintptr_t)ci;
	pcb->PC = (uintptr_t)slaverun + 2;
	pcb->PSL = 0;

	cq = malloc(sizeof(*cq), M_TEMP, M_NOWAIT);
	if (cq == NULL)
		panic("cpu_slavesetup4");
	memset(cq, 0, sizeof(*cq));
	cq->cq_ci = ci;
	cq->cq_dev = dev;
	SIMPLEQ_INSERT_TAIL(&cpuq, cq, cq_q);
}

volatile int sta;

void
slaverun()
{
	struct cpu_info *ci = curcpu();

	((volatile struct cpu_info *)ci)->ci_flags |= CI_RUNNING;
	cpu_send_ipi(IPI_DEST_MASTER, IPI_RUNNING);
	printf("%s: running\n", ci->ci_dev->dv_xname);
	while (sta != ci->ci_dev->dv_unit)
		;
	splsched();
	sched_lock_idle();
	cpu_switch(NULL,NULL);
}

/*
 * Send an IPI of type type to the CPU with logical device number cpu.
 */
void
cpu_send_ipi(int cpu, int type)
{
	struct cpu_mp_softc *sc;
	int i;

	if (cpu >= 0) {
		sc = cpu_cd.cd_devs[cpu];
		bbssi(type, &sc->sc_ci.ci_ipimsgs);
		(*mp_dep_call->cpu_send_ipi)(&sc->sc_dev);
		return;
	}

	for (i = 0; i < cpu_cd.cd_ndevs; i++) {
		sc = cpu_cd.cd_devs[i];
		if (sc == NULL)
			continue;
		switch (cpu) {
		case IPI_DEST_MASTER:
			if (sc->sc_ci.ci_flags & CI_MASTERCPU) {
				bbssi(type, &sc->sc_ci.ci_ipimsgs);
				(*mp_dep_call->cpu_send_ipi)(&sc->sc_dev);
			}
			break;
		case IPI_DEST_ALL:
			if (i == cpu_number())
				continue;	/* No IPI to myself */
			bbssi(type, &sc->sc_ci.ci_ipimsgs);
			(*mp_dep_call->cpu_send_ipi)(&sc->sc_dev);
			break;
		}
	}
}

void
cpu_handle_ipi()
{
	struct cpu_info *ci = curcpu();
	int bitno;

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
		default:
			panic("cpu_handle_ipi: bad bit %x", bitno);
		}
	}
}
