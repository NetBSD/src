/*	$NetBSD: rapide.c,v 1.2 1997/03/15 18:09:40 is Exp $	*/

/*
 * Copyright (c) 1997 Mark Brinicombe
 * Copyright (c) 1997 Causality Limited
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
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/syslog.h>
#include <sys/proc.h>

#include <vm/vm.h>

#include <machine/irqhandler.h>
#include <machine/katelib.h>
#include <machine/io.h>
#include <machine/iomd.h>
#include <machine/bus.h>
#include <machine/bootconfig.h>
#include <arm32/podulebus/podulebus.h>
#include <arm32/podulebus/podules.h>
#include <arm32/podulebus/rapidereg.h>

#include <arm32/mainbus/wdreg.h>
#include <arm32/mainbus/wdvar.h>

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
	bus_space_tag_t		sc_iot;			/* Bus tag */
	bus_space_handle_t	sc_ctl_ioh;		/* control handler */
};

int	rapide_probe	__P((struct device *, void *, void *));
void	rapide_attach	__P((struct device *, struct device *, void *));
void	rapide_shutdown	__P((void *arg));

struct cfattach rapide_ca = {
	sizeof(struct rapide_softc), rapide_probe, rapide_attach
};

struct cfdriver rapide_cd = {
	NULL, "rapide", DV_DULL, NULL, 0
};

/*
 * Attach arguments for child devices.
 * Pass the podule details, the parent softc and the channel
 */

struct rapide_attach_args {
	struct podule_attach_args *ra_pa;	/* podule info */
	struct rapide_softc *ra_softc;		/* parent softc */
	bus_space_tag_t ra_iot;			/* bus space tag */
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

	return(UNCONF);
}

/*
 * Card probe function
 *
 * Just match the manufacturer and podule ID's
 */

int
rapide_probe(parent, match, aux)
	struct device *parent;
	void *match, *aux;
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
	bus_space_handle_t ctl_ioh;
	int dummy;

	/* Note the podule number and validate */

	if (pa->pa_podule_number == -1)
		panic("Podule has disappeared !");

	sc->sc_podule_number = pa->pa_podule_number;
	sc->sc_podule = pa->pa_podule;
	podules[sc->sc_podule_number].attached = 1;

	set_easi_cycle_type(sc->sc_podule_number, EASI_CYCLE_TYPE_C);

	/*
	 * Duplicate the podule bus space tag and provide alternative
	 * bus_space_read_multiple_4() and bus_space_write_multiple_4()
	 * functions.
	 */

	rapide_bs_tag = *pa->pa_iot;
	rapide_bs_tag.bs_rm_4 = rapide_rm_4;
	rapide_bs_tag.bs_wm_4 = rapide_wm_4;
	sc->sc_iot = iot = &rapide_bs_tag;

	if (bus_space_map(iot, pa->pa_podule->easi_base +
	    CONTROL_REGISTERS_OFFSET, CONTROL_REGISTER_SPACE, 0, &ctl_ioh))
		panic("%s: Cannot map control registers\n", self->dv_xname);

	sc->sc_ctl_ioh = ctl_ioh;
	sc->sc_version = bus_space_read_1(iot, ctl_ioh, VERSION_REGISTER_OFFSET) & VERSION_REGISTER_MASK;
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

	printf("%s: stopped\n", sc->sc_dev.dv_xname);

	/* Disable card interrupts */
	bus_space_write_1(sc->sc_iot, sc->sc_ctl_ioh,
	    IRQ_MASK_REGISTER_OFFSET, 0);
}

/*
 * RapIDE probe and attach code for the wdc device.
 *
 * This provides a different pair of probe and attach functions
 * for attaching the wdc device (mainbus/wd.c) to the RapIDE card.
 */

struct rapwdc_softc {
	struct wdc_softc	sc_wdc;			/* Device node */
	irqhandler_t		sc_ih;			/* interrupt handler */
	podule_t 		*sc_podule;		/* Our podule */
	int 			sc_podule_number;	/* Our podule number */
	bus_space_tag_t		sc_iot;			/* Bus space tag */
	bus_space_handle_t	sc_ioh;			/* handle for registers */
	bus_space_handle_t	sc_aux_ioh;		/* handle for aux registers */
	bus_space_handle_t	sc_ctl_ioh;		/* handle for control space */
	bus_space_handle_t	sc_data_ioh;		/* handle for 32bit data */
	int			sc_irqmask;		/* IRQ mask for this channel */
	int			sc_channel;		/* channel number */
	int			sc_pio_mode[2];		/* PIO mode */
	struct rapide_softc	*sc_softc;		/* pointer to parent */
};

int	wdc_rapide_probe	__P((struct device *, void *, void *));
void	wdc_rapide_attach	__P((struct device *, struct device *, void *));
int	wdc_rapide_intr		__P((void *));

extern int wdcintr __P((void *));

struct cfattach wdc_rapide_ca = {
	sizeof(struct rapwdc_softc), wdc_rapide_probe, wdc_rapide_attach
};

