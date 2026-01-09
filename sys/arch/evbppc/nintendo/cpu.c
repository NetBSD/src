/* $NetBSD: cpu.c,v 1.1 2026/01/09 22:54:28 jmcneill Exp $ */

/*-
 * Copyright (c) 2025 Jared McNeill <jmcneill@invisible.ca>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.1 2026/01/09 22:54:28 jmcneill Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/kprintf.h>
#include <machine/wii.h>
#include <machine/wiiu.h>
#include <powerpc/include/spr.h>
#include <powerpc/include/oea/spr.h>
#include <powerpc/include/psl.h>
#include <arch/evbppc/nintendo/dev/mainbus.h>
#include <arch/evbppc/nintendo/pic_pi.h>

static int	cpu_match(device_t, cfdata_t, void *);
static void	cpu_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cpu, 0, cpu_match, cpu_attach, NULL, NULL);

extern struct cfdriver cpu_cd;

#ifdef MULTIPROCESSOR
static uint32_t cpu_start_code[] = {
	0x3c600000,	// lis     r3, 0	/* upper 32-bits of entry pt */
	0x60630000,	// ori     r3, r3, 0	/* lower 32-bits of entry pt */
	0x7c7a03a6,	// mtsrr0  r3

	0x38600000,	// li      r3, 0
	0x7c7b03a6,	// mtsrr1  r3

	0x3c600011,	// lis     r3, 0x0011
	0x60630024,	// ori     r3, r3, 0x0024
	0x7c70fba6,	// mtspr   1008, r3	/* HID0 */

	0x3c60b1b0,	// lis     r3, 0xb1b0
	0x7c73fba6,	// mtspr   1011, r3	/* HID4 */
	0x7c0004ac,	// sync

	0x3c60e7fd,	// lis     r3, 0xe7fd
	0x6063c000,	// ori     r3, r3, 0xc000
	0x7c70eba6,	// mtspr   944, r3	/* HID5 */
	0x7c0004ac,	// sync

	0x4c000064,	// rfi
};
#endif

int
cpu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *maa = aux;

	if (strcmp(maa->maa_name, cpu_cd.cd_name) != 0) {
		return 0;
	}

	return 1;
}

void
cpu_attach(device_t parent, device_t self, void *aux)
{
	u_int cpu_num = device_unit(self);
	struct cpu_info *ci;

	ci = cpu_attach_common(self, cpu_num);
	if (ci == NULL) {
		return;
	}

#ifdef MULTIPROCESSOR
	if (cpu_num > 0) {
		cpu_spinup(self, ci);
	}

	if (wiiu_native) {
		/*
		 * All cores are the same speed, but cores 1 has a bigger
		 * cache, so let the scheduler know that the other cores
		 * are slower.
		 */
		cpu_topology_setspeed(ci, cpu_index(ci) != 1);
	}
#endif

	ci->ci_data.cpu_cc_freq = cpu_timebase;
}

#ifdef MULTIPROCESSOR

int
md_setup_trampoline(volatile struct cpu_hatch_data *h, struct cpu_info *ci)
{
	extern volatile u_int cpu_spinstart_ack;
	u_int cpu_num = cpu_index(ci);
	u_int i;

	if (!wiiu_native) {
		return -1;
	}

	/* construct an absolute branch instruction */
	/* ba cpu_spinup_trampoline */
	cpu_start_code[0] |= ((u_int)cpu_spinup_trampoline >> 16) & 0x7fff;
	cpu_start_code[1] |= (u_int)cpu_spinup_trampoline & 0xffff;
	memcpy((void *)WIIU_BOOT_VECTOR, cpu_start_code, sizeof(cpu_start_code));
	__syncicache((void *)WIIU_BOOT_VECTOR, sizeof(cpu_start_code));
	h->hatch_running = -1;

	/* Start secondary CPU. */
	mtspr(SPR_SCR, mfspr(SPR_SCR) | SPR_SCR_WAKE(cpu_num));

	for (i = 0; i < 100000000; i++) {
		if (cpu_spinstart_ack == 0) {
			break;
		}
	}

	/*
	 * XXX FIXME
	 * Without this printf, CPU startup takes a lot longer. I haven't
	 * worked out why yet, so leave it for now.
	 */
	printf_flags(TOCONS|NOTSTAMP, " \x08");

	return 1;
}

void
md_presync_timebase(volatile struct cpu_hatch_data *h)
{
	uint64_t tb;

	/* Sync timebase. */
	tb = mftb();
	tb += 100000;  /* 3ms @ 33MHz */

	h->hatch_tbu = tb >> 32;
	h->hatch_tbl = tb & 0xffffffff;

	while (tb > mftb())
		;

	__asm volatile ("sync; isync" ::: "memory");
	h->hatch_running = 0;

	delay(500000);
}

void
md_start_timebase(volatile struct cpu_hatch_data *h)
{
	u_int i;

	for (i = 0; i < 1000000; i++) {
		if (h->hatch_running) {
			break;
		}
	}
}

void
md_sync_timebase(volatile struct cpu_hatch_data *h)
{
	/* Sync timebase. */
	u_int tbu = h->hatch_tbu;
	u_int tbl = h->hatch_tbl;

	while (h->hatch_running == -1)
		;

	__asm volatile ("sync; isync" ::: "memory");
	__asm volatile ("mttbl %0" :: "r"(0));
	__asm volatile ("mttbu %0" :: "r"(tbu));
	__asm volatile ("mttbl %0" :: "r"(tbl));
}

void
md_setup_interrupts(void)
{
	mtmsr(mfmsr() & ~PSL_IP);
}

#endif /* !MULTIPROCESSOR */
