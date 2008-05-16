/*	$NetBSD: cpu.c,v 1.14.20.1 2008/05/16 02:23:03 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.14.20.1 2008/05/16 02:23:03 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/platform.h>

#ifdef MULTIPROCESSOR
#include <arch/powerpc/pic/picvar.h>
#include <arch/powerpc/pic/ipivar.h>
#include <powerpc/openpic.h>
#include <powerpc/trap.h>

extern void openpic_set_priority(int, int);
#endif

int cpumatch(struct device *, struct cfdata *, void *);
void cpuattach(struct device *, struct device *, void *);

CFATTACH_DECL(cpu, sizeof(struct device),
    cpumatch, cpuattach, NULL, NULL);

extern struct cfdriver cpu_cd;

int
cpumatch(struct device *parent, struct cfdata *cfdata, void *aux)
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, cpu_cd.cd_name) != 0)
		return (0);
#if 0
	if (cpu_info[0].ci_dev != NULL)
		return (0);
#endif
	return (1);
}

void
cpuattach(struct device *parent, struct device *self, void *aux)
{
	struct cpu_info *ci;
	struct confargs *ca = aux;
	int id = ca->ca_node;

	ci = cpu_attach_common(self, id);
	if (ci == NULL)
		return;

#ifdef MULTIPROCESSOR
	if (id > 0) {
		cpu_spinup(self, ci);
		return;
	}
#endif

	cpu_setup_prep_generic(self);
}

#ifdef MULTIPROCESSOR

int
md_setup_trampoline(volatile struct cpu_hatch_data *h, struct cpu_info *ci)
{
	if (!openpic_base)
		return -1;

	/* construct an absolute branch instruction */
	/* ba cpu_spinup_trampoline */
	*(u_int *)EXC_RST = 0x48000002 | (u_int)cpu_spinup_trampoline;
	__syncicache((void *)EXC_RST, 0x100);
	h->running = -1;

	/* Start secondary CPU. */
	openpic_write(OPENPIC_PROC_INIT, (1 << 1));
	return 1;
}

void
md_presync_timebase(volatile struct cpu_hatch_data *h)
{
	uint64_t tb;

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
}

void
md_start_timebase(volatile struct cpu_hatch_data *h)
{
	/* do nada */
}

void
md_sync_timebase(volatile struct cpu_hatch_data *h)
{
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
	if (!openpic_base)
		return;
	/* clear the reset on all CPU's */
	openpic_write(OPENPIC_PROC_INIT, 0);
	openpic_set_priority(cpu_number(), 0);
}
#endif /* MULTIPROCESSOR */
