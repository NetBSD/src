/*	$NetBSD: rside.c,v 1.1 2004/01/03 14:31:28 chris Exp $	*/

/*
 * Copyright (c) 1997-1998 Mark Brinicombe
 * Copyright (c) 1997-1998 Causality Limited
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/intr.h>
#include <machine/io.h>
#include <machine/bus.h>
#include <acorn32/eb7500atx/rsidereg.h>
#include <machine/irqhandler.h>

#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>
#include <acorn32/eb7500atx/rsbus.h>

/*
 * RiscStation IDE device.
 *
 * This probes and attaches the top level IDE device to the rsbus.
 * It then configures any children of the IDE device.
 * The attach args specify whether it is configuring the primary or
 * secondary channel.
 * The children are expected to be wdc devices using rside attachments.
 *
 * The hardware notes are:
 * Two ide ports are fitted, each with registers spaced 0x40 bytes apart
 * with the extra control register at offset 0x380 from the base of the
 * port.
 *
 * Primary:
 * 	Registers at 0x302b800 (nPCCS1 + 0x0) 
 * 	IRQ connected to nEvent1 (IRQ register D)
 *
 * Secondary:
 * 	Registers at 0x302bc00 (nPCCS1 + 0x400)
 * 	IRQ connected to nEvent2 (IRQ register D)
 *
 * PIO timings can be changed by modifying the access speed register in the
 * IOMD, as there is nothing else in the nPCCS1 space.
 *
 * The Reset line is asserted by unsetting bit 4 in IO register
 * IOMD + 0x121CC.
 */

/*
 * make a private tag so that we can use mainbus's map/unmap
 */

static struct bus_space rside_bs_tag;

/*
 * RiscStation IDE card softc structure.
 *
 * Contains the device node, podule information and global information
 * required by the driver such as the card version and the interrupt mask.
 */

struct rside_softc {
	struct wdc_softc	sc_wdcdev;	/* common wdc definitions */
	struct wdc_channel	*wdc_chanarray[2]; /* channels definition */
	struct rside_channel {
		struct wdc_channel wdc_channel; /* generic part */
		struct ata_queue wdc_chqueue; /* channel queue */
		irqhandler_t *wdc_ih;	/* interrupt handler */
	} rside_channels[2];
};

int	rside_probe	__P((struct device *, struct cfdata *, void *));
void	rside_attach	__P((struct device *, struct device *, void *));

CFATTACH_DECL(rside, sizeof(struct rside_softc),
		rside_probe, rside_attach, NULL, NULL);

/*
 * Create an array of address structures. These define the addresses and
 * masks needed for the different channels.
 *
 * index = channel
 */

struct {
	u_int drive_registers;
	u_int aux_register;
} rside_info[] = {
	{ PRIMARY_DRIVE_REGISTERS_POFFSET, PRIMARY_AUX_REGISTER_POFFSET,
	  },
	{ SECONDARY_DRIVE_REGISTERS_POFFSET, SECONDARY_AUX_REGISTER_POFFSET,
	   }
};

/*
 * Card probe function
 */

int
rside_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	/* if we're including this, then for now assume it exists */
	return 1;
}

/*
 * Card attach function
 *
 * Identify the card version and configure any children.
 */

void
rside_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct rside_softc *sc = (void *)self;
	struct rsbus_attach_args *rs = (void *)aux;
	int channel, i;
	struct rside_channel *scp;
	struct wdc_channel *cp;

	printf("\n");

	/*
	 * we need our own bus tag as the register spacing
	 * is not the default.
	 *
	 * For the rsbus the bus tag cookie is the shift
	 * to apply to registers
	 * So duplicate the bus space tag and change the
	 * cookie.
	 */

	rside_bs_tag = *rs->sa_iot;
	rside_bs_tag.bs_cookie = (void *) DRIVE_REGISTER_SPACING_SHIFT;

	/* Fill in wdc and channel infos */
	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA16;
	sc->sc_wdcdev.PIO_cap = 0;
	sc->sc_wdcdev.DMA_cap = 0;
	sc->sc_wdcdev.UDMA_cap = 0;
	sc->sc_wdcdev.channels = sc->wdc_chanarray;
	sc->sc_wdcdev.nchannels = 2;
	for (channel = 0 ; channel < 2; channel++) {
		scp = &sc->rside_channels[channel];
		sc->wdc_chanarray[channel] = &scp->wdc_channel;
		cp = &scp->wdc_channel;

		cp->channel = channel;
		cp->wdc = &sc->sc_wdcdev;
		cp->ch_queue = &scp->wdc_chqueue;
		cp->cmd_iot = cp->ctl_iot = &rside_bs_tag;
		if (bus_space_map(cp->cmd_iot,
		    rside_info[channel].drive_registers,
		    DRIVE_REGISTERS_SPACE, 0, &cp->cmd_baseioh)) 
			panic("couldn't map drive registers channel = %d,"
					"registers@0x08%x\n",
					channel, rside_info[channel].drive_registers);

		for (i = 0; i < WDC_NREG; i++) {
			if (bus_space_subregion(cp->cmd_iot, cp->cmd_baseioh,
				i * (DRIVE_REGISTER_BYTE_SPACING >> 2), 4,
				&cp->cmd_iohs[i]) != 0) {
				bus_space_unmap(cp->cmd_iot, cp->cmd_baseioh,
				    DRIVE_REGISTERS_SPACE);
				continue;
			}
		}

		if (bus_space_map(cp->ctl_iot,
		    rside_info[channel].aux_register, 0x4, 0, &cp->ctl_ioh))
		{
			bus_space_unmap(cp->cmd_iot, cp->cmd_baseioh,
			    DRIVE_REGISTERS_SPACE);
			continue;
		}


		/* attach it to the interrupt */
		if ((scp->wdc_ih = intr_claim((channel == 0 ? IRQ_NEVENT1 : IRQ_NEVENT2), IPL_BIO,
			    "rside", wdcintr, cp)) == NULL)
			panic("%s: Cannot claim interrupt %d\n",
			    self->dv_xname, (channel == 0 ? IRQ_NEVENT1 : IRQ_NEVENT2));

		wdcattach(cp);
	}
}
