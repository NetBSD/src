/*	$NetBSD: iomd.c,v 1.17.2.1 2012/05/23 10:07:42 yamt Exp $	*/

/*
 * Copyright (c) 1996-1997 Mark Brinicombe.
 * Copyright (c) 1997 Causality Limited
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Mark Brinicombe.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * iomd.c
 *
 * Probing and configuration for the IOMD
 *
 * Created      : 10/10/95
 * Updated	: 18/03/01 for rpckbd as part of the wscons project
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: iomd.c,v 1.17.2.1 2012/05/23 10:07:42 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <machine/cpu.h>
#include <machine/intr.h>
#include <arm/iomd/iomdreg.h>
#include <arm/iomd/iomdvar.h>

#include "iomd.h"

/*
 * IOMD device.
 *
 * This probes and attaches the top level IOMD device.
 * It then configures any children of the IOMD device.
 */

/*
 * IOMD softc structure.
 *
 * Contains the device node, bus space tag, handle and address
 * and the IOMD id.
 */

struct iomd_softc {
	device_t 		sc_dev;	/* device node */
	bus_space_tag_t		sc_iot;	/* bus tag */
	bus_space_handle_t	sc_ioh;	/* bus handle */
	int			sc_id;	/* IOMD id */
};

static int iomdmatch(device_t parent, cfdata_t cf, void *aux);
static void iomdattach(device_t parent, device_t self, void *aux);
static int iomdprint(void *aux, const char *iomdbus);

CFATTACH_DECL_NEW(iomd, sizeof(struct iomd_softc),
    iomdmatch, iomdattach, NULL, NULL);

extern struct bus_space iomd_bs_tag;

int       iomd_found;
uint32_t iomd_base = IOMD_BASE;

/* following flag is used in iomd_irq.s ... has to be cleaned up one day ! */
uint32_t arm7500_ioc_found = 0;


/* Declare prototypes */

/*
 * int iomdprint(void *aux, const char *name)
 *
 * print configuration info for children
 */

static int
iomdprint(void *aux, const char *name)
{
/*	union iomd_attach_args *ia = aux;*/

	return QUIET;
}

/*
 * int iomdmatch(device_t parent, cfdata_t cf, void *aux)
 *
 * Just return ok for this if it is device 0
 */ 
 
static int
iomdmatch(device_t parent, cfdata_t cf, void *aux)
{

	if (iomd_found)
		return 0;
	return 1;
}


/*
 * void iomdattach(device_t parent, device_t dev, void *aux)
 *
 * Map the IOMD and identify it.
 * Then configure the child devices based on the IOMD ID.
 */
  
