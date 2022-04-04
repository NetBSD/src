/*	$NetBSD: icside.c,v 1.35 2022/04/04 19:33:44 andvar Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: icside.c,v 1.35 2022/04/04 19:33:44 andvar Exp $");

#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/bus.h>

#include <machine/intr.h>
#include <machine/io.h>
#include <acorn32/podulebus/podulebus.h>
#include <acorn32/podulebus/icsidereg.h>

#include <dev/ata/atavar.h>
#include <dev/ic/wdcreg.h>
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
	bus_space_tag_t		sc_latchiot;	/* EEPROM page latch etc */
	bus_space_handle_t	sc_latchioh;
	void			*sc_shutdownhook;
	struct ata_channel *sc_chp[ICSIDE_MAX_CHANNELS];
	struct icside_channel {
		struct ata_channel	ic_channel;	/* generic part */
		void			*ic_ih;		/* interrupt handler */
		struct evcnt		ic_intrcnt;	/* interrupt count */
		u_int			ic_irqaddr;	/* interrupt flag */
		u_int			ic_irqmask;	/*  location */
		bus_space_tag_t		ic_irqiot;	/* Bus space tag */
		bus_space_handle_t	ic_irqioh;	/* handle for IRQ */
	} sc_chan[ICSIDE_MAX_CHANNELS];
	struct wdc_regs sc_wdc_regs[ICSIDE_MAX_CHANNELS];
};

int	icside_probe(device_t, cfdata_t, void *);
void	icside_attach(device_t, device_t, void *);
int	icside_intr(void *);
void	icside_v6_shutdown(void *);

CFATTACH_DECL_NEW(icside, sizeof(struct icside_softc),
    icside_probe, icside_attach, NULL, NULL);

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
	int		latchreg;	/* EEPROM latch register */
	int		ideregs[MAX_CHANNELS];	/* IDE registers */
	int		auxregs[MAX_CHANNELS];	/* AUXSTAT register */
	int		irqregs[MAX_CHANNELS];	/* IRQ register */
	int		irqstatregs[MAX_CHANNELS];
} const ide_versions[] = {
	/* A3IN - Unsupported */
/*	{ 0,  0, 0, NULL, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },*/
	/* A3USER - Unsupported */
/*	{ 1,  0, 0, NULL, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },*/
	/* ARCIN V6 - Supported */
	{ 3,  0, 2, "ARCIN V6", V6_ADDRLATCH,
	  { V6_P_IDE_BASE, V6_S_IDE_BASE },
	  { V6_P_AUX_BASE, V6_S_AUX_BASE },
	  { V6_P_IRQ_BASE, V6_S_IRQ_BASE },
	  { V6_P_IRQSTAT_BASE, V6_S_IRQSTAT_BASE }
	},
	/* ARCIN V5 - Supported (ID reg not supported so reads as 15) */
	{ 15,  1, 1, "ARCIN V5", -1,
	  { V5_IDE_BASE, -1 },
	  { V5_AUX_BASE, -1 },
	  { V5_IRQ_BASE, -1 },
	  { V5_IRQSTAT_BASE, -1 }
	}
};

/*
 * Card probe function
 *
 * Just match the manufacturer and podule ID's
 */

int
icside_probe(device_t parent, cfdata_t cf, void *aux)
{
	struct podule_attach_args *pa = (void *)aux;

	return (pa->pa_product == PODULE_ICS_IDE);
}

/*
 * Card attach function
 *
 * Identify the card version and configure any children.
 */

