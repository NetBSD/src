/*	$NetBSD: rapide.c,v 1.9 1998/09/22 00:40:37 mark Exp $	*/

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
 * for the RapIDE podule
 */

/*
 * Thanks to Chris Honey at Raymond Datalink for providing information on
 * addressing the RapIDE podule.
 * RapIDE32 is Copyright (C) 1995,1996 Raymond Datalink. RapIDE32 is
 * manufactured under license by Yellowstone Educational Solutions.
 */

/*
 * At present this driver only supports the Issue 2 RapIDE podule.
 */

/*
 * A small amount of work is required for Issue 1 podule support.
 * The primary differences are the register addresses.
 * Things are eased by the fact that we can identify the card by register
 * the same register on both issues of the podule.
 * Once we kmnow the issue we must change all our addresses accordingly.
 * All the control registers are mapped the same between cards.
 * The interrupt handler needs to take note that the issue 1 card needs
 * the interrupt to be cleared via the interrupt clear register.
 * This means we share addresses for the mapping of the control block and
 * thus the card driver does not need to know about the differences.
 * The differences show up a the controller level.
 * A structure is used to hold the information about the addressing etc.
 * An array of these structures holds the information for the primary and
 * secondary connectors. This needs to be extended to hold this information
 * for both issues. Then the indexing of this structures will utilise the
 * card version number.
 *
 * Opps just noticed a mistake. The interrupt request register is different
 * between cards so the card level attach routine will need to consider this.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <machine/irqhandler.h>
#include <machine/io.h>
#include <machine/bus.h>
#include <machine/bootconfig.h>
#include <arm32/iomd/iomdreg.h>
#include <arm32/podulebus/podulebus.h>
#include <arm32/podulebus/podules.h>
#include <arm32/podulebus/rapidereg.h>

#include <dev/ic/wdcvar.h>


/*
 * RapIDE podule device.
 *
 * This probes and attaches the top level RapIDE device to the podulebus.
 * It then configures any children of the RapIDE device.
 * The attach args specify whether it is configuring the primary or
 * secondary channel.
 * The children are expected to be wdc devices using rapide attachments.
 */

/*
 * RapIDE card softc structure.
 *
 * Contains the device node, podule information and global information
 * required by the driver such as the card version and the interrupt mask.
 */

struct rapide_softc {
	struct device		sc_dev;			/* device node */
	podule_t 		*sc_podule;		/* Our podule info */
	int 			sc_podule_number;	/* Our podule number */
	int			sc_intr_enable_mask;	/* Global intr mask */
	int			sc_version;		/* Card version */
	bus_space_tag_t		sc_ctliot;		/* Bus tag */
	bus_space_handle_t	sc_ctlioh;		/* control handler */
};

int	rapide_probe	__P((struct device *, struct cfdata *, void *));
void	rapide_attach	__P((struct device *, struct device *, void *));
void	rapide_shutdown	__P((void *arg));

struct cfattach rapide_ca = {
	sizeof(struct rapide_softc), rapide_probe, rapide_attach
};

/*
 * Attach arguments for child devices.
 * Pass the podule details, the parent softc and the channel
 */

struct rapide_attach_args {
	struct podule_attach_args *ra_pa;	/* podule info */
	struct rapide_softc *ra_softc;		/* parent softc */
	bus_space_tag_t ra_iot;			/* bus space tag */
	u_int ra_iobase;			/* I/O base address */
	int ra_channel;      			/* IDE channel */
};

/*
 * We have a private bus space tag.
 * This is created by copying the podulebus tag and then replacing
 * a couple of the transfer functions.
 */

static struct bus_space rapide_bs_tag;

bs_rm_4_proto(rapide);
bs_wm_4_proto(rapide);

/* Print function used during child config */

int
rapide_print(aux, name)
	void *aux;
	const char *name;
{
	struct rapide_attach_args *ra = aux;

	if (!name)
		printf(": %s channel", (ra->ra_channel == 0) ? "primary" : "secondary");

	return(QUIET);
}