static void
iomdattach(device_t parent, device_t self, void *aux)
{
	struct iomd_softc *sc = device_private(self);
/*	struct mainbus_attach_args *mb = aux;*/
	int refresh;
#if 0
	int i, tmp;
#endif
	union iomd_attach_args ia;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	/* There can be only 1 IOMD. */
	iomd_found = 1;

	sc->sc_dev = self;
	iot = sc->sc_iot = &iomd_bs_tag;

	/* Map the IOMD */
	if (bus_space_map(iot, (int) iomd_base, IOMD_SIZE, 0, &ioh))
		panic("%s: Cannot map registers", self->dv_xname);

	sc->sc_ioh = ioh;

	/* Get the ID */
	sc->sc_id = bus_space_read_1(iot, ioh, IOMD_ID0)
		  | (bus_space_read_1(iot, ioh, IOMD_ID1) << 8);
	aprint_normal(": ");

	/* Identify it and get the DRAM refresh rate */
	switch (sc->sc_id) {
	case ARM7500_IOC_ID:
		aprint_normal("ARM7500 IOMD ");
		refresh = bus_space_read_1(iot, ioh, IOMD_REFCR) & 0x0f;
		arm7500_ioc_found = 1;
		break;
	case ARM7500FE_IOC_ID:
		aprint_normal("ARM7500FE IOMD ");
		refresh = bus_space_read_1(iot, ioh, IOMD_REFCR) & 0x0f;
		arm7500_ioc_found = 1;
		break;
	case RPC600_IOMD_ID:
		aprint_normal("IOMD20 ");
		refresh = bus_space_read_1(iot, ioh, IOMD_VREFCR) & 0x09;
		arm7500_ioc_found = 0;
		break;
	default:
		aprint_normal("Unknown IOMD ID=%04x ", sc->sc_id);
		refresh = -1;
		arm7500_ioc_found = 0;		/* just in case */
		break;
	}
	aprint_normal("version %d\n", bus_space_read_1(iot, ioh, IOMD_VERSION));

	/* Report the DRAM refresh rate */
	aprint_normal("%s: ", self->dv_xname);
	aprint_normal("DRAM refresh=");
	switch (refresh) {
	case 0x0:
		aprint_normal("off");
		break;
	case 0x1:
		aprint_normal("16us");
		break;
	case 0x2:
		aprint_normal("32us");
		break;
	case 0x4:
		aprint_normal("64us");
		break;
	case 0x8:
		aprint_normal("128us");
		break;
	default:
		aprint_normal("unknown [%02x]", refresh);
		break;
	}

	aprint_normal("\n");
#if 0
	/*
	 * No point in reporting this as it may get changed when devices are
	 * attached
	 */
	tmp = bus_space_read_1(iot, ioh, IOMD_IOTCR);
	aprint_normal("%s: I/O timings: combo %c, NPCCS1/2 %c", self->dv_xname,
	    'A' + ((tmp >>2) & 3), 'A' + (tmp & 3));
	tmp = bus_space_read_1(iot, ioh, IOMD_ECTCR);
	aprint_normal(", EASI ");
	for (i = 0; i < 8; i++, tmp >>= 1)
		aprint_normal("%c", 'A' + ((tmp & 1) << 2));
	tmp = bus_space_read_1(iot, ioh, IOMD_DMATCR);
	aprint_normal(", DMA ");
	for (i = 0; i < 4; i++, tmp >>= 2)
		aprint_normal("%c", 'A' + (tmp & 3));	
	aprint_normal("\n");
#endif

	/* Set up the external DMA channels */
	/* XXX - this should be machine dependent not IOMD dependent */
	switch (sc->sc_id) {
	case ARM7500_IOC_ID:
	case ARM7500FE_IOC_ID:
		break;
	case RPC600_IOMD_ID:
		/* DMA channels 2 & 3 are external */
		bus_space_write_1(iot, ioh, IOMD_DMAEXT, 0x0c);
		break;
   	}

	/* Configure the child devices */

	/* Attach clock device */

	ia.ia_clk.ca_name = "clk";
	ia.ia_clk.ca_iot = iot;
	ia.ia_clk.ca_ioh = ioh;
	config_found(self, &ia, iomdprint);

	/* Attach kbd device when configured */
	if (bus_space_subregion(iot, ioh, IOMD_KBDDAT, 8, &ia.ia_kbd.ka_ioh))
		panic("%s: Cannot map kbd registers", self->dv_xname);
	ia.ia_kbd.ka_name = "kbd";
	ia.ia_kbd.ka_iot = iot;
	ia.ia_kbd.ka_rxirq = IRQ_KBDRX;
	ia.ia_kbd.ka_txirq = IRQ_KBDTX;
	config_found(self, &ia, iomdprint);

	/* Attach iic device */

	if (bus_space_subregion(iot, ioh, IOMD_IOCR, 4, &ia.ia_iic.ia_ioh))
		panic("%s: Cannot map iic registers", self->dv_xname);
	ia.ia_iic.ia_name = "iic";
	ia.ia_iic.ia_iot = iot;
	ia.ia_iic.ia_irq = -1;
	config_found(self, &ia, iomdprint);

	switch (sc->sc_id) {
	case ARM7500_IOC_ID:
	case ARM7500FE_IOC_ID:
		/* Attach opms device */

		if (bus_space_subregion(iot, ioh, IOMD_MSDATA, 8,
			&ia.ia_opms.pa_ioh))
			panic("%s: Cannot map opms registers", self->dv_xname);
		ia.ia_opms.pa_name = "opms";
		ia.ia_opms.pa_iot = iot;
		ia.ia_opms.pa_irq = IRQ_MSDRX;
		config_found(self, &ia, iomdprint);
		break;
	case RPC600_IOMD_ID:
		/* Attach (ws)qms device */

		if (bus_space_subregion(iot, ioh, IOMD_MOUSEX, 8,
			&ia.ia_qms.qa_ioh))
			panic("%s: Cannot map qms registers", self->dv_xname);

		if (bus_space_map(iot, IO_MOUSE_BUTTONS, 4, 0, &ia.ia_qms.qa_ioh_but))
			panic("%s: Cannot map registers", self->dv_xname);
		ia.ia_qms.qa_name = "qms";
		ia.ia_qms.qa_iot = iot;
		ia.ia_qms.qa_irq = IRQ_VSYNC;
		config_found(self, &ia, iomdprint);
		break;
	}
}

/* End of iomd.c */
