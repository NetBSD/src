/*	$NetBSD: multicpu.c,v 1.6 2001/05/29 21:55:00 ragge Exp $	*/

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

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>

#include "ioconf.h"

static	void slaverun(void);

struct cpuq {
	SIMPLEQ_ENTRY(cpuq) cq_q;
	struct cpu_info *cq_ci;
	struct device *cq_dev;
};

SIMPLEQ_HEAD(, cpuq) cpuq = SIMPLEQ_HEAD_INITIALIZER(cpuq);

extern long avail_start, avail_end, proc0paddr;

void
cpu_boot_secondary_processors()
{
	struct cpuq *q;

	while ((q = SIMPLEQ_FIRST(&cpuq))) {
		SIMPLEQ_REMOVE_HEAD(&cpuq, q, cq_q);
		(*dep_call->cpu_startslave)(q->cq_dev, q->cq_ci);
		free(q, M_TEMP);
	}
}

/*
 * Allocate an UAREA for this CPU, create an idle PCB, get a cpu_info 
 * struct and fill it in and prepare for getting started by 
 * cpu_boot_secondary_processors().
 */
struct cpu_info *
cpu_slavesetup(struct device *dev)
{
	struct cpuq *cq;
	struct cpu_info *ci;
	struct pglist mlist;
	struct vm_page *pg;
	struct pcb *pcb;
	vaddr_t istackbase, scratch;
	int error;

	/* Get an UAREA */
	TAILQ_INIT(&mlist);
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
	ci = (struct cpu_info *)(scratch + VAX_NBPG);
	memset(ci, 0, sizeof(struct cpu_info));
	ci->ci_dev = dev;
	ci->ci_exit = scratch;
	(u_long)ci->ci_pcb = (u_long)pcb & ~KERNBASE;
	ci->ci_istack = istackbase + NBPG;
	pcb->KSP = (u_long)pcb + USPACE; /* Idle kernel stack */
	pcb->SSP = (u_long)ci;
	pcb->PC = (u_long)slaverun + 2;
	pcb->PSL = 0;

	cq = malloc(sizeof(*cq), M_TEMP, M_NOWAIT);
	if (cq == NULL)
		panic("cpu_slavesetup4");
	memset(cq, 0, sizeof(*cq));
	cq->cq_ci = ci;
	cq->cq_dev = dev;
	SIMPLEQ_INSERT_TAIL(&cpuq, cq, cq_q);
	return ci;
}

void
slaverun()
{
	struct cpu_info *ci = curcpu();

	((volatile struct cpu_info *)ci)->ci_flags |= CI_RUNNING;
	printf("%s: running\n", ci->ci_dev->dv_xname);
	splsched();
	sched_lock_idle();
	cpu_switch(0);
}