/*
 * Card probe function
 *
 * Just match the manufacturer and podule ID's
 */

int
rapide_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct podule_attach_args *pa = (void *)aux;

	if (matchpodule(pa, MANUFACTURER_YES, PODULE_YES_RAPIDE, -1) == 0)
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
rapide_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct rapide_softc *sc = (void *)self;
	struct podule_attach_args *pa = (void *)aux;
	struct rapide_attach_args ra;
	bus_space_tag_t iot;
	bus_space_handle_t ctlioh;

	/* Note the podule number and validate */
	if (pa->pa_podule_number == -1)
		panic("Podule has disappeared !");

	sc->sc_podule_number = pa->pa_podule_number;
	sc->sc_podule = pa->pa_podule;
	podules[sc->sc_podule_number].attached = 1;

	set_easi_cycle_type(sc->sc_podule_number, EASI_CYCLE_TYPE_C);

	/*
	 * Duplicate the podule bus space tag and provide alternative
	 * bus_space_read_multi_4() and bus_space_write_multi_4()
	 * functions.
	 */
	rapide_bs_tag = *pa->pa_iot;
	rapide_bs_tag.bs_rm_4 = rapide_bs_rm_4;
	rapide_bs_tag.bs_wm_4 = rapide_bs_wm_4;
	sc->sc_ctliot = iot = &rapide_bs_tag;

	if (bus_space_map(iot, pa->pa_podule->easi_base +
	    CONTROL_REGISTERS_OFFSET, CONTROL_REGISTER_SPACE, 0, &ctlioh))
		panic("%s: Cannot map control registers\n", self->dv_xname);

	sc->sc_ctlioh = ctlioh;
	sc->sc_version = bus_space_read_1(iot, ctlioh, VERSION_REGISTER_OFFSET) & VERSION_REGISTER_MASK;
/*	bus_space_unmap(iot, ctl_ioh, CONTROL_REGISTER_SPACE);*/

	printf(" Issue %d\n", sc->sc_version + 1);
	if (sc->sc_version != VERSION_2_ID)
		return;

	if (shutdownhook_establish(rapide_shutdown, (void *)sc) == NULL)
		panic("%s: Cannot install shutdown handler", self->dv_xname);

	/* Set the interrupt info for this podule */
	sc->sc_podule->irq_addr = pa->pa_podule->easi_base
	    + CONTROL_REGISTERS_OFFSET + IRQ_REQUEST_REGISTER_BYTE_OFFSET;
	sc->sc_podule->irq_mask = IRQ_MASK;

	/* Configure the children */
	sc->sc_intr_enable_mask = 0;
	ra.ra_softc = sc;
	ra.ra_pa = pa;
	ra.ra_iot = iot;
	ra.ra_iobase = pa->pa_podule->easi_base;

	ra.ra_channel = 0;
	config_found_sm(self, &ra, rapide_print, NULL);

	ra.ra_channel = 1;
	config_found_sm(self, &ra, rapide_print, NULL);
}

/*
 * Card shutdown function
 *
 * Called via do_shutdown_hooks() during kernel shutdown.
 * Clear the cards's interrupt mask to stop any podule interrupts.
 */

void
rapide_shutdown(arg)
	void *arg;
{
	struct rapide_softc *sc = arg;

	/* Disable card interrupts */
	bus_space_write_1(sc->sc_ctliot, sc->sc_ctlioh,
	    IRQ_MASK_REGISTER_OFFSET, 0);
}

/*
 * RapIDE probe and attach code for the wdc device.
 *
 * This provides a different pair of probe and attach functions
 * for attaching the wdc device (mainbus/wd.c) to the RapIDE card.
 */

