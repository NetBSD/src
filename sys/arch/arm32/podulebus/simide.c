/*	$NetBSD: simide.c,v 1.10.10.1 2001/03/27 15:30:32 bouyer Exp $	*/

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
 * Card driver and probe and attach functions to use generic IDE driver
 * for the Simtec IDE podule
 */

/*
 * Thanks to Gareth Simpson, Simtec Electronics for providing
 * the hardware information
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
#include <arm32/podulebus/simidereg.h>

#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>
#include <dev/podulebus/podules.h>


/*
 * Simtec IDE podule device.
 *
 * This probes and attaches the top level Simtec IDE device to the podulebus.
 * It then configures any children of the Simtec IDE device.
 * The attach args specify whether it is configuring the primary or
 * secondary channel.
 * The children are expected to be wdc devices using simide attachments.
 */

/*
 * Simtec IDE card softc structure.
 *
 * Contains the device node, podule information and global information
 * required by the driver such as the card version and the interrupt mask.
 */

struct simide_softc {
	struct wdc_softc	sc_wdcdev;	/* common wdc definitions */
	struct channel_softc	*wdc_chanarray[2]; /* channels definition */
	podule_t 		*sc_podule;		/* Our podule info */
	int 			sc_podule_number;	/* Our podule number */
	int			sc_ctl_reg;		/* Global ctl reg */
	int			sc_version;		/* Card version */
	bus_space_tag_t		sc_ctliot;		/* Bus tag */
	bus_space_handle_t	sc_ctlioh;		/* control handle */
	struct bus_space 	sc_tag;			/* custom tag */
	struct simide_channel {
		struct channel_softc wdc_channel; /* generic part */
		irqhandler_t	sc_ih;			/* interrupt handler */
		int		sc_irqmask;	/* IRQ mask for this channel */
	} simide_channels[2];
};

int	simide_probe	__P((struct device *, struct cfdata *, void *));
void	simide_attach	__P((struct device *, struct device *, void *));
void	simide_shutdown	__P((void *arg));
int	simide_intr	__P((void *arg));

struct cfattach simide_ca = {
	sizeof(struct simide_softc), simide_probe, simide_attach
};


/*
 * Define prototypes for custom bus space functions.
 */

bs_rm_2_proto(simide);
bs_wm_2_proto(simide);

/*
 * Create an array of address structures. These define the addresses and
 * masks needed for the different channels.
 *
 * index = channel
 */

struct {
	u_int drive_registers;
	u_int aux_register;
	u_int irq_mask;
} simide_info[] = {
	{ PRIMARY_DRIVE_REGISTERS_POFFSET, PRIMARY_AUX_REGISTER_POFFSET,
	  CONTROL_PRIMARY_IRQ },
	{ SECONDARY_DRIVE_REGISTERS_POFFSET, SECONDARY_AUX_REGISTER_POFFSET,
	  CONTROL_SECONDARY_IRQ }
};

/*
 * Card probe function
 *
 * Just match the manufacturer and podule ID's
 */

int
simide_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct podule_attach_args *pa = (void *)aux;

	if (matchpodule(pa, MANUFACTURER_SIMTEC, PODULE_SIMTEC_IDE, -1) == 0)
		return(0);
	return(1);
}

/*
 * Card attach function
 *
 * Identify the card version and configure any children.
 * Install a shutdown handler to kill interrupts on shutdown
 */

void
simide_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct simide_softc *sc = (void *)self;
	struct podule_attach_args *pa = (void *)aux;
	int status;
	u_int iobase;
	int channel;
	struct simide_channel *scp;
	struct channel_softc *cp;
	irqhandler_t *ihp;

	/* Note the podule number and validate */
	if (pa->pa_podule_number == -1)
		panic("Podule has disappeared !");

	sc->sc_podule_number = pa->pa_podule_number;
	sc->sc_podule = pa->pa_podule;
	podules[sc->sc_podule_number].attached = 1;

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
	sc->sc_tag.bs_cookie = (void *) DRIVE_REGISTER_SPACING_SHIFT;
	sc->sc_tag.bs_rm_2 = simide_bs_rm_2;
	sc->sc_tag.bs_wm_2 = simide_bs_wm_2;
	sc->sc_ctliot = pa->pa_iot;

	/* Obtain bus space handles for all the control registers */
	if (bus_space_map(sc->sc_ctliot, pa->pa_podule->mod_base +
	    CONTROL_REGISTERS_POFFSET, CONTROL_REGISTER_SPACE, 0,
	    &sc->sc_ctlioh))
		panic("%s: Cannot map control registers\n", self->dv_xname);

	/* Install a clean up handler to make sure IRQ's are disabled */
	if (shutdownhook_establish(simide_shutdown, (void *)sc) == NULL)
		panic("%s: Cannot install shutdown handler", self->dv_xname);

	/* Set the interrupt info for this podule */
	sc->sc_podule->irq_addr = pa->pa_podule->mod_base
	    + CONTROL_REGISTERS_POFFSET + (CONTROL_REGISTER_OFFSET << 2);
	sc->sc_podule->irq_mask = STATUS_IRQ;

	sc->sc_ctl_reg = 0;

	status = bus_space_read_1(sc->sc_ctliot, sc->sc_ctlioh,
	    STATUS_REGISTER_OFFSET);

	printf(":");
	/* If any of the bits in STATUS_FAULT are zero then we have a fault. */
	if ((status & STATUS_FAULT) != STATUS_FAULT)
		printf(" card/cable fault (%02x) -", status);

	if (!(status & STATUS_RESET))
		printf(" (reset)");
	if (!(status & STATUS_ADDR_TEST))
		printf(" (addr)");
	if (!(status & STATUS_CS_TEST))
		printf(" (cs)");
	if (!(status & STATUS_RW_TEST))
		printf(" (rw)");

	printf("\n");

	/* Perhaps we should just abort at this point. */
