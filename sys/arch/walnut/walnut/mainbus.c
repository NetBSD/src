/*	$NetBSD: mainbus.c,v 1.4.2.2 2002/03/16 16:00:21 jdolecek Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "locators.h"
#include "pckbc.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#define _GALAXY_BUS_DMA_PRIVATE
#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/walnut.h>

#include <powerpc/ibm4xx/ibm405gp.h>

/*
 * The devices built in to the 405GP cpu.
 */
const struct ppc405gp_dev {
	const char *name;
	bus_addr_t addr;
	int irq;
} ppc405gp_devs [] = {
	{ "com",	UART0_BASE,	 0 },
	{ "com",	UART1_BASE,	 1 },
	{ "dsrtc",	NVRAM_BASE,	-1 },
	{ "emac",	EMAC0_BASE,	15 }, /* XXX: really irq 9..15 */
	{ "gpio",	GPIO0_BASE,	-1 },
	{ "iic",	IIC0_BASE,	 2 },
	{ "wdog",	-1,        	-1 },
	{ "pckbc",	KEY_MOUSE_BASE,	10 }, /* XXX: really irq x..x+1 */
	{ "pchb",	PCIC0_BASE,	-1 },
	{ NULL }
};

static int	mainbus_match(struct device *, struct cfdata *, void *);
static void	mainbus_attach(struct device *, struct device *, void *);
static int	mainbus_submatch(struct device *, struct cfdata *, void *);
static int	mainbus_print(void *, const char *);

struct cfattach mainbus_ca = {
	sizeof(struct device), mainbus_match, mainbus_attach
};


/*
 * Probe for the mainbus; always succeeds.
 */
static int
mainbus_match(struct device *parent, struct cfdata *match, void *aux)
{

	return 1;
}

static int
mainbus_submatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct mainbus_attach_args *maa = aux;

	if (cf->cf_loc[MAINBUSCF_ADDR] != MAINBUSCF_ADDR_DEFAULT &&
	    cf->cf_loc[MAINBUSCF_ADDR] != maa->mb_addr)
		return (0);

	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

/*
 * Attach the mainbus.
 */
static void
mainbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct mainbus_attach_args mba;
	int i;
#if NPCKBC > 0
	bus_space_handle_t ioh_fpga;
	bus_space_tag_t iot_fpga = galaxy_make_bus_space_tag(0, 0);
	uint8_t fpga_reg;
#endif

	printf("\n");

	/* Attach the CPU first */
	mba.mb_name = "cpu";
	mba.mb_addr = MAINBUSCF_ADDR_DEFAULT;
	mba.mb_irq = MAINBUSCF_IRQ_DEFAULT;
	mba.mb_bt = galaxy_make_bus_space_tag(0, 0);
	config_found(self, &mba, mainbus_print);
	
	for (i = 0; ppc405gp_devs[i].name != NULL; i++) {
		mba.mb_name = ppc405gp_devs[i].name;
		mba.mb_addr = ppc405gp_devs[i].addr;
		mba.mb_irq = ppc405gp_devs[i].irq;
		mba.mb_bt = galaxy_make_bus_space_tag(0, 0);
		mba.mb_dmat = &galaxy_default_bus_dma_tag;

		(void) config_found_sm(self, &mba, mainbus_print,
		    mainbus_submatch);
	}

#if NPCKBC > 0
	/* Configure FPGA */
	if (bus_space_map(iot_fpga, FPGA_BASE, FPGA_SIZE, 0, &ioh_fpga)) {
		printf("mainbus_attach: can't map FPGA\n");
		/* XXX - disable keyboard probe? */
	} else {
		/* Use separate interrupts for keyboard and mouse */
		fpga_reg = bus_space_read_1(iot_fpga, ioh_fpga, FPGA_BRDC);
		fpga_reg |= FPGA_BRDC_INT;
		bus_space_write_1(iot_fpga, ioh_fpga, FPGA_BRDC, fpga_reg);

		/* Set interrupts to active high */
		fpga_reg = bus_space_read_1(iot_fpga, ioh_fpga, FPGA_INT_POL);
		fpga_reg |= (FPGA_IRQ_KYBD | FPGA_IRQ_MOUSE);
		bus_space_write_1(iot_fpga, ioh_fpga, FPGA_INT_POL, fpga_reg);

		/* Set interrupts to level triggered */
		fpga_reg = bus_space_read_1(iot_fpga, ioh_fpga, FPGA_INT_TRIG);
		fpga_reg |= (FPGA_IRQ_KYBD | FPGA_IRQ_MOUSE);
		bus_space_write_1(iot_fpga, ioh_fpga, FPGA_INT_TRIG, fpga_reg);

		/* Enable interrupts */
		fpga_reg = bus_space_read_1(iot_fpga, ioh_fpga, FPGA_INT_ENABLE);
		fpga_reg |= (FPGA_IRQ_KYBD | FPGA_IRQ_MOUSE);
		bus_space_write_1(iot_fpga, ioh_fpga, FPGA_INT_ENABLE, fpga_reg);

		bus_space_unmap(&iot_fpga, ioh_fpga, 2);
	}
#endif

}

static int
mainbus_print(void *aux, const char *pnp)
{
	struct mainbus_attach_args *mba = aux;

	if (pnp)
		printf("%s at %s", mba->mb_name, pnp);

	if (mba->mb_addr != MAINBUSCF_ADDR_DEFAULT)
		printf(" addr 0x%08lx", mba->mb_addr);
	if (mba->mb_irq != MAINBUSCF_IRQ_DEFAULT)
		printf(" irq %d", mba->mb_irq);

	return (UNCONF);
}

