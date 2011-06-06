/*	$NetBSD: cpu.c,v 1.13.2.1 2011/06/06 09:06:21 jruoho Exp $	*/

/*-
 * Copyright (c) 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by NONAKA Kimihiro; by Jason R. Thorpe.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.13.2.1 2011/06/06 09:06:21 jruoho Exp $");

#include "opt_ppcparam.h"
#include "opt_multiprocessor.h"
#include "opt_interrupt.h"
#include "opt_altivec.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/ofw/openfirm.h>

#include <powerpc/openpic.h>
#include <powerpc/spr.h>
#include <powerpc/oea/spr.h>
#include <powerpc/oea/hid.h>
#include <powerpc/oea/bat.h>
#ifdef ALTIVEC
#include <powerpc/altivec.h>
#endif

#ifdef MULTIPROCESSOR
#include <arch/powerpc/pic/picvar.h>
#include <arch/powerpc/pic/ipivar.h>
#include <powerpc/rtas.h>
#endif

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/fpu.h>
#include <machine/pcb.h>
#include <machine/pio.h>
#include <machine/trap.h>

#include "pic_openpic.h"

#ifndef OPENPIC
#if NPIC_OPENPIC > 0
#define OPENPIC
#endif /* NOPENPIC > 0 */
#endif /* OPENPIC */

static int cpu_match(device_t, cfdata_t, void *);
static void cpu_attach(device_t, device_t, void *);
void cpu_OFgetspeed(device_t, struct cpu_info *);

CFATTACH_DECL_NEW(cpu, 0,
    cpu_match, cpu_attach, NULL, NULL);

extern struct cfdriver cpu_cd;
extern int machine_has_rtas;

int
cpu_match(device_t parent, cfdata_t cfdata, void *aux)
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
	if (reg[0] == 0)
		return 1;
	return 0;
}

void
cpu_OFgetspeed(device_t self, struct cpu_info *ci)
{
	int node;
	node = OF_finddevice("/cpus");
	if (node != -1) {
		for (node = OF_child(node); node; node = OF_peer(node)) {
			uint32_t cpunum;
			int l;

			l = OF_getprop(node, "reg", &cpunum, sizeof(cpunum));
			if (l == sizeof(uint32_t) && ci->ci_cpuid == cpunum) {
				uint32_t cf;

				l = OF_getprop(node, "clock-frequency",
				    &cf, sizeof(cf));
				if (l == sizeof(uint32_t))
					ci->ci_khz = cf / 1000;
				break;
			}
		}
	}
	if (ci->ci_khz)
		aprint_normal_dev(self, "%u.%02u MHz\n",
		    ci->ci_khz / 1000, (ci->ci_khz / 10) % 100);
}

static void
cpu_print_cache_config(uint32_t size, uint32_t line)
{
	char cbuf[7];

	format_bytes(cbuf, sizeof(cbuf), size);
	aprint_normal("%s %dB/line", cbuf, line);
}

static void
cpu_OFprintcacheinfo(int node)
{
	int l;
	uint32_t dcache=0, icache=0, dline=0, iline=0;

	OF_getprop(node, "i-cache-size", &icache, sizeof(icache));
	OF_getprop(node, "d-cache-size", &dcache, sizeof(dcache));
	OF_getprop(node, "i-cache-line-size", &iline, sizeof(iline));
	OF_getprop(node, "d-cache-line-size", &dline, sizeof(dline));
	if (OF_getprop(node, "cache-unified", &l, sizeof(l)) != -1) {
		aprint_normal("cache ");
		cpu_print_cache_config(icache, iline);
	} else {
		aprint_normal("I-cache ");
		cpu_print_cache_config(icache, iline);
		aprint_normal(", D-cache ");
		cpu_print_cache_config(dcache, dline);
	}
	aprint_normal("\n");
}

