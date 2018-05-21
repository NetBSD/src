/*	$NetBSD: cpu.c,v 1.60.28.2 2018/05/21 04:36:01 pgoyette Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.60.28.2 2018/05/21 04:36:01 pgoyette Exp $");

#include "opt_ppcparam.h"
#include "opt_multiprocessor.h"
#include "opt_interrupt.h"
#include "opt_altivec.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/types.h>
#include <sys/lwp.h>

#include <dev/ofw/openfirm.h>
#include <powerpc/oea/hid.h>
#include <powerpc/oea/bat.h>
#include <powerpc/openpic.h>
#include <powerpc/spr.h>
#include <powerpc/oea/spr.h>
#ifdef ALTIVEC
#include <powerpc/altivec.h>
#endif

#ifdef MULTIPROCESSOR
#include <arch/powerpc/pic/picvar.h>
#include <arch/powerpc/pic/ipivar.h>
#endif

#include <machine/autoconf.h>
#include <machine/fpu.h>
#include <machine/pcb.h>
#include <machine/pio.h>
#include <machine/trap.h>

#include "pic_openpic.h"
#include "pic_u3_ht.h"

#ifndef OPENPIC
#if NPIC_OPENPIC > 0
#define OPENPIC
#endif /* NOPENPIC > 0 */
#endif /* OPENPIC */

int cpumatch(device_t, cfdata_t, void *);
void cpuattach(device_t, device_t, void *);

void identifycpu(char *);
static void ohare_init(void);

CFATTACH_DECL_NEW(cpu, 0,
    cpumatch, cpuattach, NULL, NULL);

extern struct cfdriver cpu_cd;

#define HH_INTR_SECONDARY	0xf80000c0
#define HH_ARBCONF		0xf8000090

extern uint32_t ticks_per_intr;

#ifdef OPENPIC
extern void openpic_set_priority(int, int);
#endif

int
cpumatch(device_t parent, cfdata_t cf, void *aux)
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

void cpu_OFgetspeed(device_t, struct cpu_info *);

void
cpu_OFgetspeed(device_t self, struct cpu_info *ci)
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
}

void
cpuattach(device_t parent, device_t self, void *aux)
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
		printf("%s", device_xname(self));
		ohare_init();
	}
}

#define CACHE_REG 0xf8000000

void
ohare_init(void)
{
	volatile uint32_t *cache_reg, x;

	/* enable L2 cache */
	cache_reg = mapiodev(CACHE_REG, PAGE_SIZE, false);
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

#if NPIC_U3_HT > 0
extern int have_u3_ht(void);
extern void __u3_ht_set_priority(int, int);
#else
#define have_u3_ht() 0
#define __u3_ht_set_priority(a, b)
#endif

int
md_setup_trampoline(volatile struct cpu_hatch_data *h, struct cpu_info *ci)
{
#ifdef OPENPIC
	if ((openpic_base != NULL) || have_u3_ht()) {
		uint32_t kl_base = (uint32_t)oea_mapiodev(0x80000000, 0x1000);
		uint32_t gpio = kl_base + 0x5c;	/* XXX */
		u_int node, off;
		char cpupath[32];

		/* construct an absolute branch instruction */
		*(u_int *)EXC_RST =		/* ba cpu_spinup_trampoline */
		    0x48000002 | (u_int)cpu_spinup_trampoline;
		__syncicache((void *)EXC_RST, 0x100);
		h->hatch_running = -1;

		/* see if there's an OF property for the reset register */
		snprintf(cpupath, sizeof(cpupath), "/cpus/@%x", ci->ci_cpuid);
		node = OF_finddevice(cpupath);
		if (node == -1) {
			printf(": no OF node for CPU %d?\n", ci->ci_cpuid);
			return -1;
		}
		if (OF_getprop(node, "soft-reset", &off, 4) == 4) {
			gpio = kl_base + off;
		}

		/* Start secondary CPU. */
#if 1
		out8(gpio, 4);
		out8(gpio, 0);
#else
		openpic_write(OPENPIC_PROC_INIT, (1 << 1));
#endif
	} else {
#endif /* OPENPIC */
		/* Start secondary CPU and stop timebase. */
		out32(0xf2800000, (int)cpu_spinup_trampoline);
		cpu_send_ipi(1, IPI_NOMESG);
#ifdef OPENPIC
	}
#endif
	return 1;
}

void
md_presync_timebase(volatile struct cpu_hatch_data *h)
{
#ifdef OPENPIC
	if ((openpic_base != NULL) || have_u3_ht()) {
		uint64_t tb;

		/* Sync timebase. */
		tb = mftb();
		tb += 100000;  /* 3ms @ 33MHz */

		h->hatch_tbu = tb >> 32;
		h->hatch_tbl = tb & 0xffffffff;

		while (tb > mftb())
			;

		__asm volatile ("sync; isync");
		h->hatch_running = 0;

		delay(500000);
	} else
#endif /* OPENPIC */
	{
		/* sync timebase (XXX shouldn't be zero'ed) */
		__asm volatile ("mttbl %0; mttbu %0; mttbl %0" :: "r"(0));
	}
}

void
md_start_timebase(volatile struct cpu_hatch_data *h)
{
	int i;
#ifdef OPENPIC
	if (!((openpic_base != NULL) || have_u3_ht())) {
#endif
		/*
		 * wait for secondary spin up (1.5ms @ 604/200MHz)
		 * XXX we cannot use delay() here because timebase is not
		 * running.
		 */
		for (i = 0; i < 100000; i++)
			if (h->hatch_running)
				break;

		/* Start timebase. */
		out32(0xf2800000, 0x100);
		cpu_send_ipi(1, IPI_NOMESG);
#ifdef OPENPIC
	}
#endif
}

void
md_sync_timebase(volatile struct cpu_hatch_data *h)
{
#ifdef OPENPIC
	if ((openpic_base != NULL) || have_u3_ht()) {
		/* Sync timebase. */
		u_int tbu = h->hatch_tbu;
		u_int tbl = h->hatch_tbl;
		while (h->hatch_running == -1)
			;
		__asm volatile ("sync; isync");
		__asm volatile ("mttbl %0" :: "r"(0));
		__asm volatile ("mttbu %0" :: "r"(tbu));
		__asm volatile ("mttbl %0" :: "r"(tbl));
	}
#endif
}

void
md_setup_interrupts(void)
{
#ifdef OPENPIC
	if (openpic_base) {
		openpic_set_priority(cpu_number(), 0);
	} else if (have_u3_ht()) {
		__u3_ht_set_priority(cpu_number(), 0);		
	} else
#endif /* OPENPIC */
		out32(HH_INTR_SECONDARY, ~0);	/* Reset interrupt. */
}
#endif /* MULTIPROCESSOR */
