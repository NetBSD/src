/*	$NetBSD: icside.c,v 1.2 1997/03/15 18:09:33 is Exp $	*/

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
 * Probe and attach functions to use generic IDE driver for the ICS IDE podule
 */

/*
 * Thanks to Ian Copestake and David Basildon for providing interrupt enable
 * information
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
#include <machine/io.h>
#include <machine/bus.h>
#include <machine/katelib.h>
#include <arm32/podulebus/podulebus.h>
#include <arm32/podulebus/podules.h>
#include <arm32/podulebus/icsidereg.h>

#include <arm32/mainbus/wdreg.h>
#include <arm32/mainbus/wdvar.h>

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
	struct device		sc_dev;
	podule_t 		*sc_podule;		/* Our podule */
	int 			sc_podule_number;	/* Our podule number */
};

int	icside_probe	__P((struct device *, void *, void *));
void	icside_attach	__P((struct device *, struct device *, void *));

struct cfattach icside_ca = {
	sizeof(struct icside_softc), icside_probe, icside_attach
};

struct cfdriver icside_cd = {
	NULL, "icside", DV_DULL, NULL, 0
};

/* Print function used during child config */

int
icside_print(aux, name)
	void *aux;
	const char *name;
{
	struct podule_attach_args *pa = aux;

	/* XXXX print flags */
	return (QUIET);
}

/*
 * Card probe function
 *
 * Just match the manufacturer and podule ID's
 */

int
icside_probe(parent, match, aux)
	struct device *parent;
	void *match, *aux;
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
	int dummy;

	/* Note the podule number and validate */

	if (pa->pa_podule_number == -1)
		panic("Podule has disappeared !");

	sc->sc_podule_number = pa->pa_podule_number;
	sc->sc_podule = pa->pa_podule;
	podules[sc->sc_podule_number].attached = 1;
	printf("\n");

	config_found_sm(self, aux, icside_print, NULL);
}

/*
 * ICS IDE probe and attach code for the wdc device.
 *
 * This provides a different pair of probe and attach functions
 * for attaching the wdc device (mainbus/wd.c) to the ICS IDE card.
 */

struct icswdc_softc {
	struct wdc_softc	sc_wdc;
	irqhandler_t		sc_ih;
	podule_t 		*sc_podule;		/* Our podule */
	int 			sc_podule_number;	/* Our podule number */
	bus_space_tag_t		sc_iot;			/* Bus space tag */
	bus_space_handle_t	sc_irq_ioh;		/* handle for IRQ */
};

int	wdc_ics_probe	__P((struct device *, void *, void *));
void	wdc_ics_attach	__P((struct device *, struct device *, void *));
int	wdc_ics_intr	__P((void *));
void	wdc_ics_inten	__P((struct wdc_softc *wdc, int enable));

extern int wdcintr __P((void *));

struct cfattach wdc_ics_ca = {
	sizeof(struct icswdc_softc), wdc_ics_probe, wdc_ics_attach
};

extern struct cfdriver wdc_cd;

extern struct bus_space icside_bs_tag;

static int intr_expected = 0;

/*
 * Controller probe function
 *
 * Map all the required I/O space for this channel, make sure interrupts
 * are disabled and probe the bus.
 */