static void
cpu_OFgetcache(device_t self, struct cpu_info *ci)
{
	int node, cpu=-1;
	char name[32];

	node = OF_finddevice("/cpus");
	if (node == -1)
		return;

	for (node = OF_child(node); node; node = OF_peer(node)) {
		uint32_t cpunum;
		int l;

		l = OF_getprop(node, "reg", &cpunum, sizeof(cpunum));
		if (l == sizeof(uint32_t) && ci->ci_cpuid == cpunum) {
			cpu = node;
			break;
		}
	}
	if (cpu == -1)
		return;
	/* now we have cpu */
	aprint_normal_dev(self, "L1 ");
	cpu_OFprintcacheinfo(cpu);
	for (node = OF_child(cpu); node; node = OF_peer(node)) {
		if (OF_getprop(node, "name", name, sizeof(name)) != -1) {
			if (strcmp("l2-cache", name) == 0) {
				aprint_normal_dev(self, "L2 ");
				cpu_OFprintcacheinfo(node);
			} else if (strcmp("l3-cache", name) == 0) {
				aprint_normal_dev(self, "L3 ");
				cpu_OFprintcacheinfo(node);
			}
		}
	}
}


void
cpu_attach(device_t parent, device_t self, void *aux)
{
	struct cpu_info *ci;
	struct confargs *ca = aux;
	int id = ca->ca_reg[0];

	ci = cpu_attach_common(self, id);
	if (ci == NULL)
		return;

	if (id > 0)
#ifdef MULTIPROCESSOR
		cpu_spinup(self, ci);
#endif

	if (ci->ci_khz == 0)
		cpu_OFgetspeed(self, ci);

	cpu_OFgetcache(self, ci);
	return;
}

#ifdef MULTIPROCESSOR

extern volatile u_int cpu_spinstart_cpunum;
extern volatile u_int cpu_spinstart_ack;

int
md_setup_trampoline(volatile struct cpu_hatch_data *h, struct cpu_info *ci)
{
	int i;
	u_int msr;

	msr = mfmsr();
	h->running = -1;
	cpu_spinstart_cpunum = ci->ci_cpuid;
	__asm volatile("dcbf 0,%0"::"r"(&cpu_spinstart_cpunum):"memory");

	for (i=0; i < 100000000; i++)
		if (cpu_spinstart_ack == 0)
			break;
	return 1;
}

void
md_presync_timebase(volatile struct cpu_hatch_data *h)
{
	uint64_t tb;
	int junk;

	if (machine_has_rtas && rtas_has_func(RTAS_FUNC_FREEZE_TIME_BASE)) {
		rtas_call(RTAS_FUNC_FREEZE_TIME_BASE, 0, 1, &junk);
		/* Sync timebase. */
		tb = mftb();

		h->tbu = tb >> 32;
		h->tbl = tb & 0xffffffff;

		h->running = 0;
	}
	/* otherwise, the machine has no rtas, or if it does, things
	 * are pre-syncd, per PAPR v2.2.  I don't have anything without
	 * rtas, so if such a machine exists, someone will have to write
	 * code for it
	 */
}

void
md_start_timebase(volatile struct cpu_hatch_data *h)
{
	int i, junk;
	/*
	 * wait for secondary spin up (1.5ms @ 604/200MHz)
	 * XXX we cannot use delay() here because timebase is not
	 * running.
	 */
	for (i = 0; i < 100000; i++)
		if (h->running)
			break;

	/* Start timebase. */
	if (machine_has_rtas && rtas_has_func(RTAS_FUNC_THAW_TIME_BASE))
		rtas_call(RTAS_FUNC_THAW_TIME_BASE, 0, 1, &junk);
}

/*
 * We wait for h->running to become 0, and then we know that the time is
 * frozen and h->tb is correct.
 */

void
md_sync_timebase(volatile struct cpu_hatch_data *h)
{
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

void
md_setup_interrupts(void)
{
/* do nothing, this is handled in ofwpci */
}
#endif /* MULTIPROCESSOR */
