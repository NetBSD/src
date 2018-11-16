/* $NetBSD: gtmr_acpi.c,v 1.2 2018/11/16 23:24:28 jmcneill Exp $ */

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jared McNeill <jmcneill@invisible.ca>.
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
__KERNEL_RCSID(0, "$NetBSD: gtmr_acpi.c,v 1.2 2018/11/16 23:24:28 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <arm/cortex/gtmr_intr.h>
#include <arm/cortex/mpcore_var.h>
#include <arm/cortex/gtmr_var.h>

#include <dev/fdt/fdtvar.h>
#include <arm/fdt/arm_fdtvar.h>

extern struct bus_space arm_generic_bs_tag;

static int	gtmr_acpi_match(device_t, cfdata_t, void *);
static void	gtmr_acpi_attach(device_t, device_t, void *);

static void	gtmr_acpi_cpu_hatch(void *, struct cpu_info *);

CFATTACH_DECL_NEW(gtmr_acpi, 0, gtmr_acpi_match, gtmr_acpi_attach, NULL, NULL);

static int
gtmr_acpi_match(device_t parent, cfdata_t cf, void *aux)
{
	ACPI_TABLE_HEADER *hdrp = aux;

	return memcmp(hdrp->Signature, ACPI_SIG_GTDT, ACPI_NAME_SIZE) == 0;
}

static void
gtmr_acpi_attach(device_t parent, device_t self, void *aux)
{
	ACPI_TABLE_GTDT *gtdt = aux;
	struct mpcore_attach_args mpcaa;
	void *ih;

	const int irq = gtdt->VirtualTimerInterrupt;
	const int ipl = IPL_CLOCK;
	const int type = (gtdt->VirtualTimerFlags & ACPI_GTDT_INTERRUPT_MODE) ?
	    IST_EDGE : IST_LEVEL;

	aprint_naive("\n");
	aprint_normal(": irq %d\n", irq);

	ih = intr_establish_xname(irq, ipl, type | IST_MPSAFE, gtmr_intr, NULL, device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't install interrupt handler\n");
		return;
	}

	memset(&mpcaa, 0, sizeof(mpcaa));
	mpcaa.mpcaa_name = "armgtmr";
	mpcaa.mpcaa_irq = -1;
	config_found(self, &mpcaa, NULL);

	arm_fdt_cpu_hatch_register(self, gtmr_acpi_cpu_hatch);
	arm_fdt_timer_register(gtmr_cpu_initclocks);
}

static void
gtmr_acpi_cpu_hatch(void *priv, struct cpu_info *ci)
{
	gtmr_init_cpu_clock(ci);
}
