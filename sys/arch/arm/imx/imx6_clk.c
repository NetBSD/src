/*	$NetBSD: imx6_clk.c,v 1.1.10.2 2020/04/13 08:03:35 martin Exp $	*/

/*
 * Copyright (c) 2019  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx6_clk.c,v 1.1.10.2 2020/04/13 08:03:35 martin Exp $");

#include "locators.h"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/sysctl.h>
#include <sys/cpufreq.h>
#include <sys/malloc.h>
#include <sys/param.h>

#include <machine/cpu.h>

#include <arm/imx/imx6_ccmvar.h>
#include <arm/imx/imx6_ccmreg.h>

#include <arm/imx/imx6var.h>
#include <arm/imx/imx6_reg.h>

static int imxccm_match(device_t, cfdata_t, void *);
static void imxccm_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(imxccm, sizeof(struct imxccm_softc),
    imxccm_match, imxccm_attach, NULL, NULL);

static int
imxccm_match(device_t parent, cfdata_t cfdata, void *aux)
{
	struct axi_attach_args *aa = aux;

	if (aa->aa_addr == IMX6_AIPS1_BASE + AIPS1_CCM_BASE)
		return 1;

	return 0;
}

static void
imxccm_attach(device_t parent, device_t self, void *aux)
{
	struct imxccm_softc * const sc = device_private(self);
	struct axi_attach_args *aa = aux;
	bus_space_tag_t iot = aa->aa_iot;

	sc->sc_dev = self;
	sc->sc_iot = iot;

	if (bus_space_map(iot, aa->aa_addr, AIPS1_CCM_SIZE, 0, &sc->sc_ioh)) {
		aprint_error(": can't map CCM registers\n");
		return;
	}
	if (bus_space_map(iot, aa->aa_addr + CCM_ANALOG_BASE,
	    CCM_ANALOG_SIZE, 0, &sc->sc_ioh_analog)) {
		aprint_error(": can't map CCM_ANALOG registers\n");
		bus_space_unmap(iot, sc->sc_ioh, AIPS1_CCM_SIZE);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Clock Control Module\n");

	imxccm_attach_common(self);
}

