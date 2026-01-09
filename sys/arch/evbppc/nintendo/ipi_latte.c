/* $NetBSD: ipi_latte.c,v 1.1 2026/01/09 22:54:28 jmcneill Exp $ */

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

/*
 * Latte inter-processor interrupt support.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ipi_latte.c,v 1.1 2026/01/09 22:54:28 jmcneill Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/atomic.h>
#include <sys/cpu.h>
#include <powerpc/pic/ipivar.h>
#include <powerpc/include/spr.h>
#include <powerpc/include/oea/spr.h>
#include <machine/pio.h>
#include <machine/wiiu.h>
#include "ipi_latte.h"

#ifdef MULTIPROCESSOR

extern struct ipi_ops ipiops;

static void
ipi_latte_send_ipi(cpuid_t target, uint32_t mesg)
{
	struct cpu_info *ci = curcpu();
	struct cpu_info *dst_ci;
	uint32_t scr_mask = 0;
	int n;

	if (target == IPI_DST_ALL || target == IPI_DST_NOTME) {
		for (n = 0; n < ncpu; n++) {
			dst_ci = cpu_lookup(n);
			if (target == IPI_DST_ALL || dst_ci != ci) {
				atomic_or_32(&dst_ci->ci_pending_ipis, mesg);
				scr_mask |= SPR_SCR_IPI_PEND(n);
			}
		}
	} else {
		dst_ci = cpu_lookup(target);
		atomic_or_32(&dst_ci->ci_pending_ipis, mesg);
		scr_mask |= SPR_SCR_IPI_PEND(target);
	}

	mtspr(SPR_SCR, mfspr(SPR_SCR) | scr_mask);
}

static void
ipi_latte_establish_ipi(int type, int level, void *ih_args)
{
	char name[INTRDEVNAMEBUF];
	int n;

	for (n = 0; n < uimin(3, CPU_MAXNUM); n++) {
		snprintf(name, sizeof(name), "IPI cpu%u", n);
		intr_establish_xname(WIIU_PI_IRQ_MB_CPU(n), type, level,
		    ipi_intr, ih_args, name);
	}
}

void
ipi_latte_init(void)
{
	ipiops.ppc_send_ipi = ipi_latte_send_ipi;
	ipiops.ppc_establish_ipi = ipi_latte_establish_ipi;
}

#endif /* !MULTIPROCESSOR */