struct wdc_rapide_softc {
	struct wdc_softc	sc_wdcdev;		/* Device node */
	struct wdc_attachment_data	sc_ad;		/* Attachment data */
	irqhandler_t		sc_ih;			/* interrupt handler */
	int			sc_irqmask;		/* IRQ mask for this channel */
	int			sc_channel;		/* channel number */
	int			sc_pio_mode[2];		/* PIO mode */
	struct rapide_softc	*sc_softc;		/* pointer to parent */
};

int	wdc_rapide_probe	__P((struct device *, struct cfdata *, void *));
void	wdc_rapide_attach	__P((struct device *, struct device *, void *));
int	wdc_rapide_intr		__P((void *));

struct cfattach wdc_rapide_ca = {
	sizeof(struct wdc_rapide_softc), wdc_rapide_probe, wdc_rapide_attach
};

/*
 * Create an array of address structures. These define the addresses and
 * masks needed for the different channels for the card.
 *
 * XXX - Needs some work for issue 1 cards.
 */

struct {
	u_int registers;
	u_int aux_register;
	u_int data_register;
	u_int irq_mask;
} rapide_info[] = {
	{ PRIMARY_DRIVE_REGISTERS_OFFSET, PRIMARY_AUX_REGISTER_OFFSET,
	     PRIMARY_DATA_REGISTER_OFFSET, PRIMARY_IRQ_MASK },
	{ SECONDARY_DRIVE_REGISTERS_OFFSET, SECONDARY_AUX_REGISTER_OFFSET,
	    SECONDARY_DATA_REGISTER_OFFSET, SECONDARY_IRQ_MASK }
};

/*
 * Controller probe function
 *
 * Map all the required I/O space for this channel, make sure interrupts
 * are disabled and probe the bus.
 */

int
wdc_rapide_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct rapide_attach_args *ra = (void *)aux;
	struct rapide_softc *sc;
	struct wdc_attachment_data ad;
	int result = 0;
	
	bzero(&ad, sizeof ad);
	sc = ra->ra_softc;

#ifdef DIAGNOSTIC
	if (ra->ra_channel < 0 || ra->ra_channel > 1)
		return(0);
#endif /* DIAGNOSTIC */

	ad.iot = ra->ra_iot;
	if (bus_space_map(ad.iot, ra->ra_iobase +
	    rapide_info[ra->ra_channel].registers, DRIVE_REGISTERS_SPACE,
	    0, &ad.ioh))
		return(0);

	ad.auxiot = ra->ra_iot;
	if (bus_space_map(ad.auxiot, ra->ra_iobase +
	    rapide_info[ra->ra_channel].aux_register, 4, 0, &ad.auxioh)) {
		bus_space_unmap(ad.iot, ad.ioh, DRIVE_REGISTERS_SPACE);
		return(0);
	}

	ad.data32iot = ra->ra_iot;
	ad.cap |= WDC_CAPABILITY_DATA32;
	if (bus_space_map(ad.data32iot, ra->ra_iobase +
	    rapide_info[ra->ra_channel].data_register, 4, 0, &ad.data32ioh)) {
		bus_space_unmap(ad.iot, ad.ioh, DRIVE_REGISTERS_SPACE);
		bus_space_unmap(ad.auxiot, ad.auxioh, 4);
		return(0);
	}

	/* Disable interrupts and clear any pending interrupts */
	sc->sc_intr_enable_mask &= ~rapide_info[ra->ra_channel].irq_mask;

	bus_space_write_1(sc->sc_ctliot, sc->sc_ctlioh, IRQ_MASK_REGISTER_OFFSET,
	    sc->sc_intr_enable_mask);

	/* XXX - Issue 1 cards will need to clear any pending interrupts */
	result = wdcprobe(&ad);

	bus_space_unmap(ad.iot, ad.ioh, DRIVE_REGISTERS_SPACE);
	bus_space_unmap(ad.auxiot, ad.auxioh, 4);
	bus_space_unmap(ad.data32iot, ad.data32ioh, 4);
	return(result);
}

