/*	$NetBSD: cpu.c,v 1.26 2002/06/02 14:44:40 drochner Exp $	*/

/*-
 * Copyright (c) 2001 Tsubai Masanari.
 * Copyright (c) 1998, 1999, 2001 Internet Research Institute, Inc.
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
 *	This product includes software developed by
 *	Internet Research Institute, Inc.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_l2cr_config.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>
#include <dev/ofw/openfirm.h>
#include <powerpc/mpc6xx/hid.h>
#include <powerpc/openpic.h>

#include <machine/autoconf.h>
#include <machine/bat.h>
#include <machine/fpu.h>
#include <machine/pcb.h>
#include <machine/pio.h>

int cpumatch(struct device *, struct cfdata *, void *);
void cpuattach(struct device *, struct device *, void *);

void identifycpu(char *);
static void ohare_init(void);
int cpu_spinup(void);
void cpu_hatch(void);
void cpu_spinup_trampoline(void);
int cpuintr(void *v);

struct cfattach cpu_ca = {
	sizeof(struct device), cpumatch, cpuattach
};

extern struct cfdriver cpu_cd;

#define HH_ARBCONF		0xf8000090
#define HH_INTR_SECONDARY	0xf80000c0
#define HH_INTR_PRIMARY		0xf3019000
#define GC_IPI_IRQ		30

int
cpumatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;
	int *reg = ca->ca_reg;
	int node;

	if (strcmp(ca->ca_name, cpu_cd.cd_name) != 0)
		return 0;

	node = OF_finddevice("/cpus");
	if (node != -1) {
		for (node = OF_child(node); node != 0; node = OF_peer(node)) {
			uint32_t cpunum;
			int l;
			l = OF_getprop(node, "reg", &cpunum, sizeof(cpunum));
			if (l == 4 && reg[0] == cpunum)
				return 1;
		}
	}
	switch (reg[0]) {
	case 0:	/* primary CPU */
		return 1;
	case 1:	/* secondary CPU */
		if (OF_finddevice("/hammerhead") != -1)
			if (in32rb(HH_ARBCONF) & 0x02)
				return 1;
		break;
	}

	return 0;
}

void
cpuattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct cpu_info *ci;
	struct confargs *ca = aux;
	int id = ca->ca_reg[0];

	ci = cpu_attach_common(self, id);

	if (id > 0) {
#ifdef MULTIPROCESSOR
		if (ci != NULL)
			cpu_spinup();
#endif
		return;
	}
	if (ci == NULL)
		return;

	if (OF_finddevice("/bandit/ohare") != -1) {
		printf("%s", self->dv_xname);
		ohare_init();
	}
}

#define CACHE_REG 0xf8000000

void
ohare_init()
{
	u_int *cache_reg, x;

	/* enable L2 cache */
	cache_reg = mapiodev(CACHE_REG, NBPG);
	if (((cache_reg[2] >> 24) & 0x0f) >= 3) {
		x = cache_reg[4];
		if ((x & 0x10) == 0)
                	x |= 0x04000000;
		else
                	x |= 0x04000020;

		cache_reg[4] = x;
		printf(": ohare L2 cache enabled\n");
	}
}

#ifdef MULTIPROCESSOR

struct cpu_hatch_data {
	int running;
	int pir;
	int hid0;
	int sdr1;
	int sr[16];
	int tbu, tbl;
};

volatile struct cpu_hatch_data *cpu_hatch_data;
volatile int cpu_hatch_stack;

int
cpu_spinup()
{
	volatile struct cpu_hatch_data hatch_data, *h = &hatch_data;
	int i;
	struct pcb *pcb;
	struct pglist mlist;
	int error;
	int size = 0;
	char *cp;

	/*
	 * Allocate some contiguous pages for the idle PCB and stack
	 * from the lowest 256MB (because bat0 always maps it va == pa).
	 */
	size += USPACE;
	size += 8192;	/* INTSTK */
	size += 4096;	/* SPILLSTK */

	error = uvm_pglistalloc(size, 0x0, 0x10000000, 0, 0, &mlist, 1, 1);
	if (error) {
		printf(": unable to allocate idle stack\n");
		return -1;
	}

	cp = (void *)VM_PAGE_TO_PHYS(TAILQ_FIRST(&mlist));
	memset(cp, 0, size);

	pcb = (struct pcb *)cp;
	cp += USPACE;
	cpu_info[1].ci_idle_pcb = pcb;

	cpu_info[1].ci_intstk = cp + 8192;
	cp += 8192;
	cpu_info[1].ci_spillstk = cp + 4096;
	cp += 4096;

	/*
	 * Initialize the idle stack pointer, reserving space for an
	 * (empty) trapframe (XXX is the trapframe really necessary?)
	 */
	pcb->pcb_sp = (paddr_t)pcb + USPACE - sizeof(struct trapframe);

	cpu_hatch_data = h;
	h->running = 0;
	h->pir = 1;
	cpu_hatch_stack = pcb->pcb_sp;
	cpu_info[1].ci_lasttb = cpu_info[0].ci_lasttb;

	/* copy special registers */
	asm volatile ("mfspr %0,1008" : "=r"(h->hid0));
	asm volatile ("mfsdr1 %0" : "=r"(h->sdr1));
	for (i = 0; i < 16; i++)
		asm ("mfsrin %0,%1" : "=r"(h->sr[i]) : "r"(i << ADDR_SR_SHFT));

	asm volatile ("sync; isync");

	if (openpic_base) {
		/* XXX */
		panic("cpu_spinup");
	} else {
		/* Start secondary cpu and stop timebase. */
		out32(0xf2800000, (int)cpu_spinup_trampoline);
		out32(HH_INTR_SECONDARY, ~0);
		out32(HH_INTR_SECONDARY, 0);

		/* sync timebase (XXX shouldn't be zero'ed) */
		asm volatile ("mttbl %0; mttbu %0; mttbl %0" :: "r"(0));

		/*
		 * wait for secondary spin up (1.5ms @ 604/200MHz)
		 * XXX we cannot use delay() here because timebase is not
		 * running.
		 */
		for (i = 0; i < 100000; i++)
			if (h->running)
				break;

		/* Start timebase. */
		out32(0xf2800000, 0x100);
		out32(HH_INTR_SECONDARY, ~0);
		out32(HH_INTR_SECONDARY, 0);
	}

	delay(100000);		/* wait for secondary printf */

	if (h->running == 0) {
		printf(": secondary cpu didn't start\n");
		return -1;
	}

	if (!openpic_base) {
		/* Register IPI. */
		intr_establish(GC_IPI_IRQ, IST_LEVEL, IPL_HIGH, cpuintr, NULL);
	}

	return 0;
}

