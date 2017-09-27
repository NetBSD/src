/*	$NetBSD: rapide.c,v 1.30.28.2 2017/09/27 07:19:33 jdolecek Exp $	*/

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
 * for both issues. Then the indexing of these structures will use the
 * card version number.
 *
 * Opps just noticed a mistake. The interrupt request register is different
 * between cards so the card level attach routine will need to consider this.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rapide.c,v 1.30.28.2 2017/09/27 07:19:33 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/bus.h>

#include <machine/intr.h>
#include <machine/io.h>
#include <machine/bootconfig.h>
#include <arm/iomd/iomdreg.h>
#include <arm/iomd/iomdvar.h>
#include <acorn32/podulebus/podulebus.h>
#include <acorn32/podulebus/rapidereg.h>

#include <dev/ata/atavar.h>
#include <dev/ic/wdcreg.h>
#include <dev/ic/wdcvar.h>
#include <dev/podulebus/podules.h>


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
	struct wdc_softc	sc_wdcdev;	/* common wdc definitions */
	struct ata_channel	*sc_chanarray[2]; /* channels definition */
	podule_t 		*sc_podule;		/* Our podule info */
	int 			sc_podule_number;	/* Our podule number */
	int			sc_intr_enable_mask;	/* Global intr mask */
	int			sc_version;		/* Card version */
	bus_space_tag_t		sc_ctliot;		/* Bus tag */
	bus_space_handle_t	sc_ctlioh;		/* control handler */
	struct rapide_channel {
		struct ata_channel rc_channel;	/* generic part */
		irqhandler_t	rc_ih;			/* interrupt handler */
		int		rc_irqmask;	/* IRQ mask for this channel */
	} rapide_channels[2];
	struct wdc_regs sc_wdc_regs[2];
};

int	rapide_probe	(device_t, struct cfdata *, void *);
void	rapide_attach	(device_t, device_t, void *);
void	rapide_shutdown	(void *arg);
int	rapide_intr	(void *);

CFATTACH_DECL_NEW(rapide, sizeof(struct rapide_softc),
    rapide_probe, rapide_attach, NULL, NULL);

/*
 * We have a private bus space tag.
 * This is created by copying the podulebus tag and then replacing
 * a couple of the transfer functions.
 */

static struct bus_space rapide_bs_tag;

bs_rm_4_proto(rapide);
bs_wm_4_proto(rapide);

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
 * Card probe function
 *
 * Just match the manufacturer and podule ID's
 */

int
rapide_probe(device_t parent, cfdata_t cf, void *aux)
{
	struct podule_attach_args *pa = (void *)aux;

	return (pa->pa_product == PODULE_RAPIDE);
}

/*
 * Card attach function
 *
 * Identify the card version and configure any children.
 * Install a shutdown handler to kill interrupts on shutdown
 */