/*
 * Controller attach function
 *
 * Map all the required I/O space for this channel, disble interrupts
 * and attach the controller. The generic attach will probe and attach
 * any devices.
 * Install an interrupt handler and we are ready to rock.
 */    

void
wdc_rapide_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct wdc_rapide_softc *wdc = (void *)self;
	struct rapide_attach_args *ra = (void *)aux;
	struct podule_attach_args *pa = ra->ra_pa;
	struct rapide_softc *sc;

	printf("\n");

	wdc->sc_channel = ra->ra_channel;
	wdc->sc_irqmask = rapide_info[ra->ra_channel].irq_mask;

	wdc->sc_ad.iot = ra->ra_iot;
	wdc->sc_ad.auxiot = ra->ra_iot;
	wdc->sc_ad.data32iot = ra->ra_iot;
	wdc->sc_ad.cap |= WDC_CAPABILITY_DATA32;
	sc = ra->ra_softc;

	if (bus_space_map(wdc->sc_ad.iot, ra->ra_iobase
	    + rapide_info[ra->ra_channel].registers,
	    DRIVE_REGISTERS_SPACE, 0, &wdc->sc_ad.ioh))
		panic("%s: Cannot map drive registers\n", self->dv_xname);

	if (bus_space_map(wdc->sc_ad.auxiot, ra->ra_iobase +
	    rapide_info[ra->ra_channel].aux_register, 4, 0, &wdc->sc_ad.auxioh))
		panic("%s: Cannot map auxilary register\n", self->dv_xname);

	if (bus_space_map(wdc->sc_ad.data32iot, ra->ra_iobase +
	    rapide_info[ra->ra_channel].data_register, 4, 0, &wdc->sc_ad.data32ioh))
		panic("%s: Cannot map data register\n", self->dv_xname);

	/* Disable interrupts and clear any pending interrupts */
	sc->sc_intr_enable_mask &= ~wdc->sc_irqmask;

	bus_space_write_1(sc->sc_ctliot, sc->sc_ctlioh,
	    IRQ_MASK_REGISTER_OFFSET, sc->sc_intr_enable_mask);

	/* XXX - Issue 1 cards will need to clear any pending interrupts */
	wdcattach(&wdc->sc_wdcdev, &wdc->sc_ad);

  	wdc->sc_ih.ih_func = wdc_rapide_intr;
   	wdc->sc_ih.ih_arg = wdc;
   	wdc->sc_ih.ih_level = IPL_BIO;
   	wdc->sc_ih.ih_name = "rapide";
	wdc->sc_ih.ih_maskaddr = pa->pa_podule->irq_addr;
	wdc->sc_ih.ih_maskbits = wdc->sc_irqmask;

	if (irq_claim(sc->sc_podule->interrupt, &wdc->sc_ih))
		panic("%s: Cannot claim interrupt %d\n", self->dv_xname,
		    sc->sc_podule->interrupt);

	/* clear any pending interrupts and enable interrupts */
	sc->sc_intr_enable_mask |= wdc->sc_irqmask;

	bus_space_write_1(sc->sc_ctliot, sc->sc_ctlioh,
	    IRQ_MASK_REGISTER_OFFSET, sc->sc_intr_enable_mask);

	/* XXX - Issue 1 cards will need to clear any pending interrupts */
}

/*
 * Podule interrupt handler
 *
 * If the interrupt was from our card pass it on to the wdc interrupt handler
 */

int
wdc_rapide_intr(arg)
	void *arg;
{
	struct wdc_rapide_softc *wdc = arg;
	volatile u_char *intraddr = (volatile u_char *)wdc->sc_ih.ih_maskaddr;

	/* XXX - Issue 1 cards will need to clear the interrupt */

	/* XXX - not bus space yet - should really be handled by podulebus */
	if ((*intraddr) & wdc->sc_ih.ih_maskbits)
		wdcintr(arg);

	return(0);
}