int
wdc_ics_probe(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct podule_attach_args *pa = (void *)aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_space_handle_t aux_ioh;
	bus_space_handle_t irq_ioh;
	int rv;
	int dummy;

	iot = &icside_bs_tag;

	if (bus_space_map(iot, pa->pa_podule->mod_base + 
	    DRIVE_REGISTERS_OFFSET, DRIVE_REGISTER_SPACE, 0, &ioh))
		return(0);

	if (bus_space_map(iot, pa->pa_podule->mod_base +
	    AUX_STATUS_REGISTER_OFFSET, 4, 0, &aux_ioh)) {
		bus_space_unmap(iot, ioh, DRIVE_REGISTER_SPACE);
		return(0);
	}

	if (bus_space_map(iot, pa->pa_podule->mod_base +
	    IRQ_ENABLE_REGISTER_OFFSET, 4, 0, &irq_ioh)) {
		bus_space_unmap(iot, ioh, DRIVE_REGISTER_SPACE);
		bus_space_unmap(iot, aux_ioh, 4);
		return(0);
	}

	/* Disable interrupts */
	dummy = bus_space_read_1(iot, irq_ioh, 0);

	rv = wdcprobe_internal(iot, ioh, aux_ioh, ioh, -1, "icside_probe");	/* XXX */

	/* Disable interrupts */
	dummy = bus_space_read_1(iot, irq_ioh, 0);

	bus_space_unmap(iot, ioh, DRIVE_REGISTER_SPACE);
	bus_space_unmap(iot, aux_ioh, 4);
	bus_space_unmap(iot, irq_ioh, 4);
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
wdc_ics_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct icswdc_softc *wdc = (void *)self;
	struct podule_attach_args *pa = (void *)aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_space_handle_t aux_ioh;
	bus_space_handle_t irq_ioh;
	int dummy;

	/* Note the podule number and validate */

	wdc->sc_podule_number = pa->pa_podule_number;
	wdc->sc_podule = pa->pa_podule;

	iot = &icside_bs_tag;
	wdc->sc_iot = iot;

	if (bus_space_map(iot, pa->pa_podule->mod_base
	    + DRIVE_REGISTERS_OFFSET, DRIVE_REGISTER_SPACE, 0, &ioh))
		panic("%s: Cannot map IO\n", self->dv_xname);

	if (bus_space_map(iot, pa->pa_podule->mod_base +
	    AUX_STATUS_REGISTER_OFFSET, 4, 0, &aux_ioh))
		panic("%s: Cannot map IO\n", self->dv_xname);

	if (bus_space_map(iot, pa->pa_podule->mod_base +
	    IRQ_ENABLE_REGISTER_OFFSET, 4, 0, &irq_ioh))
		panic("%s: Cannot map IO\n", self->dv_xname);

	wdc->sc_irq_ioh = irq_ioh;

	/* Disable interrupts */
	dummy = bus_space_read_1(iot, irq_ioh, 0);

	wdcattach_internal((struct wdc_softc *)wdc, iot, ioh, aux_ioh, ioh, -1, -1);

	wdc->sc_podule->irq_addr = pa->pa_podule->mod_base
	    + IRQ_STATUS_REGISTER_OFFSET;
	wdc->sc_podule->irq_mask = IRQ_STATUS_REGISTER_MASK;

  	wdc->sc_ih.ih_func = wdc_ics_intr;
   	wdc->sc_ih.ih_arg = wdc;
   	wdc->sc_ih.ih_level = IPL_BIO;
   	wdc->sc_ih.ih_name = "icside";
	wdc->sc_ih.ih_maskaddr = wdc->sc_podule->irq_addr;
	wdc->sc_ih.ih_maskbits = wdc->sc_podule->irq_mask;

	if (irq_claim(IRQ_PODULE, &wdc->sc_ih))
		panic("Cannot claim IRQ %d for %s\n", IRQ_PODULE, self->dv_xname);

	wdc->sc_wdc.sc_inten = wdc_ics_inten;

	/* Disable interrupts */
	dummy = bus_space_read_1(iot, irq_ioh, 0);
}

/*
 * Podule interrupt handler
 *
 * If the interrupt was from our card pass it on to the wdc interrupt handler
 */

int
wdc_ics_intr(arg)
	void *arg;
{
	struct icswdc_softc *wdc = arg;
	int dummy;

	if (!intr_expected) return(0);

	/* XXX - not bus space yet */
	if (ReadByte(wdc->sc_podule->irq_addr) & wdc->sc_podule->irq_mask) {

		/* Disable interrupts */
		dummy = bus_space_read_1(wdc->sc_iot, wdc->sc_irq_ioh, 0);
		intr_expected = 0;
		wdcintr(arg);
	}

	return(0);
}

/*
 * Podule interrupt enable/disable routine
 *
 * Enable or disable interrupts from the card based on the value in
 * the enable field.
 */

void
wdc_ics_inten(wdc, enable)
	struct wdc_softc *wdc;
	int enable;
{
	struct icswdc_softc *iwdc = (struct icswdc_softc *)wdc;

	if (enable) {
		/* Enable interrupts */
		bus_space_write_1(iwdc->sc_iot, iwdc->sc_irq_ioh, 0, 0);
		intr_expected = 1;
	} else {
		/* Disable interrupts */
		bus_space_read_1(iwdc->sc_iot, iwdc->sc_irq_ioh, 0);
		intr_expected = 0;
	}
}
