/*	$NetBSD: icside.c,v 1.13 2001/03/17 20:34:44 bjh21 Exp $	*/

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
 *
 * Probe and attach functions to use generic IDE driver for the ICS IDE podule
 */

/*
 * Thanks to David Baildon for loaning an IDE card for the development
 * of this driver.
 * Thanks to Ian Copestake and David Baildon for providing register mapping
 * information
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/irqhandler.h>
#include <machine/io.h>
#include <machine/bus.h>
#include <arm32/podulebus/podulebus.h>
#include <arm32/podulebus/icsidereg.h>

#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>
#include <dev/podulebus/podules.h>

/*
 * ICS IDE podule device.
 *
 * This probes and attaches the top level ICS IDE device to the podulebus.
 * It then configures any children of the ICS IDE device.
 * The child is expected to be a wdc device using icside attachments.
 */

/*
 * ICS IDE card softc structure.
 *
 * Contains the device node and podule information.
 */

struct icside_softc {
	struct wdc_softc	sc_wdcdev;	/* common wdc definitions */
	podule_t 		*sc_podule;		/* Our podule */
	int 			sc_podule_number;	/* Our podule number */
	struct bus_space 	sc_tag;			/* custom tag */
	struct podule_attach_args *sc_pa;		/* podule info */
	struct icside_channel {
		struct channel_softc	wdc_channel;	/* generic part */
		irqhandler_t		ic_ih;		/* interrupt handler */
		bus_space_tag_t		ic_irqiot;	/* Bus space tag */
		bus_space_handle_t	ic_irqioh;	/* handle for IRQ */
	} *icside_channels;
};

int	icside_probe	__P((struct device *, struct cfdata *, void *));
void	icside_attach	__P((struct device *, struct device *, void *));
int	icside_intr	__P((void *));

struct cfattach icside_ca = {
	sizeof(struct icside_softc), icside_probe, icside_attach
};

/*
 * Define prototypes for custom bus space functions.
 */

bs_rm_2_proto(icside);
bs_wm_2_proto(icside);

#define MAX_CHANNELS	2

/*
 * Define a structure for describing the different card versions
 */
struct ide_version {
	int		id;		/* IDE card ID */
	int		modspace;	/* Type of podule space */
	int		channels;	/* Number of channels */
	const char	*name;		/* name */
	int		ideregs[MAX_CHANNELS];	/* IDE registers */
	int		auxregs[MAX_CHANNELS];	/* AUXSTAT register */
	int		irqregs[MAX_CHANNELS];	/* IRQ register */
	int		irqstatregs[MAX_CHANNELS];
} ide_versions[] = {
	/* A3IN - Unsupported */
/*	{ 0,  0, 0, NULL, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },*/
	/* A3USER - Unsupported */
/*	{ 1,  0, 0, NULL, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },*/
	/* ARCIN V6 - Supported */
	{ 3,  0, 2, "ARCIN V6", 
	  { V6_P_IDE_BASE, V6_S_IDE_BASE },
	  { V6_P_AUX_BASE, V6_S_AUX_BASE },
	  { V6_P_IRQ_BASE, V6_S_IRQ_BASE },
	  { V6_P_IRQSTAT_BASE, V6_S_IRQSTAT_BASE }
	},
	/* ARCIN V5 - Supported (ID reg not supported so reads as 15) */
	{ 15,  1, 1, "ARCIN V5", 
	  { V5_IDE_BASE, 0 },
	  { V5_AUX_BASE, 0 },
	  { V5_IRQ_BASE, 0 },
	  { V5_IRQSTAT_BASE, 0 }
	}
};

/*
 * Card probe function
 *
 * Just match the manufacturer and podule ID's
 */

int
icside_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct podule_attach_args *pa = (void *)aux;
	if (matchpodule(pa, MANUFACTURER_ICS, PODULE_ICS_IDE, -1) == 0)
		return(0);

	return(1);
}

/*
 * Card attach function
 *
 * Identify the card version and configure any children.
 */

