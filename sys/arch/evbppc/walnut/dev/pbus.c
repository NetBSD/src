/*	$NetBSD: pbus.c,v 1.4 2003/07/16 08:06:10 simonb Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pbus.c,v 1.4 2003/07/16 08:06:10 simonb Exp $");

#include "locators.h"
#include "pckbc.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/walnut.h>

#include <evbppc/walnut/dev/pbusvar.h>

#include <powerpc/ibm4xx/ibm405gp.h>
#include <powerpc/ibm4xx/dev/plbvar.h>

/*
 * The external devices on the Walnut 405GP evaluation board.
 */
const struct pbus_dev {
	const char *name;
	bus_addr_t addr;
	int irq;
} pbus_devs [] = {
	{ "dsrtc",	NVRAM_BASE,	-1 },
	{ "pckbc",	KEY_MOUSE_BASE,	25 }, /* XXX: really irq x..x+1 */
	{ NULL }
};

static int	pbus_match(struct device *, struct cfdata *, void *);
static void	pbus_attach(struct device *, struct device *, void *);
static int	pbus_submatch(struct device *, struct cfdata *, void *);
static int	pbus_print(void *, const char *);

CFATTACH_DECL(pbus, sizeof(struct device),
    pbus_match, pbus_attach, NULL, NULL);

/*
 * Probe for the peripheral bus.
 */
static int
pbus_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct pbus_attach_args *pba = aux;

	/* match only pbus devices */
	if (strcmp(pba->pb_name, cf->cf_name) != 0)
		return (0);

	return (1);
}

static int
pbus_submatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct pbus_attach_args *pba = aux;

	if (cf->cf_loc[PBUSCF_ADDR] != PBUSCF_ADDR_DEFAULT &&
	    cf->cf_loc[PBUSCF_ADDR] != pba->pb_addr)
		return (0);

	return (config_match(parent, cf, aux));
}

/*
 * Attach the peripheral bus.
 */
static void
pbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct plb_attach_args *paa = aux;
	struct pbus_attach_args pba;
	int i;
#if NPCKBC > 0
	bus_space_handle_t ioh_fpga;
	bus_space_tag_t iot_fpga = paa->plb_bt;
	uint8_t fpga_reg;
#endif

	printf("\n");

	for (i = 0; pbus_devs[i].name != NULL; i++) {
		pba.pb_name = pbus_devs[i].name;
		pba.pb_addr = pbus_devs[i].addr;
		pba.pb_irq = pbus_devs[i].irq;
		pba.pb_bt = paa->plb_bt;
		pba.pb_dmat = paa->plb_dmat;

		(void) config_found_sm(self, &pba, pbus_print, pbus_submatch);
	}

#if NPCKBC > 0
	/* Configure FPGA */
	if (bus_space_map(iot_fpga, FPGA_BASE, FPGA_SIZE, 0, &ioh_fpga)) {
		printf("pbus_attach: can't map FPGA\n");
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
pbus_print(void *aux, const char *pnp)
{
	struct pbus_attach_args *pba = aux;

	if (pnp)
		printf("%s at %s", pba->pb_name, pnp);

	if (pba->pb_addr != PBUSCF_ADDR_DEFAULT)
		aprint_normal(" addr 0x%08lx", pba->pb_addr);
	if (pba->pb_irq != PBUSCF_IRQ_DEFAULT)
		aprint_normal(" irq %d", pba->pb_irq);

	return (UNCONF);
}
