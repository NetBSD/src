/*	$NetBSD: iomd.c,v 1.13 2001/07/09 23:42:18 bjh21 Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/irqhandler.h>
#include <arm32/iomd/iomdreg.h>
#include <arm32/iomd/iomdvar.h>

#include "iomd.h"
#if NIOMD != 1
#error Need one IOMD device configured
#endif

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
	struct device 		sc_dev;	/* device node */
	bus_space_tag_t		sc_iot;	/* bus tag */
	bus_space_handle_t	sc_ioh;	/* bus handle */
	int			sc_id;	/* IOMD id */
};

static int iomdmatch	__P((struct device *parent, struct cfdata *cf,
                             void *aux));
static void iomdattach	__P((struct device *parent, struct device *self,
                             void *aux));
static int iomdprint	__P((void *aux, const char *iomdbus));

struct cfattach iomd_ca = {
	sizeof(struct iomd_softc), iomdmatch, iomdattach
};

extern struct bus_space iomd_bs_tag;

int       iomd_found;
u_int32_t iomd_base = IOMD_BASE;

/* following flag is used in iomd_irq.s ... has to be cleaned up one day ! */
u_int32_t arm7500_ioc_found = 0;


/* Declare prototypes */

/*
 * int iomdprint(void *aux, const char *name)
 *
 * print configuration info for children
 */

static int
iomdprint(aux, name)
	void *aux;
	const char *name;
{
/*	union iomd_attach_args *ia = aux;*/

	return(QUIET);
}

/*
 * int iomdmatch(struct device *parent, struct cfdata *cf, void *aux)
 *
 * Just return ok for this if it is device 0
 */ 
 
static int
iomdmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	if (iomd_found)
		return 0;
	return 1;
}


/*
 * void iomdattach(struct device *parent, struct device *dev, void *aux)
 *
 * Map the IOMD and identify it.
 * Then configure the child devices based on the IOMD ID.
 */
  
static void
iomdattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct iomd_softc *sc = (struct iomd_softc *)self;
/*	struct mainbus_attach_args *mb = aux;*/
	int refresh;
#if 0
	int dma_time;
	int combo_time;
	int loop;
#endif
	union iomd_attach_args ia;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	/* There can be only 1 IOMD. */
	iomd_found = 1;

	iot = sc->sc_iot = &iomd_bs_tag;

	/* Map the IOMD */
	if (bus_space_map(iot, (int) iomd_base, IOMD_SIZE, 0, &ioh))
		panic("%s: Cannot map registers\n", self->dv_xname);

	sc->sc_ioh = ioh;

	/* Get the ID */
	sc->sc_id = bus_space_read_1(iot, ioh, IOMD_ID0)
		  | (bus_space_read_1(iot, ioh, IOMD_ID1) << 8);

	printf(": ");

	/* Identify it and get the DRAM refresh rate */
	switch (sc->sc_id) {
	case ARM7500_IOC_ID:
		printf("ARM7500 IOMD ");
		refresh = bus_space_read_1(iot, ioh, IOMD_REFCR) & 0x0f;
		arm7500_ioc_found = 1;
		break;
	case ARM7500FE_IOC_ID:
		printf("ARM7500FE IOMD ");
		refresh = bus_space_read_1(iot, ioh, IOMD_REFCR) & 0x0f;
		arm7500_ioc_found = 1;
		break;
	case RPC600_IOMD_ID:
		printf("IOMD20 ");
		refresh = bus_space_read_1(iot, ioh, IOMD_VREFCR) & 0x09;
		arm7500_ioc_found = 0;
		break;
	default:
		printf("Unknown IOMD ID=%04x ", sc->sc_id);
		refresh = -1;
		arm7500_ioc_found = 0;		/* just in case */
		break;
	}
	
	printf("\n");

	/* Report the DRAM refresh rate */
	printf("%s: ", self->dv_xname);
	printf("DRAM refresh=");
	switch (refresh) {
	case 0x0:
		printf("off");
		break;
	case 0x1:
		printf("16us");
		break;
	case 0x2:
		printf("32us");
		break;
	case 0x4:
		printf("64us");
		break;
	case 0x8:
		printf("128us");
		break;
	default:
		printf("unknown [%02x]", refresh);
		break;
	}

#if 0
	/*
	 * No point in reporting this as it may get changed when devices are
	 * attached
	 */
	dma_time = ReadByte(IOMD_DMATCR);
	printf(", dma cycle types=");
	for (loop = 0; loop < 4; ++loop,dma_time = dma_time >> 2)
		printf("%c", 'A' + (dma_time & 3));

     	combo_time = ReadByte(IOMD_IOTCR);
	printf(", combo cycle type=%c", 'A' + ((combo_time >> 2) & 3));
#endif
	printf("\n");

	/* Set up the external DMA channels */
	/* XXX - this should be machine dependant not IOMD dependant */
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
		panic("%s: Cannot map kbd registers\n", self->dv_xname);
	ia.ia_kbd.ka_name = "kbd";
	ia.ia_kbd.ka_iot = iot;
	ia.ia_kbd.ka_rxirq = IRQ_KBDRX;
	ia.ia_kbd.ka_txirq = IRQ_KBDTX;
	config_found(self, &ia, iomdprint);

	/* Attach rpckbc device when configured */
	ia.ia_kbd.ka_name = "rpckbd";
	ia.ia_kbd.ka_rxirq = IRQ_KBDRX;
	ia.ia_kbd.ka_txirq = IRQ_KBDTX;
	config_found(self, &ia, iomdprint);

	/* Attach iic device */

	if (bus_space_subregion(iot, ioh, IOMD_IOCR, 4, &ia.ia_iic.ia_ioh))
		panic("%s: Cannot map iic registers\n", self->dv_xname);
	ia.ia_iic.ia_name = "iic";
	ia.ia_iic.ia_iot = iot;
	ia.ia_iic.ia_irq = -1;
	config_found(self, &ia, iomdprint);

	switch (sc->sc_id) {
	case ARM7500_IOC_ID:
	case ARM7500FE_IOC_ID:
		/* Attach pms device */

		if (bus_space_subregion(iot, ioh, IOMD_MSDATA, 8, &ia.ia_pms.pa_ioh))
			panic("%s: Cannot map pms registers\n", self->dv_xname);
		ia.ia_pms.pa_name = "pms";
		ia.ia_pms.pa_iot = iot;
		ia.ia_pms.pa_irq = IRQ_MSDRX;
		config_found(self, &ia, iomdprint);
		break;
	case RPC600_IOMD_ID:
		/* Attach (ws)qms device */

		if (bus_space_subregion(iot, ioh, IOMD_MOUSEX, 8, &ia.ia_qms.qa_ioh))
			panic("%s: Cannot map qms registers\n", self->dv_xname);

		if (bus_space_map(iot, IO_MOUSE_BUTTONS, 4, 0, &ia.ia_qms.qa_ioh_but))
			panic("%s: Cannot map registers\n", self->dv_xname);
		ia.ia_qms.qa_name = "qms";
		ia.ia_qms.qa_iot = iot;
		ia.ia_qms.qa_irq = IRQ_VSYNC;
		config_found(self, &ia, iomdprint);

		ia.ia_qms.qa_name = "wsqms";
		config_found(self, &ia, iomdprint);
		break;
	}
}

/* End of iomd.c */