void
icside_attach(device_t parent, device_t self, void *aux)
{
	struct icside_softc *sc = device_private(self);
	struct podule_attach_args *pa = (void *)aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	const struct ide_version *ide = NULL;
	u_int iobase;
	int channel, i;
	struct icside_channel *icp;
	struct ata_channel *cp;
	struct wdc_regs *wdr;
	int loop;
	int id;

	/* Note the podule number and validate */

	if (pa->pa_podule_number == -1)
		panic("Podule has disappeared !");

	sc->sc_wdcdev.sc_atac.atac_dev = self;
	sc->sc_podule_number = pa->pa_podule_number;
	sc->sc_podule = pa->pa_podule;
	podules[sc->sc_podule_number].attached = 1;

	sc->sc_wdcdev.regs = sc->sc_wdc_regs;

	/* The ID register if present is always in FAST podule space */
	iot = pa->pa_iot;
	if (bus_space_map(iot, pa->pa_podule->fast_base +
	    ID_REGISTER_OFFSET, ID_REGISTER_SPACE, 0, &ioh)) {
		aprint_error_dev(self, "cannot map ID register\n");
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
		aprint_error(": rev %d is unsupported\n", id);
		return;
	} else
		aprint_normal(": %s\n", ide->name);

	if (ide->latchreg != -1) {
		sc->sc_latchiot = pa->pa_iot;
		if (bus_space_map(iot, pa->pa_podule->fast_base +
			ide->latchreg, 1, 0, &sc->sc_latchioh)) {
			aprint_error_dev(self,
			    "cannot map latch register\n");
			return;
		}
		sc->sc_shutdownhook =
		    shutdownhook_establish(icside_v6_shutdown, sc);
	}

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
	 * read/write multiple short functions with
	 * optimised versions
	 */

	sc->sc_tag = *pa->pa_iot;
	sc->sc_tag.bs_cookie = (void *) REGISTER_SPACING_SHIFT;
	sc->sc_tag.bs_rm_2 = icside_bs_rm_2;
	sc->sc_tag.bs_wm_2 = icside_bs_wm_2;

	/* Initialize wdc struct */
	sc->sc_wdcdev.sc_atac.atac_channels = sc->sc_chp;
	sc->sc_wdcdev.sc_atac.atac_nchannels = ide->channels;
	sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DATA16;
	sc->sc_wdcdev.sc_atac.atac_pio_cap = 0;
	sc->sc_wdcdev.wdc_maxdrives = 2;
	sc->sc_pa = pa;

	for (channel = 0; channel < ide->channels; ++channel) {
		icp = &sc->sc_chan[channel];
		sc->sc_wdcdev.sc_atac.atac_channels[channel] = &icp->ic_channel;
		cp = &icp->ic_channel;
		wdr = &sc->sc_wdc_regs[channel];

		cp->ch_channel = channel;
		cp->ch_atac = &sc->sc_wdcdev.sc_atac;
		wdr->cmd_iot = &sc->sc_tag;
		wdr->ctl_iot = &sc->sc_tag;
		if (ide->modspace)
			iobase = pa->pa_podule->mod_base;
		else
			iobase = pa->pa_podule->fast_base;

		if (bus_space_map(iot, iobase + ide->ideregs[channel],
		    IDE_REGISTER_SPACE, 0, &wdr->cmd_baseioh))
			return;
		for (i = 0; i < WDC_NREG; i++) {
			if (bus_space_subregion(wdr->cmd_iot, wdr->cmd_baseioh,
				i, i == 0 ? 4 : 1, &wdr->cmd_iohs[i]) != 0)
				return;
		}
		wdc_init_shadow_regs(wdr);
		if (bus_space_map(iot, iobase + ide->auxregs[channel],
		    AUX_REGISTER_SPACE, 0, &wdr->ctl_ioh))
			return;
		icp->ic_irqiot = iot;
		if (bus_space_map(iot, iobase + ide->irqregs[channel],
		    IRQ_REGISTER_SPACE, 0, &icp->ic_irqioh))
			return;
		/* Disable interrupts */
		(void)bus_space_read_1(iot, icp->ic_irqioh, 0);
		pa->pa_podule->irq_addr = iobase + ide->irqstatregs[channel];
		pa->pa_podule->irq_mask = IRQ_STATUS_REGISTER_MASK;
		icp->ic_irqaddr = pa->pa_podule->irq_addr;
		icp->ic_irqmask = pa->pa_podule->irq_mask;
		evcnt_attach_dynamic(&icp->ic_intrcnt, EVCNT_TYPE_INTR, NULL,
		    device_xname(self), "intr");
		icp->ic_ih = podulebus_irq_establish(pa->pa_ih, IPL_BIO,
		    icside_intr, icp, &icp->ic_intrcnt);
		if (icp->ic_ih == NULL) {
			aprint_error_dev(self, "Cannot claim interrupt %d\n",
			    pa->pa_podule->interrupt);
			continue;
		}
		/* Enable interrupts */
		bus_space_write_1(iot, icp->ic_irqioh, 0, 0);
		/* Call common attach routines */
		wdcattach(cp);

	}
}

/*
 * Shutdown handler -- try to restore the card to a state where
 * RISC OS will see it.
 */
void
icside_v6_shutdown(void *arg)
{
	struct icside_softc *sc = arg;

	bus_space_write_1(sc->sc_latchiot, sc->sc_latchioh, 0, 0);
}

/*
 * Podule interrupt handler
 *
 * If the interrupt was from our card pass it on to the wdc interrupt handler
 */
int
icside_intr(void *arg)
{
	struct icside_channel *icp = arg;
	volatile u_char *intraddr = (volatile u_char *)icp->ic_irqaddr;

	/* XXX - not bus space yet - should really be handled by podulebus */
	if ((*intraddr) & icp->ic_irqmask)
		wdcintr(&icp->ic_channel);
	return(0);
}