/*	if ((status & STATUS_FAULT) != STATUS_FAULT)
		return;*/

	/*
	 * Enable IDE, Obey IORDY and disabled slow mode
	 */
	sc->sc_ctl_reg |= CONTROL_IDE_ENABLE | CONTROL_IORDY
			| CONTROL_SLOW_MODE_OFF;
	bus_space_write_1(sc->sc_ctliot, sc->sc_ctlioh,
	    CONTROL_REGISTER_OFFSET, sc->sc_ctl_reg);

	/* Fill in wdc and channel infos */
	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA16;
	sc->sc_wdcdev.PIO_cap = 0;
	sc->sc_wdcdev.channels = sc->wdc_chanarray;
	sc->sc_wdcdev.nchannels = 2;
	for (channel = 0 ; channel < 2; channel++) {
		scp = &sc->simide_channels[channel];
		sc->wdc_chanarray[channel] = &scp->wdc_channel;
		cp = &scp->wdc_channel;

		cp->channel = channel;
		cp->wdc = &sc->sc_wdcdev;
		cp->ch_queue = malloc(sizeof(struct channel_queue),
		    M_DEVBUF, M_NOWAIT);
		if (cp->ch_queue == NULL) {
			printf("%s %s channel: can't allocate memory for "
			    "command queue", self->dv_xname,
			    (channel == 0) ? "primary" : "secondary");
			continue;
		}
		cp->cmd_iot = cp->ctl_iot = &sc->sc_tag;
		iobase = pa->pa_podule->mod_base;
		if (bus_space_map(cp->cmd_iot, iobase +
		    simide_info[channel].drive_registers,
		    DRIVE_REGISTERS_SPACE, 0, &cp->cmd_ioh)) 
			continue;
		if (bus_space_map(cp->ctl_iot, iobase +
		    simide_info[channel].aux_register, 4, 0, &cp->ctl_ioh)) {
			bus_space_unmap(cp->cmd_iot, cp->cmd_ioh,
			    DRIVE_REGISTERS_SPACE);
			continue;
		}
		/* Disable interrupts and clear any pending interrupts */
		scp->sc_irqmask = simide_info[channel].irq_mask;
		sc->sc_ctl_reg &= ~scp->sc_irqmask;
		bus_space_write_1(sc->sc_ctliot, sc->sc_ctlioh,
		    CONTROL_REGISTER_OFFSET, sc->sc_ctl_reg);
		wdcattach(cp);
		ihp = &scp->sc_ih;
		ihp->ih_func = simide_intr;
		ihp->ih_arg = scp;
		ihp->ih_level = IPL_BIO;
		ihp->ih_name = "simide";
		ihp->ih_maskaddr = pa->pa_podule->irq_addr;
		ihp->ih_maskbits = scp->sc_irqmask;
		if (irq_claim(sc->sc_podule->interrupt, ihp))
			panic("%s: Cannot claim interrupt %d\n",
			    self->dv_xname, sc->sc_podule->interrupt);
		/* clear any pending interrupts and enable interrupts */
		sc->sc_ctl_reg |= scp->sc_irqmask;
		bus_space_write_1(sc->sc_ctliot, sc->sc_ctlioh,
		    CONTROL_REGISTER_OFFSET, sc->sc_ctl_reg);
	}

}

/*
 * Card shutdown function
 *
 * Called via do_shutdown_hooks() during kernel shutdown.
 * Clear the cards's interrupt mask to stop any podule interrupts.
 */

void
simide_shutdown(arg)
	void *arg;
{
	struct simide_softc *sc = arg;

	sc->sc_ctl_reg &= (CONTROL_PRIMARY_IRQ | CONTROL_SECONDARY_IRQ);

	/* Disable card interrupts */
	bus_space_write_1(sc->sc_ctliot, sc->sc_ctlioh,
	    CONTROL_REGISTER_OFFSET, sc->sc_ctl_reg);
}

/*
 * Podule interrupt handler
 *
 * If the interrupt was from our card pass it on to the wdc interrupt handler
 */
int
simide_intr(arg)
	void *arg;
{
	struct simide_channel *scp = arg;
	irqhandler_t *ihp = &scp->sc_ih;
	volatile u_char *intraddr = (volatile u_char *)ihp->ih_maskaddr;

	/* XXX - not bus space yet - should really be handled by podulebus */
	if ((*intraddr) & ihp->ih_maskbits)
		wdcintr(&scp->wdc_channel);

	return(0);
}
