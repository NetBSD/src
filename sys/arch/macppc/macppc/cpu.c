/*	$NetBSD: cpu.c,v 1.44 2007/05/29 13:26:39 macallan Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.44 2007/05/29 13:26:39 macallan Exp $");

#include "opt_ppcparam.h"
#include "opt_multiprocessor.h"
#include "opt_altivec.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>
#include <dev/ofw/openfirm.h>
#include <powerpc/oea/hid.h>
#include <powerpc/oea/bat.h>
#include <powerpc/openpic.h>
#include <powerpc/atomic.h>
#include <powerpc/spr.h>
#ifdef ALTIVEC
#include <powerpc/altivec.h>
#endif

#include <machine/autoconf.h>
#include <machine/fpu.h>
#include <machine/pcb.h>
#include <machine/pio.h>
#include <machine/trap.h>

int cpumatch(struct device *, struct cfdata *, void *);
void cpuattach(struct device *, struct device *, void *);

void identifycpu(char *);
static void ohare_init(void);
int cpu_spinup(struct device *, struct cpu_info *);
void cpu_hatch(void);
void cpu_spinup_trampoline(void);
int cpuintr(void *);

CFATTACH_DECL(cpu, sizeof(struct device),
    cpumatch, cpuattach, NULL, NULL);

extern struct cfdriver cpu_cd;

#define HH_ARBCONF		0xf8000090
#define HH_INTR_SECONDARY	0xf80000c0
#define HH_INTR_PRIMARY		0xf3019000
#define GC_IPI_IRQ		30

extern uint32_t ticks_per_intr;

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

void cpu_OFgetspeed(struct device *, struct cpu_info *);

void
cpu_OFgetspeed(struct device *self, struct cpu_info *ci)
{
	int	node;

	node = OF_finddevice("/cpus");
	if (node != -1) {
		for (node = OF_child(node); node; node = OF_peer(node)) {
			uint32_t cpunum;
			int l;
			l = OF_getprop(node, "reg", &cpunum, sizeof(cpunum));
			if (l == 4 && ci->ci_cpuid == cpunum) {
				uint32_t cf;
				l = OF_getprop(node, "clock-frequency",
						&cf, sizeof(cf));
				if (l == 4)
					ci->ci_khz = cf / 1000;
				break;
			}
		}
	}
	if (ci->ci_khz) {
		aprint_normal("%s: %u.%02u MHz\n",
			      self->dv_xname,
			      ci->ci_khz / 1000, (ci->ci_khz / 10) % 100);
	}
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
	if (ci == NULL)
		return;

	if (ci->ci_khz == 0) {
		cpu_OFgetspeed(self, ci);
	}

	if (id > 0) {
#ifdef MULTIPROCESSOR
		cpu_spinup(self, ci);
#endif
		return;
	}

	if (OF_finddevice("/bandit/ohare") != -1) {
		printf("%s", self->dv_xname);
		ohare_init();
	}
}

#define CACHE_REG 0xf8000000

void
ohare_init()
{
	volatile uint32_t *cache_reg, x;

	/* enable L2 cache */
	cache_reg = mapiodev(CACHE_REG, PAGE_SIZE);
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
	struct device *self;
	struct cpu_info *ci;
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
cpu_spinup(self, ci)
	struct device *self;
	struct cpu_info *ci;
{
	volatile struct cpu_hatch_data hatch_data, *h = &hatch_data;
	int i;
	struct pcb *pcb;
	struct pglist mlist;
	int pvr, vers;
	int error;
	int size = 0;
	char *cp;

	pvr = mfpvr();
	vers = pvr >> 16;

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

	cpu_info[1].ci_intstk = cp + INTSTK;
	cp += INTSTK;

	/*
	 * Initialize the idle stack pointer, reserving space for an
	 * (empty) trapframe (XXX is the trapframe really necessary?)
	 */
	pcb->pcb_sp = (paddr_t)pcb + USPACE - sizeof(struct trapframe);

