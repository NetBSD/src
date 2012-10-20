/*	$NetBSD: cpu.c,v 1.12 2012/10/20 14:53:37 kiyohara Exp $	*/

/*-
 * Copyright (c) 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by NONAKA Kimihiro.
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
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.12 2012/10/20 14:53:37 kiyohara Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <powerpc/oea/hid.h>
#include <powerpc/oea/spr.h>
#include <powerpc/spr.h>

#include <uvm/uvm.h>


int cpumatch(device_t, cfdata_t, void *);
void cpuattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cpu, 0, cpumatch, cpuattach, NULL, NULL);

extern struct cfdriver cpu_cd;

int
cpumatch(device_t parent, cfdata_t cfdata, void *aux)
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, cpu_cd.cd_name) != 0)
		return 0;
	return 1;
}

void
cpuattach(device_t parent, device_t self, void *aux)
{
	struct confargs *ca = aux;
	struct cpu_info *ci;
	int id = ca->ca_node;

	ci = cpu_attach_common(self, id);
	if (ci == NULL)
		return;

#ifdef MULTIPROCESSOR
	if (id > 0)
		cpu_spinup(self, ci);
#endif
}


#ifdef MULTIPROCESSOR

extern volatile u_int cpu_spinstart_ack, cpu_spinstart_cpunum;

int
md_setup_trampoline(volatile struct cpu_hatch_data *h, struct cpu_info *ci)
{
	int blocks, i;
	extern void cache_flush_invalidate_all(int blocks);

	/*
	 * Our CPU(603e) supports control protocol for cache only MEI
	 * (Modified/Exclusive/Invalid).  Flush dirty caches myself.
	 */

	h->hatch_running = -1;
	cpu_spinstart_ack = -1;

	/* Flush-invalidate all blocks */
	blocks = curcpu()->ci_ci.dcache_size / curcpu()->ci_ci.dcache_line_size;
	cache_flush_invalidate_all(blocks);

	cpu_spinstart_cpunum = ci->ci_cpuid;
	__asm volatile("dcbf 0,%0"::"r"(&cpu_spinstart_cpunum):"memory");

	for (i = 0; i < 100000000; i++) {
		if (cpu_spinstart_ack != -1)
			break;
		__asm volatile("dcbi 0,%0"::"r"(&cpu_spinstart_ack):"memory");
		__asm volatile("sync; isync");
	}
	return 1;
}

void
md_presync_timebase(volatile struct cpu_hatch_data *h)
{
	uint64_t tb;

	/* Sync timebase. */
	tb = mftb();
	tb += 1000000;	/* 30ms @ 33MHz */

	h->hatch_tbu = tb >> 32;
	h->hatch_tbl = tb & 0xffffffff;

	while (tb > mftb())
		;

	h->hatch_running = 0;

	delay(500000);
}

void
md_start_timebase(volatile struct cpu_hatch_data *h)
{
	/* Nothing */
}

void
md_sync_timebase(volatile struct cpu_hatch_data *h)
{
	u_int tbu, tbl;

	do {
		__asm volatile("dcbi 0,%0"::"r"(&h->hatch_running):"memory");
		__asm volatile("sync; isync");

		__asm volatile("dcbst 0,%0"::"r"(&h->hatch_running):"memory");
		__asm volatile("sync; isync");
	} while (h->hatch_running == -1);

	__asm volatile("dcbi 0,%0"::"r"(&h->hatch_tbu):"memory");
	__asm volatile("dcbi 0,%0"::"r"(&h->hatch_tbl):"memory");
	__asm volatile("sync; isync");
	__asm volatile("dcbst 0,%0"::"r"(&h->hatch_tbu):"memory");
	__asm volatile("dcbst 0,%0"::"r"(&h->hatch_tbl):"memory");
	__asm volatile("sync; isync");

	/* Sync timebase. */
	tbu = h->hatch_tbu;
	tbl = h->hatch_tbl;
	__asm volatile ("sync; isync");
	__asm volatile ("mttbl %0" :: "r"(0));
	__asm volatile ("mttbu %0" :: "r"(tbu));
	__asm volatile ("mttbl %0" :: "r"(tbl));
}

void
md_setup_interrupts(void)
{

	CLEAR_BEBOX_REG(CPU1_INT_MASK, BEBOX_INTR_MASK);

	/* XXXXXX: We can handle interrupts on CPU0 only now. */
}

#endif /* MULTIPROCESSOR */
