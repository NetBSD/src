/* $NetBSD: mainbus.c,v 1.1 2026/01/09 22:54:29 jmcneill Exp $ */

/*
 * Copyright (c) 2002, 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at Sandburst Corp.
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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.1 2026/01/09 22:54:29 jmcneill Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <sys/bus.h>
#include <machine/wii.h>
#include <machine/wiiu.h>
#include <machine/pio.h>
#include <arch/evbppc/nintendo/dev/mainbus.h>

#include "locators.h"

extern struct powerpc_bus_space wii_mem_tag;
extern struct powerpc_bus_dma_tag wii_bus_dma_tag;

int	mainbus_match(device_t, cfdata_t, void *);
void	mainbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mainbus, 0,
    mainbus_match, mainbus_attach, NULL, NULL);

int
mainbus_match(device_t parent, cfdata_t match, void *aux)
{
	return 1;
}

static int
mainbus_print(void *aux, const char *pnp)
{
	struct mainbus_attach_args *mba = aux;

	if (pnp) {
		aprint_normal("%s at %s", mba->maa_name, pnp);
	}
	if (mba->maa_addr != MAINBUSCF_ADDR_DEFAULT) {
		aprint_normal(" addr 0x%08lx", mba->maa_addr);
	}
	if (mba->maa_irq != MAINBUSCF_IRQ_DEFAULT) {
		aprint_normal(" irq %d", mba->maa_irq);
	}
	return UNCONF;
}

void
mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args maa;
	int maxcpu, n;

	aprint_normal(": Nintendo Wii%s%s\n",
	    wiiu_plat ? " U" : "",
	    (!wiiu_plat || wiiu_native) ? "" : " (vWii)");

	maa.maa_bst = &wii_mem_tag;
	maa.maa_dmat = &wii_bus_dma_tag;

	maxcpu = wiiu_native ? 3 : 1;

	for (n = 0; n < uimin(maxcpu, CPU_MAXNUM); n++) {
		maa.maa_name = "cpu";
		maa.maa_addr = MAINBUSCF_ADDR_DEFAULT;
		maa.maa_irq = MAINBUSCF_IRQ_DEFAULT;
		config_found(self, &maa, mainbus_print, CFARGS_NONE);
	}

	maa.maa_name = "exi";
	maa.maa_addr = EXI_BASE;
	maa.maa_irq = PI_IRQ_EXI;
	config_found(self, &maa, mainbus_print, CFARGS_NONE);

	maa.maa_name = "genfb";
	maa.maa_addr = VI_BASE;
	maa.maa_irq = PI_IRQ_VI;
	config_found(self, &maa, mainbus_print, CFARGS_NONE);

	maa.maa_name = "ahb";
	maa.maa_addr = MAINBUSCF_ADDR_DEFAULT;
	maa.maa_irq = PI_IRQ_HOLLYWOOD;
	config_found(self, &maa, mainbus_print, CFARGS_NONE);

	maa.maa_name = "bwai";
	maa.maa_addr = AI_BASE;
	maa.maa_irq = PI_IRQ_AI;
	config_found(self, &maa, mainbus_print, CFARGS_NONE);

	maa.maa_name = "bwdsp";
	maa.maa_addr = wiiu_native ? WIIU_DSP_BASE : DSP_BASE;
	maa.maa_irq = MAINBUSCF_IRQ_DEFAULT;
	config_found(self, &maa, mainbus_print, CFARGS_NONE);

	if (!wiiu_native) {
		maa.maa_name = "si";
		maa.maa_addr = SI_BASE;
		maa.maa_irq = PI_IRQ_SI;
		config_found(self, &maa, mainbus_print, CFARGS_NONE);
	}
}