extern struct cfdriver wdc_cd;

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
wdc_rapide_probe(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct rapide_attach_args *ra = (void *)aux;
	struct podule_attach_args *pa = ra->ra_pa;
	struct rapide_softc *sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_space_handle_t aux_ioh;
	bus_space_handle_t ctl_ioh;
	bus_space_handle_t data_ioh;
	int rv;

	iot = ra->ra_iot;
	sc = ra->ra_softc;

#ifdef DIAGNOSTIC
	if (ra->ra_channel < 0 || ra->ra_channel > 1)
		return(0);
#endif /* DIAGNOSTIC */

	if (bus_space_map(iot, pa->pa_podule->easi_base + 
	    rapide_info[ra->ra_channel].registers, DRIVE_REGISTERS_SPACE, 0, &ioh))
		return(0);

	if (bus_space_map(iot, pa->pa_podule->easi_base +
	    rapide_info[ra->ra_channel].aux_register, 4, 0, &aux_ioh)) {
		bus_space_unmap(iot, ioh, DRIVE_REGISTERS_SPACE);
		return(0);
	}

	if (bus_space_map(iot, pa->pa_podule->easi_base +
	    rapide_info[ra->ra_channel].data_register, 4, 0, &data_ioh)) {
		bus_space_unmap(iot, ioh, DRIVE_REGISTERS_SPACE);
		bus_space_unmap(iot, aux_ioh, 4);
		return(0);
	}

	if (bus_space_map(iot, pa->pa_podule->easi_base +
	    CONTROL_REGISTERS_OFFSET, CONTROL_REGISTER_SPACE, 0, &ctl_ioh)) {
		bus_space_unmap(iot, ioh, DRIVE_REGISTERS_SPACE);
		bus_space_unmap(iot, aux_ioh, 4);
		bus_space_unmap(iot, data_ioh, 4);
		return(0);
	}

	/* Disable interrupts and clear any pending interrupts */

	sc->sc_intr_enable_mask &= ~rapide_info[ra->ra_channel].irq_mask;

	bus_space_write_1(iot, ctl_ioh, IRQ_MASK_REGISTER_OFFSET,
	    sc->sc_intr_enable_mask);

	/* XXX - Issue 1 cards will need to clear any pending interrupts */

	rv = wdcprobe_internal(iot, ioh, aux_ioh, ioh, data_ioh, "rapide_probe");	/* XXX */

	bus_space_unmap(iot, ioh, DRIVE_REGISTERS_SPACE);
	bus_space_unmap(iot, aux_ioh, 4);
	bus_space_unmap(iot, ctl_ioh, CONTROL_REGISTER_SPACE);
	bus_space_unmap(iot, data_ioh, 4);
	return(rv);
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
	struct rapwdc_softc *wdc = (void *)self;
	struct rapide_attach_args *ra = (void *)aux;
	struct podule_attach_args *pa = ra->ra_pa;
	struct rapide_softc *sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_space_handle_t aux_ioh;
	bus_space_handle_t ctl_ioh;
	bus_space_handle_t data_ioh;

	/* Note the podule number and validate */

	wdc->sc_podule_number = pa->pa_podule_number;
	wdc->sc_podule = pa->pa_podule;

	wdc->sc_channel = ra->ra_channel;
	wdc->sc_irqmask = rapide_info[ra->ra_channel].irq_mask;

	wdc->sc_iot = iot = ra->ra_iot;
	sc = ra->ra_softc;

	if (bus_space_map(iot, pa->pa_podule->easi_base
	    + rapide_info[ra->ra_channel].registers,
	    DRIVE_REGISTERS_SPACE, 0, &ioh))
		panic("%s: Cannot map drive registers\n", self->dv_xname);

	if (bus_space_map(iot, pa->pa_podule->easi_base +
	    rapide_info[ra->ra_channel].aux_register, 4, 0, &aux_ioh))
		panic("%s: Cannot map auxilary register\n", self->dv_xname);

	if (bus_space_map(iot, pa->pa_podule->easi_base +
	    rapide_info[ra->ra_channel].data_register, 4, 0, &data_ioh))
		panic("%s: Cannot map data register\n", self->dv_xname);

	if (bus_space_map(iot, pa->pa_podule->easi_base +
	    CONTROL_REGISTERS_OFFSET, CONTROL_REGISTER_SPACE, 0, &ctl_ioh))
    		panic("%s: Cannot map control registers\n", self->dv_xname);

	wdc->sc_ioh = ioh;
	wdc->sc_aux_ioh = aux_ioh;
	wdc->sc_ctl_ioh = ctl_ioh;
	wdc->sc_data_ioh = data_ioh;
	wdc->sc_wdc.sc_flags = WDCF_32BIT;	/* flag 32 bit data xfers */

	/* Disable interrupts and clear any pending interrupts */

	sc->sc_intr_enable_mask &= ~wdc->sc_irqmask;

	bus_space_write_1(iot, ctl_ioh, IRQ_MASK_REGISTER_OFFSET,
	    sc->sc_intr_enable_mask);

	/* XXX - Issue 1 cards will need to clear any pending interrupts */

	wdcattach_internal((struct wdc_softc *)wdc, iot, ioh, aux_ioh, ioh, data_ioh, -1);

  	wdc->sc_ih.ih_func = wdc_rapide_intr;
   	wdc->sc_ih.ih_arg = wdc;
   	wdc->sc_ih.ih_level = IPL_BIO;
   	wdc->sc_ih.ih_name = "rapide";
	wdc->sc_ih.ih_maskaddr = wdc->sc_podule->irq_addr;
	wdc->sc_ih.ih_maskbits = wdc->sc_irqmask;

	if (irq_claim(IRQ_PODULE, &wdc->sc_ih))
		panic("Cannot claim IRQ %d for %s\n", IRQ_PODULE, self->dv_xname);

	/* clear any pending interrupts and enable interrupts */

	sc->sc_intr_enable_mask |= wdc->sc_irqmask;

	bus_space_write_1(iot, ctl_ioh, IRQ_MASK_REGISTER_OFFSET,
	    sc->sc_intr_enable_mask);

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
	struct rapwdc_softc *wdc = arg;

	/* XXX - Issue 1 cards will need to clear the interrupt */

	if (ReadByte(wdc->sc_ih.ih_maskaddr) & wdc->sc_ih.ih_maskbits)
		wdcintr(arg);

	return(0);
}