void
rapide_attach(device_t parent, device_t self, void *aux)
{
	struct rapide_softc *sc = device_private(self);
	struct podule_attach_args *pa = (void *)aux;
	bus_space_tag_t iot;
	bus_space_handle_t ctlioh;
	u_int iobase;
	int channel, i;
	struct rapide_channel *rcp;
	struct ata_channel *cp;
	struct wdc_regs *wdr;
	irqhandler_t *ihp;

	/* Note the podule number and validate */
	if (pa->pa_podule_number == -1)
		panic("Podule has disappeared !");

	sc->sc_wdcdev.sc_atac.atac_dev = self;
	sc->sc_podule_number = pa->pa_podule_number;
	sc->sc_podule = pa->pa_podule;
	podules[sc->sc_podule_number].attached = 1;

	sc->sc_wdcdev.regs = sc->sc_wdc_regs;

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
		panic("%s: Cannot map control registers", device_xname(self));

	sc->sc_ctlioh = ctlioh;
	sc->sc_version = bus_space_read_1(iot, ctlioh, VERSION_REGISTER_OFFSET) & VERSION_REGISTER_MASK;
/*	bus_space_unmap(iot, ctl_ioh, CONTROL_REGISTER_SPACE);*/

	aprint_normal(": Issue %d\n", sc->sc_version + 1);
	if (sc->sc_version != VERSION_2_ID)
		return;

	if (shutdownhook_establish(rapide_shutdown, (void *)sc) == NULL)
		panic("%s: Cannot install shutdown handler", device_xname(self));

	/* Set the interrupt info for this podule */
	sc->sc_podule->irq_addr = pa->pa_podule->easi_base
	    + CONTROL_REGISTERS_OFFSET + IRQ_REQUEST_REGISTER_BYTE_OFFSET;
	sc->sc_podule->irq_mask = IRQ_MASK;

	iobase = pa->pa_podule->easi_base;

	/* Fill in wdc and channel infos */
	sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DATA32;
	sc->sc_wdcdev.sc_atac.atac_pio_cap = 0;
	sc->sc_wdcdev.sc_atac.atac_channels = sc->sc_chanarray;
	sc->sc_wdcdev.sc_atac.atac_nchannels = 2;
	sc->sc_wdcdev.wdc_maxdrives = 2;
	for (channel = 0 ; channel < 2; channel++) {
		rcp = &sc->rapide_channels[channel];
		sc->sc_chanarray[channel] = &rcp->rc_channel;
		cp = &rcp->rc_channel;
		wdr = &sc->sc_wdc_regs[channel];

		cp->ch_channel = channel;
		cp->ch_atac = &sc->sc_wdcdev.sc_atac;
		cp->ch_queue = ata_queue_alloc(1);
		wdr->cmd_iot = iot;
		wdr->ctl_iot = iot;
		wdr->data32iot = iot;

		if (bus_space_map(iot, iobase + rapide_info[channel].registers,
		    DRIVE_REGISTERS_SPACE, 0, &wdr->cmd_baseioh))
			continue;
		for (i = 0; i < WDC_NREG; i++) {
			if (bus_space_subregion(wdr->cmd_iot, wdr->cmd_baseioh,
				i, i == 0 ? 4 : 1, &wdr->cmd_iohs[i]) != 0) {
				bus_space_unmap(iot, wdr->cmd_baseioh,
				    DRIVE_REGISTERS_SPACE);
				continue;
			}
		}
		wdc_init_shadow_regs(wdr);
		if (bus_space_map(iot, iobase +
		    rapide_info[channel].aux_register, 4, 0, &wdr->ctl_ioh)) {
			bus_space_unmap(iot, wdr->cmd_baseioh,
			   DRIVE_REGISTERS_SPACE);
			continue;
		}
		if (bus_space_map(iot, iobase +
		    rapide_info[channel].data_register, 4, 0, &wdr->data32ioh)) {
			bus_space_unmap(iot, wdr->cmd_baseioh,
			   DRIVE_REGISTERS_SPACE);
			bus_space_unmap(iot, wdr->ctl_ioh, 4);
			continue;
		}
		/* Disable interrupts and clear any pending interrupts */
		rcp->rc_irqmask = rapide_info[channel].irq_mask;
		sc->sc_intr_enable_mask &= ~rcp->rc_irqmask;
		bus_space_write_1(iot, sc->sc_ctlioh, IRQ_MASK_REGISTER_OFFSET,
		    sc->sc_intr_enable_mask);
		/* XXX - Issue 1 cards will need to clear any pending interrupts */
		ihp = &rcp->rc_ih;
		ihp->ih_func = rapide_intr;
		ihp->ih_arg = rcp;
		ihp->ih_level = IPL_BIO;
		ihp->ih_name = "rapide";
		ihp->ih_maskaddr = pa->pa_podule->irq_addr;
		ihp->ih_maskbits = rcp->rc_irqmask;
		if (irq_claim(sc->sc_podule->interrupt, ihp))
			panic("%s: Cannot claim interrupt %d",
			    device_xname(self), sc->sc_podule->interrupt);
		/* clear any pending interrupts and enable interrupts */
		sc->sc_intr_enable_mask |= rcp->rc_irqmask;
		bus_space_write_1(iot, sc->sc_ctlioh,
		    IRQ_MASK_REGISTER_OFFSET, sc->sc_intr_enable_mask);
		/* XXX - Issue 1 cards will need to clear any pending interrupts */
		wdcattach(cp);
	}
}

/*
 * Card shutdown function
 *
 * Called via do_shutdown_hooks() during kernel shutdown.
 * Clear the cards's interrupt mask to stop any podule interrupts.
 */

void
rapide_shutdown(void *arg)
{
	struct rapide_softc *sc = arg;

	/* Disable card interrupts */
	bus_space_write_1(sc->sc_ctliot, sc->sc_ctlioh,
	    IRQ_MASK_REGISTER_OFFSET, 0);
}

/*
 * Podule interrupt handler
 *
 * If the interrupt was from our card pass it on to the wdc interrupt handler
 */

int
rapide_intr(void *arg)
{
	struct rapide_channel *rcp = arg;
	irqhandler_t *ihp = &rcp->rc_ih;
	volatile u_char *intraddr = (volatile u_char *)ihp->ih_maskaddr;

	/* XXX - Issue 1 cards will need to clear the interrupt */

	/* XXX - not bus space yet - should really be handled by podulebus */
	if ((*intraddr) & ihp->ih_maskbits)
		wdcintr(&rcp->rc_channel);

	return(0);
}