	cpu_hatch_data = h;
	h->running = 0;
	h->self = self;
	h->ci = ci;
	h->pir = 1;
	cpu_hatch_stack = pcb->pcb_sp;
	cpu_info[1].ci_lasttb = cpu_info[0].ci_lasttb;

	/* copy special registers */
	__asm volatile ("mfspr %0,%1" : "=r"(h->hid0) : "n"(SPR_HID0));
	__asm volatile ("mfsdr1 %0" : "=r"(h->sdr1));
	for (i = 0; i < 16; i++)
		__asm ("mfsrin %0,%1" : "=r"(h->sr[i]) : "r"(i << ADDR_SR_SHFT));

	__asm volatile ("sync; isync");

	if (openpic_base) {
		uint64_t tb;
		u_int kl_base = 0x80000000;	/* XXX */
		u_int gpio = kl_base + 0x5c;	/* XXX */
		u_int node, off;
		char cpupath[32];

		*(u_int *)EXC_RST =		/* ba cpu_spinup_trampoline */
		    0x48000002 | (u_int)cpu_spinup_trampoline;
		__syncicache((void *)EXC_RST, 0x100);

		h->running = -1;

		/* see if there's an OF property for the reset register */
		sprintf(cpupath, "/cpus/@%x", ci->ci_cpuid);
		node = OF_finddevice(cpupath);
		if (node == -1) {
			printf(": no OF node for CPU %d?\n", ci->ci_cpuid);
			return -1;
		}
		if (OF_getprop(node, "soft-reset", &off, 4) == 4) {
			gpio = kl_base + off;
		}

		/* Start secondary CPU. */
		out8(gpio, 4);
		out8(gpio, 5);

		/* Sync timebase. */
		tb = mftb();
		tb += 100000;  /* 3ms @ 33MHz */

		h->tbu = tb >> 32;
		h->tbl = tb & 0xffffffff;

		while (tb > mftb())
			;

		__asm volatile ("sync; isync");
		h->running = 0;

		delay(500000);
	} else {
		/* Start secondary CPU and stop timebase. */
		out32(0xf2800000, (int)cpu_spinup_trampoline);
		out32(HH_INTR_SECONDARY, ~0);
		out32(HH_INTR_SECONDARY, 0);

		/* sync timebase (XXX shouldn't be zero'ed) */
		__asm volatile ("mttbl %0; mttbu %0; mttbl %0" :: "r"(0));

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

	delay(200000);		/* wait for secondary printf */

	if (h->running == 0) {
		printf(": secondary CPU didn't start\n");
		return -1;
	}

	if (!openpic_base) {
		/* Register IPI. */
		intr_establish(GC_IPI_IRQ, IST_LEVEL, IPL_HIGH, cpuintr, NULL);
	}

	return 0;
}

volatile static int start_secondary_cpu;

void
cpu_hatch()
{
	volatile struct cpu_hatch_data *h = cpu_hatch_data;
	u_int msr;
	int i;

	/* Initialize timebase. */
	__asm ("mttbl %0; mttbu %0; mttbl %0" :: "r"(0));

	/* Set PIR (Processor Identification Register).  i.e. whoami */
	__asm volatile ("mtspr 1023,%0" :: "r"(h->pir));
	__asm volatile ("mtsprg 0,%0" :: "r"(h->ci));

	/* Initialize MMU. */
	__asm ("mtibatu 0,%0" :: "r"(0));
	__asm ("mtibatu 1,%0" :: "r"(0));
	__asm ("mtibatu 2,%0" :: "r"(0));
	__asm ("mtibatu 3,%0" :: "r"(0));
	__asm ("mtdbatu 0,%0" :: "r"(0));
	__asm ("mtdbatu 1,%0" :: "r"(0));
	__asm ("mtdbatu 2,%0" :: "r"(0));
	__asm ("mtdbatu 3,%0" :: "r"(0));

	__asm ("mtspr %1,%0" :: "r"(h->hid0), "n"(SPR_HID0));

	__asm ("mtibatl 0,%0; mtibatu 0,%1;"
	     "mtdbatl 0,%0; mtdbatu 0,%1;"
		:: "r"(battable[0].batl), "r"(battable[0].batu));

	if (openpic_base) {
		__asm ("mtibatl 1,%0; mtibatu 1,%1;"
		     "mtdbatl 1,%0; mtdbatu 1,%1;"
			:: "r"(battable[0x8].batl), "r"(battable[0x8].batu));
	} else {
		__asm ("mtibatl 1,%0; mtibatu 1,%1;"
		     "mtdbatl 1,%0; mtdbatu 1,%1;"
			:: "r"(battable[0xf].batl), "r"(battable[0xf].batu));
	}

	for (i = 0; i < 16; i++)
		__asm ("mtsrin %0,%1" :: "r"(h->sr[i]), "r"(i << ADDR_SR_SHFT));
	__asm ("mtsdr1 %0" :: "r"(h->sdr1));

	__asm volatile ("isync");

	/* Enable I/D address translations. */
	__asm volatile ("mfmsr %0" : "=r"(msr));
	msr |= PSL_IR|PSL_DR|PSL_ME|PSL_RI;
	__asm volatile ("mtmsr %0" :: "r"(msr));

	__asm volatile ("sync; isync");

	if (openpic_base) {
		/* Sync timebase. */
		u_int tbu = h->tbu;
		u_int tbl = h->tbl;
		while (h->running == -1)
			;
		__asm volatile ("sync; isync");
		__asm volatile ("mttbl %0" :: "r"(0));
		__asm volatile ("mttbu %0" :: "r"(tbu));
		__asm volatile ("mttbl %0" :: "r"(tbl));
	}

	cpu_setup(h->self, h->ci);

	h->running = 1;
	__asm volatile ("sync; isync");

	while (start_secondary_cpu == 0)
		;

	__asm volatile ("sync; isync");

	printf("cpu%d: started\n", cpu_number());
	__asm volatile ("mtdec %0" :: "r"(ticks_per_intr));

	if (openpic_base)
		openpic_set_priority(cpu_number(), 0);
	else
		out32(HH_INTR_SECONDARY, ~0);	/* Reset interrupt. */

	curcpu()->ci_ipending = 0;
	curcpu()->ci_cpl = 0;
}

void
cpu_boot_secondary_processors()
{

	start_secondary_cpu = 1;
	__asm volatile ("sync");
}

static volatile u_long IPI[CPU_MAXNUM];

void
macppc_send_ipi(ci, mesg)
	volatile struct cpu_info *ci;
	u_long mesg;
{
	int cpu_id = ci->ci_cpuid;

	/* printf("send_ipi(%d, 0x%lx)\n", cpu_id, mesg); */
	atomic_setbits_ulong(&IPI[cpu_id], mesg);

	if (openpic_base) {
		openpic_write(OPENPIC_IPI(cpu_number(), 1), 1 << cpu_id);
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

/*
 * Process IPIs.  External interrupts are blocked.
 */
int
cpuintr(v)
	void *v;
{
	int cpu_id = cpu_number();
	int msr;
	u_long ipi;

	/* printf("cpuintr{%d}\n", cpu_id); */

	ipi = atomic_loadlatch_ulong(&IPI[cpu_id], 0);
	if (ipi & MACPPC_IPI_FLUSH_FPU) {
		save_fpu_cpu();
	}
#ifdef ALTIVEC
	if (ipi & MACPPC_IPI_FLUSH_VEC) {
		save_vec_cpu();
	}
#endif
	if (ipi & MACPPC_IPI_HALT) {
		printf("halt{%d}\n", cpu_id);
		msr = (mfmsr() & ~PSL_EE) | PSL_POW;
		for (;;) {
			__asm volatile ("sync; isync");
			mtmsr(msr);
		}
	}
	return 1;
}
#endif /* MULTIPROCESSOR */