volatile static int start_secondary_cpu;
extern long ticks_per_intr;

void
cpu_hatch()
{
	volatile struct cpu_hatch_data *h = cpu_hatch_data;
	u_int msr;
	int i;
	char model[80];

	/* Initialize timebase. */
	asm ("mttbl %0; mttbu %0; mttbl %0" :: "r"(0));

	/* Set PIR (Processor Identification Register).  i.e. whoami */
	asm volatile ("mtspr 1023,%0" :: "r"(h->pir));
	asm volatile ("mtsprg 0,%0" :: "r"(&cpu_info[h->pir]));

	/* Initialize MMU. */
	asm ("mtibatu 0,%0" :: "r"(0));
	asm ("mtibatu 1,%0" :: "r"(0));
	asm ("mtibatu 2,%0" :: "r"(0));
	asm ("mtibatu 3,%0" :: "r"(0));
	asm ("mtdbatu 0,%0" :: "r"(0));
	asm ("mtdbatu 1,%0" :: "r"(0));
	asm ("mtdbatu 2,%0" :: "r"(0));
	asm ("mtdbatu 3,%0" :: "r"(0));

	asm ("mtspr 1008,%0" :: "r"(h->hid0));

	asm ("mtibatl 0,%0; mtibatu 0,%1;"
	     "mtdbatl 0,%0; mtdbatu 0,%1;"
		:: "r"(battable[0].batl), "r"(battable[0].batu));

	/* XXX obio (for now) */
	asm ("mtibatl 1,%0; mtibatu 1,%1;"
	     "mtdbatl 1,%0; mtdbatu 1,%1;"
		:: "r"(battable[0xf].batl), "r"(battable[0xf].batu));

	for (i = 0; i < 16; i++)
		asm ("mtsrin %0,%1" :: "r"(h->sr[i]), "r"(i << ADDR_SR_SHFT));
	asm ("mtsdr1 %0" :: "r"(h->sdr1));

	asm volatile ("isync");

	/* Enable I/D address translations. */
	asm volatile ("mfmsr %0" : "=r"(msr));
	msr |= PSL_IR|PSL_DR|PSL_ME|PSL_RI;
	asm volatile ("mtmsr %0" :: "r"(msr));

	asm volatile ("sync; isync");
	h->running = 1;

	cpu_identify(model, sizeof(model));
	printf(": %s, ID %d\n", model, cpu_number());

	while (start_secondary_cpu == 0);

	printf("secondary CPU started\n");
	asm volatile ("mtdec %0" :: "r"(ticks_per_intr));

	if (!openpic_base)
		out32(HH_INTR_SECONDARY, ~0);	/* Reset interrupt. */

	curcpu()->ci_ipending = 0;
	curcpu()->ci_cpl = 0;
}

void
cpu_boot_secondary_processors()
{
	start_secondary_cpu = 1;
}

/*static*/ volatile int IPI[2];

void
macppc_send_ipi(ci, mesg)
	volatile struct cpu_info *ci;
	int mesg;
{
	int cpu_id = ci->ci_cpuid;

	/* printf("send_ipi(%d,%d)\n", cpu_id, mesg); */
	IPI[cpu_id] |= mesg;

	if (openpic_base) {
		/* XXX */
	} else {
		switch (cpu_id) {
		case 0:
			in32(HH_INTR_PRIMARY);
			break;
		case 1:
			out32(HH_INTR_SECONDARY, ~0);
			out32(HH_INTR_SECONDARY, 0);
			break;
		}
	}
}

int
cpuintr(v)
	void *v;
{
	int cpu_id = cpu_number();
	int msr;

	/* printf("cpuintr{%d}\n", cpu_id); */

	if (IPI[cpu_id] & MACPPC_IPI_FLUSH_FPU) {
		if (curcpu()->ci_fpuproc) {
			save_fpu(curcpu()->ci_fpuproc);
			if (curcpu()->ci_fpuproc)
				panic("cpuintr");
		}
	}
	if (IPI[cpu_id] & MACPPC_IPI_HALT) {
		printf("halt{%d}\n", cpu_id);
		asm volatile ("mfmsr %0" : "=r"(msr));
		msr &= ~PSL_EE;
		msr |= PSL_POW;
		for (;;) {
			asm volatile ("sync; isync");
			asm volatile ("mtmsr %0" :: "r"(msr));
		}
	}
	IPI[cpu_id] = 0;

	return 1;
}
#endif /* MULTIPROCESSOR */
