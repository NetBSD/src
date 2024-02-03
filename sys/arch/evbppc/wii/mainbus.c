/* $NetBSD: mainbus.c,v 1.3.2.2 2024/02/03 11:47:07 martin Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.3.2.2 2024/02/03 11:47:07 martin Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <sys/bus.h>
#include <machine/wii.h>
#include <machine/pio.h>
#include <arch/evbppc/wii/dev/mainbus.h>

#include "locators.h"
#include "mainbus.h"

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

	aprint_normal(": Nintendo Wii\n");
	aprint_debug_dev(self, "mem1 0x%x, mem2 0x%x\n",
	    in32(GLOBAL_MEM1_SIZE), in32(GLOBAL_MEM2_SIZE));
	aprint_debug_dev(self, "cpu %u, bus %u, vidmode %u\n",
	    in32(GLOBAL_CPU_SPEED), in32(GLOBAL_BUS_SPEED),
	    in32(GLOBAL_CUR_VID_MODE));
	aprint_debug_dev(self, "ios version 0x%x\n", in32(GLOBAL_IOS_VERSION));

	maa.maa_bst = &wii_mem_tag;
	maa.maa_dmat = &wii_bus_dma_tag;

	maa.maa_name = "cpu";
	maa.maa_addr = MAINBUSCF_ADDR_DEFAULT;
	maa.maa_irq = MAINBUSCF_IRQ_DEFAULT;
	config_found(self, &maa, mainbus_print, CFARGS_NONE);

	maa.maa_name = "genfb";
	maa.maa_addr = VI_BASE;
	maa.maa_irq = MAINBUSCF_IRQ_DEFAULT;
	config_found(self, &maa, mainbus_print, CFARGS_NONE);

	maa.maa_name = "exi";
	maa.maa_addr = EXI_BASE;
	maa.maa_irq = PI_IRQ_EXI;
	config_found(self, &maa, mainbus_print, CFARGS_NONE);

	maa.maa_name = "hollywood";
	maa.maa_addr = MAINBUSCF_ADDR_DEFAULT;
	maa.maa_irq = PI_IRQ_HOLLYWOOD;
	config_found(self, &maa, mainbus_print, CFARGS_NONE);

	maa.maa_name = "bwai";
	maa.maa_addr = AI_BASE;
	maa.maa_irq = PI_IRQ_AI;
	config_found(self, &maa, mainbus_print, CFARGS_NONE);

	maa.maa_name = "bwdsp";
	maa.maa_addr = DSP_BASE;
	maa.maa_irq = PI_IRQ_DSP;
	config_found(self, &maa, mainbus_print, CFARGS_NONE);
}

static int	cpu_match(device_t, cfdata_t, void *);
static void	cpu_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cpu, 0,
    cpu_match, cpu_attach, NULL, NULL);

extern struct cfdriver cpu_cd;

int
cpu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *maa = aux;

	if (strcmp(maa->maa_name, cpu_cd.cd_name) != 0) {
		return 0;
	}

	if (cpu_info[0].ci_dev != NULL) {
		return 0;
	}

	return 1;
}

void
cpu_attach(device_t parent, device_t self, void *aux)
{
	cpu_attach_common(self, 0);
}