void
icside_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct icside_softc *sc = (void *)self;
	struct podule_attach_args *pa = (void *)aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	irqhandler_t *ihp;
	struct ide_version *ide = NULL;
	u_int iobase;
	int channel;
	struct icside_channel *icp;
	struct channel_softc *cp;
	int loop;
	int id;

	/* Note the podule number and validate */

	if (pa->pa_podule_number == -1)
		panic("Podule has disappeared !");

	sc->sc_podule_number = pa->pa_podule_number;
	sc->sc_podule = pa->pa_podule;
	podules[sc->sc_podule_number].attached = 1;

	/* The ID register if present is always in FAST podule space */
	iot = pa->pa_iot;
	if (bus_space_map(iot, pa->pa_podule->fast_base +
	    ID_REGISTER_OFFSET, ID_REGISTER_SPACE, 0, &ioh)) {
		printf("%s: cannot map ID register\n", self->dv_xname);
		return;
	}

	for (id = 0, loop = 0; loop < 4; ++loop)
		id |= (bus_space_read_1(iot, ioh, loop) & 1) << loop;

	/* Do we recognise the ID ? */
	for (loop = 0; loop < sizeof(ide_versions) / sizeof(struct ide_version);
	    ++loop) {
		if (ide_versions[loop].id == id) {
			ide = &ide_versions[loop];
			break;
		}
	}

	/* Report the version and name */
	if (ide == NULL || ide->name == NULL) {
		printf(": rev %d is unsupported\n", id);
		return;
	} else
		printf(": %s\n", ide->name);

	/*
	 * Ok we need our own bus tag as the register spacing
	 * is not the default.
	 *
	 * For the podulebus the bus tag cookie is the shift
	 * to apply to registers
	 * So duplicate the bus space tag and change the
	 * cookie.
	 *
	 * Also while we are at it replace the default
	 * read/write mulitple short functions with
	 * optimised versions
	 */

	sc->sc_tag = *pa->pa_iot;
	sc->sc_tag.bs_cookie = (void *) REGISTER_SPACING_SHIFT;
	sc->sc_tag.bs_rm_2 = icside_bs_rm_2;
	sc->sc_tag.bs_wm_2 = icside_bs_wm_2;

	/* Initialize wdc struct */
	sc->sc_wdcdev.channels = malloc(
	    sizeof(struct channel_softc *) * ide->channels, M_DEVBUF, M_NOWAIT);
	sc->icside_channels = malloc(
	    sizeof(struct icside_channel) * ide->channels, M_DEVBUF, M_NOWAIT);
	if (sc->sc_wdcdev.channels == NULL || sc->icside_channels == NULL) {
		printf("%s: can't allocate channel infos\n",
		    sc->sc_wdcdev.sc_dev.dv_xname);
		return;
	}
	sc->sc_wdcdev.nchannels = ide->channels;
	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA16;
	sc->sc_wdcdev.PIO_cap = 0;
	sc->sc_pa = pa;

	for (channel = 0; channel < ide->channels; ++channel) {
		icp = &sc->icside_channels[channel];
		sc->sc_wdcdev.channels[channel] = &icp->wdc_channel;
		cp = &icp->wdc_channel;

		cp->channel = channel;
		cp->wdc = &sc->sc_wdcdev;
		cp->ch_queue = malloc(sizeof(struct channel_queue), M_DEVBUF,
		    M_NOWAIT);
		if (cp->ch_queue == NULL) {
			printf("%s %s channel: "
			    "can't allocate memory for command queue",
			    sc->sc_wdcdev.sc_dev.dv_xname,
			    (channel == 0) ? "primary" : "secondary");
			continue;
		}
		cp->cmd_iot = &sc->sc_tag;
		cp->ctl_iot = &sc->sc_tag;
		if (ide->modspace)
			iobase = pa->pa_podule->mod_base;
		else
			iobase = pa->pa_podule->fast_base;

		if (bus_space_map(iot, iobase + ide->ideregs[channel],
		    IDE_REGISTER_SPACE, 0, &cp->cmd_ioh))
			return;
		if (bus_space_map(iot, iobase + ide->auxregs[channel],
		    AUX_REGISTER_SPACE, 0, &cp->ctl_ioh))
			return;
		icp->ic_irqiot = iot;
		if (bus_space_map(iot, iobase + ide->irqregs[channel],
		    IRQ_REGISTER_SPACE, 0, &icp->ic_irqioh))
			return;
		/* Disable interrupts */
		(void)bus_space_read_1(iot, icp->ic_irqioh, 0);
		/* Call common attach routines */
		wdcattach(cp);
		/* Disable interrupts */
		(void)bus_space_read_1(iot, icp->ic_irqioh, 0);
		pa->pa_podule->irq_addr = iobase + ide->irqstatregs[channel];
		pa->pa_podule->irq_mask = IRQ_STATUS_REGISTER_MASK;
		ihp = &icp->ic_ih;
		ihp->ih_func = icside_intr;
		ihp->ih_arg = icp;
		ihp->ih_level = IPL_BIO;
		ihp->ih_name = "icside";
		ihp->ih_maskaddr = pa->pa_podule->irq_addr;
		ihp->ih_maskbits = pa->pa_podule->irq_mask;
		if (irq_claim(pa->pa_podule->interrupt, ihp)) {
			printf("%s: Cannot claim interrupt %d\n",
			    sc->sc_wdcdev.sc_dev.dv_xname,
			    pa->pa_podule->interrupt);
			continue;
		}
		/* Enable interrupts */
		bus_space_write_1(iot, icp->ic_irqioh, 0, 0);
	}
}

/*
 * Podule interrupt handler
 *
 * If the interrupt was from our card pass it on to the wdc interrupt handler
 */
int
icside_intr(arg)
	void *arg;
{
	struct icside_channel *icp = arg;
	irqhandler_t *ihp = &icp->ic_ih;
	volatile u_char *intraddr = (volatile u_char *)ihp->ih_maskaddr;

	/* XXX - not bus space yet - should really be handled by podulebus */
	if ((*intraddr) & ihp->ih_maskbits)
		wdcintr(&icp->wdc_channel);
	return(0);
}
